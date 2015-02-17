#include "ProcessPool.h"
#include <rct/Process.h>

ProcessPool::ProcessPool(int count)
    : mCount(count), mNextId(0)
{
}

ProcessPool::~ProcessPool()
{
}

void ProcessPool::runProcess(Process*& proc, const Job& job)
{
    if (!proc) {
        const Id id = job.id;
        proc = new Process;
        proc->readyReadStdOut().connect([this, id](Process* proc) {
                mReadyReadStdOut(id, proc);
            });
        proc->readyReadStdErr().connect([this, id](Process* proc) {
                mReadyReadStdErr(id, proc);
            });
        proc->finished().connect([this, id](Process* proc) {
                mFinished(id, proc);
                if (!mPending.isEmpty()) {
                    // take one from the back of mPending if possible
                    runProcess(proc, mPending.front());
                    mPending.pop_front();
                } else {
                    // make this process available for new jobs
                    mAvail.push_back(proc);
                }
            });
    }
    proc->start(job.command, job.arguments, job.environ);
}

ProcessPool::Id ProcessPool::run(const Path &command, const List<String> &arguments, const List<String> &environ)
{
    const Id id = ++mNextId;
    Job job = { id, command, arguments, environ };
    if (!mAvail.isEmpty()) {
        Process* proc = mAvail.back();
        mAvail.pop_back();
        runProcess(proc, job);
    } else if (mProcs.size() < mCount) {
        mProcs.push_back(0);
        runProcess(mProcs.back(), job);
    } else {
        mPending.push_back(job);
    }
    return id;
}
