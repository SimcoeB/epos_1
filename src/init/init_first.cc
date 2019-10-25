// EPOS First Thread Initializer

#include <utility/heap.h>
#include <system.h>

#include <thread.h>

extern "C" { void __epos_app_entry(); }

__BEGIN_SYS

class Init_First
{
private:
    typedef CPU::Log_Addr Log_Addr;

public:
    Init_First() {

        if(!Traits<System>::multithread) {
            CPU::int_enable();
            return;
        }
        Thread * main;
        main = new (SYSTEM) Thread(Thread::Configuration(Thread::RUNNING, Thread::MAIN), reinterpret_cast<int (*)()>(__epos_app_entry));
        new (SYSTEM) Thread(Thread::Configuration(Thread::READY, Thread::IDLE), &Thread::idle);
        This_Thread::not_booting();
        main->m_context->load();
    }
};

// Global object "init_first" must be constructed last in the context of the
// OS, for it activates the first application thread (usually main()) 
Init_First init_first;

__END_SYS
