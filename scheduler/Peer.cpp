#include "Peer.h"
#include <Messages.h>

Peer::Peer(const SocketClient::SharedPtr& client)
    : mConnection(client)
{
    mConnection.newMessage().connect([this](const std::shared_ptr<Message>& msg, Connection* conn) {
            switch (msg->messageId()) {
            case HasJobsMessage::MessageId: {
                const HasJobsMessage::SharedPtr jobsmsg = std::static_pointer_cast<HasJobsMessage>(msg);
                Value value;
                value["port"] = jobsmsg->port();
                value["count"] = jobsmsg->count();
                value["peer"] = conn->client()->peerName();
                mEvent(shared_from_this(), JobsAvailable, value);
                break; }
            default:
                error() << "Unexpected message Scheduler" << msg->messageId();
                conn->finish(1);
                break;
            }
        });
    mConnection.disconnected().connect([this](Connection* conn) {
            conn->disconnected().disconnect();
            mEvent(shared_from_this(), Disconnected, Value());
        });
}

Peer::~Peer()
{
}