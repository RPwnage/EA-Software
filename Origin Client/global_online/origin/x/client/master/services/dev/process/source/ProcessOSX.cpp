#include "ProcessOSX.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include "services/downloader/StringHelpers.h"
#include "services/platform/PlatformService.h"
#include "Common.h"

#include <QSet>
#include <QThreadPool>
#include <QElapsedTimer>
#include <QCoreApplication>


#if defined(ORIGIN_MAC)

#include <QTimer>
#include <QMutexLocker>

#include <unistd.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <ApplicationServices/ApplicationServices.h>

namespace Origin {
    namespace Services {
        
        
        
        SignalWaiter::SignalWaiter( QObject* object, const char* signal )
        {
            // Prepare to exit the event loop upon the signal.
            ORIGIN_VERIFY_CONNECT( object, signal, &pauseLoop, SLOT( quit() ) );
        }
        
        SignalWaiter::~SignalWaiter()
        {
            // Ensure the event loop quits.
            pauseLoop.exit();
        }
        
        void SignalWaiter::wait( int msecs )
        {
            // If msecs is positive,
            if ( msecs > 0 )
            {
                // Create a timer to abort the wait after some time period.
                QTimer::singleShot( msecs, &pauseLoop, SLOT( quit() ) );
            }
            
            // Wait for the event loop to exit.
            pauseLoop.exec();
        }
        
        QList< AppProcessWatcher* > AppProcessWatcher::sWatchers;
        bool AppProcessWatcher::sInitialized;
        
        void AppProcessWatcher::initialize()
        {
            // Initialize if we are not initialized.
            if ( ! sInitialized )
                initializeProcessNotificationHandler();
        }
        
        void AppProcessWatcher::notifyProcessCreated( pid_t pid, const char* utf8PathName )
        {
//            ORIGIN_LOG_EVENT << "______GOT_PROCESS_CREATED: " << pid << " Path: " << utf8PathName;

            QString path( QString::fromUtf8(utf8PathName) );
            
            // Iterate over the watchers.
            for (int i = 0; i < sWatchers.size(); ++i)
            {
                // If this process is monitored by this watcher,
                AppProcessWatcher* watcher = sWatchers.at( i );
                QMutexLocker lock( &watcher->mMutex );
                if ( watcher->matches( path ) )
                {
                    // Add the process to that watcher.
                    watcher->addProcess( pid, path );
                    return;
                }
            }
        }
        
        void AppProcessWatcher::notifyProcessDestroyed( pid_t pid, const char* utf8PathName )
        {
//            ORIGIN_LOG_EVENT << "______GOT_PROCESS_DESTROYED: " << pid << " Path: " << utf8PathName;
            
            QString path( QString::fromUtf8(utf8PathName) );
            
            // Iterate over the watchers.
            for (int i = 0; i < sWatchers.size(); ++i)
            {
                // If this process is monitored by this watcher,
                AppProcessWatcher* watcher = sWatchers.at( i );
                QMutexLocker lock( &watcher->mMutex );
                if ( watcher->mProcesses.contains( path ) )
                {
                    // Remove it.
                    watcher->delProcess( pid, path );
                }
            }
        }
        
        void AppProcessWatcher::processScanned( pid_t pid, const char* utf8PathName )
        {
            // Add the path if it matches.
            QString path( QString::fromUtf8(utf8PathName) );
            if ( matches( path ) ) addProcess( pid, path );
        }
        
        AppProcessWatcher::AppProcessWatcher( const QString& path, QObject* parent )
            : QObject( parent )
            , mPath( path )
            , mPrepopulated( false )
            , mTimer( new QTimer( this ) )
        {
//            ORIGIN_LOG_EVENT << "______WATCHING_PATH:" << path;
            initialize();
            sWatchers.push_back( this );
            
            // Scan running processes.
            scanProcesses();
            
            // Periodically rescan running processes.
            // This is required because the Cocoa NS Workspace Manager does not provide
            // all notifications for all processes, so we manually re-scan running processes
            // to ensure we see everything.
            static const int ProcessRescanDelayMsecs = 6 * 1000; // 6 seconds
            ORIGIN_VERIFY_CONNECT( mTimer, SIGNAL( timeout() ), this, SLOT( scanProcesses() ) );
            mTimer->start( ProcessRescanDelayMsecs );
        }
        
        AppProcessWatcher::~AppProcessWatcher()
        {
            // Abort all blocking calls and clean up.
            abortWait();
        }
        
