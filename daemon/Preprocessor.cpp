#include "Preprocessor.h"
#include <rct/Process.h>
#include <assert.h>

Preprocessor::Preprocessor()
    : mPool(10)
{
    mPool.readyReadStdOut().connect([this](ProcessPool::Id id, Process* proc) {
            mBuffers[id].buf += proc->readAllStdOut();
        });
    mPool.readyReadStdErr().connect([this](ProcessPool::Id id, Process* proc) {
            mBuffers[id].hasError = true;
        });
    mPool.finished().connect([this](ProcessPool::Id id, Process* proc) {
            Hash<ProcessPool::Id, Data>::iterator data = mBuffers.find(id);
            assert(data != mBuffers.end());
            if (data->second.hasError) {
                mError(id, "Got data on stderr");
            } else if (data->second.buf.isEmpty()) {
                mError(id, "Got no data");
            } else {
                mPreprocessed(id, std::move(data->second.buf));
            }
            mBuffers.erase(data);
        });
}

Preprocessor::~Preprocessor()
{
}

ProcessPool::Id Preprocessor::preprocess(const Path& command, const List<String>& args)
{
    return mPool.run(command, args);
}
