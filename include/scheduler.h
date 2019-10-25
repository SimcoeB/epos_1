#ifndef __scheduler_h
#define __scheduler_h

#include <utility/list.h>
#include <cpu.h>
#include <machine.h>

__BEGIN_SYS

namespace Scheduling_Criteria 
{

class Priority
{
public:
    enum {
        HIGH = 0,
        NORMAL = 15,
		MAIN = 22,
        LOW = 31,
        IDLE = (unsigned(1) << (sizeof(int) * 8 - 1)) - 1
    };
    
    Priority(int t_priority = NORMAL)
        : m_priority(t_priority) {
    
    }
    
    operator const volatile int() const volatile {
        return m_priority;
    }
    
    static const bool timed = false;
    static const bool preemptive = true;

protected:
    volatile int m_priority;
};

class RoundRobin : public Priority 
{
public:    
    RoundRobin(int t_priority = NORMAL)
        : Priority(t_priority) {
        
    }
    
    static const bool timed = true;
    static const bool preemptive = true;
};

class FirstInFirstOut : public Priority
{
public:   
 
    FirstInFirstOut(int t_priority = NORMAL)
        : Priority((t_priority == IDLE) ? IDLE : TSC::time_stamp()) {
        
    }
    
    static const bool timed = false;
    static const bool preemptive = false;
};

class RM: public Priority
    {
    public:
    enum {
        MAIN      = 0,
        PERIODIC  = 1,
        APERIODIC = (unsigned(1) << (sizeof(int) * 8 - 1)) -2,
        NORMAL    = APERIODIC,
        IDLE      = (unsigned(1) << (sizeof(int) * 8 - 1)) -1
    };

    static const bool timed = true;
    static const bool preemptive = true;
    static const bool energy_aware = false;

    public:
    RM(int p): Priority(p), _deadline(0) {} // Aperiodic
    RM(const RTC::Microsecond & d): Priority(PERIODIC), _deadline(d) {}

    private:
    RTC::Microsecond _deadline;
    };

class EDF: public Priority
    {
    public:
    enum {
        MAIN      = 0,
        PERIODIC  = 1,
        APERIODIC = (unsigned(1) << (sizeof(int) * 8 - 1)) -2,
        NORMAL    = APERIODIC,
        IDLE      = (unsigned(1) << (sizeof(int) * 8 - 1)) -1
    };

    static const bool timed = false;
    static const bool preemptive = true;
    static const bool energy_aware = false;

    public:
    EDF(int p): Priority(p), _deadline(0) {} // Aperiodic
    EDF(const RTC::Microsecond & d): Priority(d >> 8), _deadline(d) {}

    private:
    RTC::Microsecond _deadline;
    };
};
template<typename T>
class Scheduler : Scheduling_List<T,typename T::Criterion>
{

public:
    typedef Scheduling_List<T,typename T::Criterion> List;
    typedef typename List::Element Element;

    Scheduler() {

    }
    
    unsigned int size() {
        return List::size();
    }  
    
    T * volatile chosen() {
        T * volatile o = List::chosen()->object();
        return o;
    }
 
    void insert(T* t_elem) {
        List::insert(t_elem->link());
    }

    T* remove(T* t_elem) {
        if (List::remove(t_elem->link())) {
            return t_elem;
        } else {
            return 0;
        }
    }

    void suspend(T* t_elem) {
        remove(t_elem);
    }

    void resume(T* t_elem) {
        //kout<<"resume\n";
        insert(t_elem);
    }

    T* choose() {
        T * o =List::choose()->object();
        //kout<<"choose\n";
        return o;
    }

    T* choose_another() {
        T * o =List::choose_another()->object();
        return o;
    }

    T* choose(T* t_elem) {
        T * o =List::choose(t_elem->link())->object();
        return o;
    }
};

__END_SYS

#endif
