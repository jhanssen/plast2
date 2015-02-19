#ifndef REMOTE_H
#define REMOTE_H

#include "Job.h"
#include "Preprocessor.h"
#include <rct/Hash.h>
#include <rct/Map.h>
#include <rct/List.h>
#include <rct/Connection.h>
#include <rct/SocketClient.h>
#include <rct/SocketServer.h>
#include <Messages.h>

class Remote
{
public:
    Remote();
    ~Remote();

    void init();

    void post(const Job::SharedPtr& job);

private:
    Connection* addClient(const SocketClient::SharedPtr& client);
    void handleJobMessage(const JobMessage::SharedPtr& msg, Connection* conn);
    void handleHasJobsMessage(const HasJobsMessage::SharedPtr& msg, Connection* conn);
    void handleRequestJobsMessage(const RequestJobsMessage::SharedPtr& msg, Connection* conn);
    void handleHandshakeMessage(const HandshakeMessage::SharedPtr& msg, Connection* conn);
    void handleJobResponseMessage(const JobResponseMessage::SharedPtr& msg, Connection* conn);

private:
    SocketServer mServer;
    Connection mConnection;
    Preprocessor mPreprocessor;
    unsigned int mNextId;

    List<Job::WeakPtr> mPending;
    Hash<unsigned int, Job::WeakPtr> mBuidling;

    struct Peer
    {
        String peer;
        uint16_t port;

        bool operator<(const Peer& other) const
        {
            if (peer < other.peer)
                return true;
            else if (peer > other.peer)
                return false;
            return port < other.port;
        }
    };
    Map<Peer, Connection*> mPeersByKey;
    Hash<Connection*, Peer> mPeersByConn;
};

#endif
