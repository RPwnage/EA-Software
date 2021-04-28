///////////////////////////////////////////////////////////////////////////////
// EbisuTelemetryAPI.cpp
//
// Implements a ebisu specific API for the telemetry
//
// Owner: Origin Dev Support
// Copyright (c) Electronic Arts. All rights reserved.
//
// Telemetry definition reference at:
// https://confluence.ea.com/pages/viewpage.action?title=DevSupport+-+Telemetry+Hooks&spaceKey=EBI
//
///////////////////////////////////////////////////////////////////////////////

#include "EbisuTelemetryAPI.h"
#include "EbisuMetrics.h"
#include "TelemetryConfig.h"
#include "GameSession.h"
#include "DownloadSession.h"
#include "InstallSession.h"
#include "TelemetryLoginSession.h"

#include "SystemInformation.h"
#include "Utilities.h"

#include <EAIO/PathString.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAIniFile.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EAString.h>
#include <EASTL/string.h>
#include <eathread/eathread_futex.h>
#include <eathread/eathread_thread.h>
#include <EATrace/EATrace.h>
#include "EAStdC/EADateTime.h"

#if defined(ORIGIN_PC)
#include <IPTypes.h>
#include <IPHlpApi.h>
#endif

#if defined(ORIGIN_MAC)
#include <cstdlib>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <libproc.h>

#include <mach/mach.h>
#include <mach/task.h>
#include <mach/task_info.h>
#include <mach/vm_map.h>

#elif defined(ORIGIN_PC)
#include <windows.h>

#elif defined(__linux__)
#include <assert.h>

#endif


namespace OriginTelemetry
{
    const uint64_t TimeDivisorMS = UINT64_C(1000000);

    static const char* IGOEnableSettingStrings[] = 
    {
        "Disabled",
        "AllGames",
        "SupportedGames"
    };

    static const char* DefaultTabSettingStrings[] = 
    {
        "Home",
        "MyGames",
        "Store",
        "Decide"
    };

    static const char* LaunchStrings[] =
    {
        "unknown",
        "desktop",
        "client", 
        "mpinvite",
        "ito",
        "sdk",
        "softwareid"
    };

    static const char* TelemetrySendSettingStrings[] = 
    {
        "notset",
        "critical",
        "all"
    };

    static const char* TelemetryUnderageSettingStrings[] = 
    {
        "notset",
        "true",
        "false"
    };

    const char* PowerModeStrings[] =
    {
#define TELEMETRYPOWERMODEDEF(e) #e,
        TELEMETRYPOWERMODEDEFS
#undef TELEMETRYPOWERMODEDEF
    };

    // strips folder info info from filepath
    void StripFolderInfo(eastl::string8 &path)
    {
        // converts a file path like "C:\dev\global_online\...\ContentInstallFlowDiP.cpp" to just "ContentInstallFlow.cpp"

        // unify our slash direction
        eastl_size_t i=0;
        while((i = path.find("/")) != eastl::string8::npos)
            path.replace(i, 1, "\\");

        eastl::string8::size_type pos=path.find_last_of("\\");
        if (pos!=eastl::string8::npos)
            path=path.substr(path.length()>pos+1 ? pos+1 : pos, path.length());
    }

}

using namespace OriginTelemetry;

// little session helpers
template <class T> class SessionListHelper
{
private:
    EA::Thread::Futex m_lock;

public:
    typedef eastl::pair< eastl::string8, T* > SessionPair;
    typedef eastl::map<eastl::string8, T*> SessionListType;
    eastl::map<eastl::string8, T*> map;

    EA::Thread::Futex& GetLock() { return m_lock; } 

    ~SessionListHelper()
    {
        for ( typename SessionListType::iterator iter = map.begin(); iter != map.end(); ++iter )
        {
            if ( iter->second != NULL)
                delete iter->second;
        }
    }
};

EbisuTelemetryAPI::EbisuTelemetryAPI()
{   
#ifdef _DEBUG       
    //_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
    gameSessionList = NULL;
    downloadSessionList = NULL;
    installSessionList = NULL;
    loginSession = NULL;
    m_sysInfo = &NonQTOriginServices::SystemInformation::instance();
    mIsFreeToPlayITO = false;
    mUsingAutoStart = false;
    m_start_games = 0;
    mGameLaunchSource = Launch_Unknown;

    m_start_time = EA::StdC::GetTime() / TimeDivisorMS;

    m_active_timer_started = false;
    m_running_timer_started = false;
    m_active_time = 0;
    m_total_active_time = 0;
    m_running_time = 0;

    gameSessionList = new GameSessionList;
    downloadSessionList = new DownloadSessionList();
    installSessionList = new InstallSessionList();
    loginSession = new TelemetryLoginSession();
}

EbisuTelemetryAPI::~EbisuTelemetryAPI()
{
    if (gameSessionList)
        delete gameSessionList;

    gameSessionList = NULL;

    if (downloadSessionList)
        delete downloadSessionList;

    downloadSessionList = NULL;

    if (installSessionList)
        delete installSessionList;

    installSessionList = NULL;

    if(loginSession)
        delete loginSession;

    loginSession = NULL;

    m_sysInfo = NULL;
}

//  Added accessor function to avoid client code from working with SystemInformation.h
//  directly since it requires certain Windows header files.
bool EbisuTelemetryAPI::IsComputerSurfacePro() const
{
    if (m_sysInfo != NULL)
        return (m_sysInfo->IsComputerSurfacePro() == 1);
    return false;
}

const char8_t* EbisuTelemetryAPI::GetMacAddr() const
{
    if (m_sysInfo != NULL)
        return m_sysInfo->GetMacAddr();
    return NULL;
}

const eastl::string8 EbisuTelemetryAPI::GetMacHash() const
{
    return OriginTelemetry::TelemetryConfig::uniqueMachineHash();
}

void EbisuTelemetryAPI::SetIsFreeToPlayITO(bool b)
{
    mIsFreeToPlayITO = b;
}

void EbisuTelemetryAPI::SetGameLaunchSource(LaunchSource source)
{
    mGameLaunchSource = source;
}

void EbisuTelemetryAPI::setTelemetryServer(const eastl::string8& telemetryServer)
{
    TelemetryConfig::setTelemetryServer(telemetryServer);
}

void EbisuTelemetryAPI::setHooksBlacklist(const eastl::string8& blacklist)
{
    TelemetryConfig::setHooksBlacklist(blacklist);
}

void EbisuTelemetryAPI::setCountriesBlacklist(const eastl::string8& blacklist)
{
    TelemetryConfig::setCountryBlacklist(blacklist);
}

void EbisuTelemetryAPI::setAlternateSoftwareIDLaunch(const eastl::string8& source_product_id, const eastl::string8& target_product_id)
{
    if (m_alt_launcher_info != NULL)
    {
        //If there is data in the pair when we try to set new data it means the original data never sent. This is an error!
        this->Metric_ALTLAUNCHER_SESSION_FAIL(m_alt_launcher_info->first.c_str(), "software_id", "game never launched", m_alt_launcher_info->second.c_str());
        delete m_alt_launcher_info;
    }
    m_alt_launcher_info = new eastl::pair<eastl::string8,eastl::string8>(source_product_id, target_product_id);
}

void EbisuTelemetryAPI::setSDKStartGameLaunch(const eastl::string8& source_product_id, const eastl::string8& target_product_id)
{
    if (m_sdk_launching_info != NULL)
    {
        //If there is data in the pair when we try to set new data it means the original data never sent. This is an error!
        this->Metric_GAME_ALTLAUNCHER_FAIL(m_sdk_launching_info->first.c_str(), m_sdk_launching_info->second.c_str(), "Lost");
        delete m_sdk_launching_info;
    }
    m_sdk_launching_info = new eastl::pair<eastl::string8, eastl::string8>(source_product_id, target_product_id);

}

uint64_t EbisuTelemetryAPI::CalculateIdFromString8(const char8_t *str)
{
    return NonQTOriginServices::Utilities::calculateFNV1A(str);
}

uint64_t EbisuTelemetryAPI::CalculateIdFromString16(const char16_t *str)
{
    return NonQTOriginServices::Utilities::calculateFNV1A(EA::StdC::ConvertString<eastl::string16, eastl::string8>(str).c_str());
}


void EbisuTelemetryAPI::SetBootstrapVersion(const char8_t *versionString)
{
    m_sysInfo->SetBootstrapVersion(versionString);
}

int EbisuTelemetryAPI::GetTelemetryEnum( va_list vl )
{
    int telemetryEnum = va_arg( vl, int32_t );
    return telemetryEnum;
}

enum ThreadAction{
    THREADACTION_SUSPEND,
    THREADACTION_RESUME,
};

#if defined(ORIGIN_PC)
#include <Tlhelp32.h>

static void SuspendResumeAllOtherThreads(ThreadAction action)
{
    DWORD currentThreadId = GetCurrentThreadId();
    DWORD currentProcessId = GetCurrentProcessId();

    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (h != INVALID_HANDLE_VALUE)
    {
        THREADENTRY32 te;
        te.dwSize = sizeof(te);
        if (Thread32First(h, &te))
        {
            do
            {
                if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID)) 
                {
                    // Suspend all threads EXCEPT the one we want to keep running
                    if(te.th32ThreadID != currentThreadId && te.th32OwnerProcessID == currentProcessId)
                    {
                        HANDLE thread = ::OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
                        if(thread != NULL)
                        {
                            switch(action)
                            {
                            case THREADACTION_SUSPEND:
                                SuspendThread(thread);
                                break;
                            case THREADACTION_RESUME:
                                ResumeThread(thread);
                                break;
                            default:
                                break;
                            };

                            CloseHandle(thread);
                        }
                    }
                }
                te.dwSize = sizeof(te);
            } while (Thread32Next(h, &te));
        }
        CloseHandle(h);  
    }
}
#elif defined(ORIGIN_MAC)

static void SuspendResumeAllOtherThreads(ThreadAction action)
{
    // Prepare to either suspend or resume threads.
    kern_return_t (*threadOperation)(thread_act_t);
    if (action == THREADACTION_SUSPEND)
        threadOperation = thread_suspend;
    else if (action == THREADACTION_RESUME)
        threadOperation = thread_resume;
    else
        return; // Fail if unknown function.

    // Get the threads for the current task.
    mach_port_t currentTask = mach_task_self();
    thread_port_t currentThread = mach_thread_self();
    thread_port_array_t allThreads;
    unsigned allThreadsCount;
    if (task_threads(currentTask, &allThreads, &allThreadsCount) == KERN_SUCCESS)
    {
        // Loop over all threads.
        for (int i=0; i != allThreadsCount; ++i)
        {
            // If the thread is not the current thread.
            if (allThreads[i] != currentThread)
            {
                // Perform the requested action.
                (*threadOperation)(allThreads[i]);

                mach_port_deallocate(currentTask, allThreads[i]);
            }
        }
        // Clean up memory.
        vm_deallocate(currentTask, (vm_address_t) allThreads, sizeof(*allThreads) * allThreadsCount);
    }
}

#elif defined(__linux__)

static void SuspendResumeAllOtherThreads(ThreadAction action)
{
    if (action == THREADACTION_SUSPEND)
    {
        assert(false);
    }
    else if (action == THREADACTION_RESUME)
    {
        assert(false);
    }
}

#endif

static void SuspendAllOtherThreads()
{
    SuspendResumeAllOtherThreads(THREADACTION_SUSPEND);
}

static void ResumeAllOtherThreads()
{
    SuspendResumeAllOtherThreads(THREADACTION_RESUME);
}





// WARNING: If you change these, you will also need to change them in 'TelemetryBreakpointWindow.cpp'.
enum ExitCode
{
    EXITCODE_CONTINUE = 2,
    EXITCODE_IGNORE = 3,
    EXITCODE_IGNOREALL = 4
};

void EbisuTelemetryAPI::HandleTelemetryBreakpoint(const TelemetryDataFormatTypes *format, va_list vl)
{
    int breakpointAction=0;

    int telemetryEnum = GetTelemetryEnum( vl );

    eastl::map<int32_t, eastl::wstring> telemetryBreakpointMap = TelemetryConfig::telemetryBreakpointMap();

    eastl::map<int, eastl::wstring>::iterator it = telemetryBreakpointMap.find(telemetryEnum);
    if( it != telemetryBreakpointMap.end() )
    {

        // Suspend all other threads
        SuspendAllOtherThreads();

#if defined(ORIGIN_PC)

        // Launch and wait for the 'TelemetryBreakpointHandler.exe'
        wchar_t commandLine[MAX_PATH*5];
        STARTUPINFOW startupInfo;
        memset(&startupInfo,0,sizeof(startupInfo));
        PROCESS_INFORMATION procInfo;
        memset(&procInfo,0,sizeof(procInfo));

        int size = swprintf_s(commandLine, L"%s %s", L"TelemetryBreakpointHandler.exe", (*it).second.c_str());
        if( size <= 0 ) return;

        BOOL result = ::CreateProcess(NULL, commandLine, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &procInfo);
        if (result)
        {
            WaitForSingleObject(procInfo.hProcess, INFINITE);

            DWORD exitCode = 0;
            GetExitCodeProcess(procInfo.hProcess, &exitCode);
            breakpointAction = exitCode;

            CloseHandle(procInfo.hProcess);
        }

#elif defined(ORIGIN_MAC)

        // Find the path of the current executable image.
        char tmpPath[PROC_PIDPATHINFO_MAXSIZE];
        char executablePath[PROC_PIDPATHINFO_MAXSIZE];
        proc_pidpath(getpid(), tmpPath, sizeof(tmpPath));
        eastl::string path(tmpPath);
        int pos = path.rfind( "/" );

        // Bail if we cannot find the last path component in the current executable path.
        if (pos == eastl::string::npos) return;

        // Set the executable path for the TelemetryBreakpointHandler.
        path.erase( pos );
        path.append( "/TelemetryBreakpointHandler" );

        // Prepare the command-line arguments for the TelemetryBreakpointHandler.
        char breakpointString[512];
        eastl::string breakpoint;
        EA::StdC::Strlcpy(breakpoint, (*it).second);
        snprintf(executablePath, sizeof(executablePath), "%s",  path.c_str());
        snprintf(breakpointString, sizeof(breakpointString), "%s",  breakpoint.c_str());

        // Assemble the complete command-line for the TelemetryBreakpointHandler.
        char* args[3];
        args[0] = executablePath;
        args[1] = breakpointString;
        args[2] = 0;

        // Fork into a parent and child process.
        int pid = fork();

        // If we are the parent,
        if ( pid )
        {
            // Wait for the TelemetryBreakpointHandler exit code.
            int status, ret;
            for (ret = waitpid( pid, &status, 0 ); ret == -1 && errno == EINTR; ret = waitpid( pid, &status, 0 )); // loop over EINTR

            // If the child did not successfully exit (i.e., it crashed),
            if ( ! WIFEXITED( status ) )
            {
                // Ingore all future breakpoints.
                breakpointAction = EXITCODE_IGNOREALL;

                // Write a message to stderr.
                const char* error = "Error: TelemetryBreakpointHandler encountered a fatal error.\n";
                write(2, error, strlen(error));
            }
            // Otherwise,
            else
            {
                // Get the return value.
                breakpointAction = WEXITSTATUS( status );

                // If the return value is the special code 250,
                if (breakpointAction == 250)
                {
                    // Ingore all future breakpoints.
                    breakpointAction = EXITCODE_IGNOREALL;

                    // Write a message to stderr.
                    const char* error = "Error: Unable to find+execute TelemetryBreakpointHandler.\n";
                    write(2, error, strlen(error));
                }
            }
        }

        // Otherwise,
        else
        {
            // SECURITY: close all file descriptors beyond stdin, stdout, and stderrr.
            struct rlimit lim;
            getrlimit( RLIMIT_NOFILE, &lim );
            for ( rlim_t i = 3; i != lim.rlim_cur; ++i ) close( i );

            // Exec the TelemetryBreakpointHandler.
            execv(args[0], args);

            // Write a message to stderr.
            const char* error = "Error: Unable to execv TelemetryBreakpointHandler.\n";
            write(2, error, strlen(error));

            // If we get here, the TelemetryBreakpointHandler executable is not available or executable.
            // Exit with a special code indicating there is a failure.
            _exit(250);
        }

#elif defined(__linux__)
        assert(false);  
#endif

        // Resume all other threads
        ResumeAllOtherThreads();

        // When 'TelemetryBreakpointHandler' is done, process the button click.
        switch( breakpointAction )
        {
        case EXITCODE_CONTINUE:
            // do nothing
            break;
        case EXITCODE_IGNORE:
            IgnoreTelemetryBreakpoint(telemetryEnum);
            break;
        case EXITCODE_IGNOREALL:
            TelemetryConfig::disableTelemetryBreakpoints();
            break;
        default:
            break;
        }
    }
}

void EbisuTelemetryAPI::IgnoreTelemetryBreakpoint(int id)
{
    TelemetryConfig::telemetryBreakpointMap().erase(id);
}

void EbisuTelemetryAPI::UpdateDownloadSession(const TYPE_S8 product_id, TYPE_U64 bytes_downloaded, TYPE_U64 DL_bitrate_kbps)
{ 
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    DownloadSessionList::SessionListType::iterator iter=downloadSessionList->map.find(Id);
    if ( iter != downloadSessionList->map.end() )
    {
        iter->second->setBytesDownloaded(bytes_downloaded);
        iter->second->setBitrateKbps(DL_bitrate_kbps);
    }
}


//-----------------------------------------------------------------------------
//  Locale
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::SetLocale(const char* pLocale)
{
    // convert language from de_DE into deDE
    eastl::string8 lang = pLocale;
    eastl::string8::size_type pos = lang.find("_");
    if ( pos!=eastl::string8::npos )
        lang = lang.replace(pos, 1, "");

    m_sysInfo->SetLocaleSetting(lang.c_str());
}


