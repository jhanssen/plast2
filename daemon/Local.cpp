#include "Local.h"
#include "Daemon.h"
#include "CompilerArgs.h"
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
            const int retcode = proc->returnCode();
            if (retcode != 0) {
                if (retcode < 0) {
                    // this is bad
                    job->mError = "Invalid return code for local compile";
                }
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
            Job::finish(job.get());
        });
}

void Local::post(const Job::SharedPtr& job)
{
    error() << "local post";
    std::shared_ptr<CompilerArgs> args = job->compilerArgs();
    List<String> cmdline = args->commandLine;
    const Path cmd = plast::resolveCompiler(cmdline.front());
    if (cmd.isEmpty()) {
        error() << "Unable to resolve compiler" << cmdline.front();
        job->mError = "Unable to resolve compiler for Local post";
        job->mStatusChanged(job.get(), Job::Error);
        return;
    }

    if (job->type() == Job::RemoteJob) {
        assert(job->isPreprocessed());
        assert(args->sourceFileIndexes.size() == 1);

        String lang;
        if (!(args->flags & CompilerArgs::HasDashX)) {
            // need to add language, see if CompilerArgs already knows
            if (args->flags & CompilerArgs::C) {
                lang = "c";
            } else if (args->flags & CompilerArgs::CPlusPlus) {
                lang = "c++";
            } else if (args->flags & CompilerArgs::ObjectiveC) {
                lang = "objective-c";
            } else if (args->flags & CompilerArgs::ObjectiveCPlusPlus) {
                lang = "objective-c++";
            } else {
                error() << "Unknown language";
                job->mError = "Unknown language for remote job";
                job->mStatusChanged(job.get(), Job::Error);
                return;
            }
        }

        // hack the command line input argument to - and send stuff to stdin
        cmdline[args->sourceFileIndexes[0]] = "-";

        if (args->flags & CompilerArgs::HasDashO) {
            cmdline[args->objectFileIndex] = "-";
        } else {
            cmdline.push_back("-o");
            cmdline.push_back("-");
        }

        cmdline.removeFirst();
        cmdline.prepend(lang);
        cmdline.prepend("-x");
    } else {
        cmdline.removeFirst();
    }

    error() << "Compiler resolved to" << cmd << job->path() << cmdline;
    const ProcessPool::Id id = mPool.prepare(job->path(), cmd, cmdline, List<String>(), job->preprocessed());
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
    args.removeFirst();
    error() << "Compiler resolved to" << cmd << job->path() << args;
    const ProcessPool::Id id = mPool.prepare(job->path(), cmd, args);
    mJobs[id] = job;
    mPool.run(id);
}
