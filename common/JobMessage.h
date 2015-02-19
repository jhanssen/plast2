#ifndef JOBMESSAGE_H
#define JOBMESSAGE_H

#include <Plast.h>
#include <rct/List.h>
#include <rct/Message.h>
#include <rct/Path.h>
#include <rct/String.h>
#include <rct/Log.h>

class JobMessage : public Message
{
public:
    typedef std::shared_ptr<JobMessage> SharedPtr;

    enum { MessageId = plast::JobMessageId };

    JobMessage()
        : Message(MessageId)
    {
    }
    JobMessage(const Path& path, const List<String>& args, const String& pre = String())
        : Message(MessageId), mPath(path), mArgs(args), mPreprocessed(pre)
    {
    }

    Path path() const { return mPath; }
    List<String> args() const { return mArgs; }
    String preprocessed() const { return mPreprocessed; }

    virtual void encode(Serializer& serializer) const;
    virtual void decode(Deserializer& deserializer);

private:
    Path mPath;
    List<String> mArgs;
    String mPreprocessed;
};

inline void JobMessage::encode(Serializer& serializer) const
{
    serializer << mPath << mArgs << mPreprocessed;
}

inline void JobMessage::decode(Deserializer& deserializer)
{
    deserializer >> mPath >> mArgs >> mPreprocessed;
}

#endif
