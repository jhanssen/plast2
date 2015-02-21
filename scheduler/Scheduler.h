#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "Peer.h"
#include <Messages.h>
#include <HttpServer.h>
#include <rct/SocketClient.h>
#include <rct/SocketServer.h>
#include <rct/Set.h>
#include <memory>

class Scheduler : public std::enable_shared_from_this<Scheduler>
{
public:
    typedef std::shared_ptr<Scheduler> SharedPtr;
    typedef std::weak_ptr<Scheduler> WeakPtr;

    struct Options
    {
        uint16_t port;
    };

    Scheduler(const Options& opts);
    ~Scheduler();

    void init();

    const Options& options() { return mOpts; }

    static SharedPtr instance();

private:
    void addPeer(const Peer::SharedPtr& peer);

private:
    SocketServer mServer;
    HttpServer mHttpServer;
    Set<Peer::SharedPtr> mPeers;
    Options mOpts;

private:
    static WeakPtr sInstance;
};

inline Scheduler::SharedPtr Scheduler::instance()
{
    return sInstance.lock();
}

#endif
