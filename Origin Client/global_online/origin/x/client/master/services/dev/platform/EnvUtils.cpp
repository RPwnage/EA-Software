///////////////////////////////////////////////////////////////////////////////
// EnvUtils.cpp
//
// Copyright (c) 2010-2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "services/debug/DebugService.h"
#include "services/downloader/Timer.h"
#include "services/log/LogService.h"
#include "services/platform/EnvUtils.h"
#include "services/platform/PlatformService.h"
#include "services/settings/SettingsManager.h"

#include "TelemetryAPIDLL.h"
#include "SystemInformation.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <shlwapi.h>
#include <ShlObj.h>
#endif

#ifdef __APPLE__
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <QMutex>
#include <QMutexLocker>
#include <QFile>
#include <QSysInfo>
#include <QString>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <TlHelp32.h>
#endif

#ifdef ORIGIN_PC
#pragma comment(lib, "version.lib")
#include "Win32CppHelpers.h"
#include <RestartManager.h>
#endif

QString EnvUtils::ConvertToShortPath(const QString& sPath)
{
#ifdef Q_OS_WIN
    wchar_t* sPath_Short = new wchar_t[MAX_PATH];
    memset(sPath_Short, 0, sizeof(wchar_t) * MAX_PATH);
    ::GetShortPathName(sPath.utf16(), sPath_Short, MAX_PATH);
    QString shortPath = QString::fromWCharArray(sPath_Short);
    delete[] sPath_Short;

    return shortPath;
#else
    return sPath;
#endif
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// EnvUtils::IsDriveAvailable
//
// Returns true if the drive for the path passed in is present.  (For checking removable drives.)
//
// On Windows that's a question of checking the drive part of the path, example "C:\".  The passed in path must contain that.
// On Mac anything but the main drive will start with "/Volumes" followed by the volume name.  Example: "/Volumes/MyHDD1"
//
bool EnvUtils::IsDriveAvailable( const QString& sPath )
{
#ifdef __APPLE__
    QString volumesTag("/Volumes/");
    if (sPath.left(volumesTag.length()).compare( volumesTag ) == 0)
    {
        QString sVolumeName( sPath.mid( volumesTag.length() ) );     // start by stripping off the "/Volumes" from the front
        int nIndex = sVolumeName.indexOf( "/" );
        sVolumeName = sVolumeName.left( nIndex );

        QDir rootLocation (volumesTag + sVolumeName);
        return rootLocation.exists();
    }

    return true;        // If path doesn't start with "/Volumes" then it's on the main drive and is available
#endif

#ifdef Q_OS_WIN
    QString rootLocationPath( sPath.left(3) );    // drive portion, "E:\"
    QDir rootLocation( rootLocationPath );
                    
    return rootLocation.exists();
#endif

    return true;        // default
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// EnvUtils::DeleteFolderIfPresent
//
// delete folder with contents
//
// Note: Used recursively for sub-folders
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
bool EnvUtils::DeleteFolderIfPresent( const QString& sFolderName, bool bSafety /* = false */, bool bIncludingRoot /* = false */, void *pProgressBar /* = NULL */, void (*pfcnCallback)(void*) /* = NULL */ )
{
    bool bRetVal = true;
    QDir dir(sFolderName);

#if defined(__APPLE__)

    // Make sure the folder is visible to Qt
    struct stat fileStat;
    stat(qPrintable(sFolderName), &fileStat);

    chflags(qPrintable(sFolderName), (fileStat.st_flags & ~UF_HIDDEN));

#endif

    if (dir.exists(sFolderName)) 
    {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) 
        {
            // in windows, modify file attributes to make files deleteable (in case they were read-only, etc.)
#ifdef Q_OS_WIN
            ::SetFileAttributesW( info.absoluteFilePath().utf16(), FILE_ATTRIBUTE_NORMAL );
#endif
            // if it's a subdirectory
            if (info.isDir()) 
            {
                bRetVal = DeleteFolderIfPresent(info.absoluteFilePath(), false, true, pProgressBar, pfcnCallback ); // Recurse.
                if (!bRetVal)
                {
                    QString sError;
                    sError = QString("Failed to delete the directory: \"%1\"!").arg(info.absoluteFilePath());
                    ORIGIN_LOG_ERROR << sError;
                    GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("EnvUtils_DeleteFolderIfPresent_Dir", info.absoluteFilePath().toUtf8().data(), 0);
                }
            }
            // else, it's a file
            else 
            {
                QFile fileToDelete(info.absoluteFilePath());
                bRetVal = fileToDelete.remove();
                                                    
                if (!bRetVal)
                {
#ifdef Q_OS_WIN
                    DWORD nError = ::GetLastError();
#else
                    QFile::FileError nError = fileToDelete.error();
#endif
                    QString sError;
                    sError = QString("Failed to delete the file: \"%1\"! error code [%2]\r\n").arg(info.absoluteFilePath()).arg(nError);
                    ORIGIN_LOG_ERROR << sError;
                    GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("EnvUtils_DeleteFolderIfPresent_File", info.absoluteFilePath().toUtf8().data(), fileToDelete.error());
                }
                
                // If a progress bar and a progress bar callback was passed in, call it
                else if (pProgressBar && pfcnCallback)
                {
                    pfcnCallback(pProgressBar);
                }
            }
            if (!bRetVal) 
            {
                return bRetVal;
            }
        }
        // delete the root dir?
        if (bIncludingRoot)
        {
            bRetVal = dir.rmdir(sFolderName);
            if (!bRetVal)
            {
                QString sError;
                sError = QString("Failed to delete the directory: \"%1\"!").arg(sFolderName);
                ORIGIN_LOG_ERROR << sError;
                GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("EnvUtils_DeleteFolderIfPresent_RootDir", sFolderName.toUtf8().data(), 0);
            }
        }
    }
    else
    {
        bRetVal = false;
        QString sError;
        sError = QString("Failed to delete the directory: \"%1\"!  The directory does not exist.").arg(sFolderName);
        ORIGIN_LOG_ERROR << sError;
    }
    return bRetVal;
}


QString EnvUtils::GetOSVersion()
{
    QString osVersion = Origin::Services::readSetting(Origin::Services::SETTING_OverrideOSVersion);
    if (!osVersion.isEmpty())
    {
        // verify that the override looks reasonable
#if defined(ORIGIN_MAC)
        // match "n.n.n" with an optional ".n" at the end
        //QRegExp versionExp("[0-9]+[.[0-9]+]{2}[.[0-9]+]{,1}");
        //QRegExp versionExp("[0-9]+(\\.[0-9]+){2}(\\.[0-9]+){,1}");
        QRegExp versionExp("\\d+(\\.\\d+){2}(\\.\\d+){,1}");
#elif defined(ORIGIN_PC)
        // match "n.n" with an optional ".n.n" at the end
        QRegExp versionExp("\\d+\\.\\d+(\\.\\d+\\.\\d+){,1}");
#endif
        ORIGIN_ASSERT(versionExp.isValid());
        
        if (versionExp.exactMatch(osVersion))
            return osVersion;
    }
    
#ifdef Q_OS_WIN
    // format major.minor
    OSVERSIONINFOEX    osvi;
    QString sVersion("0.0");

    ZeroMemory( &osvi, sizeof( OSVERSIONINFOEX ) );
    osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );

    if ( ::GetVersionEx( ( OSVERSIONINFO * ) &osvi ) )
    {
        sVersion = QString("%1.%2").arg(osvi.dwMajorVersion).arg(osvi.dwMinorVersion);
    }
    return sVersion;
    
#elif defined(ORIGIN_MAC)
    
    return Origin::Services::PlatformService::getOSXVersion();

#else

#error Needs implementation

#endif
}

