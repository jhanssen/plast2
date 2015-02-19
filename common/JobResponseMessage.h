#ifndef JOBRESPONSEMESSAGE_H
#define JOBRESPONSEMESSAGE_H

#include <Plast.h>
#include <rct/Message.h>

class JobResponseMessage : public Message
{
public:
    typedef std::shared_ptr<JobResponseMessage> SharedPtr;

    enum { MessageId = plast::JobResponseMessageId };
    enum Mode { Stdout, Stderr };

    JobResponseMessage() : Message(MessageId), mMode(Stdout) {}
    JobResponseMessage(Mode mode, const String& data) : Message(MessageId), mMode(mode), mData(data) {}

    Mode mode() const { return mMode; }
    String data() const { return mData; }

    virtual void encode(Serializer& serializer) const;
    virtual void decode(Deserializer& deserializer);

private:
    Mode mMode;
    String mData;
};

inline void JobResponseMessage::encode(Serializer& serializer) const
{
    serializer << static_cast<int>(mMode) << mData;
}

inline void JobResponseMessage::decode(Deserializer& deserializer)
{
    int mode;
    deserializer >> mode >> mData;
    mMode = static_cast<Mode>(mode);
}

#endif
