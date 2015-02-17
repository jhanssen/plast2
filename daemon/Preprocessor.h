#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <rct/SignalSlot.h>
#include <rct/List.h>
#include <rct/Hash.h>
#include <rct/Path.h>
#include <rct/String.h>
#include "ProcessPool.h"

class Preprocessor
{
public:
    Preprocessor();
    ~Preprocessor();

    ProcessPool::Id preprocess(const Path& command, const List<String>& args);

    Signal<std::function<void(ProcessPool::Id, String&&)> >& preprocessed() { return mPreprocessed; }
    Signal<std::function<void(ProcessPool::Id, const String&)> >& error() { return mError; }

private:
    Signal<std::function<void(ProcessPool::Id, String&&)> > mPreprocessed;
    Signal<std::function<void(ProcessPool::Id, const String&)> > mError;

private:
    ProcessPool mPool;
    struct Data
    {
        Data() : hasError(false) {}

        String buf;
        bool hasError;
    };
    Hash<ProcessPool::Id, Data> mBuffers;
};

#endif