bool EnvUtils::GetISWindowsXPOrOlder()
{
#ifdef Q_OS_WIN

    OSVERSIONINFOEX    osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if (::GetVersionEx((OSVERSIONINFO *)&osvi))
    {
        return osvi.dwMajorVersion < 6;
    }


#endif
    return false;
}

bool EnvUtils::GetIS64BitOS()
{
#ifdef Q_OS_WIN

    // Detecting if 
    // From http://msdn.microsoft.com/en-us/library/windows/desktop/ms684139%28v=vs.85%29.aspx
    BOOL bIsWow64 = FALSE;
    typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(GetModuleHandle(TEXT("kernel32")),"IsWow64Process");
    if (fnIsWow64Process)
    {
        if (fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
        {
            return bIsWow64;
        }
    }

    return false;
#else
    // Needs implementation?
    return true;
#endif
}

bool EnvUtils::GetIsProcessElevated()
{
#if defined(ORIGIN_PC)
    // First determine if the user is running on Windows XP
    
    // Major version numbers of the Windows Operating Systems.
    const int NMAJOR_2003R2_2003_XP_2000 = 5;
    const int NMINOR_2003R2_2003_XP64        = 2;
    const int NMINOR_XP                        = 1;

    OSVERSIONINFOEX    osvi;
    SYSTEM_INFO        si;

    bool isXP = false;

    ZeroMemory( &osvi, sizeof( OSVERSIONINFOEX ) );
    ZeroMemory( &si, sizeof( SYSTEM_INFO ) );
    osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );

    // Get the OS version
    if ( ::GetVersionEx( ( OSVERSIONINFO * ) &osvi ) )
    {
        ::GetSystemInfo( &si );

        // Is this OS and NT or 9x platform?
        if ( osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)        
        {
            if ( osvi.dwMajorVersion == NMAJOR_2003R2_2003_XP_2000 && osvi.dwMinorVersion == NMINOR_2003R2_2003_XP64 )
            {
                if( osvi.wProductType == VER_NT_WORKSTATION && si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ) 
                { 
                    isXP = true; // XP64;
                }
            } 
            else 
            if ( osvi.dwMajorVersion == NMAJOR_2003R2_2003_XP_2000 && osvi.dwMinorVersion == NMINOR_XP ) 
            { 
                isXP = true; // XP
            }
        }
    }

    if (isXP)
    {
        // For Windows XP, if you are a member of the administrators group, you are already 'elevated'
        return (IsUserAnAdmin() == 1);
    }
    else
    {
        // For Windows Vista/7/8/etc, determine if your token is elevated
        bool isElevated = false;

        // Open the access token associated with the calling process.
        HANDLE hToken;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        {
            const DWORD lastError = ::GetLastError();
            
            ORIGIN_LOG_ERROR << "Error with OpenProcessToken, unable to determine elevation status.  Code: " << lastError;

            // Nothing to close because we weren't able to open the handle
            return false;
        }

        // Get the Token Elevation Type
        DWORD bytesUsed = 0;
        TOKEN_ELEVATION_TYPE tokenElevationType;
        if (!::GetTokenInformation(hToken, TokenElevationType, &tokenElevationType, sizeof(tokenElevationType), &bytesUsed))
        {
            const DWORD lastError = ::GetLastError();

            ORIGIN_LOG_ERROR << "Error with GetTokenInformation, unable to determine elevation status.  Code: " << lastError;
        }
        else
        {
            if (tokenElevationType == TokenElevationTypeFull)
            {
                ORIGIN_LOG_EVENT << "Detected User is running elevated.";
                isElevated = true;
            }
        }

        // Clean up
        CloseHandle(hToken);

        return isElevated;
    }
#elif defined (ORIGIN_MAC)
    // Determine if we are effectively running as root
    uid_t userId = geteuid();
    //Check if the user is effectively running as root meaning euid == 0
    if (userId == 0)
        return true;
    else
        return false;
#else
#  warning "TODO: implement GetIsProcessElevated()" 
    return false;
#endif
}

