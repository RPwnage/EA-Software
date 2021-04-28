///////////////////////////////////////////////////////////////////////////////
// CommandProcessorWin.cpp
//
// Created by Paul Pedriana & Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <limits>
#include <QDateTime>

#include "qt_windows.h"
#include <AccCtrl.h>
#include <Aclapi.h>
#include <Sddl.h>
#include <Shlobj.h>

#include "CommandProcessorWin.h"
#include "ServiceUtilsWin.h"

namespace Origin
{

    namespace Escalation
    {
        static bool g_createOnUserSession = false;
        static quint32 g_originPid = 0;

        CommandProcessorWin::CommandProcessorWin(QObject* parent)
            : ICommandProcessor(parent)
        {
        }


        CommandProcessorWin::CommandProcessorWin(const QString& sCommandText, QObject* parent)
            : ICommandProcessor(sCommandText, parent)
        {
        }

        void CommandProcessorWin::setProcessExecutionMode(bool createOnUserSession)
        {
            g_createOnUserSession = createOnUserSession;
        }

        void CommandProcessorWin::setOriginPid(const quint32 pid)
        {
            g_originPid = pid;
        }


        bool CommandProcessorWin::isOriginUserAdmin()
        {
            BOOL isAdmin;

            OSVERSIONINFO verInfo;
            ZeroMemory(&verInfo, sizeof(OSVERSIONINFO));
            verInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
            GetVersionEx(&verInfo);


            SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
            PSID AdministratorsGroup; 
            isAdmin = AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                &AdministratorsGroup); 

            if(isAdmin) 
            {
                if (!CheckTokenMembership( NULL, AdministratorsGroup, &isAdmin)) 
                {
                    isAdmin = TRUE;
                } 
                FreeSid(AdministratorsGroup); 
            }

            return isAdmin;
        }

        int CommandProcessorWin::addToMonitorList()
        {
            if(mCommandArray.size() > 1)
            {
                // get the process handle to terminate
                const QString processIdString = ((mCommandArray.size() > 2) && !mCommandArray[1].isEmpty()) ? mCommandArray[1] : QString();
                const QString exeName = ((mCommandArray.size() > 2) && !mCommandArray[2].isEmpty()) ? mCommandArray[2] : QString();

                mResponseText = "addMonitorList," + processIdString +"," + exeName;
            }
            return 0;
        }

        int CommandProcessorWin::removeFromMonitorList()
        {
            if(mCommandArray.size() > 1)
            {
                // get the process handle to terminate
                const QString processIdString = ((mCommandArray.size() > 1) && !mCommandArray[1].isEmpty()) ? mCommandArray[1] : QString();

                mResponseText = "removeMonitorList," + processIdString;
            }
            return 0;
        }


        int CommandProcessorWin::closeProcess()
        {
            int result = 0;
            if(mCommandArray.size() > 1)
            {
                // get the process handle to terminate
                const QString windowHandleString = ((mCommandArray.size() > 1) && !mCommandArray[1].isEmpty()) ? mCommandArray[1] : QString();
  
                HWND windowsHandle = (HWND) windowHandleString.toInt();
  
                //try closing down the app nicely
                result = SendMessageTimeout(windowsHandle, WM_CLOSE, 0,0, SMTO_BLOCK, 5000, NULL);

                // write the result to the response text.
                mResponseText += QString::number(result, 16);
                mResponseText += '\n\0';
            }

            return result;
        }

        int CommandProcessorWin::injectIGOIntoProcess()
        {
            int result = -1;
            if(mCommandArray.size() > 1)
            {
                // get the process handle to inject to
                const QString processIdString = ((mCommandArray.size() > 1) && !mCommandArray[1].isEmpty()) ? mCommandArray[1] : QString();
                qint32 processId = processIdString.toInt();

                const QString threadIdString = ((mCommandArray.size() > 2) && !mCommandArray[2].isEmpty()) ? mCommandArray[2] : QString();
                qint32 threadId = threadIdString.toInt();
                
    #ifdef DEBUG
                const QString igoName = "igo32d.dll";
    #else
                const QString igoName = "igo32.dll";
    #endif
                HMODULE hIGO = GetModuleHandle(igoName.utf16());
                if (!hIGO)
                    hIGO = LoadLibrary(igoName.utf16());

                typedef bool (*InjectIGOFunc)(HANDLE, HANDLE);
                InjectIGOFunc InjectIGODll = hIGO!=NULL ? (InjectIGOFunc)GetProcAddress(hIGO, "InjectIGODll") : NULL;

                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_VM_OPERATION, FALSE, processId);
                if (hProcess == NULL)
                    result = -1;    // process not open/exisiting

                HANDLE hThread = (threadId != 0) ? OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadId) : NULL;
                // don't error out, if thread is not valid, we may not have a thread handle depending on the way the process was created

