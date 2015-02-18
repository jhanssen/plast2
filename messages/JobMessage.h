#ifndef JOBMESSAGE_H
#define JOBMESSAGE_H

#include <rct/List.h>
#include <rct/Message.h>
#include <rct/Path.h>
#include <rct/String.h>
#include <rct/Log.h>

class JobMessage : public Message
{
public:
    typedef std::shared_ptr<JobMessage> SharedPtr;

    enum { MessageId = 33 };

    JobMessage() : Message(MessageId) {}
    JobMessage(const Path& path, const List<String>& args) : Message(MessageId), mPath(path), mArgs(args) {}

    Path path() const { return mPath; }
    List<String> args() const { return mArgs; }

    virtual void encode(Serializer& serializer) const;
    virtual void decode(Deserializer& deserializer);

private:
    Path mPath;
    List<String> mArgs;
};

inline void JobMessage::encode(Serializer& serializer) const
{
    serializer << mPath << mArgs;
}

inline void JobMessage::decode(Deserializer& deserializer)
{
    deserializer >> mPath >> mArgs;
}

#endif