        bool AppProcessWatcher::isRunning() const
        {
            return processCount() > 0;
        }
        
        size_t AppProcessWatcher::processCount() const
        {
            return mProcesses.size();
        }
        
        bool AppProcessWatcher::matches( const QString& path )
        {
            return path.startsWith( mPath );
        }
        
        void AppProcessWatcher::addProcess( pid_t pid, const QString& path )
        {
            // Add this process.
            mProcesses.append( path );
            
            // If we are finshed pre-populating our list,
            if ( mPrepopulated )
            {
                // Notify clients via a signal.
                emit processCreated( pid, path );
            }
//            else
//                ORIGIN_LOG_EVENT << "______WATCHING_PROCESS: " << pid << " Path: " << path;

        }
        
        void AppProcessWatcher::delProcess( pid_t pid, const QString& path )
        {
//            ORIGIN_LOG_EVENT << "______PROCESS_EXITED: " << pid << " Path: " << path;
            // Bail if the process count is already 0.
            if ( processCount() == 0 ) return;

            // Remove this process.
            mProcesses.removeOne( path );
        
            // Notify clients.
            emit processDestroyed( pid, path );
            if ( processCount() == 0 ) emit finished();
        }
        
        void AppProcessWatcher::waitUntilFinished( int msecs )
        {
            if ( processCount() > 0 )
            {
                SignalWaiter* blocker = new SignalWaiter( this, SIGNAL( finished() ) );
                mBlockers.push_back( blocker );
                blocker->wait( msecs );
            }
        }
        
        void AppProcessWatcher::abortWait()
        {
            // Remove all blocking calls.
            while ( ! mBlockers.isEmpty() )
                delete mBlockers.takeFirst();
            
            sWatchers.removeOne( this );
        }
        
        //
        // ProcessOSX implementation.
        // this simple OSX version does not support: bRunElevated, bCreateSuspended, bRunInstallationProxy
        //
        
        QString getMonitorPathForExecutable( QString executablePath )
        {
            // Monitor the executable path it *is* an app bundle path.
            if ( executablePath.endsWith( ".app" ) ) return executablePath;
                
            // Attempt to find the outermost app bundle containing the executable.
            QString appPath;
            int appPathIndex = executablePath.indexOf( ".app/", 0, Qt::CaseInsensitive );
            
            // If the app bundle was found,
            if ( appPathIndex != -1 )
            {
                // Monitor the app bundle path.
                appPathIndex += 5; // account for ".app/"
                appPath = executablePath.left( appPathIndex );
            }
            
            // Otherwise,
            else
            {
                // Attempt to find the directory containing the executable itself.
                appPathIndex = executablePath.lastIndexOf( "/" );
                
                // Monitor that directory if it was found.
                if ( appPathIndex != -1 ) appPath = executablePath.left( appPathIndex );
                
                // Monitor the executable string itself otherwise.
                else appPath = executablePath;
            }
            return appPath;
        }
        
        ProcessOSX::ProcessOSX(
            const QString& sFullPath,
            const QString& sCmdLineArgs,
            const QString& sCurrentDir,
            const QString& sAdditionalEnvVars /*=""*/,
            bool bWaitForExit /*= true*/,
            bool bBlockOnWait /*= false*/,
            bool bRunElevated /*= false*/,
			bool bRunProtocol /*= false*/, 
            bool bCreateSuspended /*= false*/,
            bool bRunInstallationProxy /*= false*/,
            bool bKillGameWhenOriginStops/* = false*/,
            QObject *parent /*= 0*/)
            : IProcess(sFullPath, sCmdLineArgs, sCurrentDir, sAdditionalEnvVars, bWaitForExit, bBlockOnWait, bRunElevated, bRunProtocol, bCreateSuspended, bRunInstallationProxy, bKillGameWhenOriginStops, parent)
            , mProcessEnvironment( QProcessEnvironment::systemEnvironment() )
            , mProcessWatcher( getMonitorPathForExecutable( sFullPath ) )
            , isDeleted( false )
        {
            // Initialize base data members.
            mPid = 0;
            mWaitThread = 0;
        }

        ProcessOSX::~ProcessOSX()
        {
            // Set the deleted flag.
            isDeleted = true;
            
            // Unblocks any calls to AppProcessWatcher::waitUntilFinished().
            mProcessWatcher.abortWait();
        }

