#include "Utilities.h"

QString g_logFolder = "/Origin/Logs/OriginClientService_Log";
QString g_logFolderTool = "/Origin/Logs/OriginClientService_InstallTool_Log"; // Used on Windows only for when running in 'install tool' mode

void initLoggingService(int argc, char *argv[])
{
    //	Initiate logging
    TCHAR szPath[MAX_PATH];
    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, szPath))) 
    {
        QString strLogFolder;

        strLogFolder = QString::fromWCharArray(szPath);
        strLogFolder += g_logFolder;
        Origin::Services::Logger::Instance()->Init(strLogFolder, true);

        //	Output the command-line parameters
        QString cmdLine;
        for (int i = 1; i < argc; i++)
        {
            cmdLine += argv[i];
            cmdLine += " ";
        }
        Origin::Services::LoggerFilter::DumpCommandLineToLog("Cmdline Param", cmdLine);
    }
}

ArgsParser::ArgsParser(int argc, char *argv[]) : 
    _valid(false), 
    _toolMode(Normal),
    _nsis(false)
{
    if (argc == 0)
    {
        _valid = false;
        return;
    }

    _valid = true;

    // Get the current executable name
    WCHAR modulePath[MAX_PATH+1];
    ZeroMemory(modulePath, sizeof(modulePath));
    GetModuleFileName(NULL, modulePath, MAX_PATH+1);

    _binPath = QString::fromWCharArray(modulePath);

    for (int c = 1; c < argc; ++c)
    {
        QString arg = QString(argv[c]).trimmed();

        if (arg.compare("/install", Qt::CaseInsensitive) == 0)
        {
            _toolMode = Install;
        }
        else if (arg.compare("/nsisinstall", Qt::CaseInsensitive) == 0)
        {
            _toolMode = Install;
            _nsis = true;
        }
        else if (arg.compare("/selfinstall", Qt::CaseInsensitive) == 0)
        {
            _toolMode = SelfInstall;
        }
        else if (arg.compare("/uninstall", Qt::CaseInsensitive) == 0)
        {
            _toolMode = Uninstall;
        }
        else if (arg.compare("/nsisuninstall", Qt::CaseInsensitive) == 0)
        {
            _toolMode = Uninstall;
            _nsis = true;
        }
        else if (arg.compare("/config", Qt::CaseInsensitive) == 0)
        {
            _toolMode = ReadConfig;
        }
    }
}

int InstallToolMain(ArgsParser& args, bool installed)
{
    if (args.toolMode() == ArgsParser::Install || args.toolMode() == ArgsParser::SelfInstall)
    {
        if (args.toolMode() == ArgsParser::Install)
        {
            ORIGIN_LOG_EVENT << "Install mode.";
        }
        else
        {
            ORIGIN_LOG_EVENT << "Self-Install mode.";
        }

        if (args.nsisMode())
        {
            ORIGIN_LOG_EVENT << "Running from NSIS.";
        }

        if (installed)
        {
            ORIGIN_LOG_EVENT << "Origin Client Service already installed, checking config...";
        }

        if (RegisterService(QS16(ORIGIN_ESCALATION_SERVICE_NAME), args.binPath(), true))
        {
            ORIGIN_LOG_EVENT << "Operation Successful.";
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else if (args.toolMode() == ArgsParser::Uninstall)
    {
        ORIGIN_LOG_EVENT << "Uninstall mode.";

        if (args.nsisMode())
        {
            ORIGIN_LOG_EVENT << "Running from NSIS.";
        }

        if (installed)
        {
            if (UnRegisterService(QS16(ORIGIN_ESCALATION_SERVICE_NAME)))
            {
                ORIGIN_LOG_EVENT << "Operation Successful.";
                return 1;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            ORIGIN_LOG_EVENT << "Origin Client Service not installed.";
            return 0;
        }
    }
    else if (args.toolMode() == ArgsParser::ReadConfig)
    {
        ORIGIN_LOG_EVENT << "Read Config mode.";

        if (QueryOriginServiceConfig(QS16(ORIGIN_ESCALATION_SERVICE_NAME)))
        {
            ORIGIN_LOG_EVENT << "Operation Successful.";
            return 1;
        }
        else
        {
            return 0;
        }
    }
    return 0;
}