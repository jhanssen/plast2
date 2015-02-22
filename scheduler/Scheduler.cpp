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
    mHttpServer.request().connect([](const HttpServer::Request::SharedPtr& req) {
            error() << "got request" << req->protocol() << req->method() << req->path();
            if (req->method() == HttpServer::Request::Post) {
                if (req->headers().value("Expect") == "100-continue") {
                    // send a 100 response
                    error() << "sending a 100 response";
                    HttpServer::Response response(req->protocol(), 100);
                    req->write(response);
                }
                req->body().readyRead().connect([](HttpServer::Body* body) {
                        error() << "body data" << body->read();
                        if (body->done()) {
                            HttpServer::Request::SharedPtr req = body->request();
                            if (!req) {
                                error() << "!NO GOOD";
                                return;
                            }
                            error() << "!DONE body";
                            HttpServer::Response response(req->protocol(), 200);
                            response.headers().add("Content-Length", "9");
                            response.headers().add("Content-Type", "text/plain");
                            response.headers().add("Connection", "keep-alive");
                            response.setBody("blah body");
                            req->write(response);
                        }
                    });
            } else {
                HttpServer::Response response(req->protocol(), 200);
                response.headers().add("Content-Length", "4");
                response.headers().add("Content-Type", "text/plain");
                response.headers().add("Connection", "keep-alive");
                response.setBody("blah");
                req->write(response);
            }
        });
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
