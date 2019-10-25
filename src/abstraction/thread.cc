
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
    m_num_thread++;
    m_scheduler.insert(this);
    m_stack = new (SYSTEM) char[stack_size];
} 

void Thread::constructor_epilog(const Log_Addr & entry, 
                                unsigned int stack_size)
{
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
    m_scheduler.resume(this);
    if (preemptive) {
        reschedule();
    }
    unlock();
}

void Thread::yield()
{
    lock();
    Thread *prev = running();
    Thread *next = m_scheduler.choose_another();
    dispatch(prev,next);
}

void Thread::exit(int status)
{
    lock();
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
    Thread *prev = running();
    m_scheduler.suspend(prev);
    t_queue->insert(&prev->m_link);
    Thread *next = m_scheduler.chosen();
    dispatch(prev,next);
}

void Thread::wakeup(Queue *t_queue)
{
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
    Thread * prev = running();
    Thread * next = m_scheduler.choose();
    dispatch(prev,next);
}


void Thread::time_slicer(const IC::Interrupt_Id & i)
{
    lock();
    reschedule();
}

void Thread::dispatch(Thread * prev, Thread * next)
{
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
