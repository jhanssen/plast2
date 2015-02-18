#include "Local.h"
#include "Daemon.h"
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
    mPool.error().connect([this](ProcessPool::Id id) {
            Job::SharedPtr job = mJobs[id].lock();
            if (!job)
                return;
            job->mStatusChanged(job.get(), Job::Error);
        });
}

static inline Path resolveCommand(const Path &path)
{
    const String fileName = path.fileName();
    const List<String> paths = String(getenv("PATH")).split(':');
    // error() << fileName;
    for (const auto &p : paths) {
        const Path orig = p + "/" + fileName;
        Path exec = orig;
        // error() << "Trying" << exec;
        if (exec.resolve()) {
            const char *fileName = exec.fileName();
            if (strcmp(fileName, "plastc") && strcmp(fileName, "gcc-rtags-wrapper.sh") && strcmp(fileName, "icecc")) {
                return orig;
            }
        }
    }
    return Path();
}

void Local::post(const Job::SharedPtr& job)
{
    List<String> args = job->args();
    const Path cmd = resolveCommand(args.front());
    if (cmd.isEmpty()) {
        error() << "Unable to resolve compiler" << args.front();
        job->mStatusChanged(job.get(), Job::Error);
        return;
    }
    error() << "Compiler resolved to" << cmd;
    args.removeFirst();
    const ProcessPool::Id id = mPool.run(job->path(), cmd, args);
    if (id) {
        mJobs[id] = job;
    } else {
        job->mStatusChanged(job.get(), Job::Error);
    }
}
