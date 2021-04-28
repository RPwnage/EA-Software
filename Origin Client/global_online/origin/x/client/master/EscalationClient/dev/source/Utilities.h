#ifndef ESCALATIONUTILITIES_H
#define ESCALATIONUTILITIES_H

#ifdef _WIN32
#include <EABase/eabase.h>
#endif

#include <QString>
#include <limits>

#define NOMINMAX
#include <shlobj.h>
#include "services/escalation/EscalationServiceWin.h"
using namespace Origin::Escalation;

#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
#include "services/log/LogService.h"

void initLoggingService(int argc, char *argv[]);

extern QString g_logFolder;
extern QString g_logFolderTool;

class LogServiceInitManager
{
public:
    LogServiceInitManager(int argc, char *argv[])
    {
        initLoggingService(argc, argv);
    }
    ~LogServiceInitManager()
    {
        Origin::Services::Logger::Instance()->Shutdown();
    }
};
#endif

class ArgsParser
{
public:
    enum ToolMode { Normal = 0, Install, SelfInstall, Uninstall, ReadConfig };

    ArgsParser(int argc, char *argv[]);

    bool valid()
    {
        return _valid;
    }

    QString binPath()
    {
        return _binPath;
    }

    ToolMode toolMode()
    {
        return _toolMode;
    }

    bool nsisMode()
    {
        return _nsis;
    }

private:
    bool _valid;
    QString _binPath;
    ToolMode _toolMode;
    bool _nsis;
};

int InstallToolMain(ArgsParser& args, bool installed);

#endif // ESCALATIONUTILITIES_H