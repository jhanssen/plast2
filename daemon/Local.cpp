#include "Local.h"
#include "Daemon.h"
#include <Plast.h>
#include <rct/Process.h>
#include <rct/ThreadPool.h>
#include <rct/Log.h>

Local::Local()
{
}

Local::~Local()
{
}

void Local::init()
{
    mPool.setCount(Daemon::instance()->options().jobCount);
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
    mPool.started().connect([this](ProcessPool::Id id, Process*) {
            Job::SharedPtr job = mJobs[id].lock();
            if (!job)
                return;
            job->mStatusChanged(job.get(), Job::Compiling);
        });
    mPool.finished().connect([this](ProcessPool::Id id, Process* proc) {
            Job::SharedPtr job = mJobs[id].lock();
            mJobs.erase(id);
            if (!job)
                return;
            if (proc->returnCode() < 0) {
                // this is bad
                job->mError = "Invalid return code for local compile";
                job->mStatusChanged(job.get(), Job::Error);
            } else {
                job->mStatusChanged(job.get(), Job::Compiled);
            }
            Job::finish(job.get());
        });
    mPool.error().connect([this](ProcessPool::Id id) {
            Job::SharedPtr job = mJobs[id].lock();
            mJobs.erase(id);
            if (!job)
                return;
            job->mError = "Local compile pool returned error";
            job->mStatusChanged(job.get(), Job::Error);
        });
}

void Local::post(const Job::SharedPtr& job)
{
    error() << "local post";
    List<String> args = job->args();
    const Path cmd = plast::resolveCompiler(args.front());
    if (cmd.isEmpty()) {
        error() << "Unable to resolve compiler" << args.front();
        job->mError = "Unable to resolve compiler for Local post";
        job->mStatusChanged(job.get(), Job::Error);
        return;
    }
    error() << "Compiler resolved to" << cmd;
    args.removeFirst();
    const ProcessPool::Id id = mPool.prepare(job->path(), cmd, args);
    mJobs[id] = job;
    mPool.post(id);
}

void Local::run(const Job::SharedPtr& job)
{
    error() << "local run";
    List<String> args = job->args();
    const Path cmd = plast::resolveCompiler(args.front());
    if (cmd.isEmpty()) {
        error() << "Unable to resolve compiler" << args.front();
        job->mError = "Unable to resolve compiler for Local post";
        job->mStatusChanged(job.get(), Job::Error);
        return;
    }
    error() << "Compiler resolved to" << cmd;
    args.removeFirst();
    const ProcessPool::Id id = mPool.prepare(job->path(), cmd, args);
    mJobs[id] = job;
    mPool.run(id);
}
