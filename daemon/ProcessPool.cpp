#include "ProcessPool.h"
#include <rct/Process.h>
#include <rct/Log.h>

ProcessPool::ProcessPool(int count)
    : mCount(count), mNextId(0)
{
}

ProcessPool::~ProcessPool()
{
}

bool ProcessPool::runProcess(Process*& proc, const Job& job)
{
    static Hash<Process*, Id> ids;
    if (!proc) {
        proc = new Process;
        proc->readyReadStdOut().connect([this](Process* proc) {
                const Id id = ids[proc];
                mReadyReadStdOut(id, proc);
            });
        proc->readyReadStdErr().connect([this](Process* proc) {
                const Id id = ids[proc];
                mReadyReadStdErr(id, proc);
            });
        proc->finished().connect([this](Process* proc) {
                Hash<Process*, Id>::iterator idit = ids.find(proc);
                assert(idit != ids.end());
                const Id id = idit->second;
                ids.erase(idit);
                mFinished(id, proc);
                while (!mPending.isEmpty()) {
                    // take one from the back of mPending if possible
                    if (!runProcess(proc, mPending.front())) {
                        mError(mPending.front().id);
                        mPending.pop_front();
                    } else {
                        mPending.pop_front();
                        return;
                    }
                }
                // make this process available for new jobs
                mAvail.push_back(proc);
            });
    }
    ids[proc] = job.id;
    if (!job.path.isEmpty()) {
        proc->setCwd(job.path);
    }
    return proc->start(job.command, job.arguments, job.environ);
}

ProcessPool::Id ProcessPool::run(const Path& path, const Path &command, const List<String> &arguments, const List<String> &environ)
{
    Id id = ++mNextId;
    if (!id)
        id = ++mNextId;

    Job job = { id, path, command, arguments, environ };
    if (!mAvail.isEmpty()) {
        Process* proc = mAvail.back();
        mAvail.pop_back();
        if (!runProcess(proc, job))
            return 0;
    } else if (mProcs.size() < mCount) {
        mProcs.push_back(0);
        if (!runProcess(mProcs.back(), job))
            return 0;
    } else {
        mPending.push_back(job);
    }
    return id;
}
