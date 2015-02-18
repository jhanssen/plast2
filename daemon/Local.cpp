#include "Local.h"
#include <rct/Process.h>
#include <rct/ThreadPool.h>

Local::Local()
    : mPool(ThreadPool::idealThreadCount() + 2)
{
    mPool.readyReadStdOut().connect([this](ProcessPool::Id id, Process* proc) {
            Job::SharedPtr job = mJobs[id].lock();
            if (!job)
                return;
            job->mStdOut += proc->readAllStdOut();
            job->mReadyReadStdOut(job.get());
        });
    mPool.readyReadStdErr().connect([this](ProcessPool::Id id, Process* proc) {
            Job::SharedPtr job = mJobs[id].lock();
            if (!job)
                return;
            job->mStdErr += proc->readAllStdErr();
            job->mReadyReadStdErr(job.get());
        });
    mPool.finished().connect([this](ProcessPool::Id id, Process* proc) {
            Job::SharedPtr job = mJobs[id].lock();
            if (!job)
                return;
            if (proc->returnCode() < 0) {
                // this is bad
                job->mStatusChanged(job.get(), Job::Error);
            } else {
                job->mStatusChanged(job.get(), Job::Complete);
            }
            Job::finish(job.get());
            mJobs.erase(id);
        });
}

Local::~Local()
{
}

void Local::post(const Job::SharedPtr& job)
{
    List<String> args = job->args();
    const String cmd = args.front();
    args.removeFirst();
    const ProcessPool::Id id = mPool.run(job->path(), cmd, args);
    mJobs[id] = job;
}
