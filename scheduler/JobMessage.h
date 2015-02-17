#ifndef JOBMESSAGE_H
#define JOBMESSAGE_H

#include <rct/Message.h>

class JobMessage : public Message
{
public:
    typedef std::shared_ptr<JobMessage> SharedPtr;

    enum { MessageId = 32 };

    JobMessage() : Message(MessageId), mCount(0) {}
    JobMessage(int count) : Message(MessageId), mCount(count) {}

    int count() const { return mCount; }

    virtual void encode(Serializer& serializer);
    virtual void decode(Deserializer& deserializer);

private:
    int mCount;
};

inline void JobMessage::encode(Serializer& serializer)
{
    serializer << mCount;
}

inline void JobMessage::decode(Deserializer& deserializer)
{
    deserializer >> mCount;
}

#endif
