#ifndef PROCESSPOOL_H
#define PROCESSPOOL_H

#include <rct/List.h>
#include <rct/LinkedList.h>
#include <rct/String.h>
#include <rct/Path.h>
#include <rct/SignalSlot.h>

class Process;

class ProcessPool
{
public:
    ProcessPool(int count);
    ~ProcessPool();

    void run(const Path &command,
             const List<String> &arguments = List<String>(),
             const List<String> &environ = List<String>());

    Signal<std::function<void(Process*)> > &readyReadStdOut() { return mReadyReadStdOut; }
    Signal<std::function<void(Process*)> > &readyReadStdErr() { return mReadyReadStdErr; }
    Signal<std::function<void(Process*)> > &finished() { return mFinished; }

private:
    struct Job
    {
        Path command;
        List<String> arguments, environ;
    };

    void runProcess(Process*& proc, const Job& job);

private:
    int mCount;
    List<Process*> mProcs, mAvail;
    Signal<std::function<void(Process*)> > mReadyReadStdOut, mReadyReadStdErr, mFinished;

    LinkedList<Job> mPending;
};

#endif
