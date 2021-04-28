///////////////////////////////////////////////////////////////////////////////
// main.cpp
// 
// Created by Kevin Moore
// Copyright (c) 2011 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "Common.h"
#include "Registration.h"
#include "AppRunDetector.h"
#include "Updater.h"
#include "Locale.h"
#include "Resource.h"
#include "LogService_win.h"
#include "VerifyEmbeddedSignature.h"
#include "Win32CppHelpers.h"
#include <string>
#include <atlbase.h>
#include <atlwin.h>
#include <atlapp.h>
#include <algorithm>
#include <ShlObj.h>
#include "version/version.h"
#include <TlHelp32.h>
#include <EAStdC/EADateTime.h>

using namespace std;
CAppModule _Module;

// prototypes
int setOriginConfig(const char*, HMODULE);
int bind(int, char**, HMODULE);
bool checkForSelfRegisterSettings(const wchar_t* const);
bool checkForRestart(const wchar_t* const);
bool checkMultipleInstances(bool, EA::AppRunDetector&, bool);
void updateOrigin(const wchar_t* const, const wchar_t* const);
void initLanguages(void);
void initializeLogService (void);

DWORD WINAPI updateThread(VOID* lParam);
LRESULT CALLBACK DlgProcError(HWND, UINT, WPARAM, LPARAM);

/// \brief initially set to kClientStartupMinimized if launched via autostart but it then keeps track of user's choices
/// i.e. if user restores window, then all subsequent dialogs are shown (kClientStartupNormal)
ClientStartupState gClientStartupState = kClientStartupNormal;

bool gIsDownloadSuccessful = false;
bool gIsOkWinHttpReadData = true;
bool gIsUpToDate = false;
bool gIsOkQuerySession = false;
__time64_t gStartupTime = 0;

static const unsigned int CONFIG_ARG_TEXT_SIZE = 256;
static char gConfigArgText[CONFIG_ARG_TEXT_SIZE];

int argc = __argc;
char** argv = new char*[argc + 4];  //+1 for /StartOfflineMode, +2 for /StartClientMinimized or /StartClientMaximized, +3 for /StartupTime:, +4 for /downloadConfigSuccess or /downloadConfigFailed

namespace
{
    /// \brief Appends extra info surrounding downloading application config for client reporting.
    /// If the request was successful, report the amount of time it took to
    /// receiving the dynamic config response. Otherwise, report extra error
    /// info surrounding the failure.
    void addDownloadConfigArgument(const bool success, const std::string& errorText)
    {
        char* argString = 0;

        if (success)
        {
            argString = "/downloadConfigSuccess";
        }
        else
        {
            argString = "/downloadConfigFailed";
        }

        argv [argc] = argString;
        
        // Calculate total size of argument and add one for separating 
        // delimiter.
        static const unsigned int ARG_DELIM_COUNT = 1;
        unsigned int textLength = strlen(argString) + errorText.size() + ARG_DELIM_COUNT;

        // Guard against error messages that are too big
        if (textLength < CONFIG_ARG_TEXT_SIZE)
        {
            sprintf(gConfigArgText, "%s:%s", argString, errorText.c_str());
            argv [argc] = gConfigArgText;
        }
        else
        {
            ORIGINBS_LOG_WARNING << "Config failure error message string too large to report: " << errorText;
        }

        argc++;
    }
}

template <typename T>
void lower(T& str)
{
    std::transform(str.begin(), str.end(), str.begin(), tolower);
}

bool contains(const wchar_t* const commandLine, const wchar_t* const token)
{
    std::wstring cl(commandLine);
    lower<std::wstring>(cl);

    std::wstring tk(token);
    lower<std::wstring>(tk);

    const wchar_t* findStr = wcsstr(cl.c_str(), tk.c_str());
    return NULL != findStr;
}

