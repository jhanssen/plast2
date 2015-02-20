#ifndef JOB_H
#define JOB_H

#include <rct/Hash.h>
#include <rct/List.h>
#include <rct/Path.h>
#include <rct/String.h>
#include <rct/SignalSlot.h>
#include <memory>
#include <cstdint>
#include <stdio.h>

class CompilerArgs;

class Job : public std::enable_shared_from_this<Job>
{
public:
    typedef std::shared_ptr<Job> SharedPtr;
    typedef std::weak_ptr<Job> WeakPtr;

    ~Job();

    enum Type { LocalJob, RemoteJob };

    static SharedPtr create(const Path& path, const List<String>& args, Type type,
                            uintptr_t remoteId = 0, const String& preprocessed = String());
    static SharedPtr job(uintptr_t j);

    void start();

    enum Status { Preprocessing, Preprocessed, Compiling, Compiled, Error };
    Signal<std::function<void(Job*, Status)> >& statusChanged() { return mStatusChanged; }
    Signal<std::function<void(Job*)> >& readyReadStdOut() { return mReadyReadStdOut; }
    Signal<std::function<void(Job*)> >& readyReadStdErr() { return mReadyReadStdErr; }

    bool isPreprocessed() const { return !mPreprocessed.isEmpty(); }
    Path path() const { return mPath; }
    String preprocessed() const { return mPreprocessed; }
    String objectCode() const { return mObjectCode; }
    List<String> args() const { return mArgs; }
    std::shared_ptr<CompilerArgs> compilerArgs() const { return mCompilerArgs; }
    Type type() const { return mType; }

    String readAllStdOut();
    String readAllStdErr();

    String error() const { return mError; }

    uintptr_t id() const { return reinterpret_cast<uintptr_t>(this); }
    uintptr_t remoteId() const { return mRemoteId; }

private:
    Job(const Path& path, const List<String>& args, Type type,
        uintptr_t remoteId, const String& preprocessed);

    void writeFile(const String& data);

    static void finish(Job* job);

private:
    Signal<std::function<void(Job*, Status)> > mStatusChanged;
    Signal<std::function<void(Job*)> > mReadyReadStdOut, mReadyReadStdErr;
    String mError;
    List<String> mArgs;
    std::shared_ptr<CompilerArgs> mCompilerArgs;
    Path mPath;
    uintptr_t mRemoteId;
    String mPreprocessed, mObjectCode;
    String mStdOut, mStdErr;
    Type mType;

    static Hash<uintptr_t, SharedPtr> sJobs;

    friend class Local;
    friend class Preprocessor;
    friend class Remote;
};

#endif
