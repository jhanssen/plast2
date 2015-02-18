#include "Daemon.h"
#include <rct/EventLoop.h>
#include <rct/Log.h>
#include <rct/Config.h>
#include <stdio.h>

static inline bool validate(int c, const char *name, String &err)
{
    if (c < 0) {
        err = String::format<128>("Invalid %s. Must be >= 0", name);
        return false;
    }
    return true;
}

int main(int argc, char** argv)
{
    Rct::findExecutablePath(*argv);

    // const int idealThreadCount = ThreadPool::idealThreadCount();
    // Config::registerOption<int>("job-count", String::format<128>("Job count (defaults to %d)", idealThreadCount), 'j', idealThreadCount,
    //                             [](const int &count, String &err) { return validate(count, "job-count", err); });
    // Config::registerOption<String>("server",
    //                                String::format<128>("Server to connect to. (defaults to port %d if hostname doesn't contain a port)", Plast::DefaultServerPort), 's');
    // Config::registerOption<int>("port", String::format<129>("Use this port, (default %d)", Plast::DefaultDaemonPort),'p', Plast::DefaultDaemonPort,
    //                             [](const int &count, String &err) { return validate(count, "port", err); });
    // const String socketPath = Plast::defaultSocketFile();
    // Config::registerOption<String>("socket",
    //                                String::format<128>("Run daemon with this domain socket. (default %s)", socketPath.constData()),
    //                                'n', socketPath);

    const char *logFile = 0;
    int logLevel = 0;
    unsigned int logFlags = 0;
    Path logPath;
    if (!initLogging(argv[0], LogStderr, logLevel, logPath.constData(), logFlags)) {
        fprintf(stderr, "Can't initialize logging with %d %s 0x%0x\n",
                logLevel, logFile ? logFile : "", logFlags);
        return 1;
    }

    EventLoop::SharedPtr loop(new EventLoop);
    loop->init(EventLoop::MainEventLoop|EventLoop::EnableSigIntHandler);

    Daemon::SharedPtr daemon = std::make_shared<Daemon>();
    daemon->init();

    loop->exec();

    return 0;
}