bool isShouldUpdate(const wchar_t* const commandLine)
{
    wchar_t szOriginPath[MAX_PATH];
    ::GetModuleFileName(::GetModuleHandle(NULL), szOriginPath, MAX_PATH);
    std::wstring originLocation = szOriginPath;
    // Remove executable
    int lastSlash = originLocation.find_last_of(L"\\");
    originLocation = originLocation.substr(0, lastSlash + 1);

    wchar_t iniLocation[256] = {0};
    wsprintfW(iniLocation, L"%sEACore.ini", originLocation.c_str());
    wchar_t enableUpdatingSetting[256] = {0};

    // Update/NoUpdate override
    if (GetPrivateProfileString(L"Bootstrap", L"EnableUpdating", NULL, enableUpdatingSetting, 256, iniLocation))
    {
        if (_wcsicmp(enableUpdatingSetting, L"true") == 0)
        {
            return true;
        }
        if (_wcsicmp(enableUpdatingSetting, L"false") == 0)
        {
            return false;
        }
    }

    if (contains(commandLine, L"/noUpdate"))
    {
        return false;
    }
    if (contains(commandLine, L"/update"))
    {
        return true;
    }
#ifdef DEBUG
    return false;
#else
    return true;
#endif
}

bool isUseEmbeddedEULA(const wchar_t* const commandLine)
{
    if (contains(commandLine, L"/useEmbeddedEULA"))
        return true;

    return false;
}


////////////////////////////////////////////////////////////////////////////////
// Unfortunately we need this function GetParentProcessIsBrowser in two places
// This function is here because main.cpp doesn't include any common Origin code (such as EnvUtils where the duplicate resides.)
//
// In the flow where another running instance of Origin is running, no OriginApplicationObject will be created.
// Therefore, in order to maintain the fact that a origin:// protocol command was generated by a browser,
// we have to pass that information to the running instance.
// We detect via the code below, and add a special command line parameter letting the other instance of origin know.
bool caseInsensitiveStringCompare(const std::wstring& a, const std::wstring& b)
{
    unsigned int sz = a.size();
    if (b.size() != sz)
        return false;
    for (unsigned int i = 0; i < sz; ++i)
        if (towlower(a[i]) != towlower(b[i]))
            return false;
    return true;

}
bool GetParentProcessIsBrowser()
    {
        DWORD pid;
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
                if (pe.th32ProcessID == pid) 
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
                            std::wstring parentEXE(pe.szExeFile);

                            if (caseInsensitiveStringCompare(parentEXE, L"chrome.exe") ||
                                caseInsensitiveStringCompare(parentEXE, L"iexplore.exe") ||
                                caseInsensitiveStringCompare(parentEXE, L"firefox.exe") ||
                                caseInsensitiveStringCompare(parentEXE, L"safari.exe") ||
                                caseInsensitiveStringCompare(parentEXE, L"opera.exe") ||
                                caseInsensitiveStringCompare(parentEXE, L"outlook.exe") ||
                                caseInsensitiveStringCompare(parentEXE, L"thunderbird.exe") ||
                                caseInsensitiveStringCompare(parentEXE, L"maxthon.exe") ||
                                caseInsensitiveStringCompare(parentEXE, L"rockmelt.exe") ||
                                caseInsensitiveStringCompare(parentEXE, L"seamonkey.exe"))
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
    }

static const size_t MESSAGE_BUF_SIZE = 512;
/// \brief Helper to display update error
void displayUpdateError()
{
    wchar_t* text = new wchar_t[MESSAGE_BUF_SIZE+1];
    wchar_t* title = new wchar_t[MESSAGE_BUF_SIZE+1];
    wchar_t* buffer = new wchar_t[MESSAGE_BUF_SIZE+1];
    Locale::GetInstance()->Localize(L"ebisu_client_update_error", &title, MESSAGE_BUF_SIZE);
    Locale::GetInstance()->Localize(L"ebisu_client_update_error_text", &buffer, MESSAGE_BUF_SIZE);
    wsprintfW(text, buffer, L"<A HREF=\"https://download.dm.origin.com/origin/live/OriginSetup.exe\">https://download.dm.origin.com/origin/live/OriginSetup.exe</A>");
    HWND hErrorDialog = CreateDialog(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ERRORDIALOG), NULL, reinterpret_cast<DLGPROC>(DlgProcError));
    SetDlgItemText(hErrorDialog, IDC_SYSLINK1, text);
    SetWindowText(hErrorDialog, title);
    ShowWindow(hErrorDialog, SW_SHOW);
    delete[] text;
    delete[] title;
    delete[] buffer;

    // Just wait in this loop until our callback exits
    MSG msg;
    while(GetMessage( &msg, NULL, 0, 0 ))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

