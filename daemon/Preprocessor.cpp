#include "Preprocessor.h"
#include "CompilerArgs.h"
#include <Plast.h>
#include <rct/Process.h>
#include <assert.h>

Preprocessor::Preprocessor()
{
    mPool.readyReadStdOut().connect([this](ProcessPool::Id id, Process* proc) {
            Job::SharedPtr job = mJobs[id].job.lock();
            if (!job)
                return;
            job->mPreprocessed += proc->readAllStdOut();
        });
    mPool.readyReadStdErr().connect([this](ProcessPool::Id id, Process* proc) {
            // throw stderr data away, mark job as having errors
            proc->readAllStdErr();
            mJobs[id].hasError = true;
        });
    mPool.started().connect([this](ProcessPool::Id id, Process*) {
            Job::SharedPtr job = mJobs[id].job.lock();
            if (!job)
                return;
            job->mStatusChanged(job.get(), Job::Preprocessing);
        });
    mPool.finished().connect([this](ProcessPool::Id id, Process* proc) {
            Hash<ProcessPool::Id, Data>::iterator data = mJobs.find(id);
            assert(data != mJobs.end());
            Job::SharedPtr job = data->second.job.lock();
            if (job) {
                if (data->second.hasError) {
                    job->mError = "Got data on stderr for preprocess";
                    job->mStatusChanged(job.get(), Job::Error);
                } else if (job->mPreprocessed.isEmpty()) {
                    job->mError = "Got no data from stdout for preprocess";
                    job->mStatusChanged(job.get(), Job::Error);
                } else {
                    job->mStatusChanged(job.get(), Job::Preprocessed);
                }
            }
            mJobs.erase(data);
        });
    mPool.error().connect([this](ProcessPool::Id id) {
            Hash<ProcessPool::Id, Data>::iterator data = mJobs.find(id);
            assert(data != mJobs.end());
            Job::SharedPtr job = data->second.job.lock();
            if (job) {
                job->mError = "Unable to start job for preprocess";
                job->mStatusChanged(job.get(), Job::Error);
            }
            mJobs.erase(data);
        });
}

Preprocessor::~Preprocessor()
{
}

void Preprocessor::preprocess(const Job::SharedPtr& job)
{
    std::shared_ptr<CompilerArgs> args = job->compilerArgs();
    List<String> cmdline = args->commandLine;
    if (args->flags & CompilerArgs::HasDashO) {
        cmdline[args->objectFileIndex] = "-";
    } else {
        cmdline.push_back("-o");
        cmdline.push_back("-");
    }
    cmdline.push_back("-E");
    const Path compiler = plast::resolveCompiler(cmdline.front());
    cmdline.removeFirst();

    Data data(job);
    const ProcessPool::Id id = mPool.prepare(job->path(), compiler, cmdline);
    mJobs[id] = data;
    mPool.post(id);
}
