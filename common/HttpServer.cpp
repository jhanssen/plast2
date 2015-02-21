#include "HttpServer.h"
#include <rct/Log.h>

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
    mTcpServer.listen(port, mode);
    mProtocol = proto;
}

static inline unsigned int bufferSize(const LinkedList<Buffer>& buffers)
{
    unsigned int sz = 0;
    for (const Buffer& buffer: buffers) {
        sz += buffer.size();
    }
    return sz;
}

static inline int bufferRead(LinkedList<Buffer>& buffers, char* out, unsigned int size)
{
    if (!size)
        return 0;
    unsigned int num = 0, rem = size, cur;
    LinkedList<Buffer>::iterator it = buffers.begin();
    while (it != buffers.end()) {
        cur = std::min(it->size(), rem);
        memcpy(out + num, it->data(), cur);
        rem -= cur;
        num += cur;
        if (cur == it->size()) {
            // we've read the entire buffer, remove it
            it = buffers.erase(it);
        } else {
            assert(!rem);
            assert(it->size() > cur);
            assert(cur > 0);
            // we need to shrink & memmove the front buffer at this point
            Buffer& front = *it;
            memmove(front.data(), front.data() + cur, front.size() - cur);
            front.resize(front.size() - cur);
        }
        if (!rem) {
            assert(num == size);
            return size;
        }
        assert(rem > 0);
    }
    return num;
}

void HttpServer::makeRequest(const SocketClient::SharedPtr& client, const String& hstr)
{
    Headers headers;
    for (const String& h : hstr.split("\r\n")) {
        const int eq = h.indexOf(':');
        if (eq == -1)
            continue;
        headers.add(h.left(eq), h.mid(eq + 1).trimmed());
    }
    Request::SharedPtr req(new Request(this, client, headers));
    mRequest(req);
}

void HttpServer::addClient(const SocketClient::SharedPtr& client)
{
    const uint64_t id = ++mNextId;
    mData[id] = { id, 0, client, false };
    Data& data = mData[id];

    unsigned char nbuf[4] = { 0, 0, 0, 0 };
    auto rnrn = [&nbuf](unsigned char c) -> bool {
        memmove(nbuf, nbuf + 1, 3);
        nbuf[3] = c;
        return (nbuf[0] == '\r' && nbuf[1] == '\n' && nbuf[2] == '\r' && nbuf[3] == '\n');
    };

    client->readyRead().connect([this, &data, rnrn](const SocketClient::SharedPtr& c, Buffer&& buf) {
            if (!buf.isEmpty()) {
                if (!data.readingBody) {
                    bool done = false;
                    if (!data.buffers.isEmpty()) {
                        // if the end of the last buffer + the start of the new buffer is \r\n\r\n then we're done with headers
                        const Buffer& last = data.buffers.back();
                        const unsigned char* lastData = last.data();
                        {
                            int cnt = last.size();
                            for (int i = std::min<int>(3, last.size()); i > 0; --i) {
                                rnrn(lastData[last.size() - i]);
                            }
                        }
                        for (int i = 0; i < 3; ++i) {
                            if (rnrn(buf.data()[i])) {
                                done = true;
                                break;
                            }
                        }
                        if (done) {
                            // read everything
                            String str;
                            unsigned int sz = bufferSize(data.buffers);
                            str.resize(sz + 1);
                            str[sz] = '\n';
                            bufferRead(data.buffers, str.data(), sz);
                            makeRequest(c, str);

                            // remove the first char of this buffer
                            if (buf.size() > 1) {
                                memmove(buf.data(), buf.data() + 1, buf.size() - 1);
                                buf.resize(buf.size() - 1);
                            } else {
                                // we're done with the buffer
                                return;
                            }
                        }
                    }
                    if (!done) {
                        // see if we can find \n\n in the buffer
                        unsigned char* d = buf.data();
                        for (unsigned int i = 0; i < buf.size() - 3; ++i) {
                            if (d[i] == '\r' && d[i + 1] == '\n' && d[i + 2] == '\r' && d[i + 3] == '\n') {
                                // yes, we're done here
                                const unsigned int sz = bufferSize(data.buffers) + i + 2;
                                data.buffers.push_back(std::move(buf));
                                String str;
                                str.resize(sz);
                                bufferRead(data.buffers, str.data(), sz);
                                makeRequest(c, str);
#warning need to read the body here, should support content-length and chunked transfer encoding
                                return;
                            }
                        }
                    }
                    data.buffers.push_back(std::move(buf));
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

HttpServer::Request::Request(HttpServer* server, const SocketClient::SharedPtr& client,
                             const Headers& headers)
    : mClient(client), mHeaders(headers), mBody(this), mServer(server)
{
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