/// \brief Helper to display connection error
void displayConnectionError()
{
    wchar_t* text = new wchar_t[MESSAGE_BUF_SIZE+1];
    wchar_t* title = new wchar_t[MESSAGE_BUF_SIZE+1];
    wchar_t* buffer = new wchar_t[MESSAGE_BUF_SIZE+1];
    Locale::GetInstance()->Localize(L"ebisu_client_offline_update_error_title_caps", &title, MESSAGE_BUF_SIZE);
    Locale::GetInstance()->Localize(L"ebisu_client_offline_update_error_text", &buffer, MESSAGE_BUF_SIZE);
    wsprintfW(text, buffer, L"\n<A HREF=\"https://download.dm.origin.com/origin/live/OriginSetup.exe\">https://download.dm.origin.com/origin/live/OriginSetup.exe</A>");
    HWND hErrorDialog = CreateDialog(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ERRORDIALOG), NULL, reinterpret_cast<DLGPROC>(DlgProcError));
    SetDlgItemText(hErrorDialog, IDC_SYSLINK1, text);
    SetWindowText(hErrorDialog, title);
    ShowWindow(hErrorDialog, SW_SHOW);
    delete[] text;
    delete[] title;
    delete[] buffer;

    // Just wait in this loop until our callback exits
    MSG msg;
    while(GetMessage( &msg, NULL, 0, 0 ))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

