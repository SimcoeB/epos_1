#ifndef __periodic_thread_h
#define __periodic_thread_h

#include <utility/handler.h>
#include <thread.h>
#include <alarm.h>
__BEGIN_SYS

class Periodic_Thread: public Thread
{

public:
    Periodic_Thread(int (* entry)(), 
		    const RTC::Microsecond & period,
		    int time = Alarm::INFINITE,
		    const State & state = READY,
		    unsigned int stack_size = STACK_SIZE)
	: Thread(Configuration(state,period,stack_size),entry),
	  _semaphore(0),
	  _handler(&_semaphore),
	  _alarm(period, &_handler, time)
    {}
/*
	template<typename ... Tn>
    Periodic_Thread(int (* entry)(Tn ...), Tn ... an,
		    const RTC::Microsecond & period,
		    int time = Alarm::INFINITE,
		    const State & state = READY,
		    unsigned int stack_size = STACK_SIZE)
	: Thread(entry, state, period, stack_size),
	  _semaphore(0),
	  _handler(&_semaphore),
	  _alarm(period, &_handler, time)
    {
	if((state == READY) || (state == RUNNING)) {
	    m_state = SUSPENDED;
	    resume();
	} else
	    m_state = state;
    }
*/
    static void wait_next() {
		reinterpret_cast<Periodic_Thread *>(self())->_semaphore.p();
    }

private:
    Semaphore _semaphore;
    Semaphore_Handler _handler;
    Alarm _alarm;
};

__END_SYS

#endif
