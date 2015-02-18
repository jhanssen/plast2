#ifndef HASJOBSMESSAGE_H
#define HASJOBSMESSAGE_H

#include <rct/Message.h>

class HasJobsMessage : public Message
{
public:
    typedef std::shared_ptr<HasJobsMessage> SharedPtr;

    enum { MessageId = 32 };

    HasJobsMessage() : Message(MessageId), mCount(0) {}
    HasJobsMessage(int count) : Message(MessageId), mCount(count) {}

    int count() const { return mCount; }

    virtual void encode(Serializer& serializer);
    virtual void decode(Deserializer& deserializer);

private:
    int mCount;
};

inline void HasJobsMessage::encode(Serializer& serializer)
{
    serializer << mCount;
}

inline void HasJobsMessage::decode(Deserializer& deserializer)
{
    deserializer >> mCount;
}

#endif
