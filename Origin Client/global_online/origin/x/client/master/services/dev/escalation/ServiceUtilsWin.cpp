#include "ServiceUtilsWin.h"

#include "services/log/LogService.h"

#include <AccCtrl.h>
#include <AclAPI.h>
#include <TlHelp32.h>
#include <UserEnv.h>

#define ARGS_BUFFER_SIZE 1024

namespace Origin
{
    namespace Escalation
    {
        // Internal-only C++ helpers to make using the Win32 functions not so painful
        class AutoSC_HANDLE
        {
        public:
            AutoSC_HANDLE(SC_HANDLE handle) : _handle(handle) 
            { 
            }
            ~AutoSC_HANDLE() 
            {
                if (_handle)
                {
                    CloseServiceHandle(_handle);
                }
            }

            operator SC_HANDLE()
            {
                return _handle;
            }

            AutoSC_HANDLE& operator=(const SC_HANDLE rhs)
            {
                _handle = rhs;
                return *this;
            }

        private:
            SC_HANDLE _handle;
        };

        class Auto_TCBPrivilegeManager
        {
        public:
            Auto_TCBPrivilegeManager() : _acquired(false)
            {
                Auto_HANDLE ourToken;
                if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &ourToken))
                {
                    return;
                }

                if (!SetTokenPrivilege(ourToken, SE_TCB_NAME, true))
                {
                    return;
                }

                //ORIGIN_LOG_EVENT << "TCB Privilege Acquired.";
                _acquired = true;
            }

            ~Auto_TCBPrivilegeManager()
            {
                if (_acquired)
                {
                    Auto_HANDLE ourToken;
                    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &ourToken))
                    {
                        return;
                    }

                    if (!SetTokenPrivilege(ourToken, SE_TCB_NAME, false))
                    {
                        return;
                    }

