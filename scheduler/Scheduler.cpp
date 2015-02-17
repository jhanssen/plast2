#include <Scheduler.h>

Scheduler::WeakPtr Scheduler::sInstance;

Scheduler::Scheduler()
{
}

Scheduler::~Scheduler()
{
}

void Scheduler::init()
{
    sInstance = shared_from_this();
}
