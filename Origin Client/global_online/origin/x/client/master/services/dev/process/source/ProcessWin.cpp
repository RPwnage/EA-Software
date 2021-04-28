#include "services/process/ProcessWin.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformService.h"
#include "services/downloader/StringHelpers.h"
#include "Common.h"

#include <QThreadPool>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QtConcurrentRun>


#ifdef Q_OS_WIN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
//#include <windows.h>

#include <tchar.h>
#include <Strsafe.h>
#include <shellapi.h>
#include <winsafer.h>
#include <sddl.h>
#include <psapi.h>
#include <Tlhelp32.h>
#include "IEscalationClient.h"

#ifdef _CIDER_SUPPORT_

#include "core/Common/source/TGCider/tg_enums.h"
#include "core/Common/source/TGCider/tg_os.h"

//
// MAC OSX Cider Helpers
bool getCiderInfo(bool &bIsMac, char *pVersion, int nVersionLen)
{
    HMODULE h = ::LoadLibraryW(L"ntdll.dll");
    if (NULL == h)
        return false;

    TYPEOF(IsTransgaming) pIsTransgaming = (TYPEOF(IsTransgaming))GetProcAddress(h, "IsTransgaming");
    TYPEOF(TGGetVersion)  pGetVersion = NULL;
    if (pVersion)
        pGetVersion = (TYPEOF(TGGetVersion))GetProcAddress(h, "TGGetVersion");
    TYPEOF(TGGetOS) pGetOS = (TYPEOF(TGGetOS))GetProcAddress(h, "TGGetOS");

    if (pIsTransgaming && pIsTransgaming())
    {
        if (pGetOS)
        {
            const char* pOS = pGetOS();
            bIsMac = ( pOS && 0 == _strnicmp(pOS, "MacOSX", 6) );
        }

        if (pGetVersion)
            pGetVersion(pVersion, nVersionLen);
    }

    ::FreeLibrary(h);

    return true;
}

#endif

bool isRunningUnderCider()
{
#ifdef _CIDER_SUPPORT_
    static bool s_bRunningOnCider = false, 
        s_bCached = false;

    // Cache response since it won't change during a session
    if (!s_bCached)
    {
        if (!getCiderInfo(s_bRunningOnCider, NULL, 0))
            return false;  // This should never happen and neither true NOR false is correct but...

        s_bCached = true;
    }
    return s_bRunningOnCider;
#else
    return false;
#endif
}

namespace Origin
{
    namespace Services
    {
        ProcessWin::ProcessWin(const QString& sFullPath, const QString& sCmdLineArgs, const QString& sCurrentDir, 
            const QString& sAdditionalEnvVars /*=""*/, bool bWaitForExit /*= true*/, bool bBlockOnWait /*= false*/,
            bool bRunElevated /*= false*/, bool bRunProtocol /*= false*/, bool bCreateSuspended /*= false*/,
            bool bRunInstallationProxy /*= false*/, bool bKillGameWhenOriginStops/*= false*/, QObject *parent /*= 0*/)
            :
        IProcess(sFullPath, sCmdLineArgs, sCurrentDir, 
            sAdditionalEnvVars, bWaitForExit, bBlockOnWait, bRunElevated, bRunProtocol, bCreateSuspended, bRunInstallationProxy, bKillGameWhenOriginStops, parent)
        {
            mOriginClientServicesMonitorThread = NULL;
            mJob = NULL;
            mWaitThread = NULL;
            mPid = new PROCESS_INFORMATION;
            memset(mPid, 0, sizeof(PROCESS_INFORMATION));
        }

        ProcessWin::~ProcessWin()
        {
            close();

            delete mPid;
            mPid = NULL;
        }

        void ProcessWin::startProcess()
        {
            // Valid parameters?
            ORIGIN_ASSERT(!mFullPath.isEmpty());
            if (mFullPath.isEmpty())
            {
                mError = FileNotFound;
                return;
            }

            if (!mRunProtocol)
            {
                QCryptographicHash hash(QCryptographicHash::Sha1);
                hash.addData(mFullPath.toUtf8());

                CreateJobObjectForMonitoring(QString("Global\\OriginGame::%1").arg(QString(hash.result().toHex())));
            }

            // Warn if additional environmental variables were specified but using ShellExec
            if (mRunElevated)
                mPid->hProcess = createProcessElevated(&(mPid->hThread), &(mPid->dwProcessId), &mError);
            else if (mRunProtocol)
                mPid->hProcess = createProcessProtocol();
            else
                mPid->hProcess = createProcess(&(mPid->hThread), &(mPid->dwProcessId));

            // we have to return immediately, and setup the "waiting later", otherwise we break IGO for legacy games! 
        }
        
        void ProcessWin::setupProcessMonitoring()
        {
            if (mWaitForExit)
                waitForExit(mBlockOnWait, mTimeout);

            //send out the error to anyone who cares
            if (mError!=NoError)
                emit(error(mError, mSystemErrorValue));
        }

        void ProcessWin::start()
        {
            startProcess();
            setupProcessMonitoring();
        }

        void ProcessWin::startUnmonitored()
        {
            startProcess();
        }

        bool ProcessWin::attach(pid_t pid)
        {
            HANDLE hProcess = OpenProcessForMonitoring(pid);
            if (hProcess)
            {
                mPid->hProcess = hProcess;
                mPid->hThread = NULL;
                mPid->dwProcessId = pid;

                setProcessState(Running);

                setupProcessMonitoring();
                return true;
            }

            return false;
        }

        void ProcessWin::terminateNonEscalation()
        {

            QSet<qint32> gameProcs = mGameProcessList.getGameProcs();
            for(QSet<qint32>::const_iterator it = gameProcs.constBegin();
                it != gameProcs.constEnd();
                it++)
            {
                qint32 processId = (*it);
                HANDLE hGameProcess = ::OpenProcess(PROCESS_TERMINATE, FALSE, processId);
                ::TerminateProcess(hGameProcess, 0xf291);
                ::CloseHandle(hGameProcess);
            }
        }

        void ProcessWin::terminateTaskHelper(qint32 processId)
        {
            HWND hProcMainWnd = getProcMainWindow(processId);

            QString escalationReasonStr = QString("terminateTaskHelper (%1)").arg(processId);
            int escalationError = 0;
            QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;
            if (!Origin::Escalation::IEscalationClient::quickEscalate(escalationReasonStr, escalationError, escalationClient))
                return;

            //first we try to close it nicely, by sending a WM_CLOSE via escalation client
            escalationClient->closeProcess((qint32)hProcMainWnd);

            //lets see if the process is still active, if so we wait and if it hasn't closed before a minute
            //lets drop the hammer
            HANDLE hProcess = ::OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE, FALSE, processId);
            // Different operating systems behave differently with this call.  XP doesn't support PROCESS_QUERY_LIMITED_INFORMATION,
            // but in some situations Windows 7 fails
            if (!hProcess)
                hProcess = ::OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE, FALSE, processId);




            DWORD exitCode = STILL_ACTIVE;
            bool isSuccess = 0 != ::GetExitCodeProcess( hProcess, &exitCode );
            if( isSuccess && exitCode == STILL_ACTIVE )
            {
                DWORD result = WaitForSingleObject(hProcess, 60000);

                //if we reached here for any reason other than then app terminated
                if(result != WAIT_OBJECT_0)
                {
                    //dropping hammer
                    escalationClient->terminateProcess(processId);
                }
            }

