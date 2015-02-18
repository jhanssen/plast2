#include "Remote.h"
#include "Daemon.h"
#include <rct/Log.h>

Remote::Remote()
    : mNextId(0)
{
}

Remote::~Remote()
{
}

void Remote::init()
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

    const Daemon::Options& opts = Daemon::instance()->options();
    mPreprocessor.setCount(opts.preprocessCount);

    if (!mServer.listen(opts.localPort)) {
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
    if (!mConnection.connectTcp(opts.serverHost, opts.serverPort)) {
        error("Can't seem to connect to server");
        abort();
    }
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
