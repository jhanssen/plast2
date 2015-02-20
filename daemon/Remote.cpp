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
                error() << "remote client connected";
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
    // let's make a job out of this
    Job::SharedPtr job = Job::create(msg->path(), msg->args(), Job::RemoteJob, msg->id(), msg->preprocessed());
    job->statusChanged().connect([conn](Job* job, Job::Status status) {
            error() << "remote job status changed" << job << status;
            switch (status) {
            case Job::Compiled:
                conn->send(JobResponseMessage(JobResponseMessage::Compiled, job->remoteId(), job->objectCode()));
                break;
            case Job::Error:
                conn->send(JobResponseMessage(JobResponseMessage::Error, job->remoteId(), job->error()));
                break;
            default:
                break;
            }
        });
    job->readyReadStdOut().connect([conn](Job* job) {
            error() << "remote job ready stdout";
            conn->send(JobResponseMessage(JobResponseMessage::Stdout, job->remoteId(),
                                          job->readAllStdOut()));
        });
    job->readyReadStdErr().connect([conn](Job* job) {
            error() << "remote job ready stderr";
            conn->send(JobResponseMessage(JobResponseMessage::Stderr, job->remoteId(),
                                          job->readAllStdErr()));
        });
    job->start();
}

void Remote::handleRequestJobsMessage(const RequestJobsMessage::SharedPtr& msg, Connection* conn)
{
    error() << "handle request jobs message";
    // take count jobs
    int rem = msg->count();
    for (;;) {
        if (mPending.empty())
            break;
        Job::SharedPtr job = mPending.front().lock();
        mPending.removeFirst();
        if (job) {
            assert(job->isPreprocessed());
            // send this job to remote;
            error() << "sending job back";
            conn->send(JobMessage(job->path(), job->args(), job->id(), job->preprocessed()));
            if (!--rem)
                break;
        }
    }
}

void Remote::handleHasJobsMessage(const HasJobsMessage::SharedPtr& msg, Connection* conn)
{
    error() << "handle has jobs message";
    // assume we have room for the job for now, make a connection and ask for jobs
    Connection* remoteConn = 0;
    {
        const Peer key = { msg->peer(), msg->port() };
        Map<Peer, Connection*>::const_iterator peer = mPeersByKey.find(key);
        if (peer == mPeersByKey.end()) {
            // make connection
            SocketClient::SharedPtr client = std::make_shared<SocketClient>();
            client->connect(key.peer, key.port);
            Connection* conn = addClient(client);

            mPeersByKey[key] = conn;
            mPeersByConn[conn] = key;
            remoteConn = conn;

            conn->send(HandshakeMessage(Daemon::instance()->options().localPort));
        } else {
            remoteConn = peer->second;
        }
    }
    assert(remoteConn);
    remoteConn->send(RequestJobsMessage(1));
}

void Remote::handleHandshakeMessage(const HandshakeMessage::SharedPtr& msg, Connection* conn)
{
    const Peer key = { conn->client()->peerName(), msg->port() };
    if (mPeersByKey.contains(key)) {
        // drop the connection
        assert(!mPeersByConn.contains(conn));
        conn->finish();
    } else {
        mPeersByKey[key] = conn;
        mPeersByConn[conn] = key;
    }
}

void Remote::handleJobResponseMessage(const JobResponseMessage::SharedPtr& msg, Connection* conn)
{
    error() << "handle job response" << msg->mode() << msg->id();
    Job::SharedPtr job = Job::job(msg->id());
    if (!job) {
        error() << "job not found for response";
        return;
    }
    switch (msg->mode()) {
    case JobResponseMessage::Stdout:
        job->mStdOut += msg->data();
        job->mReadyReadStdOut(job.get());
        break;
    case JobResponseMessage::Stderr:
        job->mStdErr += msg->data();
        job->mReadyReadStdErr(job.get());
        break;
    case JobResponseMessage::Error:
        job->closeFile();
        job->mError = msg->data();
        job->mStatusChanged(job.get(), Job::Error);
        Job::finish(job.get());
        break;
    case JobResponseMessage::Compiled:
        job->appendFile(msg->data());
        job->closeFile();
        job->mStatusChanged(job.get(), Job::Compiled);
        Job::finish(job.get());
        break;
    }
}

Connection* Remote::addClient(const SocketClient::SharedPtr& client)
{
    error() << "remote client added";
    Connection* conn = new Connection(client);
    conn->newMessage().connect([this](const std::shared_ptr<Message>& msg, Connection* conn) {
            switch (msg->messageId()) {
            case JobMessage::MessageId:
                handleJobMessage(std::static_pointer_cast<JobMessage>(msg), conn);
                break;
            case RequestJobsMessage::MessageId:
                handleRequestJobsMessage(std::static_pointer_cast<RequestJobsMessage>(msg), conn);
                break;
            case HandshakeMessage::MessageId:
                handleHandshakeMessage(std::static_pointer_cast<HandshakeMessage>(msg), conn);
                break;
            case JobResponseMessage::MessageId:
                handleJobResponseMessage(std::static_pointer_cast<JobResponseMessage>(msg), conn);
                break;
            default:
                error() << "Unexpected message Remote::addClient" << msg->messageId();
                conn->finish(1);
                break;
            }
        });
    conn->disconnected().connect([this](Connection* conn) {
            conn->disconnected().disconnect();
            EventLoop::deleteLater(conn);

            auto itc = mPeersByConn.find(conn);
            if (itc == mPeersByConn.end())
                return;
            const Peer key = itc->second;
            mPeersByConn.erase(itc);
            assert(mPeersByKey.contains(key));
            mPeersByKey.erase(key);
        });
    return conn;
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