        void ProcessOSX::start()
        {
            startProcess();
            startProcessMonitoring();
        }
        
        void ProcessOSX::startUnmonitored()
        {
            startProcess();
        }
        
        /// Allocates a char array using operator new[] and copies in the suppplied string.
        static char* allocCopyString(QString string)
        {
            QByteArray data = string.toUtf8();
            size_t len = data.size();
            char* str = new char[len+1];
            strcpy(str, data.constData());
            return str;
        }
        
        ///
        /// launchTask -- launch tasks through open() using UNIX system calls.
        ///
        /// Origin needs to launch processes through Finder to ensure they are started
        /// with the correct environment, etc.  To accomplish this, Origin uses the
        /// open(1) command, passing in appropriate command-line arguments.
        ///
        /// When launching processes, open will return failure if the application is unable
        /// to be launched.  This causes launchTask2 to return 0, indicating failure.
        ///
        /// Open cannot monitor the exit status or actual PID of the launched process.
        ///
        pid_t launchTaskWithOpen( QString applicationPath, QStringList args, QStringList environment )
        {
            // Create the argc array of arguments.
            int argc = args.size() + 5;
            char* argv[argc];
            argv[0] = allocCopyString("/usr/bin/open");
            argv[1] = allocCopyString("-a");
            argv[2] = allocCopyString(applicationPath);
            argv[3] = allocCopyString("--args");
            for (int i=0; i != args.size(); ++i)
                argv[i+4] = allocCopyString(args.at(i));
            argv[argc-1] = 0;
            
            // Create the envp array of environment variables.
            int envc = environment.size() + 1;
            char* envp[envc];
            for (int i=0; i != environment.size(); ++i)
                envp[i] = allocCopyString(environment.at(i));
            envp[envc-1] = 0;
            
            // Fork into a parent and child process.
            pid_t pid = fork();
            
            // If we are the parent,
            if (pid)
            {
                // Wait for the child to exit.
                int status, ret;
                for (ret = waitpid( pid, &status, 0 ); ret == -1 && errno == EINTR; ret = waitpid( pid, &status, 0 )); // loop over EINTR

                // Delete our allocated strings.
                for (int i=0; i != argc-1; ++i) delete[] argv[i];
                for (int i=0; i != envc-1; ++i) delete[] envp[i];
                
                // Return the error code 0 if the program returned a nonzero exit status.
                if (WIFEXITED(status) && WEXITSTATUS(status)) return 0;
                
                // Return the unknown PID -1 otherwise.
                else return -1;
            }
            // Execute the program otherwise.
            else
            {
                // SECURITY: close all file descriptors beyond stdin, stdout, and stderrr.
                struct rlimit lim;
                getrlimit( RLIMIT_NOFILE, &lim );
                for ( rlim_t i = 3; i != lim.rlim_cur; ++i ) close( i );

                execve("/usr/bin/open", argv, envp);
                _exit(1); // bail with error code if we get here
            }
        }
        
        void ProcessOSX::startProcess()
        {
            // Valid parameters?
            ORIGIN_ASSERT(!mFullPath.isEmpty());
            if (mFullPath.isEmpty())
            {
                mError = FileNotFound;
                return;
            }
            
            mError = NoError;
            mState = Starting;

            //Check to see if there are any additional environment variables that need to be passed to the game
            if (!mAdditionalEnvVars.isEmpty())
            {
                //split the QString and add each envionment varialbe to the QProcessEnvironment
                QStringList envVarsSplit = mAdditionalEnvVars.split(QChar(0xFF));
                for(int i = 0; i < envVarsSplit.count(); i++)
                {
                    QStringList envVarPair = envVarsSplit[i].split("=");
                    if (envVarPair.count() == 2)
                        mProcessEnvironment.insert(envVarPair[0], envVarPair[1]);
                }
            }
            
            // Split the command-line arguments.
            const QString ArgSeperator = " ";
            QStringList cmdArgList;
            if ( mCmdLineArgs.size() > 0 )
                cmdArgList = mCmdLineArgs.split( ArgSeperator );

            QString args;
            for (QStringListIterator it(cmdArgList); it.hasNext(); )
            {
                args += "arg: [" + it.next() + "] ";
            }
            ORIGIN_LOG_EVENT << "Launching executable: " << mFullPath << " with " << cmdArgList.size() << " args: " << args;
            
            mError = Timedout;
            mState = NotRunning;

            // Check the OS - starting with El Capitan, because of S.I.P., we need to use NSTask
            const Origin::VersionInfo ElCapitanOSVersion(10, 11, 0, 0);
            Origin::VersionInfo currentVersion = PlatformService::currentOSVersion();
            
            if (currentVersion >= ElCapitanOSVersion)
                mPid = launchTaskWithNSTask( mFullPath, cmdArgList, mProcessEnvironment.toStringList(), mExceptionSystem );
            
            else
                mPid = launchTaskWithOpen( mFullPath, cmdArgList, mProcessEnvironment.toStringList() );
            
            if ( mPid == 0 )
            {
                ORIGIN_LOG_EVENT << "Process failed to start.";
                mError = FailedToStart;
                mState = NotRunning;
                emit error(mError, 0);
                emit stateChanged(mState);
            }
            else
            {
                mProcessWatcher.addProcess( mPid, mFullPath );
                mError = NoError;
                mState = Running;
                emit started();
                emit stateChanged(mState);
                ORIGIN_LOG_EVENT << "Process has PID: " << (int)mPid;
            }
        }
        
