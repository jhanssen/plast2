#include "Job.h"
#include "CompilerArgs.h"

Hash<Job*, Job::SharedPtr> Job::sJobs;

Job::Job(const List<String>& args)
{
    mArgs = CompilerArgs::create(args);
}

Job::~Job()
{
}

Job::SharedPtr Job::create(const List<String>& args)
{
    Job::SharedPtr job(new Job(args));
    sJobs[job.get()] = job;
    return job;
}

void Job::start()
{
}