                if (InjectIGODll != NULL && hProcess != NULL && InjectIGODll(hProcess, hThread) == true)	// do the actual injection
                {
                    result = 0;     // everything went fine
                }
                else
                    result = -2;    // IGO not found or not injected

                if (hProcess != NULL)
                    CloseHandle(hProcess);

                if (hThread != NULL)
                    CloseHandle(hThread);

                // write the result to the response text.
                mResponseText += QString::number(result, 16);
                mResponseText += '\n\0';
            }

            return result;
        }

        int CommandProcessorWin::terminateProcess()
        {
            int result = 0;
            if(mCommandArray.size() > 1)
            {
                // get the process handle to terminate
                const QString processIdString = ((mCommandArray.size() > 1) && !mCommandArray[1].isEmpty()) ? mCommandArray[1] : QString();
                qint32 processId = processIdString.toInt();

                //forcibly close the app
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
                result = TerminateProcess(hProcess, 0xf291)?1:0;
                CloseHandle(hProcess);

                // write the result to the response text.
                mResponseText += QString::number(result, 16);
                mResponseText += '\n\0';
            }

            return result;
        }

        int CommandProcessorWin::executeProcess()
        {
            ProcessId processId = 0;
            bool bResult = false;
                        
            if(mCommandArray.size() > 1) // If there is a process path string at mCommandArray[1]...
            {
                bResult = false;

                QString pProcessPath         = ((mCommandArray.size() > 1) && !mCommandArray[1].isEmpty()) ? mCommandArray[1] : QString();
                QString pProcessArguments    = ((mCommandArray.size() > 2) && !mCommandArray[2].isEmpty()) ? mCommandArray[2] : QString();
                QString pProcessDirectory    = ((mCommandArray.size() > 3) && !mCommandArray[3].isEmpty()) ? mCommandArray[3] : QString();
                QString processEnvironment   = ((mCommandArray.size() > 4) && !mCommandArray[4].isEmpty()) ? mCommandArray[4] : QString();
                QString jobName              = (mCommandArray.size() > 5) ? mCommandArray[5] : QString();

				// Make sure we are using the correct path separators.  Origin itself always uses '/' for
				// portability, however, the SH* Win32 shell commands expect Win32-style '\' separators
				pProcessPath.replace("/","\\");
				pProcessDirectory.replace("/","\\");

                quint32 errorCode = 0;
                quint32 pid = 0;

                if (g_createOnUserSession)
                {
                    bResult = createProcessOnUserSession(pProcessPath, pProcessArguments, pProcessDirectory, processEnvironment, pid, errorCode);

                    if (!bResult)
                    {
                        mSystemError = errorCode;
                        doError(kCommandErrorCommandFailed, "CommandProcessorWin::ExecuteProcess: CreateProcessAsUserW failed", true);
                    }
                }
                else
                {
                    bResult = shellExecuteProcess(pProcessPath, pProcessArguments, pProcessDirectory, processEnvironment, pid, errorCode);

                    if (!bResult)
                    {
                        mSystemError = errorCode;
                        doError(kCommandErrorCommandFailed, "CommandProcessorWin::ExecuteProcess: ShellExecuteExW failed", true);
                    }
                }

                if (bResult)
                {
                    processId = pid;

                    // Use Win32 job object to track the entire process tree
                    if (!jobName.isEmpty())
                    {
                        Auto_HANDLE hJob, hProcess;

                        if (!(hJob = CreateJobObject(NULL, jobName.utf16())))
                        {
                            ORIGIN_LOG_WARNING << "Unable to open job " << jobName;
                        }
                        else if (!(hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, processId)))
                        {
                            ORIGIN_LOG_WARNING << "Unable to open process to add to job " << jobName;
                        }
                        else if (!::AssignProcessToJobObject(hJob, hProcess))
                        {
                            ORIGIN_LOG_WARNING << L"Error: Unable to add process to job monitoring object. LastError: " << GetLastError();
                        }
                    }
                }
            }
            else
                doError(kCommandErrorInvalidCommand, "CommandProcessor::ExecuteProcess: Invalid command.", false);

            if(bResult)
                doSuccess();

            // Write the process id result to the response text.
            mResponseText += QString::number(processId, 16);
            mResponseText += '\n\0';
            return mError;
        }

        bool CommandProcessorWin::shellExecuteProcess(QString pProcessPath, QString pProcessArguments, QString pProcessDirectory, QString processEnvironment, quint32& pid, quint32& errorCode)
        {
            // We use ShellExecuteEx instead of the lower level CreateProcess function.
            // The primarty difference is that the ShellExecuteEx function lets us launch
            // files and URLs in addition to processes themselves. In the case of files and
            // URLS the returned process id might be 0, even though the execution succeeded.

            SHELLEXECUTEINFOW shellExecuteInfo;
            memset(&shellExecuteInfo, 0, sizeof(shellExecuteInfo));

            shellExecuteInfo.cbSize       = sizeof(shellExecuteInfo);
            shellExecuteInfo.lpFile       = (LPCWSTR)pProcessPath.utf16();
            shellExecuteInfo.lpParameters = (LPCWSTR)pProcessArguments.utf16();
            shellExecuteInfo.lpDirectory  = (LPCWSTR)pProcessDirectory.utf16();
            shellExecuteInfo.lpVerb       = L"open";
            shellExecuteInfo.hwnd         = NULL;
            shellExecuteInfo.fMask        = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI; // Indicates that the hProcess value should be set.
            shellExecuteInfo.nShow        = SW_SHOWNORMAL;

            // Parse the environment strings (passed in separated by 0xff chars) and set each variable one by one
            QStringList vars = processEnvironment.split(0xff);
            for(QStringList::iterator it = vars.begin(); it != vars.end(); it++)
            {
                QString sVarLine = *it;
                QStringList keyValueArray = sVarLine.split('=');
                if (keyValueArray.size() == 2)
                {
                    QString sKey = keyValueArray[0];
                    QString sValue = keyValueArray[1];
                    SetEnvironmentVariable(reinterpret_cast<const wchar_t*>(sKey.utf16()), reinterpret_cast<const wchar_t*>(sValue.utf16()));
                }
                else
                {
                    ORIGIN_LOG_ERROR << "Couldn't split environment key/value pair from:" << sVarLine;
                }
            }

            bool bResult = (ShellExecuteExW(&shellExecuteInfo) != 0);

            if(bResult)
            {
                if(shellExecuteInfo.hProcess)
                {
                    // We need to loosen up the security DACL for the process so anybody can monitor it
                    UpdateProcessDACL(shellExecuteInfo.hProcess);

                    pid = GetProcessId(shellExecuteInfo.hProcess);
                }
            }
            else
            {
                errorCode = GetLastError();
            }

            return bResult;
        }

        bool CommandProcessorWin::createProcessOnUserSession(QString pProcessPath, QString pProcessArguments, QString pProcessDirectory, QString processEnvironment, quint32& pid, quint32& errorCode)
        {
            if (!LaunchProcessElevatedFromService(g_originPid, pProcessPath, pProcessArguments, pProcessDirectory, processEnvironment, pid))
            {
                errorCode = GetLastError();
                return false;
            }
            return true;
        }

        bool CommandProcessorWin::setDirectoryPermissions(const QString& sFile, SECURITY_DESCRIPTOR* pSD, int &errCode)
        {
            PACL pACL = NULL;
            BOOL bDACLPresent = FALSE;
            BOOL bDACLDefaulted = FALSE;
            DWORD dwRes;

            if (GetSecurityDescriptorDacl(pSD, &bDACLPresent, &pACL, &bDACLDefaulted) == FALSE)
            {
                errCode = (int)GetLastError();
                ORIGIN_LOG_ERROR << "Failed to get Dacl from Security Descriptor";
                return false;
            }

            // Try to modify the object's DACL.
            dwRes = SetNamedSecurityInfo(
                (LPWSTR) sFile.utf16(),      // name of the object
                SE_FILE_OBJECT,              // type of object
                DACL_SECURITY_INFORMATION,   // change only the object's DACL
                NULL, NULL,                  // do not change owner or group
                pACL,                        // DACL specified
                NULL);                       // do not change SACL

            errCode = (int)dwRes;

            if (dwRes == ERROR_SUCCESS) 
            {
                ORIGIN_LOG_DEBUG << "Successfully changed DACL";
                return true;
            }

            ORIGIN_LOG_ERROR << "SetNamedSecurityInfo failed: " << dwRes; 

            return false;
        }


        namespace inner
        {
            HKEY hkey(const QString& registryKey)
            {

                bool isConvOk = false;
                const static int BASE = 16;
                const unsigned long long myVal = registryKey.toULongLong(&isConvOk, BASE);
                if (!isConvOk)
                {
                    return NULL;
                }

                switch (myVal)
                {
                case 0x80000000:
                    return HKEY_CLASSES_ROOT;
                    break;

                case 0x80000001:
                    return HKEY_CURRENT_USER;
                    break;

                case 0x80000002:
                    return HKEY_LOCAL_MACHINE;
                    break;

                case 0x80000003:
                    return HKEY_USERS;
                    break;

                case 0x80000005:
                    return HKEY_CURRENT_CONFIG;
                    break;

                default:
                    return NULL;
                    break;
                }
            }

            // returns true if we will be able to store then new value in the registry
            bool isCapableOfNewValue(const DWORD myValueSize)
            {
                DWORD quotaAllowed = 0;
                DWORD quotaUsed = 0;

                if (GetSystemRegistryQuota(&quotaAllowed, &quotaUsed))
                {
                    if (quotaAllowed > quotaUsed + myValueSize)
                    {
                        return true;
                    }
                }
                return false;
            }

            int stringToValType(const QString& valType)
            {
                if ("REG_EXPAND_SZ"==valType)
                {
                    return REG_EXPAND_SZ;
                } 
                else if ("REG_LINK"==valType)
                {
                    return REG_LINK;
                }
                else if("REG_MULTI_SZ"==valType)
                {
                    return REG_MULTI_SZ;
                }
                else if ("REG_DWORD"==valType)
                {
                    return REG_DWORD;
                }
                else if ("REG_DWORD_BIG_ENDIAN"==valType)
                {
                    return REG_DWORD_BIG_ENDIAN;
                }
                else if ("REG_QWORD"==valType)
                {
                    return REG_QWORD;
                }
                else
                {
                    return REG_SZ;
                }
            }

            int openOrCreateKeyForWriting(HKEY& myRegistryKey, HKEY& key, const QString& myRegistrySubKey)
            {
                int iResult = RegOpenKeyEx(
                      myRegistryKey
                    , myRegistrySubKey.toStdWString().c_str()
                    , 0
                    , KEY_ALL_ACCESS|KEY_WOW64_32KEY
                    , &key);

                if (ERROR_SUCCESS != iResult) // key doesn't exist? Try creating it.
                {
                    DWORD disposition = 0;
                    iResult = RegCreateKeyEx(
                        myRegistryKey,
                        myRegistrySubKey.toStdWString().c_str(),
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS|KEY_WOW64_32KEY,
                        NULL,
                        &key,
                        &disposition
                        );
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                    ORIGIN_LOG_EVENT << "RegCreateKeyEx [" << iResult << " : " << disposition << "]";
#endif
                }
                return iResult ;
            }

            int setRegistryValue(const QStringList& commandArray, DWORD myValueSize)
            {
                if (!isCapableOfNewValue(myValueSize))
                {
                    return ERROR_REGISTRY_QUOTA_LIMIT;
                }

                HKEY myRegistryKey = hkey(commandArray[RegistryKey]);
                HKEY key = NULL;
                QString myRegistrySubKey = commandArray[RegistrySubKey];

                int iResult = openOrCreateKeyForWriting(myRegistryKey, key, myRegistrySubKey);

                if (ERROR_SUCCESS == iResult)
                {
                    QString myValueName = commandArray[RegistryValueName];
                    QString myValType = commandArray[RegistryValueType];

                    int valType = stringToValType(myValType);
                    switch (valType)
                    {
                    case REG_SZ:
                    case REG_EXPAND_SZ:
                    case REG_LINK:
                    case REG_MULTI_SZ:
                    default:
                        {
                            QString myValue = commandArray[RegistryValue];
                            iResult = RegSetValueEx(
                                key
                                , myValueName.toStdWString().c_str() 
                                , 0
                                , valType
                                , (BYTE*) myValue.toStdWString().c_str()
                                , myValueSize
                                );
                        } 
                    	break;
                    case REG_DWORD:
                    case REG_DWORD_BIG_ENDIAN:
                        {
                            DWORD myValue = commandArray[RegistryValue].toInt();
                            iResult = RegSetValueEx(
                                key
                                , myValueName.toStdWString().c_str() 
                                , 0
                                , valType
                                , reinterpret_cast<BYTE*>(&myValue)
                                , sizeof(myValue)
                                );
                        }
                        break;
                    case REG_QWORD:
                        {
                            __int64 myValue = commandArray[RegistryValue].toLongLong();
                            iResult = RegSetValueEx(
                                key
                                , myValueName.toStdWString().c_str() 
                                , 0
                                , valType
                                , reinterpret_cast<BYTE*>(&myValue)
                                , sizeof(myValue)
                                );
                        }
                        break;
                    }
                    RegCloseKey(key);
                }
                return iResult;
            }

            DWORD size(const QString& valueType)
            {
                if ("REG_DWORD" == valueType || "REG_DWORD_BIG_ENDIAN" == valueType)
                {
                    return sizeof(DWORD);
                } 
                else if ("REG_QWORD"==valueType)
                {
                    return 64;
                } 
                else
                {
                    return sizeof(PDWORD);
                } 
            }
        }

        QString CommandProcessorWin::systemError()
        {
            QString errorText;
            getSystemErrorText(mSystemError, errorText);
            return errorText;
        }

        void CommandProcessorWin::setRegistryValuePostProcess()
        {
            if(mSystemError==ERROR_SUCCESS)
            {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                ORIGIN_LOG_EVENT << "Successfully written registry key[" << mCommandArray[RegistryValueName] << " : " << mCommandArray[RegistryValue] << "]";
#endif
                doSuccess();
            }
            else
            {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                ORIGIN_LOG_ERROR << "Failed to write registry keys in escalation service, error [" << mError << "] system error [" << mSystemError <<"] : " << systemError();
#endif
                doError(kCommandErrorCommandFailed, "CommandProcessorWin::setRegistrySoftwareOriginValue: error writing to Registry.", true);
            }
        }

        int CommandProcessorWin::setRegistryStringValue()
        {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
            ORIGIN_LOG_EVENT << "CommandProcessorWin::setRegistryStringValue()";
#endif

            if(mCommandArray.size() > 0)
            {
                QString myValue = mCommandArray[RegistryValue];
                DWORD myValueSize = wcslen(myValue.toStdWString().c_str()) * sizeof(wchar_t);
                mSystemError = inner::setRegistryValue(mCommandArray, myValueSize);
                setRegistryValuePostProcess();
            }
            else
            {
                doError(kCommandErrorInvalidCommand, "CommandProcessorWin::setRegistryStringValue: Invalid command.", true);
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                ORIGIN_LOG_ERROR << "Failed to write registry keys in escalation service, error was invalid command.";
#endif
            }

            // We are done and don't need to add any additional data to the response.
            mResponseText += '\0';
            return mError;
        }

        int CommandProcessorWin::setRegistryIntValue()
        {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
            ORIGIN_LOG_EVENT << "CommandProcessorWin::setRegistryIntValue()";
#endif

            if(mCommandArray.size() > 0)
            {
                QString myValue = mCommandArray[RegistryValue];
                DWORD myValueSize = inner::size(mCommandArray[RegistryValueType]);
                mSystemError = inner::setRegistryValue(mCommandArray, myValueSize);
                setRegistryValuePostProcess();
            }
            else
            {
                doError(kCommandErrorInvalidCommand, "CommandProcessorWin::setRegistryIntValue: Invalid command.", true);
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                ORIGIN_LOG_ERROR << "Failed to write registry keys in escalation service, error was invalid command.";
#endif
            }

            // We are done and don't need to add any additional data to the response.
            mResponseText += '\0';
            return mError;
        }

        int CommandProcessorWin::deleteRegistrySubKey()
        {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
            ORIGIN_LOG_EVENT << "CommandProcessorWin::deleteRegistrySubKey() NOT IMPLEMENTED";
#endif

#if 1 // fix warnings about unreachable code in the #else section 
            return mError;
#else
            if(mCommandArray.size() > 0)
            {
                HKEY myRegistryKey = inner::hkey(mCommandArray[RegistryKey]);
                HKEY key = NULL;
                QString myRegistrySubKey = mCommandArray[RegistrySubKey];

                mSystemError = inner::openOrCreateKeyForWriting(myRegistryKey, key, myRegistrySubKey);

                if (ERROR_SUCCESS == mSystemError)
                {
                    mSystemError = RegDeleteKeyEx(
                        key
                        , myRegistrySubKey.toStdWString().c_str()
                        ,KEY_WOW64_64KEY
                        ,0
                        );
                }
                setRegistryValuePostProcess();
            }
            else
            {
                doError(kCommandErrorInvalidCommand, "CommandProcessorWin::deleteRegistrySubKey: Invalid command.", true);
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                ORIGIN_LOG_ERROR << "Failed to delete registry subkey in escalation service, error was invalid command.";
#endif
            }

            // We are done and don't need to add any additional data to the response.
            mResponseText += '\0';
            return mError;
#endif
        }
        // CreateDirectory
        //
        // As of this writing, CreateDirectory doesn't have the ability to build a directory tree.
        // It can only create a single leaf node. To build a tree currently requires multiple calls 
        // to CreateDirectory to build each part of it.
        //
        int CommandProcessorWin::createDirectory()
        {
            if(mCommandArray.size() > 1) // If there is a directory path string at mCommandArray[1]...
            {
                int iResult = 0;

				// Make sure we are using the correct path separators.  Origin itself always uses '/' for
				// portability, however, the SH* Win32 shell commands expect Win32-style '\' separators
				QString dirPath = mCommandArray[1].replace("/","\\");

                // We have a small problem if we are trying to create a directory that already exists
                // but has the wrong access rights. In that case we would need to update the access
                // rights to those specified by the caller.

                if(mCommandArray.size() > 2) // If the user is specifying an access list string, use it.
                {
                    const QString & pAccessControlList = mCommandArray[2];

                    SECURITY_ATTRIBUTES sa;
                    memset(&sa, 0, sizeof(sa));

                    sa.nLength        = sizeof(SECURITY_ATTRIBUTES);
                    sa.bInheritHandle = FALSE;  

                    ConvertStringSecurityDescriptorToSecurityDescriptor(pAccessControlList.utf16(), SDDL_REVISION_1, &sa.lpSecurityDescriptor, NULL);

                    iResult = ::SHCreateDirectoryEx(NULL, dirPath.utf16(), &sa);
                    if (iResult == ERROR_ALREADY_EXISTS)
                    {
                        // If the folder already exists make sure the permissions are set according to the passed DACL
                        if (!setDirectoryPermissions(dirPath, (SECURITY_DESCRIPTOR*) sa.lpSecurityDescriptor, iResult))
                        {
                            // Not erroring out to avoid any potential knock-ons
                            ORIGIN_LOG_WARNING << "CommandProcessorWin::createDirectory: Folder exists but can't set requisite permissions for folder.";
                        }
                    }

                    if(sa.lpSecurityDescriptor) // This should always be true.
                        LocalFree(sa.lpSecurityDescriptor);
                }
                else
                    iResult = ::SHCreateDirectoryEx(NULL, dirPath.utf16(), NULL);

                if(iResult==ERROR_SUCCESS || iResult==ERROR_ALREADY_EXISTS)
                {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                    ORIGIN_LOG_EVENT << "Successfully created directory [" << dirPath << "]";
#endif
                    doSuccess();
                }
                else
                {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                    ORIGIN_LOG_ERROR << "Failed to create directory or set permissions in escalation service, error was: " << iResult;
#endif
                    mSystemError = iResult;
                    doError(kCommandErrorCommandFailed, "CommandProcessorWin::CreateDirectory: Creation failure.", true);
                }
            }
            else
            {
                doError(kCommandErrorInvalidCommand, "CommandProcessorWin::CreateDirectory: Invalid command.", true);
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                ORIGIN_LOG_ERROR << "Failed to create directory in escalation service, error was invalid command.";
#endif
            }

            // We are done and don't need to add any additional data to the response.
            mResponseText += '\0';
            return mError;
        }
        
        // This command is currently only used on Mac to delete Access licenses from game bundles.
        int CommandProcessorWin::deleteBundleLicense()
        {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
            ORIGIN_LOG_ERROR << "DeleteBundleLicense command is not supported on Windows";
#endif
            doError(kCommandErrorInvalidCommand, "CommandProcessorWin::DeleteBundleLicense: Not supported.", false);
            
            mResponseText += '\0';
            return mError;
        }

		// This command is currently only used on Mac to run compiled AppleScript files.
		int CommandProcessorWin::runScript()
		{
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
			ORIGIN_LOG_ERROR << "RunScript command is not supported on Windows";
#endif
            doError(kCommandErrorInvalidCommand, "CommandProcessorWin::RunScript: Not supported.", false);

			mResponseText += '\0';
			return mError;
		}

		// This command is currently only used on Mac to enable assistive devices.
		int CommandProcessorWin::enableAssistiveDevices()
		{
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
			ORIGIN_LOG_ERROR << "EnableAssistiveDevices command is not supported on Windows";
#endif
            doError(kCommandErrorInvalidCommand, "CommandProcessorWin::EnableAssistiveDevices: Not supported.", false);

			mResponseText += '\0';
			return mError;
		}

		int CommandProcessorWin::deleteDirectory()
		{
            if(mCommandArray.size() > 1) // If there is a directory path string at mCommandArray[1]...
            {
                int iResult = 0;

				// Make sure we are using the correct path separators.  Origin itself always uses '/' for
				// portability, however, the SH* Win32 shell commands expect Win32-style '\' separators
				QString dirPath = mCommandArray[1].replace("/","\\");

                // SHFileOperation expects double-NULL terminated wide string.                
                const long UNICODE_MAX_PATH = 32767;
                wchar_t path[UNICODE_MAX_PATH];
                std::wstring tempPath = dirPath.toStdWString();
                wcscpy_s(path, UNICODE_MAX_PATH, tempPath.c_str());
                path[tempPath.size()] = '\0';
                path[tempPath.size() + 1] = '\0';

                SHFILEOPSTRUCT sfo = {0};
	
                sfo.hwnd = NULL;
                sfo.wFunc = FO_DELETE;
                sfo.pFrom = path;
                sfo.pTo = L"";
                sfo.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_MULTIDESTFILES | FOF_SILENT;
                sfo.lpszProgressTitle = L"";

                iResult = SHFileOperation(&sfo);

                if(iResult==ERROR_SUCCESS)
                {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                    ORIGIN_LOG_EVENT << "Successfully deleted directory [" << dirPath << "]";
#endif
                    doSuccess();
                }
                else
                {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                    ORIGIN_LOG_ERROR << "Failed to delete directory in escalation service, error was: " << iResult;
#endif
                    mSystemError = iResult;
                    doError(kCommandErrorCommandFailed, "CommandProcessorWin::DeleteDirectory: Deletion failure.", true);
                }
            }
            else
            {
                doError(kCommandErrorInvalidCommand, "CommandProcessorWin::DeleteDirectory: Invalid command.", true);
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                ORIGIN_LOG_ERROR << "Failed to delete directory in escalation service, error was invalid command.";
#endif
            }

            // We are done and don't need to add any additional data to the response.
            mResponseText += '\0';
            return mError;
		}
        
       void CommandProcessorWin::getSystemErrorText(int systemErrorId, QString& errorText)
        {
            wchar_t* pTemp    = NULL;
            DWORD    dwResult = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                NULL, systemErrorId, LANG_NEUTRAL, (wchar_t*)&pTemp, 0, NULL);
            if(dwResult && pTemp)
            {
                // Remove any trailing newline characters.
                while(!errorText.isEmpty() && ((errorText[errorText.size() - 1] == '\r') || (errorText[errorText.size() - 1] == '\n')))
                    errorText.resize(errorText.size() - 1);
            }

            if(pTemp)
                LocalFree((HLOCAL)pTemp);
        }

    } // namespace Escalation
} // namespace Origin

