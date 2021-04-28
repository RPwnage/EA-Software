#ifndef _PROCESSSOSX_H_
#define _PROCESSSOSX_H_

#include <QObject>
#include <QProcess>
#include <QThread>
#include <QString>
#include "services/process/IProcess.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Engine
    {
        class IGOController;
    }
}

#if defined(ORIGIN_MAC)
#include <QObject>
#include <QEventLoop>
#include <QList>
#include <QMutex>
#include <QTimer>

namespace Origin
{
    namespace Services
    {
        namespace Exception
        {
            class ExceptionSystem;
        }
        
        ///
        /// Waits (without polling) for a signal with an optional timeout.
        ///
        class ORIGIN_PLUGIN_API SignalWaiter: public QObject
        {
            Q_OBJECT
            
            QEventLoop pauseLoop;
        public:
            SignalWaiter( QObject* object, const char* signal );
            virtual ~SignalWaiter();
            void wait( int msecs = -1 );
        };
        
        class AppProcessWatcher;
        
        /// Initialize the OS-specific process status notification handler.
        ORIGIN_PLUGIN_API void initializeProcessNotificationHandler();
        
        /// Used by AppProcessWatcher to populate the initial list of running processes upon construction.
        ORIGIN_PLUGIN_API void populateCurrentlyRunningProcesses( AppProcessWatcher* watcher );

        /// Launches a process through the Finder (i.e., the former NS Workspace Manager.)
        ORIGIN_PLUGIN_API pid_t launchThroughFinder( QString applicationPath, QStringList args, QStringList environment );
        
        /// Launches a process through NSTask or /usr/bin/open depending on the OS (starting with El Capitan, because
        /// of the System Integrity Protection, the DYLD_INSERT_LIBRARIES doesn't work on system processes)
        ORIGIN_PLUGIN_API pid_t launchTaskWithOpen( QString applicationPath, QStringList args, QStringList environment );
        ORIGIN_PLUGIN_API pid_t launchTaskWithNSTask( QString applicationPath, QStringList args, QStringList environment, Exception::ExceptionSystem* exceptionSystem );

        /// Launches a process through the Finder (i.e., the former NS Workspace Manager.)
        ORIGIN_PLUGIN_API bool terminateThroughFinder( pid_t pid );
        
        /// Launches a process through the Finder (i.e., the former NS Workspace Manager.)
        ORIGIN_PLUGIN_API bool openURL( const char* url );

        ///
        /// Monitors processes running within a certain directory.
        /// Emits signals to indicate when processes within that directory are created and destroyed.
        ///
        /// This design uses both polling and NS Workspace Manager (i.e., Finder) notifications
        /// to observe running processes and compare them to the provided path.
        /// Some processes are seen by the notifications (some only provide the Terminate notification)
        /// but all can be seen by polling.  Unfortunately, polling is expensive, so a hybrid is used here.
        /// An alternative polling mechanism would be to hook into the kernel's BSD subsystem using
        /// the kvm_getprocs() interface.  (That would require root privileges.)
        ///
        class ORIGIN_PLUGIN_API AppProcessWatcher: public QObject
        {
            Q_OBJECT

            friend void populateCurrentlyRunningProcesses( Origin::Services::AppProcessWatcher* );
            
            static void initialize();
            static QList< AppProcessWatcher* > sWatchers;
            static bool sInitialized;

            QString mPath;
            QList< QString > mProcesses;
            bool mPrepopulated;
            QList<SignalWaiter*> mBlockers;
            QMutex mMutex;
            QTimer* mTimer;

        public: // Callbacks used by the operating system hooks.
            static void notifyProcessCreated( pid_t pid, const char* utf8PathName );
            static void notifyProcessDestroyed( pid_t pid, const char* utf8PathName );

        public:
            AppProcessWatcher( const QString& path, QObject* parent = 0 );
            ~AppProcessWatcher();

            bool isRunning() const;
            size_t processCount() const;

            void addProcess( pid_t pid, const QString& path );
            void delProcess( pid_t pid, const QString& path );
            
            void waitUntilFinished( int msecs = -1 );
            void abortWait();

        signals:
            void processCreated( pid_t pid, QString path );
            void processDestroyed( pid_t pid, QString path );
            void finished();
            
        private:
            bool matches( const QString& path );
            void processScanned( pid_t pid, const char* utf8PathName );

        protected slots:
            void scanProcesses();
        };
        

        ///
        /// ProcessOSX is a MacOS X-specific implementation of IProcess.  Because Mac games
        /// typically are bundles, this monitors running processes within the app bundle (if any).
        /// This allows transparent monitoring of all launchers and helper processes detected to
        /// be running from within the same app bundle.
        ///
        class ORIGIN_PLUGIN_API ProcessOSX : public IProcess
        {
            friend class    Origin::Services::IProcess;
            
        Q_OBJECT

            explicit ProcessOSX(const QString& sFullPath, const QString& sCmdLineArgs, const QString& sCurrentDir, 
                            const QString& sAdditionalEnvVars ="", bool bWaitForExit = true, bool bBlockOnWait = false,
                            bool bRunElevated = false, bool bRunProtocol = false, bool bCreateSuspended = false,
                            bool bRunInstallationProxy = false,  bool bKillGameWhenOriginStops = false, QObject *parent = 0);
            virtual ~ProcessOSX();

        public:

            void                start();
            void                startUnmonitored();
            bool                attach(pid_t pid);
            void                resume();
            void                close();
            
            IProcess::ExitStatus exitStatus() const;

            static bool runProtocol(const QString& sFullPath, const QString& sCmdLineArgs, const QString& sCurrentDir);
            static bool removeEnvironmentVar (const QString sEnvironmentVar);

        public Q_SLOTS:
            void terminate();

        protected:
            void                startProcess();
            void                startProcessMonitoring();
            
            bool waitForExit(bool bBlocking, qint32 iTimeout = -1);
            bool isRunning(bool bVerify = false) const;

            QProcessEnvironment mProcessEnvironment;
            AppProcessWatcher mProcessWatcher;
            bool isDeleted;
        };
        
    } // namespace Origin
} // namespace Services

#endif // ORIGIN_MAC


#endif // _PROCESSSOSX_H_