void EnvUtils::ScanFolders(QString path, QFileInfoList& files)
{
    QDir dir(path);
    dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);

    files += dir.entryInfoList();

    dir.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    QStringList dirList = dir.entryList();
    for (int i = 0; i < dirList.size(); i++)
    {
        QString newPath = QString("%1/%2").arg(dir.absolutePath()).arg(dirList.at(i));
        ScanFolders(newPath, files);
    }
}



const QStringList browserList = (QStringList() <<
    "iexplore.exe" <<
    "chrome.exe" <<
    "firefox.exe" <<
    "safari.exe" <<
    "opera.exe" <<
    "outlook.exe" <<
    "thunderbird.exe" <<
    "maxthon.exe" <<
    "rockmelt.exe" <<
    "seamonkey.exe");

bool EnvUtils::GetParentProcessIsBrowser()
{
#ifdef Q_OS_WIN
    int pid;
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe = { 0 };
    pe.dwSize = sizeof(PROCESSENTRY32);
    bool bParentIsBrowser = false;

    pid = GetCurrentProcessId();

    if( Process32First(h, &pe)) 
    {
        DWORD nParentPID = 0;
        do 
        {
            if (pe.th32ProcessID == (unsigned long)pid) 
            {
                nParentPID = pe.th32ParentProcessID;
            }
        } while( Process32Next(h, &pe));

        // We have a parent ID, now check to see if it's in our list of known browsers
        if (nParentPID != 0)
        {
            if (Process32First(h, &pe))
            {
                do 
                {
                    if (pe.th32ProcessID == nParentPID) 
                    {
                        QString parentEXE = QString::fromWCharArray(pe.szExeFile);

                        if (browserList.contains(parentEXE, Qt::CaseInsensitive))
                        {
                            bParentIsBrowser = true;                            
                            break;
                        }
                    }
                } while( Process32Next(h, &pe));
            }
        }
    }

    CloseHandle(h);

    return bParentIsBrowser;
#else
    // TBD on Mac if needed
    return false;
#endif
}

