#ifdef _WIN32
#include <EABase/eabase.h>
#endif
#include <limits>
#include <QDateTime>
#include <QProcess>
#include <QtWidgets/QApplication>

#include "Utilities.h"
#include "SystemService.h"

#include "LocalHost/LocalHost.h"
#include "LocalHost/LocalHostServiceConfig.h"

#include "services/Services.h"
#include "services/escalation/EscalationServiceWin.h"
#include "services/platform/PlatformService.h"

using namespace Origin::Escalation;

int main(int argc, char *argv[])
{
    bool runningAsWindowsService = false;

    ArgsParser args(argc, argv);

    // Must have valid args
    if (!args.valid())
        return 0;

    // Save the binary path
    g_binPath = args.binPath();

    // Change the log file name if we're running in install tool mode to avoid stepping on the 'real' log file
    if (args.toolMode() != ArgsParser::Normal)
    {
        g_logFolder = g_logFolderTool;
    }
    else
    {
        // make this app a singleton
        QSharedMemory sharedMemory;
        sharedMemory.setKey(QS16(ORIGIN_BEACON_SERVICE_NAME));
        if (sharedMemory.attach())
        {
            return 0;
        }
        // create the single instance
        sharedMemory.create(1);
    }

#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
    LogServiceInitManager logSvcMgr(argc, argv);
#endif

    if (IsUserInteractive())
    {
        ORIGIN_LOG_EVENT << "Origin Web Helper Service: Running as normal executable.";
    }
    else
    {
        ORIGIN_LOG_EVENT << "Origin Web Helper Service: Running as service.";
        runningAsWindowsService = true;
    }

    // format major.minor
    OSVERSIONINFOEX  osvi;
    QString sVersion("0.0");

    ZeroMemory( &osvi, sizeof( OSVERSIONINFOEX ) );
    osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );

    if ( ::GetVersionEx( ( OSVERSIONINFO * ) &osvi ) )
    {
        sVersion = QString("%1.%2").arg(osvi.dwMajorVersion).arg(osvi.dwMinorVersion);
    }

    ORIGIN_LOG_EVENT << "OS Version: " << sVersion;

    // If we're not running as a service, make sure the service is registered properly
    if (!runningAsWindowsService)
    {
        bool running = false;
        bool installed = QueryOriginServiceStatus(QS16(ORIGIN_BEACON_SERVICE_NAME), running);

        if (args.toolMode() != ArgsParser::Normal)
        {
            ORIGIN_LOG_EVENT << "Origin Web Helper Service Installed: " << installed << " Running: " << running;

            // Hand it off to the install tool code
            return InstallToolMain(args, installed);
        }

// Re-enable the below if you want automatic self-installation.  This was disabled because it isn't really necessary and created
// several problems for developers when running in dev-opt (release) mode.  This has the side effect of permitting end users
// to remove the Escalation Service (if they know how) and it will not be automatically re-installed until Origin updates again.
#if 0
#ifndef DEBUG
        // We don't want self-installation behavior in DEBUG mode... developers can run OriginClientService.exe /install if they want the service
        // Only for Windows Vista and newer
        if (osvi.dwMajorVersion >= 6)
        {
            if (!installed)
            {
                ORIGIN_LOG_EVENT << "Origin Web Helper Service not installed, self-installing...";

                // Self-install (Run another instance of this executable with the /install argument)
                QStringList selfInstallArgs;
                selfInstallArgs.push_back("/selfinstall");
                QProcess::execute(args.binPath(), selfInstallArgs);
            }
        }
#endif
#endif
    }

    if (runningAsWindowsService)
    {
        ORIGIN_LOG_EVENT << "Origin Web Helper Service starting up...";

        // Preserved for Qt use
        g_svcArgC = argc;
        g_svcArgV = argv;

        // Create a service table entry for the SCM
        SERVICE_TABLE_ENTRY ServiceTable[] = 
        {
            {ORIGIN_BEACON_SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION) ServiceMain},
            {NULL, NULL}
        };

        // Ask the SCM to start the service event dispatcher (ServiceMain)
        if (StartServiceCtrlDispatcher (ServiceTable) == FALSE)
        {
            ORIGIN_LOG_ERROR << "Failed to start Service Control Dispatcher.  LastError: " << GetLastError();

            return GetLastError ();
        }

        return 1;
    }
    else
    {
        QApplication app(argc, argv);

        if (!startLocalhost())
        {
            return 0;
        }

        return app.exec();
    }
}
