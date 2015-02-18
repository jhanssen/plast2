#include "Scheduler.h"
#include <rct/Log.h>

Scheduler::WeakPtr Scheduler::sInstance;

Scheduler::Scheduler()
{
    mServer.newConnection().connect([this](SocketServer* server) {
            SocketClient::SharedPtr client;
            for (;;) {
                client = server->nextConnection();
                if (!client)
                    return;
                addClient(client);
            }
        });
    if (!mServer.listen(13290)) {
        error() << "couldn't tcp listen";
        abort();
    }
}

Scheduler::~Scheduler()
{
}

void Scheduler::handleHasJobsMessage(const HasJobsMessage::SharedPtr& msg, Connection* from)
{
    // send to everyone but the source
    HasJobsMessage newmsg(*msg);
    newmsg.setPeer(from->client()->peerName());
    for (Connection* conn : mConns) {
        if (conn != from) {
            conn->send(newmsg);
        }
    }
}

void Scheduler::addClient(const SocketClient::SharedPtr& client)
{
    Connection* conn = new Connection(client);
    conn->newMessage().connect([this](const std::shared_ptr<Message>& msg, Connection* conn) {
            switch (msg->messageId()) {
            case HasJobsMessage::MessageId:
                handleHasJobsMessage(std::static_pointer_cast<HasJobsMessage>(msg), conn);
                break;
            default:
                error() << "Unexpected message" << msg->messageId();
                conn->finish(1);
                break;
            }
        });
    conn->disconnected().connect([this](Connection* conn) {
            conn->disconnected().disconnect();
            EventLoop::deleteLater(conn);
            mConns.erase(conn);
        });
    mConns.insert(conn);
}

void Scheduler::init()
{
    sInstance = shared_from_this();
    messages::init();
}
