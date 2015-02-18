#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Messages.h>
#include <rct/SocketClient.h>
#include <rct/SocketServer.h>
#include <rct/Set.h>
#include <rct/Connection.h>
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
    void addClient(const SocketClient::SharedPtr& client);
    void handleHasJobsMessage(const HasJobsMessage::SharedPtr& msg, Connection* from);

private:
    SocketServer mServer;
    Set<Connection*> mConns;
    Options mOpts;

private:
    static WeakPtr sInstance;
};

inline Scheduler::SharedPtr Scheduler::instance()
{
    return sInstance.lock();
}

#endif
