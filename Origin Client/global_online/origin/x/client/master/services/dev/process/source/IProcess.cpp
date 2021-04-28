#include "services/process/IProcess.h"

#if defined(ORIGIN_PC)
#include "services/process/ProcessWin.h"
#elif defined(ORIGIN_MAC)
#include "services/process/ProcessOSX.h"
#else
#error "Required OS-specific specialization"
#endif

namespace Origin
{
    namespace Services
    {
        Origin::Services::IProcess* Origin::Services::IProcess::createNewProcess(const QString& sFullPath, const QString& sCmdLineArgs, const QString& sCurrentDir,
                                                            const QString& sAdditionalEnvVars/* =""*/, bool bWaitForExit/* = true*/, bool bBlockOnWait /*= false*/,
                                                            bool bRunElevated/* = false*/, bool bRunProtocol /* = false*/, bool bCreateSuspended/* = false*/,
                                                            bool bRunInstallationProxy/* = false*/, bool bKillGameWhenOriginStops/*= false*/,  QObject *parent/* = 0*/)
        {
        #if defined(ORIGIN_PC)
            return new Origin::Services::ProcessWin(sFullPath, sCmdLineArgs, sCurrentDir, 
                                    sAdditionalEnvVars,bWaitForExit,bBlockOnWait,
                                    bRunElevated, bRunProtocol, bCreateSuspended,
                                    bRunInstallationProxy, bKillGameWhenOriginStops, parent);
        #elif defined(ORIGIN_MAC)
            return new Origin::Services::ProcessOSX(sFullPath, sCmdLineArgs, sCurrentDir, 
                                    sAdditionalEnvVars,bWaitForExit,bBlockOnWait,
                                    bRunElevated, bRunProtocol, bCreateSuspended,
                                    bRunInstallationProxy, bKillGameWhenOriginStops, parent);
        #endif
        }


        bool Origin::Services::IProcess::runProtocol(const QString& sFullPath, const QString& sCmdLineArgs, const QString& sCurrentDir)
        {
#if defined(ORIGIN_PC)
            return Origin::Services::ProcessWin::runProtocol(sFullPath, sCmdLineArgs, sCurrentDir);
#elif defined(ORIGIN_MAC)
            return Origin::Services::ProcessOSX::runProtocol(sFullPath, sCmdLineArgs, sCurrentDir);
#else
#error "Required OS-specific specialization"
#endif
        }

        bool Origin::Services::IProcess::removeEnvironmentVar (const QString sEnvironmentVar)
        {
#if defined(ORIGIN_PC)
            return Origin::Services::ProcessWin::removeEnvironmentVar (sEnvironmentVar);
#elif defined(ORIGIN_MAC)
            return Origin::Services::ProcessOSX::removeEnvironmentVar (sEnvironmentVar);
#else
#error "Required OS-specific specialization"
#endif
        }

        void Origin::Services::IProcess::addEnvironmentVariable(const QString& key, const QString& value)
        {
            mAdditionalEnvVars.append(QString("%1%2=%3").arg(QChar(0xFF)).arg(key).arg(value));
        }
        
        Origin::Services::IProcess::IProcess(const QString& sFullPath, const QString& sCmdLineArgs, const QString& sCurrentDir, 
                                            const QString& sAdditionalEnvVars/* =""*/, bool bWaitForExit/* = true*/, bool bBlockOnWait /*= false*/,
                                            bool bRunElevated/* = false*/, bool bRunProtocol /* = false*/, bool bCreateSuspended/* = false*/,
                                            bool bRunInstallationProxy/* = false*/, bool bKillGameWhenOriginStops/* = false*/, QObject *parent/* = 0*/)
                        : QObject(parent),
                        mExceptionSystem(NULL),
						mState(NotRunning),
                        mError(NoError),
						mSystemErrorValue(0),
                        mExitCode(0),
						mCrashed(false),
                        mFullPath(sFullPath),
                        mCmdLineArgs(sCmdLineArgs),
                        mCurrentDir(sCurrentDir),
                        mAdditionalEnvVars(sAdditionalEnvVars),
						mRunInstallationProxy(bRunInstallationProxy),
                        mWaitForExit(bWaitForExit),
                        mBlockOnWait(bBlockOnWait),
						mRunElevated(bRunElevated),
                        mRunProtocol(bRunProtocol),
						mCreateSuspended(bCreateSuspended),
                        mKillGameWhenOriginStops(bKillGameWhenOriginStops),
                        mClosed(false),
                        mTimeout(-1)
                        {
                            qRegisterMetaType<Origin::Services::IProcess::ExitStatus>("Origin::Services::IProcess::ExitStatus");
                            qRegisterMetaType<Origin::Services::IProcess::ProcessState>("Origin::Services::IProcess::ProcessState");
                            qRegisterMetaType<Origin::Services::IProcess::ProcessError>("Origin::Services::IProcess::ProcessError");
                        }

        Origin::Services::IProcess::ProcessError IProcess::getError() const
        {
            return mError;
        }
    }
}