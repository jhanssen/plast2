#include <Messages.h>
#include <JobMessage.h>

namespace messages {

void init()
{
    Message::registerMessage<JobMessage>();
}

} // namespace messages
