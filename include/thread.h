// EPOS Thread Abstraction Declarations

#ifndef __thread_h
#define __thread_h

#include <utility/queue.h>
#include <cpu.h>
#include <machine.h>
#include <system.h>
#include <scheduler.h>

extern "C" { void __exit(); }

__BEGIN_SYS

class Thread
{
public:

    enum State {
        RUNNING,    
        READY,
        SUSPENDED,
        WAITING,
        FINISHING
    };
    
    typedef Scheduling_Criteria::Priority Criterion;
	typedef Scheduling_Criteria::Priority Priority;
    enum {
        HIGH    = Criterion::HIGH,
        NORMAL  = Criterion::NORMAL,
		MAIN	= Criterion::MAIN,
        LOW     = Criterion::LOW,
        IDLE    = Criterion::IDLE
    };
    
    struct Configuration {
        Configuration(const State & s       = READY,
                      const Criterion & c   = NORMAL,
                      unsigned int ss       = STACK_SIZE)
            : state(s), criterion(c), stack_size(ss) {
            
        }

        State state;
        Criterion criterion;
        unsigned int stack_size;
    };
    
    typedef Ordered_Queue<Thread, 
                          Criterion, 
                          Scheduler<Thread>::Element> Queue;
    
    template<typename ... Tn>
    Thread(int (* entry)(Tn ...), Tn ... an);
    template<typename ... Tn>
    Thread(const Configuration & conf, 
           int (* entry)(Tn ...), 
           Tn ... an);
    ~Thread();

    const volatile State & state() const { 
        return m_state; 
    }
    const volatile Priority & priority() const { 
        return m_link.rank(); 
    }
    void priority(const Priority & p);
    int join();
    void pass();
    void suspend();
    void resume();
    static Thread * volatile self() { 
        return running();
    }
    static void yield();
    static void exit(int status = 0);
                    
protected:
    static const bool timed = Criterion::timed;
    static const bool preemptive = Criterion::preemptive;
    static const bool reboot = Traits<System>::reboot;
    static const unsigned int QUANTUM = Traits<Thread>::QUANTUM;
    static const unsigned int STACK_SIZE = Traits<Application>::STACK_SIZE;
    typedef CPU::Log_Addr Log_Addr;
    typedef CPU::Context Context;
    
    void constructor_prolog(unsigned int stack_size);
    void constructor_epilog(const Log_Addr & entry, 
                            unsigned int stack_size);
    
    static Thread * volatile running() { 
        return m_scheduler.chosen(); 
    }
    Queue::Element * link() { 
        return &m_link; 
    }
    static void lock() { 
        CPU::int_disable(); 
    }
    static void unlock() { 
        CPU::int_enable(); 
    }
    static bool locked() { 
        return CPU::int_disabled(); 
    }
    const Criterion & criterion() { 
        return m_link.rank();
    }
    static void reschedule();
    static void time_slicer(const IC::Interrupt_Id & interrupt);
    static void dispatch(Thread * prev, Thread * next);
    static int idle();
    static void sleep(Queue *t_queue);		
    static void wakeup(Queue *t_queue);		
	static void wakeup_all(Queue *t_queue);	
	
	char * m_stack;
    Context * volatile m_context;
    volatile State m_state;
    Queue::Element m_link;
	Thread * volatile m_joining;			
    static Scheduler_Timer * m_timer;
    static volatile unsigned int m_num_thread;
    static Scheduler<Thread> m_scheduler;
    static Queue _suspended;

private:
    friend class Init_First;
    friend class System;
    friend class Synchronizer_Common;
    friend class Alarm;
    friend class IA32;
    friend class Scheduler<Thread>;
    
    static void init();
};


template<typename ... Tn>
inline Thread::Thread(int (* entry)(Tn ...), Tn ... an)
    : m_state(READY),m_link(this, NORMAL),m_joining(0)				
{
    constructor_prolog(STACK_SIZE);
    m_context = CPU::init_stack(m_stack + STACK_SIZE,
                                &__exit,
                                entry,
                                an ...);
    constructor_epilog(entry, STACK_SIZE);
}

template<typename ... Tn>
inline Thread::Thread(const Configuration & conf,
                      int (* entry)(Tn ...),
                      Tn ... an)
    : m_state(conf.state),m_link(this,conf.criterion)
{
    constructor_prolog(conf.stack_size);
    m_context = CPU::init_stack(m_stack + conf.stack_size,
                                &__exit,
                                entry,
                                an ...);
    constructor_epilog(entry,conf.stack_size);
}

__END_SYS

#endif

