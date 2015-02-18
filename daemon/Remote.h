#ifndef REMOTE_H
#define REMOTE_H

#include "Job.h"
#include "Preprocessor.h"
#include <rct/Hash.h>
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

    void post(const Job::SharedPtr& job);

private:
    void addClient(const SocketClient::SharedPtr& client);
    void handleJobMessage(const JobMessage::SharedPtr& msg, Connection* conn);
    void handleHasJobsMessage(const HasJobsMessage::SharedPtr& msg, Connection* conn);

private:
    SocketServer mServer;
    Connection mConnection;
    Preprocessor mPreprocessor;
    unsigned int mNextId;

    List<Job::WeakPtr> mPending;
    Hash<unsigned int, Job::WeakPtr> mPreprocessing, mBuidling;
};

#endif
