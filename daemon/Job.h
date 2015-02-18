#ifndef JOB_H
#define JOB_H

#include <rct/Hash.h>
#include <rct/List.h>
#include <rct/Path.h>
#include <rct/String.h>
#include <rct/SignalSlot.h>
#include <memory>

class CompilerArgs;

class Job : public std::enable_shared_from_this<Job>
{
public:
    typedef std::shared_ptr<Job> SharedPtr;
    typedef std::weak_ptr<Job> WeakPtr;

    ~Job();

    static SharedPtr create(const Path& path, const List<String>& args, const String& preprocessed = String());

    void start();

    enum Status { Starting, Complete, Error };
    Signal<std::function<void(Job*, Status)> >& statusChanged() { return mStatusChanged; }
    Signal<std::function<void(Job*)> >& readyReadStdOut() { return mReadyReadStdOut; }
    Signal<std::function<void(Job*)> >& readyReadStdErr() { return mReadyReadStdErr; }

    Path path() const { return mPath; }
    String preprocessed() const { return mPreprocessed; }
    List<String> args() const { return mArgs; }
    std::shared_ptr<CompilerArgs> compilerArgs() const { return mCompilerArgs; }

    String readAllStdOut();
    String readAllStdErr();

private:
    Job(const Path& path, const List<String>& args, const String& preprocessed);

    static void finish(Job* job);

private:
    Signal<std::function<void(Job*, Status)> > mStatusChanged;
    Signal<std::function<void(Job*)> > mReadyReadStdOut, mReadyReadStdErr;
    List<String> mArgs;
    std::shared_ptr<CompilerArgs> mCompilerArgs;
    Path mPath;
    String mPreprocessed;
    String mStdOut, mStdErr;

    static Hash<Job*, SharedPtr> sJobs;

    friend class Local;
};

#endif
