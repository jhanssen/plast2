#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <rct/LinkedList.h>
#include <rct/SocketClient.h>
#include <rct/SignalSlot.h>
#include <cstdint>

struct wslay_event_context;
struct wslay_event_on_msg_recv_arg;

class WebSocket
{
public:
    WebSocket(const SocketClient::SharedPtr& client);
    ~WebSocket();

    class Message
    {
    public:
        enum Opcode {
            ContinuationFrame,
            TextFrame,
            BinaryFrame,
            ConnectionClose,
            Ping,
            Pong
        };

        Message(Opcode opcode, const String& message = String());

        Opcode opcode() const { return mOpcode; }
        String message() const { return mMessage; }

        // set if opcode is ConnectionClose
        uint16_t statusCode() { return mStatusCode; }

    private:
        Message(Opcode opcode, const String& message, uint16_t statusCode);

        Opcode mOpcode;
        String mMessage;
        uint16_t mStatusCode;

        friend class WebSocket;
    };

    void write(const Message& message);

    Signal<std::function<void(WebSocket*, const Message&)> >& message() { return mMessage; }
    Signal<std::function<void(WebSocket*)> >& error() { return mError; }

private:
    static ssize_t wslaySendCallback(wslay_event_context* ctx,
                                     const uint8_t* data, size_t len, int flags,
                                     void* user_data);
    static ssize_t wslayRecvCallback(wslay_event_context* ctx,
                                     uint8_t* data, size_t len, int flags,
                                     void* user_data);
    static void wslayOnMsgRecvCallback(wslay_event_context*,
                                       const wslay_event_on_msg_recv_arg* arg,
                                       void* user_data);

private:
    SocketClient::SharedPtr mClient;
    wslay_event_context* mCtx;
    LinkedList<Buffer> mBuffers;

    Signal<std::function<void(WebSocket*, const Message&)> > mMessage;
    Signal<std::function<void(WebSocket*)> > mError;
};

inline WebSocket::Message::Message(Opcode opcode, const String& message)
    : mOpcode(opcode), mMessage(message), mStatusCode(0)
{
}

inline WebSocket::Message::Message(Opcode opcode, const String& message, uint16_t statusCode)
    : mOpcode(opcode), mMessage(message), mStatusCode(statusCode)
{
}

#endif
