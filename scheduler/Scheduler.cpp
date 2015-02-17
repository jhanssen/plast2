#include <Scheduler.h>

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
}

Scheduler::~Scheduler()
{
}

void Scheduler::handleJobMessage(const JobMessage::SharedPtr& msg)
{
    error() << "handle job message!";
}

void Scheduler::addClient(const SocketClient::SharedPtr& client)
{
    Connection* conn = new Connection(client);
    conn->newMessage().connect([this](const std::shared_ptr<Message>& msg, Connection* conn) {
            switch (msg->messageId()) {
            case JobMessage::MessageId:
                handleJobMessage(std::static_pointer_cast<JobMessage>(msg));
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

void Scheduler::init()
{
    sInstance = shared_from_this();
    messages::init();
}
