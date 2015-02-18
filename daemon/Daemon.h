#ifndef DAEMON_H
#define DAEMON_H

#include "Local.h"
#include <Messages.h>
#include <rct/SocketClient.h>
#include <rct/SocketServer.h>
#include <rct/Connection.h>
#include <memory>

class Daemon : public std::enable_shared_from_this<Daemon>
{
public:
    typedef std::shared_ptr<Daemon> SharedPtr;
    typedef std::weak_ptr<Daemon> WeakPtr;

    Daemon();
    ~Daemon();

    void init();

    Local& local() { return mLocal; }

    static SharedPtr instance();

private:
    void addClient(const SocketClient::SharedPtr& client);
    void handleJobMessage(const JobMessage::SharedPtr& msg, Connection* conn);
    void handleHasJobsMessage(const HasJobsMessage::SharedPtr& msg, Connection* conn);

private:
    SocketServer mTcpServer, mUnixServer;
    Local mLocal;

private:
    static WeakPtr sInstance;
};

inline Daemon::SharedPtr Daemon::instance()
{
    return sInstance.lock();
}

#endif
