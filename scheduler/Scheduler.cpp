#include "Scheduler.h"
#include <rct/Log.h>

Scheduler::WeakPtr Scheduler::sInstance;

Scheduler::Scheduler(const Options& opts)
    : mOpts(opts)
{
    mServer.newConnection().connect([this](SocketServer* server) {
            SocketClient::SharedPtr client;
            for (;;) {
                client = server->nextConnection();
                if (!client)
                    return;
                addPeer(std::make_shared<Peer>(client));
            }
        });
    error() << "listening on" << mOpts.port;
    if (!mServer.listen(mOpts.port)) {
        error() << "couldn't tcp listen";
        abort();
    }

    mHttpServer.listen(8089);
}

Scheduler::~Scheduler()
{
}

void Scheduler::addPeer(const Peer::SharedPtr& peer)
{
    mPeers.insert(peer);
    peer->event().connect([this](const Peer::SharedPtr& peer, Peer::Event event, const Value& value) {
            switch (event) {
            case Peer::JobsAvailable: {
                HasJobsMessage msg(value["count"].toInteger(),
                                   value["port"].toInteger());
                msg.setPeer(value["peer"].toString());
                for (const Peer::SharedPtr& other : mPeers) {
                    if (other != peer) {
                        other->connection()->send(msg);
                    }
                }
                break; }
            case Peer::Disconnected:
                mPeers.erase(peer);
                break;
            }
        });
}

void Scheduler::init()
{
    sInstance = shared_from_this();
    messages::init();
}
