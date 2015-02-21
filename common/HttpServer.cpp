#include "HttpServer.h"
#include <rct/Log.h>
#include <string.h>
#include <stdio.h>

HttpServer::HttpServer()
    : mProtocol(Http10), mNextId(0)
{
    mTcpServer.newConnection().connect([this](SocketServer* server) {
            SocketClient::SharedPtr client;
            for (;;) {
                client = server->nextConnection();
                if (!client)
                    return;
                addClient(client);
            }
        });
}

HttpServer::~HttpServer()
{
}

bool HttpServer::listen(uint16_t port, Protocol proto, Mode mode)
{
    mProtocol = proto;
    return mTcpServer.listen(port, mode);
}

bool HttpServer::Data::readFrom(const LinkedList<Buffer>::iterator& startBuffer, unsigned int startPos,
                                const LinkedList<Buffer>::iterator& endBuffer, unsigned int endPos,
                                String& data)
{
    LinkedList<Buffer>::const_iterator end = endBuffer;
    ++end;
    for (LinkedList<Buffer>::iterator it = startBuffer; it != end; ++it) {
        unsigned int sz, pos;
        if (it == startBuffer) {
            if (it == endBuffer) {
                sz = endPos - startPos;
            } else {
                sz = it->size() - endPos;
            }
            pos = startPos;
        } else if (it == endBuffer) {
            sz = endPos;
            pos = 0;
        } else {
            sz = it->size();
            pos = 0;
        }
        data.resize(data.size() + sz);
        char* buf = data.data() + data.size() - sz;
        //::error() << "memcpy" << static_cast<void*>(buf) << it->data() + pos << sz;
        memcpy(buf, it->data() + pos, sz);
    }
    return true;
}

bool HttpServer::Data::readLine(String& data)
{
    // find \r\n, the \r might be at the end of a buffer and the \n at the beginning of the next
    data.clear();
    if (currentBuffer == buffers.end())
        return false;
    bool lastr = false;
    const auto startBuffer = currentBuffer;
    const unsigned int startPos = currentPos;
    for (;;) {
        Buffer& buf = *currentBuffer;
        //::error() << "balle" << startPos << buf.size();
        assert(startPos < buf.size());
        if (lastr) {
            assert(currentPos == 0);
            if (buf.data()[0] == '\n') {
                ++currentPos;
                const auto endBuffer = currentBuffer;
                const unsigned int endPos = currentPos;
                if (currentPos == buf.size()) {
                    currentPos = 0;
                    ++currentBuffer;
                }
                return readFrom(startBuffer, startPos, endBuffer, endPos, data);
            }
        }
        for (; currentPos < buf.size() - 1; ++currentPos) {
            if (buf.data()[currentPos] == '\r' && buf.data()[currentPos + 1] == '\n') {
                currentPos += 2;
                const auto endBuffer = currentBuffer;
                const unsigned int endPos = currentPos;
                if (currentPos == buf.size()) {
                    currentPos = 0;
                    ++currentBuffer;
                }
                //::error() << "reading from" << &*startBuffer << startPos << &*endBuffer << endPos;
                return readFrom(startBuffer, startPos, endBuffer, endPos, data);
            }
        }
        if (buf.data()[buf.size() - 1] == '\r')
            lastr = true;
        ++currentBuffer;
        currentPos = 0;
        if (currentBuffer == buffers.end()) {
            // no luck, reset current to start and try again next time
            currentBuffer = startBuffer;
            currentPos = startPos;
            return false;
        }
    }
    assert(false);
    return false;
}

void HttpServer::Data::discardRead()
{
    for (auto it = buffers.begin(); it != currentBuffer;) {
        buffers.erase(it++);
    }
    if (currentBuffer == buffers.end() || currentPos == 0)
        return;

    // we need to shrink & memmove the front buffer at this point
    assert(currentBuffer == buffers.begin());
    Buffer& buf = *currentBuffer;
    assert(currentPos < buf.size() - 1);
    memmove(buf.data(), buf.data() + currentPos, buf.size() - currentPos);
    buf.resize(buf.size() - currentPos);
    currentPos = 0;
}

void HttpServer::addClient(const SocketClient::SharedPtr& client)
{
    const uint64_t id = ++mNextId;
    mData[id] = { id, 0, client, Data::ReadingStatus, 0 };
    Data& data = mData[id];
    data.currentBuffer = data.buffers.begin();

    client->readyRead().connect([this, &data, id](const SocketClient::SharedPtr& c, Buffer&& buf) {
            if (!buf.isEmpty()) {
                data.buffers.push_back(std::move(buf));
                if (data.currentBuffer == data.buffers.end()) {
                    // set currentBuffer to be the buffer we just added
                    --data.currentBuffer;
                    data.currentPos = 0;
                }
                String line;
                while (data.readLine(line)) {
                    if (data.state == Data::ReadingStatus) {
                        data.request.reset(new Request(this, data.client));
                        if (!data.request->parseStatus(line)) {
                            data.client->close();
                            data.client.reset();
                            mData.erase(id);
                        }
                        data.state = Data::ReadingHeaders;
                    } else if (data.state == Data::ReadingHeaders) {
                        if (line.size() == 2) {
                            // we're done
                            assert(line[0] == '\r' && line[1] == '\n');
                            mRequest(data.request);
                            data.request.reset();
                            data.discardRead();
                            data.state = Data::ReadingBody;
                        } else {
                            if (!data.request->parseHeader(line)) {
                                data.client->close();
                                data.client.reset();
                                mData.erase(id);
                            }
                        }
                    }
                }
            }
        });
    client->disconnected().connect([this, id](const SocketClient::SharedPtr& client) {
            mData.erase(id);
        });
}