bool repairLaunchParameters(int i, char *launchParam, int max_length)
{
    bool protocol_found = false;
    bool use_adj_param = false;
    wchar_t *protocol_str = L"origin://";

    if (strnicmp(argv[i], "origin://", 9) == 0)  // first test if the argument starts with "origin://" as the launch identifier
    {
        protocol_found = true;
    }
    else
    if (strnicmp(argv[i], "origin2://", 10) == 0)  // if not test if the argument starts with "origin2://" as the launch identifier
    {
        protocol_found = true;
        protocol_str = L"origin2://";
    }

    if (protocol_found)
    {
        int len = strlen(argv[i]);

        wstring command_line = GetCommandLine();
        string::size_type pos = command_line.find(protocol_str, 0); // then find the same id in the 16-bit command line
        string::size_type endpos = command_line.find_first_of(L" ", pos);   // find the end of the parameter
        if (wstring::npos == endpos)    // if this is the last parameter just use the size of the string as the end position
            endpos = command_line.size();

        if (wstring::npos != pos)
        {
            int w_index = pos;
            int w_length = endpos - pos;

            if (w_length >= max_length)
            {
                ORIGINBS_LOG_WARNING << "launch parameter length too long to check for repairing";
                return false;      // it is too big and will overflow the launchParam string size, just use the current param
            }
            else
            if (w_length <= len)
            {
                ORIGINBS_LOG_WARNING << "parsing issue - space-delimited command line shorter than argument length";
                return false;      // parsing issue (due to IE9 not URL-encoding spaces), just use the current param
            }

            int d_index = 0;

            // we want to copy over the existing parameter content to the repaired string and insert double-quotes where they are missing
            for (int j = 0; j < len; )
            {
                int w_char = command_line[w_index++];
                if ((char) w_char == argv[i][j])
                {
                    launchParam[d_index++] = argv[i][j];    // just copy
                    j++;
                }
                else
                    if ((char) w_char == '\"')
                    {
                        launchParam[d_index++] = '\"';
                        use_adj_param = true;   // found a double-quote so marked as needs repair
                    }
                    else
                    {
                        if (use_adj_param)
                        {
                            ORIGINBS_LOG_WARNING << "launch parameter may need repair but something didn't match up after a double-quote";
                        }
                        use_adj_param = false;  // something doesn't match up - just use the current param
                        break;
                    }
            }

            if (use_adj_param)
            {   
                if (w_index <= (int) endpos)  // check for trailing double-quote
                {
                    int w_char = command_line[w_index];
                    if ((char) w_char == '\"')
                    {
                        launchParam[d_index++] = '\"';
                    }
                }

                launchParam[d_index] = 0;   // close string
                argv[i] = launchParam; // use our new param string instead
            }
        }
    }

    return use_adj_param;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    char* lpCmdLine, int nCmdShow)
{
    // Comment this in for remote debugging
    // MessageBox(NULL, L"You can attach the debugger now.", L"Hey", MB_OK);
    //
    gStartupTime = EA::StdC::GetTime();
    initializeLogService();

    //!!! PLEASE BE VERY CAREFUL if you're going to change mpDetectorWindow type to something other than QWidget
    //by doing so, it will make it unable to detect if any pre-9.4 version of the client is running
    EA::AppRunDetector appRunDetector(L"OriginApp", L"QWidget", L"Origin", L"Qt5QWindowIcon", hInstance);
    bool bAllowMultipleInstances = false;

    for (int i = 0; i < __argc; i++)
    {
        if (strcmp(__argv[i], "-Origin_MultipleInstances") == 0)
        {
            bAllowMultipleInstances = true;
        }
    }

    wstring commandLine = GetCommandLine();

    // We need to initialized languages now
    initLanguages();

    if (checkMultipleInstances(bAllowMultipleInstances, appRunDetector, false))
    {
        // Only return if we didn't restart ourselves since the application might still be shutting down
        if (!checkForRestart(commandLine.c_str()))
        {
            ORIGINBS_LOG_EVENT << "Multiple instance detected and didn't restart self";
            LogService::release();
            return 0;
        }
        else
        {
            Sleep(5000); // give the other instance(s) 5 seconds to shut down
            appRunDetector.ForceRunningInstanceToTerminate();
            for(int i=0; i<128; i++)	// worst case: 128 Origin instances running, 1.3 seconds 
            {
                appRunDetector.Invalidate();
                if (checkMultipleInstances(bAllowMultipleInstances, appRunDetector, true))
                {
                    appRunDetector.ForceRunningInstanceToTerminate();
                    Sleep(10);
                }
                else
                {
                    break;
                }
            }

            // if we could not close the other instances, simply quit and let the already running Origin instances handle it
            appRunDetector.Invalidate();
            if (checkMultipleInstances(bAllowMultipleInstances, appRunDetector, false))
            {
                ORIGINBS_LOG_EVENT << "Unable to close other running instance";
                LogService::release();
                return 0;
            }
        }
    }

    if (checkForSelfRegisterSettings(commandLine.c_str()))
    {
        ORIGINBS_LOG_EVENT << "Self registering";
        LogService::release();
        return 0;
    }
    
    switch(nCmdShow)
    {
        case SW_SHOWMINIMIZED:
        case SW_SHOWMINNOACTIVE:
        case SW_MINIMIZE:
            gClientStartupState = kClientStartupMinimized;
            break;

        case SW_SHOWMAXIMIZED:
            gClientStartupState = kClientStartupMaximized;
            break;

        case SW_SHOW:
        case SW_SHOWNORMAL:
        default:
            gClientStartupState = kClientStartupNormal;
            break;
    }
    
    //it's not enough to rely on just -AutoStart to determine whether we should start out minimized
    //because this code base gets called twice when the update occurs
    //1. normally if autostart is enabled, we want the window minimized
    //2. but if we're updating, then -AutoStart would be set to true, then we'd update. if it's auto-update then the window could remain minimized
    //   and we'd be passing in -AutoStart /Installed /StartMinimized to the updateTool which would relaunch Origin after applying the update so we'd come back thru
    //   this logic again (in the updated code)
    //   if we had unminimized the window during the update process, updateTool would be started with -AutoStart /Installed
    //3. so we need to actually check for all 3 parameters
    if (contains (commandLine.c_str(), L"-AutoStart"))
    {
        if (contains (commandLine.c_str(), L"/Installed"))
        {
            if (contains (commandLine.c_str(), L"/StartMinimized"))
            {
                //previous version had the windows minimized
                gClientStartupState = kClientStartupMinimized;
            }
        }
        else
        {
            gClientStartupState = kClientStartupMinimized;    //just -AutoStart so start minimized
        }
    }

    HANDLE updateHandle = CreateThread(NULL, 0, updateThread, NULL, 0, NULL);
    ORIGINBS_LOG_EVENT << "Waiting for update thread to finish";
    WaitForSingleObject(updateHandle, INFINITE);

#ifndef ORIGIN_MAC
    // set environment variable for IGO logging (can't call SHGetFolderPath from inside injected dll)
    WCHAR commonDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, commonDataPath)))
    {
        SetEnvironmentVariableW(L"IGOLogPath", commonDataPath);
    }
