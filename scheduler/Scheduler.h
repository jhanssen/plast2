#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <memory>

class Scheduler : public std::enable_shared_from_this<Scheduler>
{
public:
    typedef std::shared_ptr<Scheduler> SharedPtr;
    typedef std::weak_ptr<Scheduler> WeakPtr;

    Scheduler();
    ~Scheduler();

    void init();

    static SharedPtr instance();

private:
    static WeakPtr sInstance;
};

inline Scheduler::SharedPtr Scheduler::instance()
{
    return sInstance.lock();
}

#endif
