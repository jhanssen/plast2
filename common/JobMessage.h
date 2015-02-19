#ifndef JOBMESSAGE_H
#define JOBMESSAGE_H

#include <Plast.h>
#include <rct/List.h>
#include <rct/Message.h>
#include <rct/Path.h>
#include <rct/String.h>
#include <rct/Log.h>
#include <cstdint>

class JobMessage : public Message
{
public:
    typedef std::shared_ptr<JobMessage> SharedPtr;

    enum { MessageId = plast::JobMessageId };

    JobMessage()
        : Message(MessageId), mId(0)
    {
    }
    JobMessage(const Path& path, const List<String>& args, uintptr_t id = 0, const String& pre = String())
        : Message(MessageId), mPath(path), mArgs(args), mId(id), mPreprocessed(pre)
    {
    }

    Path path() const { return mPath; }
    List<String> args() const { return mArgs; }
    String preprocessed() const { return mPreprocessed; }
    uintptr_t id() const { return mId; }

    virtual void encode(Serializer& serializer) const;
    virtual void decode(Deserializer& deserializer);

private:
    Path mPath;
    List<String> mArgs;
    uintptr_t mId;
    String mPreprocessed;
};

inline void JobMessage::encode(Serializer& serializer) const
{
    serializer << mPath << mArgs << mId << mPreprocessed;
}

inline void JobMessage::decode(Deserializer& deserializer)
{
    deserializer >> mPath >> mArgs >> mId >> mPreprocessed;
}

#endif