#endif

    bool signatureValid = false;

    // Build full paths to the critical Origin binaries (Origin.exe and OriginClient.dll)
    wchar_t exePath[MAX_PATH] = {0};
    ::GetModuleFileName(::GetModuleHandle(NULL), exePath, MAX_PATH);

    std::wstring originLocation = exePath;
    // Remove executable
    int lastSlash = originLocation.find_last_of(L"\\");
    originLocation = originLocation.substr(0, lastSlash + 1);

    wchar_t clientDllFilename[MAX_PATH] = {0};
    wsprintfW(clientDllFilename, L"%sOriginClient.dll", originLocation.c_str());

    // Lock the DLL before we check the signature so nobody can modify it after we check (prevent race condition)
    Auto_FileLock dllLock(clientDllFilename);

#if !defined(CHECK_SIGNATURES) && !defined(DEBUG)
    // We should never see this for production builds!
    ::MessageBox(NULL, L"This non-debug version of Origin has been built without file signature validation.\n\nDo not distribute!\n\nPress OK to continue.", L"Security Warning", MB_OK | MB_ICONWARNING);
#endif

#if !defined(CHECK_SIGNATURES)
    signatureValid = true;
#else
    if (!dllLock.acquired())
    {
        ORIGINBS_LOG_ERROR << "Unable to lock file for signature checking: " << clientDllFilename;
    }
    else
    {
        //Verify our own executable
        bool exe_verify = isValidEACertificate(exePath);
        if(!exe_verify) {
            ORIGINBS_LOG_ERROR << "Problem found in executable signature.";
        }	

        //Verify dll
        bool dll_verify = isValidEACertificate(clientDllFilename);
        if(!dll_verify) {
            ORIGINBS_LOG_ERROR << "Problem found in dll signature.";
        }    
        signatureValid = dll_verify && exe_verify;
    }
#endif

    // If the signature is not valid, it is not safe to continue
    if (!signatureValid)
    {
        ORIGINBS_LOG_ERROR << "OriginClient.dll Signature invalid";
        if (gIsOkQuerySession)
            displayUpdateError();
        else
            displayConnectionError();

        // This will never execute, both of the error functions above eventually call exit, but just in case...
        return -1;
    }

    HMODULE hOrigin = LoadLibrary(clientDllFilename);

    // We've loaded the module, it is now safe to release our lock
    dllLock.release();

    if (contains(GetCommandLine(), L"/Uninstall"))
    {
        // Don't show anything if we're uninstalling
    }
    else if (!gIsDownloadSuccessful && !gIsUpToDate)
    {
        if (gIsOkWinHttpReadData && gIsOkQuerySession)
        {
            ORIGINBS_LOG_ERROR << "Update failed";
            displayUpdateError();
        }
        else
        {
            ORIGINBS_LOG_ERROR << "Network connection failed";
            displayConnectionError();
        }
    }
    else if (!hOrigin)
    {
        ORIGINBS_LOG_ERROR << "Unable to load OriginClient.dll";
        // OriginClient.dll not found
        if (gIsOkQuerySession)
            displayUpdateError();
        else
            displayConnectionError();

    } 
    

    int ret;
    if (hOrigin)
    {
        const int kMaxLaunchParamLength = 1024;
        char launchParam[kMaxLaunchParamLength];
        bool launchParamRepaired = false;

        // tack on the startup time for telemetry
        char startupTime[64];
        _snprintf_s(startupTime, sizeof(startupTime), _TRUNCATE, "-startupTime:%I64u", gStartupTime);
        argv[argc++] = startupTime;

        string cmdLineargs;
        int i;
        for (i = 0; i < argc; i++)
        {
            // check "origin:" launch param for missing double-quotes and repair it (EBIBUGS-27419)
            if (!launchParamRepaired)   // can only do it once
                launchParamRepaired = repairLaunchParameters(i, launchParam, kMaxLaunchParamLength);

            cmdLineargs.append (argv [i]);
            if (i < argc-1)
            {
                cmdLineargs.append (" ");
            }
        }

        if (launchParamRepaired)
        {
            ORIGINBS_LOG_EVENT << "Launch parameters repaired";
        }

        ORIGINBS_LOG_EVENT << "Launching client: cmdLine =[" << cmdLineargs << "]";
        ret =bind(argc, argv, hOrigin);
    }
    else
    {
        ret = GetLastError();
        ORIGINBS_LOG_ERROR << "Launching failed with error: " << ret;
    }
    LogService::release();
    return ret;
}

