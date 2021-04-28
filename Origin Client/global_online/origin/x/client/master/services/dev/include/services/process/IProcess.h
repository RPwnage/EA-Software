#ifndef _IPROCESSINTERFACE_H_
#define _IPROCESSINTERFACE_H_

#include <qglobal.h>
#include <QObject>
#include <QString>
#include <QThread>
#include <QProcess>
#include <QPointer>
#include <QMetaType>
#include "services/debug/DebugService.h"

#include "services/plugin/PluginAPI.h"

#ifdef ORIGIN_PC
typedef uint32_t pid_t;
#endif

namespace Origin
{
    namespace Services
    {
        namespace Exception
        {
            class ExceptionSystem;
        }
        
        /// \class IProcess
        /// \brief Interface for our basic process starting & monitoring class
        class ORIGIN_PLUGIN_API IProcess : public QObject
        {
            friend class ProcessOSX;
            friend class ProcessWin;

            Q_OBJECT
        public:
            enum ProcessError {
                NoError,
                FailedToStart,
                FileNotFound,
                Crashed,
                Timedout,
                RequiresAdminRights,
                UACRequired,
                InvalidPID,
                InvalidHandle,
                UnknownError,
            };

            enum ProcessState {
                NotRunning,
                Starting,
                Running
            };

            enum ExitStatus {
                NormalExit,
                CrashExit,
                NoExit
            };

            static IProcess* createNewProcess(const QString& sFullPath, const QString& sCmdLineArgs, const QString& sCurrentDir, 
                                                const QString& sAdditionalEnvVars ="", bool bWaitForExit = true, bool bBlockOnWait = false,
                                                bool bRunElevated = false, bool bRunProtocol = false, bool bCreateSuspended = false,
                                                bool bRunInstallationProxy = false, bool bKillGameWhenOriginStops = false, QObject *parent = 0);

            //used to launch any protocol like the steam:// one we use in 3PDD games
            static bool runProtocol(const QString& sFullPath, const QString& sCmdLineArgs, const QString& sCurrentDir);

            ///
			/// will check and see if launch var exists, if so, will set it to NULL which will remove it from the enviromentlast time we retrived from the server
            ///
            /// \return a true if environment var existed
            ///
        	static bool removeEnvironmentVar (const QString sEnvironmentVar);

        protected:

            explicit IProcess(const QString& sFullPath, const QString& sCmdLineArgs, const QString& sCurrentDir, 
                            const QString& sAdditionalEnvVars ="", bool bWaitForExit = true, bool bBlockOnWait = false,
                            bool bRunElevated = false, bool bRunProtocol = false, bool bCreateSuspended = false,
                            bool bRunInstallationProxy = false, bool bKillGameWhenOriginStops = false, QObject *parent = 0);

        public:
            virtual void                start() = 0;
            virtual void                startUnmonitored() = 0;
            virtual bool                attach(pid_t pid) = 0;
            virtual void                close() = 0;
            virtual void                resume() = 0;
            Q_PID                       pid() { return mPid; }
            bool                        isElevated() { return mRunElevated; }

            int                                    exitCode() const { return mExitCode; }
            virtual Origin::Services::IProcess::ExitStatus exitStatus() const = 0;
            Origin::Services::IProcess::ProcessError getError() const;
            Origin::Services::IProcess::ProcessState state() const;
            void addEnvironmentVariable(const QString& key, const QString& value);
            void setExceptionSystem(Origin::Services::Exception::ExceptionSystem* exceptionSystem) { mExceptionSystem = exceptionSystem; }

        signals:
            void started();
            void finished(uint32_t pid, int exitCode);
            void finished(uint32_t pid, int exitCode, Origin::Services::IProcess::ExitStatus exitStatus);
            void error(Origin::Services::IProcess::ProcessError error, quint32 systemErrorValue);
            void stateChanged(Origin::Services::IProcess::ProcessState state);
        
        public Q_SLOTS:
            virtual void                terminate() = 0;
        
        protected:
            virtual bool waitForExit(bool bBlocking, qint32 iTimeout = -1) = 0;

            Origin::Services::Exception::ExceptionSystem* mExceptionSystem;
            Origin::Services::IProcess::ProcessState mState;
            Origin::Services::IProcess::ProcessError mError;
            quint32    mSystemErrorValue;     // this value is supposed to hold GetLastError or other OS specific error codes!!! 
            qint32     mExitCode;
            bool       mCrashed;
            QString    mFullPath;
            QString    mCmdLineArgs;
            QString    mCurrentDir;
            QString    mAdditionalEnvVars;
            bool       mRunInstallationProxy;
            bool       mWaitForExit;
            bool       mBlockOnWait;
            bool       mRunElevated;
            bool       mRunProtocol;
            bool       mCreateSuspended;
            bool       mKillGameWhenOriginStops;
            bool       mClosed;
            qint32     mTimeout;
            Q_PID      mPid;

            class ORIGIN_PLUGIN_API WaitForExitAsyncTrampoline : public QThread
            {
            public:
                explicit WaitForExitAsyncTrampoline(IProcess& process) : QThread(&process), mProcess(process)
                {
                    ORIGIN_VERIFY_CONNECT_EX(this, SIGNAL(finished()), this, SLOT(deleteLater()), Qt::QueuedConnection);
                    setObjectName("WaitForExitAsyncTrampoline Thread");
                }

                void run()
                {
                    mProcess.waitForExit(true, mProcess.mTimeout);
                }

            protected:
                IProcess& mProcess;
            };

            // this thread deletes itself when finished so IProcess class would never know when to set the pointer to NULL
            // using QPointer (instead off a bear pointer) to avoid a dangling pointer
            QPointer<WaitForExitAsyncTrampoline> mWaitThread;
            bool    mMonitoringProc;



        };
    }
}

#endif