            ::CloseHandle(hProcess);
        }

        void ProcessWin::terminateTask()
        {
            if (NULL == mPid)
                return;

            QSet<qint32> gameProcs = mGameProcessList.getGameProcs();

            // 'gameProcs' could be empty if 'terminateTask()' was called too early,
            // e.g., before the 'JOB_OBJECT_MSG_NEW_PROCESS' gets processed for a newly started process
            if( gameProcs.isEmpty() )
            {
                terminateTaskHelper(mPid->dwProcessId);
                return;
            }

            for(QSet<qint32>::const_iterator it = gameProcs.constBegin();
                it != gameProcs.constEnd();
                it++)
            {
                qint32 processId = (*it);
                terminateTaskHelper(processId);
            }
        }


        void ProcessWin::terminate()
        {
            QtConcurrent::run(this, &ProcessWin::terminateTask);
        }

        void ProcessWin::close()
        {
            if (mClosed)
                return;

            mClosed = true;

            ::CloseHandle(mPid->hProcess);
            mPid->hProcess = NULL;
            mPid->hThread = NULL;
 
            // terminate mWaitThread to make sure waitForExit does not crash because of the NULL pointer
            if(mWaitThread)
            {
                mWaitThread->terminate();
            }

            if(mOriginClientServicesMonitorThread)
            {
                mOriginClientServicesMonitorThread->setGameStopped(true);
                mOriginClientServicesMonitorThread->wait();
            }

            // Keep the processID around in case we need it before destroying the object
        }

        IProcess::ExitStatus ProcessWin::exitStatus() const
        {
            if (!isRunning(true))
            {
                return mCrashed ? CrashExit : NormalExit;
            }

            return NoExit;
        }

        bool ProcessWin::isRunning(bool bVerify /*= false*/) const
        {
            if (NULL == mPid)
                return false;
            if (bVerify)
                return verifyActivePID(mPid->dwProcessId);
            return true;
        }

        bool ProcessWin::waitForExit(bool bBlocking, qint32 iTimeout /*= -1*/)
        {
            bool bRet = false;
            // exit early if the process faile to start no reason to wait for exit
            if(mError==FailedToStart)
                return bRet;


            mTimeout = iTimeout;
            // Spawn monitoring thread if non-blocking
            if (!bBlocking)
            {
                mWaitThread = new WaitForExitAsyncTrampoline(*this);
                mWaitThread->start();
                mMonitoringProc = false;
                // Wait for job setup to be established
                qint64 timeout = 15000;
                QElapsedTimer timer;
                timer.start();
                while (timer.elapsed() < timeout)
                {
                    if (mMonitoringProc)
                        return true;

                    Origin::Services::PlatformService::sleep(10);
                }

                return false;
            }

            if (mRunInstallationProxy)
            {
                mMonitoringProc = true;

                // Let proxy process deal with PID group monitoring and we'll just watch that single proxy
                HANDLE ahObjs[3];
                ahObjs[0] = mPid->hProcess;

                int8_t iObjCnt = 1;
                quint32 dwResult = ::WaitForMultipleObjects(iObjCnt, ahObjs, false, mTimeout);

                ORIGIN_LOG_ERROR << L"ProcessWin: 0x" << QString("%1").arg((unsigned int)mPid->hProcess, 0, 16, QChar(L'0')) << L"    Exit result: " << dwResult;

                // Success if the process or its main thread exited. False for timeouts etc.
                bRet = (dwResult <= WAIT_OBJECT_0+iObjCnt-1);
            }
            else
            {
                bRet = waitForProcessGroup(mPid->hProcess, mPid->hThread, mTimeout);
            }

            // Signal exit
            DWORD exitCode = STILL_ACTIVE;
            bool isSuccess = 0 != ::GetExitCodeProcess( mPid->hProcess, &exitCode );
            if (isSuccess && exitCode == STILL_ACTIVE)
            {
                // We may need to wait a tiny little bit because of our waitForProcessGroup
                DWORD extraCompletionDelayInMS = 500;
                Sleep(extraCompletionDelayInMS);

                isSuccess = 0 != ::GetExitCodeProcess( mPid->hProcess, &exitCode );
            }

            if( isSuccess && exitCode != STILL_ACTIVE )
            {
                mExitCode = exitCode;
                //### for now we assume a crash if exit code is less than -1 or the magic number
                mCrashed = (mExitCode == 0xf291 || mExitCode < 0);
            }
            else
            {
                ORIGIN_ASSERT(0);
                ORIGIN_LOG_WARNING << "Unable to query process exit code, error: " << GetLastError();
            }
            
            ::CloseHandle(mPid->hThread);
            mPid->hThread = 0;
            ::CloseHandle(mPid->hProcess);
            mPid->hProcess = 0;

            if (mCrashed || mError != NoError)
                emit error(mError, mSystemErrorValue);

            emit finished(mPid->dwProcessId, mExitCode);
            emit finished(mPid->dwProcessId, mExitCode, exitStatus());
            
            // set the state as the last item, some code(LocalContent.cpp) uses the state to kill our process class,
            // that way, we try to prevent a crash!
            setProcessState(NotRunning);
            mWaitThread = NULL;
            return bRet;
        }


        void ProcessWin::resume()
        {
            if (mPid->hThread)
                ResumeThread(mPid->hThread);
        }

        void ProcessWin::setProcessError(Origin::Services::IProcess::ProcessError state)
        {
            mError = state;
            emit error(mError, mSystemErrorValue);
        }

        void ProcessWin::setProcessState(Origin::Services::IProcess::ProcessState state)
        {
            mState = state;
            emit stateChanged(mState);
        }

        bool ProcessWin::verifyActivePID(DWORD uiPID) const
        {
            HANDLE hConnProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, uiPID);
            // PROCESS_QUERY_INFORMATION may fail on Vista under certain circumstances (UAC Off?) so try reduced version (introduced in Vista)
            if (NULL == hConnProcess)
                hConnProcess = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, uiPID);
            if (NULL == hConnProcess)
                return false;

            DWORD exitCode = 0;
            bool bRet = (NULL != hConnProcess && ::GetExitCodeProcess(hConnProcess, &exitCode) && exitCode == STILL_ACTIVE);
            ::CloseHandle(hConnProcess);
            return bRet;
        }

        HANDLE ProcessWin::createProcessElevated(HANDLE* phProcessThread, DWORD* pPID, ProcessError *returnErrorStatus)
        {
            setProcessState(Starting);
            QString sErrMessage;
            QString proxyPath;
            QString cacheDir;

            const QString SPROXY_EXE( "EAProxyInstaller.exe" ); // Name of the EAProxyInstaller process.
            const QString SREGFILE  ( "staging.reg" );		  // Name of the .reg file containing *this* processes HKEY_CURRENT_USER staging branch.

            // Command line parameters (for the proxy).
            const QString SPROXY_FULLPATH	 ( "/proxyFullPath" );	
            const QString SPROXY_CMDLINEARGS ( "/proxyCmdLineArgs" );
            const QString SPROXY_mCurrentDir ( "/proxyCurrentDir" );
            const QString SPROXY_SHOWUI		 ( "/proxyShowUI" );
            const QString SPROXY_REGPATH	 ( "/proxyRegPath" );	  // Path to a .reg file containing an HKCU import.
            const QString SPROXY_WAIT		 ( "/proxyWait" );

            SHELLEXECUTEINFO shellExecuteInfo;
            ZeroMemory(&shellExecuteInfo, sizeof(shellExecuteInfo));

            shellExecuteInfo.cbSize	  = sizeof(SHELLEXECUTEINFO);
            shellExecuteInfo.fMask	  = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC; // Keep the handle and make the process a group jobless root.
            shellExecuteInfo.hwnd	  = 0;
            shellExecuteInfo.lpVerb   = L"open";
            shellExecuteInfo.hProcess = 0;
            shellExecuteInfo.nShow	  = SW_SHOW;

            if(returnErrorStatus)
                *returnErrorStatus = NoError;

            // Should we use the EAProxyInstaller process?
            QString	proxyCmdLineArgs;
            if ( mRunInstallationProxy ) // Yes.
            {
                // Construct the path of information.
                proxyPath = QString("%1/%2").arg(QCoreApplication::applicationDirPath()).arg(SPROXY_EXE);
                cacheDir = Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOAD_CACHEDIR, Origin::Services::Session::SessionRef());
                cacheDir.replace("\\", "/");
                if (!cacheDir.endsWith("/"))
                {
                    cacheDir.append("/");
                }

                // Set up the command line for the proxy installer.
                proxyCmdLineArgs = QString("%1=%2 %3=%4 %5=%6 %7=%8 %9=%10%11 %12=%13")
                    .arg(SPROXY_FULLPATH).arg(mFullPath)
                    .arg(SPROXY_CMDLINEARGS).arg(mCmdLineArgs)
                    .arg(SPROXY_mCurrentDir).arg(mCurrentDir)
                    .arg(SPROXY_SHOWUI).arg("1")
                    .arg(SPROXY_REGPATH).arg(cacheDir).arg(SREGFILE)
                    .arg(SPROXY_WAIT).arg(mWaitForExit ? "1" : "0");

                shellExecuteInfo.lpFile		  = proxyPath.utf16();
                shellExecuteInfo.lpParameters = proxyCmdLineArgs.utf16();
                shellExecuteInfo.lpDirectory  = mCurrentDir.utf16();
            }
            else // No.
            {
                shellExecuteInfo.lpFile		  = mFullPath.utf16();
                shellExecuteInfo.lpParameters = mCmdLineArgs.utf16();
                shellExecuteInfo.lpDirectory  = mCurrentDir.utf16();
            }

            ORIGIN_LOG_EVENT << L"createProcessElevated Path (" << shellExecuteInfo.lpFile << L"," << shellExecuteInfo.lpParameters << L")";

            //
            // Use Escalation client to reuse/initiate a UAC session while we're at it
            // We don't want default behavior, we will handle failures
            QScopedPointer<Origin::Escalation::IEscalationClient> escalationClient;
            escalationClient.reset(Origin::Escalation::IEscalationClient::createEscalationClient());
            escalationClient->enableEscalationService(true);

            int escalationError = escalationClient->checkIsRunningElevatedAndUACOK();

            if(escalationError == Origin::Escalation::kCommandErrorNotElevatedUser)
            {
                if(returnErrorStatus)
                    *returnErrorStatus = RequiresAdminRights;

                setProcessState(NotRunning);
                return 0;
            }
            else
                if(escalationError != Origin::Escalation::kCommandErrorNone)
                {
                    if(returnErrorStatus)
                        *returnErrorStatus = UACRequired;

                    setProcessState(NotRunning);

                    return 0;
                }
                else
                {
                    DWORD pid = escalationClient->executeProcess(QString::fromUtf16(shellExecuteInfo.lpFile), 
                        QString::fromUtf16(shellExecuteInfo.lpParameters), 
                        QString::fromUtf16(shellExecuteInfo.lpDirectory),
                        mAdditionalEnvVars,
                        mJobName);

                    // Get handled to main process thread
                    if (phProcessThread)
                    {
                        HANDLE hMainThread = NULL;
                        HWND hProcMainWnd = getProcMainWindow(pid);
                        if (hProcMainWnd)
                        {
                            DWORD dwDummyPID;
                            qint32 dwThreadId = ::GetWindowThreadProcessId( hProcMainWnd, &dwDummyPID);
                            ORIGIN_ASSERT(dwThreadId);					

                            if (NULL==dwThreadId)
                            {
                                ORIGIN_LOG_ERROR << "GetWindowThreadProcessId failed.";
                                ORIGIN_ASSERT(false);					
                            }
                            ORIGIN_ASSERT(dwDummyPID == pid);					
                            // Open a handle to the main thread
                            hMainThread = ::OpenThread(SYNCHRONIZE, FALSE, dwThreadId);

                            if (NULL==hMainThread)
                            {
                                ORIGIN_LOG_ERROR << "OpenThread failed.";
                                ORIGIN_ASSERT(false);
                            }
                        }

                        *phProcessThread = hMainThread;
                    }

                    if ( pid != 0 )
                    {
                        shellExecuteInfo.hProcess = OpenProcessForMonitoring(pid);
                        if(NULL == shellExecuteInfo.hProcess)
                        {
                            ORIGIN_LOG_ERROR << "Failed to open elevated process [" << pid << "] for monitoring.  Last error was: " << GetLastError();
                        }

                        // Get a PID if requested
                        if (NULL != pPID)
                            *pPID = pid;

                        setProcessState(Running);

                        return shellExecuteInfo.hProcess;
                    }
                    else
                    {
                        ORIGIN_LOG_ERROR << L"Escalation Client error " << escalationClient->getError() << L" (OS: " << 
                            escalationClient->getSystemError() << L"). Opening " << shellExecuteInfo.lpFile << L" " << shellExecuteInfo.lpParameters;

                        if (pid == 0)
                        {
                            mSystemErrorValue = escalationClient->getError();
                            if(returnErrorStatus)
                                *returnErrorStatus = InvalidPID;
                        }
                    }

                }

                // Error Handling
                HANDLE hProcess = shellExecuteInfo.hProcess;

                // return result is HINSTANCE for backwards compatibility - you're expected to cast to an int when checking.
