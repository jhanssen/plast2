#include "Scheduler.h"
#include <WebSocket.h>
#include <rct/Log.h>
#include <string.h>

Scheduler::WeakPtr Scheduler::sInstance;

static inline const char* guessMime(const Path& file)
{
    const char* ext = file.extension();
    if (!ext)
        return "text/plain";
    if (!strcmp(ext, "html"))
        return "text/html";
    if (!strcmp(ext, "txt"))
        return "text/plain";
    if (!strcmp(ext, "js"))
        return "text/javascript";
    if (!strcmp(ext, "jpg"))
        return "image/jpg";
    if (!strcmp(ext, "png"))
        return "image/png";
    return "text/plain";
}

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
            if (req->method() == HttpServer::Request::Get) {
                if (req->headers().has("Upgrade")) {
                    error() << "upgrade?";
                    HttpServer::Response response;
                    if (WebSocket::response(*req, response)) {
                        req->write(response);
                        WebSocket::SharedPtr websocket = std::make_shared<WebSocket>(req->takeSocket());
                        websocket->message().connect([websocket](WebSocket*, const WebSocket::Message& msg) {
                                error() << "got message" << msg.opcode() << msg.message();
                                if (msg.opcode() == WebSocket::Message::TextFrame) {
                                    websocket->write(msg);
                                    EventLoop::eventLoop()->callLater(std::bind([websocket] {
                                                websocket->close();
                                                websocket->message().disconnect();
                                            }));
                                }
                            });
                        return;
                    }
                }

                String file = req->path();
                if (file == "/")
                    file = "/stats.html";
                if (file.contains("..") || !file.startsWith("/")) {
                    // no
                    const String data = "No.";
                    HttpServer::Response response(req->protocol(), 404);
                    response.headers().add("Content-Length", String::number(data.size()));
                    response.headers().add("Content-Type", "text/plain");
                    response.headers().add("Connection", "close");
                    response.setBody(data);
                    req->write(response, HttpServer::Response::Incomplete);
                    req->close();
                } else {
                    // serve file
                    static Path base = Rct::executablePath().parentDir().ensureTrailingSlash();
                    const Path path =  base + "stats" + file;
                    if (path.isFile()) {
                        const String data = path.readAll();
                        HttpServer::Response response(req->protocol(), 200);
                        response.headers().add("Content-Length", String::number(data.size()));
                        response.headers().add("Content-Type", guessMime(file));
                        response.headers().add("Connection", "keep-alive");
                        response.setBody(data);
                        req->write(response);
                    } else {
                        const String data = "Unable to open " + file;
                        HttpServer::Response response(req->protocol(), 404);
                        response.headers().add("Content-Length", String::number(data.size()));
                        response.headers().add("Content-Type", "text/plain");
                        response.headers().add("Connection", "keep-alive");
                        response.setBody(data);
                        req->write(response);
                    }
                }
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