int bind(int argc, char** argv, HMODULE hOrigin)
{
    int (*pFn)(int, char**) = 0; 
    pFn = (int (__cdecl *)(int, char**))GetProcAddress(hOrigin, "OriginApplicationStart");
    if (pFn == NULL)
    {
        int err = GetLastError ();
        ORIGINBS_LOG_ERROR << "Unable to GetProcAddress. error = " << err;
        return 0;
    }
    else
    {
        return pFn(argc, argv);
    }
}

void initLanguages()
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    {
        Locale::GetInstance();
    }

    // Tear down COM
    CoUninitialize();
}

DWORD WINAPI updateThread(VOID* lParam)
{
    // Init some stuff for ATL
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); 
    INITCOMMONCONTROLSEX icex;
    icex.dwSize=sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC=ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);
    _Module.Init(NULL, ::GetModuleHandle(NULL), &LIBID_ATLLib);
    AtlAxWinInit();

    {
        // Do our updates
        bool shouldUpdate = isShouldUpdate(GetCommandLine());

        ORIGINBS_LOG_EVENT << "shouldUpdate = " << shouldUpdate;

        Updater updater;

        bool useEmbeddedEULA = isUseEmbeddedEULA(GetCommandLine());
        updater.setUseEmbeddedEULA( useEmbeddedEULA );

        if (shouldUpdate)
        {
            updater.DoUpdates();
        }

        ORIGINBS_LOG_EVENT << "doUpdates completed";

        // Need to copy argc and argv to add it
        for (int i = 0; i < __argc; i++)
        {
            argv[i] = __argv[i];
        }

        //these args are used for when no update needs to be performed, used by bind
        //if updating, then need to pass the argument to updateTool (see UpdateOrigin)
        if (updater.RunOfflineMode())
        {
            argv[argc] = "/StartOffline";
            argc++;
        }

        if (gClientStartupState == kClientStartupMinimized)
        {
            argv [argc] = "/StartClientMinimized";
            argc++;
        }
        else if (gClientStartupState == kClientStartupMaximized)
        {
            argv [argc] = "/StartClientMaximized";
            argc++;
        }

        // Is there a temp Origin here?
        wchar_t path[MAX_PATH+1] = {0};
        GetModuleFileName(NULL, path, MAX_PATH);
        wstring location = path;
        int lastSlash = location.find_last_of(L"\\");
        location = location.substr(0, lastSlash+1);

        gIsDownloadSuccessful = updater.isDownloadSuccessful();
        gIsUpToDate = updater.isUpToDate();
        gIsOkWinHttpReadData = updater.isOkWinHttpReadData();
        gIsOkQuerySession = updater.isOkQuerySession();

        bool success = updater.DownloadConfiguration();
        addDownloadConfigArgument(success, updater.downloaderErrorText());

        updateOrigin(location.c_str(), updater.Updated());
    }

    // Tear down COM
    CoUninitialize();

    return 1;
}