///////////////////////////////////////////////////////////////////////////////
//  METRIC CALLS
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_APP
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_APP_INSTALL(const TYPE_S8 client_version_number)
{
    // wait for the hardware survay thread to fill in
    // the system data before firing the hook
    // so we have the default browser string ready

    CaptureTelemetryData(METRIC_APP_INSTALL, kMETRIC_APP_INSTALL, kMETRIC_GROUP_APP, 
        (mIsFreeToPlayITO ? 1 : 0),
        client_version_number, 
        m_sysInfo->GetBrowserName());
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_APP
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_APP_UNINSTALL(const TYPE_S8 client_version_number)
{
    CaptureTelemetryData(METRIC_APP_UNINSTALL, kMETRIC_APP_UNINSTALL, kMETRIC_GROUP_APP, 
        client_version_number);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_APP
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_APP_MAC_ESCALATION_DENIED()
{
    CaptureTelemetryData(METRIC_APP_MAC_ESCALATION_DENIED, kMETRIC_APP_MAC_ESCALATION_DENIED, kMETRIC_GROUP_APP);
}

//-----------------------------------------------------------------------------
//   kMETRIC_GROUP_APP
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_APP_ESCALATION_ERROR(const TYPE_S8 escalationCmd, const TYPE_I32 errorType, const TYPE_I32 sysErrorCode, const TYPE_S8 escalationReason, const TYPE_S8 errorDescrip)
{
    CaptureTelemetryData(METRIC_APP_ESCALATION_ERROR, kMETRIC_APP_ESCALATION_ERROR, kMETRIC_GROUP_APP,
        escalationCmd,
        errorType,
        sysErrorCode,
        escalationReason,
        errorDescrip);
}

//-----------------------------------------------------------------------------
//   kMETRIC_GROUP_APP
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_APP_ESCALATION_SUCCESS(const TYPE_S8 escalationCmd, const TYPE_S8 escalationReason)
{
    CaptureTelemetryData(METRIC_APP_ESCALATION_SUCCESS, kMETRIC_APP_ESCALATION_SUCCESS, kMETRIC_GROUP_APP,
        escalationCmd,
        escalationReason);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_APP
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_APP_SESSION_ID(const TYPE_S8 sessionId)
{
    CaptureTelemetryData(METRIC_APP_SESSION_ID, kMETRIC_APP_SESSION_ID, kMETRIC_GROUP_APP, 
        sessionId);
}


/// Send BOOT/SESS/STRT Telemetry Hook.
/// Sent immediately upon application start.
void EbisuTelemetryAPI::Metric_APP_START()
{
    CaptureTelemetryData(METRIC_APP_START, kMETRIC_APP_START, kMETRIC_GROUP_APP);
}


/// Send BOOT SESS SETT Telemetry Hook.
/// Should be sent at start as soon as we have all the settings
/// that are to be sent. 
void EbisuTelemetryAPI::Metric_APP_SETTINGS( bool isBeta, bool isElevatedUser, bool wasStartedFromAutoRun, bool isFreeToPlay )
{
    mUsingAutoStart = wasStartedFromAutoRun;
    mIsFreeToPlayITO = isFreeToPlay;

    CaptureTelemetryData(METRIC_APP_SETTINGS, kMETRIC_APP_SETTINGS, kMETRIC_GROUP_APP, 
        (isElevatedUser ? 1 : 0),
        (mUsingAutoStart ? 1 : 0),
        (isBeta ? 1 : 0),
        (mIsFreeToPlayITO ? 1 : 0),
        m_sysInfo->GetBootstrapVersion());
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_APP
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_APP_CMD(const TYPE_S8 cmdLine)
{
    CaptureTelemetryData(METRIC_APP_CMD, kMETRIC_APP_CMD, kMETRIC_GROUP_APP, 
        cmdLine);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_APP
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_APP_CONFIG(const TYPE_S8 configKey, const TYPE_S8 configValue, bool isOverride)
{
    CaptureTelemetryData(METRIC_APP_CONFIG, kMETRIC_APP_CONFIG, kMETRIC_GROUP_APP, 
        configKey,
        configValue,
        (isOverride ? 1 : 0));
}
//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_APP
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_APP_END()
{
    if (m_start_time == 0) // we already in a start / end phase
        return;

    uint64_t currtime = EA::StdC::GetTime() / TimeDivisorMS;
    uint32_t sessionlen = static_cast<int>((currtime - m_start_time) / 1000); // seconds

    CaptureTelemetryData(METRIC_APP_END, kMETRIC_APP_END, kMETRIC_GROUP_APP, 
        static_cast<int>(m_start_time / 1000),
        static_cast<int>(currtime / 1000),
        sessionlen, 
        (mIsFreeToPlayITO ? 1 : 0)
        );
}

//-----------------------------------------------------------------------------
//  APPLICATION:  CONNECTION:  Lost network connection
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_APP_CONNECTION_LOST()
{
    CaptureTelemetryData(METRIC_APP_CONNECTION_LOST, kMETRIC_APP_CONNECTION_LOST, kMETRIC_GROUP_APP);
}

//-----------------------------------------------------------------------------
//  APPLICATION:  EXECUTION:  Application terminated prematurely
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_APP_PREMATURE_TERMINATION()
{
    CaptureTelemetryData(METRIC_APP_PREMATURE_TERMINATION, kMETRIC_APP_PREMATURE_TERMINATION, kMETRIC_GROUP_APP);
}


//-----------------------------------------------------------------------------
//  APPLICATION:  POWER:  Changes to OS power settings
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_APP_POWER_MODE(PowerModeEnum mode)
{
    CaptureTelemetryData(METRIC_APP_POWER_MODE, kMETRIC_APP_POWER_MODE, kMETRIC_GROUP_APP, 
        static_cast<int>(EA::StdC::GetTime() / TimeDivisorMS / 1000),
        PowerModeStrings[mode]);
}

//-----------------------------------------------------------------------------
//  APPLICATION:  MOTD:  Message of the Day shown
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_APP_MOTD_SHOW(const TYPE_S8 url, const TYPE_S8 text)
{
    CaptureTelemetryData(METRIC_APP_MOTD_SHOW, kMETRIC_APP_MOTD_SHOW, kMETRIC_GROUP_APP, 
        url,
        text);
}

//-----------------------------------------------------------------------------
//  APPLICATION:  MOTD:  Message of the Day closed
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_APP_MOTD_CLOSE()
{
    CaptureTelemetryData(METRIC_APP_MOTD_CLOSE, kMETRIC_APP_MOTD_CLOSE, kMETRIC_GROUP_APP);
}

//-----------------------------------------------------------------------------
//  APPLICATION:  DYNAMIC URLS: Dynamic URL config file loaded
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_APP_DYNAMIC_URL_LOAD(const TYPE_S8 file, const TYPE_S8 revision, bool isOverride)
{
    CaptureTelemetryData(METRIC_APP_DYNAMIC_URL_LOAD, kMETRIC_APP_DYNAMIC_URL_LOAD, kMETRIC_GROUP_APP,
        file,
        revision,
        (isOverride ? 1 : 0));
}
//-----------------------------------------------------------------------------
//  APPLICATION:  ADDITIONAL CLIENT LOGS: Send when we write to Client_Log#.txt
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_APP_ADDITIONAL_CLIENT_LOG(const TYPE_S8 fileName)
{
    CaptureTelemetryData(METRIC_APP_ADDITIONAL_CLIENT_LOG, kMETRIC_APP_ADDITIONAL_CLIENT_LOG, kMETRIC_GROUP_APP,
        fileName);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_LOGIN_START(const TYPE_S8 type)
{
    EA::Thread::AutoFutex m2(loginSession->GetLock());

    CaptureTelemetryData(METRIC_LOGIN_START, kMETRIC_LOGIN_START, kMETRIC_GROUP_USER,
        type,
        loginSession->startedByLocalHost(),
        loginSession->localHostOriginUri(),
        (mIsFreeToPlayITO ? 1 : 0));
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_LOGIN_OK(bool mode, bool underage, const TYPE_S8 type, bool isHwOptOut)
{
    EA::Thread::AutoFutex m2(loginSession->GetLock());

    CaptureTelemetryData(METRIC_LOGIN_OK, kMETRIC_LOGIN_OK, kMETRIC_GROUP_USER,
        mode         ? "online" : "offline",
        underage     ? 1 : 0,
        type,
        loginSession->startedByLocalHost(),
        loginSession->localHostOriginUri(),
        (mIsFreeToPlayITO ? 1 : 0));

    loginSession->reset();
    m_login_time = (EA::StdC::GetTime() / TimeDivisorMS);

    if (isHwOptOut)
        return;

    //  Send hardware survey data
    if (!m_sysInfo->isChecksumValid())
    {
        Metric_HW_OS();
        Metric_HW_CPU();
        Metric_HW_VIDEO();
        Metric_HW_WEBCAM();
        Metric_HW_DISPLAY();
        Metric_HW_RESOLUTION();
        Metric_HW_MEM();
        Metric_HW_MICROPHONE();
        Metric_HW_HDD();
        Metric_HW_ODD();
    }
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_LOGIN_FAIL(const TYPE_S8 svcd, const TYPE_S8 code, const TYPE_S8 type, 
                                                         const TYPE_I32 http, const TYPE_S8 url, const TYPE_U32 qt, const TYPE_U32 subtype, const TYPE_S8 description,
                                                         const TYPE_S8 qtErrorString)
{
    m_login_time = 0;

    EA::Thread::AutoFutex m2(loginSession->GetLock());

    CaptureTelemetryData(METRIC_LOGIN_FAIL, kMETRIC_LOGIN_FAIL, kMETRIC_GROUP_USER, 
        svcd, 
        description,
        code,
        type,
        http,
        url,
        qt,
        subtype,
        (mUsingAutoStart ? 1 : 0),
        loginSession->startedByLocalHost(),
        loginSession->localHostOriginUri(),
        (mIsFreeToPlayITO ? 1 : 0),
        qtErrorString);

    loginSession->reset();


}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_LOGIN_COOKIE_LOAD(const TYPE_S8 name, const TYPE_S8 value, const TYPE_S8 expiration)
{
    EA::Thread::AutoFutex m2(loginSession->GetLock());

    CaptureTelemetryData(METRIC_LOGIN_COOKIE_LOAD, kMETRIC_LOGIN_COOKIE_LOAD, kMETRIC_GROUP_USER,
        name,
        value,
        expiration);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_LOGIN_COOKIE_SAVE(const TYPE_S8 name, const TYPE_S8 value, const TYPE_S8 expiration, const TYPE_U32 ecode, const TYPE_U32 count)
{
    EA::Thread::AutoFutex m2(loginSession->GetLock());

    CaptureTelemetryData(METRIC_LOGIN_COOKIE_SAVE, kMETRIC_LOGIN_COOKIE_SAVE, kMETRIC_GROUP_USER,
        name,
        value,
        expiration,
        ecode,
        count);
}


//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_LOGOUT(const TYPE_S8 reason)
{
    uint64_t current_time = EA::StdC::GetTime() / TimeDivisorMS;
    uint32_t loginsessiontime = (uint32_t) (current_time - m_login_time) / 1000; // seconds
    CaptureTelemetryData(METRIC_LOGOUT, kMETRIC_LOGOUT, kMETRIC_GROUP_USER, 
        static_cast<int>(m_login_time / 1000),
        static_cast<int>(current_time / 1000),
        loginsessiontime, 
        reason);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_LOGIN_OEM(const TYPE_S8 oem)
{
    CaptureTelemetryData(METRIC_LOGIN_OEM, kMETRIC_LOGIN_OEM, kMETRIC_GROUP_USER,
        oem);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_WEB_APPLICATION_CACHE_CLEAR()
{
    CaptureTelemetryData(METRIC_WEB_APPLICATION_CACHE_CLEAR, kMETRIC_WEB_APPLICATION_CACHE_CLEAR, kMETRIC_GROUP_USER);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_WEB_APPLICATION_CACHE_DOWNLOAD()
{
    CaptureTelemetryData(METRIC_WEB_APPLICATION_CACHE_DOWNLOAD, kMETRIC_WEB_APPLICATION_CACHE_DOWNLOAD, kMETRIC_GROUP_USER);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_NUX_START(const TYPE_S8 step, bool autoStart)
{
    CaptureTelemetryData(METRIC_NUX_START, kMETRIC_NUX_START, kMETRIC_GROUP_USER, 
        step,
        (autoStart ? 1 : 0),
        (mIsFreeToPlayITO ? 1 : 0));
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_NUX_CANCEL(const TYPE_S8 step)
{
    CaptureTelemetryData(METRIC_NUX_CANCEL, kMETRIC_NUX_CANCEL, kMETRIC_GROUP_USER, 
        step);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_NUX_END(const TYPE_S8 step)
{
    CaptureTelemetryData(METRIC_NUX_END, kMETRIC_NUX_END, kMETRIC_GROUP_USER, 
        step);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_NUX_PROFILE(bool fName, bool lName, bool avatarWindow, bool avatarGallery, bool avatarUpload)
{
    CaptureTelemetryData(METRIC_NUX_PROFILE, kMETRIC_NUX_PROFILE, kMETRIC_GROUP_USER, 
        fName         ? 1 : 0,
        lName         ? 1 : 0,
        avatarWindow  ? 1 : 0,
        avatarGallery ? 1 : 0,
        avatarUpload  ? 1 : 0);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_NUX_EMAILDUPLICATE()
{
    CaptureTelemetryData(METRIC_NUX_EMAILDUP, kMETRIC_NUX_EMAILDUP, kMETRIC_GROUP_USER);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PRIVACY_START()
{
    CaptureTelemetryData(METRIC_LOGIN_PRIVSTRT, kMETRIC_LOGIN_PRIVSTRT, kMETRIC_GROUP_USER);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PRIVACY_CANCEL()
{
    CaptureTelemetryData(METRIC_LOGIN_PRIVCANC, kMETRIC_LOGIN_PRIVCANC, kMETRIC_GROUP_USER);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PRIVACY_END()
{
    CaptureTelemetryData(METRIC_LOGIN_PRIVSUCC, kMETRIC_LOGIN_PRIVSUCC, kMETRIC_GROUP_USER);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_LOGIN_UNKNOWN(const TYPE_S8 type)
{
    CaptureTelemetryData(METRIC_LOGIN_UNKNOWN, kMETRIC_LOGIN_UNKNOWN, kMETRIC_GROUP_USER, 
        type);
}

//-----------------------------------------------------------------------------
//  kMETRIC_LOGIN_FAVORITES
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_LOGIN_FAVORITES(const TYPE_I32 favorite)
{
    CaptureTelemetryData(METRIC_LOGIN_FAVORITES, kMETRIC_LOGIN_FAVORITES, kMETRIC_GROUP_USER, 
        favorite);
}

//-----------------------------------------------------------------------------
//  kMETRIC_LOGIN_HIDDEN
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_LOGIN_HIDDEN(const TYPE_I32 hidden)
{
    CaptureTelemetryData(METRIC_LOGIN_HIDDEN, kMETRIC_LOGIN_HIDDEN, kMETRIC_GROUP_USER, 
        hidden);
}

//-----------------------------------------------------------------------------
//  kMETRIC_AUTHENTICATION_LOST
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_AUTHENTICATION_LOST(const TYPE_I32 qNetworkError, const TYPE_I32 httpStatus, const TYPE_S8 url)
{
    CaptureTelemetryData(METRIC_AUTHENTICATION_LOST, kMETRIC_AUTHENTICATION_LOST, kMETRIC_GROUP_USER, 
        qNetworkError,
        httpStatus,
        url);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_NUX_OEM(const TYPE_S8 oem)
{
    CaptureTelemetryData(METRIC_NUX_OEM, kMETRIC_NUX_OEM, kMETRIC_GROUP_USER, 
        oem);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PASSWORD_SENT()
{
    CaptureTelemetryData(METRIC_PASSWORD_SENT, kMETRIC_PASSWORD_SENT, kMETRIC_GROUP_USER);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_USER
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PASSWORD_RESET()
{
    CaptureTelemetryData(METRIC_PASSWORD_RESET, kMETRIC_PASSWORD_RESET, kMETRIC_GROUP_USER);
}

//-----------------------------------------------------------------------------
//  USER:  CAPTCHA:  Start
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CAPTCHA_START()
{
    CaptureTelemetryData(METRIC_CAPTCHA_START, kMETRIC_CAPTCHA_START, kMETRIC_GROUP_USER);
}

//-----------------------------------------------------------------------------
//  USER:  CAPTCHA:  Fail
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CAPTCHA_FAIL()
{
    CaptureTelemetryData(METRIC_CAPTCHA_FAIL, kMETRIC_CAPTCHA_FAIL, kMETRIC_GROUP_USER);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_SETTINGS
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_SETTINGS( bool autoLogin, bool autoPatch, bool autoStart, bool autoClientUpdate, bool isBetaOptIn, 
    bool cloudSaveEnabled, bool hardwareOptOut, DefaultTabSetting defaultTabSetting, IGOEnableSetting igoEnableSetting )
{
    const char* IGOEnableString = IGOEnableSettingStrings[igoEnableSetting];
    const char* defaultTab = DefaultTabSettingStrings[defaultTabSetting];
    CaptureTelemetryData(METRIC_SETTINGS, kMETRIC_SETTINGS, kMETRIC_GROUP_SETTINGS, 
        m_sysInfo->GetLocaleSetting(), 
        autoLogin        ? 1 : 0, 
        autoPatch        ? 1 : 0, 
        autoStart        ? 1 : 0, 
        autoClientUpdate ? 1 : 0,
        isBetaOptIn        ? 1 : 0, 
        cloudSaveEnabled ? 1 : 0,
        //telemetry event has opt-in so we reverse the opt-out
        hardwareOptOut    ? 0 : 1,
        defaultTab,
        IGOEnableString, 
        m_sysInfo->GetUserLocale()
    );
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_SETTINGS
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PRIVACY_SETTINGS()
{
    CaptureTelemetryData(METRIC_PRIVACY_SETTINGS, kMETRIC_PRIVACY_SETTINGS, kMETRIC_GROUP_SETTINGS, 
        TelemetrySendSettingStrings[userSettingSendNonCriticalTelemetry()],
        TelemetryUnderageSettingStrings[isTelemetryUserUnderaged()],
        isSendingNonCriticalTelemetry()?"true":"false");
}

void EbisuTelemetryAPI::Metric_EMAIL_VERIFICATION_STARTS_CLIENT()
{
    CaptureTelemetryData(METRIC_EMAIL_VERIFICATION_STARTS_CLIENT, kMETRIC_EMAIL_VERIFICATION_STARTS_CLIENT, kMETRIC_GROUP_SETTINGS);
}

void EbisuTelemetryAPI::Metric_EMAIL_VERIFICATION_DISMISSED()
{
    CaptureTelemetryData(METRIC_EMAIL_VERIFICATION_DISMISSED, kMETRIC_EMAIL_VERIFICATION_DISMISSED, kMETRIC_GROUP_SETTINGS);
}

void EbisuTelemetryAPI::Metric_EMAIL_VERIFICATION_STARTS_BANNER()
{
    CaptureTelemetryData(METRIC_EMAIL_VERIFICATION_STARTS_BANNER, kMETRIC_EMAIL_VERIFICATION_STARTS_BANNER, kMETRIC_GROUP_SETTINGS);
}



void EbisuTelemetryAPI::Metric_QUIETMODE_ENABLED(bool disablePromoManager, bool ignoreNonMandatoryGameUpdates, bool shutdownOriginOnGameFinished)
{
    CaptureTelemetryData(METRIC_QUIETMODE_ENABLED, kMETRIC_QUIETMODE_ENABLED, kMETRIC_GROUP_SETTINGS, 
        disablePromoManager           ? 1 : 0,
        ignoreNonMandatoryGameUpdates ? 1 : 0,
        shutdownOriginOnGameFinished  ? 1 : 0);
}

//-----------------------------------------------------------------------------
//  kMETRIC_GROUP_SETTINGS
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_INVALID_SETTINGS_PATH_CHAR(const TYPE_S8 path)
{
    CaptureTelemetryData(METRIC_INVALID_SETTINGS_PATH_CHAR, kMETRIC_INVALID_SETTINGS_PATH_CHAR, kMETRIC_GROUP_SETTINGS,
        path);
}

void EbisuTelemetryAPI::Metric_SETTINGS_SYNC_FAILED(const TYPE_S8 settingsFile, const TYPE_S8 failReason,
                                                                   const TYPE_U32 readable, const TYPE_U32 writable, const TYPE_U32 readSysError, const TYPE_U32 writeSysError,
                                                                   const TYPE_S8 winFileAttrs, const TYPE_S8 permFlag)
{
    CaptureTelemetryData(METRIC_SETTINGS_SYNC_FAILED, kMETRIC_SETTINGS_SYNC_FAILED, kMETRIC_GROUP_SETTINGS,
        settingsFile,
        failReason,
        readable,
        writable,
        readSysError,
        writeSysError,
        winFileAttrs,
        permFlag);
}

void EbisuTelemetryAPI::Metric_SETTINGS_FILE_FIX_RESULT(const SettingFixResult result, const TYPE_U32 read, 
                                                                       const TYPE_U32 write, const TYPE_U32 qtFixSuccess, const TYPE_S8 settingsFile)
{
    CaptureTelemetryData(METRIC_SETTINGS_FILE_FIX_RESULT, kMETRIC_SETTINGS_FILE_FIX_RESULT, kMETRIC_GROUP_SETTINGS,
        static_cast<TYPE_U32>(result),
        read,
        write,
        qtFixSuccess,
        settingsFile);
}


///////////////////////////////////////////////////////////////////////////////
//  HARDWARE SURVEY
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  HARDWARE SURVEY:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HW_OS()
{
    CaptureTelemetryData(METRIC_HW_OS, kMETRIC_HW_OS, kMETRIC_GROUP_HARDWARE, 
        m_sysInfo->GetOSArchitecture(),
        m_sysInfo->GetOSBuildString(),
        (char8_t*)m_sysInfo->GetOSVersion(),
        (char8_t*)m_sysInfo->GetServicePackName(), 
        m_sysInfo->GetHyperVisorString());
}

//-----------------------------------------------------------------------------
//  HARDWARE SURVEY:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HW_CPU()
{
    CaptureTelemetryData(METRIC_HW_CPU, kMETRIC_HW_CPU, kMETRIC_GROUP_HARDWARE, 
        (char8_t*) m_sysInfo->GetCPUName(), 
        m_sysInfo->GetNumCPUs(),
        m_sysInfo->GetNumCores(),
        m_sysInfo->GetCPUSpeed(),
        m_sysInfo->GetInstructionSet(),
        m_sysInfo->GetBIOSString(),
        m_sysInfo->GetMotherboardString(),
        NonQTOriginServices::Utilities::MD5Hash(m_sysInfo->GetMotherboardSerialNumber()).c_str(),
        NonQTOriginServices::Utilities::MD5Hash(m_sysInfo->GetBIOSSerialNumber()).c_str(),
        m_sysInfo->IsComputerSurfacePro(),
        m_sysInfo->GetUniqueMachineHash());
}

//-----------------------------------------------------------------------------
//  HARDWARE SURVEY:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HW_VIDEO()
{
    // get all the video cards
    for (uint32_t i = 0; i < m_sysInfo->GetNumVideoControllers(); i++)
    {
        CaptureTelemetryData(METRIC_HW_VIDEO, kMETRIC_HW_VIDEO, kMETRIC_GROUP_HARDWARE, 
            m_sysInfo->GetNumVideoControllers(),
            i + 1,                                // TYPEID_U32  video card number
            m_sysInfo->GetVideoAdapterName(i),      // TYPEID_S8   video card name
            m_sysInfo->GetVideoAdapterBitsDepth(i), // TYPEID_U32  bit depth
            m_sysInfo->GetVideoAdapterVRAM(i),      // TYPEID_U32  vram
            m_sysInfo->GetVideoAdapterVendorID(i),  // TYPEID_U32  video card:  Vendor ID
            m_sysInfo->GetVideoAdapterDeviceID(i),  // TYPEID_U32  video card:  Device ID
            m_sysInfo->GetVideoAdapterDriverVersion(i));  // TYPEID_S8   video card:  driver version string
    }
}

//-----------------------------------------------------------------------------
//  HARDWARE SURVEY:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HW_WEBCAM()
{
    if (m_sysInfo->GetNumVideoCaptureDevice() == 0)
    {
        CaptureTelemetryData(METRIC_HW_WEBCAM, kMETRIC_HW_WEBCAM, kMETRIC_GROUP_HARDWARE, 
            0,
            0,
            "0");
    }
    else
    {
        // get all the webcams
        for (uint32_t i = 0; i < m_sysInfo->GetNumVideoCaptureDevice(); i++)
        {
            CaptureTelemetryData(METRIC_HW_WEBCAM, kMETRIC_HW_WEBCAM, kMETRIC_GROUP_HARDWARE, 
                m_sysInfo->GetNumVideoCaptureDevice(),
                i + 1,
                m_sysInfo->GetVideoCaptureName(i));
        }
    }
}

//-----------------------------------------------------------------------------
//  HARDWARE SURVEY:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HW_DISPLAY()
{
    //  Iterate through the displays
    for (uint32_t i = 0; i < m_sysInfo->GetNumDisplays(); i++)
    {
        CaptureTelemetryData(METRIC_HW_DISPLAY, kMETRIC_HW_DISPLAY, kMETRIC_GROUP_HARDWARE, 
            m_sysInfo->GetNumDisplays(),
            i + 1,                       // TYPEID_U32 display number, 
            m_sysInfo->GetDisplayResX(i),  // TYPEID_U32 resolution x, 
            m_sysInfo->GetDisplayResY(i));  // TYPEID_U32 resolution x);// TYPEID_S8  client version number
    }
}

//-----------------------------------------------------------------------------
//  HARDWARE SURVEY:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HW_RESOLUTION()
{
    CaptureTelemetryData(METRIC_HW_RESOLUTION, kMETRIC_HW_RESOLUTION, kMETRIC_GROUP_HARDWARE, 
        m_sysInfo->GetVirtualResX(),     // TYPEID_U32 resolution x
        m_sysInfo->GetVirtualResY());     // TYPEID_U32 resolution y

}

//-----------------------------------------------------------------------------
//  HARDWARE SURVEY:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HW_MEM()
{
    CaptureTelemetryData(
        METRIC_HW_MEM, 
        kMETRIC_HW_MEM, 
        kMETRIC_GROUP_HARDWARE, 
        TYPE_U32(m_sysInfo->GetInstalledMemory_MB())); // TYPE_U32(size_MB));
}

//-----------------------------------------------------------------------------
//  HARDWARE SURVEY:  Hard disk drives
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HW_HDD()
{
    for (uint32_t i = 0; i < m_sysInfo->GetNumHDDs(); i++)
    {
        CaptureTelemetryData(METRIC_HW_HDD, kMETRIC_HW_HDD, kMETRIC_GROUP_HARDWARE, 
            m_sysInfo->GetNumHDDs(),
            i + 1,                                 // drive_number, 
            TYPE_U32(m_sysInfo->GetHDDSpace_GB(i)),  // TYPE_U32(size_MB), 
            m_sysInfo->GetHDDInterface(i),           // driveinterface,
            m_sysInfo->GetHDDType(i),                // type,
            m_sysInfo->GetHDDName(i),                // name,
            NonQTOriginServices::Utilities::MD5Hash(m_sysInfo->GetHDDSerialNumber(i)).c_str(),// serial number,
            m_sysInfo->GetUniqueMachineHash());
    }
}

//-----------------------------------------------------------------------------
//  HARDWARE SURVEY:  Optical disk drives
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HW_ODD()
{
    //  Need special case if not ODDS since Mac do not have any built-in
    if (m_sysInfo->GetNumODDs() == 0)
    {
        CaptureTelemetryData(METRIC_HW_ODD, kMETRIC_HW_ODD, kMETRIC_GROUP_HARDWARE, 
            0,
            0,
            "",
            "");
    }
    else
    {
        for (uint32_t i = 0; i < m_sysInfo->GetNumODDs(); i++)
        {
            CaptureTelemetryData(METRIC_HW_ODD, kMETRIC_HW_ODD, kMETRIC_GROUP_HARDWARE, 
                m_sysInfo->GetNumODDs(),
                i + 1,                  // drive_number, 
                m_sysInfo->GetODDType(i), //type,
                m_sysInfo->GetODDName(i)); //name);
        }
    }

}

//-----------------------------------------------------------------------------
//  HARDWARE SURVEY:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HW_MICROPHONE()
{
    CaptureTelemetryData(METRIC_HW_MICROPHONE, kMETRIC_HW_MICROPHONE, kMETRIC_GROUP_HARDWARE, 
        m_sysInfo->GetMicrophone() ? "yes" : "no");
}


///////////////////////////////////////////////////////////////////////////////
//  GAME INSTALLS
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  GAME INSTALLS:  START
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GAME_INSTALL_START(const TYPE_S8 product_id, const TYPE_S8 download_type)
{
    EA::Thread::AutoFutex m2(installSessionList->GetLock());

    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    if ( installSessionList->map.find(Id) == installSessionList->map.end() )
    {
        InstallSession *session = new InstallSession(Id.c_str());
        installSessionList->map.insert( InstallSessionList::SessionPair(Id, session) ); 

        CaptureTelemetryData(METRIC_GAME_INSTALL_START, kMETRIC_GAME_INSTALL_START, kMETRIC_GROUP_INSTALL, 
            session->productId(),
            download_type);
    }
}

//-----------------------------------------------------------------------------
//  GAME INSTALLS:  SUCCESS
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GAME_INSTALL_SUCCESS(const TYPE_S8 product_id, const TYPE_S8 download_type)
{
    EA::Thread::AutoFutex m2(installSessionList->GetLock());

    eastl::string8 Id = (product_id == NULL ? "" : product_id); 
    InstallSessionList::SessionListType::iterator iter=installSessionList->map.find(Id);

    if (iter != installSessionList->map.end())
    {
        CaptureTelemetryData(METRIC_GAME_INSTALL_SUCCESS, kMETRIC_GAME_INSTALL_SUCCESS, kMETRIC_GROUP_INSTALL, 
            Id.c_str(), 
            download_type);
        delete iter->second;
        installSessionList->map.erase(iter);
    }

}

//-----------------------------------------------------------------------------
//  GAME INSTALLS:  ERROR
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GAME_INSTALL_ERROR(const TYPE_S8 product_id, const TYPE_S8 download_type, TYPE_I32 error_type, TYPE_I32 error_code)
{
    EA::Thread::AutoFutex m2(installSessionList->GetLock());

    eastl::string8 Id = (product_id == NULL ? "" : product_id); 
    InstallSessionList::SessionListType::iterator iter=installSessionList->map.find(Id);

    if (iter != installSessionList->map.end())
    {
        CaptureTelemetryData(METRIC_GAME_INSTALL_ERROR, kMETRIC_GAME_INSTALL_ERROR, kMETRIC_GROUP_INSTALL, 
            Id.c_str(), 
            download_type,
            error_type,
            error_code);
        delete iter->second;
        installSessionList->map.erase(iter);
    }

}

void EbisuTelemetryAPI::Metric_GAME_UNINSTALL_CLICKED(const TYPE_S8 product_id)
{
    eastl::string8 Id = (product_id == NULL ? "" : product_id); 

    CaptureTelemetryData(METRIC_GAME_UNINSTALL_CLICKED, kMETRIC_GAME_UNINSTALL_CLICKED, kMETRIC_GROUP_INSTALL, 
        Id.c_str());
}

void EbisuTelemetryAPI::Metric_GAME_UNINSTALL_CANCEL(const TYPE_S8 product_id)
{
    eastl::string8 Id = (product_id == NULL ? "" : product_id); 

    CaptureTelemetryData(METRIC_GAME_UNINSTALL_CANCEL, kMETRIC_GAME_UNINSTALL_CANCEL, kMETRIC_GROUP_INSTALL, 
        Id.c_str());
}

void EbisuTelemetryAPI::Metric_GAME_UNINSTALL_START(const TYPE_S8 product_id)
{
    eastl::string8 Id = (product_id == NULL ? "" : product_id); 

    CaptureTelemetryData(METRIC_GAME_UNINSTALL_START, kMETRIC_GAME_UNINSTALL_START, kMETRIC_GROUP_INSTALL, 
        Id.c_str());
}

void EbisuTelemetryAPI::Metric_GAME_ZERO_LENGTH_UPDATE(const TYPE_S8 product_id)
{
    eastl::string8 Id = (product_id == NULL ? "" : product_id); 

    CaptureTelemetryData(METRIC_GAME_ZERO_LENGTH_UPDATE, kMETRIC_GAME_ZERO_LENGTH_UPDATE, kMETRIC_GROUP_INSTALL, 
        Id.c_str());
}

///////////////////////////////////////////////////////////////////////////////
//  ITO:
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  ITO:
//  running:  True if eadm is running on machine
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ITE_CLIENT_RUNNING(bool running)
{
    CaptureTelemetryData(METRIC_ITE_CLIENT_RUNNING, kMETRIC_ITE_CLIENT_RUNNING, kMETRIC_GROUP_INSTALL, 
        running ? 1 : 0);
}

//-----------------------------------------------------------------------------
//  ITO:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ITE_CLIENT_SUCCESS()
{
    CaptureTelemetryData(METRIC_ITE_CLIENT_SUCCESS, kMETRIC_ITE_CLIENT_SUCCESS, kMETRIC_GROUP_INSTALL);
}

//-----------------------------------------------------------------------------
//  ITO:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ITE_CLIENT_FAILED(const TYPE_S8 reason)
{
    CaptureTelemetryData(METRIC_ITE_CLIENT_FAILED, kMETRIC_ITE_CLIENT_FAILED, kMETRIC_GROUP_INSTALL, 
        reason);
}

//-----------------------------------------------------------------------------
//  ITO:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ITE_CLIENT_CANCELLED(const TYPE_S8 reason)
{
    CaptureTelemetryData(METRIC_ITE_CLIENT_CANCELLED, kMETRIC_ITE_CLIENT_CANCELLED, kMETRIC_GROUP_INSTALL, 
        reason);
}

///////////////////////////////////////////////////////////////////////////////
//  GAME LAUNCH:
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  LAUNCH CANCELLED:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GAME_LAUNCH_CANCELLED(const TYPE_S8 product_id, TYPE_S8 reason)
{
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    CaptureTelemetryData(METRIC_GAME_LAUNCH_CANCELLED, kMETRIC_GAME_LAUNCH_CANCELLED, kMETRIC_GROUP_GAME_SESSION, 
        product_id,
        reason);
}


///////////////////////////////////////////////////////////////////////////////
//  GAME SESSION:
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  GAME SESSION: START
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GAME_SESSION_START(const TYPE_S8 product_id, const TYPE_S8 launch_type, const TYPE_S8 children_content, 
                                                                 const TYPE_S8 game_version, bool nonOrigin, const TYPE_S8 last_played, const TYPE_S8 sub_first_start_date, 
                                                                 const TYPE_S8 sub_instance_start_date, bool is_subscription_entitlement)
{
    EA::Thread::AutoFutex m2(gameSessionList->GetLock());

    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    //  Delete old session info
    GameSessionList::SessionListType::iterator iter = gameSessionList->map.find(Id);
    if (iter != gameSessionList->map.end())
    {
        delete iter->second;
    }

    //  Create new session info
    GameSession *session = new GameSession(Id.c_str());
    gameSessionList->map.insert( GameSessionList::SessionPair(Id, session) ); 
    eastl::string8 launcher_src = "";
    
    //Game was launched through softwareID configuration (ie Sparta was launched from a sparta-enabled entitlement)
    if (m_alt_launcher_info != NULL)
    {
        if (strcmp(m_alt_launcher_info->second.c_str(), product_id)==0)
        {
            launcher_src = m_alt_launcher_info->first;
            mGameLaunchSource = Launch_SoftwareID;
        }
        else
        {
            //The product id from the stashed information didn't match, which means a previous launch got initiated
            //but the GAME SESS STRT never fired for that launch. So this is an error!
            this->Metric_ALTLAUNCHER_SESSION_FAIL(m_alt_launcher_info->first.c_str(), "software_id", "Game Never Launched", m_alt_launcher_info->second.c_str());
        }
        //If we stashed alt launcher information, clear it.
        delete m_alt_launcher_info;
        m_alt_launcher_info = NULL;
    }
    //Game was launched through SDK OriginStartGame call
    if (m_sdk_launching_info != NULL)
    {
        if (strcmp(m_sdk_launching_info->second.c_str(), product_id)==0)
        {
            launcher_src = m_sdk_launching_info->first;
            mGameLaunchSource = Launch_SDK;
        }
        else
        {
            //The product id from the stashed information didn't match, which means a previous launch got initiated
            //but the GAME SESS STRT never fired for that launch. So this is an error!
            this->Metric_GAME_ALTLAUNCHER_FAIL(m_sdk_launching_info->first.c_str(), m_sdk_launching_info->second.c_str(), "Game Never Launched");
        }
        //If we stashed SDK launcher information, clear it
        delete m_sdk_launching_info;
        m_sdk_launching_info = NULL;
    }

    CaptureTelemetryData(METRIC_GAME_SESSION_START, kMETRIC_GAME_SESSION_START, kMETRIC_GROUP_GAME_SESSION,
        session->sessionId(),
        launcher_src.c_str(),
        session->productId(),
        nonOrigin,
        launch_type,
        LaunchStrings[mGameLaunchSource],
        children_content,
        game_version,
        TelemetryUnderageSettingStrings[isTelemetryUserUnderaged()],
        last_played,
        sub_first_start_date,
        sub_instance_start_date,
        is_subscription_entitlement ? 1 : 0);
    mGameLaunchSource = Launch_Unknown;
}

//-----------------------------------------------------------------------------
//  GAME SESSION: UNMONITORED
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GAME_SESSION_UNMONITORED(const TYPE_S8 product_id, const TYPE_S8 launch_type, const TYPE_S8 children_content, const TYPE_S8 game_version)
{
    //  Send event
    CaptureTelemetryData(METRIC_GAME_SESSION_UNMONITORED, kMETRIC_GAME_SESSION_UNMONITORED, kMETRIC_GROUP_GAME_SESSION,
        product_id,
        launch_type,
        LaunchStrings[mGameLaunchSource],
        children_content,
        game_version
        );
}

//-----------------------------------------------------------------------------
//  GAME SESSION: END
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GAME_SESSION_END(const TYPE_S8 product_id, TYPE_I32 error_code, bool nonOrigin, const TYPE_S8 last_played, const TYPE_S8 sub_first_start_date, const TYPE_S8 sub_instance_start_date, bool is_subscription_entitlement)
{
    EA::Thread::AutoFutex m2(gameSessionList->GetLock());

    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    GameSessionList::SessionListType::iterator iter = gameSessionList->map.find(Id);

    if ( iter != gameSessionList->map.end() )
    {
        CaptureTelemetryData(METRIC_GAME_SESSION_END, kMETRIC_GAME_SESSION_END, kMETRIC_GROUP_GAME_SESSION, 
            iter->second->sessionId(), 
            iter->second->sessionLength(), 
            iter->second->productId(), 
            nonOrigin,
            error_code,
            TelemetryUnderageSettingStrings[isTelemetryUserUnderaged()],
            last_played,
            sub_first_start_date,
            sub_instance_start_date,
            is_subscription_entitlement ? 1 : 0);

        delete iter->second;

        gameSessionList->map.erase( iter );
    }
}


void EbisuTelemetryAPI::Metric_ALTLAUNCHER_SESSION_FAIL(const TYPE_S8 product_id, const TYPE_S8 launch_type, const TYPE_S8 reas, const TYPE_S8 valu)
{
    CaptureTelemetryData(METRIC_ALTLAUNCHER_SESSION_FAIL, kMETRIC_ALTLAUNCHER_SESSION_FAIL, kMETRIC_GROUP_GAME_SESSION,
        product_id,
        launch_type,
        reas,
        valu
        );
}

void EbisuTelemetryAPI::Metric_GAME_ALTLAUNCHER_FAIL(const TYPE_S8 launcher_product_id, const TYPE_S8 target_product_id, const TYPE_S8 game_state)
{
    CaptureTelemetryData(METRIC_GAME_SESSION_ALTLAUNCHER_FAIL, kMETRIC_GAME_SESSION_ALTLAUNCHER_FAIL, kMETRIC_GROUP_GAME_SESSION,
        launcher_product_id,
        target_product_id,
        game_state
        );
}

///////////////////////////////////////////////////////////////////////////////
//  DOWNLOAD:
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  DOWNLOAD:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_START(const TYPE_S8 environment, const TYPE_S8 product_id, const TYPE_S8 DL_start_status, const TYPE_S8 content_type, const TYPE_S8 package_type, const TYPE_S8 url,
                                                       TYPE_U64 bytes_downloaded, const TYPE_S8 auto_patch, const TYPE_S8 target_path, bool symlink_detected, bool isNonDipUpgrade, bool isPreload)
{
    EA::Thread::AutoFutex m2(downloadSessionList->GetLock());

    //  Filter out email address in content id
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    DownloadSession *session = NULL;

    // If we haven't tracked this content yet, create a new entry in the cache
    DownloadSessionList::SessionListType::iterator iter = downloadSessionList->map.find(Id);
    if ( iter == downloadSessionList->map.end() )
    {
        session = new DownloadSession(Id.c_str(), DL_start_status);
        downloadSessionList->map.insert( DownloadSessionList::SessionPair(Id, session) ); 
    }
    else
    {
        // Use the cached entry
        session = iter->second;
    }

    // Set or update all the fields
    if (strcmp(environment, "production"))  // Only set environment if not "production"
        session->setEnvironment(environment);
    session->setContentType(content_type);
    session->setPackageType(package_type);
    session->setUrl(url);
    session->setBytesDownloaded(bytes_downloaded);
    session->setBitrateKbps(0);
    session->setAutoPatch(auto_patch);
    session->setSymlink(symlink_detected);
    session->setNonDipUpgrade(isNonDipUpgrade);
        

    CaptureTelemetryData(METRIC_DL_START, kMETRIC_DL_START, kMETRIC_GROUP_DOWNLOAD, 
        session->environment(),
        session->startTimeUTC(), 
        session->startStatus(),
        package_type, 
        session->productId(), 
        content_type, 
        url, 
        session->autoPatch(),
        (session->nonDipUpgrade() ? 1 : 0),
        target_path,
        (isPreload ? 1 : 0));


}

//-----------------------------------------------------------------------------
//  DOWNLOAD:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_IMMEDIATE_ERROR(const TYPE_S8 product_id, const TYPE_S8 error_string1, const TYPE_S8 error_string2, const TYPE_S8 source_file, TYPE_U32 source_line)
{
    // Strip extra folder info from the source file
    eastl::string8 local_source_file = (source_file == NULL ? "" : source_file);
    StripFolderInfo(local_source_file);

    CaptureTelemetryData(METRIC_DL_IMMEDIATE_ERROR, kMETRIC_DL_IMMEDIATE_ERROR, kMETRIC_GROUP_DOWNLOAD, 
        product_id, 
        error_string1,
        error_string2,
        local_source_file.c_str(),
        source_line);
}

//-----------------------------------------------------------------------------
//  DOWNLOAD: Data error
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_DATA_ERROR(const TYPE_S8 product_id, const TYPE_S8 error_category, const TYPE_S8 error_details, const TYPE_S8 source_file, TYPE_U32 source_line)
{
    // Strip extra folder info from the source file
    eastl::string8 local_source_file = (source_file == NULL ? "" : source_file);
    StripFolderInfo(local_source_file);

    CaptureTelemetryData(METRIC_DL_IMMEDIATE_ERROR, kMETRIC_DL_IMMEDIATE_ERROR, kMETRIC_GROUP_DOWNLOAD, 
        product_id, 
        error_category,
        error_details,
        local_source_file.c_str(),
        source_line);
}

//-----------------------------------------------------------------------------
//  DOWNLOAD:  Error send when error code is populated in downloader
//  Called before download start is fired.
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_POPULATE_ERROR(const TYPE_S8 product_id, const TYPE_S8 error_type, const TYPE_I32 error_code, const TYPE_S8 source_file, TYPE_U32 source_line)
{
    // Strip extra folder info from the source file
    eastl::string8 local_source_file = (source_file == NULL ? "" : source_file);
    StripFolderInfo(local_source_file);

    CaptureTelemetryData(METRIC_DL_POPULATE_ERROR, kMETRIC_DL_POPULATE_ERROR, kMETRIC_GROUP_DOWNLOAD, 
        product_id, 
        error_type,
        error_code,
        local_source_file.c_str(),
        source_line);
}

//-----------------------------------------------------------------------------
//  DOWNLOAD:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_ERROR(const TYPE_S8 product_id, const TYPE_S8 dler_error_string, const TYPE_S8 dler_error_context, TYPE_I32 dler_error_code, TYPE_I32 sys_error_code, const TYPE_S8 source_file, TYPE_U32 source_line, bool isPreload)
{
    EA::Thread::AutoFutex m2(downloadSessionList->GetLock());

    //  Filter out email address in content id
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    // Strip extra folder info from the source file
    eastl::string8 local_source_file = (source_file == NULL ? "" : source_file);
    StripFolderInfo(local_source_file);

    DownloadSessionList::SessionListType::iterator iter=downloadSessionList->map.find(Id);
    if ( iter != downloadSessionList->map.end() )
    {
        CaptureTelemetryData(METRIC_DL_ERROR, kMETRIC_DL_ERROR, kMETRIC_GROUP_DOWNLOAD,
            iter->second->packageType(),
            iter->second->productId(),
            iter->second->contentType(),
            iter->second->url(),
            dler_error_context,
            dler_error_code,
            sys_error_code,
            local_source_file.c_str(),
            source_line,
            iter->second->autoPatch(),
            (iter->second->nonDipUpgrade() ? 1 : 0),
            iter->second->symlink(),
            (isPreload ? 1 : 0));
    }
    // if a download related failure happened before the download started
    // we still want to fire the hook although the the download session is
    // not in the download session list
    else if (dler_error_code == 131077 /* EscalationFailureAdminRequired */)
        CaptureTelemetryData(METRIC_DL_ERROR, kMETRIC_DL_ERROR, kMETRIC_GROUP_DOWNLOAD, 
            NULL, 
            NULL, 
            NULL, 
            NULL, 
            dler_error_context,
            dler_error_code, 
            sys_error_code, 
            "", 
            0, 
            NULL, 
            0, 
            0,
            false);


}

//-----------------------------------------------------------------------------
//  DOWNLOAD:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_ERROR_PREALLOCATE(const TYPE_S8 product_id, const TYPE_S8 error_type, const TYPE_S8 error_path, TYPE_U32 error_code, TYPE_U32 file_size)
{
    EA::Thread::AutoFutex m2(downloadSessionList->GetLock());

    //  Filter out email address in content id
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    DownloadSessionList::SessionListType::iterator iter=downloadSessionList->map.find(Id);
    if ( iter != downloadSessionList->map.end() )
    {
        CaptureTelemetryData(METRIC_DL_ERROR_PREALLOCATE, kMETRIC_DL_ERROR_PREALLOCATE, kMETRIC_GROUP_DOWNLOAD, 
            iter->second->environment(),
            iter->second->productId(), 
            iter->second->packageType(), 
            error_type, // "file" or "dir"
            error_path,
            error_code, 
            file_size,
            (iter->second->nonDipUpgrade() ? 1 : 0));
    }
}

//-----------------------------------------------------------------------------
//  DOWNLOAD:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_ERRORBOX(const TYPE_S8 product_id, const TYPE_I32 error_code, TYPE_I32 error_type)
{
    EA::Thread::AutoFutex m2(downloadSessionList->GetLock());

    CaptureTelemetryData(METRIC_DL_ERRORBOX, kMETRIC_DL_ERRORBOX, kMETRIC_GROUP_DOWNLOAD, 
        product_id,
        error_code,
        error_type);
}

//-----------------------------------------------------------------------------
//  DOWNLOAD: Report CDN vendor IP address when a download error occurs
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_ERROR_VENDOR_IP(const TYPE_S8 product_id, const TYPE_S8 vendor_ip, TYPE_I32 dler_error_code, TYPE_I32 sys_error_code)
{
    EA::Thread::AutoFutex m2(downloadSessionList->GetLock());

    //  Filter out email address in content id
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    // Get the DNS IPs
    eastl::string8 dnsIpList = "";
#if defined(ORIGIN_PC)
    FIXED_INFO *pFixedInfo;
    ULONG ulOutBufLen;
    DWORD dwRetVal;
    IP_ADDR_STRING *pIPAddr;

    pFixedInfo = new(FIXED_INFO);
    ulOutBufLen = sizeof (FIXED_INFO);

    dwRetVal = GetNetworkParams(pFixedInfo, &ulOutBufLen);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW)
    {
        delete pFixedInfo;
        pFixedInfo = (FIXED_INFO *) malloc(ulOutBufLen);
        dwRetVal = GetNetworkParams(pFixedInfo, &ulOutBufLen);
    }

    if (dwRetVal == NO_ERROR)
    {
        dnsIpList = pFixedInfo->DnsServerList.IpAddress.String;
        pIPAddr = pFixedInfo->DnsServerList.Next;
        while (pIPAddr)
        {
            dnsIpList += ", ";
            dnsIpList += pIPAddr->IpAddress.String;
            pIPAddr = pIPAddr->Next;
        }
    }

#elif defined(ORIGIN_MAC)
    char str[30];
    char *c;
    bool in;

    FILE *f = fopen("/etc/resolv.conf", "r");
    if(f)
    {
        while (fgets(str, 30, f) != NULL)
        {
            // filter the lines that have 'namespace' string
            if((c = strstr(str, "nameserver")) != NULL)
            {
                // the line needs to begin with 'nameserver'
                if(c != str)
                    continue;
                in = false;
                // extract the IP address
                c += strlen("nameserver");
                while((c - str) < 30)
                {
                    // IPv4 address contains dots and decimal numbers
                    // IPv6 address contains colons and HEX digits
                    if(((*c == '.') && (in)) || (isdigit(*c)) ||
                        ((*c == ':') && (in)) || ((*c >= 'A') && (*c <= 'F')) || ((*c >= 'a') && (*c <= 'f')))
                    {
                        in = true;
                        dnsIpList += *c;
                    }
                    else if (in)
                        break;
                    c++;
                }
                dnsIpList += ", ";
            }
        }

        // remove the trailing ', '
        if(dnsIpList.length() > 2)
            dnsIpList.resize(dnsIpList.length()-2);
        fclose(f);
    }

#endif

    DownloadSessionList::SessionListType::iterator iter=downloadSessionList->map.find(Id);
    if ( iter != downloadSessionList->map.end() )
    {
        CaptureTelemetryData(METRIC_DL_ERROR_VENDOR_IP, kMETRIC_DL_ERROR_VENDOR_IP, kMETRIC_GROUP_DOWNLOAD, 
            iter->second->productId(), 
            iter->second->url(),
            vendor_ip,
            dler_error_code,
            sys_error_code);
    }


}

//-----------------------------------------------------------------------------
//  DOWNLOAD:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_REDOWNLOAD(const TYPE_S8 product_id, const TYPE_S8 filename, const TYPE_S8 redl_error_code, 
                                                            TYPE_I32 sys_error_code, TYPE_U32 extra1, TYPE_U32 extra2)
{
    //  Filter out email address in content id
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    DownloadSessionList::SessionListType::iterator iter=downloadSessionList->map.find(Id);
    if ( iter != downloadSessionList->map.end() )
    {
        CaptureTelemetryData(METRIC_DL_REDOWNLOAD, kMETRIC_DL_REDOWNLOAD, kMETRIC_GROUP_DOWNLOAD, 
            iter->second->environment(),
            Id.c_str(), 
            filename, 
            redl_error_code, 
            sys_error_code,
            extra1,
            extra2,
            iter->second->packageType(), 
            (iter->second->nonDipUpgrade() ? 1 : 0));
    }

}

//-----------------------------------------------------------------------------
//  DOWNLOAD:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_CRC_FAILURE(const TYPE_S8 product_id, const TYPE_S8 filename, TYPE_U32 unpack_type, TYPE_U32 file_crc, TYPE_U32 calculated_crc, TYPE_U32 file_date, TYPE_U32 file_time)
{
    EA::Thread::AutoFutex m2(downloadSessionList->GetLock());

    //  Filter out email address in content id
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    DownloadSessionList::SessionListType::iterator iter=downloadSessionList->map.find(Id);
    if( iter != downloadSessionList->map.end() )
    {
        CaptureTelemetryData(METRIC_DL_CRC_FAILURE, kMETRIC_DL_CRC_FAILURE, kMETRIC_GROUP_DOWNLOAD, 
            iter->second->environment(),
            iter->second->productId(), 
            iter->second->packageType(), 
            filename,
            unpack_type,
            file_crc,
            calculated_crc, 
            file_date,
            file_time,
            (iter->second->nonDipUpgrade() ? 1 : 0));
    }
}

//-----------------------------------------------------------------------------
//  DOWNLOAD:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_CANCEL(const TYPE_S8 product_id, bool isPreload)
{
    EA::Thread::AutoFutex m2(downloadSessionList->GetLock());

    //  Filter out email address in content id
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    DownloadSessionList::SessionListType::iterator iter=downloadSessionList->map.find(Id);
    if ( iter != downloadSessionList->map.end() )
    {
        CaptureTelemetryData(METRIC_DL_CANCEL, kMETRIC_DL_CANCEL, kMETRIC_GROUP_DOWNLOAD, 
            iter->second->environment(),
            iter->second->startTimeUTC(), 
            iter->second->startStatus(), 
            iter->second->startTimeUTC() + iter->second->sessionLength(), 
            "cancel", 
            iter->second->packageType(),
            iter->second->bytesDownloaded(), 
            iter->second->bitrateKbps(),  
            iter->second->productId(), 
            iter->second->contentType(), 
            iter->second->url(), 
            iter->second->autoPatch(),
            (iter->second->nonDipUpgrade() ? 1 : 0),
            (isPreload ? 1 : 0));
    }
}

//-----------------------------------------------------------------------------
//  DOWNLOAD:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_SUCCESS(const TYPE_S8 product_id, bool isPreload)
{
    EA::Thread::AutoFutex m2(downloadSessionList->GetLock());

    //  Filter out email address in content id
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    DownloadSessionList::SessionListType::iterator iter=downloadSessionList->map.find(Id);
    if ( iter != downloadSessionList->map.end() )
    {
        CaptureTelemetryData(METRIC_DL_SUCCESS, kMETRIC_DL_SUCCESS, kMETRIC_GROUP_DOWNLOAD, 
            iter->second->environment(),
            iter->second->startTimeUTC(), 
            iter->second->startStatus(), 
            iter->second->startTimeUTC() + iter->second->sessionLength(), 
            "success", 
            iter->second->packageType(),
            iter->second->bytesDownloaded(), 
            iter->second->bitrateKbps(), 
            iter->second->productId(), 
            iter->second->contentType(), 
            iter->second->url(), 
            iter->second->autoPatch(),
            (iter->second->nonDipUpgrade() ? 1 : 0),
            (isPreload ? 1 : 0));
    }
}

//-----------------------------------------------------------------------------
//  DOWNLOAD:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_PAUSE(const TYPE_S8 product_id, const TYPE_S8 reason)
{
    EA::Thread::AutoFutex m2(downloadSessionList->GetLock());

    //  Filter out email address in content id
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    DownloadSessionList::SessionListType::iterator iter=downloadSessionList->map.find(Id);
    if ( iter != downloadSessionList->map.end() )
    {
        CaptureTelemetryData(METRIC_DL_PAUSE, kMETRIC_DL_PAUSE, kMETRIC_GROUP_DOWNLOAD, 
            iter->second->environment(),
            iter->second->productId(),
            reason,
            iter->second->contentType(),
            iter->second->packageType(),
            iter->second->bytesDownloaded(),
            iter->second->autoPatch(),
            (iter->second->nonDipUpgrade() ? 1 : 0));
    }
}


//-----------------------------------------------------------------------------
//  DOWNLOAD:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_CONTENT_LENGTH(const TYPE_S8 product_id, const TYPE_S8 link, const TYPE_S8 type, const TYPE_S8 result, const TYPE_S8 startEndbyte, const TYPE_S8 totalbytes, const TYPE_S8 vendor_ip)
{
    //  Filter out email address in content id
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    DownloadSessionList::SessionListType::iterator iter=downloadSessionList->map.find(Id);
    if ( iter != downloadSessionList->map.end() )
    {
        // truncate download URL to reduce payload size in other hooks
        eastl::string8 truncatedLink(link);
        int truncPos = truncatedLink.find('?');

        if (truncPos >= 0)
        {
            truncatedLink = truncatedLink.left(truncPos);
        }

        //update with the latest updated URL from JIT Call
        iter->second->setUrl(truncatedLink.c_str());

        // note that for this hook we report the full download URL
        CaptureTelemetryData(METRIC_DL_CONTENT_LENGTH, kMETRIC_DL_CONTENT_LENGTH, kMETRIC_GROUP_DOWNLOAD, 
            iter->second->environment(),
            product_id, 
            link,
            type,
            result,
            startEndbyte,
            totalbytes,
            vendor_ip);
    }
}

//-----------------------------------------------------------------------------
//  DOWNLOAD:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_OPTI_DATA(const TYPE_S8 product_id, const TYPE_S8 final_dl_rate, const TYPE_S8 avg_ip_dl_rate, const TYPE_S8 worker_saturation, const TYPE_S8 channel_saturation, const TYPE_U32 avg_chunk_bytes, const TYPE_S8 host, const TYPE_I16 num_ips_used, const TYPE_I16 num_ip_errors, const TYPE_U32 num_request_in_clusters, const TYPE_U32 num_single_file_chunks)
{
    //  Filter out email address in content id
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    DownloadSessionList::SessionListType::iterator iter=downloadSessionList->map.find(Id);
    if ( iter != downloadSessionList->map.end() )
    {
        // note that for this hook we report the full download URL
        CaptureTelemetryData(METRIC_DL_OPTI_DATA, kMETRIC_DL_OPTI_DATA, kMETRIC_GROUP_DOWNLOAD, 
            product_id, 
            final_dl_rate, 
            avg_ip_dl_rate, 
            worker_saturation,
            channel_saturation,
            avg_chunk_bytes,
            host,
            num_ips_used,
            num_ip_errors,
            num_request_in_clusters,
            num_single_file_chunks
            );
    }
}

//-----------------------------------------------------------------------------
//  DOWNLOAD:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_PROXY_DETECTED()
{
    CaptureTelemetryData(METRIC_DL_PROXY_DETECTED, kMETRIC_DL_PROXY_DETECTED, kMETRIC_GROUP_DOWNLOAD );
}

//-----------------------------------------------------------------------------
//  DOWNLOAD:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_CONNECTION_STATS(const TYPE_S8 product_id, const TYPE_S8 uuid, const TYPE_S8 dest_ip, const TYPE_S8 url, TYPE_U64 bytes_downloaded, TYPE_U64 time_elapsed_ms, TYPE_U64 times_used, TYPE_U16 error_count)
{
    EA::Thread::AutoFutex m2(downloadSessionList->GetLock());

    eastl::string8 Id = (product_id == NULL ? "" : product_id);
    DownloadSessionList::SessionListType::iterator iter=downloadSessionList->map.find(Id);
    if ( iter != downloadSessionList->map.end() )
    {
        CaptureTelemetryData(METRIC_DL_CONNECTION_STATS, kMETRIC_DL_CONNECTION_STATS, kMETRIC_GROUP_DOWNLOAD
            , iter->second->environment()
            , iter->second->productId()
            , uuid
            , dest_ip
            , url
            , bytes_downloaded
            , time_elapsed_ms
            , times_used
            , error_count);
    }
}

//-----------------------------------------------------------------------------
//  DOWNLOAD: DiP 3 Prepare Patching
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_DIP3_PREPARE(const TYPE_S8 product_id, const TYPE_U32 filesToPatch)
{
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    DownloadSessionList::SessionListType::iterator iter=downloadSessionList->map.find(Id);
    if ( iter != downloadSessionList->map.end() )
    {
        CaptureTelemetryData(METRIC_DL_DIP3_PREPARE, kMETRIC_DL_DIP3_PREPARE, kMETRIC_GROUP_DOWNLOAD, 
            iter->second->environment(),
            product_id, 
            filesToPatch);
    }

}

//-----------------------------------------------------------------------------
//  DOWNLOAD: DiP 3 Patch Summary
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_DIP3_SUMMARY(const TYPE_S8 product_id, const TYPE_U32 totalFiles, const TYPE_U32 filesToPatch, const TYPE_U32 filesRejected, const TYPE_U64 addedDataSz, const TYPE_U64 origUpdateSz, const TYPE_U64 dip3TotPatchSz, const TYPE_U64 dip3NonPatchSz, const TYPE_U64 dip3PatchedSz, const TYPE_U64 dip3PatchSavingsSz, const TYPE_U64 diffCalcBitrate)
{
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    DownloadSessionList::SessionListType::iterator iter=downloadSessionList->map.find(Id);
    if ( iter != downloadSessionList->map.end() )
    {
        CaptureTelemetryData(METRIC_DL_DIP3_SUMMARY, kMETRIC_DL_DIP3_SUMMARY, kMETRIC_GROUP_DOWNLOAD, 
            iter->second->environment(),
            product_id, 
            totalFiles,
            filesToPatch,
            filesRejected,
            addedDataSz,
            origUpdateSz,
            dip3TotPatchSz,
            dip3NonPatchSz,
            dip3PatchedSz,
            dip3PatchSavingsSz,
            diffCalcBitrate,
            (iter->second->nonDipUpgrade() ? 1 : 0));
    }

}

//-----------------------------------------------------------------------------
//  DOWNLOAD: DiP 3 Patching Canceled
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_DIP3_CANCELED(const TYPE_S8 product_id, const TYPE_U32 reason_type, const TYPE_U32 reason_code)
{
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    DownloadSessionList::SessionListType::iterator iter=downloadSessionList->map.find(Id);
    if ( iter != downloadSessionList->map.end() )
    {
        CaptureTelemetryData(METRIC_DL_DIP3_CANCELED, kMETRIC_DL_DIP3_CANCELED, kMETRIC_GROUP_DOWNLOAD, 
            iter->second->environment(),
            product_id, 
            reason_type,
            reason_code,
            (iter->second->nonDipUpgrade() ? 1 : 0));
    }

}

//-----------------------------------------------------------------------------
//  DOWNLOAD: DiP 3 Patch Error
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_DIP3_ERROR(const TYPE_S8 product_id, const TYPE_U32 error_type, const TYPE_U32 error_code, const TYPE_S8 error_context)
{
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    DownloadSessionList::SessionListType::iterator iter=downloadSessionList->map.find(Id);
    if ( iter != downloadSessionList->map.end() )
    {
        CaptureTelemetryData(METRIC_DL_DIP3_ERROR, kMETRIC_DL_DIP3_ERROR, kMETRIC_GROUP_DOWNLOAD, 
            iter->second->environment(),
            product_id, 
            error_type,
            error_code,
            error_context,
            (iter->second->nonDipUpgrade() ? 1 : 0));
    }
}

//-----------------------------------------------------------------------------
//  DOWNLOAD: DiP 3 Patch Success
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_DIP3_SUCCESS(const TYPE_S8 product_id)
{
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    DownloadSessionList::SessionListType::iterator iter=downloadSessionList->map.find(Id);
    if ( iter != downloadSessionList->map.end() )
    {
        CaptureTelemetryData(METRIC_DL_DIP3_SUCCESS, kMETRIC_DL_DIP3_SUCCESS, kMETRIC_GROUP_DOWNLOAD, 
            iter->second->environment(),
            product_id,
            (iter->second->nonDipUpgrade() ? 1 : 0));
    }
}

//-----------------------------------------------------------------------------
//  DOWNLOAD: elapsed time from download start to ready to play
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DL_ELAPSEDTIME_TO_READYTOPLAY(const TYPE_S8 product_id, TYPE_U64 timeElapsed, bool isITO, bool isLocalSource, bool isUpdate, bool isRepair)
{
    eastl::string8 Id = (product_id == NULL ? "" : product_id);
    CaptureTelemetryData(METRIC_DL_ELAPSEDTIME_TO_READYTOPLAY, kMETRIC_DL_ELAPSEDTIME_TO_READYTOPLAY, kMETRIC_GROUP_DOWNLOAD, 
        product_id,
        timeElapsed,
        isITO,
        isLocalSource,
        isUpdate,
        isRepair);
}

//-----------------------------------------------------------------------------
//  DYNAMIC DOWNLOAD: Game launched
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DYNAMICDOWNLOAD_GAMELAUNCH(const TYPE_S8 product_id, const TYPE_S8 uuid, const TYPE_S8 launchSource, const TYPE_U64 bytesDownloaded, const TYPE_U64 bytesRequired, const TYPE_U64 bytesTotal)
{
    CaptureTelemetryData(METRIC_DYNAMICDOWNLOAD_GAMELAUNCH, kMETRIC_DYNAMICDOWNLOAD_GAMELAUNCH, kMETRIC_GROUP_DYNAMICDOWNLOAD, 
        product_id,
        uuid,
        launchSource,
        bytesDownloaded,
        bytesRequired,
        bytesTotal);
}

///////////////////////////////////////////////////////////////////////////////
//  PATCH
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  PATCH:  Download skipped
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_AUTOPATCH_DOWNLOAD_SKIP(const TYPE_S8 product_id)
{
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    CaptureTelemetryData(METRIC_AUTOPATCH_DOWNLOAD_SKIP, kMETRIC_AUTOPATCH_DOWNLOAD_SKIP, kMETRIC_GROUP_AUTOPATCH, 
        Id.c_str());
}

//-----------------------------------------------------------------------------
//  PATCH:  Client detected that server version does not match the currently 
//  installed version therefore trigger the automatic patch.
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_AUTOPATCH_DOWNLOAD_START(const TYPE_S8 product_id, const TYPE_S8 version_installed, const TYPE_S8 version_server)
{
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    CaptureTelemetryData(METRIC_AUTOPATCH_DOWNLOAD_START, kMETRIC_AUTOPATCH_DOWNLOAD_START, kMETRIC_GROUP_AUTOPATCH, 
        Id.c_str(),
        version_installed,
        version_server);
}

//-----------------------------------------------------------------------------
//  PATCH:  User triggered a manual update of content.  This hook will fire
//  regardless if the content is out of date or not.
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_MANUALPATCH_DOWNLOAD_START(const TYPE_S8 product_id, const TYPE_S8 version_installed, const TYPE_S8 version_server)
{
    eastl::string8 Id = (product_id == NULL ? "" : product_id);

    CaptureTelemetryData(METRIC_MANUALPATCH_DOWNLOAD_START, kMETRIC_MANUALPATCH_DOWNLOAD_START, kMETRIC_GROUP_AUTOPATCH, 
        Id.c_str(),
        version_installed,
        version_server);
}

//-----------------------------------------------------------------------------
//  SELF PATCH:  configuration version and DIP version don't match
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_AUTOPATCH_CONFIGVERSION_DIPVERSION_DONTMATCH(const TYPE_S8 entitl, const TYPE_S8 installedVersion, const TYPE_S8 serverVersion)
{
    CaptureTelemetryData(METRIC_AUTOPATCH_CONFIGVERSION_DIPVERSION_DONTMATCH, kMETRIC_AUTOPATCH_CONFIGVERSION_DIPVERSION_DONTMATCH, kMETRIC_GROUP_AUTOPATCH, 
        entitl,
        installedVersion,
        serverVersion);
}

///////////////////////////////////////////////////////////////////////////////
//  AUTOREPAIR
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  AUTOREPAIR:  User accepted task
//-----------------------------------------------------------------------------

void EbisuTelemetryAPI::Metric_AUTOREPAIR_TASK_ACCEPTED()
{
    CaptureTelemetryData(METRIC_AUTOREPAIR_TASK_ACCEPTED, kMETRIC_AUTOREPAIR_TASK_ACCEPTED, kMETRIC_GROUP_AUTOREPAIR);
}

//-----------------------------------------------------------------------------
//  AUTOREPAIR:  User declined task
//-----------------------------------------------------------------------------

void EbisuTelemetryAPI::Metric_AUTOREPAIR_TASK_DECLINED()
{
    CaptureTelemetryData(METRIC_AUTOREPAIR_TASK_DECLINED, kMETRIC_AUTOREPAIR_TASK_DECLINED, kMETRIC_GROUP_AUTOREPAIR);
}

//-----------------------------------------------------------------------------
//  AUTOREPAIR:  Content needs repair
//-----------------------------------------------------------------------------

void EbisuTelemetryAPI::Metric_AUTOREPAIR_NEEDS_REPAIR(const TYPE_S8 product_id, const TYPE_S8 version_installed, const TYPE_S8 version_server)
{
    CaptureTelemetryData(METRIC_AUTOREPAIR_NEEDS_REPAIR, kMETRIC_AUTOREPAIR_NEEDS_REPAIR, kMETRIC_GROUP_AUTOREPAIR, 
        product_id,
        version_installed,
        version_server);
}

//-----------------------------------------------------------------------------
//  AUTOREPAIR:  Repair started
//-----------------------------------------------------------------------------

void EbisuTelemetryAPI::Metric_AUTOREPAIR_DOWNLOAD_START(const TYPE_S8 product_id, const TYPE_S8 version_installed, const TYPE_S8 version_server)
{
    CaptureTelemetryData(METRIC_AUTOREPAIR_DOWNLOAD_START, kMETRIC_AUTOREPAIR_DOWNLOAD_START, kMETRIC_GROUP_AUTOREPAIR, 
        product_id,
        version_installed,
        version_server);
}

///////////////////////////////////////////////////////////////////////////////
//  ERROR REPORTING
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  ERROR REPORTING:  General failure point notification
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ERROR_NOTIFY(const TYPE_S8 error_reason, const TYPE_S8 error_code)
{
    CaptureTelemetryData(METRIC_ERROR_NOTIFY, kMETRIC_ERROR_NOTIFY, kMETRIC_GROUP_ERROR, 
        error_reason, 
        error_code);
}

//-----------------------------------------------------------------------------
//  ERROR REPORTING:  Error when trying to delete file
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ERROR_DELETE_FILE(const TYPE_S8 context, const TYPE_S8 path, const TYPE_I32 error_code)
{
    CaptureTelemetryData(METRIC_ERROR_DELETE_FILE, kMETRIC_ERROR_DELETE_FILE, kMETRIC_GROUP_ERROR, 
        context, 
        path,
        error_code);
}

//-----------------------------------------------------------------------------
//  ERROR REPORTING:  REST error
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ERROR_REST(const TYPE_I32 restError, const TYPE_I32 qnetworkReply, const TYPE_I32 httpStatus, const TYPE_S8 url, const TYPE_S8 qErrorString)
{
    CaptureTelemetryData(METRIC_ERROR_REST, kMETRIC_ERROR_REST, kMETRIC_GROUP_ERROR,
        restError,
        qnetworkReply,
        httpStatus,
        url,
        qErrorString);
}

//-----------------------------------------------------------------------------
//  ERROR REPORTING:  Triggerred on crashes
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_BUG_REPORT(const TYPE_S8 sessionid, const TYPE_S8 error_categoryid)
{
    CaptureTelemetryData(METRIC_BUG_REPORT, kMETRIC_BUG_REPORT, kMETRIC_GROUP_ERROR, 
        sessionid,
        error_categoryid);

}

//-----------------------------------------------------------------------------
//  ERROR REPORTING:  Triggerred on crashes
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ERROR_CRASH(const TYPE_S8 error_categoryid, const TYPE_S8 user_report_preference, bool onShutdown)
{
    CaptureTelemetryData(METRIC_ERROR_CRASH, kMETRIC_ERROR_CRASH, kMETRIC_GROUP_ERROR,
        error_categoryid,
        user_report_preference,
        onShutdown);
}


///////////////////////////////////////////////////////////////////////////////
//  ERROR REPORTING
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Chat
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CHAT_SESSION_START(bool isInIgo, const uint64_t sendersNucleusId)
{
    //TODO if sendersNucleusId is 0 we could insert the local users nucid.
    CaptureTelemetryData(METRIC_CHAT_SESSION_START, kMETRIC_CHAT_SESSION_START, kMETRIC_GROUP_SOCIAL, isInIgo, sendersNucleusId);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Chat
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CHAT_SESSION_END(bool isInIgo)
{
    CaptureTelemetryData(METRIC_CHAT_SESSION_END, kMETRIC_CHAT_SESSION_END, kMETRIC_GROUP_SOCIAL, isInIgo);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  View Source
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_FRIEND_VIEWSOURCE(const TYPE_S8 source)
{
    CaptureTelemetryData(METRIC_FRIEND_VIEWSOURCE, kMETRIC_FRIEND_VIEWSOURCE, kMETRIC_GROUP_SOCIAL, 
        source);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Count
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_FRIEND_COUNT(int count)
{
    CaptureTelemetryData(METRIC_FRIEND_COUNT, kMETRIC_FRIEND_COUNT, kMETRIC_GROUP_SOCIAL, 
        count);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Import
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_FRIEND_IMPORT()
{

    CaptureTelemetryData(METRIC_FRIEND_IMPORT, kMETRIC_FRIEND_IMPORT, kMETRIC_GROUP_SOCIAL);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Invitation accepted
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_FRIEND_INVITE_ACCEPTED(bool isInIgo)
{
    CaptureTelemetryData(METRIC_FRIEND_INVITE_ACCEPTED, kMETRIC_FRIEND_INVITE_ACCEPTED, kMETRIC_GROUP_SOCIAL, isInIgo);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Invitation ignored
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_FRIEND_INVITE_IGNORED(bool isInIgo)
{
    CaptureTelemetryData(METRIC_FRIEND_INVITE_IGNORED, kMETRIC_FRIEND_INVITE_IGNORED, kMETRIC_GROUP_SOCIAL, isInIgo);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Invitation sent
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_FRIEND_INVITE_SENT(const TYPE_S8 source, bool isInIgo)
{
    CaptureTelemetryData(METRIC_FRIEND_INVITE_SENT, kMETRIC_FRIEND_INVITE_SENT, kMETRIC_GROUP_SOCIAL,
        source, isInIgo);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Block add request sent
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_FRIEND_BLOCK_ADD_SENT()
{
    CaptureTelemetryData(METRIC_FRIEND_BLOCK_ADD_SENT, kMETRIC_FRIEND_BLOCK_ADD_SENT, kMETRIC_GROUP_SOCIAL);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Block remove request sent
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_FRIEND_BLOCK_REMOVE_SENT()
{
    CaptureTelemetryData(METRIC_FRIEND_BLOCK_REMOVE_SENT, kMETRIC_FRIEND_BLOCK_REMOVE_SENT, kMETRIC_GROUP_SOCIAL);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Report request sent
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_FRIEND_REPORT_SENT()
{
    CaptureTelemetryData(METRIC_FRIEND_REPORT_SENT, kMETRIC_FRIEND_REPORT_SENT, kMETRIC_GROUP_SOCIAL);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Remove request sent
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_FRIEND_REMOVAL_SENT()
{
    CaptureTelemetryData(METRIC_FRIEND_REMOVAL_SENT, kMETRIC_FRIEND_REMOVAL_SENT, kMETRIC_GROUP_SOCIAL);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  FRIENDS:  Roster returned missing names
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_FRIEND_USERNAME_MISSING(int count)
{
    CaptureTelemetryData(METRIC_FRIEND_USERNAME_MISSING, kMETRIC_FRIEND_USERNAME_MISSING, kMETRIC_GROUP_SOCIAL, 
        count);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  GAME:  Invitation accepted
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GAME_INVITE_ACCEPTED(const TYPE_S8 offerId)
{
    CaptureTelemetryData(METRIC_GAME_INVITE_ACCEPTED, kMETRIC_GAME_INVITE_ACCEPTED, kMETRIC_GROUP_SOCIAL,
        offerId);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  GAME:  Invitation ignored
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GAME_INVITE_DECLINE_SENT() 
{
    CaptureTelemetryData(METRIC_GAME_INVITE_DECLINE_SENT, kMETRIC_GAME_INVITE_DECLINE_SENT, kMETRIC_GROUP_SOCIAL);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  GAME:  Invitation ignored
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GAME_INVITE_DECLINE_RECEIVED() 
{
    CaptureTelemetryData(METRIC_GAME_INVITE_DECLINE_RECEIVED, kMETRIC_GAME_INVITE_DECLINE_RECEIVED, kMETRIC_GROUP_SOCIAL);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  GAME:  Invitation sent
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GAME_INVITE_SENT(const TYPE_S8 offerId)
{
    CaptureTelemetryData(METRIC_GAME_INVITE_SENT, kMETRIC_GAME_INVITE_SENT, kMETRIC_GROUP_SOCIAL, offerId);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  GAME:  Invitation sent
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_LOGIN_INVISIBLE(bool invisible)
{
    CaptureTelemetryData(METRIC_LOGIN_INVISIBLE, kMETRIC_LOGIN_INVISIBLE, kMETRIC_GROUP_SOCIAL, invisible ? 1 : 0);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  USER PROFILE:  View source
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_USER_PROFILE_VIEW(const TYPE_S8 scope, const TYPE_S8 source, const TYPE_S8 view)
{
    CaptureTelemetryData(METRIC_USER_PROFILE_VIEW, kMETRIC_USER_PROFILE_VIEW, kMETRIC_GROUP_SOCIAL, 
        scope,
        source,
        view);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  USER PROFILE:  View game source - this is where the profile was clicked in the games
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_USER_PROFILE_VIEWGAMESOURCE(const TYPE_S8 source)
{
    EbisuTelemetryAPI::ViewGameSourceEnum srcx;
    if (strcmp(source, "EXPANSION") == 0)
        srcx = EbisuTelemetryAPI::Expansion;
    else if (strcmp(source, "DETAILS") == 0)
        srcx = EbisuTelemetryAPI::Details;
    else srcx = EbisuTelemetryAPI::Hovercard;

    CaptureTelemetryData(METRIC_USER_PROFILE_VIEWGAMESOURCE, kMETRIC_USER_PROFILE_VIEWGAMESOURCE, kMETRIC_GROUP_SOCIAL, 
        srcx);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  PRESENCE:  Set presence
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_USER_PRESENCE_SET(const TYPE_S8 presence)
{
    CaptureTelemetryData(METRIC_USER_PRESENCE_SET, kMETRIC_USER_PRESENCE_SET, kMETRIC_GROUP_SOCIAL, 
        presence);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  GROUP CHAT:  Create MUC Room
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GC_CREATE_BORN_MUC_ROOM()
{
    CaptureTelemetryData(METRIC_GC_CREATE_BORN_MUC_ROOM, kMETRIC_GC_CREATE_BORN_MUC_ROOM, kMETRIC_GROUP_SOCIAL);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  GROUP CHAT:  Receive MUC Invite
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GC_RECEIVE_MUC_INVITE()
{
    CaptureTelemetryData(METRIC_GC_RECEIVE_MUC_INVITE, kMETRIC_GC_RECEIVE_MUC_INVITE, kMETRIC_GROUP_SOCIAL);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  GROUP CHAT:  Decline MUC Invite
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GC_DECLINING_MUC_INVITE(const TYPE_S8 reason)
{
    CaptureTelemetryData(METRIC_GC_DECLINING_MUC_INVITE, kMETRIC_GC_DECLINING_MUC_INVITE, kMETRIC_GROUP_SOCIAL, 
        reason);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  GROUP CHAT:  Accepting MUC Invite
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GC_ACCEPTING_MUC_INVITE()
{
    CaptureTelemetryData(METRIC_GC_ACCEPTING_MUC_INVITE, kMETRIC_GC_ACCEPTING_MUC_INVITE, kMETRIC_GROUP_SOCIAL);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  GROUP CHAT:  Accepting Close Room Invite
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GC_ACCEPTING_CR_INVITE()
{
    CaptureTelemetryData(METRIC_GC_ACCEPTING_CR_INVITE, kMETRIC_GC_ACCEPTING_CR_INVITE, kMETRIC_GROUP_SOCIAL);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  GROUP CHAT:  Initiating MUC Upgrade
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GC_MUC_UPGRADE_INITIATING(bool lastThreadEmpty)
{
    CaptureTelemetryData(METRIC_GC_MUC_UPGRADE_INITIATING, kMETRIC_GC_MUC_UPGRADE_INITIATING, kMETRIC_GROUP_SOCIAL, 
        (lastThreadEmpty?1:0));
}

//-----------------------------------------------------------------------------
//  SOCIAL:  GROUP CHAT:  Auto-acceptiong MUC Upgrade
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GC_MUC_UPGRADE_AUTO_ACCEPT()
{
    CaptureTelemetryData(METRIC_GC_MUC_UPGRADE_AUTO_ACCEPT, kMETRIC_GC_MUC_UPGRADE_AUTO_ACCEPT, kMETRIC_GROUP_SOCIAL);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  GROUP CHAT:  Exited MUC room
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GC_MUC_EXIT(TYPE_I32 durationSeconds, TYPE_I32 messagesSent, TYPE_I32 maximumParticipants)
{
    CaptureTelemetryData(METRIC_GC_MUC_EXIT, kMETRIC_GC_MUC_EXIT, kMETRIC_GROUP_SOCIAL, 
        durationSeconds,
        messagesSent,
        maximumParticipants);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Enter room failure
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CHATROOM_ENTER_ROOM_FAIL(const TYPE_S8 xmppNode, const TYPE_S8 roomId, const TYPE_I32 errorCode)
{
    CaptureTelemetryData(METRIC_CHATROOM_ENTER_ROOM_FAIL, kMETRIC_CHATROOM_ENTER_ROOM_FAIL, kMETRIC_GROUP_SOCIAL,
        xmppNode,
        roomId,
        errorCode);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Create new Group
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CHATROOM_CREATE_GROUP(const TYPE_S8 groupId, const TYPE_I32 count)
{
    CaptureTelemetryData(METRIC_CHATROOM_CREATE_GROUP, kMETRIC_CHATROOM_CREATE_GROUP, kMETRIC_GROUP_SOCIAL,
        groupId,
        count);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Leave Group
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CHATROOM_LEAVE_GROUP(const TYPE_S8 groupId, const TYPE_I32 count)
{
    CaptureTelemetryData(METRIC_CHATROOM_LEAVE_GROUP, kMETRIC_CHATROOM_LEAVE_GROUP, kMETRIC_GROUP_SOCIAL,
        groupId,
        count);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Delete Group
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CHATROOM_DELETE_GROUP(const TYPE_S8 groupId, const TYPE_I32 count)
{
    CaptureTelemetryData(METRIC_CHATROOM_DELETE_GROUP, kMETRIC_CHATROOM_DELETE_GROUP, kMETRIC_GROUP_SOCIAL,
        groupId,
        count);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Delete Group Failed
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CHATROOM_DELETE_GROUP_FAILED(const TYPE_S8 groupId, const TYPE_I32 count)
{
    CaptureTelemetryData(METRIC_CHATROOM_DELETE_GROUP_FAILED, kMETRIC_CHATROOM_DELETE_GROUP_FAILED, kMETRIC_GROUP_SOCIAL,
        groupId,
        count);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Room Deactivated
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CHATROOM_DEACTIVATED(const TYPE_S8 groupId, const TYPE_S8 roomId, const TYPE_I32 deactivationType, const TYPE_S8 by)
{
    CaptureTelemetryData(METRIC_CHATROOM_DEACTIVATED, kMETRIC_CHATROOM_DEACTIVATED, kMETRIC_GROUP_SOCIAL,
        groupId,
        roomId,
        deactivationType,
        by);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Group was deleted
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CHATROOM_GROUP_WAS_DELETED(const TYPE_S8 groupId, const TYPE_I32 count)
{
    CaptureTelemetryData(METRIC_CHATROOM_GROUP_WAS_DELETED, kMETRIC_CHATROOM_GROUP_WAS_DELETED, kMETRIC_GROUP_SOCIAL,
        groupId,
        count);
}


//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Kicked from Group
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CHATROOM_KICKED_FROM_GROUP(const TYPE_S8 groupId, const TYPE_I32 count)
{
    CaptureTelemetryData(METRIC_CHATROOM_KICKED_FROM_GROUP, kMETRIC_CHATROOM_KICKED_FROM_GROUP, kMETRIC_GROUP_SOCIAL,
        groupId,
        count);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Group Count at login
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CHATROOM_GROUP_COUNT(const TYPE_I32 count)
{
    CaptureTelemetryData(METRIC_CHATROOM_GROUP_COUNT, kMETRIC_CHATROOM_GROUP_COUNT, kMETRIC_GROUP_SOCIAL,
        count);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Chat Room Enter
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CHATROOM_ENTER_ROOM(const TYPE_S8 roomId, const TYPE_S8 groupId, bool isInIgo)
{
    CaptureTelemetryData(METRIC_CHATROOM_ENTER_ROOM, kMETRIC_CHATROOM_ENTER_ROOM, kMETRIC_GROUP_SOCIAL,
        roomId,
        groupId,
        isInIgo);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Chat Room Exit
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CHATROOM_EXIT_ROOM(const TYPE_S8 roomId, const TYPE_S8 groupId, bool isInIgo)
{
    CaptureTelemetryData(METRIC_CHATROOM_EXIT_ROOM, kMETRIC_CHATROOM_EXIT_ROOM, kMETRIC_GROUP_SOCIAL,
        roomId,
        groupId,
        isInIgo);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Invite sent
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CHATROOM_INVITE_SENT(const TYPE_S8 groupId, const TYPE_S8 invitee)
{
    CaptureTelemetryData(METRIC_CHATROOM_INVITE_SENT, kMETRIC_CHATROOM_INVITE_SENT, kMETRIC_GROUP_SOCIAL,
        groupId,
        invitee);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Invite received
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CHATROOM_INVITE_RECEIVED(const TYPE_S8 groupId)
{
    CaptureTelemetryData(METRIC_CHATROOM_INVITE_RECEIVED, kMETRIC_CHATROOM_INVITE_RECEIVED, kMETRIC_GROUP_SOCIAL,
        groupId);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Invite accepted
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CHATROOM_INVITE_ACCEPTED(const TYPE_S8 groupId, const TYPE_I32 count)
{
    CaptureTelemetryData(METRIC_CHATROOM_INVITE_ACCEPTED, kMETRIC_CHATROOM_INVITE_ACCEPTED, kMETRIC_GROUP_SOCIAL,
        groupId,
        count);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  CHATROOM:  Invite declined
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CHATROOM_INVITE_DECLINED(const TYPE_S8 groupId)
{
    CaptureTelemetryData(METRIC_CHATROOM_INVITE_DECLINED, kMETRIC_CHATROOM_INVITE_DECLINED, kMETRIC_GROUP_SOCIAL,
        groupId);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  VOICE CHAT:  Voice Channel Error
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_VC_CHANNEL_ERROR(const TYPE_S8 type, const TYPE_S8 errorMessage, const TYPE_I32 errorCode)
{
    CaptureTelemetryData(METRIC_VC_CHANNEL_ERROR, kMETRIC_VC_CHANNEL_ERROR, kMETRIC_GROUP_SOCIAL,
        type,
        errorMessage,
        errorCode);
}


//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Browser started
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_IGO_BROWSER_START()
{
    CaptureTelemetryData(METRIC_IGO_BROWSER_START, kMETRIC_IGO_BROWSER_START, kMETRIC_GROUP_IGO);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Browser ended
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_IGO_BROWSER_END()
{
    CaptureTelemetryData(METRIC_IGO_BROWSER_END, kMETRIC_IGO_BROWSER_END, kMETRIC_GROUP_IGO);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Browser tab added
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_IGO_BROWSER_ADDTAB()
{
    CaptureTelemetryData(METRIC_IGO_BROWSER_ADDTAB, kMETRIC_IGO_BROWSER_ADDTAB, kMETRIC_GROUP_IGO);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Browser tab closed
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_IGO_BROWSER_CLOSETAB()
{
    CaptureTelemetryData(METRIC_IGO_BROWSER_CLOSETAB, kMETRIC_IGO_BROWSER_CLOSETAB, kMETRIC_GROUP_IGO);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Browser page load started
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_IGO_BROWSER_PAGELOAD()
{
    CaptureTelemetryData(METRIC_IGO_BROWSER_PAGELOAD, kMETRIC_IGO_BROWSER_PAGELOAD, kMETRIC_GROUP_IGO);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Browser URL Shortcut entered
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_IGO_BROWSER_URLSHORTCUT(const TYPE_S8 shortcut_id)
{
    CaptureTelemetryData(METRIC_IGO_BROWSER_URLSHORTCUT, kMETRIC_IGO_BROWSER_URLSHORTCUT, kMETRIC_GROUP_IGO,
        shortcut_id);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Browser miniapp opened
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_IGO_BROWSER_MAPP(const TYPE_S8 product_id)
{
    EA::Thread::AutoFutex m2(gameSessionList->GetLock());

    eastl::string8 Id = (product_id == NULL ? "" : product_id);
    uint64_t sessionId = 0;
    GameSessionList::SessionListType::iterator iter = gameSessionList->map.find(Id);
    if (iter != gameSessionList->map.end())
    {
        GameSession* session = iter->second;
        sessionId = session->sessionId();

        return CaptureTelemetryData(METRIC_IGO_BROWSER_MINIAPP, kMETRIC_IGO_BROWSER_MINIAPP, kMETRIC_GROUP_IGO,
            Id.c_str(),
            sessionId
            );
    }
}

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Overlay session started
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_IGO_OVERLAY_START()
{
    CaptureTelemetryData(METRIC_IGO_OVERLAY_START, kMETRIC_IGO_OVERLAY_START, kMETRIC_GROUP_IGO);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Overlay session finished
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_IGO_OVERLAY_END()
{
    CaptureTelemetryData(METRIC_IGO_OVERLAY_END, kMETRIC_IGO_OVERLAY_END, kMETRIC_GROUP_IGO);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Overlay session finished
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_IGO_OVERLAY_3RDPARTY_DLL(bool host, bool game, const TYPE_S8 name)
{
    CaptureTelemetryData(METRIC_IGO_3RDPARTY_DLL, kMETRIC_IGO_3RDPARTY_DLL, kMETRIC_GROUP_IGO, 
        host ? 1 : 0,
        game ? 1 : 0,
        name);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Injection failure
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_IGO_INJECTION_FAIL(const TYPE_S8 productId, bool elevated, bool freetrial)
{
    CaptureTelemetryData(METRIC_IGO_INJECTION_FAIL, kMETRIC_IGO_INJECTION_FAIL, kMETRIC_GROUP_IGO, 
        productId,
        elevated  ? 1 : 0,
        freetrial ? 1 : 0);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Hooking begin session
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_IGO_HOOKING_BEGIN(const TYPE_S8 productId, TYPE_I64 timestamp, TYPE_U32 pid, bool showHardwareSpecs)
{

    eastl::string8 osInfo;
    eastl::string8 cpuInfo;
    eastl::string8 gpuInfo;

    if (showHardwareSpecs)
    {
        const char8_t* description = m_sysInfo->GetOSDescription();
        const char8_t* version = m_sysInfo->GetOSVersion();
        osInfo.sprintf("%s, %s(%d bits)", description ? description : "", version ? version : "", m_sysInfo->GetOSArchitecture());

        const char8_t* cpuName = m_sysInfo->GetCPUName();
        cpuInfo.sprintf("%s (%d/%d)", cpuName ? cpuName : "", m_sysInfo->GetNumCPUs(), m_sysInfo->GetNumCores());

        uint32_t cnt = m_sysInfo->GetNumVideoControllers();
        for (uint32_t idx = 0; idx < cnt; ++idx)
        {
            const char8_t* adapterName = m_sysInfo->GetVideoAdapterName(idx);
            if (idx > 0)
                gpuInfo += " - ";

            gpuInfo.sprintf("%s (%d)", adapterName ? adapterName : NULL, m_sysInfo->GetVideoAdapterVRAM(idx));
        }
    }

    CaptureTelemetryData(METRIC_IGO_HOOKING_BEGIN, kMETRIC_IGO_HOOKING_BEGIN, kMETRIC_GROUP_IGO, 
        productId, timestamp, pid, osInfo.c_str(), cpuInfo.c_str(), gpuInfo.c_str());
}

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Hooking failure
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_IGO_HOOKING_FAIL(const TYPE_S8 productId, TYPE_I64 timestamp, TYPE_U32 pid, const TYPE_S8 context, const TYPE_S8 renderer, const TYPE_S8 message)
{
    CaptureTelemetryData(METRIC_IGO_HOOKING_FAIL, kMETRIC_IGO_HOOKING_FAIL, kMETRIC_GROUP_IGO, 
        productId, timestamp, pid, context, renderer, message);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Hooking info
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_IGO_HOOKING_INFO(const TYPE_S8 productId, TYPE_I64 timestamp, TYPE_U32 pid, const TYPE_S8 context, const TYPE_S8 renderer, const TYPE_S8 message)
{
    CaptureTelemetryData(METRIC_IGO_HOOKING_INFO, kMETRIC_IGO_HOOKING_INFO, kMETRIC_GROUP_IGO, 
        productId, timestamp, pid, context, renderer, message);
}

//-----------------------------------------------------------------------------
//  SOCIAL:  IGO:  Hooking end session
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_IGO_HOOKING_END(const TYPE_S8 productId, TYPE_I64 timestamp, TYPE_U32 pid)
{
    CaptureTelemetryData(METRIC_IGO_HOOKING_END, kMETRIC_IGO_HOOKING_END, kMETRIC_GROUP_IGO, 
        productId, timestamp, pid);
}

///////////////////////////////////////////////////////////////////////////////
//  STORE
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  STORE:  NAVIGATION URL
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_STORE_NAVIGATE_URL(const TYPE_S8 url)
{
    CaptureTelemetryData(METRIC_STORE_URL, kMETRIC_STORE_URL, kMETRIC_GROUP_STORE, 
        url);
}

//-----------------------------------------------------------------------------
//  STORE:  NAVIGATION:  Load status
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_STORE_LOAD_STATUS(const TYPE_S8 url, const TYPE_U32 durationInMs, const TYPE_U32 success)
{
    CaptureTelemetryData(METRIC_STORE_LOAD_STATUS, kMETRIC_STORE_LOAD_STATUS, kMETRIC_GROUP_STORE,
        url,
        durationInMs,
        success);
}

//-----------------------------------------------------------------------------
//  STORE:  NAVIGATION: On the House
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_STORE_NAVIGATE_OTH(OnTheHouseEnum type)
{
    CaptureTelemetryData(METRIC_STORE_OTH, kMETRIC_STORE_OTH, kMETRIC_GROUP_STORE, 
        type);
}
///////////////////////////////////////////////////////////////////////////////
//  HOME PAGES
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  HOME: NAVIGATION URL
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HOME_NAVIGATE_URL(const TYPE_S8 url)
{
    CaptureTelemetryData(METRIC_HOME_URL, kMETRIC_HOME_URL, kMETRIC_GROUP_HOME, 
        url);
}


///////////////////////////////////////////////////////////////////////////////
//  SELF PATCH
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  SELF PATCH:  Patch found
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_SELFPATCH_FOUND(const TYPE_S8 new_version, TYPE_U16 required, TYPE_U16 emergency)
{
    CaptureTelemetryData(METRIC_SELFPATCH_FOUND, kMETRIC_SELFPATCH_FOUND, kMETRIC_GROUP_SELFPATCH, 
        new_version,
        required,
        emergency);
}

//-----------------------------------------------------------------------------
//  SELF PATCH:  Patch declined
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_SELFPATCH_DECLINED(const TYPE_S8 new_version, TYPE_U16 required, TYPE_U16 emergency)
{
    CaptureTelemetryData(METRIC_SELFPATCH_DECLINED, kMETRIC_SELFPATCH_DECLINED, kMETRIC_GROUP_SELFPATCH, 
        new_version,
        required,
        emergency);
}

//-----------------------------------------------------------------------------
//  SELF PATCH:  Download started
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_SELFPATCH_DL_START(const TYPE_S8 new_version, TYPE_U16 required, TYPE_U16 emergency)
{
    CaptureTelemetryData(METRIC_SELFPATCH_DL_START, kMETRIC_SELFPATCH_DL_START, kMETRIC_GROUP_SELFPATCH, 
        new_version,
        required,
        emergency);
}

//-----------------------------------------------------------------------------
//  SELF PATCH:  Download finished
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_SELFPATCH_DL_FINISHED(const TYPE_S8 new_version, TYPE_U16 required, TYPE_U16 emergency)
{
    CaptureTelemetryData(METRIC_SELFPATCH_DL_FINISHED, kMETRIC_SELFPATCH_DL_FINISHED, kMETRIC_GROUP_SELFPATCH, 
        new_version,
        required,
        emergency);
}

//-----------------------------------------------------------------------------
//  SELF PATCH:  Launched
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_SELFPATCH_LAUNCHED(const TYPE_S8 new_version, TYPE_U16 required, TYPE_U16 emergency)
{
    CaptureTelemetryData(METRIC_SELFPATCH_LAUNCHED, kMETRIC_SELFPATCH_LAUNCHED, kMETRIC_GROUP_SELFPATCH, 
        new_version,
        required,
        emergency);
}

//-----------------------------------------------------------------------------
//  SELF PATCH:  Patch switch offline mode
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_SELFPATCH_OFFLINE_MODE(const TYPE_S8 new_version, TYPE_U16 required, TYPE_U16 emergency)
{
    CaptureTelemetryData(METRIC_SELFPATCH_OFFLINE_MODE, kMETRIC_SELFPATCH_OFFLINE_MODE, kMETRIC_GROUP_SELFPATCH, 
        new_version,
        required,
        emergency);
}

//-----------------------------------------------------------------------------
//  SELF PATCH:  Signature invalid hash
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_SELFPATCH_SIGNATURE_INVALID_HASH(const TYPE_S8 new_version, TYPE_U16 required, TYPE_U16 emergency)
{
    CaptureTelemetryData(METRIC_SELFPATCH_SIGNATURE_INVALID_HASH, kMETRIC_SELFPATCH_SIGNATURE_INVALID_HASH, kMETRIC_GROUP_SELFPATCH, 
        new_version,
        required,
        emergency);

}

//-----------------------------------------------------------------------------
//  SELF PATCH:  Signature untrusted signer
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_SELFPATCH_SIGNATURE_UNTRUSTED_SIGNER(const TYPE_S8 new_version, TYPE_U16 required, TYPE_U16 emergency)
{
    CaptureTelemetryData(METRIC_SELFPATCH_SIGNATURE_UNTRUSTED_SIGNER, kMETRIC_SELFPATCH_SIGNATURE_UNTRUSTED_SIGNER, kMETRIC_GROUP_SELFPATCH, 
        new_version,
        required,
        emergency);
}

///////////////////////////////////////////////////////////////////////////////
//  SSL ERROR
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  NETWORK:  SSL error
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_NETWORK_SSL_ERROR(const TYPE_S8 url, TYPE_U32 code, const TYPE_S8 cert_sha1, const TYPE_S8 cert_name, const TYPE_S8 issuer_name)
{
    CaptureTelemetryData(METRIC_NETWORK_SSL_ERROR, kMETRIC_NETWORK_SSL_ERROR, kMETRIC_GROUP_NETWORK, 
        url, 
        code,
        cert_sha1,
        cert_name,
        issuer_name);
}


///////////////////////////////////////////////////////////////////////////////
//  ACTIVATION
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  ACTIVATION:  Successful code redemption
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ACTIVATION_CODE_REDEEM_SUCCESS()
{
    CaptureTelemetryData(METRIC_ACTIVATION_CODE_REDEEM_SUCCESS, kMETRIC_ACTIVATION_CODE_REDEEM_SUCCESS, kMETRIC_GROUP_ACTIVATION);
}

//-----------------------------------------------------------------------------
//  ACTIVATION:  Error on code redemption
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ACTIVATION_CODE_REDEEM_ERROR(TYPE_I32 errorCode)
{
    CaptureTelemetryData(METRIC_ACTIVATION_CODE_REDEEM_ERROR, kMETRIC_ACTIVATION_CODE_REDEEM_ERROR, kMETRIC_GROUP_ACTIVATION, 
        errorCode);
}

//-----------------------------------------------------------------------------
//  ACTIVATION:  Code redemption page request
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ACTIVATION_REDEEM_PAGE_REQUEST()
{
    CaptureTelemetryData(METRIC_ACTIVATION_REDEEM_PAGE_REQUEST, kMETRIC_ACTIVATION_REDEEM_PAGE_REQUEST, kMETRIC_GROUP_ACTIVATION);
}

//-----------------------------------------------------------------------------
//  ACTIVATION:  Code redemption page error
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ACTIVATION_REDEEM_PAGE_SUCCESS()
{
    CaptureTelemetryData(METRIC_ACTIVATION_REDEEM_PAGE_SUCCESS, kMETRIC_ACTIVATION_REDEEM_PAGE_SUCCESS, kMETRIC_GROUP_ACTIVATION);
}

//-----------------------------------------------------------------------------
//  ACTIVATION:  Code redemption page error
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ACTIVATION_REDEEM_PAGE_ERROR(TYPE_I32 errorCode, TYPE_I32 httpCode)
{
    CaptureTelemetryData(METRIC_ACTIVATION_REDEEM_PAGE_ERROR, kMETRIC_ACTIVATION_REDEEM_PAGE_ERROR, kMETRIC_GROUP_ACTIVATION, 
        errorCode,
        httpCode);
}

//-----------------------------------------------------------------------------
//  ACTIVATION:  FAQ page request
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ACTIVATION_FAQ_PAGE_REQUEST()
{
    CaptureTelemetryData(METRIC_ACTIVATION_FAQ_PAGE_REQUEST, kMETRIC_ACTIVATION_FAQ_PAGE_REQUEST, kMETRIC_GROUP_ACTIVATION);
}

//-----------------------------------------------------------------------------
//  ACTIVATION:  Close window event for code redemption pages
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ACTIVATION_WINDOW_CLOSE(const TYPE_S8 eventString)
{
    CaptureTelemetryData(METRIC_ACTIVATION_WINDOW_CLOSE, kMETRIC_ACTIVATION_WINDOW_CLOSE, kMETRIC_GROUP_ACTIVATION, 
        eventString);
}


///////////////////////////////////////////////////////////////////////////////
//  REPAIR INSTALL
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  REPAIR INSTALL:  Num files replaced
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_REPAIRINSTALL_FILES_STATS(const TYPE_S8 product_id, TYPE_U32 replaced, TYPE_U32 total_files)
{
    CaptureTelemetryData(METRIC_REPAIRINSTALL_FILES_STATS, kMETRIC_REPAIRINSTALL_FILES_STATS, kMETRIC_GROUP_REPAIRINSTALL, 
        product_id,
        replaced, total_files);
}

//-----------------------------------------------------------------------------
//  REPAIR INSTALL:  requested
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_REPAIRINSTALL_REQUESTED(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_REPAIRINSTALL_REQUESTED, kMETRIC_REPAIRINSTALL_REQUESTED, kMETRIC_GROUP_REPAIRINSTALL, 
        product_id);
}

//-----------------------------------------------------------------------------
//  REPAIR INSTALL:  verify canceled
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_REPAIRINSTALL_VERIFYCANCELED(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_REPAIRINSTALL_VERIFYCANCELED, kMETRIC_REPAIRINSTALL_VERIFYCANCELED, kMETRIC_GROUP_REPAIRINSTALL, 
        product_id);
}

//-----------------------------------------------------------------------------
//  REPAIR INSTALL:  replace canceled
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_REPAIRINSTALL_REPLACECANCELED(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_REPAIRINSTALL_REPLACECANCELED, kMETRIC_REPAIRINSTALL_REPLACECANCELED, kMETRIC_GROUP_REPAIRINSTALL, 
        product_id);
}


//-----------------------------------------------------------------------------
//  REPAIR INSTALL:  replace canceled
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_REPAIRINSTALL_REPLACINGFILES(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_REPAIRINSTALL_REPLACINGFILES, kMETRIC_REPAIRINSTALL_REPLACINGFILES, kMETRIC_GROUP_REPAIRINSTALL, 
        product_id);
}


//-----------------------------------------------------------------------------
//  REPAIR INSTALL:  verify files completed
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_REPAIRINSTALL_VERIFYFILESCOMPLETED(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_REPAIRINSTALL_VERIFYFILESCOMPLETED, kMETRIC_REPAIRINSTALL_VERIFYFILESCOMPLETED, kMETRIC_GROUP_REPAIRINSTALL, 
        product_id);
}


//-----------------------------------------------------------------------------
//  REPAIR INSTALL:  replacing files completed
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_REPAIRINSTALL_REPLACINGFILESCOMPLETED(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_REPAIRINSTALL_REPLACINGFILESCOMPLETED, kMETRIC_REPAIRINSTALL_REPLACINGFILESCOMPLETED, kMETRIC_GROUP_REPAIRINSTALL, 
        product_id);
}


///////////////////////////////////////////////////////////////////////////////
//  CLOUD SAVES
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  CLOUD SAVE:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CLOUD_WARNING_SPACE_CLOUD_LOW(const TYPE_S8 product_id, TYPE_U32 clsp)
{
    CaptureTelemetryData(METRIC_CLOUD_WARNING_SPACE_CLOUD_LOW, kMETRIC_CLOUD_WARNING_SPACE_CLOUD_LOW, kMETRIC_GROUP_CLOUD,
        product_id,
        clsp);
}

//-----------------------------------------------------------------------------
//  CLOUD SAVE:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CLOUD_WARNING_SPACE_CLOUD_MAX_CAPACITY(const TYPE_S8 product_id, TYPE_U32 clsp)
{
    CaptureTelemetryData(METRIC_CLOUD_WARNING_SPACE_CLOUD_MAX_CAPACITY, kMETRIC_CLOUD_WARNING_SPACE_CLOUD_MAX_CAPACITY, kMETRIC_GROUP_CLOUD,
        product_id,
        clsp);
}

//-----------------------------------------------------------------------------
//  CLOUD SAVE:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CLOUD_WARNING_SYNC_CONFLICT(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_CLOUD_WARNING_SYNC_CONFLICT, kMETRIC_CLOUD_WARNING_SYNC_CONFLICT, kMETRIC_GROUP_CLOUD,
        product_id);
}

//-----------------------------------------------------------------------------
//  CLOUD SAVE:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CLOUD_ACTION_OBJECT_DELETION(const TYPE_S8 product_id, TYPE_U32 num_obj_deleted)
{
    CaptureTelemetryData(METRIC_CLOUD_ACTION_OBJECT_DELETION, kMETRIC_CLOUD_ACTION_OBJECT_DELETION, kMETRIC_GROUP_CLOUD,
        product_id,
        num_obj_deleted);
}

//-----------------------------------------------------------------------------
//  CLOUD SAVE:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CLOUD_ACTION_GAME_NEW_CONTENT_DOWNLOADED(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_CLOUD_ACTION_GAME_NEW_CONTENT_DOWNLOADED, kMETRIC_CLOUD_ACTION_GAME_NEW_CONTENT_DOWNLOADED, kMETRIC_GROUP_CLOUD,
        product_id);
}

//-----------------------------------------------------------------------------
//  CLOUD SAVE:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CLOUD_MANUAL_RECOVERY(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_CLOUD_MANUAL_RECOVERY, kMETRIC_CLOUD_MANUAL_RECOVERY, kMETRIC_GROUP_CLOUD,
        product_id);
}

//-----------------------------------------------------------------------------
//  CLOUD SAVE:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CLOUD_AUTOMATIC_SAVE_SUCCESS(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_CLOUD_AUTOMATIC_SAVE_SUCCESS, kMETRIC_CLOUD_AUTOMATIC_SAVE_SUCCESS, kMETRIC_GROUP_CLOUD,
        product_id);
}

//-----------------------------------------------------------------------------
//  CLOUD SAVE:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CLOUD_AUTOMATIC_RECOVERY_SUCCESS(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_CLOUD_AUTOMATIC_RECOVERY_SUCCESS, kMETRIC_CLOUD_AUTOMATIC_RECOVERY_SUCCESS, kMETRIC_GROUP_CLOUD,
        product_id);
}

//-----------------------------------------------------------------------------
//  CLOUD SAVE:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CLOUD_ERROR_SYNC_LOCKING_UNSUCCESFUL(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_CLOUD_ACTION_SYNC_LOCKING_UNSUCCESFUL, kMETRIC_CLOUD_ACTION_SYNC_LOCKING_UNSUCCESFUL, kMETRIC_GROUP_CLOUD,
        product_id);
}

//-----------------------------------------------------------------------------
//  CLOUD SAVE:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CLOUD_ERROR_PUSH_FAILED(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_CLOUD_ACTION_PUSH_FAILED, kMETRIC_CLOUD_ACTION_PUSH_FAILED, kMETRIC_GROUP_CLOUD,
        product_id);
}

//-----------------------------------------------------------------------------
//  CLOUD SAVE:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CLOUD_ERROR_PULL_FAILED(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_CLOUD_ACTION_PULL_FAILED, kMETRIC_CLOUD_ACTION_PULL_FAILED, kMETRIC_GROUP_CLOUD,
        product_id);
}

//-----------------------------------------------------------------------------
//  CLOUD SAVE:
//-----------------------------------------------------------------------------
void  EbisuTelemetryAPI::Metric_CLOUD_MIGRATION_SUCCESS(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_CLOUD_MIGRATION_SUCCESS, kMETRIC_CLOUD_MIGRATION_SUCCESS, kMETRIC_GROUP_CLOUD,
        product_id);
}

//-----------------------------------------------------------------------------
//  CLOUD SAVE:
//-----------------------------------------------------------------------------
void  EbisuTelemetryAPI::Metric_CLOUD_PUSH_MONITORING(const TYPE_S8 product_id, const TYPE_U32 numFiles, TYPE_U32 totalUploadSize)
{
    CaptureTelemetryData(METRIC_CLOUD_PUSH_MONITORING, kMETRIC_CLOUD_PUSH_MONITORING, kMETRIC_GROUP_CLOUD,
        product_id,
        numFiles,
        totalUploadSize);
}

///////////////////////////////////////////////////////////////////////////////
//  SUBSCRIPTION
///////////////////////////////////////////////////////////////////////////////
void EbisuTelemetryAPI::Metric_SUBSCRIPTION_ENT_UPGRADED(const TYPE_S8 ownedOfferId, const TYPE_S8 subscriptionOfferId, const TYPE_S8 subscriptionElapsedTime, const bool successful, const TYPE_S8 context)
{
    return CaptureTelemetryData(METRIC_SUBSCRIPTION_ENT_UPGRADED, kMETRIC_SUBSCRIPTION_ENT_UPGRADED, kMETRIC_GROUP_SUBSCRIPTION,
        ownedOfferId,
        subscriptionOfferId,
        subscriptionElapsedTime,
        successful ? 1 : 0,
        context);
}

void EbisuTelemetryAPI::Metric_SUBSCRIPTION_ENT_REMOVE_START(const TYPE_S8 subscriptionOfferId, const TYPE_S8 subscriptionElapsedTime, const bool successful, const TYPE_S8 context)
{
    return CaptureTelemetryData(METRIC_SUBSCRIPTION_ENT_REMOVE_START, kMETRIC_SUBSCRIPTION_ENT_REMOVE_START, kMETRIC_GROUP_SUBSCRIPTION,
        subscriptionOfferId,
        subscriptionElapsedTime,
        successful ? 1 : 0,
        context);
}

void EbisuTelemetryAPI::Metric_SUBSCRIPTION_ENT_REMOVED(const TYPE_S8 subscriptionOfferId, const bool successful, const TYPE_S8 context)
{
    return CaptureTelemetryData(METRIC_SUBSCRIPTION_ENT_REMOVED, kMETRIC_SUBSCRIPTION_ENT_REMOVED, kMETRIC_GROUP_SUBSCRIPTION,
        subscriptionOfferId,
        successful ? 1 : 0,
        context);
}

void EbisuTelemetryAPI::Metric_SUBSCRIPTION_UPSELL(const TYPE_S8 context, const bool isSubscriptionActive)
{
    return CaptureTelemetryData(METRIC_SUBSCRIPTION_UPSELL, kMETRIC_SUBSCRIPTION_UPSELL, kMETRIC_GROUP_SUBSCRIPTION,
        context,
        isSubscriptionActive ? 1 : 0);
}

void EbisuTelemetryAPI::Metric_SUBSCRIPTION_FAQ_PAGE_REQUEST(const TYPE_S8 context)
{
    return CaptureTelemetryData(METRIC_SUBSCRIPTION_FAQ_PAGE_REQUEST, kMETRIC_SUBSCRIPTION_FAQ_PAGE_REQUEST, kMETRIC_GROUP_SUBSCRIPTION,
        context);
}

void EbisuTelemetryAPI::Metric_SUBSCRIPTION_USER_GOES_ONLINE(const TYPE_S8 lastKnownGoodTime, const TYPE_S8 subscriptionEndTime)
{
    return CaptureTelemetryData(METRIC_SUBSCRIPTION_ONLINE, kMETRIC_SUBSCRIPTION_ONLINE, kMETRIC_GROUP_SUBSCRIPTION,
        lastKnownGoodTime, subscriptionEndTime);
}

///////////////////////////////////////////////////////////////////////////////
//  MEMORY FOOTPRINT
///////////////////////////////////////////////////////////////////////////////

void EbisuTelemetryAPI::Metric_METRIC_FOOTPRINT_INFO_STORE(TYPE_U32 unloadmemory_data, TYPE_U32 reloadmemory_data)
{
    CaptureTelemetryData(METRIC_FOOTPRINT_INFO_STORE, kMETRIC_FOOTPRINT_INFO_STORE
        , kMETRIC_GROUP_MEMORYFOOTPRINT,
        unloadmemory_data,
        reloadmemory_data);
}


///////////////////////////////////////////////////////////////////////////////
//  SINGLE LOGIN
///////////////////////////////////////////////////////////////////////////////

void EbisuTelemetryAPI::Metric_SINGLE_LOGIN_SECOND_USER_LOGGING_IN(const TYPE_S8 loginType)
{
    CaptureTelemetryData(METRIC_SINGLE_LOGIN_USER_LOGGING_IN, kMETRIC_SINGLE_LOGIN_USER_LOGGING_IN, kMETRIC_GROUP_SINGLE_LOGIN,
        loginType);
}

void EbisuTelemetryAPI::Metric_SINGLE_LOGIN_FIRST_USER_LOGGING_OUT()
{
    CaptureTelemetryData(METRIC_SINGLE_LOGIN_USER_LOGGING_OUT, kMETRIC_SINGLE_LOGIN_USER_LOGGING_OUT, kMETRIC_GROUP_SINGLE_LOGIN);
}

void EbisuTelemetryAPI::Metric_SINGLE_LOGIN_GO_OFFLINE_ACTION()
{
    CaptureTelemetryData(METRIC_SINGLE_LOGIN_GO_OFFLINE_ACTION, kMETRIC_SINGLE_LOGIN_GO_OFFLINE_ACTION, kMETRIC_GROUP_SINGLE_LOGIN);
}

void EbisuTelemetryAPI::Metric_SINGLE_LOGIN_GO_ONLINE_ACTION()
{
    CaptureTelemetryData(METRIC_SINGLE_LOGIN_GO_ONLINE_ACTION, kMETRIC_SINGLE_LOGIN_GO_ONLINE_ACTION, kMETRIC_GROUP_SINGLE_LOGIN);
}

///////////////////////////////////////////////////////////////////////////////
//  SSO
///////////////////////////////////////////////////////////////////////////////

void EbisuTelemetryAPI::Metric_SSO_FLOW_START(const TYPE_S8 authCode, const TYPE_S8 authToken, const TYPE_S8 loginType, const TYPE_U32 userOnline)
{
    CaptureTelemetryData(METRIC_SSO_FLOW_START, kMETRIC_SSO_FLOW_START, kMETRIC_GROUP_SSO,
        authCode,
        authToken,
        loginType,
        userOnline);
}

void EbisuTelemetryAPI::Metric_SSO_FLOW_RESULT(const TYPE_S8 result, const TYPE_S8 action, const TYPE_S8 loginType)
{
    CaptureTelemetryData(METRIC_SSO_FLOW_RESULT, kMETRIC_SSO_FLOW_RESULT, kMETRIC_GROUP_SSO,
        result, 
        action,
        loginType);
}

void EbisuTelemetryAPI::Metric_SSO_FLOW_ERROR(const TYPE_S8 reason, const TYPE_S8 restString, const TYPE_I32 rest, const TYPE_U32 http,  const TYPE_S8 offerIds)
{
    CaptureTelemetryData(METRIC_SSO_FLOW_ERROR, kMETRIC_SSO_FLOW_ERROR, kMETRIC_GROUP_SSO,
        reason, 
        restString,
        rest,
        http,
        offerIds);
}

void EbisuTelemetryAPI::Metric_SSO_FLOW_INFO(const TYPE_S8 info)
{
    CaptureTelemetryData(METRIC_SSO_FLOW_INFO, kMETRIC_SSO_FLOW_INFO, kMETRIC_GROUP_SSO,
        info);
}

///////////////////////////////////////////////////////////////////////////////
//  3PDD 
///////////////////////////////////////////////////////////////////////////////

void EbisuTelemetryAPI::Metric_3PDD_INSTALL_DIALOG_SHOW()
{
    CaptureTelemetryData(METRIC_3PDD_INSTALL_DIALOG_SHOW, kMETRIC_3PDD_INSTALL_DIALOG_SHOW, kMETRIC_GROUP_3PDD_INSTALL);
}

void EbisuTelemetryAPI::Metric_3PDD_INSTALL_DIALOG_CANCEL()
{
    CaptureTelemetryData(METRIC_3PDD_INSTALL_DIALOG_CANCEL, kMETRIC_3PDD_INSTALL_DIALOG_CANCEL, kMETRIC_GROUP_3PDD_INSTALL);
}

void EbisuTelemetryAPI::Metric_3PDD_INSTALL_TYPE(const TYPE_S8 installType)
{
    CaptureTelemetryData(METRIC_3PDD_INSTALL_TYPE, kMETRIC_3PDD_INSTALL_TYPE, kMETRIC_GROUP_3PDD_INSTALL,
        installType);
}

void EbisuTelemetryAPI::Metric_3PDD_PLAY_DIALOG_SHOW()
{
    CaptureTelemetryData(METRIC_3PDD_PLAY_DIALOG_SHOW, kMETRIC_3PDD_PLAY_DIALOG_SHOW, kMETRIC_GROUP_3PDD_INSTALL);
}

void EbisuTelemetryAPI::Metric_3PDD_PLAY_DIALOG_CANCEL()
{
    CaptureTelemetryData(METRIC_3PDD_PLAY_DIALOG_CANCEL, kMETRIC_3PDD_PLAY_DIALOG_CANCEL, kMETRIC_GROUP_3PDD_INSTALL);
}

void EbisuTelemetryAPI::Metric_3PDD_PLAY_TYPE(const TYPE_S8 playType)
{
    CaptureTelemetryData(METRIC_3PDD_PLAY_TYPE, kMETRIC_3PDD_PLAY_TYPE, kMETRIC_GROUP_3PDD_INSTALL,
        playType);
}

void EbisuTelemetryAPI::Metric_3PDD_INSTALL_COPYCDKEY()
{
    CaptureTelemetryData(METRIC_3PDD_INSTALL_COPYCDKEY, kMETRIC_3PDD_INSTALL_COPYCDKEY, kMETRIC_GROUP_3PDD_INSTALL);
}

void EbisuTelemetryAPI::Metric_3PDD_PLAY_COPYCDKEY()
{
    CaptureTelemetryData(METRIC_3PDD_PLAY_COPYCDKEY, kMETRIC_3PDD_PLAY_COPYCDKEY, kMETRIC_GROUP_3PDD_INSTALL);
}

///////////////////////////////////////////////////////////////////////////////
//  FREE TRIALS
///////////////////////////////////////////////////////////////////////////////
void EbisuTelemetryAPI::Metric_FREETRIAL_PURCHASE(const TYPE_S8 product_id, const TYPE_S8 expired, const TYPE_S8 source )
{
    CaptureTelemetryData(METRIC_FREETRIAL_PURCHASE, kMETRIC_FREETRIAL_PURCHASE, kMETRIC_GROUP_FREETRIAL,
        product_id,
        expired,
        source);
}

///////////////////////////////////////////////////////////////////////////////
//  CONTENT SALES
///////////////////////////////////////////////////////////////////////////////
void EbisuTelemetryAPI::Metric_CONTENTSALES_PURCHASE(const TYPE_S8 id, const TYPE_S8 productType, const TYPE_S8 context)
{
    CaptureTelemetryData(METRIC_CONTENTSALES_PURCHASE, kMETRIC_CONTENTSALES_PURCHASE, kMETRIC_GROUP_CONTENTSALES,
        id,
        productType,
        context);
}

void EbisuTelemetryAPI::Metric_CONTENTSALES_VIEW_IN_STORE(const TYPE_S8 id, const TYPE_S8 productType, const TYPE_S8 context)
{
    CaptureTelemetryData(METRIC_CONTENTSALES_VIEW_IN_STORE, kMETRIC_CONTENTSALES_VIEW_IN_STORE, kMETRIC_GROUP_CONTENTSALES,
        id,
        productType,
        context);
}

///////////////////////////////////////////////////////////////////////////////
//  HELP
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  HELP:  NAVIGATION:  Origin Help
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HELP_ORIGIN_HELP()
{
    CaptureTelemetryData(METRIC_HELP_ORIGIN_HELP, kMETRIC_HELP_ORIGIN_HELP, kMETRIC_GROUP_HELP);
}

//-----------------------------------------------------------------------------
//  HELP:  NAVIGATION:  What's New
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HELP_WHATS_NEW()
{
    CaptureTelemetryData(METRIC_HELP_WHATS_NEW, kMETRIC_HELP_WHATS_NEW, kMETRIC_GROUP_HELP);
}

//-----------------------------------------------------------------------------
//  HELP:  NAVIGATION:  Origin Forum
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HELP_ORIGIN_FORUM()
{
    CaptureTelemetryData(METRIC_HELP_ORIGIN_FORUM, kMETRIC_HELP_ORIGIN_FORUM, kMETRIC_GROUP_HELP);
}

//-----------------------------------------------------------------------------
// HELP: NAVIGATION: Origin ER tool
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HELP_ORIGIN_ER()
{
    CaptureTelemetryData(METRIC_HELP_ORIGIN_ER, kMETRIC_HELP_ORIGIN_ER, kMETRIC_GROUP_HELP);

}

///////////////////////////////////////////////////////////////////////////////
//  ORIGIN ERROR REPORTER
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Origin Error Reporter: Launched
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_OER_LAUNCHED(bool fromClient)
{
    CaptureTelemetryData(METRIC_OER_LAUNCHED, kMETRIC_OER_LAUNCHED, kMETRIC_GROUP_OER,
        fromClient);

}

//-----------------------------------------------------------------------------
// Origin Error Reporter: Error
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_OER_ERROR()
{
    CaptureTelemetryData(METRIC_OER_ERROR, kMETRIC_OER_ERROR, kMETRIC_GROUP_OER);

}

//-----------------------------------------------------------------------------
// Origin Error Reporter: Report Sent
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_OER_REPORT_SENT(const TYPE_S8 reportId)
{
    CaptureTelemetryData(METRIC_OER_REPORT_SENT, kMETRIC_OER_REPORT_SENT, kMETRIC_GROUP_OER, 
        reportId);

}

///////////////////////////////////////////////////////////////////////////////
//  PERFORMANCE: TIMERS
///////////////////////////////////////////////////////////////////////////////

enum
{
    IS_ONEOFF_TIMER = 1,
    HAS_TRIGGERED = 1,
    STOPPED = 1
};

struct PerformanceTimerInfo
{
    uint64_t time;
    const char* name;
    bool isOneOff;
    bool hasTriggered;
    bool isStopped;
};

PerformanceTimerInfo TimerInfos[EbisuTelemetryAPI::NUM_PERF_TIMERS] =
{
    { 0, "Authentication",          !IS_ONEOFF_TIMER,   !HAS_TRIGGERED,   !STOPPED },
    { 0, "Bootstrap",               !IS_ONEOFF_TIMER,   !HAS_TRIGGERED,   !STOPPED },
    { 0, "ContentRetrieval",        !IS_ONEOFF_TIMER,   !HAS_TRIGGERED,   !STOPPED },
    { 0, "LoginToFriendsList",      IS_ONEOFF_TIMER,    !HAS_TRIGGERED,   !STOPPED },
    { 0, "LoginToMainPage",         IS_ONEOFF_TIMER,    !HAS_TRIGGERED,   !STOPPED },
    { 0, "LoginToMainPageLoaded",   IS_ONEOFF_TIMER,    !HAS_TRIGGERED,   !STOPPED },
    { 0, "RTPGameLaunch_launching", !IS_ONEOFF_TIMER,   !HAS_TRIGGERED,   !STOPPED },
    { 0, "RTPGameLaunch_running",   !IS_ONEOFF_TIMER,   !HAS_TRIGGERED,   !STOPPED },
    { 0, "ShowLoginPage",           IS_ONEOFF_TIMER,    !HAS_TRIGGERED,   !STOPPED },
    { 0, "LoginPageLoaded",         IS_ONEOFF_TIMER,    !HAS_TRIGGERED,   !STOPPED },
};

//-----------------------------------------------------------------------------
//  PERFORMANCE: TIMERS: Stops a performance timer
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PERFORMANCE_TIMER_CLEAR(PerformanceTimerEnum area)
{
    EA_TRACE_FORMATTED(("**Timer %d cleared", area));
    TimerInfos[area].time = 0;
}

//-----------------------------------------------------------------------------
//  PERFORMANCE: TIMERS: Start of duration
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PERFORMANCE_TIMER_START(PerformanceTimerEnum area, uint64_t startTime)
{
    EA_TRACE_FORMATTED(("**Timer %d started", area));
    TimerInfos[area].time = (startTime != 0) ? (startTime / TimeDivisorMS) : (EA::StdC::GetTime() / TimeDivisorMS);
}

//-----------------------------------------------------------------------------
//  PERFORMANCE: TIMERS: Recorde the time
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PERFORMANCE_TIMER_STOP(PerformanceTimerEnum area)
{
    // Save the current count
    EA_TRACE_FORMATTED(("**Timer %d recorded", area));

    // if telemetry hasn't started or the specific timer hasn't been started, bail
    if ((TimerInfos[area].time == 0))
        return;

    TYPE_U64 currentTime = (EA::StdC::GetTime() / TimeDivisorMS);
    TYPE_U64 startTime = TimerInfos[area].time;
    TimerInfos[area].time = currentTime - startTime;
    TimerInfos[area].isStopped = true;
}

//-----------------------------------------------------------------------------
//  PERFORMANCE: TIMERS: End of duration
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PERFORMANCE_TIMER_END(PerformanceTimerEnum area, TYPE_U32 extraInfo, TYPE_U32 extraInfo2)
{
    EA_TRACE_FORMATTED(("**Timer %d ended", area));

    // if telemetry hasn't started or the specific timer hasn't been started, bail
    if (TimerInfos[area].time == 0)
        return;

    int64_t elapsedTime = 0;
    TYPE_U64 currentTime = (EA::StdC::GetTime() / TimeDivisorMS);
    TYPE_U64 startTime = 0;
    const char* areaString = NULL;

    if (!TimerInfos[area].isOneOff || !TimerInfos[area].hasTriggered)
    {
        areaString = TimerInfos[area].name;
        if(TimerInfos[area].isStopped == true)
            elapsedTime = TimerInfos[area].time;
        else
        {
            startTime = TimerInfos[area].time;
            if( startTime == 0 || startTime > currentTime)
            {
                //Send only if data is valid
                return;
            }
            elapsedTime = currentTime - startTime;
        }
        TimerInfos[area].hasTriggered = true;
        TimerInfos[area].isStopped = false;
    }

    //  Send only if data is valid
    if ( areaString == NULL )
        return;

    CaptureTelemetryData(METRIC_PERFORMANCE_TIMER, kMETRIC_PERFORMANCE_TIMER, kMETRIC_GROUP_PERFORMANCE,
        areaString,
        static_cast<int>(elapsedTime),
        extraInfo,
        extraInfo2,
        static_cast<int>(startTime / 1000),
        static_cast<int>(currentTime / 1000));
}

//-----------------------------------------------------------------------------
//  PERFORMANCE: TIMERS: Start Running Client Timer
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::RUNNING_TIMER_START(uint64_t startTime)
{
    if(!m_running_timer_started)
    {
        m_running_timer_started = true;
        EA_TRACE_MESSAGE("**Timer RunningClient started");
        m_running_time = (startTime != 0) ? (startTime / TimeDivisorMS) : (EA::StdC::GetTime() / TimeDivisorMS);
    }
}

//-----------------------------------------------------------------------------
//  PERFORMANCE: TIMERS: Stops a performance timer
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::RUNNING_TIMER_CLEAR()
{
    if (m_running_timer_started)
    {
        m_running_timer_started = false;
        EA_TRACE_MESSAGE("**Timer RunningClient cleared");
        m_running_time = 0;
    }
}

//-----------------------------------------------------------------------------
//  PERFORMANCE: TIMERS: Start Active Client Timer
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ACTIVE_TIMER_START()
{
    if(!m_active_timer_started)
    {
        m_active_timer_started = true;
        EA_TRACE_MESSAGE("**Timer ActiveClient started");
        m_active_time = EA::StdC::GetTime() / TimeDivisorMS;
    }
}

//-----------------------------------------------------------------------------
//  PERFORMANCE: TIMERS: End of duration
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ACTIVE_TIMER_END(bool closed)
{
    // if telemetry hasn't started or the specific timer hasn't been started, bail
    if (m_active_time == 0)
        return;

    uint64_t elapsedTime = 0;
    TYPE_U64 currentTime = (EA::StdC::GetTime() / TimeDivisorMS);
    TYPE_U64 startTime = m_active_time;

    elapsedTime = currentTime - startTime;

    m_total_active_time  += elapsedTime;

    // restart this timer here so we don't add more elaped time that needed
    m_active_time = EA::StdC::GetTime() / TimeDivisorMS;

    // Send telemetry only if the client is closing and data is valid
    if (closed && (m_total_active_time != 0))
    {
        EA_TRACE_MESSAGE("**Timer ActiveClient ended and reported");
        // set the this to false so we only get one telemetry sent.
        m_active_timer_started = false;
        m_active_time = 0;
        uint64_t active_report_time = m_total_active_time;
        m_total_active_time = 0;
        m_running_time = (m_running_time == 0) ? (0) : (currentTime - m_running_time);

        CaptureTelemetryData(METRIC_ACTIVE_CLIENT_TIMER, kMETRIC_ACTIVE_CLIENT_TIMER, kMETRIC_GROUP_PERFORMANCE,
            active_report_time,
            m_running_time);
    }
}

///////////////////////////////////////////////////////////////////////////////
//  GUI
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  GUI: My Games Play Source
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GUI_MYGAMES_PLAY_SOURCE(const TYPE_S8 product_id, const TYPE_S8 source)
{
    CaptureTelemetryData(METRIC_GUI_MYGAMES_PLAY_SOURCE, kMETRIC_GUI_MYGAMES_PLAY_SOURCE, kMETRIC_GROUP_GUI,
        product_id, source);
}


//-----------------------------------------------------------------------------
//  GUI:  My Games Details Page Viewed
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GUI_MYGAMESDETAILSPAGEVIEW(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_GUI_MYGAMESDETAILSPAGEVIEW, kMETRIC_GUI_MYGAMESDETAILSPAGEVIEW, kMETRIC_GROUP_GUI,
        product_id);
}

//-----------------------------------------------------------------------------
//  GUI:  My Games Cloud Storage Tab Viewed
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GUI_MYGAMES_CLOUD_STORAGE_TAB_VIEW(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_GUI_MYGAMES_CLOUD_STORAGE_TAB_VIEW, kMETRIC_GUI_MYGAMES_CLOUD_STORAGE_TAB_VIEW, kMETRIC_GROUP_GUI,
        product_id);
}

//-----------------------------------------------------------------------------
//  GUI:  My Games Manual Link Click
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GUI_MYGAMES_MANUAL_LINK_CLICK(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_GUI_MYGAMES_MANUAL_LINK_CLICK, kMETRIC_GUI_MYGAMES_MANUAL_LINK_CLICK, kMETRIC_GROUP_GUI,
        product_id);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GUI_MYGAMEVIEWSTATETOGGLE(int viewstate)
{
    CaptureTelemetryData(METRIC_GUI_MYGAMEVIEWSTATETOGGLE, kMETRIC_GUI_MYGAMEVIEWSTATETOGGLE, kMETRIC_GROUP_GUI,
        viewstate ? "list" : "grid");
}

//-----------------------------------------------------------------------------
//  GUI:  My Games View State Exit
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GUI_MYGAMEVIEWSTATEEXIT(int viewstate)
{
 

    CaptureTelemetryData(METRIC_GUI_MYGAMEVIEWSTATEEXIT, kMETRIC_GUI_MYGAMEVIEWSTATEEXIT, kMETRIC_GROUP_GUI,
        viewstate ? "list" : "grid");
}

//-----------------------------------------------------------------------------
//  GUI:  NAVIGATION:  Tab
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GUI_TAB_VIEW(const TYPE_S8 newtab, const TYPE_S8 init)
{
    CaptureTelemetryData(METRIC_GUI_TAB_VIEW, kMETRIC_GUI_TAB_VIEW, kMETRIC_GROUP_GUI, 
        newtab,
        init);
}

//-----------------------------------------------------------------------------
//  GUI:  NAVIGATION:  Application Settings
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GUI_SETTINGS_VIEW(GUISettingsViewEnum type)
{
    CaptureTelemetryData(METRIC_GUI_SETTINGS_VIEW, kMETRIC_GUI_SETTINGS_VIEW, kMETRIC_GROUP_GUI, 
        type);
}

//-----------------------------------------------------------------------------
//  GUI:  DETAILS INSTALL CLICKED
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GUI_DETAILS_UPDATE_CLICKED(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_GUI_DETAILS_UPDATE_CLICKED, kMETRIC_GUI_DETAILS_UPDATE_CLICKED, kMETRIC_GROUP_GUI, 
        product_id);
}

//-----------------------------------------------------------------------------
//  GUI:  MY GAMES HTTP REQUEST ERROR
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GUI_MYGAMES_HTTP_ERROR(const TYPE_I32 qnetworkerror, const TYPE_I32 httpstatus, const TYPE_I32 cachecontrol, const TYPE_S8 url)
{
    CaptureTelemetryData(METRIC_GUI_MYGAMES_HTTP_ERROR, kMETRIC_GUI_MYGAMES_HTTP_ERROR, kMETRIC_GROUP_GUI, 
        qnetworkerror,
        httpstatus,
        cachecontrol,
        url);
}

///////////////////////////////////////////////////////////////////////////////
//  ORIGIN DEVELOPER MODE
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  DEVELOPER MODE:  Activation Successful
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DEVMODE_ACTIVATE_SUCCESS(const TYPE_S8 tool_version, const TYPE_S8 platform)
{
    CaptureTelemetryData(METRIC_DEVMODE_ACTIVATE_SUCCESS, kMETRIC_DEVMODE_ACTIVATE_SUCCESS, kMETRIC_GROUP_DEVMODE,
        tool_version,
        platform);
}

//-----------------------------------------------------------------------------
//  DEVELOPER MODE:  Activation Error
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DEVMODE_ACTIVATE_ERROR(const TYPE_S8 tool_version, const TYPE_S8 platform, const TYPE_U32 error_code)
{
    CaptureTelemetryData(METRIC_DEVMODE_ACTIVATE_ERROR, kMETRIC_DEVMODE_ACTIVATE_ERROR, kMETRIC_GROUP_DEVMODE,
        tool_version,
        platform,
        error_code);
}

//-----------------------------------------------------------------------------
//  DEVELOPER MODE:  Deactivation Successful
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DEVMODE_DEACTIVATE_SUCCESS(const TYPE_S8 tool_version, const TYPE_S8 platform)
{
 

    CaptureTelemetryData(METRIC_DEVMODE_DEACTIVATE_SUCCESS, kMETRIC_DEVMODE_DEACTIVATE_SUCCESS, kMETRIC_GROUP_DEVMODE,
        tool_version,
        platform);
}

//-----------------------------------------------------------------------------
//  DEVELOPER MODE:  Deactivation Error
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DEVMODE_DEACTIVATE_ERROR(const TYPE_S8 tool_version, const TYPE_S8 platform, const TYPE_U32 error_code)
{
 

    CaptureTelemetryData(METRIC_DEVMODE_DEACTIVATE_ERROR, kMETRIC_DEVMODE_DEACTIVATE_ERROR, kMETRIC_GROUP_DEVMODE,
        tool_version,
        platform,
        error_code);
}


///////////////////////////////////////////////////////////////////////////////
//  NON ORIGIN GAMES
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  NON ORIGIN GAMES: game id at login
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_NON_ORIGIN_GAME_ID_LOGIN(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_NON_ORIGIN_GAME_ID_LOGIN, kMETRIC_NON_ORIGIN_GAME_ID_LOGIN, kMETRIC_GROUP_NON_ORIGIN_GAME, 
        product_id);
}

//-----------------------------------------------------------------------------
//  NON ORIGIN GAMES: game id at logout
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_NON_ORIGIN_GAME_ID_LOGOUT(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_NON_ORIGIN_GAME_ID_LOGOUT, kMETRIC_NON_ORIGIN_GAME_ID_LOGOUT, kMETRIC_GROUP_NON_ORIGIN_GAME, 
        product_id);
}

//-----------------------------------------------------------------------------
//  NON ORIGIN GAMES:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_NON_ORIGIN_GAME_COUNT_LOGIN(int total)
{
    m_start_non_origin_games = total;
    CaptureTelemetryData(METRIC_NON_ORIGIN_GAME_COUNT_LOGIN, kMETRIC_NON_ORIGIN_GAME_COUNT_LOGIN, kMETRIC_GROUP_NON_ORIGIN_GAME, 
        m_start_non_origin_games);
}

//-----------------------------------------------------------------------------
//  NON ORIGIN GAMES:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_NON_ORIGIN_GAME_COUNT_LOGOUT(int total)
{
    int delta = total - m_start_non_origin_games;
    if (delta >= 0)
    {
        CaptureTelemetryData(METRIC_NON_ORIGIN_GAME_COUNT_LOGOUT, kMETRIC_NON_ORIGIN_GAME_COUNT_LOGOUT, kMETRIC_GROUP_NON_ORIGIN_GAME, 
            m_start_non_origin_games, 
            total,
            delta);
    }
    else
    {
    
    }
}

//-----------------------------------------------------------------------------
//  NON ORIGIN GAMES:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_NON_ORIGIN_GAME_ADD(const TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_NON_ORIGIN_GAME_ADD, kMETRIC_NON_ORIGIN_GAME_ADD, kMETRIC_GROUP_NON_ORIGIN_GAME, 
        product_id);
}

//-----------------------------------------------------------------------------
//  NON ORIGIN GAMES:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CUSTOMIZE_BOXART_START(bool isNog)
{
    CaptureTelemetryData(METRIC_CUSTOMIZE_BOXART_START, kMETRIC_CUSTOMIZE_BOXART_START, kMETRIC_GROUP_NON_ORIGIN_GAME, 
        isNog);
}

//-----------------------------------------------------------------------------
//  NON ORIGIN GAMES:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CUSTOMIZE_BOXART_APPLY(bool isNog)
{
    CaptureTelemetryData(METRIC_CUSTOMIZE_BOXART_APPLY, kMETRIC_CUSTOMIZE_BOXART_APPLY, kMETRIC_GROUP_NON_ORIGIN_GAME,
        isNog);
}

//-----------------------------------------------------------------------------
//  NON ORIGIN GAMES:
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_CUSTOMIZE_BOXART_CANCEL(bool isNog)
{
    CaptureTelemetryData(METRIC_CUSTOMIZE_BOXART_CANCEL, kMETRIC_CUSTOMIZE_BOXART_CANCEL, kMETRIC_GROUP_NON_ORIGIN_GAME,
        isNog);
}

///////////////////////////////////////////////////////////////////////////////
//  SECURITY QUESTION
///////////////////////////////////////////////////////////////////////////////

void EbisuTelemetryAPI::Metric_SECURITY_QUESTION_SHOW()
{
    CaptureTelemetryData(METRIC_SECURITY_QUESTION_SHOW, kMETRIC_SECURITY_QUESTION_SHOW, kMETRIC_GROUP_USER);
}

void EbisuTelemetryAPI::Metric_SECURITY_QUESTION_SET()
{
    CaptureTelemetryData(METRIC_SECURITY_QUESTION_SET, kMETRIC_SECURITY_QUESTION_SET, kMETRIC_GROUP_USER);
}

void EbisuTelemetryAPI::Metric_SECURITY_QUESTION_CANCEL()
{
    CaptureTelemetryData(METRIC_SECURITY_QUESTION_CANCEL, kMETRIC_SECURITY_QUESTION_CANCEL, kMETRIC_GROUP_USER);
}

///////////////////////////////////////////////////////////////////////////////
//  ENTITLEMENTS
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  NO GAMES:  Go to store
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ENTITLEMENTS_NONE_GOTO_STORE(EntitlementNoneEnum e)
{
    CaptureTelemetryData(METRIC_ENTITLEMENTS_NONE_GOTO_STORE, kMETRIC_ENTITLEMENTS_NONE_GOTO_STORE, kMETRIC_GROUP_ENTITLEMENT, 
        e);
}

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  GAMES:  Count on Login
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ENTITLEMENT_GAME_COUNT_LOGIN(int games, int nogs)
{
    m_start_games = games;
    CaptureTelemetryData(METRIC_ENTITLEMENT_GAME_COUNT_LOGIN, kMETRIC_ENTITLEMENT_GAME_COUNT_LOGIN, kMETRIC_GROUP_ENTITLEMENT, 
        m_start_games,
        nogs);
}

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  GAMES:  Count on Logout
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ENTITLEMENT_GAME_COUNT_LOGOUT(int total, int games, int favs, int hidden, int nogs)
{

    // Negative "m_start_games" value means initial refresh failed and there was no cache to fall back on.  
    // If we recovered, we need to adjust our delta so it is not off by 1.
    int deltaGames = 0;
    if(m_start_games < 0 && games >= 0)
    {
        deltaGames = games;
    }
    else
    {
        deltaGames = games - m_start_games;
    }

    CaptureTelemetryData(METRIC_ENTITLEMENT_GAME_COUNT_LOGOUT, kMETRIC_ENTITLEMENT_GAME_COUNT_LOGOUT, kMETRIC_GROUP_ENTITLEMENT, 
        total,

        m_start_games,
        games,
        deltaGames,

        favs,
        hidden,
        nogs);
}

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  Hide game
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ENTITLEMENT_HIDE(TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_ENTITLEMENT_HIDE, kMETRIC_ENTITLEMENT_HIDE, kMETRIC_GROUP_ENTITLEMENT,
        product_id);
}

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  Unhide game
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ENTITLEMENT_UNHIDE(TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_ENTITLEMENT_UNHIDE, kMETRIC_ENTITLEMENT_UNHIDE, kMETRIC_GROUP_ENTITLEMENT,
        product_id);
}

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  Favourite game
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ENTITLEMENT_FAVORITE(TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_ENTITLEMENT_FAVORITE, kMETRIC_ENTITLEMENT_FAVORITE, kMETRIC_GROUP_ENTITLEMENT,
        product_id);
}

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  Unfavourite game
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ENTITLEMENT_UNFAVORITE(TYPE_S8 product_id)
{
    CaptureTelemetryData(METRIC_ENTITLEMENT_UNFAVORITE, kMETRIC_ENTITLEMENT_UNFAVORITE, kMETRIC_GROUP_ENTITLEMENT,
        product_id);
}

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  Reload
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ENTITLEMENT_RELOAD(const TYPE_S8 source)
{
    CaptureTelemetryData(METRIC_ENTITLEMENT_RELOAD, kMETRIC_ENTITLEMENT_RELOAD, kMETRIC_GROUP_ENTITLEMENT,
        source);
}

//-----------------------------------------------------------------------------
//  ENTITLEMENTS:  Free trial error
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ENTITLEMENT_FREE_TRIAL_ERROR(const TYPE_I32 restError, const TYPE_I32 httpStatus)
{
    CaptureTelemetryData(METRIC_ENTITLEMENT_FREE_TRIAL_ERROR, kMETRIC_ENTITLEMENT_FREE_TRIAL_ERROR, kMETRIC_GROUP_ENTITLEMENT,
        restError,
        httpStatus);
}

void EbisuTelemetryAPI::Metric_ENTITLEMENT_DIRTYBIT_NOTIFICATION()
{
    CaptureTelemetryData(METRIC_ENTITLEMENT_DIRTYBIT_NOTIFICATION, kMETRIC_ENTITLEMENT_DIRTYBIT_NOTIFICATION, kMETRIC_GROUP_ENTITLEMENT);
}

void EbisuTelemetryAPI::Metric_ENTITLEMENT_DIRTYBIT_REFRESH()
{
    CaptureTelemetryData(METRIC_ENTITLEMENT_DIRTYBIT_REFRESH, kMETRIC_ENTITLEMENT_DIRTYBIT_REFRESH, kMETRIC_GROUP_ENTITLEMENT);
}

void EbisuTelemetryAPI::Metric_ENTITLEMENT_CATALOG_UPDATE_DBIT_PARSE_ERROR()
{
    CaptureTelemetryData(METRIC_ENTITLEMENT_CATALOG_UPDATE_DBIT_PARSE_ERROR, kMETRIC_ENTITLEMENT_CATALOG_UPDATE_DBIT_PARSE_ERROR, kMETRIC_GROUP_ENTITLEMENT);
}

void EbisuTelemetryAPI::Metric_ENTITLEMENT_CATALOG_OFFER_UPDATE_DETECTED(const TYPE_S8 offerId, const TYPE_U64 catalogBatchTime)
{
    CaptureTelemetryData(METRIC_ENTITLEMENT_CATALOG_OFFER_UPDATE_DETECTED, kMETRIC_ENTITLEMENT_CATALOG_OFFER_UPDATE_DETECTED, kMETRIC_GROUP_ENTITLEMENT,
        catalogBatchTime,
        offerId);
}

void EbisuTelemetryAPI::Metric_ENTITLEMENT_CATALOG_MASTERTITLE_UPDATE_DETECTED(const TYPE_S8 masterTitleId, const TYPE_U64 catalogBatchTime)
{
    CaptureTelemetryData(METRIC_ENTITLEMENT_CATALOG_MASTERTITLE_UPDATE_DETECTED, kMETRIC_ENTITLEMENT_CATALOG_MASTERTITLE_UPDATE_DETECTED, kMETRIC_GROUP_ENTITLEMENT,
        catalogBatchTime,
        masterTitleId);
}

void EbisuTelemetryAPI::Metric_CATALOG_DEFINTION_LOOKUP_ERROR(const TYPE_S8 product_id, const TYPE_I32 qt_network_code, const TYPE_I32 http_code, const bool public_offer)
{
    CaptureTelemetryData(METRIC_CATALOG_DEFINTION_LOOKUP_ERROR, kMETRIC_CATALOG_DEFINTION_LOOKUP_ERROR, kMETRIC_GROUP_CATALOGDEFINITIONS,
        product_id,
        qt_network_code,
        http_code,
        public_offer ? 1 : 0);
}


void EbisuTelemetryAPI::Metric_CATALOG_DEFINTION_PARSE_ERROR(const TYPE_S8 product_id, const TYPE_S8 context, const bool from_cache, const bool public_offer, const TYPE_I32 code)
{
    CaptureTelemetryData(METRIC_CATALOG_DEFINTION_PARSE_ERROR, kMETRIC_CATALOG_DEFINTION_PARSE_ERROR, kMETRIC_GROUP_CATALOGDEFINITIONS,
        product_id,
        context,
        from_cache ? 1 : 0,
        public_offer ? 1: 0,
        code);
}

void EbisuTelemetryAPI::Metric_CATALOG_DEFINTION_CDN_LOOKUP_STATS(const TYPE_U32 successCount, const TYPE_U32 failureCount, const TYPE_U32 confidentialCount, const TYPE_U32 unchangedCount)
{
    CaptureTelemetryData(METRIC_CATALOG_DEFINTION_CDN_LOOKUP_STATS, kMETRIC_CATALOG_DEFINTION_CDN_LOOKUP_STATS, kMETRIC_GROUP_CATALOGDEFINITIONS,
        successCount,
        failureCount,
        confidentialCount,
        unchangedCount);
}

void EbisuTelemetryAPI::Metric_ENTITLEMENT_REFRESH_COMPLETED(const TYPE_U32 serverResponseTime, const TYPE_U32 entitlementCount, const TYPE_I32 entitlementDelta)
{

    uint32_t timeStampUTC = (uint32_t) (EA::StdC::GetTime() / UINT64_C(1000000000));    // in UTC seconds
    uint32_t deviation = labs(timeStampUTC - serverResponseTime);
    CaptureTelemetryData(METRIC_ENTITLEMENT_REFRESH_COMPLETED, kMETRIC_ENTITLEMENT_REFRESH_COMPLETED, kMETRIC_GROUP_ENTITLEMENT,
        serverResponseTime,
        entitlementCount,
        entitlementDelta,
        timeStampUTC,
        deviation);
}

void EbisuTelemetryAPI::Metric_ENTITLEMENT_REFRESH_ERROR(const TYPE_I32 restError, const TYPE_I32 httpStatus, const TYPE_I32 clientErrorCode)
{
    CaptureTelemetryData(METRIC_ENTITLEMENT_REFRESH_ERROR, kMETRIC_ENTITLEMENT_REFRESH_ERROR, kMETRIC_GROUP_ENTITLEMENT,
        restError,
        httpStatus,
        clientErrorCode);
}

void EbisuTelemetryAPI::Metric_ENTITLEMENT_EXTRACONTENT_REFRESH_COMPLETED(const TYPE_S8 masterTitleId, const TYPE_U32 serverResponseTime, const TYPE_U32 entitlementCount)
{
    CaptureTelemetryData(METRIC_ENTITLEMENT_EXTRACONTENT_REFRESH_COMPLETED, kMETRIC_ENTITLEMENT_EXTRACONTENT_REFRESH_COMPLETED, kMETRIC_GROUP_ENTITLEMENT,
        masterTitleId,
        serverResponseTime,
        entitlementCount);
}

void EbisuTelemetryAPI::Metric_ENTITLEMENT_EXTRACONTENT_REFRESH_ERROR(const TYPE_S8 masterTitleId, const TYPE_I32 restError, const TYPE_I32 httpStatus, const TYPE_I32 clientErrorCode)
{
    CaptureTelemetryData(METRIC_ENTITLEMENT_EXTRACONTENT_REFRESH_ERROR, kMETRIC_ENTITLEMENT_EXTRACONTENT_REFRESH_ERROR, kMETRIC_GROUP_ENTITLEMENT,
        masterTitleId,
        restError,
        httpStatus,
        clientErrorCode);
}
void EbisuTelemetryAPI::Metric_ENTITLEMENT_REFRESH_REQUEST_REFUSED(const TYPE_U32 requestDelta, const TYPE_U32 clientRefused)
{
    uint64_t currtime = EA::StdC::GetTime() / TimeDivisorMS;
    uint32_t sessionlen = static_cast<int>((currtime - m_start_time) / 1000); // seconds

    CaptureTelemetryData(METRIC_ENTITLEMENT_REFRESH_REQUEST_REFUSED, kMETRIC_ENTITLEMENT_REFRESH_REQUEST_REFUSED, kMETRIC_GROUP_ENTITLEMENT,
        requestDelta,
        clientRefused,
        sessionlen);
}

void EbisuTelemetryAPI::Metric_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK(const TYPE_S8 offerId, const TYPE_S8 installedVersion)
{

    CaptureTelemetryData(METRIC_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK, kMETRIC_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK, kMETRIC_GROUP_ENTITLEMENT,
        offerId,
        installedVersion);
}

void EbisuTelemetryAPI::Metric_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK_COMPLETED(const TYPE_S8 offerId, const TYPE_S8 installedVersion, const TYPE_S8 catalogVersion)
{
    CaptureTelemetryData(METRIC_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK_COMPLETED, kMETRIC_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK_COMPLETED, kMETRIC_GROUP_ENTITLEMENT,
        offerId,
        installedVersion,
        catalogVersion);
}

void EbisuTelemetryAPI::Metric_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK_TIMEOUT(const TYPE_S8 offerId, const TYPE_S8 installedVersion)
{

    CaptureTelemetryData(METRIC_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK_TIMEOUT, kMETRIC_ENTITLEMENT_OFFER_VERSION_UPDATE_CHECK_TIMEOUT, kMETRIC_GROUP_ENTITLEMENT,
        offerId,
        installedVersion);
}

///////////////////////////////////////////////////////////////////////////////
//  SONAR
///////////////////////////////////////////////////////////////////////////////

void EbisuTelemetryAPI::Metric_SONAR_MESSAGE(
    const TYPE_S8 category,
    const TYPE_S8 message)
{

    CaptureTelemetryData(
        METRIC_SONAR_MESSAGE,
        kMETRIC_SONAR_MESSAGE,
        kMETRIC_GROUP_SONAR,
        category,
        message);
}

void EbisuTelemetryAPI::Metric_SONAR_STAT(
    const TYPE_S8 category,
    const TYPE_S8 name,
    const TYPE_U32 value)
{

    CaptureTelemetryData(
        METRIC_SONAR_STAT,
        kMETRIC_SONAR_STAT,
        kMETRIC_GROUP_SONAR,
        category,
        name,
        value);
}

void EbisuTelemetryAPI::Metric_SONAR_ERROR(
    const TYPE_S8 category,
    const TYPE_S8 message,
    const TYPE_U32 error)
{

    CaptureTelemetryData(
        METRIC_SONAR_ERROR,
        kMETRIC_SONAR_ERROR,
        kMETRIC_GROUP_SONAR,
        category,
        message,
        error);
}

void EbisuTelemetryAPI::Metric_SONAR_CLIENT_CONNECTED(
    const TYPE_S8 channelId,
    const TYPE_S8 inputDeviceId,
    const TYPE_I8 inputDeviceAGC,
    const TYPE_I16 inputDeviceGain,
    const TYPE_I16 inputDeviceThreshold,
    const TYPE_S8 outputDeviceId,
    const TYPE_I16 outputDeviceGain,
    const TYPE_U32 serverIP,
    const TYPE_S8 captureMode)
{

    CaptureTelemetryData(
        METRIC_SONAR_CLIENT_CONNECTED, 
        kMETRIC_SONAR_CLIENT_CONNECTED, 
        kMETRIC_GROUP_SONAR, 
        channelId,
        inputDeviceId,
        inputDeviceAGC,
        inputDeviceGain,
        outputDeviceId,
        outputDeviceGain,
        serverIP,
        captureMode,
        inputDeviceThreshold);
}

void EbisuTelemetryAPI::Metric_SONAR_CLIENT_DISCONNECTED(
    const TYPE_S8 channelId,
    const TYPE_S8 reason,
    const TYPE_S8 reasonDesc,
    const TYPE_U32 messagesSent,
    const TYPE_U32 messagesReceived,
    const TYPE_U32 messagesLost,
    const TYPE_U32 messagesOutOfSequence,
    const TYPE_U32 badCid,
    const TYPE_U32 messagesTruncated,
    const TYPE_U32 audioFramesClippedInMixer,
    const TYPE_U32 socketSendError,
    const TYPE_U32 socketReceiveError,
    const TYPE_U32 receiveToPlaybackLatencyMean,
    const TYPE_U32 receiveToPlaybackLatencyMax,
    const TYPE_U32 playbackToPlaybackLatencyMean,
    const TYPE_U32 playbackToPlaybackLatencyMax,
    const TYPE_I32 jitterArrivalMean,
    const TYPE_I32 jitterArrivalMax,
    const TYPE_I32 jitterPlaybackMean,
    const TYPE_I32 jitterPlaybackMax,
    const TYPE_U32 playbackUnderrun,
    const TYPE_U32 playbackOverflow,
    const TYPE_U32 playbackDeviceLost,
    const TYPE_U32 dropNotConnected,
    const TYPE_U32 dropReadServerFrameCounter,
    const TYPE_U32 dropReadTakeNumber,
    const TYPE_U32 dropTakeStopped,
    const TYPE_U32 dropReadCid,
    const TYPE_U32 dropNullPayload,
    const TYPE_U32 dropReadClientFrameCounter,
    const TYPE_U32 jitterbufferUnderrun,
    const TYPE_U32 networkLoss,
    const TYPE_U32 networkJitter,
    const TYPE_U32 networkPing,
    const TYPE_U32 networkReceived,
    const TYPE_U32 networkQuality,
    const TYPE_U32 voiceServerIP,
    const TYPE_S8  captureMode,
    const TYPE_I32 jitterbufferOccupancyMean,
    const TYPE_I32 jitterbufferOccupancyMax
    )
{

    CaptureTelemetryData(
        METRIC_SONAR_CLIENT_DISCONNECTED,
        kMETRIC_SONAR_CLIENT_DISCONNECTED,
        kMETRIC_GROUP_SONAR,
        channelId,
        reason,
        reasonDesc,
        messagesSent,
        messagesReceived,
        messagesLost,
        messagesOutOfSequence,
        badCid,
        messagesTruncated,
        audioFramesClippedInMixer,
        socketSendError,
        socketReceiveError,
        receiveToPlaybackLatencyMean,
        receiveToPlaybackLatencyMax,
        playbackToPlaybackLatencyMean,
        playbackToPlaybackLatencyMax,
        jitterArrivalMean,
        jitterArrivalMax,
        jitterPlaybackMean,
        jitterPlaybackMax,
        playbackUnderrun,
        playbackOverflow,
        playbackDeviceLost,
        dropNotConnected,
        dropReadServerFrameCounter,
        dropReadTakeNumber,
        dropTakeStopped,
        dropReadCid,
        dropNullPayload,
        dropReadClientFrameCounter,
        jitterbufferUnderrun,
        networkLoss,
        networkJitter,
        networkPing,
        networkReceived,
        networkQuality,
        voiceServerIP,
        captureMode,
        jitterbufferOccupancyMean,
        jitterbufferOccupancyMax
        );
}

void EbisuTelemetryAPI::Metric_SONAR_CLIENT_STATS(
    const TYPE_S8 channelId,
    // Avg, max, min latency(RTT)
    const TYPE_U32 networkLatencyMin,
    const TYPE_U32 networkLatencyMax,
    const TYPE_U32 networkLatencyMean,
    // bandwidth(bytes received and bytes sent)
    const TYPE_U32 bytesSent,
    const TYPE_U32 bytesReceived,
    // packet loss
    const TYPE_U32 messagesLost,
    // Out of order messages received
    const TYPE_U32 messagesReceivedOutOfSequence,
    // jitter(between successively received packets)
    const TYPE_U32 messagesReceivedJitterMin,
    const TYPE_U32 messagesReceivedJitterMax,
    const TYPE_U32 messagesReceivedJitterMean,
    const TYPE_U32 cpuLoad
    ) {

    return CaptureTelemetryData(
        METRIC_SONAR_CLIENT_STATS,
        kMETRIC_SONAR_CLIENT_STATS,
        kMETRIC_GROUP_SONAR,
        m_sysInfo->GetClientVersion(),
        channelId,
        networkLatencyMin,
        networkLatencyMax,
        networkLatencyMean,
        bytesSent,
        bytesReceived,
        messagesLost,
        messagesReceivedOutOfSequence,
        messagesReceivedJitterMin,
        messagesReceivedJitterMax,
        messagesReceivedJitterMean,
        cpuLoad
        );
}

///////////////////////////////////////////////////////////////////////////////
//  PROMO MANAGER
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  PROMO MANAGER:  Start dialog
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PROMOMANAGER_START(const TYPE_S8 offer_id, const TYPE_S8 primary_context, const TYPE_S8 secondary_context)
{

    if (!primary_context) return;


    m_promoMgrDialogTime[primary_context] = (EA::StdC::GetTime() / TimeDivisorMS);

    CaptureTelemetryData(METRIC_PROMOMANAGER_START, kMETRIC_PROMOMANAGER_START, kMETRIC_GROUP_PROMOMANAGER,
        offer_id,
        primary_context,
        secondary_context);
}

//-----------------------------------------------------------------------------
//  PROMO MANAGER:  End dialog
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PROMOMANAGER_END(const TYPE_S8 offer_id, const TYPE_S8 primary_context, const TYPE_S8 secondary_context)
{
    if (!primary_context) return;

    PromoStartMap::const_iterator citer = m_promoMgrDialogTime.find(primary_context);

    if (citer == m_promoMgrDialogTime.end()) return;

    const uint64_t& promoStartTime = (*citer).second;

    //  Compute the duration that the dialog box was up
    int64_t currentTime = (EA::StdC::GetTime() / TimeDivisorMS);
    int32_t elapsedTime = (TYPE_I32) ((currentTime - promoStartTime) / 1000); // ms -> seconds
    if (elapsedTime < 0)
        elapsedTime = 0;

    CaptureTelemetryData(METRIC_PROMOMANAGER_END, kMETRIC_PROMOMANAGER_END, kMETRIC_GROUP_PROMOMANAGER,
        elapsedTime,
        static_cast<int>(promoStartTime / 1000),
        static_cast<int>(currentTime / 1000),
        offer_id,
        primary_context,
        secondary_context);
}

//-----------------------------------------------------------------------------
//  PROMO MANAGER:  User manually opened dialog
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PROMOMANAGER_MANUAL_OPEN(const TYPE_S8 offer_id, const TYPE_S8 primary_context, const TYPE_S8 secondary_context)
{
    CaptureTelemetryData(METRIC_PROMOMANAGER_MANUAL_OPEN, kMETRIC_PROMOMANAGER_MANUAL_OPEN, kMETRIC_GROUP_PROMOMANAGER,
        offer_id,
        primary_context,
        secondary_context);
}

//-----------------------------------------------------------------------------
//  PROMO MANAGER:  Result of manually opened dialog
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PROMOMANAGER_MANUAL_RESULT(const TYPE_U32 result, const TYPE_S8 offer_id, const TYPE_S8 primary_context, const TYPE_S8 secondary_context)
{
    CaptureTelemetryData(METRIC_PROMOMANAGER_MANUAL_RESULT, kMETRIC_PROMOMANAGER_MANUAL_RESULT, kMETRIC_GROUP_PROMOMANAGER,
        result,
        offer_id,
        primary_context,
        secondary_context);
}

//-----------------------------------------------------------------------------
//  PROMO MANAGER:  "View in store" button clicked from "no promo" window
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PROMOMANAGER_MANUAL_VIEW_IN_STORE(const TYPE_S8 offer_id, const TYPE_S8 primary_context, const TYPE_S8 secondary_context)
{
    CaptureTelemetryData(METRIC_PROMOMANAGER_MANUAL_VIEW_IN_STORE, kMETRIC_PROMOMANAGER_MANUAL_VIEW_IN_STORE, kMETRIC_GROUP_PROMOMANAGER,
        offer_id,
        primary_context,
        secondary_context);
}

//-----------------------------------------------------------------------------
//  PROMO MANAGER:  On The House banner was loaded in client
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_OTH_BANNER_LOADED()
{
    CaptureTelemetryData(METRIC_OTH_BANNER_LOADED, kMETRIC_OTH_BANNER_LOADED, kMETRIC_GROUP_PROMOMANAGER);
}


///////////////////////////////////////////////////////////////////////////////
//  SUBSCRIPTION INFO
///////////////////////////////////////////////////////////////////////////////
void EbisuTelemetryAPI::Metric_SUBSCRIPTION_INFO(const TYPE_I32 isTrial, const TYPE_S8 subscription_id, const TYPE_S8 machine_id)
{
    return CaptureTelemetryData(METRIC_SUBSCRIPTION_INFO, kMETRIC_SUBSCRIPTION_INFO, kMETRIC_GROUP_SUBSCRIPTION,
                                isTrial,
                                subscription_id,
                                machine_id);
}

///////////////////////////////////////////////////////////////////////////////
//  GREY MARKET
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  GREY MARKET:  Language Selection
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GREYMARKET_LANGUAGE_SELECTION(const TYPE_S8 offerID, const TYPE_S8 langSelection, const TYPE_S8 langList, const TYPE_S8 manifestLangList)
{

    CaptureTelemetryData(
        METRIC_GREYMARKET_LANGUAGE_SELECTION, 
        kMETRIC_GREYMARKET_LANGUAGE_SELECTION, 
        kMETRIC_GROUP_GREYMARKET,
        offerID,
        langSelection,
        langList,
        manifestLangList);
}

//-----------------------------------------------------------------------------
//  GREY MARKET:  Language Selection for Bypass Content
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GREYMARKET_LANGUAGE_SELECTION_BYPASS(const TYPE_S8 offerID, const TYPE_S8 langSelection, const TYPE_S8 langList, const TYPE_S8 manifestLangList)
{

    CaptureTelemetryData(
        METRIC_GREYMARKET_LANGUAGE_SELECTION_BYPASS, 
        kMETRIC_GREYMARKET_LANGUAGE_SELECTION_BYPASS, 
        kMETRIC_GROUP_GREYMARKET,
        offerID,
        langSelection,
        langList,
        manifestLangList);
}

//-----------------------------------------------------------------------------
//  GREY MARKET:  Bypass filter
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GREYMARKET_BYPASS_FILTER(const TYPE_S8 offerID, const TYPE_S8 langList, const TYPE_S8 manifestLangList)
{

    CaptureTelemetryData(
        METRIC_GREYMARKET_BYPASS_FILTER, 
        kMETRIC_GREYMARKET_BYPASS_FILTER, 
        kMETRIC_GROUP_GREYMARKET,
        offerID,
        langList,
        manifestLangList);
}

//-----------------------------------------------------------------------------
//  GREY MARKET:  Language Selection Error
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GREYMARKET_LANGUAGE_SELECTION_ERROR(const TYPE_S8 offerID, const TYPE_S8 availableLanguages, const TYPE_S8 manifestLanguages)
{

    CaptureTelemetryData(
        METRIC_GREYMARKET_LANGUAGE_SELECTION_ERROR, 
        kMETRIC_GREYMARKET_LANGUAGE_SELECTION_ERROR, 
        kMETRIC_GROUP_GREYMARKET,
        offerID,
        availableLanguages,
        manifestLanguages);
}

//-----------------------------------------------------------------------------
//  GREY MARKET:  Available Languages List
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GREYMARKET_LANGUAGE_LIST(const TYPE_S8 offerID, const TYPE_S8 availableLanguages)
{

    CaptureTelemetryData(
        METRIC_GREYMARKET_LANGUAGE_LIST, 
        kMETRIC_GREYMARKET_LANGUAGE_LIST, 
        kMETRIC_GROUP_GREYMARKET,
        offerID,
        availableLanguages,
        "sdk");                                      // Requests for list of available languages only come from Origin SDK (for now)
}


///////////////////////////////////////////////////////////////////////////////
//  NET PROMOTER SCORE:
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  NET PROMOTER SCORE: return results
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_NET_PROMOTER_SCORE(const TYPE_S8 score, const TYPE_S8 feedback, const TYPE_S8 locale, bool canContact)
{
#if defined(WIN32) || defined(_WIN32)
    TYPE_S8 platform = "pc";
#elif defined(__APPLE__)
    const TYPE_S8 platform = "mac";
#elif defined(__linux__)
    const TYPE_S8 platform = "linux";
#endif


    CaptureTelemetryData(
        METRIC_NETPROMOTER_RESULT, 
        kMETRIC_NETPROMOTER_RESULT, 
        kMETRIC_GROUP_USER,
        score,
        platform,
        feedback,
        locale,
        canContact);
}

//-----------------------------------------------------------------------------
//  NET PROMOTER GAME SCORE: return results
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_NET_PROMOTER_GAME_SCORE(const TYPE_S8 offerId, const TYPE_S8 score, const TYPE_S8 feedback, const TYPE_S8 locale, bool canContact)
{
#if defined(WIN32) || defined(_WIN32)
    TYPE_S8 platform = "pc";
#endif
#if defined(__APPLE__)
    const TYPE_S8 platform = "mac";
#endif

    CaptureTelemetryData(
        METRIC_NETPROMOTER_GAME_RESULT, 
        kMETRIC_NETPROMOTER_GAME_RESULT, 
        kMETRIC_GROUP_USER,
        offerId,
        score,
        platform,
        feedback,
        locale,
        canContact);
}


///////////////////////////////////////////////////////////////////////////////
//  BROADCAST
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  BROADCAST:  Streaming Started
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_BROADCAST_STREAM_START(const TYPE_S8 product_id, const TYPE_S8 channelid, const TYPE_U64 streamid, const TYPE_U16 fromSDK, const TYPE_S8 settings)
{
    CaptureTelemetryData(
        METRIC_BROADCAST_STREAM_START, 
        kMETRIC_BROADCAST_STREAM_START, 
        kMETRIC_GROUP_BROADCAST,
        product_id,
        channelid,
        streamid,
        fromSDK,
        settings);
}

//-----------------------------------------------------------------------------
//  BROADCAST:  Streaming Ended
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_BROADCAST_STREAM_STOP(const TYPE_S8 product_id, const TYPE_S8 channelid, const TYPE_U64 streamid, const TYPE_U32 minViewers, const TYPE_U32 maxViewers, const TYPE_U32 duration)
{
    CaptureTelemetryData(
        METRIC_BROADCAST_STREAM_STOP, 
        kMETRIC_BROADCAST_STREAM_STOP, 
        kMETRIC_GROUP_BROADCAST,
        product_id,
        channelid,
        streamid,
        minViewers,
        maxViewers,
        duration);
}

//-----------------------------------------------------------------------------
//  BROADCAST:  Account Linked
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_BROADCAST_ACCOUNT_LINKED(const TYPE_U16 fromSDK)
{
    CaptureTelemetryData(
        METRIC_BROADCAST_ACCOUNT_LINKED, 
        kMETRIC_BROADCAST_ACCOUNT_LINKED, 
        kMETRIC_GROUP_BROADCAST,
        fromSDK);
}

//-----------------------------------------------------------------------------
//  BROADCAST:  Streaming Error
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_BROADCAST_STREAM_ERROR(const TYPE_S8 product_id, const TYPE_S8 error_reason, const TYPE_S8 error_code, const TYPE_S8 settings)
{
    CaptureTelemetryData(METRIC_BROADCAST_STREAM_ERROR, kMETRIC_BROADCAST_STREAM_ERROR, kMETRIC_GROUP_BROADCAST,
        product_id,
        error_reason, 
        error_code,
        settings);
}

//-----------------------------------------------------------------------------
//  BROADCAST:  Joined
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_BROADCAST_JOINED(const TYPE_S8 source)
{
    CaptureTelemetryData(METRIC_BROADCAST_JOINED, kMETRIC_BROADCAST_JOINED, kMETRIC_GROUP_BROADCAST, 
        source);
}


///////////////////////////////////////////////////////////////////////////////
//  ACHIEVEMENTS
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Post Success
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ACHIEVEMENT_ACH_POST_SUCCESS(const TYPE_S8 product_id, const TYPE_S8 achievement_set_id, const TYPE_S8 achievementId)
{
    CaptureTelemetryData(
        METRIC_ACHIEVEMENT_ACH_POST_SUCCESS, 
        kMETRIC_ACHIEVEMENT_ACH_POST_SUCCESS, 
        kMETRIC_GROUP_ACHIEVEMENT,
        product_id,
        achievement_set_id,
        achievementId);
}

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Post Fail
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ACHIEVEMENT_ACH_POST_FAIL(const TYPE_S8 product_id, const TYPE_S8 achievement_set_id, const TYPE_S8 achievementId, const TYPE_I32 code, const TYPE_S8 reason)
{
    CaptureTelemetryData(
        METRIC_ACHIEVEMENT_ACH_POST_FAIL, 
        kMETRIC_ACHIEVEMENT_ACH_POST_FAIL, 
        kMETRIC_GROUP_ACHIEVEMENT,
        product_id,
        achievement_set_id,
        achievementId,
        code,
        reason);
}

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Post Fail
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ACHIEVEMENT_WC_POST_FAIL(const TYPE_S8 product_id, const TYPE_S8 achievement_set_id, const TYPE_S8 wincode, const TYPE_I32 code, const TYPE_S8 reason)
{
    CaptureTelemetryData(
        METRIC_ACHIEVEMENT_WC_POST_FAIL,
        kMETRIC_ACHIEVEMENT_WC_POST_FAIL, 
        kMETRIC_GROUP_ACHIEVEMENT,
        product_id,
        achievement_set_id,
        wincode,
        code,
        reason);
}

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Wincode Post Fail
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ACHIEVEMENT_WIDGET_PAGE_VIEW(const TYPE_S8 page_name, const TYPE_S8 product_id, bool in_igo) 
{
    CaptureTelemetryData(
        METRIC_ACHIEVEMENT_WIDGET_PAGE_VIEW,
        kMETRIC_ACHIEVEMENT_WIDGET_PAGE_VIEW, 
        kMETRIC_GROUP_ACHIEVEMENT,
        page_name,
        product_id,
        in_igo);
}

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Purchase click
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ACHIEVEMENT_WIDGET_PURCHASE_CLICK(const TYPE_S8 product_id)
{
    CaptureTelemetryData(
        METRIC_ACHIEVEMENT_WIDGET_PURCHASE_CLICK,
        kMETRIC_ACHIEVEMENT_WIDGET_PURCHASE_CLICK, 
        kMETRIC_GROUP_ACHIEVEMENT,
        product_id);
}

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Achievement Achieved XP/RP
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ACHIEVEMENT_ACHIEVED(TYPE_I32 achieved, TYPE_I32 xp, TYPE_I32 rp)
{
    CaptureTelemetryData(
        METRIC_ACHIEVEMENT_ACHIEVED,
        kMETRIC_ACHIEVEMENT_ACHIEVED,
        kMETRIC_GROUP_ACHIEVEMENT,
        achieved,
        xp,
        rp);
}

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Achievement Achieved per Game XP/RP
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ACHIEVEMENT_ACHIEVED_PER_GAME(const TYPE_S8 productId, const TYPE_S8 achievementSetId, TYPE_I32 achieved, TYPE_I32 xp, TYPE_I32 rp)
{
    CaptureTelemetryData(
        METRIC_ACHIEVEMENT_ACHIEVED_PER_GAME,
        kMETRIC_ACHIEVEMENT_ACHIEVED_PER_GAME,
        kMETRIC_GROUP_ACHIEVEMENT,
        productId,
        achievementSetId,
        achieved,
        xp,
        rp);
}

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Grant Detected
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ACHIEVEMENT_GRANT_DETECTED(const TYPE_S8 productId, const TYPE_S8 achievementSetId, const TYPE_S8 achievementId)
{
    CaptureTelemetryData(
        METRIC_ACHIEVEMENT_GRANT_DETECTED,
        kMETRIC_ACHIEVEMENT_GRANT_DETECTED,
        kMETRIC_GROUP_ACHIEVEMENT,
        productId,
        achievementSetId,
        achievementId);
}

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Share achievement show
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ACHIEVEMENT_SHARE_ACHIEVEMENT_SHOW(const TYPE_S8 source)
{
    CaptureTelemetryData(
        METRIC_ACHIEVEMENT_SHARE_ACHIEVEMENT_SHOW, 
        kMETRIC_ACHIEVEMENT_SHARE_ACHIEVEMENT_SHOW, 
        kMETRIC_GROUP_ACHIEVEMENT,
        source);
}

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Share achievement dismissed
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ACHIEVEMENT_SHARE_ACHIEVEMENT_DISMISSED(const TYPE_S8 source)
{
    CaptureTelemetryData(
        METRIC_ACHIEVEMENT_SHARE_ACHIEVEMENT_DISMISSED, 
        kMETRIC_ACHIEVEMENT_SHARE_ACHIEVEMENT_DISMISSED, 
        kMETRIC_GROUP_ACHIEVEMENT,
        source);
}

//-----------------------------------------------------------------------------
//  ACHIEVEMENTS:  Comparison View
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ACHIEVEMENT_COMPARISON_VIEW(const TYPE_S8 product_id, bool user_owns_product)
{
    CaptureTelemetryData(
        METRIC_ACHIEVEMENT_COMPARISON_VIEW,
        kMETRIC_ACHIEVEMENT_COMPARISON_VIEW, 
        kMETRIC_GROUP_ACHIEVEMENT,
        product_id,
        user_owns_product ? 1 : 0);
}


///////////////////////////////////////////////////////////////////////////////
//  DIRTY BITS
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  DIRTY BITS Network connected
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DIRTYBITS_NETWORK_CONNECTED()
{
    CaptureTelemetryData(METRIC_DIRTYBITS_NETWORK_CONNECTED
        , kMETRIC_DIRTYBITS_NETWORK_CONNECTED
        , kMETRIC_GROUP_DIRTYBITS);
}

//-----------------------------------------------------------------------------
//  DIRTY BITS Network disconnected
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DIRTYBITS_NETWORK_DISCONNECTED(const TYPE_S8 reason)
{
    CaptureTelemetryData(METRIC_DIRTYBITS_NETWORK_DISCONNECTED
        , kMETRIC_DIRTYBITS_NETWORK_DISCONNECTED
        , kMETRIC_GROUP_DIRTYBITS,
        reason);
}

//-----------------------------------------------------------------------------
//  DIRTY BITS Message counter
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DIRTYBITS_MESSAGE_COUNTER(const TYPE_S8 totl, const TYPE_S8 emal, const TYPE_S8 orid, const TYPE_S8 pass, const TYPE_S8 dblc, const TYPE_S8 achi, const TYPE_S8 avat, const TYPE_S8 entl, const TYPE_S8 catl, const TYPE_S8 unkn, const TYPE_S8 dupl)
{
    CaptureTelemetryData(METRIC_DIRTYBITS_MESSAGE_COUNTER
        , kMETRIC_DIRTYBITS_MESSAGE_COUNTER
        , kMETRIC_GROUP_DIRTYBITS
        ,totl
        ,emal
        ,orid
        ,pass
        ,dblc
        ,achi
        ,avat
        ,entl
        ,catl
        ,unkn
        ,dupl);
}

//-----------------------------------------------------------------------------
//  DIRTY BITS Retries capped
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DIRTYBITS_RETRIES_CAPPED(const TYPE_S8 tout)
{
    CaptureTelemetryData(METRIC_DIRTYBITS_RETRIES_CAPPED
        , kMETRIC_DIRTYBITS_RETRIES_CAPPED
        , kMETRIC_GROUP_DIRTYBITS,
        tout);
}

//-----------------------------------------------------------------------------
//  DIRTY BITS Sudden disconnection
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_DIRTYBITS_NETWORK_SUDDEN_DISCONNECTION(const TYPE_I64 tcon)
{
    CaptureTelemetryData(METRIC_DIRTYBITS_NETWORK_SUDDEN_DISCONNECTION
        , kMETRIC_DIRTYBITS_NETWORK_SUDDEN_DISCONNECTION
        , kMETRIC_GROUP_DIRTYBITS,
        tcon);
}

///////////////////////////////////////////////////////////////////////////////
//  GAME PROPERTIES
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  GAME PROPERTIES: apply changes
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GAMEPROPERTIES_APPLY_CHANGES(bool is_nog, const TYPE_S8 product_id, const TYPE_S8 install_path, const TYPE_S8 command_args, bool disable_igo)
{
    CaptureTelemetryData(
        METRIC_GAMEPROPERTIES_APPLY_CHANGES, 
        kMETRIC_GAMEPROPERTIES_APPLY_CHANGES,
        kMETRIC_GROUP_GAMEPROPERTIES,
        is_nog,
        product_id,
        install_path,
        command_args,
        disable_igo);
}

void EbisuTelemetryAPI::Metric_GAMEPROPERTIES_LANG_CHANGES(const TYPE_S8 product_id, const TYPE_S8 locale)
{
    return CaptureTelemetryData(
        METRIC_GAMEPROPERTIES_LANG_CHANGES, 
        kMETRIC_GAMEPROPERTIES_LANG_CHANGES,
        kMETRIC_GROUP_GAMEPROPERTIES,
        product_id,
        locale);
}

void EbisuTelemetryAPI::Metric_GAMEPROPERTIES_AUTO_DOWNLOAD_EXTRA_CONTENT(const TYPE_S8 masterTitleId, const bool& enabled, const TYPE_S8 context)
{
    return CaptureTelemetryData(
        METRIC_GAMEPROPERTIES_AUTO_DOWNLOAD_EXTRA_CONTENT, 
        kMETRIC_GAMEPROPERTIES_AUTO_DOWNLOAD_EXTRA_CONTENT,
        kMETRIC_GROUP_GAMEPROPERTIES,
        masterTitleId,
        enabled ? 1: 0,
        context);
}


//-----------------------------------------------------------------------------
//  GAME PROPERTIES: apply changes
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GAMEPROPERTIES_UPDATE_COUNT_LOGOUT(TYPE_I32 update_count)
{
    CaptureTelemetryData(
        METRIC_GAMEPROPERTIES_UPDATE_COUNT_LOGOUT, 
        kMETRIC_GAMEPROPERTIES_UPDATE_COUNT_LOGOUT,
        kMETRIC_GROUP_GAMEPROPERTIES,
        update_count);
}

//-----------------------------------------------------------------------------
//  GAME PROPERTIES: add on description expanded
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GAMEPROPERTIES_ADDON_DESCR_EXPANDED(const TYPE_S8 addon_id)
{
    CaptureTelemetryData(
        METRIC_GAMEPROPERTIES_ADDON_DESCR_EXPANDED, 
        kMETRIC_GAMEPROPERTIES_ADDON_DESCR_EXPANDED,
        kMETRIC_GROUP_GAMEPROPERTIES,
        addon_id);
}

//-----------------------------------------------------------------------------
//  GAME PROPERTIES: add on buy click
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_GAMEPROPERTIES_ADDON_BUY_CLICK(const TYPE_S8 addon_id)
{
    CaptureTelemetryData(
        METRIC_GAMEPROPERTIES_ADDON_BUY_CLICK, 
        kMETRIC_GAMEPROPERTIES_ADDON_BUY_CLICK,
        kMETRIC_GROUP_GAMEPROPERTIES,
        addon_id);
}

///////////////////////////////////////////////////////////////////////////////
//  JUMP LIST
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  JUMP LIST: return what user selected
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_JUMPLIST_ACTION(const TYPE_S8 type, const TYPE_S8 source, const TYPE_S8 extra)
{
    CaptureTelemetryData(
        METRIC_JUMPLIST_ACTION, 
        kMETRIC_JUMPLIST_ACTION, 
        kMETRIC_GROUP_GUI,
        type,
        source,
        extra);
}


///////////////////////////////////////////////////////////////////////////////
//  SDK
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  SDK:  Sends hook with the version of game's SDK
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ORIGIN_SDK_VERSION_CONNECTED(TYPE_S8 product_id, TYPE_S8 sdk_version)
{
    CaptureTelemetryData(
        METRIC_SDK_VERSION, 
        kMETRIC_SDK_VERSION, 
        kMETRIC_GROUP_GUI,
        product_id,
        sdk_version);
}


//-----------------------------------------------------------------------------
//  SDK:  Client requested entitlements without filtering
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ORIGIN_SDK_UNFILTERED_ENTITLEMENTS_REQUEST(TYPE_S8 product_id, TYPE_S8 sdk_version)
{
    CaptureTelemetryData(
        METRIC_SDK_UNFILTERED_ENTITLEMENTS_REQUEST, 
        kMETRIC_SDK_UNFILTERED_ENTITLEMENTS_REQUEST, 
        kMETRIC_GROUP_SDK,
        product_id,
        sdk_version);
}


///////////////////////////////////////////////////////////////////////////////
//  WEB WIDGETS
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  WEB WIDGETS
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_WEBWIDGET_DL_START(TYPE_S8 etag)
{
    CaptureTelemetryData(
        METRIC_WEBWIDGET_DL_START, 
        kMETRIC_WEBWIDGET_DL_START, 
        kMETRIC_GROUP_GUI,
        etag);
}

//-----------------------------------------------------------------------------
//  WEB WIDGETS
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_WEBWIDGET_DL_SUCCESS(TYPE_S8 etag, TYPE_U32 duration)
{
    CaptureTelemetryData(
        METRIC_WEBWIDGET_DL_SUCCESS, 
        kMETRIC_WEBWIDGET_DL_SUCCESS, 
        kMETRIC_GROUP_GUI,
        etag,
        duration);
}

//-----------------------------------------------------------------------------
//  WEB WIDGETS
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_WEBWIDGET_DL_ERROR(TYPE_S8 etag, TYPE_I32 error_code)
{
    CaptureTelemetryData(
        METRIC_WEBWIDGET_DL_ERROR, 
        kMETRIC_WEBWIDGET_DL_ERROR, 
        kMETRIC_GROUP_GUI,
        etag,
        error_code);
}

//-----------------------------------------------------------------------------
//  WEB WIDGETS
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_WEBWIDGET_LOADED(TYPE_S8 widget_name, TYPE_S8 current_version)
{
    CaptureTelemetryData(
        METRIC_WEBWIDGET_LOADED, 
        kMETRIC_WEBWIDGET_LOADED, 
        kMETRIC_GROUP_GUI,
        widget_name,
        current_version);
}


///////////////////////////////////////////////////////////////////////////////
//  ORIGIN DEVELOPER TOOL (ODT)
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  ODT: ODT is launched
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ODT_LAUNCH()
{
    CaptureTelemetryData(
        METRIC_ODT_LAUNCH, 
        kMETRIC_ODT_LAUNCH, 
        kMETRIC_GROUP_ODT);
}

//-----------------------------------------------------------------------------
//  ODT: Apply Changes button is pressed
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ODT_APPLY_CHANGES()
{
    CaptureTelemetryData(
        METRIC_ODT_APPLY_CHANGES, 
        kMETRIC_ODT_APPLY_CHANGES, 
        kMETRIC_GROUP_ODT);
}

//-----------------------------------------------------------------------------
//  ODT: An override setting was modified
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ODT_OVERRIDE_MODIFIED(const TYPE_S8 section, const TYPE_S8 entry, const TYPE_S8 value)
{
    CaptureTelemetryData(
        METRIC_ODT_OVERRIDE_MODIFIED,
        kMETRIC_ODT_OVERRIDE_MODIFIED,
        kMETRIC_GROUP_ODT,
        section,
        entry,
        value);
}

//-----------------------------------------------------------------------------
//  ODT: A button was pressed in ODT
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ODT_BUTTON_PRESSED(const TYPE_S8 button)
{
    CaptureTelemetryData(
        METRIC_ODT_BUTTON_PRESSED,
        kMETRIC_ODT_BUTTON_PRESSED,
        kMETRIC_GROUP_ODT,
        button);
}

//-----------------------------------------------------------------------------
//  ODT:  NAVIGATION:  Open url in browser
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ODT_NAVIGATE_URL(const TYPE_S8 url)
{
    CaptureTelemetryData(
        METRIC_ODT_NAVIGATE_URL,
        kMETRIC_ODT_NAVIGATE_URL,
        kMETRIC_GROUP_ODT,
        url);
}

//-----------------------------------------------------------------------------
//  ODT: The client is being restarted to reload settings
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ODT_RESTART_CLIENT()
{
    CaptureTelemetryData(
        METRIC_ODT_RESTART_CLIENT,
        kMETRIC_ODT_RESTART_CLIENT,
        kMETRIC_GROUP_ODT);
}

//-----------------------------------------------------------------------------
//  ODT: The telemetry viewer was opened
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ODT_TELEMETRY_VIEWER_OPENED()
{
    CaptureTelemetryData(
        METRIC_ODT_TELEMETRY_VIEWER_OPENED,
        kMETRIC_ODT_TELEMETRY_VIEWER_OPENED,
        kMETRIC_GROUP_ODT);
}

//-----------------------------------------------------------------------------
//  ODT: The telemetry viewer was closed
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_ODT_TELEMETRY_VIEWER_CLOSED()
{
    CaptureTelemetryData(
        METRIC_ODT_TELEMETRY_VIEWER_CLOSED,
        kMETRIC_ODT_TELEMETRY_VIEWER_CLOSED,
        kMETRIC_GROUP_ODT);
}


///////////////////////////////////////////////////////////////////////////////
//  LOCALHOST
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  LOCALHOST: SSO via local host started
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_LOCALHOST_AUTH_SSO_START(const TYPE_S8 authType, const TYPE_S8 uuid, const TYPE_S8 origin)
{
    EA::Thread::AutoFutex m2(loginSession->GetLock());
    loginSession->setStartedByLocalHost(true);
    loginSession->setLocalHostOriginUri(origin);

    CaptureTelemetryData(
        METRIC_LOCALHOST_AUTH_SSO_START, 
        kMETRIC_LOCALHOST_AUTH_SSO_START, 
        kMETRIC_GROUP_LOCALHOST,
        authType,
        origin,
        uuid);
}

//-----------------------------------------------------------------------------
//  LOCALHOST: Server started event
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_LOCALHOST_SERVER_START(const TYPE_U32 port,  const TYPE_U32 success)
{
    CaptureTelemetryData(
        METRIC_LOCALHOST_SERVER_START, 
        kMETRIC_LOCALHOST_SERVER_START, 
        kMETRIC_GROUP_LOCALHOST,
        port,
        success);
}

//-----------------------------------------------------------------------------
//  LOCALHOST: Server started event
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_LOCALHOST_SERVER_STOP()
{
    CaptureTelemetryData(
        METRIC_LOCALHOST_SERVER_STOP, 
        kMETRIC_LOCALHOST_SERVER_STOP, 
        kMETRIC_GROUP_LOCALHOST);
}

//-----------------------------------------------------------------------------
//  LOCALHOST: Server disabled
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_LOCALHOST_SERVER_DISABLED()
{
    CaptureTelemetryData(
        METRIC_LOCALHOST_SERVER_DISABLED, 
        kMETRIC_LOCALHOST_SERVER_DISABLED, 
        kMETRIC_GROUP_LOCALHOST);
}


//-----------------------------------------------------------------------------
//  LOCALHOST: Localhost service security failure
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_LOCALHOST_SECURITY_FAILURE(const TYPE_S8 reason)
{
    CaptureTelemetryData(
        METRIC_LOCALHOST_SECURITY_FAILURE, 
        kMETRIC_LOCALHOST_SECURITY_FAILURE, 
        kMETRIC_GROUP_LOCALHOST,
        reason
        );
}
///////////////////////////////////////////////////////////////////////////////////////////
// HOVERCARD
///////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------
// HOVERCARD: Buy Now Click
//---------------------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HOVERCARD_BUYNOW_CLICK(const TYPE_S8 gameid)
{
    CaptureTelemetryData(
        METRIC_HOVERCARD_BUYNOW_CLICK, 
        kMETRIC_HOVERCARD_BUYNOW_CLICK, 
        kMETRIC_GROUP_HOVERCARD,
        gameid);
}

//---------------------------------------------------------------------------------------
// HOVERCARD: Buy Click
//---------------------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HOVERCARD_BUY_CLICK(const TYPE_S8 gameid)
{
    CaptureTelemetryData(
        METRIC_HOVERCARD_BUY_CLICK, 
        kMETRIC_HOVERCARD_BUY_CLICK, 
        kMETRIC_GROUP_HOVERCARD,
        gameid);
}
//---------------------------------------------------------------------------------------
// HOVERCARD: Download Click
//---------------------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HOVERCARD_DOWNLOAD_CLICK(const TYPE_S8 gameid)
{

    CaptureTelemetryData(
        METRIC_HOVERCARD_DOWNLOAD_CLICK, 
        kMETRIC_HOVERCARD_DOWNLOAD_CLICK, 
        kMETRIC_GROUP_HOVERCARD,
        gameid);
}
//---------------------------------------------------------------------------------------
// HOVERCARD: Details Click
//---------------------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HOVERCARD_DETAILS_CLICK(const TYPE_S8 gameid)
{

    CaptureTelemetryData(
        METRIC_HOVERCARD_DETAILS_CLICK, 
        kMETRIC_HOVERCARD_DETAILS_CLICK, 
        kMETRIC_GROUP_HOVERCARD,
        gameid);
}
//---------------------------------------------------------------------------------------
// HOVERCARD: Achievements Click
//---------------------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HOVERCARD_ACHIEVEMENTS_CLICK(const TYPE_S8 gameid)
{

    CaptureTelemetryData(
        METRIC_HOVERCARD_ACHIEVEMENTS_CLICK, 
        kMETRIC_HOVERCARD_ACHIEVEMENTS_CLICK, 
        kMETRIC_GROUP_HOVERCARD,
        gameid);
}

//---------------------------------------------------------------------------------------
// HOVERCARD: Update Click
//---------------------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_HOVERCARD_UPDATE_CLICK(const TYPE_S8 gameid)
{

    CaptureTelemetryData(
        METRIC_HOVERCARD_UPDATE_CLICK, 
        kMETRIC_HOVERCARD_UPDATE_CLICK, 
        kMETRIC_GROUP_HOVERCARD,
        gameid);
}


///////////////////////////////////////////////////////////////////////////////
//  PINNED OIG WIDGETS
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  PINNED OIG WIDGETS: Pinning state of a widget
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PINNED_WIDGETS_STATE_CHANGED(const TYPE_S8 product_id, const TYPE_U16 widget_id, const TYPE_U16 pinnedState, const TYPE_U16 fromSDK)
{

    CaptureTelemetryData(
        METRIC_PINNED_WIDGETS_STATE_CHANGED,
        kMETRIC_PINNED_WIDGETS_STATE_CHANGED,
        kMETRIC_GROUP_PINNED_WIDGETS,
        product_id,
        widget_id,
        pinnedState,
        fromSDK);
}

//-----------------------------------------------------------------------------
//  PINNED OIG WIDGETS: Pinning hotkey toggled
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PINNED_WIDGETS_HOTKEY_TOGGLED(const TYPE_S8 product_id, const TYPE_U16 pinnedState, const TYPE_U16 fromSDK)
{

    CaptureTelemetryData(
        METRIC_PINNED_WIDGETS_HOTKEY_TOGGLED,
        kMETRIC_PINNED_WIDGETS_HOTKEY_TOGGLED,
        kMETRIC_GROUP_PINNED_WIDGETS,
        product_id,
        pinnedState,
        fromSDK);
}

//-----------------------------------------------------------------------------
//  PINNED OIG WIDGETS: Max. pinned widgtes per game
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PINNED_WIDGETS_COUNT(const TYPE_S8 product_id, const TYPE_U16 pinnedWindowsCount, const TYPE_U16 fromSDK)
{

    CaptureTelemetryData(
        METRIC_PINNED_WIDGETS_COUNT,
        kMETRIC_PINNED_WIDGETS_COUNT,
        kMETRIC_GROUP_PINNED_WIDGETS,
        product_id,
        pinnedWindowsCount,
        fromSDK);
}


///////////////////////////////////////////////////////////////////////////////
//  PLUGINS
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//  PLUGIN: Plugin was successfully loaded
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PLUGIN_LOAD_SUCCESS(const TYPE_S8 product_id, const TYPE_S8 plugin_version, const TYPE_S8 compatible_client_version)
{

    CaptureTelemetryData(
        METRIC_PLUGIN_LOAD_SUCCESS,
        kMETRIC_PLUGIN_LOAD_SUCCESS,
        kMETRIC_GROUP_PLUGIN,
        product_id,
        plugin_version,
        compatible_client_version);
}

//-----------------------------------------------------------------------------
//  PLUGIN: Plugin failed to load
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PLUGIN_LOAD_FAILED(const TYPE_S8 product_id, const TYPE_S8 plugin_version, const TYPE_S8 compatible_client_version, const TYPE_I32 error_code, const TYPE_I32 os_error_code)
{

    CaptureTelemetryData(
        METRIC_PLUGIN_LOAD_FAILED,
        kMETRIC_PLUGIN_LOAD_FAILED,
        kMETRIC_GROUP_PLUGIN,
        product_id,
        plugin_version,
        compatible_client_version,
        error_code,
        os_error_code);
}

//-----------------------------------------------------------------------------
//  PLUGIN: Origin executed some operation on the plug-in
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_PLUGIN_OPERATION(const TYPE_S8 product_id, const TYPE_S8 plugin_version, const TYPE_S8 compatible_client_version, const TYPE_S8 operation, const TYPE_I32 exit_code)
{

    CaptureTelemetryData(
        METRIC_PLUGIN_OPERATION,
        kMETRIC_PLUGIN_OPERATION,
        kMETRIC_GROUP_PLUGIN,
        product_id,
        plugin_version,
        compatible_client_version,
        operation,
        exit_code);
}
//----------------------------------------------------------------------------
// DOWNLOAD QUEUED: QUEUE ENTITLEMENT COUNT
//----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_QUEUE_ENTITLMENT_COUNT(const TYPE_I32 count)
{

    CaptureTelemetryData(
        METRIC_QUEUE_ENTITLMENT_COUNT,
        kMETRIC_QUEUE_ENTITLMENT_COUNT,
        kMETRIC_GROUP_DOWNLOADQUEUE,
        count);
}

//----------------------------------------------------------------------------
// DOWNLOAD QUEUED: QUEUE WINDOW OPENED
//----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_QUEUE_WINDOW_OPENED(const TYPE_S8 context)
{

    CaptureTelemetryData(
        METRIC_QUEUE_WINDOW_OPENED,
        kMETRIC_QUEUE_WINDOW_OPENED,
        kMETRIC_GROUP_DOWNLOADQUEUE,
        context);
}

//----------------------------------------------------------------------------
// DOWNLOAD QUEUED: QUEUE ORDER CHANGED
//----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_QUEUE_ORDER_CHANGED(const TYPE_S8 old_product_id, const TYPE_S8 new_product_id, const TYPE_S8 context)
{

    CaptureTelemetryData(
        METRIC_QUEUE_ORDER_CHANGED,
        kMETRIC_QUEUE_ORDER_CHANGED,
        kMETRIC_GROUP_DOWNLOADQUEUE,
        old_product_id,
        new_product_id,
        context);
}

//----------------------------------------------------------------------------
// DOWNLOAD QUEUED: ITEM REMOVED
//----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_QUEUE_ITEM_REMOVED(const TYPE_S8 product_id, const TYPE_S8 wasHead, const TYPE_S8 context)
{

    CaptureTelemetryData(
        METRIC_QUEUE_ITEM_REMOVED,
        kMETRIC_QUEUE_ITEM_REMOVED,
        kMETRIC_GROUP_DOWNLOADQUEUE,
        product_id,
        wasHead,
        context);
}

//----------------------------------------------------------------------------
// DOWNLOAD QUEUED: QUEUE CALLOUT SHOWN
//----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_QUEUE_CALLOUT_SHOWN(const TYPE_S8 product_id, const TYPE_S8 context)
{

    CaptureTelemetryData(
        METRIC_QUEUE_CALLOUT_SHOWN,
        kMETRIC_QUEUE_CALLOUT_SHOWN,
        kMETRIC_GROUP_DOWNLOADQUEUE,
        product_id,
        context);
}

//----------------------------------------------------------------------------
// DOWNLOAD QUEUED: QUEUE HEAD BUSY WANRING SHOWN
//----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_QUEUE_HEAD_BUSY_WARNING_SHOWN(const TYPE_S8 head_product_id, const TYPE_S8 head_operation, const TYPE_S8 ent_product_id, const TYPE_S8 ent_operation)
{

    CaptureTelemetryData(
        METRIC_QUEUE_HEAD_BUSY_WARNING_SHOWN,
        kMETRIC_QUEUE_HEAD_BUSY_WARNING_SHOWN,
        kMETRIC_GROUP_DOWNLOADQUEUE,
        head_product_id,
        head_operation,
        ent_product_id,
        ent_operation);
}

void EbisuTelemetryAPI::Metric_QUEUE_GUI_HTTP_ERROR(const TYPE_I32 qnetworkerror, const TYPE_I32 httpstatus, const TYPE_I32 cachecontrol, const TYPE_S8 url)
{
    CaptureTelemetryData(METRIC_QUEUE_GUI_HTTP_ERROR, kMETRIC_QUEUE_GUI_HTTP_ERROR, kMETRIC_GROUP_DOWNLOADQUEUE, 
        qnetworkerror,
        httpstatus,
        cachecontrol,
        url);
}

//-----------------------------------------------------------------------------
//  FEATURE CALLOUTS: Callout was dismissed
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_FEATURE_CALLOUT_DISMISSED(const TYPE_S8 seriesName, const TYPE_S8 objectName, const bool result)
{

    CaptureTelemetryData(
        METRIC_FEATURE_CALLOUT_DISMISSED,
        kMETRIC_FEATURE_CALLOUT_DISMISSED,
        kMETRIC_GROUP_FEATURE_CALLOUTS,
        seriesName,
        objectName,
        result ? 1 : 0);
}

//-----------------------------------------------------------------------------
//  FEATURE CALLOUTS: Callout was series was complete
//-----------------------------------------------------------------------------
void EbisuTelemetryAPI::Metric_FEATURE_CALLOUT_SERIES_COMPLETE(const TYPE_S8 seriesName, const bool result)
{

    CaptureTelemetryData(
        METRIC_FEATURE_CALLOUT_SERIES_COMPLETE,
        kMETRIC_FEATURE_CALLOUT_SERIES_COMPLETE,
        kMETRIC_GROUP_FEATURE_CALLOUTS,
        seriesName,
        result ? 1 : 0);
}

