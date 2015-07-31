
#ifndef __OTSERV_SCHEDULER_H
#define __OTSERV_SCHEDULER_H

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <functional>
#include "otsystem.h"

class Game;

class SchedulerTask
{
public:
    SchedulerTask()
    {
        _eventid = 0;
        _cycle = 0;
    }
    virtual ~SchedulerTask() { };
    // definition to make sure lower cycles end up front
    // in the priority_queue used in the scheduler
    inline bool operator<(const SchedulerTask& other) const
    {
        return getCycle() > other.getCycle();
    }

    virtual void operator()(Game* arg) = 0;

    virtual void setEventId(uint32_t id)
    {
        _eventid = id;
    }

    inline uint32_t getEventId() const
    {
        return _eventid;
    }

    virtual void setTicks(const __int64 ticks)
    {
        _cycle = OTSYS_TIME() + ticks;
    }

    inline __int64 getCycle() const
    {
        return _cycle;
    }

protected:
    uint32_t _eventid;
    __int64 _cycle;
};

class TSchedulerTask : public SchedulerTask
{
public:
    TSchedulerTask(boost::function1<void, Game*> f) : _f(f)
    {
    }

    virtual void operator()(Game* arg)
    {
        _f(arg);
    }

    virtual ~TSchedulerTask() { }

protected:
    boost::function1<void, Game*> _f;
};

SchedulerTask* makeTask(boost::function1<void, Game*> f);
SchedulerTask* makeTask(__int64 ticks, boost::function1<void, Game*> f);


class lessSchedTask : public std::binary_function<SchedulerTask*, SchedulerTask*, bool>
{
public:
    bool operator()(SchedulerTask*& t1, SchedulerTask*& t2)
    {
        return *t1 < *t2;
    }
};
#endif
