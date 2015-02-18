#include "Job.h"
#include "CompilerArgs.h"

Job::Job(const List<String>& args)
{
    mArgs = CompilerArgs::create(args);
}

Job::~Job()
{
}

void Job::start()
{
}
