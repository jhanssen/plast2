#include "Job.h"
#include "CompilerArgs.h"
#include "Local.h"
#include "Daemon.h"

Hash<Job*, Job::SharedPtr> Job::sJobs;

Job::Job(const Path& path, const List<String>& args, const String& preprocessed)
    : mArgs(args), mPath(path), mPreprocessed(preprocessed)
{
    mCompilerArgs = CompilerArgs::create(mArgs);
}

Job::~Job()
{
}

Job::SharedPtr Job::create(const Path& path, const List<String>& args, const String& preprocessed)
{
    Job::SharedPtr job(new Job(path, args, preprocessed));
    sJobs[job.get()] = job;
    return job;
}

void Job::start()
{
    Local& local = Daemon::instance()->local();
    if (local.isAvailable())
        local.post(shared_from_this());
    else
        abort();
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
