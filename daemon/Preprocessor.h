#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <rct/SignalSlot.h>
#include <rct/Buffer.h>
#include <rct/List.h>
#include <rct/String.h>
#include "ProcessPool.h"

class Preprocessor
{
public:
    Preprocessor();
    ~Preprocessor();

    void preprocess(const List<String>& args);

    Signal<std::function<void(Buffer&&)> >& preprocessed() { return mPreprocessed; }
    Signal<std::function<void(const String&)> >& error() { return mError; }

private:
    Signal<std::function<void(Buffer&&)> > mPreprocessed;
    Signal<std::function<void(const String&)> > mError;

private:
    ProcessPool* mPool
};

#endif