bool checkMultipleInstances(bool bAllowMultipleInstances, EA::AppRunDetector &appRunDetector, bool wait30Seconds)
{
    bool bInstanceActivated = false;

    if (!bAllowMultipleInstances)
    {
        if(appRunDetector.IsAnotherInstanceRunning())
        {
            //we use GetCommandLine here, because argv/argc strips out the quotes (if they are not properly escaped)
            std::wstring sCommandLine(GetCommandLine());


            if (GetParentProcessIsBrowser())
                sCommandLine += L" \"-Origin_LaunchedFromBrowser\"";

            bInstanceActivated = appRunDetector.ActivateRunningInstance(sCommandLine, true);

            // Exit silently if there is another instance running on the same account
            // Print out the error message and exit if there is another instance of client running on another account/session
            // If an instance of Origin is running in a separate session/account, we would have detected:
            // 1) the running instance from the global mutex
            // 2) no local instance from the local mutex
            // 3) we wouldn't have been able to activate the instance (which would require accessing the client window in the other session)
            if(appRunDetector.IsAnotherInstanceRunningLocal() == false && !bInstanceActivated)
            {
                ORIGINBS_LOG_ERROR << "Another instance running on another account";
                // print the error message
                wchar_t* text = new wchar_t[512];
                wchar_t* title = new wchar_t[512];
                Locale::GetInstance()->Localize(L"ebisu_client_one_instance_warning_title", &title, 512);
                Locale::GetInstance()->Localize(L"ebisu_client_one_instance_warning_message", &text, 512);
                wstring strText(text);
                wstring::iterator it = strText.begin();
                while(it != strText.end())
                {
                    if(*it == *L"%")
                    {
                        strText.replace(it, it+2, L"Origin");
                        it = strText.begin();
                    }
                    ++it;
                }
                wstring strTitle(title);
                it = strTitle.begin();
                while(it != strTitle.end())
                {
                    if(*it == *L"%")
                    {
                        strTitle.replace(it, it+2, L"ORIGIN");
                        it = strTitle.begin();
                    }
                    ++it;
                }
                MessageBoxEx(NULL, strText.c_str(), strTitle.c_str(), MB_OK, NULL);
                delete[] text;
                delete[] title;
                return true;
            }
        }
    }

    // if another instance in the same session is running, it must be the same user, so silently quit,
    if(!bAllowMultipleInstances && appRunDetector.IsAnotherInstanceRunningLocal() && bInstanceActivated)
    {
        return true;
    }
    else
    {
        // Only if we do not allow multiple instances, wait for the other instance to quit up to 30 seconds.
        if(!bAllowMultipleInstances)
        {
            // Lets wait for the semaphore to become available, and then continue running, else quit.
            if(!bInstanceActivated)
            {
                DWORD now = GetTickCount();

                while (wait30Seconds == false && now + 30000 > GetTickCount())
                {
                    if(!appRunDetector.IsAnotherInstanceRunningLocal())
                    {
                        // We got the mutex.
                        ORIGINBS_LOG_EVENT << "Got the single instance mutex";
                        return false;
                    }

                    // Wait a bit.
                    Sleep(100);
                }

                // We couldn't get the mutex within 30 seconds.
                ORIGINBS_LOG_EVENT << "Couldn't get the single instance mutex within 30 sec.";
                return true;
            }
        }
    }

    return false;
}

void updateOrigin(const wchar_t* path, const wchar_t* newVersion)
{
    wchar_t tmpOrigin[MAX_PATH+1] = {0};
    wsprintfW(tmpOrigin, L"%sOriginTMP.exe", path);

    // Creation of the UpdateTool.exe with arguments command line
    const size_t MAX_PATH_WITH_PARAMS = 1024;
    wchar_t updateTool[MAX_PATH_WITH_PARAMS+1]={0};
    wchar_t updateToolWithArgs[MAX_PATH_WITH_PARAMS+1]={0};
    wsprintfW(updateTool, L"%sUpdateTool.exe", path);

    //  Supplement quotes in command-line to ensure they get passed back to Origin correctly.
    LPTSTR cmdsrc = GetCommandLine();    
    wstring cmd;
    while (*cmdsrc != 0)
    {
        if (*cmdsrc == '\"')
        {
            cmd += '\\';
        }
        cmd += *cmdsrc++;
    }

    if (wcscmp(newVersion, L"") != 0)
    {
        wchar_t arg[32] = {0};
        wsprintfW(arg, L" /Installed:%s", newVersion);
        cmd.append(arg);
    }

    if (gClientStartupState == kClientStartupMinimized)
    {
        wchar_t arg[32] = {0};
        wsprintfW(arg, L" /StartClientMinimized");
        cmd.append(arg);
    }
    else if (gClientStartupState == kClientStartupMaximized)
    {
        wchar_t arg[32] = {0};
        wsprintfW(arg, L" /StartClientMaximized");
        cmd.append(arg);
    }

    wsprintfW(updateToolWithArgs, L"%s \"%s\" /FinalizeSelfUpdate", updateTool, cmd.c_str());

    // Find the file to update origin with
    WIN32_FIND_DATAW findFileData;
    ZeroMemory(&findFileData, sizeof(findFileData));
    HANDLE handle = FindFirstFile(tmpOrigin, &findFileData);

    if (handle != INVALID_HANDLE_VALUE)
    {
        ORIGINBS_LOG_EVENT << "Launching Update Tool: " << updateToolWithArgs;

        // Need to call our update tool and quit
        STARTUPINFO startupInfo;
        memset(&startupInfo,0,sizeof(startupInfo));
        PROCESS_INFORMATION procInfo;
        memset(&procInfo,0,sizeof(procInfo));
        BOOL result = CreateProcess(NULL, updateToolWithArgs, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &procInfo);
        if (!result)
        {
            int err = GetLastError ();
            ORIGINBS_LOG_ERROR << "CreateProcess of UpdateTool failed. err = " << err;
        }
        LogService::releaseAndExitProcess();    //need to release LogService BEFORE exiting
    }
    else
    {
        // Delete the renaming tool if it's there
        // Don't do this anymore... we want to keep it around
        //DeleteFile(updateTool);
    }

    FindClose(handle);
}