#pragma warning( disable : 4311 ) // warning C4311: 'type cast' : pointer truncation from 'HINSTANCE' to 'int'
                int result = (int) shellExecuteInfo.hInstApp;
#pragma warning( default : 4311 )

                if( hProcess != NULL && result > 32 )
                    return hProcess; // no error

                sErrMessage = QString("ShellExecute Failed: \nexe: '%1 %2' \nmCurrentDir: '%3'\n")
                    .arg(QString::fromWCharArray(shellExecuteInfo.lpFile))
                    .arg(QString::fromWCharArray(shellExecuteInfo.lpParameters))
                    .arg(QString::fromWCharArray(shellExecuteInfo.lpDirectory));

                switch(result)
                {
                case SE_ERR_FNF:
                    sErrMessage.append("The specified file was not found.");
                    break;

                case SE_ERR_PNF:
                    sErrMessage.append("The specified path was not found.");
                    break;

                case SE_ERR_ACCESSDENIED:
                    sErrMessage.append("The operating system denied access to the specified file.");
                    break;

                case SE_ERR_SHARE:
                    sErrMessage.append("A sharing violation occurred.");
                    break;

                default:
                    sErrMessage.append(QString("Unknown error occured. Code %1").arg(500+result));
                    //sErrMessage.append(GetGenericErrorString(kCoreSvc, 500+result));
                    break;
                }

                ORIGIN_LOG_ERROR << sErrMessage;

                if (!hProcess)
                {
                    if(returnErrorStatus)
                        *returnErrorStatus = InvalidHandle;

                    mSystemErrorValue = GetLastError();
                }