#ifdef ORIGIN_PC
QString EnvUtils::GetOriginBootstrapVersionString()
{
    // Get current executable filename
    wchar_t szOriginPath[MAX_PATH];
    ::GetModuleFileName(::GetModuleHandle(NULL), szOriginPath, MAX_PATH);
    QString originLocation = QString::fromWCharArray(szOriginPath);
    
    DWORD unused = 0;
    DWORD size = GetFileVersionInfoSize(originLocation.utf16(), &unused);

    // If we can't get version info then assume we didn't find Origin.exe
    if (size == 0)
    {
        return QString("0.0.0.0");
    }

    AutoBuffer<BYTE> versionInfo(size);
    if (GetFileVersionInfo(originLocation.utf16(), unused, size, versionInfo))
    {
        VS_FIXEDFILEINFO* vsfi = NULL;
        UINT len = 0;
        VerQueryValue(versionInfo, L"\\", (void**)&vsfi, &len);
        WORD first = HIWORD(vsfi->dwFileVersionMS);
        WORD second = LOWORD(vsfi->dwFileVersionMS);
        WORD third = HIWORD(vsfi->dwFileVersionLS);
        WORD fourth = LOWORD(vsfi->dwFileVersionLS);

        wchar_t wVersionInfo[32] = {0};
        wsprintfW(wVersionInfo, L"%hu.%hu.%hu.%hu", first, second, third, fourth);
        
        return QString::fromWCharArray(wVersionInfo);
    }

    return QString("0.0.0.0");
}

///////////////////////////////////////////////////////////////////////////
// Win32 Utility Code (Windows Vista+) supporting GetIsFileInUse
// Use the Restart Manager API
///////////////////////////////////////////////////////////////////////////

typedef DWORD (WINAPI *_pfnRmRegisterResources)(
    DWORD dwSessionHandle,
    UINT nFiles,
    LPCWSTR rgsFilenames[ ],
    UINT nApplications,
    RM_UNIQUE_PROCESS rgApplications[ ],
    UINT nServices,
    LPCWSTR rgsServiceNames[ ]
);

typedef DWORD (WINAPI *_pfnRmStartSession)(
    DWORD *pSessionHandle,
    DWORD dwSessionFlags,
    WCHAR strSessionKey[ ]
);

typedef DWORD (WINAPI *_pfnRmEndSession)(
    DWORD dwSessionHandle
);

typedef DWORD (WINAPI *_pfnRmGetList)(
    DWORD dwSessionHandle,
    UINT *pnProcInfoNeeded,
    UINT *pnProcInfo,
    RM_PROCESS_INFO rgAffectedApps[ ],
    LPDWORD lpdwRebootReasons
);

typedef DWORD (WINAPI *_pfnRmShutdown)(
    DWORD dwSessionHandle,
    ULONG lActionFlags,
    RM_WRITE_STATUS_CALLBACK fnStatus
);

