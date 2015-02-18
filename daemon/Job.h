#ifndef JOB_H
#define JOB_H

#include <rct/Hash.h>
#include <rct/List.h>
#include <rct/String.h>
#include <rct/SignalSlot.h>
#include <memory>

class CompilerArgs;

class Job
{
public:
    typedef std::shared_ptr<Job> SharedPtr;
    typedef std::weak_ptr<Job> WeakPtr;

    ~Job();

    static SharedPtr create(const List<String>& args);

    void start();

    enum Status { Starting, Complete, Error };
    Signal<std::function<void(Job*, Status)> >& statusChanged() { return mStatusChanged; }

private:
    Job(const List<String>& args);

private:
    Signal<std::function<void(Job*, Status)> > mStatusChanged;
    std::shared_ptr<CompilerArgs> mArgs;

    static Hash<Job*, SharedPtr> sJobs;
};

#endif
