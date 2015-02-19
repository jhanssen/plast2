#include "Job.h"
#include "CompilerArgs.h"
#include "Local.h"
#include "Daemon.h"

Hash<Job*, Job::SharedPtr> Job::sJobs;

Job::Job(const Path& path, const List<String>& args, Type type, const String& preprocessed)
    : mArgs(args), mPath(path), mPreprocessed(preprocessed), mType(type)
{
    mCompilerArgs = CompilerArgs::create(mArgs);
}

Job::~Job()
{
}

Job::SharedPtr Job::create(const Path& path, const List<String>& args, Type type, const String& preprocessed)
{
    Job::SharedPtr job(new Job(path, args, type, preprocessed));
    sJobs[job.get()] = job;
    return job;
}

void Job::start()
{
    Local& local = Daemon::instance()->local();
    if (mCompilerArgs->mode != CompilerArgs::Compile) {
        assert(mType == LocalJob);
        local.run(shared_from_this());
    } else if (local.isAvailable() || mType == RemoteJob || mCompilerArgs->sourceFileIndexes.size() != 1) {
        local.post(shared_from_this());
    } else {
        assert(mType == LocalJob);
        Daemon::instance()->remote().post(shared_from_this());
    }
}

void Job::finish(Job* job)
{
    sJobs.erase(job);
}

String Job::readAllStdOut()
{
    String ret;
    std::swap(ret, mStdOut);
    return ret;
}

String Job::readAllStdErr()
{
    String ret;
    std::swap(ret, mStdErr);
    return ret;
}