                    //ORIGIN_LOG_EVENT << "TCB Privilege Released.";
                }
            }

            bool acquired()
            {
                return _acquired;
            }
        private:
            bool _acquired;
        };

        template <class T>
        class AutoBuffer
        {
        public:
            AutoBuffer(size_t size) : _ptr(NULL)
            {
                _ptr = (T*) new char[size];
                if (_ptr)
                {
                    ZeroMemory(_ptr, size);
                }
            }
            ~AutoBuffer()
            {
                if (_ptr)
                {
                    delete [] _ptr;
                }
            }

            operator T*()
            {
                return _ptr;
            }

            T* operator->()
            {
                return _ptr;
            }

            T* data()
            {
                return _ptr;
            }
        private:
            T* _ptr;
        };

        template <class T>
        class AutoLocalHeap
        {
        public:
            AutoLocalHeap() : _ptr(0) { }
            AutoLocalHeap(T p) : _ptr(p) { }
            ~AutoLocalHeap()
            {
                if (_ptr)
                {
                    LocalFree(_ptr);
                }
            }

            T* operator&()
            {
                return &_ptr;
            }

            operator T()
            {
                return _ptr;
            }

            T data()
            {
                return _ptr;
            }
        private:
            T _ptr;
        };

        bool IsWindowsXP()
        {
            OSVERSIONINFOEX  osvi;
            ZeroMemory( &osvi, sizeof( OSVERSIONINFOEX ) );
            osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
            if (::GetVersionEx((OSVERSIONINFO*)&osvi))
            {
                if (osvi.dwMajorVersion >= 6)
                    return false;
            }
            return true;
        }

        bool QueryOriginServiceStatusInternal(QString serviceName, int serviceState, SERVICE_STATUS_PROCESS& status)
        {
            bool result = false;
            AutoSC_HANDLE hScm = OpenSCManager(0, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ENUMERATE_SERVICE);
            if (hScm == 0) 
            {
                ORIGIN_LOG_ERROR << "Error opening SCM.  Error: " << GetLastError();
                return result;
            }
            DWORD servicesBufferRequired = 0;
            DWORD resumeHandle = 0;

            DWORD servicesBufferSize = 0;
            DWORD servicesCount = 0;

            // Call once to determine proper buffer sizing
            EnumServicesStatusEx(hScm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, serviceState, 0, 0, &servicesBufferRequired, &servicesCount, &resumeHandle, 0);

            AutoBuffer<ENUM_SERVICE_STATUS_PROCESS> servicesBuffer(servicesBufferRequired);
            servicesBufferSize = servicesBufferRequired;
            if (EnumServicesStatusEx(hScm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, serviceState, (LPBYTE) servicesBuffer.data(), servicesBufferSize, &servicesBufferRequired, &servicesCount, &resumeHandle, 0))
            {
                ENUM_SERVICE_STATUS_PROCESS* servicesBufferPtr = servicesBuffer;
                while (servicesCount--) 
                {
                    if (_wcsicmp(serviceName.utf16(), servicesBufferPtr->lpServiceName) == 0)
                    {
                        result = true;

                        status = servicesBufferPtr->ServiceStatusProcess;

                        break;
                    }
                    servicesBufferPtr++;
                }

                if (!result)
                {
                    ORIGIN_LOG_EVENT << serviceName << " not found in services database.";
                }
            }
            else
            {
                ORIGIN_LOG_ERROR << "EnumServicesStatusEx failed, Service: " << serviceName << " error: " << GetLastError();
            }

            return result;
        }

        bool QueryOriginServiceStatus(QString serviceName, bool& running)
        {
            SERVICE_STATUS_PROCESS status = { 0 };
            if (QueryOriginServiceStatusInternal(serviceName, SERVICE_STATE_ALL, status))
            {
                if (status.dwCurrentState == SERVICE_RUNNING)
                {
                    running = true;
                }
                else
                {
                    running = false;
                }
                return true;
            }
            return false;
        }

        bool IsUserInteractive()
        {
           bool bIsUserInteractive = true;

           HWINSTA hWinStation = GetProcessWindowStation();
           if (hWinStation != NULL)
           {     
             USEROBJECTFLAGS uof = {0};     
             if (GetUserObjectInformation(hWinStation, UOI_FLAGS, &uof, sizeof(USEROBJECTFLAGS), NULL) && ((uof.dwFlags & WSF_VISIBLE) == 0))
             {
                bIsUserInteractive = false;
             }     
           }
           return bIsUserInteractive;
        }

        bool RegisterService(QString serviceName, QString executablePath, bool useLocalSystemAccount, bool autoStart)
        {
            static wchar_t s_localServiceStartName[] = L"NT AUTHORITY\\LocalService";

            // Enquote the executable path
            // This is critical for security because of the way that Windows evaluates paths with spaces
            // For example, "C:\Program Files\Origin\OriginClientService.exe" is interpreted as, in this order:
            // - C:\Program.exe
            // - C:\Program Files\Origin\OriginClientService.exe
            // This is a security vulnerability because if somebody can create C:\Program.exe, they can run it with administrative rights
            //
            executablePath = QString("\"%1\"").arg(executablePath);

            LPWSTR serviceStartName = NULL;
            if (!useLocalSystemAccount)
            {
                serviceStartName = s_localServiceStartName;
            }

            DWORD serviceStartType = SERVICE_DEMAND_START;
            if (autoStart)
            {
                serviceStartType = SERVICE_AUTO_START;
            }

            AutoSC_HANDLE hScm = OpenSCManager(0, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);
            if (!hScm) 
            {
                ORIGIN_LOG_ERROR << "Error opening SCM.  Error: " << GetLastError();
                return false;
            }

            bool wasAlreadyInstalled = true;
            AutoSC_HANDLE hService = OpenService(hScm, serviceName.utf16(), SERVICE_ALL_ACCESS);
            if (!hService)
            {
                if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
                {
                    wasAlreadyInstalled = false;

                    ORIGIN_LOG_EVENT << serviceName << " not registered in SCM database.";

                    hService = CreateService(hScm, serviceName.utf16(), serviceName.utf16(), SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, serviceStartType, SERVICE_ERROR_NORMAL, executablePath.toStdWString().c_str(), NULL, NULL, NULL, serviceStartName, NULL);
                    if (!hService)
                    {
                        ORIGIN_LOG_ERROR << "Unable to create service.  Error: " << GetLastError();
                        return false;
                    }

                    ORIGIN_LOG_EVENT << "Service registered successfully in SCM database.";
                }
                else
                {
                    ORIGIN_LOG_ERROR << "Error opening service: " << serviceName << "  Error: " << GetLastError();
                    return false;
                }
            }

            if (wasAlreadyInstalled)
            {
                DWORD bytesNeededQueryStatus = 0;
                QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, NULL, 0, &bytesNeededQueryStatus);
                if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                {
                    ORIGIN_LOG_ERROR << "Unable to query service status. Service: " << serviceName << " Error: " << GetLastError();
                    return false;
                }

                AutoBuffer<SERVICE_STATUS_PROCESS> serviceStatus(bytesNeededQueryStatus);
                if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)serviceStatus.data(), bytesNeededQueryStatus, &bytesNeededQueryStatus))
                {
                    ORIGIN_LOG_ERROR << "Unable to query service status. Service: " << serviceName << " Error: " << GetLastError();
                    return false;
                }

                // If the service is running, we can't go changing it around
                if (serviceStatus->dwCurrentState != SERVICE_STOPPED)
                {
                    ORIGIN_LOG_ERROR << "Cannot install or update config, Service is running.  Service: " << serviceName;
                    return false;
                }

                DWORD bytesNeededQueryConfig = 0;
                QueryServiceConfig(hService, NULL, 0, &bytesNeededQueryConfig);
                if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                {
                    ORIGIN_LOG_ERROR << "Unable to query service config. Service: " << serviceName << " Error: " << GetLastError();
                    return false;
                }

                AutoBuffer<QUERY_SERVICE_CONFIG> serviceConfig(bytesNeededQueryConfig);
                if (!QueryServiceConfig(hService, serviceConfig.data(), bytesNeededQueryConfig, &bytesNeededQueryConfig))
                {
                    ORIGIN_LOG_ERROR << "Unable to query service config. Service: " << serviceName << " Error: " << GetLastError();
                    return false;
                }

                QString existingServiceBinPath = QString::fromWCharArray(serviceConfig->lpBinaryPathName);
                if (existingServiceBinPath.compare(executablePath, Qt::CaseInsensitive) != 0)
                {
                    ORIGIN_LOG_EVENT << "Already installed service bin path did not match current executable path.  Updating bin path.";
                    ORIGIN_LOG_EVENT << "Installed path: " << existingServiceBinPath;
                    ORIGIN_LOG_EVENT << "New path: " << executablePath;

                    WCHAR newBinPath[MAX_PATH+1];
                    ZeroMemory(newBinPath, sizeof(newBinPath));
                    if (executablePath.length() > MAX_PATH)
                    {
                        ORIGIN_LOG_ERROR << "Invalid executable path.  Exceeds MAX_PATH.";
                        return false;
                    }
                    executablePath.toWCharArray(newBinPath);

                    if (!ChangeServiceConfig(hService, SERVICE_WIN32_OWN_PROCESS, serviceStartType, SERVICE_ERROR_NORMAL, newBinPath, NULL, NULL, NULL, NULL, NULL, NULL))
                    {
                        ORIGIN_LOG_ERROR << "Unable to change service config. Service: " << serviceName << "  Error: " << GetLastError();
                        return false;
                    }

                    ORIGIN_LOG_EVENT << "Service bin path config updated.";
                }
            }

            // Blank out the bytes required
            DWORD bytesRequired = 0;
            QueryServiceObjectSecurity(hService, DACL_SECURITY_INFORMATION, NULL, 0, &bytesRequired);
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            {
                ORIGIN_LOG_ERROR << "Unable to query service object security. Service: " << serviceName << " Error: " << GetLastError();
                return false;
            }
    
            AutoBuffer<SECURITY_DESCRIPTOR> securityDescriptor(bytesRequired);

            if (!QueryServiceObjectSecurity(hService, DACL_SECURITY_INFORMATION, securityDescriptor, bytesRequired, &bytesRequired))
            {
                ORIGIN_LOG_ERROR << "Unable to query service object security. Service: " << serviceName << " Error: " << GetLastError();
                return false;
            }
    
            AutoLocalHeap<LPWSTR> outputStr;
            ULONG len = 0;
            if (!ConvertSecurityDescriptorToStringSecurityDescriptor(securityDescriptor, SDDL_REVISION_1, DACL_SECURITY_INFORMATION, &outputStr, &len))
            {
                ORIGIN_LOG_ERROR << "Unable to convert security descriptor to string. Service: " << serviceName << "  Error: " << GetLastError();
                return false;
            }

            ORIGIN_LOG_EVENT << "Read Service Existing SDDL: " << outputStr;

            // Add Start/Stop/Control permissions for built-in users
            QString sddl = QString::fromUtf16(outputStr);
            QString sddlToAdd = "(A;;RPWPCR;;;BU)";
            if (!sddl.contains(sddlToAdd))
            {
                sddl.append(sddlToAdd);

                ORIGIN_LOG_EVENT << "Amended Service SDDL: " << sddl;

                AutoLocalHeap<PSECURITY_DESCRIPTOR> securityDescriptorAmended;
                ULONG securityDescriptorAmendedLen = 0;
                if (!ConvertStringSecurityDescriptorToSecurityDescriptor(sddl.toStdWString().c_str(), SDDL_REVISION_1, &securityDescriptorAmended, &securityDescriptorAmendedLen))
                {
                    ORIGIN_LOG_ERROR << "Unable to convert SDDL string to security descriptor. Service: " << serviceName << "  Error: " << GetLastError();
                    return false;
                }

                if (!SetServiceObjectSecurity(hService, DACL_SECURITY_INFORMATION, securityDescriptorAmended))
                {
                    ORIGIN_LOG_ERROR << "Failed to set service security attributes, Service: " << serviceName << " Error: " << GetLastError();
                    return false;
                }

                ORIGIN_LOG_EVENT << "Set service security attributes successfully.  Service: " << serviceName;
            }
            else
            {
                ORIGIN_LOG_EVENT << "DACL already contained the right entry.";
            }

            return true;
        }

        bool UnRegisterService(QString serviceName)
        {
            AutoSC_HANDLE hScm = OpenSCManager(0, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);
            if (!hScm) 
            {
                ORIGIN_LOG_ERROR << "Error opening SCM. Error: " << GetLastError();
                return false;
            }

            AutoSC_HANDLE hService = OpenService(hScm, serviceName.utf16(), SERVICE_ALL_ACCESS);
            if (!hService)
            {
                if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
                {
                    ORIGIN_LOG_EVENT << serviceName << " not registered in SCM database.";

                    return false;
                }
                else
                {
                    ORIGIN_LOG_ERROR << "Error opening service: " << serviceName << "  Error: " << GetLastError();
                    return false;
                }
            }

            DWORD bytesNeeded = 0;
            QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, NULL, 0, &bytesNeeded);
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            {
                ORIGIN_LOG_ERROR << "Unable to query service status. Service: " << serviceName << " Error: " << GetLastError();
                return false;
            }

            AutoBuffer<SERVICE_STATUS_PROCESS> serviceStatus(bytesNeeded);
            if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)serviceStatus.data(), bytesNeeded, &bytesNeeded))
            {
                ORIGIN_LOG_ERROR << "Unable to query service status. Service: " << serviceName << " Error: " << GetLastError();
                return false;
            }

            // If the service is running
            if (serviceStatus->dwCurrentState != SERVICE_STOPPED)
            {
                if (!StopOriginService(serviceName))
                {
                    ORIGIN_LOG_ERROR << "Unable to stop service: " << serviceName;
                    return false;
                }
            }

            if (!DeleteService(hService))
            {
                ORIGIN_LOG_ERROR << "Unable to delete service: " << serviceName;
                return false;
            }

            ORIGIN_LOG_EVENT << "Service uninstalled: " << serviceName;
            return true;
        }

        bool StartOriginService(QString serviceName, QString args, int timeoutMS)
        {
            AutoSC_HANDLE hScm = OpenSCManager(0, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ENUMERATE_SERVICE);
            if (!hScm) 
            {
                ORIGIN_LOG_ERROR << "Error opening SCM.  Error: " << GetLastError();
                return false;
            }

            AutoSC_HANDLE hService = OpenService(hScm, serviceName.utf16(), SERVICE_QUERY_STATUS | SERVICE_START);
            if (!hService)
            {
                if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
                {
                    ORIGIN_LOG_EVENT << serviceName << " not registered in SCM database.";
                    return false;
                }
                else
                {
                    ORIGIN_LOG_ERROR << "Error opening service. Service: " << serviceName << "  Error: " << GetLastError();
                    return false;
                }
            }

            DWORD bytesNeeded = 0;
            QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, NULL, 0, &bytesNeeded);
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            {
                ORIGIN_LOG_ERROR << "Unable to query service status. Service: " << serviceName << " Error: " << GetLastError();
                return false;
            }

            AutoBuffer<SERVICE_STATUS_PROCESS> serviceStatus(bytesNeeded);
            if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)serviceStatus.data(), bytesNeeded, &bytesNeeded))
            {
                ORIGIN_LOG_ERROR << "Unable to query service status. Service: " << serviceName << " Error: " << GetLastError();
                return false;
            }

            // If the service is running
            if (serviceStatus->dwCurrentState != SERVICE_STOPPED)
            {
                ORIGIN_LOG_ERROR << "Service is not ready to start. Service: " << serviceName;
                return false;
            }

            ORIGIN_LOG_EVENT << "Service is currently stopped.";

            int numArgs = 0;
            LPWSTR *wsargs = NULL;

            if (!args.isEmpty())
            {
                // Digest the args
                QStringList argList = args.split(" ");
                if (argList.count() > 5)
                {
                    ORIGIN_LOG_EVENT << "Too many arguments, something is wrong.";
                    return false;
                }

                numArgs = argList.count();

                // Prep them into a WIN32-compatible structure
                if (argList.count() > 0)
                {
                    wsargs = new LPWSTR[argList.count()];
                    for (int c = 0; c < argList.count(); ++c)
                    {
                        int numChars = argList[c].length() + 1;
                        wsargs[c] = new wchar_t[argList[c].length() + 1];
                        memset(wsargs[c], 0, sizeof(wchar_t) * numChars);
                        argList[c].toWCharArray(wsargs[c]);
                    }
                }
            }

            bool success = true;
            if (!StartService(hService, numArgs, (LPCWSTR*)wsargs))
            {
                ORIGIN_LOG_ERROR << "Unable to start service: " << serviceName << " Error: " << GetLastError();
                success = false;
            }

            // Cleanup
            if (wsargs)
            {
                for (int c = 0; c < numArgs; ++c)
                {
                    delete [] wsargs[c];
                    wsargs[c] = NULL;
                }
                delete [] wsargs;
            }

            if (!success)
                return false;

            DWORD waitExpiration = GetTickCount() + timeoutMS; 

            do
            {
                if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)serviceStatus.data(), bytesNeeded, &bytesNeeded))
                {
                    ORIGIN_LOG_ERROR << "Unable to re-query service status. Service: " << serviceName << " Error: " << GetLastError();
                    return false;
                }

                if (serviceStatus->dwCurrentState != SERVICE_RUNNING)
                {
                    if (serviceStatus->dwCurrentState != SERVICE_START_PENDING)
                    {
                        ORIGIN_LOG_ERROR << "Service failed to start: " << serviceName;
                        break;
                    }

                    ORIGIN_LOG_EVENT << "Waiting for service...";

                    // From MSDN, it is recommended you wait 1/10th the wait hint, but not less than 1 second and not more than 10
                    // We are impatient, so we'll go with max of 2 seconds
                    DWORD waitHint = serviceStatus->dwWaitHint / 10;
                    if (waitHint < 1000)
                        waitHint = 1000;
                    if (waitHint > 2000)
                        waitHint = 2000;

                    Sleep(waitHint);
                }
            } while (serviceStatus->dwCurrentState != SERVICE_RUNNING && GetTickCount() < waitExpiration);

            // We didn't succeed :(
            if (serviceStatus->dwCurrentState != SERVICE_RUNNING && GetTickCount() > waitExpiration)
            {
                ORIGIN_LOG_ERROR << "Timed out while waiting for the service to start. Service: " << serviceName;
                return false;
            }

            ORIGIN_LOG_EVENT << "Service started successfully: " << serviceName;

            return true;
        }

        bool StopOriginService(QString serviceName, int timeoutMS)
        {
            AutoSC_HANDLE hScm = OpenSCManager(0, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ENUMERATE_SERVICE);
            if (!hScm)
            {
                ORIGIN_LOG_ERROR << "Error opening SCM.  Error: " << GetLastError();
                return false;
            }

            AutoSC_HANDLE hService = OpenService(hScm, serviceName.utf16(), SERVICE_QUERY_STATUS | SERVICE_STOP);
            if (!hService)
            {
                if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
                {
                    ORIGIN_LOG_EVENT << serviceName << " not registered in SCM database.";
                    return false;
                }
                else
                {
                    ORIGIN_LOG_ERROR << "Error opening service. Service: " << serviceName << "  Error: " << GetLastError();
                    return false;
                }
            }

            DWORD bytesNeeded = 0;
            QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, NULL, 0, &bytesNeeded);
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            {
                ORIGIN_LOG_ERROR << "Unable to query service status. Service: " << serviceName << " Error: " << GetLastError();
                return false;
            }

            AutoBuffer<SERVICE_STATUS_PROCESS> serviceStatus(bytesNeeded);
            if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)serviceStatus.data(), bytesNeeded, &bytesNeeded))
            {
                ORIGIN_LOG_ERROR << "Unable to query service status. Service: " << serviceName << " Error: " << GetLastError();
                return false;
            }

            // If the service is running
            if (serviceStatus->dwCurrentState != SERVICE_STOPPED)
            {
                bool stopped = false;
                DWORD waitHint = serviceStatus->dwWaitHint;

                // Make sure it isn't already pending being stopped
                if (serviceStatus->dwCurrentState != SERVICE_STOP_PENDING)
                {
                    SERVICE_STATUS newStatus;

                    if (!ControlService(hService, SERVICE_CONTROL_STOP, &newStatus))
                    {
                        ORIGIN_LOG_ERROR << "Unable to stop service. Service: " << serviceName << " Error: " << GetLastError();
                        return false;
                    }

                    ORIGIN_LOG_EVENT << "Service sent stop command.";

                    if (newStatus.dwCurrentState == SERVICE_STOPPED)
                        stopped = true;

                    // Update the wait hint
                    waitHint = newStatus.dwWaitHint;
                }

                if (!stopped)
                {
                    // From MSDN, it is recommended you wait 1/10th the wait hint, but not less than 1 second and not more than 10
                    // We are impatient, so we'll go with max of 2 seconds
                    waitHint /= 10;
                    if (waitHint < 1000)
                        waitHint = 1000;
                    if (waitHint > 2000)
                        waitHint = 2000;

                    DWORD waitExpiration = GetTickCount() + (timeoutMS); // Maximum wait

                    do
                    {
                        // Wait a while
                        Sleep(waitHint);

                        // Get new status
                        if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)serviceStatus.data(), bytesNeeded, &bytesNeeded))
                        {
                            ORIGIN_LOG_ERROR << "Unable to query service status. Service: " << serviceName << " Error: " << GetLastError();
                            return false;
                        }
                    } while (serviceStatus->dwCurrentState != SERVICE_STOPPED && GetTickCount() < waitExpiration);

                    if (serviceStatus->dwCurrentState != SERVICE_STOPPED)
                    {
                        ORIGIN_LOG_ERROR << "Timed out waiting to stop service. Service: " << serviceName << " Error: " << GetLastError();
                        return false;
                    }
                }

                ORIGIN_LOG_EVENT << "Service stopped: " << serviceName;
            }
            else
            {
                ORIGIN_LOG_EVENT << "Service not running: " << serviceName;
                return true;
            }
            return true;
        }

        bool QueryOriginServiceConfig(QString serviceName)
        {
            AutoSC_HANDLE hScm = OpenSCManager(0, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ENUMERATE_SERVICE);
            if (!hScm) 
            {
                ORIGIN_LOG_ERROR << "Error opening SCM.  Error: " << GetLastError();
                return false;
            }

            AutoSC_HANDLE hService = OpenService(hScm, serviceName.utf16(), SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG);
            if (!hService)
            {
                if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
                {
                    ORIGIN_LOG_EVENT << serviceName << " not registered in SCM database.";
                    return false;
                }
                else
                {
                    ORIGIN_LOG_ERROR << "Error opening service. Service: " << serviceName << "  Error: " << GetLastError();
                    return false;
                }
            }

            DWORD bytesNeeded = 0;
            QueryServiceConfig(hService, NULL, 0, &bytesNeeded);
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            {
                ORIGIN_LOG_ERROR << "Unable to query service config. Service: " << serviceName << " Error: " << GetLastError();
                return false;
            }

            AutoBuffer<QUERY_SERVICE_CONFIG> serviceConfig(bytesNeeded);
            if (!QueryServiceConfig(hService, serviceConfig.data(), bytesNeeded, &bytesNeeded))
            {
                ORIGIN_LOG_ERROR << "Unable to query service config. Service: " << serviceName << " Error: " << GetLastError();
                return false;
            }

            ORIGIN_LOG_EVENT << "Service Config read successfully for: " << serviceName;
            ORIGIN_LOG_EVENT << "Display Name: " << serviceConfig->lpDisplayName;
            ORIGIN_LOG_EVENT << "Logon User Name: " << serviceConfig->lpServiceStartName;
            ORIGIN_LOG_EVENT << "BinPath: " << serviceConfig->lpBinaryPathName;

            return true;
        }

        bool UpdateProcessDACL(HANDLE hProcess)
        {
            // We don't need to do this on Windows XP since there is no UAC
            if (IsWindowsXP())
                return true;

            DWORD result = 0;
            AutoLocalHeap<PSECURITY_DESCRIPTOR> sd;
            PACL dacl;
            if ((result = GetSecurityInfo(hProcess, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &dacl, NULL, &sd)) != ERROR_SUCCESS)
            {
                ORIGIN_LOG_ERROR << "Unable to query process DACL. Error: " << result;
                return false;
            }
            
            DWORD sidSize = SECURITY_MAX_SID_SIZE;
            AutoLocalHeap<PSID> everyoneSID((PSID)LocalAlloc(LMEM_FIXED, sidSize));
            if (!CreateWellKnownSid(WinWorldSid, NULL, everyoneSID, &sidSize))
            {
                ORIGIN_LOG_ERROR << "Unable to create SID. Error: " << GetLastError();
                return false;
            }

            // Grant 'Everyone' (WinWorldSid) Query Information and Synchronize (Wait for exit) privileges
            // This is necessary because if we are running as a service, the installer may run as SYSTEM
            // By default, no other user accounts have access to even know of the existence of these processes
            EXPLICIT_ACCESS ea;
            ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
            ea.grfAccessMode = SET_ACCESS;
            ea.grfAccessPermissions = PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE;
            ea.grfInheritance = NO_INHERITANCE;
            ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
            ea.Trustee.ptstrName = (LPWSTR)everyoneSID.data();
            
            AutoLocalHeap<PACL> newDacl;
            if ((result = SetEntriesInAcl(1, &ea, dacl, &newDacl)) != ERROR_SUCCESS)
            {
                ORIGIN_LOG_ERROR << "Unable to update DACL. Error: " << result;
                return false;
            }

            if ((result = SetSecurityInfo(hProcess, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, newDacl, NULL)) != ERROR_SUCCESS)
            {
                ORIGIN_LOG_ERROR << "Unable to update process DACL. Error: " << result;
                return false;
            }

            ORIGIN_LOG_EVENT << "Modified process DACL successfully.";

            return true;
        }

        bool SetTokenPrivilege(HANDLE hToken, LPCWSTR lpszPrivilege, bool bEnablePrivilege)
        {
            TOKEN_PRIVILEGES tp = { 0 };
            LUID luid = { 0 };

            // Receives LUID of privilege
            if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
            {
                ORIGIN_LOG_ERROR << "LookupPrivilegeValue error: " << GetLastError();
                return false; 
            }

            // Enable/disable the privilege
            tp.PrivilegeCount = 1;
            tp.Privileges[0].Luid = luid;
            tp.Privileges[0].Attributes = (bEnablePrivilege) ? SE_PRIVILEGE_ENABLED : 0;

            // Enable the privilege or disable all privileges.
            if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES) NULL, (PDWORD) NULL))
            { 
                ORIGIN_LOG_ERROR << "AdjustTokenPrivileges error: " << GetLastError();
                return false;  
            } 

            if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
            {
                ORIGIN_LOG_ERROR << "AdjustTokenPrivileges error, the token does not have the specified privilege: " << lpszPrivilege;
                return false;  
            } 

            return true;
        }

        void ProcessEnvironmentVariables(QString processEnvironment)
        {
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
                    ORIGIN_LOG_WARNING << "Couldn't split environment key/value pair from:" << sVarLine;
                }
            }
        }

        HANDLE GetAdminTokenForCreateProcessAsUser(int originPid, QString processEnvironment, LPVOID* processEnvironmentBlock)
        {
            // Get the TCB privilege while we do this (since we need to be able to grab the linked token)
            Auto_TCBPrivilegeManager tcbPrivilege;
            if (!tcbPrivilege.acquired())
            {
                ORIGIN_LOG_ERROR << "Unable to get the TCB privilege.  Error: " << GetLastError();
                return NULL;
            }

            // Open the Origin process (we want its token)
            Auto_HANDLE hOriginProcess = OpenProcess(MAXIMUM_ALLOWED, false, originPid);
            if (!hOriginProcess)
            {
                ORIGIN_LOG_ERROR << "Unable to open Origin process. Error: " << GetLastError();
                return NULL;
            }

            // Obtain a handle to the access token of the Origin process
            Auto_HANDLE hOriginToken;
            if (!OpenProcessToken(hOriginProcess, TOKEN_QUERY | TOKEN_DUPLICATE, &hOriginToken))
            {
                ORIGIN_LOG_ERROR << "Unable to open Origin process token. Error: " << GetLastError();
                return NULL;
            }

            DWORD bytesWritten = 0;
            TOKEN_ELEVATION_TYPE tokenElevType = TokenElevationTypeDefault;
            if (!GetTokenInformation(hOriginToken, TokenElevationType, &tokenElevType, sizeof(TOKEN_ELEVATION_TYPE), &bytesWritten))
            {
                ORIGIN_LOG_ERROR << "Unable to get token elevation type. Error: " << GetLastError();
                return NULL;
            }

            // By default, we want to duplicate our Origin token, unless UAC is on, in which case we need the linked token
            HANDLE hTokenToDuplicate = hOriginToken;

            // If we end up grabbing the linked token, we'll need to close it when we're done, so don't initialize this unless we need it
            Auto_HANDLE tokenToDuplicateAutoClose;

            // If UAC is off or we already have an elevated token, we don't need to do anything else
            if (tokenElevType == TokenElevationTypeDefault || tokenElevType == TokenElevationTypeFull)
            {
                ORIGIN_LOG_EVENT << "No restricted token detected or UAC disabled.";
            }
            else
            {
                ORIGIN_LOG_EVENT << "Split token detected, getting linked full token.";

                // Get the linked full token
                bytesWritten = 0;
                TOKEN_LINKED_TOKEN linkedToken = { 0 };
                if (!GetTokenInformation(hOriginToken, TokenLinkedToken, &linkedToken, sizeof(TOKEN_LINKED_TOKEN), &bytesWritten))
                {
                    ORIGIN_LOG_ERROR << "Unable to get linked token. Error: " << GetLastError();
                    return NULL;
                }

                // We want to duplicate this token instead, since it is the full token
                hTokenToDuplicate = linkedToken.LinkedToken;

                // We are now responsible for closing this handle
                tokenToDuplicateAutoClose = hTokenToDuplicate;
            }

            // Copy the access token of the Origin process; 
            // The newly created token will be an impersonation token (necessary for use with CheckTokenMembership)
            Auto_HANDLE hImpersonationToken;
            if (!DuplicateTokenEx(hTokenToDuplicate, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenImpersonation, &hImpersonationToken))
            {
                ORIGIN_LOG_ERROR << "Unable to duplicate Origin process token. Error: " << GetLastError();
                return NULL;
            }

            // Create a SID for the builtin Administrators group
            DWORD sidSize = SECURITY_MAX_SID_SIZE;
            AutoLocalHeap<PSID> builtinAdministratorsSID((PSID)LocalAlloc(LMEM_FIXED, sidSize));
            if (!CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, builtinAdministratorsSID, &sidSize))
            {
                ORIGIN_LOG_ERROR << "Unable to create SID. Error: " << GetLastError();
                return NULL;
            }

            // Determine if the user is an administrator
            BOOL isMember = FALSE;
            if (!CheckTokenMembership(hImpersonationToken, builtinAdministratorsSID, &isMember))
            {
                ORIGIN_LOG_ERROR << "Unable to check token membership. Error: " << GetLastError();
                return NULL;
            }

            // Admins we can launch as their token (or their linked admin token, if UAC is on)
            if (isMember)
            {
                ORIGIN_LOG_EVENT << "Origin user belongs to local administrators group.";

                // Copy the access token of the Origin process; 
                // The newly created token will be a primary token (necessary for CreateProcessAsUser)
                Auto_HANDLE hDuplicatedToken;
                if (!DuplicateTokenEx(hTokenToDuplicate, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary, &hDuplicatedToken))
                {
                    ORIGIN_LOG_ERROR << "Unable to duplicate Origin process token. Error: " << GetLastError();
                    return NULL;
                }

                // Process the passed-in environment variables (sets them to this process' environment)
                ProcessEnvironmentVariables(processEnvironment);

                *processEnvironmentBlock = NULL;
                if (!CreateEnvironmentBlock(processEnvironmentBlock, hDuplicatedToken, TRUE))
                {
                    ORIGIN_LOG_ERROR << "Unable to create process environment block. Error: " << GetLastError();
                    return NULL;
                }

				// This disabled code follows the MSDN advice of calling LoadUserProfile before calling CreateProcessAsUser
				// This has been left disabled because it doesn't appear to be 100% necessary, as well as the implementation
				// itself being somewhat hacky.  In particular, to call LoadUserProfile, you need the logon username, and
				// the 'proper' API is LookupAccountSid, however this API takes >40 seconds to complete on my machine.  Instead
				// the code below calls ImpersonateLoggedOnUser, followed by GetUserName, followed by RevertToSelf, which
				// accomplishes the desired result with no lag.
				// I determined that this code may not be necessary by launching 'regedit.exe' using the Client Service.  When
				// browsing HKEY_CURRENT_USER, I see that the keys are loaded properly.  So for now, this code can live here
				// disabled in case it turns out to be necessary after all.  (It was very tricky/fussy to implement properly)
				//  - R. Binns (2014/10/21)
#ifdef ORIGINCLIENTSERVICE_LOADUSERPROFILE
                // Get the username for the token we're launching with
                WCHAR tokenUsername[1024] = { 0 };
                DWORD ccbTokenUsername = sizeof(tokenUsername);
                bool gotUsername = false;
                if (ImpersonateLoggedOnUser(hImpersonationToken))
                {
                    if (GetUserName(tokenUsername, &ccbTokenUsername))
                        gotUsername = true;

                    RevertToSelf();
                }
                else
                {
                    ORIGIN_LOG_ERROR << "Unable to impersonate logged on user. Error: " << GetLastError();
                    return NULL;
                }
                
                if (!gotUsername)
                {
                    ORIGIN_LOG_ERROR << "Unable to get username for logged on user. Error: " << GetLastError();
                    return NULL;
                }

                ORIGIN_LOG_EVENT << "Loading user profile for user: " << tokenUsername;

                // Load the user profile
                PROFILEINFO profileInfo;
                ZeroMemory(&profileInfo, sizeof(PROFILEINFO));
                profileInfo.dwSize = sizeof(PROFILEINFO);
                profileInfo.lpUserName = tokenUsername;

                if (!LoadUserProfile(hDuplicatedToken, &profileInfo))
                {
                    ORIGIN_LOG_ERROR << "Unable to load user profile for new process. Error: " << GetLastError();
                    return NULL;
                }
#endif

                // Releases ownership of this token and returns to the caller
                return hDuplicatedToken.release();
            }
            else
            {
                // Get the token for WinLogon.exe which belongs to the Origin user's session (best we can do for a non-admin)
                ORIGIN_LOG_EVENT << "Origin user is not an admin.";

                DWORD activeSessionId = 0;
                if (!ProcessIdToSessionId(originPid, &activeSessionId))
                {
                    ORIGIN_LOG_ERROR << "Unable to get Origin process session ID. Error: " << GetLastError();
                    return NULL;
                }

                // Take a snapshot of all processes in the system.
                Auto_HANDLE hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
                if (!hProcessSnap)
                {
                    ORIGIN_LOG_ERROR << "Unable to get process snapshot. Error: " << GetLastError();
                    return NULL;
                }

                PROCESSENTRY32 pe32;
                pe32.dwSize = sizeof( PROCESSENTRY32 );
            
                // Start the iterator
                if (!Process32First(hProcessSnap, &pe32))
                {
                    ORIGIN_LOG_ERROR << "Unable to get first process. Error: " << GetLastError();
                    return NULL;
                }

                DWORD winLogonPid = 0;

                // Walk through the snapshot of processes and look for winlogon
                do
                {
                    if (_wcsicmp(pe32.szExeFile, L"winlogon.exe") == 0)
                    {
                        DWORD processSessionId = 0;
                        ProcessIdToSessionId(pe32.th32ProcessID, &processSessionId);
                        if (processSessionId == activeSessionId)
                        {
                            winLogonPid = pe32.th32ProcessID;
                            break;
                        }
                    }    
                } while (Process32Next(hProcessSnap, &pe32));

                if (winLogonPid == 0)
                {
                    ORIGIN_LOG_ERROR << "Unable to get active winlogon process.";
                    return NULL;
                }

                // Open the winlogon process (we want its token)
                Auto_HANDLE hWinLogonProcess = OpenProcess(MAXIMUM_ALLOWED, false, winLogonPid);
                if (!hWinLogonProcess)
                {
                    ORIGIN_LOG_ERROR << "Unable to open winlogon process. Error: " << GetLastError();
                    return NULL;
                }

                // Obtain a handle to the access token of the winlogon process
                Auto_HANDLE hWinLogonToken;
                if (!OpenProcessToken(hWinLogonProcess, TOKEN_DUPLICATE, &hWinLogonToken))
                {
                    ORIGIN_LOG_ERROR << "Unable to open winlogon process token. Error: " << GetLastError();
                    return NULL;
                }

                // Copy the access token of the winlogon process; 
                // The newly created token will be a primary token
                Auto_HANDLE hDuplicatedToken;
                if (!DuplicateTokenEx(hWinLogonToken, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary, &hDuplicatedToken))
                {
                    ORIGIN_LOG_ERROR << "Unable to duplicate winlogon process token. Error: " << GetLastError();
                    return NULL;
                }

                // Process the passed-in environment variables (sets them to this process' environment)
                ProcessEnvironmentVariables(processEnvironment);

                // Create from the original Origin token
                *processEnvironmentBlock = NULL;
                if (!CreateEnvironmentBlock(processEnvironmentBlock, hOriginToken, TRUE))
                {
                    ORIGIN_LOG_ERROR << "Unable to create process environment block. Error: " << GetLastError();
                    return NULL;
                }

                // Releases ownership of this token and returns to the caller
                return hDuplicatedToken.release();
            }
        }

        bool LaunchProcessWithToken(HANDLE hCreateProcessToken, QString file, QString args, QString currentDirectory, LPVOID processEnvironmentBlock, quint32& pid)
        {
            STARTUPINFO si;
            ZeroMemory(&si, sizeof(STARTUPINFO));
            si.cb = sizeof(STARTUPINFO);

            PROCESS_INFORMATION procInfo;
            ZeroMemory(&procInfo, sizeof(PROCESS_INFORMATION));

            // Interactive window station parameter; basically this indicates that the process created can display a GUI on the desktop
            si.lpDesktop = L"winsta0\\default";

            // Flags that specify the priority and creation method of the process
            int dwCreationFlags = NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT;

            // Pack the arguments into command line form (e.g. ""Executable.exe" /arg1 /arg2")
            wchar_t argsBuf[ARGS_BUFFER_SIZE];
            ZeroMemory(argsBuf, sizeof(argsBuf));
            std::wstring fullArgs = std::wstring(L"\"") + file.toStdWString() + std::wstring(L"\"");
            if (args.length() > 0)
                fullArgs += std::wstring(L" ") + args.toStdWString();
            wcsncpy_s(argsBuf, ARGS_BUFFER_SIZE-1, fullArgs.c_str(), fullArgs.length());

            // Create a new process in the current User's logon session
            if (!CreateProcessAsUser(hCreateProcessToken,                // Client's access token
                                     NULL,                               // File to execute
                                     argsBuf,                            // Command line
                                     NULL,                               // Pointer to process SECURITY_ATTRIBUTES
                                     NULL,                               // Pointer to thread SECURITY_ATTRIBUTES
                                     false,                              // Handles are not inheritable
                                     dwCreationFlags,                    // Creation flags
                                     processEnvironmentBlock,            // Pointer to new environment block 
                                     (LPCWSTR)currentDirectory.utf16(),  // Name of current directory 
                                     &si,                                // Pointer to STARTUPINFO structure
                                     &procInfo                           // Receives information about new process
                                     ))
            {
                ORIGIN_LOG_ERROR << "Unable to launch new process. Error: " << GetLastError();
                return false;
            }

            // We're responsible for closing these
            Auto_HANDLE hProcess(procInfo.hProcess);
            Auto_HANDLE hThread(procInfo.hThread);

            ORIGIN_LOG_EVENT << "Successfully spawned process. PID: " << procInfo.dwProcessId;

            // We need to loosen up the security DACL for the process so anybody can monitor it
            UpdateProcessDACL(procInfo.hProcess);

            pid = procInfo.dwProcessId;

            return true;
        }

        bool LaunchProcessElevatedFromService(int originPid, QString file, QString args, QString currentDirectory, QString processEnvironment, quint32& pid)
        {
            // Clean the arguments

            // First trim the file argument since we will re-quote it later
            file.remove("\"");

            // If the caller specified no directory, we substitute the path of the executable
            // Some executables (notably EAInstaller Cleanup utilities) fail with Error 123 otherwise (Why?!?)
            if (currentDirectory.isEmpty())
            {
                QFileInfo exeFileInfo(file);
                currentDirectory = QDir::toNativeSeparators(exeFileInfo.absolutePath());

                ORIGIN_LOG_EVENT << "No current directory specified, using: " << currentDirectory;
            }

            ORIGIN_LOG_EVENT << "Launching: " << file << " with Args: " << args << " Dir: " << currentDirectory << " Environment: " << processEnvironment;

            LPVOID processEnvironmentBlock = NULL;
            Auto_HANDLE hCreateProcessToken = GetAdminTokenForCreateProcessAsUser(originPid, processEnvironment, &processEnvironmentBlock);
            if (!hCreateProcessToken)
            {
                ORIGIN_LOG_ERROR << "Unable to get token for CreateProcessAsUser.";
                return false;
            }

            bool result = LaunchProcessWithToken(hCreateProcessToken, file, args, currentDirectory, processEnvironmentBlock, pid);

            // We're done with this
            if (processEnvironmentBlock)
                DestroyEnvironmentBlock(processEnvironmentBlock);

            return result;
        }
    } // Escalation
}// Origin