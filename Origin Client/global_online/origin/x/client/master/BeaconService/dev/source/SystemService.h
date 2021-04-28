#ifndef SYSTEMSERVICE_H
#define SYSTEMSERVICE_H

#ifdef _WIN32
#include <EABase/eabase.h>
#endif

#include "services/escalation/EscalationServiceWin.h"

// Service-related entry points
VOID WINAPI ServiceMain (DWORD argc, LPTSTR *argv);

// Globals used by the Windows service
extern int g_svcArgC;
extern char **g_svcArgV;
extern SERVICE_STATUS_HANDLE g_statusHandle;
extern SERVICE_STATUS g_serviceStatus;
extern HANDLE g_serviceStopEvent;
extern QString g_binPath;
extern QStringList g_serviceArgs;

// Origin functionality
bool startLocalhost();
void stopLocalhost();
void checkDeveloperMode();

#endif // SYSTEMSERVICE_H