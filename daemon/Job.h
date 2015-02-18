#ifndef JOB_H
#define JOB_H

#include <rct/List.h>
#include <rct/String.h>
#include <rct/SignalSlot.h>
#include <memory>

class CompilerArgs;

class Job
{
public:
    Job(const List<String>& args);
    ~Job();

    void start();

    enum Status { Starting, Complete, Error };
    Signal<std::function<void(Job*, Status)> >& statusChanged() { return mStatusChanged; }

private:
    Signal<std::function<void(Job*, Status)> > mStatusChanged;

private:
    std::shared_ptr<CompilerArgs> mArgs;
};

#endif
