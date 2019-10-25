
#include <machine.h>
#include <thread.h>
#include <scheduler.h>

__BEGIN_UTIL
bool This_Thread::_not_booting;
__END_UTIL

__BEGIN_SYS

Scheduler_Timer * Thread::m_timer;
Scheduler<Thread> Thread::m_scheduler;
volatile unsigned int Thread::m_num_thread = 0;

void Thread::constructor_prolog(unsigned int stack_size)
{
    lock();
    //kout << this << "\n";
    m_num_thread++;
    m_scheduler.insert(this);
    //kout << "new\n";
    m_stack = new (SYSTEM) char[stack_size];
    //kout << "...\n";
} 

void Thread::constructor_epilog(const Log_Addr & entry, 
                                unsigned int stack_size)
{
    //kout << this << "\n";
    if ((m_state != READY) && (m_state != RUNNING)) {
        m_scheduler.suspend(this);
    }
    if (preemptive && (m_state == READY) && (m_link.rank() != IDLE)) {
        reschedule();
    } else {
        unlock();
    }
}

Thread::~Thread()
{
    lock();
    if (m_state != FINISHING) {
        m_scheduler.remove(this);
    }
    if (m_joining) {
        m_joining->resume();
    }
    unlock();
    delete m_stack;
} 

void Thread::priority(const Priority & c)
{
    lock();
    //kout<<"priority\n";
    m_link.rank(Criterion(c));

    if (m_state != RUNNING) {
        m_scheduler.remove(this);
        m_scheduler.insert(this);
    }

    if (preemptive) {
        reschedule();
    }
    
    unlock();
}

int Thread::join()
{
    lock();
    //kout<<"Join\n";
    if (m_state != FINISHING) {
        m_joining = running();
        m_joining->m_state = SUSPENDED;
        m_joining->suspend();
    } else {
        unlock();
    }
    return *reinterpret_cast<int *>(m_stack);
}

void Thread::pass()
{
    //kout<<"pass\n";
    lock();
    Thread * prev = running();
    Thread * next = m_scheduler.choose(this);
    dispatch(prev,next);
    unlock();
}

void Thread::suspend()
{
    if (!locked) {
        lock();
    }
    //kout<<"suspend\n";
    Thread *prev = running();
    prev->m_state = SUSPENDED;
    m_scheduler.suspend(this);
    Thread *next = running();
    dispatch(prev,next);
}

void Thread::resume()
{
    lock();
    m_state = READY;
    //kout<<"resume\n";
    m_scheduler.resume(this);
    if (preemptive) {
        reschedule();
    }
    unlock();
}

void Thread::yield()
{
    lock();
    //kout<<"yield\n";
    Thread *prev = running();
    Thread *next = m_scheduler.choose_another();
    dispatch(prev,next);
}

void Thread::exit(int status)
{
    lock();
    //kout<<"exit\n";
    m_num_thread--;
    Thread *prev = running();
    prev->m_state = FINISHING;
    *reinterpret_cast<int *>(prev->m_stack) = status;
    m_scheduler.remove(prev);
    if(prev->m_joining) {
        prev->m_joining->m_state = READY;
        m_scheduler.resume(prev->m_joining);
        prev->m_joining = 0;
    }
    Thread *next = m_scheduler.choose();
    dispatch(prev,next);
}

void Thread::sleep(Queue *t_queue)
{
    lock();
    //kout<<"sleep\n";
    Thread *prev = running();
    m_scheduler.suspend(prev);
    t_queue->insert(&prev->m_link);
    Thread *next = m_scheduler.chosen();
    dispatch(prev,next);
}

void Thread::wakeup(Queue *t_queue)
{
    //kout<<"wakeup\n";
    if (!t_queue->empty()) {
        Thread *thread = t_queue->remove()->object();
        thread->m_state = READY;
        m_scheduler.resume(thread);
        if(preemptive)
            reschedule();
    } else {
        unlock(); 
    }
}

void Thread::wakeup_all(Queue *t_queue)
{
    //kout<<"wakeup_all\n";
    if (!t_queue->empty()) {
        while (!t_queue->empty()) {
            wakeup(t_queue);
            lock();
        }
    } 
    unlock();
}

void Thread::reschedule()
{
    //kout<<"reschedule\n";
    Thread * prev = running();
    Thread * next = m_scheduler.choose();
    dispatch(prev,next);
}


void Thread::time_slicer(const IC::Interrupt_Id & i)
{
    //kout<<"time_slicern";
    lock();
    reschedule();
}

void Thread::dispatch(Thread * prev, Thread * next)
{
    //kout<<"dispatch\n";
    //kout <<  prev << " " << next << "\n";
    if (prev != next) {
        if (prev->m_state == RUNNING) {
            prev->m_state = READY;
        }
        next->m_state = RUNNING;
        CPU::switch_context(&prev->m_context, next->m_context);
    }
    unlock();
}


int Thread::idle()
{
	while (m_num_thread > 1) {
        //kout<<"idle\n";
	    CPU::int_enable();
	    CPU::halt();
	    if (m_scheduler.size() > 0) {
	    	yield();
	    }
    }
    CPU::int_disable();
	if (reboot) {
		Machine::reboot();
	} else {
		CPU::halt();
	}
    return 0;
}

__END_SYS

__BEGIN_UTIL

unsigned int This_Thread::id()
{
    return _not_booting ? reinterpret_cast<volatile unsigned int>(Thread::self()) : Machine::cpu_id() + 1;
}

__END_UTIL
