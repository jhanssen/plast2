#ifndef LOCAL_H
#define LOCAL_H

#include <rct/Hash.h>
#include <rct/SignalSlot.h>
#include "ProcessPool.h"
#include "Job.h"

class Local
{
public:
    Local();
    ~Local();

    bool isAvailable() const { return mPool.isIdle(); }
    void post(const Job::SharedPtr& job);

private:
    ProcessPool mPool;
    Hash<ProcessPool::Id, Job::WeakPtr> mJobs;
};

#endif