typedef BOOL (WINAPI *_pfnQueryFullProcessImageNameW)(
    HANDLE hProcess,
    DWORD dwFlags,
    LPWSTR lpExeName,
    PDWORD lpdwSize
);

class RestartManagerHelper
{
public:
    RestartManagerHelper() :
      _init(false),
      PfQueryFullProcessImageName(NULL),
      PfRmRegisterResources(NULL),
      PfRmStartSession(NULL),
      PfRmEndSession(NULL),
      PfRmGetList(NULL),
      PfRmShutdown(NULL)
    {

    }

    ~RestartManagerHelper()
    {
        // Nothing to do for now
    }

    bool valid()
    {
        return init();
    }

    QString rebootReason(DWORD reason)
    {
        return _rmRebootReasonMap[reason];
    }

    QString appType(DWORD type)
    {
        return _rmAppTypeMap[type];
    }

    // Public function pointers for utility purposes
    _pfnQueryFullProcessImageNameW PfQueryFullProcessImageName;
    _pfnRmRegisterResources PfRmRegisterResources;
    _pfnRmStartSession PfRmStartSession;
    _pfnRmEndSession PfRmEndSession;
    _pfnRmGetList PfRmGetList;
    _pfnRmShutdown PfRmShutdown;
private:
    bool init()
    {
        if (_init)
            return true;

        ORIGIN_LOG_EVENT << "Initialize RestartManager API";

        if (!_hRestartMgrLibrary)
            _hRestartMgrLibrary = LoadLibrary(L"rstrtmgr.dll");

        if (!_hRestartMgrLibrary)
            return false;

        PfQueryFullProcessImageName = (_pfnQueryFullProcessImageNameW)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "QueryFullProcessImageNameW");
        PfRmRegisterResources = (_pfnRmRegisterResources)GetProcAddress(_hRestartMgrLibrary, "RmRegisterResources");
        PfRmStartSession = (_pfnRmStartSession)GetProcAddress(_hRestartMgrLibrary, "RmStartSession");
        PfRmEndSession = (_pfnRmEndSession)GetProcAddress(_hRestartMgrLibrary, "RmEndSession");
        PfRmGetList = (_pfnRmGetList)GetProcAddress(_hRestartMgrLibrary, "RmGetList");
        PfRmShutdown = (_pfnRmShutdown)GetProcAddress(_hRestartMgrLibrary, "RmShutdown");

        if (!PfQueryFullProcessImageName || !PfRmRegisterResources || !PfRmStartSession || !PfRmEndSession || !PfRmGetList || !PfRmShutdown)
        {
            _hRestartMgrLibrary.close();

            ORIGIN_LOG_ERROR << "Unable to load RestartManager API.";

            return false;
        }

        _rmRebootReasonMap[RmRebootReasonNone]             = "Reboot Not Required";
        _rmRebootReasonMap[RmRebootReasonPermissionDenied] = "Reboot Required: Permission Denied";
        _rmRebootReasonMap[RmRebootReasonSessionMismatch]  = "Reboot Required: Session Mismatch";
        _rmRebootReasonMap[RmRebootReasonCriticalProcess]  = "Reboot Required: File in use by Critical Process";
        _rmRebootReasonMap[RmRebootReasonCriticalService]  = "Reboot Required: File in use by Critical Service";
        _rmRebootReasonMap[RmRebootReasonDetectedSelf]     = "Reboot Required: Detected Self";

        _rmAppTypeMap[RmUnknownApp]     = "Unknown";
        _rmAppTypeMap[RmMainWindow]     = "Window (Main)";
        _rmAppTypeMap[RmOtherWindow]    = "Window (Other)";
        _rmAppTypeMap[RmService]        = "Service";
        _rmAppTypeMap[RmExplorer]       = "Windows Explorer";
        _rmAppTypeMap[RmConsole]        = "Console";
        _rmAppTypeMap[RmCritical]       = "Critical";

        _init = true;

        return true;
    }

private:
    bool _init;
    Auto_HMODULE _hRestartMgrLibrary;
    QMap<DWORD, QString> _rmRebootReasonMap;
    QMap<DWORD, QString> _rmAppTypeMap;
};

static RestartManagerHelper s_rmgr;

