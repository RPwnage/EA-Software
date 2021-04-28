#ifndef _PROCESSWIN_H_
#define _PROCESSWIN_H_

#include <limits>
#include <QDateTime>
#include <QObject>
#include <QProcess>
#include <QThread>
#include <QString>
#include <QSet>
#include <QMutex>

#include "services/process/IProcess.h"
#include "services/plugin/PluginAPI.h"

#ifdef WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace Origin
{
    namespace Engine
    {
        class IGOController;
    }
}

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API OriginClientServicesMonitoringThread : public QThread
        {
        public:
            OriginClientServicesMonitoringThread(ProcessWin& process);
            ~OriginClientServicesMonitoringThread();
            void setGameStopped(bool stopped );
            void run();

        protected:
            ProcessWin& mProcess;
            bool mGameStopped;
            QMutex mGameStopLock;
        };

        class ORIGIN_PLUGIN_API ProcessList
        {
        public:
            bool isRunning() const { return (gameProcs.size() > 0); }
            void add( qint32 pid );
            void remove( qint32 pid );
            static bool getExeNameFromPID(QString & exeName, qint32 pid);
            static bool getExeNameFromFullPath(QString & exeName, const QString executablePath);
            // We only get a PID, if a process with the specified name has a TCP socket the the current process!
            static DWORD getPIDFromSocket(const QString exeName);
            QSet<qint32> getGameProcs() {return gameProcs;}

        protected:
            static void translateDeviceNameToDriveLetters( TCHAR *pszFilename );
            bool isBrowserProcess( qint32 pid );
            bool getParentProcessPID(quint32 pid, qint32* pParentId);
            bool isBrowserChildProcess( qint32 pid );

        private:
            QSet<qint32> gameProcs;
            QSet<qint32> browserProcs;
        };

        class ORIGIN_PLUGIN_API ProcessWin : public IProcess
        {
            friend class    Origin::Services::IProcess;
            friend class    Origin::Engine::IGOController;
            
            Q_OBJECT
            explicit ProcessWin(const QString& sFullPath, const QString& sCmdLineArgs, const QString& sCurrentDir, 
                            const QString& sAdditionalEnvVars ="", bool bWaitForExit = true, bool bBlockOnWait = false,
                            bool bRunElevated = false, bool bRunProtocol = false, bool bCreateSuspended = false,
                            bool bRunInstallationProxy = false, bool bKillGameWhenOriginStops = false, QObject *parent = 0);
            virtual ~ProcessWin();

        public:

            void                start();
            void                startUnmonitored();
            bool                attach(pid_t pid);
            void                close();  // Does not terminate process but closes outstanding HANDLE
            void                resume();
            Origin::Services::IProcess::ExitStatus exitStatus() const;

            static bool runProtocol(const QString& sFullPath, const QString& sCmdLineArgs, const QString& sCurrentDir);
            static bool removeEnvironmentVar (const QString sEnvironmentVar);

            void        terminateNonEscalation();

        public Q_SLOTS:
            void terminate();

        protected:
            // we do not want a dependency on IGOController, so we use these two functions to split the start() function
            void        startProcess();  // start the process and returns
            void        setupProcessMonitoring();   // after we injected IGO we have enough time to setup our monitoring thread

            bool        waitForExit(bool bBlocking, qint32 iTimeout = -1);
            HANDLE      createProcessElevated(HANDLE* phProcessThread = NULL, DWORD* pPID = NULL, ProcessError *returnErrorStatus = NULL);
            HANDLE      createProcess(HANDLE* phProcessThread = NULL, DWORD* pPID = NULL);
            HANDLE      createProcessProtocol(); // steam://

            void        terminateTask();
            void        terminateTaskHelper(qint32 processId);

            bool        waitForProcessGroup( HANDLE hProcessGroup, HANDLE hProcessThread, qint32 iTimeout);

            void        cleanupJob();
            HANDLE      OpenProcessForMonitoring(DWORD processId);
            void        CreateJobObjectForMonitoring(const QString &jobName = "");

            bool        isRunning(bool bVerify = false) const;

            void        addToEscalationKillList(qint32 processId);
            void        removeFromEscalationKillList(qint32 processId);

            // ProcessWin window search
            HWND                    getProcMainWindow(DWORD PID) const;
            bool                    verifyActivePID(DWORD PID) const;
            static BOOL CALLBACK    getProcMainWindowCallback(HWND hWnd, LPARAM lParam);
            void                    setProcessState(Origin::Services::IProcess::ProcessState state);
            void                    setProcessError(Origin::Services::IProcess::ProcessError state);

            // member vars
            ProcessList mGameProcessList;
            OriginClientServicesMonitoringThread* mOriginClientServicesMonitorThread;
            QString mJobName;
            HANDLE mJob;

        protected slots:
            void onOriginClientServicesMonitoringThreadFinished();

        };
    }
}

#endif // WIN32

#endif
