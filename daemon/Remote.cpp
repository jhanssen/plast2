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

    mConnection.newMessage().connect([this](const std::shared_ptr<Message>& message, Connection* conn) {
            switch (message->messageId()) {
            case HasJobsMessage::MessageId:
                handleHasJobsMessage(std::static_pointer_cast<HasJobsMessage>(message), conn);
                break;
            default:
                error("Unexpected message Remote::init: %d", message->messageId());
                break;
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

void Remote::handleRequestJobsMessage(const RequestJobsMessage::SharedPtr& msg, Connection* conn)
{
    error() << "handle request jobs";
}

void Remote::handleHasJobsMessage(const HasJobsMessage::SharedPtr& msg, Connection* conn)
{
    // assume we have room for the job for now, make a connection and ask for jobs
    Connection* remoteConn = 0;
    {
        const Peer key = { msg->peer(), msg->port() };
        Map<Peer, Connection*>::const_iterator peer = mPeers.find(key);
        if (peer == mPeers.end()) {
            // make connection
            Connection* conn = new Connection;
            conn->newMessage().connect([this](const std::shared_ptr<Message>& msg, Connection* conn) {
                });
            auto onerror = [key](Connection* /*conn*/, Remote* remote) {
                remote->mPeers.erase(key);
            };
            conn->error().connect([this, onerror](Connection* conn) { onerror(conn, this); });
            conn->disconnected().connect([this, onerror](Connection* conn) { onerror(conn, this); });
            conn->connectTcp(key.peer, key.port);
            mPeers.insert(key, conn);
            remoteConn = conn;
        } else {
            remoteConn = peer->second;
        }
    }
    assert(remoteConn);
    remoteConn->send(RequestJobsMessage(1));

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
            case RequestJobsMessage::MessageId:
                handleRequestJobsMessage(std::static_pointer_cast<RequestJobsMessage>(msg), conn);
                break;
            default:
                error() << "Unexpected message Remote::addClient" << msg->messageId();
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
    error() << "local post";
    // queue for preprocess if not already done
    if (!job->isPreprocessed()) {
        job->statusChanged().connect([this](Job* job, Job::Status status) {
                if (status == Job::Preprocessed) {
                    error() << "preproc size" << job->preprocessed().size();
                    mPending.push_back(job->shared_from_this());
                    // send a HasJobsMessage to the scheduler
                    mConnection.send(HasJobsMessage(mPending.size(), Daemon::instance()->options().localPort));
                }
            });
        mPreprocessor.preprocess(job);
    } else {
        mPending.push_back(job);
        // send a HasJobsMessage to the scheduler
        mConnection.send(HasJobsMessage(mPending.size(), Daemon::instance()->options().localPort));
    }
}