void HttpServer::Headers::add(const String& key, const String& value)
{
    mHeaders[key].push_back(value);
}

void HttpServer::Headers::set(const String& key, const List<String>& values)
{
    mHeaders[key] = values;
}

String HttpServer::Headers::value(const String& key) const
{
    const List<String> values = mHeaders[key];
    if (values.isEmpty())
        return String();
    return values.front();
}

List<String> HttpServer::Headers::values(const String& key) const
{
    const auto it = mHeaders.find(key);
    if (it == mHeaders.end())
        return List<String>();
    return it->second;
}

HttpServer::Request::Request(HttpServer* server, const SocketClient::SharedPtr& client)
    : mClient(client), mProtocol(Http10), mMethod(Get), mBody(this), mServer(server)
{
}

bool HttpServer::Request::parseMethod(const String& method)
{
    bool ret = false;
    if (method == "GET") {
        mMethod = Get;
        ret = true;
    } else if (method == "POST") {
        mMethod = Post;
        ret = true;
    }
    return ret;
}

bool HttpServer::Request::parseHttp(const String& http)
{
    unsigned int major, minor;
#warning is this safe?
    if (sscanf(http.constData(), "HTTP/%u.%u", &major, &minor) != 2) {
        return false;
    }
    if (major != 1)
        return false;
    switch (minor) {
    case 0:
        mProtocol = Http10;
        return true;
    case 1:
        mProtocol = Http11;
        return true;
    default:
        break;
    }
    return false;
}

bool HttpServer::Request::parseStatus(const String& line)
{
    int prev = 0;
    enum { ParseMethod, ParsePath, ParseHttp } parsing = ParseMethod;
    const char* find[] = { " ", " ", "\r\n" };
    for (;;) {
        int sp = line.indexOf(find[parsing], prev);
        if (sp == -1) {
            return false;
        }
        const String part = line.mid(prev, sp - prev);
        switch (parsing) {
        case ParseMethod:
            if (!parseMethod(part))
                return false;
            parsing = ParsePath;
            break;
        case ParsePath:
            mPath = part;
            parsing = ParseHttp;
            break;
        case ParseHttp:
            if (!parseHttp(part))
                return false;
            return true;
        }
        prev = sp + 1;
    }
    return false;
}

bool HttpServer::Request::parseHeader(const String& line)
{
    const int eq = line.indexOf(':');
    if (eq < 1)
        return false;
    const String key = line.left(eq);
    const String val = line.mid(eq + 1).trimmed();
    mHeaders.add(key, val);
    return true;
}

static inline const char* statusText(int code)
{
    switch (code) {
    case 100: return "Continue";
    case 101: return "Switching Protocols";
    case 200: return "OK";
    case 201: return "Created";
    case 202: return "Accepted";
    case 203: return "Non-Authoritative Information";
    case 204: return "No Content";
    case 205: return "Reset Content";
    case 206: return "Partial Content";
    case 300: return "Multiple Choices";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 303: return "See Other";
    case 304: return "Not Modified";
    case 305: return "Use Proxy";
    case 307: return "Temporary Redirect";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 402: return "Payment Required";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 406: return "Not Acceptable";
    case 407: return "Proxy Authentication Required";
    case 408: return "Request Timeout";
    case 409: return "Conflict";
    case 410: return "Gone";
    case 411: return "Length Required";
    case 412: return "Precondition Failed";
    case 413: return "Request Entity Too Large";
    case 414: return "Request-URI Too Long";
    case 415: return "Unsupported Media Type";
    case 416: return "Requested Range Not Satisfiable";
    case 417: return "Expectation Failed";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 502: return "Bad Gateway";
    case 503: return "Service Unavailable";
    case 504: return "Gateway Timeout";
    case 505: return "HTTP Version Not Supported";
    default: break;
    }
    return "";
}

void HttpServer::Request::write(const Response& response)
{
    SocketClient::SharedPtr client = mClient.lock();
    if (!client)
        return;

    static int ver[2][2] = { { 1, 0 }, { 1, 1 } };

    String resp = String::format<64>("HTTP/%d.%d %d %s\r\n",
                                     ver[response.mProtocol][0],
                                     ver[response.mProtocol][1],
                                     response.mStatus,
                                     statusText(response.mStatus));
    for (const auto& it : response.mHeaders.headers()) {
        resp += it.first + ": ";
        const auto& values = it.second;
        const size_t vsize = values.size();
        for (size_t i = 0; i < vsize; ++i) {
            resp += values[i];
            if (i + 1 < vsize)
                resp += ',';
            resp += "\r\n";
        }
    }
    resp += "\r\n" + response.mBody;

    client->write(resp);
}

HttpServer::Body::Body(Request* req)
    : mRequest(req)
{
}

bool HttpServer::Body::atEnd() const
{
}

String HttpServer::Body::read(int size)
{
}

String HttpServer::Body::readAll()
{
}