        void ProcessOSX::startProcessMonitoring()
        {
            if (mWaitForExit)
                waitForExit(mBlockOnWait, mTimeout);
            
            //send out the error to anyone who cares
            if (mError!=NoError)
                emit(error(mError, 0));
        }

        bool ProcessOSX::attach(pid_t pid)
        {
            mPid = pid;
            
            ORIGIN_ASSERT(!mFullPath.isEmpty());
            if (mFullPath.isEmpty())
            {
                mError = FileNotFound;
                return false;
            }
            
            mProcessWatcher.addProcess( mPid, mFullPath );
            mError = NoError;
            mState = Running;
            emit started();
            emit stateChanged(mState);
            ORIGIN_LOG_EVENT << "Process attached to PID: " << (int)mPid;
            
            startProcessMonitoring();
            
            return true;
        }
        
        void ProcessOSX::terminate()
        {
            terminateThroughFinder( mPid ); // TODO: what if this fails?
            mExitCode = 0; // TODO
        }

        void ProcessOSX::close()
        {
            // Do nothing.  The destructor for AppProcessWatcher unblocks any calls to waitUntilFinished().
        }
        
        IProcess::ExitStatus ProcessOSX::exitStatus() const
        {
            if ( mProcessWatcher.isRunning() ) return IProcess::NoExit;
            else return IProcess::NormalExit;
        }

        bool ProcessOSX::isRunning(bool bVerify /*= false*/) const
        {
            return mProcessWatcher.isRunning();
        }

        bool ProcessOSX::waitForExit(bool bBlocking, qint32 iTimeout /*= -1*/)
        {
            bool bRet = false;
            mTimeout = iTimeout;
            // Spawn monitoring thread if non-blocking
            if (!bBlocking)
            {
                mWaitThread = new WaitForExitAsyncTrampoline(*this);
                // AppProcessWatcher will fire signals, so move it to the right thread!
                mProcessWatcher.moveToThread(mWaitThread);

                mWaitThread->start();
                mMonitoringProc = false;

                return true;
            }

            mMonitoringProc = true;
            
            // Use the Process Watcher to wait until all children are done as well.
            mProcessWatcher.waitUntilFinished( mTimeout );
            
            // Bail if the object has been deleted.
            if ( isDeleted ) return false;

            mMonitoringProc = false;
            mProcessWatcher.moveToThread(QThread::currentThread());

            if (mCrashed)
                emit error(mError, 0);

            mState = NotRunning;
            emit finished(mPid, mExitCode);
            emit finished(mPid, mExitCode, exitStatus());
            emit stateChanged(mState);

            if (mWaitThread)
            {
                mWaitThread->deleteLater();
            }
            mWaitThread=NULL;
            
            return bRet;
        }
    
        void ProcessOSX::resume()
        {
            // Qprocess does not support suspend/resume....
        }

        bool ProcessOSX::runProtocol(const QString& sFullPath, const QString& sCmdLineArgs, const QString& sCurrentDir)
        {
            Q_UNUSED( sCmdLineArgs );
            Q_UNUSED( sCurrentDir );
            
            return openURL( sFullPath.toUtf8().constData() );
        }

        bool ProcessOSX::removeEnvironmentVar (const QString sEnvironmentVar)
        {
            
            return false;
        }
    } // namespace Origin
} // namespace Services


#endif

