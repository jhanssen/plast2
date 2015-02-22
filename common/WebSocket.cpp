#include "WebSocket.h"
#include <wslay/wslay.h>

WebSocket::WebSocket(const SocketClient::SharedPtr& client)
    : mClient(client)
{
    wslay_event_callbacks callbacks = {
        wslayRecvCallback,
        wslaySendCallback,
        0, // genmask_callback
        0, // on_frame_recv_start_callback
        0, // on_frame_recv_callback
        0, // on_frame_recv_end_callback
        wslayOnMsgRecvCallback
    };
    wslay_event_context_server_init(&mCtx, &callbacks, this);

    client->readyRead().connect([this](const SocketClient::SharedPtr& client, Buffer&& buf) {
            if (buf.isEmpty())
                return;
            mBuffers.push_back(std::move(buf));
            if (wslay_event_recv(mCtx) < 0) {
                // close socket
                client->close();
                mClient.reset();
                mError(this);
            }
        });
    client->disconnected().connect([this](const SocketClient::SharedPtr& client) {
            mClient.reset();
        });
}

WebSocket::~WebSocket()
{
}

ssize_t WebSocket::wslaySendCallback(wslay_event_context* ctx,
                                     const uint8_t* data, size_t len, int /*flags*/,
                                     void* user_data)
{
    WebSocket* socket = static_cast<WebSocket*>(user_data);
    if (!socket->mClient) {
        wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
        return -1;
    }
    if (!socket->mClient->write(data, len)) {
        wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
        return -1;
    }
    return len;
}

ssize_t WebSocket::wslayRecvCallback(wslay_event_context* ctx,
                                     uint8_t* data, size_t len, int /*flags*/,
                                     void* user_data)
{
    // return up to len bytes
    WebSocket* socket = static_cast<WebSocket*>(user_data);
    if (!socket->mClient) {
        wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
        return -1;
    }
    if (socket->mBuffers.isEmpty()) {
        wslay_event_set_error(ctx, WSLAY_ERR_WOULDBLOCK);
        return -1;
    }

    size_t rem = len;
    uint8_t* ptr = data;
    auto it = socket->mBuffers.begin();
    while (rem > 0 && it != socket->mBuffers.end()) {
        Buffer& buf = *it;
        if (rem >= buf.size()) {
            memcpy(ptr, buf.data(), buf.size());
            ptr += buf.size();
            rem -= buf.size();
        } else {
            // read and move
            memcpy(ptr, buf.data(), rem);

            memmove(buf.data(), buf.data() + rem, buf.size() - rem);
            buf.resize(buf.size() - rem);
            return len;
        }
        ++it;
    }
    return ptr - data;
}

void WebSocket::wslayOnMsgRecvCallback(wslay_event_context*,
                                       const wslay_event_on_msg_recv_arg* arg,
                                       void* user_data)
{
    const Message msg(static_cast<Message::Opcode>(arg->opcode),
                      String(reinterpret_cast<const char*>(arg->msg), arg->msg_length),
                      arg->status_code);
    WebSocket* socket = static_cast<WebSocket*>(user_data);
    socket->mMessage(socket, msg);
}

void WebSocket::write(const Message& msg)
{
    const String& data = msg.message();
    wslay_event_msg wmsg = {
        static_cast<uint8_t>(msg.opcode()),
        reinterpret_cast<const uint8_t*>(data.constData()),
        static_cast<size_t>(data.size())
    };
    wslay_event_queue_msg(mCtx, &wmsg);
    wslay_event_send(mCtx);
}
