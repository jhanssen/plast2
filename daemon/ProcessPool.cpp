#include "ProcessPool.h"
#include <rct/Process.h>

ProcessPool::ProcessPool(int count)
    : mCount(count)
{
}

ProcessPool::~ProcessPool()
{
}

void ProcessPool::runProcess(Process*& proc, const Job& job)
{
    if (!proc) {
        proc = new Process;
        proc->readyReadStdOut().connect([this](Process* proc) {
                mReadyReadStdOut(proc);
            });
        proc->readyReadStdErr().connect([this](Process* proc) {
                mReadyReadStdErr(proc);
            });
        proc->finished().connect([this](Process* proc) {
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

void ProcessPool::run(const Path &command, const List<String> &arguments, const List<String> &environ)
{
    Job job = { command, arguments, environ };
    if (mProcs.size() < mCount) {
        mProcs.push_back(0);
        runProcess(mProcs.back(), job);
    } else if (!mAvail.isEmpty()) {
        Process* proc = mAvail.back();
        mAvail.pop_back();
        runProcess(proc, job);
    } else {
        mPending.push_back(job);
    }
}