class RestartManagerSession
{
public:
    RestartManagerSession() :
      _session(0),
      _valid(false),
      _canShutdown(false),
      _shutdownReason(0),
      _error(0)
    {
        if (!s_rmgr.valid())
            return;

        WCHAR szSessionKey[CCH_RM_SESSION_KEY+1] = {0};
        _error = s_rmgr.PfRmStartSession(&_session, 0, szSessionKey);
        if (_error != ERROR_SUCCESS)
            return;

        _valid = true;
    }
    ~RestartManagerSession()
    {
        if (_valid)
        {
            s_rmgr.PfRmEndSession(_session);
        }
    }

    bool valid()
    {
        return _valid;
    }

    DWORD error()
    {
        return _error;
    }

    DWORD shutdownReason()
    {
        return _shutdownReason;
    }

    QString shutdownReasonString()
    {
        return s_rmgr.rebootReason(_shutdownReason);
    }

    bool registerFileResource(QString filePath)
    {
        if (!_valid)
            return false;

        WCHAR szFilePath[MAX_PATH+1] = {0};
        wcsncpy_s<MAX_PATH+1>(szFilePath, filePath.utf16(), filePath.length());
        LPCWSTR pszFilePath = szFilePath;
        _error = s_rmgr.PfRmRegisterResources(_session, 1, &pszFilePath, 0, NULL, 0, NULL);
        if (_error != ERROR_SUCCESS)
            return false;
        return true;
    }

    bool getList(QList<EnvUtils::FileLockProcessInfo>& processInfoList)
    {
        if (!_valid)
            return false;

        UINT nProcInfoNeeded = 0;
        UINT nProcInfo = 10;
        RM_PROCESS_INFO rmProcessInfoList[10];
        _error = s_rmgr.PfRmGetList(_session, &nProcInfoNeeded, &nProcInfo, rmProcessInfoList, &_shutdownReason);
        if (_error != ERROR_SUCCESS)
            return false;

        // If the OS says we can shut down the processes
        if (_shutdownReason == RmRebootReasonNone)
        {
            _canShutdown = true;
        }

#ifdef _DEBUG
        printf("RmGetList returned %d infos (%d needed) Reason: %d (%s)\n", nProcInfo, nProcInfoNeeded, _shutdownReason, qPrintable(s_rmgr.rebootReason(_shutdownReason)));
#endif

        for (UINT i = 0; i < nProcInfo; i++) 
        {
            EnvUtils::FileLockProcessInfo rmProcessInfo;

            rmProcessInfo.processType = s_rmgr.appType(rmProcessInfoList[i].ApplicationType);
            rmProcessInfo.processPid = rmProcessInfoList[i].Process.dwProcessId;
            rmProcessInfo.processDisplayName = QString::fromWCharArray(rmProcessInfoList[i].strAppName);

#ifdef _DEBUG
            printf("%d.ApplicationType = %d (%s)\n", i, rmProcessInfoList[i].ApplicationType, qPrintable(rmProcessInfo.processType));
            printf("%d.strAppName = %s\n", i, qPrintable(rmProcessInfo.processDisplayName));
            printf("%d.Process.dwProcessId = %d\n", i, rmProcessInfoList[i].Process.dwProcessId);
#endif

            Auto_HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, rmProcessInfoList[i].Process.dwProcessId);
            if (hProcess)
            {
                // This is necessary in case a process ID was recycled between now and when we had called RmGetList
                // Combination of start time and process ID is unique
                FILETIME ftCreate, ftExit, ftKernel, ftUser;
                if (GetProcessTimes(hProcess, &ftCreate, &ftExit, &ftKernel, &ftUser) 
                    && CompareFileTime(&rmProcessInfoList[i].Process.ProcessStartTime, &ftCreate) == 0) 
                {
                    QString processPath = getProcessPath(hProcess);

                    // If we got a valid process path
                    if (processPath.length() > 0)
                    {
#ifdef _DEBUG
                        printf("  = %s\n", qPrintable(processPath));
#endif

                        rmProcessInfo.processPath = processPath;

                        // Add it to the list
                        processInfoList.push_back(rmProcessInfo);
                    }
                }
            }
        }

