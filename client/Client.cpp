#include "Client.h"
#include <Plast.h>
#include <Messages.h>
#include <rct/Log.h>
#include <stdio.h>

Client::Client()
{
    messages::init();
}

Client::~Client()
{
}

bool Client::run(int argc, char** argv)
{
    mConnection.newMessage().connect([](std::shared_ptr<Message> message, Connection*) {
            if (message->messageId() == ResponseMessage::MessageId) {
                std::shared_ptr<ResponseMessage> resp = std::static_pointer_cast<ResponseMessage>(message);
                const String response = resp->data();
                if (!response.isEmpty()) {
                    FILE* f = (resp->type() == ResponseMessage::Stdout ? stdout : stderr);
                    fprintf(f, "%s\n", response.constData());
                    fflush(f);
                }
            } else {
                error("Unexpected message Client: %d", message->messageId());
            }
        });
    mConnection.finished().connect(std::bind([](){ EventLoop::eventLoop()->quit(); }));
    mConnection.disconnected().connect(std::bind([](){ EventLoop::eventLoop()->quit(); }));
    if (!mConnection.connectUnix(plast::defaultSocketFile())) {
        error("Can't seem to connect to server");
        return false;
    }

    List<String> args;
    for (int i = 0; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    mConnection.send(JobMessage(Path::pwd(), args));
    return true;
}
