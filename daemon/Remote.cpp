#include "Remote.h"
#include <rct/Log.h>

Remote::Remote()
    : mNextId(0)
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
    if (!mServer.listen(13291)) {
        error() << "Unable to tcp listen";
        abort();
    }

    mConnection.newMessage().connect([](std::shared_ptr<Message> message, Connection*) {
            if (message->messageId() == ResponseMessage::MessageId) {
                std::shared_ptr<ResponseMessage> resp = std::static_pointer_cast<ResponseMessage>(message);
                const String response = resp->data();
                if (!response.isEmpty()) {
                    FILE* f = (resp->type() == ResponseMessage::Stdout ? stdout : stderr);
                    fprintf(f, "%s\n", response.constData());
                    fflush(f);
                }
            } else {
                error("Unexpected message: %d", message->messageId());
            }
        });
    mConnection.finished().connect(std::bind([](){ error() << "server finished connection"; abort(); }));
    mConnection.disconnected().connect(std::bind([](){ error() << "server closed connection"; abort(); }));
    if (!mConnection.connectTcp("127.0.0.1", 13290)) {
        error("Can't seem to connect to server");
        abort();
    }
}

Remote::~Remote()
{
}

void Remote::handleJobMessage(const JobMessage::SharedPtr& msg, Connection* conn)
{
    error() << "handle job message!";
}

void Remote::handleHasJobsMessage(const HasJobsMessage::SharedPtr& msg, Connection* conn)
{
    error() << "handle has jobs message!";
}

void Remote::addClient(const SocketClient::SharedPtr& client)
{
    error() << "client added";
    Connection* conn = new Connection(client);
    conn->newMessage().connect([this](const std::shared_ptr<Message>& msg, Connection* conn) {
            switch (msg->messageId()) {
            case JobMessage::MessageId:
                handleJobMessage(std::static_pointer_cast<JobMessage>(msg), conn);
                break;
            case HasJobsMessage::MessageId:
                handleHasJobsMessage(std::static_pointer_cast<HasJobsMessage>(msg), conn);
                break;
            default:
                error() << "Unexpected message" << msg->messageId();
                conn->finish(1);
                break;
            }
        });
    conn->disconnected().connect([](Connection* conn) {
            conn->disconnected().disconnect();
            EventLoop::deleteLater(conn);
        });
}

void Remote::post(const Job::SharedPtr& job)
{
}