        return true;
    }

    bool forceShutdown()
    {
        if (!_valid)
            return false;

        if (!_canShutdown)
            return false;

        _error = s_rmgr.PfRmShutdown(_session, RmForceShutdown, NULL);
        if (_error != ERROR_SUCCESS)
            return false;
        return true;
    }

private:
    QString getProcessPath(HANDLE hProcess)
    {
        QString processPath;

        WCHAR szProcessPath[MAX_PATH+1] = {0};
        DWORD cch = MAX_PATH;
        if (s_rmgr.PfQueryFullProcessImageName(hProcess, 0, szProcessPath, &cch) && cch <= MAX_PATH)
        {
            processPath = QString::fromWCharArray(szProcessPath);
        }

        return processPath;
    }

private:
    DWORD _session;
    bool _valid;
    bool _canShutdown;
    DWORD _shutdownReason;
    DWORD _error;
};

///////////////////////////////////////////////////////////////////////////

bool EnvUtils::GetFileInUseDetails(const QString& filePath, QString& rebootReason, QString& lockingProcessesSummary, QList<FileLockProcessInfo>& processes)
{
    bool isXP = false;
    OSVERSIONINFO	osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&osvi))
    {
        if (osvi.dwMajorVersion == 5 && (osvi.dwMinorVersion == 1 || osvi.dwMinorVersion == 2))
        {
            isXP = true;
        }
    }

    // This API is not supported on Windows XP
    if (isXP)
        return false;

    RestartManagerSession rmSession;
    if (!rmSession.valid())
    {
        ORIGIN_LOG_ERROR << "Invalid Restart Manager session ; Error = " << rmSession.error();
        return false;
    }

    // Register the file we want to check
    rmSession.registerFileResource(filePath);

    // Ask the Restart Manager API to return a list of the processes locking it
    if (!rmSession.getList(processes))
    {
        ORIGIN_LOG_ERROR << "Unable to getList from Restart Manager session ; Error = " << rmSession.error();
        return false;
    }
    
    // Get the 'reason' why a reboot might be required (e.g. 'files in use by critical process')
    rebootReason = rmSession.shutdownReasonString();

    if (processes.count() > 0)
    {
        // Create a summary string of all the processes locking the file
        lockingProcessesSummary = QString("File in use by %1 processes.").arg(processes.count());

        for (int i = 0; i < processes.count(); ++i)
        {
            lockingProcessesSummary.append(QString(" (%1: %2)").arg(i+1).arg(processes[i].toString()));
        }
    }

    return true;
}

#endif

bool EnvUtils::GetFileExistsNative(const QString& filePath)
{
    qint64 fileSize = 0; // Unused
    return GetFileDetailsNative(filePath, fileSize);
}

bool EnvUtils::GetFileDetailsNative(const QString& filePath, qint64& fileSize)
{
#ifdef ORIGIN_PC
    // Qt handles shortcut (.lnk) in an utterly stupid way on Windows, it treats them as symlinks
    // Furthermore none of the QFile objects can operate on the .lnk file itself, they all go to the file it points to, which is incorrect for our purposes
    // Therefore we must use the native APIs
    fileSize = 0;

    WIN32_FILE_ATTRIBUTE_DATA fad;

    // If it doesn't exist, this will return false
    if (!GetFileAttributesEx(filePath.utf16(), GetFileExInfoStandard, &fad))
        return false;
    LARGE_INTEGER size;
    size.HighPart = fad.nFileSizeHigh;
    size.LowPart = fad.nFileSizeLow;
    fileSize = size.QuadPart;

    return true;
#else
    // Symlinks are processed a bit differently than regular files - the size of the link is
    // the length of the path it points to
    //    
    QFileInfo finfo(filePath);
    if (finfo.isSymLink())
    {
        // We want to get the size of the relative link
        char linktarget[512];
        memset(linktarget,0,sizeof(linktarget));
        ssize_t targetSize = readlink(qPrintable(filePath), linktarget, sizeof(linktarget)-1);
        fileSize = (qint64)targetSize;

        // Since it is a symlink, it obviously exists
        return true;
    }
    else
    {
        // Not a symlink, just fetch the size using standard means
        fileSize = finfo.size();

        return finfo.exists();
    }
#endif
}
