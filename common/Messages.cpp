#include <Messages.h>

namespace messages {

void init()
{
    Message::registerMessage<JobMessage>();
    Message::registerMessage<HasJobsMessage>();
    Message::registerMessage<RequestJobsMessage>();
}

} // namespace messages