#ifdef _DEBUG
                MessageBox(NULL, sErrMessage.utf16(), L"Error", MB_OK);
#endif                  
                setProcessState(NotRunning);
                return 0;
        }

        HANDLE ProcessWin::createProcessProtocol()
        {
            LPCWSTR cmdLine = NULL;
            LPCWSTR currentDir = NULL;
            
            if(!mCmdLineArgs.isEmpty())
                cmdLine = mCmdLineArgs.utf16();
            
            if(!mCurrentDir.isEmpty())
                currentDir = mCurrentDir.utf16();

            SHELLEXECUTEINFO executeInfo = {0};
            executeInfo.cbSize = sizeof(SHELLEXECUTEINFO);
            executeInfo.lpVerb = L"open";
            executeInfo.lpDirectory = currentDir;
            executeInfo.lpFile = mFullPath.utf16();
            executeInfo.lpParameters = cmdLine;
            executeInfo.nShow = SW_SHOW;
            executeInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
            setProcessState(Starting);
            bool s = ::ShellExecuteEx(&executeInfo);
            if (s && executeInfo.hProcess != 0 && executeInfo.hProcess != INVALID_HANDLE_VALUE)
                setProcessState(Running);
            else
            {
                mError = FailedToStart;
                mSystemErrorValue = GetLastError();
                setProcessState(NotRunning);
            }

            return executeInfo.hProcess;
        }

        HANDLE ProcessWin::createProcess(HANDLE* phProcessThread, DWORD* pPID)
        {
            setProcessState(Starting);

            QString sFullCmd;
            wchar_t sFullCmdBuffer[2048];
            memset(sFullCmdBuffer, 0, 2048 * sizeof(wchar_t));

            HANDLE hProcess_ret = NULL;
            //  Create the process with the normal user compatible access token.
            STARTUPINFO oStartUpInfo;
            memset(&oStartUpInfo, 0, sizeof(oStartUpInfo));
            oStartUpInfo.cb = sizeof(oStartUpInfo);
            PROCESS_INFORMATION oProcessInfo={0};

            // CreateProcessXXXX does some very odd things and command arguments, current directory etc. don't aways work as expected
            // so use second argument for command AND arguments and use CD to change directory instead of unreliable curdir arg.

            bool bProcSuccess = false;
            {
#if 0
                //  EBIBUGS-17521:  Using shorten path names will crash the game Syndicate which is why we've disabled it here.
				WCHAR path[MAX_PATH];
				QFileInfo filePath(mFullPath);
				//we only want to use the short file path on the path and not the file name as that can interfere with the launching of some
				//installers or executables
				if(GetShortPathName(filePath.absolutePath().utf16(), path, MAX_PATH) != 0)
				{
					mFullPath = QString::fromUtf16(path);
					mFullPath = mFullPath + "/" + filePath.fileName();
				}
#endif


                // If the full path is not quoted, then quote it
                // This is due to legacy GameLauncher that assumes the command line contains
                // a quoted command - it auto strips the first and last character from "GetCommandLine" (doh!)
                if ( !mFullPath.isEmpty() && !mFullPath.startsWith("\"") && !mFullPath.endsWith("\""))
                    sFullCmd = QString("\"%1\" %2").arg(mFullPath).arg(mCmdLineArgs);
                else
                    sFullCmd = QString("%1 %2").arg(mFullPath).arg(mCmdLineArgs);
            }

            ORIGIN_LOG_ERROR << L"Open Path (" << mFullPath << L")";
            sFullCmd.toWCharArray(sFullCmdBuffer);

            // Create a new environment with additional items
            wchar_t sEnvStringBlock[32767];  // Max size specified in MSDN
            sEnvStringBlock[0] = sEnvStringBlock[1] = NULL;

            // Create custom environmental variables if specifying additional.
            if (!mAdditionalEnvVars.isEmpty())
            {
                //Need to grab the system path from the registry in the case that an installer included a required - EBIBUGS-28617
                QProcessEnvironment runningEnv = QProcessEnvironment::systemEnvironment();
                QString path;
                if (PlatformService::registryQueryValue("HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\Control\\Session Manager\\Environment\\Path", path))
                {
                    runningEnv.remove("Path");
                    runningEnv.insert("Path", path);
                }
                QStringList envStringList = runningEnv.toStringList();
                QString envString;
                for (auto iter = envStringList.begin(); iter != envStringList.end(); ++iter)
                {
                    if (iter != envStringList.begin())
                        envString += QChar('\0');
                    envString += *iter;
                }

                // Enough room for additional variables in env block?
                if ((envString.length() + 2) < (long)(_countof(sEnvStringBlock) - mAdditionalEnvVars.length()))
                {
                    // A bit of wackiness - since our message system does not handle nulls inside of strings,
                    // and environment string that has multiple settings will have a char(FF) in the middle
                    // of the strings as a delimiter, instead of the required NULL.
                    // Replace the NULL where the char(FF) appears.
                    QString szFixedEnvironmentStr = mAdditionalEnvVars;
                    szFixedEnvironmentStr.replace(QChar(0xFF), QChar('\0'));

                    envString += QChar('\0') + szFixedEnvironmentStr;
                    envString.toWCharArray(sEnvStringBlock);
                    // don't do envString.length()+1 and envString.length()+2
                    // envString.length() is the string length without NULL termination character
                    // example: envString -> "hallo"
                    //          sEnvStringBlock -> "hallo1234"
                    //          envString.length() + 1 == 6
                    //          envString.length() + 2 == 7
                    // sEnvStringBlock[envString.length()+1] == 2
                    // sEnvStringBlock[envString.length()+2] == 3
                    // sEnvStringBlock[envString.length()] != NULL!!!
                    sEnvStringBlock[envString.length()] = NULL;  // Reterminate with 2 x NULL
                    sEnvStringBlock[envString.length()+1] = NULL;  // Reterminate with 2 x NULL
                }
            }

            // Note current directory
            // SetCurrentDirectory is tricky and might cuase problem!!!

            // In case we fail to create our process and the error is elevation
            bool bCreatedElevated = false;
            const long    UNICODE_MAX_PATH		 = 32767; // Maximum path size (in characters).
            wchar_t sRememDir[UNICODE_MAX_PATH+1]= L"";
            ::GetCurrentDirectory(_countof(sRememDir), sRememDir);
            // Change to specified directory (if specified)
            if (mCurrentDir.isEmpty() || ::SetCurrentDirectory(mCurrentDir.utf16()))
            {
                // Kick off process
                qint32 dwFlags = CREATE_UNICODE_ENVIRONMENT;
                // Note: Try JOBs at least once. 
                //       May fail on certain low security user / OS combinations so we'll monitor just the main proc in those cases if need be
                dwFlags |= CREATE_BREAKAWAY_FROM_JOB;  
                if (mCreateSuspended)
                    dwFlags |= CREATE_SUSPENDED;
                SetLastError(0);

                //We don't want to suppress any game error dialogs for missing files/dlls (happened with BF4). We temporarily set the error mode
                //so that we will enable all error dialogs. After we create our processes we will set the error mode back to what it was.
                uint32_t previousErrorMode = SetErrorMode(0);

                bProcSuccess = CreateProcessW( NULL, sFullCmdBuffer, 
                    NULL, NULL, FALSE, dwFlags,
                    (!mAdditionalEnvVars.isEmpty() ? (LPVOID)sEnvStringBlock : NULL),
                    NULL, &oStartUpInfo, &oProcessInfo );

                int err = GetLastError();
                ORIGIN_LOG_WARNING << L"CreateProcessW(" << mFullPath << L") Handle: " << oProcessInfo.dwProcessId 
                    << L"  bProcSuccess: " << (bProcSuccess ? L"t" : L"f") << L"   LastError: " << err;

                // Try without the BREAKAWAY job support if that's the possible cause of a failure
                if (!bProcSuccess && err == ERROR_ACCESS_DENIED)
                {
                    // Remove JOB related flag and retry process creation
                    dwFlags &= ~CREATE_BREAKAWAY_FROM_JOB;
                    SetLastError(0);
                    bProcSuccess = CreateProcessW( NULL, sFullCmdBuffer, 
                        NULL, NULL, FALSE, dwFlags,
                        (!mAdditionalEnvVars.isEmpty() ? (LPVOID)sEnvStringBlock : NULL),
                        NULL, &oStartUpInfo, &oProcessInfo );
                    ORIGIN_LOG_WARNING << L"CreateProcessW(" << mFullPath << L") Handle: " << oProcessInfo.dwProcessId 
                        << L"  bProcSuccess: " << (bProcSuccess ? L"t" : L"f") << L"   LastError: " << GetLastError();
                }
                else if (!bProcSuccess && err == ERROR_ELEVATION_REQUIRED)
                {
                    ORIGIN_LOG_WARNING << L"Failed to create process because we needed elevation. Elevating now";
                    hProcess_ret = createProcessElevated(phProcessThread, pPID, &mError);
                    bProcSuccess = bCreatedElevated = (hProcess_ret != NULL);
                    mRunElevated = bProcSuccess;    // mark this process as elevated in order for IGO to work correctly
                }

                //set back the error mode
                SetErrorMode(previousErrorMode);

                if (!bProcSuccess)
                {
                    mError = FailedToStart;
                    mSystemErrorValue = GetLastError();
                }

                // Restore current directory
                if (!mCurrentDir.isEmpty())
                    ::SetCurrentDirectory(sRememDir);

                // Return main process thread and/or PID
                if (!bCreatedElevated)
                {
                    if (phProcessThread)
                        *phProcessThread = oProcessInfo.hThread;
                    if (pPID)
                        *pPID = oProcessInfo.dwProcessId;
                }
            }
            else
            {
                bProcSuccess = false;
                ORIGIN_LOG_ERROR << L"SetCurrentDirectory(" << mCurrentDir << L") failed. Error " << GetLastError();

                mError = FileNotFound;
                mSystemErrorValue = GetLastError();
            }

            if (bProcSuccess) // Only use oProcessInfo if we didn't create elevated.
                hProcess_ret = bCreatedElevated ? hProcess_ret : oProcessInfo.hProcess;
            else
                ORIGIN_LOG_ERROR << L"CreateProcess(" << L") failed.";

            // Return on successful process creation
            if (hProcess_ret) 
            {   
                setProcessState(Running);
                goto Exit;
            }
            // Error Handling
            {
                setProcessState(NotRunning);
                if (mError!=NoError)
                    mError = FailedToStart;

                if (mSystemErrorValue==0)
                    mSystemErrorValue = GetLastError();

                int iResult = GetLastError();
				// Getting Windows error messages isn't portable, so we've moved it out of the StringHelpers.  The error code is sufficient.
                //QString szWindowsErrorMsg;
                //Origin::Downloader::StringHelpers::GetWindowsErrorMessage(szWindowsErrorMsg, iResult);
                QString sErrMessage;
                sErrMessage = QString("CreateProcessW FAILED [Win32 Error: %1 | %2 %3]") 
                    .arg(iResult).arg(mCurrentDir).arg(mFullPath);
                ORIGIN_LOG_ERROR << sErrMessage;
#ifdef _DEBUG
                MessageBox(NULL, sErrMessage.utf16(), L"Error", MB_OK);
#endif
            }

Exit:
            //
            // Get suspended process going if caller didn't request suspended creation
            if (oProcessInfo.hProcess && !mCreateSuspended)
            {
                if ( oProcessInfo.hThread != 0 )
                {
                    ::ResumeThread(oProcessInfo.hThread);

                    // Done with main process thread handle
                    ::CloseHandle(oProcessInfo.hThread);
                }

                if ( mRunInstallationProxy )
                {
                    //If using the ProxyInstaller then ensure it launched the target process
                    //ProxyInstaller will not wait for the target process so it should finish
                    //as soon as it launches it
                    ::WaitForSingleObject(oProcessInfo.hProcess, INFINITE);
                    setProcessState(NotRunning);

                }
            }

            return hProcess_ret;
        }



        void ProcessList::translateDeviceNameToDriveLetters( TCHAR *pszFilename )
        {
            const int BUFSIZE = 512;

            // Translate path with device name to drive letters.
            TCHAR szTemp[BUFSIZE];
            szTemp[0] = '\0';

            if (GetLogicalDriveStrings(BUFSIZE-1, szTemp)) 
            {
                TCHAR szName[MAX_PATH];
                TCHAR szDrive[3] = TEXT(" :");
                BOOL bFound = FALSE;
                TCHAR* p = szTemp;

                do 
                {
                    // Copy the drive letter to the template string
                    *szDrive = *p;

                    // Look up each device name
                    if (QueryDosDevice(szDrive, szName, MAX_PATH))
                    {
                        size_t uNameLen = _tcslen(szName);

                        if (uNameLen < MAX_PATH) 
                        {
                            bFound = _tcsnicmp(pszFilename, szName, uNameLen) == 0
                                && *(pszFilename + uNameLen) == _T('\\');

                            if (bFound) 
                            {
                                // Reconstruct pszFilename using szTempFile
                                // Replace device path with DOS path
                                TCHAR szTempFile[MAX_PATH];
                                StringCchPrintf(szTempFile,
                                    MAX_PATH,
                                    TEXT("%s%s"),
                                    szDrive,
                                    pszFilename+uNameLen);
                                StringCchCopyN(pszFilename, MAX_PATH+1, szTempFile, _tcslen(szTempFile));
                            }
                        }
                    }

                    // Go to the next NULL character.
                    while (*p++);
                } while (!bFound && *p); // end of string
            }
        }
        bool ProcessList::getExeNameFromPID(QString & exeName, qint32 pid)
        {
            HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);

            if(handle == NULL)
                handle = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);

            if( handle == NULL )
            {
                ORIGIN_LOG_WARNING << "Failed to open process to query process name information.  Last error was: " << GetLastError();
                return false;
            }

            const int UNICODE_MAX_PATH = 32767;
            TCHAR exePath[UNICODE_MAX_PATH];
            qint32 bytes = GetProcessImageFileName(handle, exePath, UNICODE_MAX_PATH);
            if( bytes == 0 )
            {
                ORIGIN_LOG_WARNING << L"Error: Could not get image file path. LastError: " << GetLastError();
                return false;
            }

            translateDeviceNameToDriveLetters( exePath );

            // extend path length to 32767 wide chars
            QString prependedPath("\\\\?\\");
            prependedPath.append( QString::fromWCharArray(exePath) );

            bytes = ::GetLongPathName(prependedPath.utf16(), exePath, UNICODE_MAX_PATH);
            if( bytes == 0 )
            {
                ORIGIN_LOG_WARNING << L"Error: Could not convert to long path name. LastError: " << GetLastError();
                return false;
            }

            QString str = QString::fromWCharArray(exePath);
            int pos = str.lastIndexOf("\\");
            exeName = str.right(str.length() - 1 - pos);
            exeName = exeName.toLower();

            ::CloseHandle(handle);

            return true;
        }

        bool ProcessList::getExeNameFromFullPath(QString & exeName, const QString executablePath)
        {
            const int UNICODE_MAX_PATH = 32767;
            TCHAR exePath[UNICODE_MAX_PATH];

            wcscpy_s(exePath, _countof(exePath), executablePath.utf16());
            QString prependedPath;
            if (!executablePath.startsWith("\\\\?\\", Qt::CaseInsensitive))
            {
                translateDeviceNameToDriveLetters(exePath);

                // extend path length to 32767 wide chars
                prependedPath = "\\\\?\\";
            }
            prependedPath.append(QString::fromWCharArray(exePath));

            qint32 bytes = ::GetLongPathName(prependedPath.utf16(), exePath, UNICODE_MAX_PATH);
            if (bytes == 0)
            {
                ORIGIN_LOG_WARNING << L"Error: Could not convert to long path name. LastError: " << GetLastError();
                return false;
            }

            QString str = QString::fromWCharArray(exePath);
            int pos = str.lastIndexOf("\\");
            exeName = str.right(str.length() - 1 - pos);
            exeName = exeName.toLower();

            return true;
        }


        // We only get a PID, if a process with the specified name has a TCP socket the the current process!
        DWORD ProcessList::getPIDFromSocket(const QString exePath)
        {
            QString exeFile;
            DWORD processID = 0;
            Origin::Services::ProcessList::getExeNameFromFullPath(exeFile, exePath);

            HMODULE hIphlpapi = GetModuleHandleW(L"Iphlpapi.dll");
            bool loaded = false;
            if (!hIphlpapi)
            {
                loaded = true;
                hIphlpapi = LoadLibraryW(L"Iphlpapi.dll");
            }

            if (hIphlpapi)
            {
                DWORD(WINAPI *pGetExtendedTcpTable)(PVOID pTcpTable,
                    PDWORD pdwSize,
                    BOOL bOrder,
                    ULONG ulAf,
                    TCP_TABLE_CLASS TableClass,
                    ULONG Reserved);

                pGetExtendedTcpTable = (DWORD(WINAPI *)(PVOID, PDWORD, BOOL, ULONG, TCP_TABLE_CLASS, ULONG)) GetProcAddress(hIphlpapi, "GetExtendedTcpTable");

                if (pGetExtendedTcpTable)
                {

                    DWORD size = 0;
                    DWORD dwResult = pGetExtendedTcpTable(NULL, &size, false, AF_INET, TCP_TABLE_OWNER_PID_LISTENER, 0);
                    if (dwResult == ERROR_INSUFFICIENT_BUFFER)
                    {
                        MIB_TCPTABLE_OWNER_PID *pTCPInfo = (MIB_TCPTABLE_OWNER_PID*)malloc(size * 2 /* take twice the size to be safe */);
                        dwResult = pGetExtendedTcpTable(pTCPInfo, &size, false, AF_INET, TCP_TABLE_OWNER_PID_LISTENER, 0);
                        if (dwResult == NO_ERROR)
                        {
                            QString exeTmp;
                            MIB_TCPROW_OWNER_PID *owner;
                            for (DWORD dwLoop = 0; dwLoop < pTCPInfo->dwNumEntries; dwLoop++)
                            {
                                owner = &pTCPInfo->table[dwLoop];
                                if (owner)
                                {
                                    Origin::Services::ProcessList::getExeNameFromPID(exeTmp, owner->dwOwningPid);
                                    if (!exeTmp.isEmpty())
                                    {
                                        if (exeTmp.compare(exeFile, Qt::CaseInsensitive) == 0)  // we found our executable
                                        {
                                            processID = owner->dwOwningPid;
                                            ORIGIN_LOG_EVENT << "[Content Controller] PID " << owner->dwOwningPid << " " << exeFile;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        free(pTCPInfo);
                    }
                }
                if (loaded)
                    FreeLibrary(hIphlpapi);
            }
            return processID;
        }


        bool ProcessList::isBrowserProcess( qint32 pid )
        {
            //top ten browsers - http://tech.mobiletod.com/top-10-web-browsers-worth-trying/ 
            QStringList excludedNames;
            excludedNames << "iexplore.exe" << "firefox.exe" << "chrome.exe" << "opera.exe" <<
                "mozilla.exe" << "safari.exe" << "avant.exe" << "netscape.exe" << "netscape.exe" <<
                "deepnet.exe" << "maxthon.exe" << "phaseout.exe";
            QString exeName;
            bool result = false;
            if( getExeNameFromPID( exeName, pid ) )
            {
                result = excludedNames.contains(exeName);
            }

            return result;
        }

        bool ProcessList::getParentProcessPID(quint32 pid, qint32* pParentId)
        {
            bool result = false;

            if ( pid == 0 )
                pid = ::GetCurrentProcessId();

            HANDLE	hSnapshot = ::CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

            if ( hSnapshot != INVALID_HANDLE_VALUE )
            {
                PROCESSENTRY32	pe;
                pe.dwSize = sizeof(pe);

                if ( ::Process32First( hSnapshot, &pe ) )
                {
                    do
                    {
                        if ( pe.th32ProcessID == pid )
                        {
                            //Got our process information!
                            if ( pParentId )
                            {
                                *pParentId = pe.th32ParentProcessID;
                                result = true;
                                break;
                            }
                        }

                        ZeroMemory(&pe, sizeof(pe));
                        pe.dwSize = sizeof(pe);

                    } while ( ::Process32Next( hSnapshot, &pe ) );
                }

                ::CloseHandle( hSnapshot );
            }
            else
            {
                ORIGIN_LOG_ERROR << L"CreateToolhelp32Snapshot failed. Last error: " << GetLastError();
            }

            return result;
        }

        bool ProcessList::isBrowserChildProcess( qint32 pid )
        {
            qint32 parentId = 0;
            if(getParentProcessPID(pid, &parentId))
            {
                QSet<qint32>::iterator it = browserProcs.find( parentId );
                if( it != browserProcs.end() )
                    return true;
            }

            return false;
        }

        void ProcessList::add( qint32 pid )
        {
            QSet<qint32> *procs = &gameProcs;
            if( isBrowserProcess(pid) || isBrowserChildProcess(pid) )
                procs = &browserProcs;

            QSet<qint32>::iterator it = (*procs).find( pid );
            if( it == (*procs).end() )
                (*procs).insert( pid );
            else
                ORIGIN_ASSERT(false);
        }

        void ProcessList::remove( qint32 pid )
        {
            QSet<qint32>::iterator it = gameProcs.find( pid );
            if( it != gameProcs.end() )
                gameProcs.erase( it );
            else
            {
                it = browserProcs.find( pid );
                if( it != browserProcs.end() )
                    browserProcs.erase( it );
                else
                    ORIGIN_ASSERT(false);
            }
        }

        bool ProcessWin::waitForProcessGroup( HANDLE hProcessGroup, HANDLE hProcessThread, qint32 timeout)
        {
            bool			bRC = false;

            // Use Win32 JOB object under Windows
            HANDLE			hIOCP = 0;			// IO completion port handle.
            LPOVERLAPPED	pOverlapped;		// A dummy API parameter not used.
            ULONG_PTR		nCompletionKey;		// A dummy API parameter not used.

            if (!mJob && !isRunningUnderCider())
            {
                CreateJobObjectForMonitoring();
            }

            if (mJob)
            {
                BOOL processInJob = FALSE;

                // -------------------------------------
                // Assign the process group to the job.
                //
                // Note: Now optional on a best effort basis. Helps support the Guest account edge case as best we can
                if (( ::IsProcessInJob(hProcessGroup, mJob, &processInJob) && processInJob) ||
                    ::AssignProcessToJobObject( mJob, hProcessGroup ) )
                {
                    // ------------------------------
                    // Create an IO Completion Port.
                    //
                    if ( hIOCP = ::CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 ) )
                    {
                        JOBOBJECT_ASSOCIATE_COMPLETION_PORT portID; // IO completion port identification.

                        // Set up all the necessary information required to associate the job to the IO completion port.
                        portID.CompletionKey  = ( PVOID ) 0; // We don't use a completion key as we only have 1 job.
                        portID.CompletionPort = hIOCP;

                        // ------------------------------------------
                        // Assign the IO completion port to the job.
                        //

                        // This is needed if the process that is monitored wants to create a break away process.
                        // Like if EA Core calls EADM, which calls a process monitor (like this). The downside
                        // is that the process that broke away will not be monitored.
                        // JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobLimitInfo;
                        // jobLimitInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;	// Create process can do a BreakawayJob
                        // if ( !::SetInformationJobObject( mJob, JobObjectExtendedLimitInformation , &jobLimitInfo, sizeof( JOBOBJECT_EXTENDED_LIMIT_INFORMATION ) ) )
                        //	ORIGIN_ASSERT(false);

                        if ( !::SetInformationJobObject( mJob, JobObjectAssociateCompletionPortInformation, &portID, sizeof( portID ) ) )
                        {
                            ORIGIN_LOG_WARNING << L"Error: Could not assign the IO completion port to the job. LastError: " << GetLastError();
                            CloseHandle(hIOCP);
                            hIOCP = NULL;
                        }
                    }
                    else
                    {
                        ORIGIN_LOG_WARNING << L"Error: Could not create an IO completion port. LastError: " << GetLastError();
                    }
                }
                else
                {
                    ORIGIN_LOG_WARNING << L"Error: Unable to add project to JOB monitoring object. LastError: " << GetLastError();
                    cleanupJob();
                }
            }  // mJob

            // Note: Job monitoring or no we continue on
            // Resume suspended process
            if (hProcessThread)
            {
                ORIGIN_LOG_WARNING << L"Resuming new process main thread. ProcessWin: " << hProcessGroup << L"  Thread: " << hProcessThread;
                if (::ResumeThread(hProcessThread) == (DWORD) -1)
                {
                    ORIGIN_LOG_ERROR << L"Error resuming process " << hProcessGroup << L" thread " << hProcessThread << L". LastError: " << GetLastError();
                    return false;
                }
            }

            // --------------------------------------------------------------------------------
            // Wait for a notification that indicates the job object has zero active processes.
            // Note: Also monitor for server shutting down
            HANDLE ahWaitObjs[3];
            int iWaitObjCnt = 0;
            if (hIOCP)
                ahWaitObjs[iWaitObjCnt++] = hIOCP;
            else if (mJob)
                ahWaitObjs[iWaitObjCnt++] = mJob;
            else    // if  Win32 Job support failed, only monitor the main process
                ahWaitObjs[iWaitObjCnt++] = hProcessGroup;

            mMonitoringProc = true;

            QElapsedTimer timer;
            timer.start();
            while (-1 == timeout || timer.elapsed() < timeout)
            {
                qint32 dwResult = ::WaitForMultipleObjects(iWaitObjCnt, ahWaitObjs, FALSE, 200);
                if (dwResult == WAIT_OBJECT_0)
                {
                    DWORD nJobStatus = 0;

                    if (ahWaitObjs[0] != hIOCP)
                    {
                        // No Win32 Job support, so exit when main process exits
                        bRC = true;
                        break;
                    }
                    else if ( ::GetQueuedCompletionStatus( hIOCP, &nJobStatus, &nCompletionKey, &pOverlapped, 200 ) )
                    {
                        // Check over job status 
                        if ( nJobStatus == JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO)
                        {
                            //::MessageBox(NULL, L"Job reached 0 processes.", L"Job Info", MB_OK);
                            bRC = true;
                            break;
                        }
                        else if (nJobStatus == JOB_OBJECT_MSG_EXIT_PROCESS)
                        {
                            mGameProcessList.remove( (qint32)pOverlapped );
                            if( !mGameProcessList.isRunning() )
                            {
                                bRC = true;
                                break;
                            }
                            //::MessageBox(NULL, L"Job process exited.", L"Job Info", MB_OK);
                        }
                        else if (nJobStatus == JOB_OBJECT_MSG_NEW_PROCESS)
                        {
                            mGameProcessList.add( (qint32)pOverlapped );
                            //::MessageBox(NULL, L"ProcessWin added to job.", L"Job Info", MB_OK);
                        }
                    }
                    else if (WAIT_TIMEOUT == GetLastError())
                    {
                        JOBOBJECT_BASIC_ACCOUNTING_INFORMATION bai;

                        if (QueryInformationJobObject(mJob, JobObjectBasicAccountingInformation, &bai, sizeof(bai), NULL) && 0 == bai.ActiveProcesses)
                        {
                            bRC = true;
                            break;
                        }
                    }
                }
                else if (dwResult == WAIT_FAILED)
                {
                    ORIGIN_LOG_WARNING << L"Error: Could not wait for a job. LastError: " << GetLastError();
                    bRC = false;
                    break;
                }
                else if (dwResult == WAIT_TIMEOUT && mJob)
                {
                    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION bai;

                    if (QueryInformationJobObject(mJob, JobObjectBasicAccountingInformation, &bai, sizeof(bai), NULL) && 0 == bai.ActiveProcesses)
                    {
                        bRC = true;
                        break;
                    }
                }
            }

            //TODO add timeout notification

            mMonitoringProc = false;

            // Win32 Job Cleanup.
            if ( hIOCP )
            {
                ::CloseHandle( hIOCP );
            }

            cleanupJob();

            return bRC;
        }

        struct structMainWindowSearch
        {
            DWORD   PID;  // PID to find window for
            HWND	hWnd;	// Found handle of requested PID
        };

        HWND ProcessWin::getProcMainWindow(DWORD PID) const
        {
            if (0 == PID) return NULL;

            structMainWindowSearch oSrch;
            oSrch.PID = PID;
            oSrch.hWnd = NULL;
            ::EnumWindows(getProcMainWindowCallback, (LPARAM)&oSrch);
            return oSrch.hWnd;  // Success or failure
        }

        BOOL CALLBACK ProcessWin::getProcMainWindowCallback(HWND hWnd, LPARAM lParam)
        {
            if (!hWnd) 
                return TRUE;
            if (!::IsWindowVisible(hWnd))
                return TRUE;

            structMainWindowSearch* pSrch = (structMainWindowSearch*)lParam;

            // Invalid search request with missing PID?
            if (0 == pSrch->PID)
                return TRUE;

            DWORD dwWindowPID = 0;
            ::GetWindowThreadProcessId(hWnd, &dwWindowPID);
            if (dwWindowPID == pSrch->PID)
            {
                pSrch->hWnd = hWnd;
                return FALSE;  // Match found so stop searching
            }

            return TRUE; // No match so keep searching
        }

        bool ProcessWin::runProtocol(const QString& sFullPath, const QString& sCmdLineArgs, const QString& sCurrentDir)
        {
            LPCWSTR cmdLine = NULL;
            LPCWSTR currentDir = NULL;
            
            if(!sCmdLineArgs.isEmpty())
                cmdLine = sCmdLineArgs.utf16();

            if(!sCurrentDir.isEmpty())
                currentDir = sCurrentDir.utf16();

            return ::ShellExecute(NULL, L"open", sFullPath.utf16(), cmdLine, currentDir, SW_SHOW);
        }

        bool ProcessWin::removeEnvironmentVar (const QString sEnvironmentVar)
        {
            //with windows 8.1, if the environment var exists but is blank, we will still get an error code of ERROR_ENVVAR_NOT_FOUND so we cannot check against that to determine difference between existence and exist+blank
			SetEnvironmentVariable (reinterpret_cast<const wchar_t*>(sEnvironmentVar.utf16()), NULL);
		    return true;
        }

        void ProcessWin::addToEscalationKillList(qint32 processId)
        {
            //this function adds the process to a list kept by the escalationclient
            //if a process is in this list, it will be killed if Origin.exe is killed
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
            // Different operating systems behave differently with this call.  XP doesn't support PROCESS_QUERY_LIMITED_INFORMATION,
            // but in some situations Windows 7 fails
            if (!hProcess)
                hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);

            if(hProcess)
            {
                QString exeName;
                QString escalationReasonStr = QString("addToEscalationKillList (%1)").arg(processId);
                int escalationError = 0;
                QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;
                if (Origin::Escalation::IEscalationClient::quickEscalate(escalationReasonStr, escalationError, escalationClient))
                {
                    mGameProcessList.getExeNameFromPID(exeName, processId);
                    escalationClient->addToMonitorList(processId, exeName);
                }

                CloseHandle(hProcess);
            }
        }

        void ProcessWin::removeFromEscalationKillList(qint32 processId)
        {
            //this function removes the process from a list kept by the escalationclient
            //if a process is in this list, it will be killed if Origin.exe is killed
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
            // Different operating systems behave differently with this call.  XP doesn't support PROCESS_QUERY_LIMITED_INFORMATION,
            // but in some situations Windows 7 fails
            if (!hProcess)
                hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);

            if(hProcess)
            {
                QString escalationReasonStr = QString("removeFromEscalationKillList (%1)").arg(processId);
                int escalationError = 0;
                QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;
                if (Origin::Escalation::IEscalationClient::quickEscalate(escalationReasonStr, escalationError, escalationClient))
                {
                    escalationClient->removeFromMonitorList(processId); 
                }

                CloseHandle(hProcess);
            }
        }

        void ProcessWin::onOriginClientServicesMonitoringThreadFinished()
        {
            mOriginClientServicesMonitorThread = NULL;
        }

        void ProcessWin::cleanupJob()
        {
            mJobName.clear();
            if (mJob)
            {
                ::CloseHandle(mJob);
                mJob = NULL;
            }
        }

        HANDLE ProcessWin::OpenProcessForMonitoring(DWORD processId)
        {
            // Different operating systems behave differently with this call.  XP doesn't support PROCESS_QUERY_LIMITED_INFORMATION,
            // but in some situations Windows 7 fails if we don't specify it.  So we'll try different versions of this method
            // starting from the best case scenario down to "oh shit please just work" just to be sure.
            const DWORD preferredAccessFlags[] =
            {
                SYNCHRONIZE | PROCESS_QUERY_INFORMATION | PROCESS_SET_QUOTA | PROCESS_TERMINATE,
                SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_QUOTA | PROCESS_TERMINATE,
                SYNCHRONIZE | PROCESS_QUERY_INFORMATION,
                SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION,
                SYNCHRONIZE,
                0
            };

            HANDLE hProcess = NULL;
            for (const DWORD *flags = preferredAccessFlags; NULL == hProcess && *flags != 0; ++flags)
            {
                hProcess = ::OpenProcess(*flags, FALSE, processId);
            }

            return hProcess;
        }

        void ProcessWin::CreateJobObjectForMonitoring(const QString &jobName)
        {
            const wchar_t *rawJobName = NULL;
            if (!jobName.isEmpty())
            {
                rawJobName = jobName.utf16();
            }

            mJob = CreateJobObject(NULL, rawJobName);
            if (mJob)
            {
                JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli;

                ZeroMemory(&jeli, sizeof(jeli));
                jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_BREAKAWAY_OK;
                SetInformationJobObject(mJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));

                mJobName = jobName;
            }
            else
            {
                mJobName.clear();
            }
        }

        OriginClientServicesMonitoringThread::OriginClientServicesMonitoringThread(ProcessWin& process) 
        : QThread(&process)
        , mProcess(process)
        {
        }

        OriginClientServicesMonitoringThread::~OriginClientServicesMonitoringThread()
        {
        }
        void OriginClientServicesMonitoringThread::setGameStopped(bool stopped )
        {
            //function is called by process win to notify the monitoring thread that hte game stopped
            QMutexLocker locker(&mGameStopLock);
            mGameStopped = stopped;
        }

        void OriginClientServicesMonitoringThread::run()
        {
            {
                QMutexLocker locker(&mGameStopLock);
                mGameStopped = false;
            }

            QString escalationReasonStr = "OriginClientServicesMonitoringThread::run()";
            int escalationError = 0;
            QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;
            if (Origin::Escalation::IEscalationClient::quickEscalate(escalationReasonStr, escalationError, escalationClient))
            {
                qint32 escalationClientProcessId = escalationClient->getProcessId();
                if(escalationClientProcessId)
                {

                    //get a handle to the escalation client
                    HANDLE hProcess = ::OpenProcess(SYNCHRONIZE, FALSE, escalationClientProcessId);
                    if(hProcess)
                    {
                        //sit in this loop and monitor if the game stopped or the escalation client stopped
                        //if either happens we break and exit the thread
                        while(true)
                        {
                            {
                                QMutexLocker locker(&mGameStopLock);
                                if(mGameStopped)
                                    break;
                            }

                            //watching the escaltion process here
                            DWORD result = WaitForSingleObject(hProcess, 10);
                            if(result == WAIT_OBJECT_0)
                            {
                                mProcess.terminateNonEscalation();
                                break;
                            }
                        }
                        ::CloseHandle(hProcess);

                    }
                }
            }
        }
    }
}

#endif // Q_OS_WIN
