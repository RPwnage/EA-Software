#include "SystemService.h"

#include <QApplication>

#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
#include "services/log/LogService.h"
#endif

// Globals used by the Windows service
int g_svcArgC = 0;
char **g_svcArgV = NULL;
SERVICE_STATUS_HANDLE g_statusHandle = NULL;
SERVICE_STATUS g_serviceStatus = { 0 };
HANDLE g_serviceStopEvent = INVALID_HANDLE_VALUE;
QString g_binPath;
QStringList g_serviceArgs;

VOID WINAPI ServiceCtrlHandler(DWORD ctrlCode)
{
    // When the user or the OS needs to control our service, it will send us commands to this function
    switch (ctrlCode)
    {
        case SERVICE_CONTROL_STOP:
            if (g_serviceStatus.dwCurrentState != SERVICE_RUNNING)
            {
                ORIGIN_LOG_WARNING << "Stop requested while service was not running.";
                break;
            }

            ORIGIN_LOG_EVENT << "Stop requested by OS.";

            g_serviceStatus.dwControlsAccepted = 0;
            g_serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            g_serviceStatus.dwCheckPoint = 4;
            g_serviceStatus.dwWin32ExitCode = 0;

            if (SetServiceStatus(g_statusHandle, &g_serviceStatus) == FALSE)
            {
                 ORIGIN_LOG_ERROR << "Failed to set service status with OS. Error: " << GetLastError();
            }

            // Signal the worker thread that we are ready to stop
            SetEvent(g_serviceStopEvent);

            break;
        default:
            //ORIGIN_LOG_EVENT << "Unhandled service control code: " << ctrlCode;
            break;
    }
};

DWORD WINAPI ServiceQtThread(LPVOID lpParam)
{
    ORIGIN_LOG_EVENT << "Qt Event Thread Started";

    QApplication app(g_svcArgC, g_svcArgV);

    ORIGIN_LOG_EVENT << "EscalationServiceWin Starting...";

    // Start the actual escalation service
    Origin::Escalation::EscalationServiceWin service(true, g_serviceArgs, 0);

    if (!service.isRunning())
        return 0;

    return app.exec();
}

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
    ORIGIN_LOG_EVENT << "Service Control Monitor Thread Started";

    // Spawn the Qt thread to do all the work
    HANDLE hQtThread = CreateThread(NULL, 0, ServiceQtThread, NULL, 0, NULL);

    HANDLE hWaitObjects[2];
    hWaitObjects[0] = g_serviceStopEvent; // The stop signal
    hWaitObjects[1] = hQtThread; // The Qt worker thread

    // Wait for the stop event
    do
    {
        DWORD waitResult = WaitForMultipleObjects(2, hWaitObjects, FALSE, 0);
    
        // If the wait timed out, sleep and wait again
        if (waitResult == WAIT_TIMEOUT)
        {
            Sleep(1000);
            continue;
        }

        // The stop event signalled
        if (waitResult == WAIT_OBJECT_0)
        {
            ORIGIN_LOG_EVENT << "Servive Stop Event signalled.";
            break;
        }
        // The Qt (escalation service) thread finished
        else if (waitResult == WAIT_OBJECT_0 + 1)
        {
            ORIGIN_LOG_EVENT << "Qt Thread event loop exited.";
            break;
        }

        ORIGIN_LOG_ERROR << "Unknown wait error.";
        return 1;
    } while (1);

    return ERROR_SUCCESS;
};

VOID WINAPI ServiceMain (DWORD argc, LPTSTR *argv)
{
    // If we got passed in valid ServiceMain args, we need to build an arglist for use by the service
    if (argc > 1)
    {
        // Copy the args
        for (unsigned int c = 0; c < argc; ++c)
        {
            QString qarg = QString::fromWCharArray(argv[c]);

            g_serviceArgs.push_back(qarg);

#ifdef DEBUG
            ORIGIN_LOG_EVENT << "Arg: " << qarg;
#endif
        }
    }

    ORIGIN_LOG_EVENT << "ServiceMain Startup";

    // Register our service event handler with the SCM
    g_statusHandle = RegisterServiceCtrlHandler(ORIGIN_ESCALATION_SERVICE_NAME, ServiceCtrlHandler);

    if (!g_statusHandle)
    {
        ORIGIN_LOG_ERROR << "Unable to register event handler with service control manager. Error: " << GetLastError();
        return;
    }

    // Prepare our struct that we will use to communicate our status with the OS
    ZeroMemory(&g_serviceStatus, sizeof(SERVICE_STATUS));

    // State = START_PENDING
    g_serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_serviceStatus.dwCurrentState = SERVICE_START_PENDING;
    
    if (SetServiceStatus(g_statusHandle, &g_serviceStatus) == FALSE)
    {
        ORIGIN_LOG_ERROR << "Failed to set service status with OS. Error: " << GetLastError();
        return;
    }

    // Create a manual reset event which we will wait on to determine when we want to stop the service
    g_serviceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_serviceStopEvent == NULL)
    {
        // If is not clear why this would ever fail unless the OS were simply out of handles...
        ORIGIN_LOG_ERROR << "Failed to create stop event. Error: " << GetLastError();

        // We failed to create our event, somehow, so stop
        g_serviceStatus.dwCurrentState = SERVICE_STOPPED;
        g_serviceStatus.dwWin32ExitCode = GetLastError();
        g_serviceStatus.dwCheckPoint = 1;

        if (SetServiceStatus(g_statusHandle, &g_serviceStatus) == FALSE)
        {
            ORIGIN_LOG_ERROR << "Failed to set service status with OS. Error: " << GetLastError();
        }
        return;
    }

    // We're running
    ORIGIN_LOG_EVENT << "Service Running";

    // State = RUNNING
    g_serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_serviceStatus.dwCurrentState = SERVICE_RUNNING;
    
    if (SetServiceStatus(g_statusHandle, &g_serviceStatus) == FALSE)
    {
        ORIGIN_LOG_ERROR << "Failed to set service status with OS. Error: " << GetLastError();
        return;
    }

    // Create our service worker thread
    HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

    // Wait for the Service worker thread to finish
    WaitForSingleObject(hThread, INFINITE);

    ORIGIN_LOG_EVENT << "Service Control Monitor Thread finished, shutting down...";

    // State = STOPPED
    g_serviceStatus.dwControlsAccepted = 0;
    g_serviceStatus.dwWin32ExitCode = 0;
    g_serviceStatus.dwCurrentState = SERVICE_STOPPED;
    g_serviceStatus.dwCheckPoint = 3;

    if (SetServiceStatus(g_statusHandle, &g_serviceStatus) == FALSE)
    {
        ORIGIN_LOG_ERROR << "Failed to set service status with OS. Error: " << GetLastError();
        return;
    }

    ORIGIN_LOG_EVENT << "Service stopped";
}