bool checkForRestart(const wchar_t* const cmdLine)
{
    return contains(cmdLine, L"-Origin_Restart");
}

bool checkForSelfRegisterSettings(const wchar_t* const cmdLine)
{
    if (contains(cmdLine, L"/register"))
    {
        Registration::registerUrlProtocol();
        return true;
    }
    else if (contains(cmdLine, L"/setautostart"))
    {
        Registration::setAutoStartEnabled(true);
        return true;
    }
    return false;
}

LRESULT CALLBACK DlgProcError(HWND hWndDlg, UINT Msg,
    WPARAM wParam, LPARAM lParam)
{
    switch(Msg)
    {
    case WM_COMMAND:
        {
            if (wParam == IDOK)
            {
                LogService::releaseAndExitProcess();    //need to release LogService BEFORE exiting
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;
            if (pnmh->idFrom == IDC_SYSLINK1)
            {
                if ((pnmh->code == NM_CLICK) || (pnmh->code == NM_RETURN))
                {
                    PNMLINK link = (PNMLINK) lParam;

                    ShellExecuteW(NULL, L"open", link->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
                }
            }
        }
        break;
    case WM_CTLCOLORSTATIC:
        {
            if(GetDlgItem(hWndDlg, IDC_BOTTOM) == (HWND)lParam)
            {
                return false;
            }
            return (BOOL)CreateSolidBrush(RGB(255,255,255));
        }
        break;
    case WM_CTLCOLORDLG:
        {
            return (BOOL)CreateSolidBrush(RGB(255,255,255));
        }
    }

    return false;
}

void initializeLogService ()
{
    const std::wstring sLogFolder(L"\\Logs\\Bootstrapper_Log");

    TCHAR szPath[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, szPath))) return;

    std::wstring strLogFolder (szPath);
    strLogFolder.append (L"\\Origin");
    strLogFolder.append (sLogFolder);

#ifdef _DEBUG
    bool logDebug = true;
#else
    bool logDebug = false;
#endif

    LogService::init();
    Logger::Instance()->Init(strLogFolder.c_str(), logDebug);

    //	Output the client version
    ORIGINBS_LOG_EVENT << "Version: " << EALS_VERSION_P_DELIMITED;
    LoggerFilter::DumpCommandLineToLog("Cmdline Param", GetCommandLine());
}

__int64 GetTimeOfDay()
{
    FILETIME      ft;
    LARGE_INTEGER li;

    GetSystemTimeAsFileTime(&ft);       // Microsoft call. The information is in Coordinated Universal Time (UTC) format.

    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;

    // since January 1, 1970
    const __int64 EpochOffset = INT64_C(116444736000000000);

    // system time units are 100 nanoseconds, but this function needs to return nanoSeconds,
    // hence the 100-factor multiplication.
    return (li.QuadPart - EpochOffset) * 100;
}
