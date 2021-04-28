///////////////////////////////////////////////////////////////////////////////
// Updater.cpp
// 
// Created by Kevin Moore
// Copyright (c) 2011 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "Updater.h"
#include "Common.h"
#include "Downloader.h"
#include "Unpacker.h"
#include "Resource.h"
#include "Locale.h"
#include "EULA.h"
#include "LogService_win.h"
#include "VerifyEmbeddedSignature.h"
#include "version/version.h"
#include "Win32CppHelpers.h"
#include <string>
#include <CommCtrl.h>
#include <ExDispid.h>
#include <ShlObj.h>

// ATL includes for Ax internet browser
#include <atlbase.h>
#include <atlwin.h>
#include <atldef.h>
#include <atltypes.h>
#define _ATL_DLL_IMPL
#include <atliface.h>

#include <cmath>
#include <map>

using namespace std;

LRESULT CALLBACK DlgProcProgress(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DlgProcEULA(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DlgProcUpdate(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DlgProcUnpack(HWND, UINT, WPARAM, LPARAM);

Downloader* gDownloader;
// For externs
HWND hDownloadDialog;
HWND hUnpackDialog;

#define BUTTON_WIDTH_PIXELS 88
#define BUTTON_HEIGHT_PIXELS 26

#define BUFFER_LENGTH 512

Updater::Updater()
    : mIsDownloadSuccessful(false)
    , mIsUpToDate(true)
    , mIsOfflineMode(false)
    , mIsFoundOrigin(false)
    , mIsHasInternetConnection(true)
    , mUseEmbeddedEULA(false)
{
    gDownloader = new Downloader();
    hIcon = LoadIcon(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
    mUpdated = new wchar_t[BUFFER_LENGTH];
    mUpdated[0] = 0;
	mEnvironment[0] = 0;
}

void Updater::DetermineEnvironment(const wchar_t* iniLocation, wchar_t* environmentBuf, int envBufSize)
{
    // Determine the environment
    
    // Debug info from EACore.ini
    GetPrivateProfileString(L"Connection", L"EnvironmentName", L"production", environmentBuf, envBufSize, iniLocation);

    // Check for command line "-EnvironmentName:<environment> which overrides the EACore.ini value if present
    wchar_t* strMatch = wcsstr(GetCommandLine(), L"-EnvironmentName:");
	if (strMatch != 0)
	{
		strMatch += wcslen(L"-EnvironmentName:");
        wstring envFromCommandLine = strMatch;
		int end = envFromCommandLine.find_first_of(L" ");
		envFromCommandLine = envFromCommandLine.substr(0, end);
		wcscpy_s(environmentBuf, envBufSize, envFromCommandLine.c_str());
	}

    // convert to lower case and replace '.' with '-', it is valid to specify either FC-QA or fc.qa in EACore.ini
    _wcslwr_s(environmentBuf, envBufSize);
    wchar_t* dash = wcschr(environmentBuf, L'.');
    while ( dash )
    {
        *dash = L'-';
        dash = wcschr(environmentBuf, L'.');
    }
}

Updater::~Updater()
{
    delete[] mUpdated;
    delete gDownloader;
}

DWORD WINAPI downloadUpdateAsynch(VOID* lParam)
{
    const wchar_t* url = *((const wchar_t**)lParam);
    gDownloader->Download(url);
    return 0;
}

DWORD WINAPI unpackAsynch(VOID* lParam)
{
    const wchar_t* unpackLocation = *((const wchar_t**)lParam);
    Unpacker unpacker;
    unpacker.Extract(unpackLocation);
    return 0;
}

wstring extractVersionStringFromUpdateFilename(const wchar_t* stagedFileName)
{
	wchar_t* context;
	wchar_t* temp = new wchar_t[BUFFER_LENGTH];
	wcscpy_s(&temp[0], BUFFER_LENGTH, stagedFileName);
    wchar_t stagedVersionComp[BUFFER_LENGTH];

	// Split off the .zip
	wchar_t* p = wcstok_s(temp, L".", &context);
	// Ignore OriginUpdate
	p = wcstok_s(p, L"_", &context);
	p = wcstok_s(NULL, L"_", &context);

    stagedVersionComp[0] = NULL;

	while (p != NULL)
	{
		wcscat_s(stagedVersionComp, p);
		p = wcstok_s(NULL, L"_", &context);
		if (p != NULL)
		{
			wcscat_s(stagedVersionComp, L".");
		}
	}

	delete[] temp;
    return wstring(stagedVersionComp);
}

int calculateVersionValue(const wchar_t* versionString)
{
    int version_value = 0;

    wchar_t* temp = new wchar_t[BUFFER_LENGTH];
    wchar_t* context;

    wcscpy_s(&temp[0], BUFFER_LENGTH, versionString);

	wchar_t* p = wcstok_s(temp, L".", &context);

    static int powExponent[4] = { 8, 6, 4, 0 };
    int i = 0;

    // Calculate long based version from components
	while (p != NULL)
	{
		version_value += (_wtoi(p)*(int)pow(10.0, powExponent[i++]));
		p = wcstok_s(NULL, L".", &context);
	}

    delete[] temp;
    return version_value;
}

// We want to sort the staged zips by version, but need to have
// all available when attempting to skip download
struct VERSION_compare
{
    bool operator()(const wstring& a, const wstring&b)
    {
        return (calculateVersionValue(a.c_str()) > calculateVersionValue(b.c_str()));
    }
};

bool getBinaryVersionTag(std::wstring sFileName, std::wstring sTagName, std::wstring& sTagValue)
{
    DWORD versionSize = GetFileVersionInfoSize(sFileName.c_str(), NULL);
    if (versionSize == 0)
    {
#ifdef _DEBUG
        wprintf(L"Couldn't load binary from: %s\n.", sFileName.c_str());
#endif  
        return false;
    }

    AutoLocalHeap<LPBYTE> versionInfoBuffer((LPBYTE)LocalAlloc(LPTR, versionSize));
    if (GetFileVersionInfo(sFileName.c_str(), NULL, versionSize, (LPVOID)versionInfoBuffer.data()) == FALSE)
    {
#ifdef _DEBUG
        wprintf(L"Couldn't load verion data from: %s\n.", sFileName.c_str());
#endif
        return false;
    }

    // Structure used to store enumerated languages and code pages
    struct LANGANDCODEPAGE {
      WORD wLanguage;
      WORD wCodePage;
    } *lpTranslate;
    UINT cbTranslate = 0;

    // Read the list of languages and code pages
    VerQueryValue((LPVOID)versionInfoBuffer.data(), L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate);

    // Read the file description for each language and code page
    for(size_t c = 0; c < (cbTranslate/sizeof(struct LANGANDCODEPAGE)); ++c)
    {
        // Construct the sub-block 'address'
        wchar_t subBlock[256] = {0};
        wsprintfW(subBlock, L"\\StringFileInfo\\%04x%04x\\%s", lpTranslate[c].wLanguage, lpTranslate[c].wCodePage, sTagName.c_str());
        
        // Retrieve the sub-block value 
        UINT cbBuffer = 0;
        LPBYTE lpbuffer = NULL;
        if (VerQueryValue((LPVOID)versionInfoBuffer.data(), subBlock, (LPVOID*)&lpbuffer, &cbBuffer) != FALSE)
        {
#ifdef _DEBUG
            wprintf(L"Found value %s = %s\n", sTagName.c_str(), (LPCWSTR)lpbuffer);
#endif
            sTagValue = (LPCWSTR)lpbuffer;
            return true;
        }
    }

    return false;
}

bool isLowerUpdateSystemVersion(std::wstring sTagValue)
{
    if (sTagValue.size() == 0)
        return false;

    long updateSystemVersion = wcstol(sTagValue.c_str(), NULL, 10);
    if (updateSystemVersion == 0)
        return false;

    if (updateSystemVersion < EBISU_UPDATE_SYSTEM_VERSION_PC)
        return true;

    return false;
}

typedef map<wstring, wstring, VERSION_compare> StagedUpdateFileMap;

void Updater::DoUpdates()
{
    wchar_t szOriginPath[MAX_PATH];
    ::GetModuleFileName(::GetModuleHandle(NULL), szOriginPath, MAX_PATH);
    std::wstring originLocation = szOriginPath;
    // Remove executable
    int lastSlash = originLocation.find_last_of(L"\\");
    originLocation = originLocation.substr(0, lastSlash + 1);

    wchar_t szProgramDataPath[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, szProgramDataPath))) 
        return;

    std::wstring strProgramDataOrigin (szProgramDataPath);
    strProgramDataOrigin.append (L"\\Origin\\SelfUpdate\\");
    wstring updateLocation = strProgramDataOrigin;

    strProgramDataOrigin.append(L"StagedUpdate\\");
    wstring stagedUpdateLocation = strProgramDataOrigin;

    ORIGINBS_LOG_EVENT << "Origin location: " << originLocation.c_str();
    ORIGINBS_LOG_EVENT << "Update location: " << updateLocation.c_str();

    // Ensure our SelfUpdate working folder exists
    int dirResult = SHCreateDirectoryEx(NULL, updateLocation.c_str(), NULL);
    if (dirResult != ERROR_SUCCESS && dirResult != ERROR_ALREADY_EXISTS)
    {
        ORIGINBS_LOG_ERROR << "DownloadUpdate: failed to create selfupdate folder: " << updateLocation.c_str() << " err = " << dirResult;
    }

    // Need to communicate with our service to see if we need to update
    // First off, what version are we, or is there a staged version here?
    
    // TODO: HANDLE DOWNGRADE SCENARIOS FOR OLDER CLIENT VERSIONS

    // Update our working DIR in case we were called from RTP
    _wchdir(updateLocation.c_str());

    // Clear out any partially downloaded files
    wchar_t partFiles[MAX_PATH];
    wsprintfW(partFiles, L"%sOriginUpdate*.zip.part", updateLocation.c_str());
    WIN32_FIND_DATAW findPartData;
    ZeroMemory(&findPartData, sizeof(findPartData));
    HANDLE handle = FindFirstFileW(partFiles, &findPartData);
    while (handle != INVALID_HANDLE_VALUE)
    {
        FindClose(handle);
        DeleteFile(findPartData.cFileName);
        handle = FindFirstFileW(partFiles, &findPartData);
    }
    FindClose(handle);

    // Look for staged update zip files in the ProgramFiles (Origin) location and move them to ProgramData
    wchar_t oldStagedFileLocation[MAX_PATH] = {0};
    wsprintfW(oldStagedFileLocation, L"%sOriginUpdate*.zip", originLocation.c_str());
    WIN32_FIND_DATAW oldStagedDownloadFileData;
    ZeroMemory(&oldStagedDownloadFileData, sizeof(oldStagedDownloadFileData));
    handle = FindFirstFileW(oldStagedFileLocation, &oldStagedDownloadFileData);

    if (handle != INVALID_HANDLE_VALUE)
    {
        ORIGINBS_LOG_EVENT << "Found staged updates in Origin folder, copying to Update folder...";

		// loop and find all update files present
        do
        {
            std::wstring oldStagedDataFullPath = originLocation + oldStagedDownloadFileData.cFileName;
            std::wstring newStagedDataFullPath = updateLocation + oldStagedDownloadFileData.cFileName;

            // Copy to the staged data folder
            ORIGINBS_LOG_EVENT << "Copying " << oldStagedDataFullPath.c_str() << " to " << newStagedDataFullPath.c_str();
            CopyFile(oldStagedDataFullPath.c_str(), newStagedDataFullPath.c_str(), FALSE);

            // Try to delete it (this might fail if the folder were already locked down, but usually won't be yet if this is an initial install)
            // If this fails, no problem because UpdateTool will take care of it eventually when it runs with elevation
            DeleteFile(oldStagedDataFullPath.c_str());
        }
		while ( FindNextFileW(handle, &oldStagedDownloadFileData));
    }
    FindClose(handle);


    wchar_t stagedFileLocation[MAX_PATH];
    wsprintfW(stagedFileLocation, L"%sOriginUpdate*.zip", updateLocation.c_str());

    wchar_t iniLocation[256];
    wsprintfW(iniLocation, L"%s%s", originLocation.c_str(), L"EACore.ini");

    char version[32] = EALS_VERSION_P_DELIMITED;
    version[strlen(EALS_VERSION_P_DELIMITED)] = 0;
    
    // If we want to impersonate the version we are running
    wchar_t debugImpersonateVersion[256];
    wchar_t debugImpersonateFromVersion[256];
    if (GetPrivateProfileString(L"Bootstrap", L"ImpersonateVersion", NULL, debugImpersonateVersion, 256, iniLocation) &&
        GetPrivateProfileString(L"Bootstrap", L"ImpersonateFromVersion", NULL, debugImpersonateFromVersion, 256, iniLocation))
    {
        char fromVersion[32] = {0};
        sprintf_s<32>(fromVersion, "%S", debugImpersonateFromVersion); // convert 'wchar_t' to 'char'

        // Compare the version we are impersonating 'from' to make sure we're still on that version
        // This is to prevent update loops, since otherwise the client would never think it had updated properly
        // Ordinarily, this would be used to impersonate from, for example, 9.5.99.4000 (pretending to be 9.5.6.719)
        // When run in this combination, the update service would tell us we need to update to, for example, 9.5.6.730.
        // After that update, our own version changes, so we no longer want to pretend to be the same version, so the
        // update can complete successfully.
        if (_strcmpi(fromVersion, version) == 0)
        {
            sprintf(version, "%S", debugImpersonateVersion); // convert 'wchar_t' to 'char'

            ORIGINBS_LOG_EVENT << "DoUpdate: EACore.ini Override client version = " << version;
        }
    }
    
    // check for client version override switch
    wchar_t* overrideVersion = wcsstr(GetCommandLine(), L"/version ");
    if (overrideVersion != 0)
    {
        overrideVersion += wcslen(L"/version ");

        wstring version_ = overrideVersion;
        int endOfVersion = version_.find_first_of(L" ");
        version_ = version_.substr(0, endOfVersion);
        sprintf(version, "%S", version_.c_str()); // convert 'wchar_t' to 'char'

        ORIGINBS_LOG_EVENT << "DoUpdate: Commandline Override client version = " << version;
    }

	bool foundStaged = false;
    wchar_t highestStagedVersion[32];
    highestStagedVersion[0] = NULL;
    StagedUpdateFileMap stagedUpdateFileMap;

    WIN32_FIND_DATAW stagedDownloadFileData;
    ZeroMemory(&stagedDownloadFileData, sizeof(stagedDownloadFileData));
    handle = FindFirstFileW(stagedFileLocation, &stagedDownloadFileData);

    if (handle != INVALID_HANDLE_VALUE)
    {
        // Use this as our version to query the service instead of installing twice
        foundStaged = true;

        ORIGINBS_LOG_EVENT << "DoUpdate: foundStaged = true";

		// loop and find all update files present - EBIBUGS-20555
        do
        {
            std::wstring stagedDataFullPath = updateLocation + stagedDownloadFileData.cFileName;
            stagedUpdateFileMap.insert(std::make_pair(extractVersionStringFromUpdateFilename(stagedDownloadFileData.cFileName), stagedDataFullPath));
        }
		while ( FindNextFileW(handle, &stagedDownloadFileData));
    }

    FindClose(handle);

    // stagedUpdateFileMap contains all staged files and are ordered from highest version to lowest.
    if(foundStaged)
    {
        ORIGINBS_LOG_EVENT << "Found " << stagedUpdateFileMap.size() << " staged files.";
        StagedUpdateFileMap::const_iterator iter = stagedUpdateFileMap.begin();
        while(iter != stagedUpdateFileMap.end())
        {
            ORIGINBS_LOG_EVENT << "Staged file: " << (iter->second) << " version: " << (iter->first);
            iter++;
        }

        // Extract highest staged version
        wcscpy_s(highestStagedVersion, 32, stagedUpdateFileMap.begin()->first.c_str());
        ORIGINBS_LOG_EVENT << "Highest staged version: " << highestStagedVersion;
    }

    wchar_t environment[256];
    wchar_t debugVersion[256];
    wchar_t debugUdateType[256];
    wchar_t debugDownloadLocation[256];
    wchar_t debugRule[256];
    wchar_t debugEulaVersion[256];
    wchar_t debugEulaLocation[256];
    wchar_t debugEulaLatestMajorVersion[256];

    wchar_t debugURL[256];

    wchar_t wVersion[BUFFER_LENGTH];
    size_t convertedChars = 0;
    mbstowcs_s(&convertedChars, wVersion, _countof(wVersion), version, _TRUNCATE);
    wVersion[convertedChars] = 0;

    DetermineEnvironment(iniLocation, environment, 256);

    // URL override
    GetPrivateProfileString(L"Bootstrap", L"URL", NULL, debugURL, 256, iniLocation);

    if (_wcsicmp(debugURL, L"") != 0)
    {
        ORIGINBS_LOG_EVENT << "Using URL override: " << debugURL;
        // Use the override
        gDownloader->setDownloadOverride(debugURL);
    }

    bool hasValidUpdateData = false;

    // If all these are present in the ini file then we should 
    if (GetPrivateProfileString(L"Bootstrap", L"Version", NULL, debugVersion, 256, iniLocation) &&
        GetPrivateProfileString(L"Bootstrap", L"Type", NULL, debugUdateType, 256, iniLocation) &&
        GetPrivateProfileString(L"Bootstrap", L"DownloadLocation", NULL, debugDownloadLocation, 256, iniLocation) &&
        GetPrivateProfileString(L"Bootstrap", L"Rule", NULL, debugRule, 256, iniLocation) &&
        GetPrivateProfileString(L"Bootstrap", L"EulaVersion", NULL, debugEulaVersion, 256, iniLocation) &&
        GetPrivateProfileString(L"Bootstrap", L"EulaLocation", NULL, debugEulaLocation, 256, iniLocation) &&
        GetPrivateProfileString(L"Bootstrap", L"EulaLatestMajorVersion", NULL, debugEulaLatestMajorVersion, 256, iniLocation))
    {
        gDownloader->ReadDebugResponse(debugVersion, debugUdateType, debugDownloadLocation, debugRule,
            debugEulaVersion, debugEulaLocation, debugEulaLatestMajorVersion);
        hasValidUpdateData = true;
    }
    else
    {
        hasValidUpdateData = gDownloader->UpdateCheck(environment, wVersion);
        mIsHasInternetConnection = gDownloader->GetInternetConnection();
    }

    int curEULAVersion = gDownloader->GetAcceptedEULAVersion();
    int newEULAVersion = _wtoi(gDownloader->getEulaVersion());
    wchar_t eulaURL[MAX_PATH];

    bool showLocalEULA = false;
    bool ignoreEulaSSLErrors = (_wcsicmp(environment, L"production") != 0);

    if (mUseEmbeddedEULA || !mIsHasInternetConnection || !hasValidUpdateData || (curEULAVersion < newEULAVersion && !gDownloader->DownloadEULA(ignoreEulaSSLErrors)))
    {
        ORIGINBS_LOG_WARNING << "Could not retrieve update information, checking against embedded EULA.";
        // If we don't have a connection or didn't hear back from the server then show our embedded EULA
        showLocalEULA = true;
        int resourceID;
        EULA::GetEULAVersion(&newEULAVersion, &resourceID);
        wsprintfW(eulaURL, L"res://%sOrigin.exe/%i/%i", originLocation.c_str(), RT_HTML, resourceID);
    }

    bool isInstalling = (wcsstr(GetCommandLine(), L"/Installing") != NULL);
    bool writeEulaVersion = false;
	bool noEula = (wcsstr(GetCommandLine(), L"/NoEULA") != NULL);
    if (!noEula && (curEULAVersion < newEULAVersion))
    {
        bool eula_pre_accepted = (StrStrIW(GetCommandLine(), L"/EULAsPreApproved") != NULL);
        if (eula_pre_accepted)
        {
            ORIGINBS_LOG_WARNING << "EULA not displayed because already accepted from free-to-play webpage";
        }
        else
        // There is a new EULA! Show it to the user
        if (showLocalEULA)
        {
            ShowEULA(eulaURL);
        }
        else
        {
            int latestMajorEULAVersion = _wtoi(gDownloader->getEulaLatestMajorVersion());

            // show EULA if
            // 1) installing Origin OR
            // 2) a major EULA version was published since we last accepted a EULA
            if( (latestMajorEULAVersion > curEULAVersion) || isInstalling )
            {
                wsprintfW(eulaURL, L"file://%s", gDownloader->getTempEULAFilePath());
                ShowEULA(eulaURL);
                gDownloader->DeleteTempEULAFile();
            }
        }

        // Keep track of EULA version for the following cases:
        // 1) If we've continued, the EULA's been accepted. Mark this in settings
        // 2) If the EULA was not shown because it was minor, write the EULA version anyway so
        //    that the correct version gets shown in the 'About' dialog.
        writeEulaVersion = true;
    }

    // Write EULA version to settings before we do anything
    bool wasUpdate = (wcsstr(GetCommandLine(), L"/Updating") != NULL);
    bool wasInstall = (wcsstr(GetCommandLine(), L"/Installing") != NULL) || wasUpdate;
    bool autoUpdate = (wcsstr(GetCommandLine(), L"/AutoUpdate") != NULL);
    bool autoPatch = (wcsstr(GetCommandLine(), L"/AutoPatchGlobal") != NULL);
    bool telemetryOptOut = (wcsstr(GetCommandLine(), L"/TelemOO") != NULL);
    wchar_t wEULAVersion[64];
    _itow_s(newEULAVersion, wEULAVersion, 64, 10);

    if (wasUpdate && gDownloader->SettingsFileFound())   // for updates only, respect the config file (if there is one!)
    {
        // respect the "AutoUpdate" setting
        if (!autoUpdate)
            autoUpdate = gDownloader->IsAutoUpdate();

        // respect the "AutoPatchGlobal" setting
        if (!autoPatch)
            autoPatch = gDownloader->IsAutoPatch();

        // respect the "TelemetryHWSurveyOptOut" setting
        if (!telemetryOptOut)
            telemetryOptOut = gDownloader->IsTelemetryOptOut();
    }

    ORIGINBS_LOG_EVENT << "calling writeSettings";
    gDownloader->WriteSettings(writeEulaVersion, wEULAVersion, gDownloader->getEulaURL(), wasInstall, autoUpdate, autoPatch, telemetryOptOut, isInstalling);

    ORIGINBS_LOG_EVENT << "versions: highest staged = " << highestStagedVersion << " downloader = " << gDownloader->getVersionNumber();
    // Now that we know what version it is that we want to update to and have shown any new EULAs
    // check to see if it is the same version of the client that we already have installed
    if (_wcsicmp(gDownloader->getDownloadUrl(), L"") == 0 && CheckForSameVersion(originLocation.c_str(), highestStagedVersion))
    {
        ORIGINBS_LOG_EVENT << "DoUpdates: installed version and stagedVersion match, no update required";
        
        // Delete all staged data
        map<wstring, wstring, VERSION_compare>::const_iterator iter = stagedUpdateFileMap.begin();
        while(iter != stagedUpdateFileMap.end())
        {
            ORIGINBS_LOG_EVENT << "Removing staged file: " << iter->second.c_str();
            DeleteFile(iter->second.c_str());
            iter++;
        }

        // Restore our working location
        _wchdir(originLocation.c_str());
        return;
    }
    if (CheckForSameVersion(originLocation.c_str(), gDownloader->getVersionNumber()))
    {
        ORIGINBS_LOG_EVENT << "DoUpdates: version numbers match, no update pending";

        // Restore our working location
        _wchdir(originLocation.c_str());
        return;
    }

    // If we're using staged data
    if (_wcsicmp(gDownloader->getDownloadUrl(), L"") == 0 && foundStaged)
    {
        ORIGINBS_LOG_EVENT << "DoUpdates: update available. version = " << highestStagedVersion;
    }
    // Otherwise we're downloading
    else if (_wcsicmp(gDownloader->getVersionNumber(), L"") != 0)
    {
        ORIGINBS_LOG_EVENT << "DoUpdates: update available. version = " << gDownloader->getVersionNumber ();
    }

    bool unpacked = false;
    bool acceptedUpdate = false;

    // Ensure our StagedUpdate working folder exists
    dirResult = SHCreateDirectoryEx(NULL, stagedUpdateLocation.c_str(), NULL);
    if (dirResult != ERROR_SUCCESS && dirResult != ERROR_ALREADY_EXISTS)
    {
        ORIGINBS_LOG_ERROR << "DownloadUpdate: failed to create selfupdate staging folder: " << stagedUpdateLocation.c_str() << " err = " << dirResult;

        // Restore our working location
        _wchdir(originLocation.c_str());
        return;
    }

    // Update our working DIR to the staged location
    _wchdir(stagedUpdateLocation.c_str());

    // We have a few different options we can use to download and apply updates. The first thing we want to do
    // is check to apply a staged update package. We will apply staged updates in the following scenarios
    // 1. If we have a staged update and it matches the update on the server
    // 2. If we have a staged update and don't see an update on the server (Either because we are testing and
    // there isn't one or we couldn't communicate with the server)
    // 3. If we attempted to download an update and failed
    // In all other cases we want to download the update that we have found online

    if (foundStaged)
    {
        StagedUpdateFileMap::const_iterator stagedUpdate;
        
        // If there's no download URL we will use the highest staged version available
        // otherwise we will only use a staged copy if it matches the download version.
        if(_wcsicmp(gDownloader->getDownloadUrl(), L"") == 0)
            stagedUpdate = stagedUpdateFileMap.begin();
        else
            stagedUpdate = stagedUpdateFileMap.find(wstring(gDownloader->getVersionNumber()));

        // Did we find valid staged data to unpack?
        if (stagedUpdate != stagedUpdateFileMap.end())
        {
            ORIGINBS_LOG_EVENT << "Updating from staged update file: " << stagedUpdate->second;

            //we want to show the update available, but even if we're not querying, we want to make sure
            //that the user is shown the login window later after the update so unset the start client minmized here
            if(gClientStartupState == kClientStartupMinimized)
            {
                gClientStartupState = kClientStartupNormal;
            }

            //Unpack
            // We want to prompt the user about the update if they don't have auto update on.
            // However, if we just came from the installer then we don't want to prompt them since
            // they know there's an update here. Also don't prompt them if OriginClient.dll isn't present
            // because declining that prompt will cause problems
            if (!gDownloader->IsAutoUpdate() && mIsFoundOrigin && !wasInstall)
            {
				acceptedUpdate = QueryUpdate(stagedUpdate->first.c_str(), environment);
                if (acceptedUpdate)
                {
                    unpacked = Unpack(stagedUpdate->second.c_str());
                }
            }
            else
            {
                unpacked = Unpack(stagedUpdate->second.c_str());
            }
        }
    }
    if (!unpacked && (_wcsicmp(gDownloader->getDownloadUrl(), L"") != 0))
    {
        //we want to show the update available, but even if we're not querying, we want to make sure
        //that the user is shown the login window later after the update so unset the start client minmized here
        if(gClientStartupState == kClientStartupMinimized)
        {
            gClientStartupState = kClientStartupNormal;
        }

        // Either we didn't have a staged file here, or we failed to unpack what was staged. Go download
        if (!gDownloader->IsAutoUpdate() && mIsFoundOrigin && !wasInstall)
        {
            // Need to query the user for this update
            // It's also possible that they accepted their update, we went to unpack it and failed and are
            // going to download again. In that case don't prompt again
			if (acceptedUpdate || QueryUpdate(gDownloader->getVersionNumber(), environment))
            {
                // The user said it's ok to download
                mIsDownloadSuccessful = DownloadUpdate();
            }
        }
        else
        {
            if (!mIsFoundOrigin && !wasInstall)
            {
                ORIGINBS_LOG_EVENT << "Initiating download because we couldn't find OriginClient.dll";
            }
            // There is a download. Download it
            mIsDownloadSuccessful = DownloadUpdate();
        }
        // If we failed to download and have a staged file unpack it
        if (foundStaged && !mIsDownloadSuccessful)
        {
            unpacked = Unpack(stagedUpdateFileMap.begin()->second.c_str());
        }
        // If we had a staged file that we didn't unpack then delete it
        if (foundStaged && mIsDownloadSuccessful)
        {
            // If we found a staged patch we should delete it if we failed to unpack it
            // Delete all staged data
            StagedUpdateFileMap::const_iterator iter = stagedUpdateFileMap.begin();
            while(iter != stagedUpdateFileMap.end())
            {
                ORIGINBS_LOG_EVENT << "Removing staged file: " << iter->second.c_str();
                DeleteFile(iter->second.c_str());
                iter++;
            }
        }
    }

    // Restore our working DIR to the update folder
    _wchdir(updateLocation.c_str());

    // Build staged UpdateTool location
    wchar_t stagedUpdateToolLocation[MAX_PATH+1] = {0};
    wnsprintfW(stagedUpdateToolLocation, MAX_PATH, L"%sUpdateTool.exe", stagedUpdateLocation.c_str());

    if (unpacked || mIsDownloadSuccessful)
    {
        // We're not up to date
        mIsUpToDate = false;

        // We've unpacked the version we're going to use already, so we don't need any other staged versions hanging around
        // Delete all staged versions
        StagedUpdateFileMap::const_iterator iter = stagedUpdateFileMap.begin();
        while(iter != stagedUpdateFileMap.end())
        {
            if (DeleteFile(iter->second.c_str()))
            {
                ORIGINBS_LOG_EVENT << "Removed unused staged file: " << iter->second.c_str();
            }
            iter++;
        }

        // Delete the existing UpdateTool log since we're doing a new update
        wstring sUpdateToolLogFilename(szProgramDataPath);
        sUpdateToolLogFilename.append(L"\\Logs\\UpdateTool_Log.txt");
        DeleteFile(sUpdateToolLogFilename.c_str());

        // Need to check OS version. If Vista or higher we want to use runas to invoke UAC.
        bool isVistaOrHigher = false;
        OSVERSIONINFO	osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (GetVersionEx(&osvi))
        {
            if (osvi.dwMajorVersion >= 6)
            {
                isVistaOrHigher = true;
            }
        }

        // Lock the file we'll signature check before we do it, so as to avoid time-of-check/time-of-use problems
        Auto_FileLock fileLock(stagedUpdateToolLocation);
        if (!fileLock.acquired())
        {
            ORIGINBS_LOG_ERROR << "DoUpdates: unable to acquire lock on UpdateTool.exe, can't continue";
                    
            return;
        }

#if defined(CHECK_SIGNATURES)
        // Verify the signature of UpdateTool
        if (!isValidEACertificate(stagedUpdateToolLocation))
        {
            ORIGINBS_LOG_ERROR << "DoUpdates: unable to verify signature of UpdateTool.exe, can't continue";
                    
            return;
        }
#endif

        // Examine the staged update to see if it is compatible with this update system, or whether it is a downgrade
        bool didDowngrade = false;
        std::wstring tagValue;
        if (getBinaryVersionTag(stagedUpdateLocation + std::wstring(L"OriginTMP.exe"), L"UpdateSystemVersion", tagValue))
        {
            ORIGINBS_LOG_EVENT << "DoUpdates: Staged update uses Update System version: " << tagValue << " Current Update System version: " << EBISU_UPDATE_SYSTEM_VERSION_PC_STR;

            if (isLowerUpdateSystemVersion(tagValue))
            {
                ORIGINBS_LOG_EVENT << "DoUpdates: Staged update is lower Update System version.  Performing downgrade.";

                // Build Origin-folder UpdateTool location
                wchar_t installedUpdateToolLocation[MAX_PATH + 1] = { 0 };
                wnsprintfW(installedUpdateToolLocation, MAX_PATH, L"%sUpdateTool.exe", originLocation.c_str());

                wchar_t installedUpdateToolArgs[MAX_PATH + 1] = { 0 };
                wnsprintfW(installedUpdateToolArgs, MAX_PATH, L"/PrepareDowngrade UpdateSystem=%s", tagValue.c_str());

                // Run the currently installed UpdateTool.exe to unlock the folder
                SHELLEXECUTEINFO  shExInfo = { 0 };
                shExInfo.cbSize = sizeof(shExInfo);
                shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
                shExInfo.lpVerb = isVistaOrHigher ? L"runas" : L"";
                shExInfo.lpFile = installedUpdateToolLocation;
                shExInfo.lpParameters = installedUpdateToolArgs;
#ifdef _DEBUG
                shExInfo.nShow = SW_SHOW;
#else
                shExInfo.nShow = SW_HIDE;
#endif

                ORIGINBS_LOG_EVENT << "Calling installed UpdateTool (ELEVATED) from: " << shExInfo.lpFile << " with args: " << shExInfo.lpParameters;

                if (ShellExecuteEx(&shExInfo))
                {
                    WaitForSingleObject(shExInfo.hProcess, INFINITE);
                    CloseHandle(shExInfo.hProcess);
                }
                else
                {
                    int err = GetLastError();
                    if (err == ERROR_CANCELLED)
                    {
                        ORIGINBS_LOG_ERROR << "DoUpdates: declined downgrade UAC, can't continue";

                        LogService::releaseAndExitProcess();    //need to release LogService BEFORE exiting
                    }
                    else
                    {
                        ORIGINBS_LOG_ERROR << "Failed to launch UpdateTool for downgrade: err =" << err << ", args =[" << shExInfo.lpParameters << "]";

                        LogService::releaseAndExitProcess();    //need to release LogService BEFORE exiting
                    }
                }
            }
        }
        else
        {
            ORIGINBS_LOG_EVENT << "DoUpdates: Staged update is an older update system, need to prepare for a downgrade.";

            // Release the lock since we'll be replacing UpdateTool anyway and the older update system is not aware of the new process
            fileLock.release();

            // Build Origin-folder UpdateTool location
            wchar_t installedUpdateToolLocation[MAX_PATH+1] = {0};
            wchar_t installedUpdateToolOldLocation[MAX_PATH+1] = {0};
            wnsprintfW(installedUpdateToolLocation, MAX_PATH, L"%sUpdateTool.exe", originLocation.c_str());
            wnsprintfW(installedUpdateToolOldLocation, MAX_PATH, L"%sUpdateToolOLD.exe", originLocation.c_str());

            wchar_t installedUpdateToolArgs[MAX_PATH+1] = {0};
            wnsprintfW(installedUpdateToolArgs, MAX_PATH, L"/Downgrade");

            // Run the currently installed UpdateTool.exe to unlock the folder
            SHELLEXECUTEINFO  shExInfo = {0};
            shExInfo.cbSize = sizeof(shExInfo);
            shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
            shExInfo.lpVerb = isVistaOrHigher ? L"runas" : L"";
            shExInfo.lpFile = installedUpdateToolLocation;
            shExInfo.lpParameters = installedUpdateToolArgs;
#ifdef _DEBUG
            shExInfo.nShow = SW_SHOW;
#else
            shExInfo.nShow = SW_HIDE;
#endif

            ORIGINBS_LOG_EVENT << "Calling installed UpdateTool (ELEVATED) from: " << shExInfo.lpFile << " with args: " << shExInfo.lpParameters;

            if (ShellExecuteEx(&shExInfo))
            {
                WaitForSingleObject(shExInfo.hProcess, INFINITE);
                CloseHandle(shExInfo.hProcess);
                DeleteFile(installedUpdateToolOldLocation);
                DeleteFile(stagedUpdateToolLocation);
                RemoveDirectory(stagedUpdateLocation.c_str());

                // Replace the staged update tool location with the actual Origin folder, since that is the one we will call now
                wnsprintfW(stagedUpdateToolLocation, MAX_PATH, L"%sUpdateTool.exe", originLocation.c_str());
                didDowngrade = true;
            }
            else
            {
                int err = GetLastError ();
                if (err == ERROR_CANCELLED)
                {
                    ORIGINBS_LOG_ERROR << "DoUpdates: declined downgrade UAC, can't continue";
                    
                    LogService::releaseAndExitProcess();    //need to release LogService BEFORE exiting
                }
                else
                {
                    ORIGINBS_LOG_ERROR <<"Failed to launch UpdateTool for downgrade: err =" << err << ", args =[" << shExInfo.lpParameters << "]";

                    LogService::releaseAndExitProcess();    //need to release LogService BEFORE exiting
                }
            }
        }

        // Call our UpdateTool to update some registry keys. Run it elevated
        bool isBeta = (wcscmp(gDownloader->getDownloadType(), L"BETA") == 0);
        const static size_t BUF_SIZE = 64;
        wchar_t args[BUF_SIZE+1] = {0};
        if (mIsDownloadSuccessful)
        {
            wcscpy_s(mUpdated, BUFFER_LENGTH, gDownloader->getVersionNumber());
            wnsprintfW(args, BUF_SIZE, L"%s /Beta:%s /Version:%s", !didDowngrade ? L"/SelfUpdate" : L"",  isBeta ? L"true" : L"false", gDownloader->getVersionNumber());
        }
        else //unpacked
        {
            wcscpy_s(mUpdated, BUFFER_LENGTH, highestStagedVersion);
            wnsprintfW(args, BUF_SIZE, L"%s /Beta:%s /Version:%s", !didDowngrade ? L"/SelfUpdate" : L"",  isBeta ? L"true" : L"false", highestStagedVersion);
        }

        SHELLEXECUTEINFO  shExInfo = {0};
        shExInfo.cbSize = sizeof(shExInfo);
        shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        shExInfo.lpVerb = isVistaOrHigher ? L"runas" : L"";
        shExInfo.lpFile = stagedUpdateToolLocation;
        shExInfo.lpParameters = args;
#ifdef _DEBUG
        shExInfo.nShow = SW_SHOW;
#else
        shExInfo.nShow = SW_HIDE;
#endif

        ORIGINBS_LOG_EVENT << "Calling UpdateTool (ELEVATED) from: " << shExInfo.lpFile << " with args: " << args;

        if (ShellExecuteEx(&shExInfo))
        {
            // Release the lock on UpdateTool
            fileLock.release();

            WaitForSingleObject(shExInfo.hProcess, INFINITE);
            CloseHandle(shExInfo.hProcess);
            DeleteFile((updateLocation + std::wstring(L"UAC.txt")).c_str());
            if (!didDowngrade)
            {
                DeleteFile(stagedUpdateToolLocation);
                RemoveDirectory(stagedUpdateLocation.c_str());
            }
        }
        else
        {
            // Release the lock on UpdateTool
            fileLock.release();

            int err = GetLastError ();
            if (err == ERROR_CANCELLED)
            {
                ORIGINBS_LOG_ERROR << "DoUpdates: declined initial UAC";
                // If they declined the UAC prompt, write a file to disk so we can prompt again on start
                HANDLE hFile = CreateFile(L"UAC.txt", (GENERIC_READ | GENERIC_WRITE),
                    0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
                DWORD written;
                char aArgs[64];
                WideCharToMultiByte(CP_UTF8, 0, args, 64, aArgs, 64, NULL, NULL);
                WriteFile(hFile, aArgs, wcslen(args) * sizeof(char), &written, NULL);
                CloseHandle(hFile);
                LogService::releaseAndExitProcess();    //need to release LogService BEFORE exiting
            }
            else
            {
                ORIGINBS_LOG_ERROR <<"Failed to launch UpdateTool: err =" << err << ", args =[" << args << "]";
            }
        }
    }
    else
    {
        // Check to see if we have a UAC prompt file waiting here
        wchar_t args[64];
        HANDLE hFile = CreateFile(L"UAC.txt", (GENERIC_READ | GENERIC_WRITE),
            0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN, NULL);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            // Lock the file we'll signature check before we do it, so as to avoid time-of-check/time-of-use problems
            Auto_FileLock fileLock(stagedUpdateToolLocation);
            if (!fileLock.acquired())
            {
                ORIGINBS_LOG_ERROR << "DoUpdates: unable to acquire lock on UpdateTool.exe, can't continue";
                    
                return;
            }

#if defined(CHECK_SIGNATURES)
            // Verify the signature of UpdateTool
            if (!isValidEACertificate(stagedUpdateToolLocation))
            {
                ORIGINBS_LOG_ERROR << "DoUpdates: unable to verify signature of UpdateTool.exe, can't continue";
                    
                return;
            }
#endif

            char buffer[64];
            DWORD read;
            ReadFile(hFile, buffer, 64, &read, NULL);
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, buffer, read, args, read);
            CloseHandle(hFile);
            args[read] = 0;
            // Need to check OS version. If Vista or higher we want to use runas to invoke UAC.
            bool isVistaOrHigher = false;
            OSVERSIONINFO	osvi;
            ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
            osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
            if (GetVersionEx(&osvi))
            {
                if (osvi.dwMajorVersion >= 6)
                {
                    isVistaOrHigher = true;
                }
            }

            SHELLEXECUTEINFO  shExInfo = {0};
            shExInfo.cbSize = sizeof(shExInfo);
            shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
            shExInfo.lpVerb = isVistaOrHigher ? L"runas" : L"";
            shExInfo.lpFile = stagedUpdateToolLocation;
            shExInfo.lpParameters = args;
#ifdef _DEBUG
            shExInfo.nShow = SW_SHOW;
#else
            shExInfo.nShow = SW_HIDE;
#endif
            ORIGINBS_LOG_EVENT << "Calling UpdateTool (ELEVATED) from: " << shExInfo.lpFile << " with args: " << args;

            if (ShellExecuteEx(&shExInfo))
            {
                // Release the lock on UpdateTool
                fileLock.release();

                WaitForSingleObject(shExInfo.hProcess, INFINITE);
                CloseHandle(shExInfo.hProcess);
                DeleteFile((updateLocation + std::wstring(L"UAC.txt")).c_str());
                DeleteFile(stagedUpdateToolLocation);
                RemoveDirectory(stagedUpdateLocation.c_str());
            }
            else
            {
                // Release the lock on UpdateTool
                fileLock.release();

                int err = GetLastError ();
                if (err == ERROR_CANCELLED)
                {          
                    ORIGINBS_LOG_ERROR << "DoUpdates: re-declined UAC";
                    LogService::releaseAndExitProcess();    //need to release LogService BEFORE exiting
                }
                else
                {
                    ORIGINBS_LOG_ERROR <<"Failed to launch UpdateTool: err =" << err << ", args =[" << args << "]";
                }
            }
        }
        else
        {
            // Restore our working location
            _wchdir(originLocation.c_str());

            ORIGINBS_LOG_EVENT << "DoUpdates: no update available.";

            // There is no UAC.txt file and we didn't download or unpack an update, so delete the staged folder
            RemoveDirectory(stagedUpdateLocation.c_str());

            return;
        }
    }

    // Restore our working location
    _wchdir(originLocation.c_str());
}

bool Updater::CheckForSameVersion(const wchar_t* location, const wchar_t* versionToCheck)
{
    // Do we already have an OriginClient.dll that matches the version we want to update to? If so then skip the update
    wchar_t originDLL[MAX_PATH];
    wsprintfW(originDLL, L"%sOriginClient.dll", location);
    WIN32_FIND_DATAW findOrigin;
    ZeroMemory(&findOrigin, sizeof(findOrigin));
    HANDLE handle = FindFirstFile(originDLL, &findOrigin);

    if (handle != INVALID_HANDLE_VALUE)
    {
        mIsFoundOrigin = true;
        FindClose(handle);
        // What version is this?
        DWORD hnd;
        DWORD size = GetFileVersionInfoSize(findOrigin.cFileName, &hnd);

        // If we can't get version info then assume we didn't find OriginClient.dll
        if (size == 0)
        {
            ORIGINBS_LOG_ERROR << "Couldn't find a valid OriginClient.dll to use";
            mIsFoundOrigin = false;
        }
        else
        {
            BYTE* versionInfo = new BYTE[size];
            if (GetFileVersionInfo(findOrigin.cFileName, hnd, size, versionInfo))
            {
                VS_FIXEDFILEINFO*   vsfi = NULL;
                UINT len = 0;
                VerQueryValue(versionInfo, L"\\", (void**)&vsfi, &len);
                WORD first = HIWORD(vsfi->dwFileVersionMS);
                WORD second = LOWORD(vsfi->dwFileVersionMS);
                WORD third = HIWORD(vsfi->dwFileVersionLS);
                WORD fourth = LOWORD(vsfi->dwFileVersionLS);

                wchar_t wVersionInfo[16];
                wsprintfW(wVersionInfo, L"%hu.%hu.%hu.%hu", first, second, third, fourth);
                ORIGINBS_LOG_EVENT << "CheckForSameVersion: installed version = " << wVersionInfo;
                if (wcscmp(wVersionInfo, versionToCheck) == 0)
                {
                    delete[] versionInfo;

                    return true;
                }
            }
            delete[] versionInfo;
        }
    }

    return false;
}


class CBrowserSink : public DWebBrowserEvents2 
{
public:
    STDMETHODIMP QueryInterface(REFIID riid,void **ppv);
    STDMETHODIMP GetTypeInfoCount(UINT *pctinfo);
    STDMETHODIMP GetTypeInfo(UINT iTInfo,LCID lcid,ITypeInfo **ppTInfo);
    STDMETHODIMP GetIDsOfNames(REFIID riid,LPOLESTR *rgszNames,UINT cNames,LCID lcid,DISPID *rgDispId);
    STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
};

static CBrowserSink sEULAEventSink;

STDMETHODIMP CBrowserSink::QueryInterface(REFIID riid, void **ppv)
{
    if (!ppv)
        return E_POINTER;

    if ((riid == IID_IUnknown) || (riid == IID_IDispatch) || (riid == DIID_DWebBrowserEvents2))
    {
        *ppv = (void*) &sEULAEventSink;
    } 
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return S_OK;
}

STDMETHODIMP_(ULONG) CBrowserSink::AddRef()
{
    return 1;
}

STDMETHODIMP_(ULONG) CBrowserSink::Release()
{
    return 1;
}

STDMETHODIMP CBrowserSink::GetTypeInfoCount(UINT *pctinfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP CBrowserSink::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP CBrowserSink::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    return E_NOTIMPL;
}

// trap events from the ActiveX browser
STDMETHODIMP CBrowserSink::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,UINT *puArgErr)
{
    if (riid != IID_NULL)
        return DISP_E_UNKNOWNINTERFACE; // riid should always be IID_NULL

    if (dispIdMember == DISPID_BEFORENAVIGATE2)
    {	// Trap the BeforeNavigate2 event for the url
        VARIANT v;
        VariantInit(&v);
        VariantChangeType(&v, &pDispParams->rgvarg[5], 0, VT_BSTR); // convert url to string

        // if there is a link, open it in the user's default browser
        if(v.vt != VT_EMPTY)
            ShellExecute(NULL, L"open", v.bstrVal, NULL, NULL, SW_SHOWNORMAL);

        *(pDispParams->rgvarg[0].pboolVal) = VARIANT_TRUE;	// this signals the ActiveX browser to not open it

        VariantClear(&v);
    }

    return S_OK;
}

void Updater::ShowEULA(wchar_t* url)
{
    HWND hEULADialog = CreateDialog(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_EULADIALOG), NULL, reinterpret_cast<DLGPROC>(DlgProcEULA));
    DWORD m_dwCustCookie = 0;

    CAxWindow eulaWindow(GetDlgItem(hEULADialog, IDC_EXPLORER));
    CComPtr<IUnknown> punkCtrl;
    eulaWindow.CreateControlEx ( L"{8856F961-340A-11D0-A96B-00C04FD705A2}", NULL, NULL, &punkCtrl );
    CComQIPtr<IWebBrowser2> pWB2;
    pWB2 = punkCtrl;
    if (pWB2)
    {
        HRESULT result = pWB2->Navigate(CComBSTR(url),0,0,0,0);
        if (result != S_OK)
        {
            ORIGINBS_LOG_ERROR << "Unable to navigate to EULA URL. error = " << result;
        }
        else
        {	// setup sink object to capture attempts to click on links in the EULA 
            LPUNKNOWN m_pSinkUnk;
            HRESULT hr;
            // Create sink object that implements the event interface methods.
            CBrowserSink *pSinkClass = &sEULAEventSink;
            hr = pSinkClass->QueryInterface (IID_IUnknown, (LPVOID*)&m_pSinkUnk);
            if (SUCCEEDED (hr))
            {
                IConnectionPointContainer* pCPC =NULL;
                hr = pWB2->QueryInterface(IID_IConnectionPointContainer, reinterpret_cast<void**>(&pCPC));
                if (SUCCEEDED(hr)) 
                {
                    IConnectionPoint* pCP = NULL;
                    hr = pCPC->FindConnectionPoint(DIID_DWebBrowserEvents2, &pCP);
                    if (SUCCEEDED(hr)) 
                    {
                        pCP->Advise(pSinkClass, &m_dwCustCookie);
                        pCP->Release();
                    }
                    pCPC->Release();    
                }
            }
        }
    }

    // An update was found, show the EULA
    HDC hdc;
    long lfHeight;
    hdc = GetDC(NULL);
    lfHeight = -MulDiv(12, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    HFONT title = CreateFont(lfHeight, 0, 0, 0, FW_DONTCARE, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, 
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    ReleaseDC(NULL, hdc);
    SendMessage(GetDlgItem(hEULADialog, IDC_TITLE), WM_SETFONT, WPARAM(title), TRUE);
    SendMessage(hEULADialog, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)hIcon);

    // Localize text
    wchar_t* buffer = new wchar_t[BUFFER_LENGTH];
    wchar_t* retBuffer = new wchar_t[BUFFER_LENGTH];
    GetDlgItemText(hEULADialog, IDC_TITLE, buffer, BUFFER_LENGTH);
    Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
    SetDlgItemText(hEULADialog, IDC_TITLE, retBuffer);
    GetDlgItemText(hEULADialog, IDC_TEXT, buffer, BUFFER_LENGTH);
    Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
    SetDlgItemText(hEULADialog, IDC_TEXT, retBuffer);
    GetDlgItemText(hEULADialog, IDC_CHECK1, buffer, BUFFER_LENGTH);
    Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
    SetDlgItemText(hEULADialog, IDC_CHECK1, retBuffer);
    GetDlgItemText(hEULADialog, IDOK, buffer, BUFFER_LENGTH);
    Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
    SetDlgItemText(hEULADialog, IDOK, retBuffer);
    GetDlgItemText(hEULADialog, IDCANCEL, buffer, BUFFER_LENGTH);
    Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
    SetDlgItemText(hEULADialog, IDCANCEL, retBuffer);
    GetDlgItemText(hEULADialog, IDC_PRINT, buffer, BUFFER_LENGTH);
    Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
    SetDlgItemText(hEULADialog, IDC_PRINT, retBuffer);
    GetWindowText(hEULADialog, buffer, BUFFER_LENGTH);
    Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
    SetWindowText(hEULADialog, retBuffer);
    delete[] retBuffer;
    delete[] buffer;

    wchar_t *eulaLocale = Locale::GetInstance()->GetEulaLocale();
    if (wcscmp(eulaLocale, L"de_DE") == 0 ||
        wcscmp(eulaLocale, L"en_DE") == 0 ||
        wcscmp(eulaLocale, L"de_EU") == 0 ||
        wcscmp(eulaLocale, L"de_RW") == 0)
    {
        // Since the German acceptance string is MASSIVE we need to adjust the window
        RECT rect;
        rect.left = 0; rect.right = 260; rect.top = 0; rect.bottom = 9;
        MapDialogRect(hEULADialog, &rect);
        int height = rect.bottom;
        rect.left = 0; rect.right = 260; rect.top = 0; rect.bottom = 40;
        MapDialogRect(hEULADialog, &rect);
        SetWindowPos(GetDlgItem(hEULADialog, IDC_CHECK1), NULL, 0, 0, rect.right, rect.bottom, SWP_NOMOVE);

        int offset = rect.bottom - height;
        // Need to adjust everything else by this amount
        GetWindowRect(GetDlgItem(hEULADialog, IDC_PRINT), &rect);
        MapWindowPoints(NULL, hEULADialog, (LPPOINT)&rect, 2);
        SetWindowPos(GetDlgItem(hEULADialog, IDC_PRINT), NULL, rect.left, rect.top + offset, 0, 0, SWP_NOSIZE);

        GetWindowRect(GetDlgItem(hEULADialog, IDOK), &rect);
        MapWindowPoints(NULL, hEULADialog, (LPPOINT)&rect, 2);
        SetWindowPos(GetDlgItem(hEULADialog, IDOK), NULL, rect.left, rect.top + offset, 0, 0, SWP_NOSIZE);

        GetWindowRect(GetDlgItem(hEULADialog, IDCANCEL), &rect);
        MapWindowPoints(NULL, hEULADialog, (LPPOINT)&rect, 2);
        SetWindowPos(GetDlgItem(hEULADialog, IDCANCEL), NULL, rect.left, rect.top + offset, 0, 0, SWP_NOSIZE);

        GetWindowRect(GetDlgItem(hEULADialog, IDC_BOTTOM), &rect);
        MapWindowPoints(NULL, hEULADialog, (LPPOINT)&rect, 2);
        SetWindowPos(GetDlgItem(hEULADialog, IDC_BOTTOM), NULL, rect.left, rect.top + offset, 0, 0, SWP_NOSIZE);

        GetWindowRect(hEULADialog, &rect);
        SetWindowPos(hEULADialog, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top + offset, SWP_NOMOVE);
    }

    //if we're showing the EULA then set subsequent minimize to false (unless user minimizes at which point it will get set to true)
    if(gClientStartupState == kClientStartupMinimized)
    {
        gClientStartupState = kClientStartupNormal;
    }
    ShowWindow(hEULADialog, SW_SHOW);

    MSG msg;	
    bool done = false;
    bool accepted = false;
	while(!done && GetMessage( &msg, NULL, 0, 0 ) > 0)
	{
		if (!IsDialogMessage(hEULADialog, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			switch (msg.message)
			{
			case EULA_ACCEPTED:
				{
					accepted = true;
					done = true;
				}
				break;
			case EULA_DECLINED:
				{
					accepted = false;
					done = true;
				}
				break;
			case EULA_CHECKED:
				{
					EnableWindow(GetDlgItem(hEULADialog, IDOK), 1);
				}
				break;
			case EULA_UNCHECKED:
				{
					EnableWindow(GetDlgItem(hEULADialog, IDOK), 0);
				}
				break;
			case EULA_PRINT:
				{
					ShellExecuteW(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);
				}
				break;
			}
		}
	}

    if (pWB2)	// clean up sink object to not capture events
    {
        IConnectionPointContainer* pCPC =NULL;
        HRESULT hr = pWB2->QueryInterface(IID_IConnectionPointContainer, reinterpret_cast<void**>(&pCPC));
        if (SUCCEEDED(hr)) 
        {
            IConnectionPoint* pCP = NULL;
            hr = pCPC->FindConnectionPoint(DIID_DWebBrowserEvents2, &pCP);
            if (SUCCEEDED(hr)) 
            {
                pCP->Unadvise(m_dwCustCookie);
                pCP->Release();
            }
            pCPC->Release();    
        }
    }

    EndDialog(hEULADialog, 0);
    if (!accepted)
    {
        ORIGINBS_LOG_EVENT << "EULA not accepted";
        gDownloader->DeleteTempEULAFile();
        LogService::releaseAndExitProcess();    //need to release LogService BEFORE exiting
    }
}

bool Updater::DownloadUpdate()
{
    hDownloadDialog = CreateDialog(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOGBAR), NULL, reinterpret_cast<DLGPROC>(DlgProcProgress));

    // Download
    const wchar_t* downloadURL = gDownloader->getDownloadUrl();
    SendMessage(hDownloadDialog, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)hIcon);
    SendMessage(GetDlgItem(hDownloadDialog, IDC_PROGRESS), PBM_SETBARCOLOR, 0, LPARAM(0xE3A92A));

    // Font
    HDC hdc;
    long lfHeight;
    hdc = GetDC(NULL);
    lfHeight = -MulDiv(12, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    HFONT title = CreateFont(lfHeight, 0, 0, 0, FW_DONTCARE, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, 
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    ReleaseDC(NULL, hdc);
    SendMessage(GetDlgItem(hDownloadDialog, IDC_TITLE), WM_SETFONT, WPARAM(title), TRUE);

    // Localize text
    wchar_t* buffer = new wchar_t[BUFFER_LENGTH];
    wchar_t* retBuffer = new wchar_t[BUFFER_LENGTH];
    GetWindowText(hDownloadDialog, buffer, BUFFER_LENGTH);
    Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
    SetWindowText(hDownloadDialog, retBuffer);
    GetDlgItemText(hDownloadDialog, IDC_TITLE, buffer, BUFFER_LENGTH);
    Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
    SetDlgItemText(hDownloadDialog, IDC_TITLE, retBuffer);
    GetDlgItemText(hDownloadDialog, IDCANCEL, buffer, BUFFER_LENGTH);
    Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
    SetDlgItemText(hDownloadDialog, IDCANCEL, retBuffer);
    GetDlgItemText(hDownloadDialog, IDC_TIMELEFT, buffer, BUFFER_LENGTH);
    Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
    SetDlgItemText(hDownloadDialog, IDC_TIMELEFT, retBuffer);
    GetDlgItemText(hDownloadDialog, IDC_TRANSFERRATE, buffer, BUFFER_LENGTH);
    Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
    SetDlgItemText(hDownloadDialog, IDC_TRANSFERRATE, retBuffer);
    delete[] buffer;
    delete[] retBuffer;

    HANDLE hDownloadThread = CreateThread(NULL, 0, downloadUpdateAsynch, &downloadURL, 0, NULL);

    MSG msg;
    bool done = false;
    bool isUpdated = false;
    bool dispatch = true;
    while(!done && GetMessage( &msg, NULL, 0, 0 ) > 0)
    {
        switch (msg.message)
        {
        case WM_SYSCOMMAND:
            {
                if ( msg.wParam == SC_RESTORE) //maximizing window
                {
                    if(gClientStartupState == kClientStartupMinimized)
                    {
                        gClientStartupState = kClientStartupNormal;    //so that subsequent windows are no longer minimized
                }
                }
                else if (msg.wParam == SC_MINIMIZE)
                {
                    //the initital detection happens in the dlgproc itself, and that posts a message so that we can set the member flag in this class
                    //but we don't want to dispatch the message again down to the dlgproc, so use the kludge dispatch flag
                    gClientStartupState = kClientStartupMinimized;
                    dispatch = false;       //so it doesn't get passed down to the dlg proc which then passes it back up here
                }
            }
            break;

        case DOWNLOAD_SUCCEEDED:
            {
                EndDialog(hDownloadDialog, 0);
                ORIGINBS_LOG_EVENT << "DownloadUpdate: update downloaded successfully";
                isUpdated = Unpack(gDownloader->getOutputFileName());
                done = true;
            }
            break;
        case DOWNLOAD_FAILED:
            {
                done = true;

                // If we failed a mandatory update, log in offline.
                if (!gDownloader->isOptionalUpdate())
                {
                    mIsOfflineMode = true;
                }
            }
            break;
        case DOWNLOAD_NOT_ENOUGH_SPACE:
            {
                done = true;
                if (!gDownloader->isOptionalUpdate() || !mIsFoundOrigin)
                {
                    // If this wasn't an optional update, exit the application
                    LogService::releaseAndExitProcess();    //need to release LogService BEFORE exiting
                }
            }
            break;
        case DOWNLOAD_CANCELED:
            {
                done = true;
                EndDialog(hDownloadDialog, 0);
                gDownloader->UserCancelDownload();
                ORIGINBS_LOG_EVENT << "DownloadUpdate: user cancelled download";
                // Wait for the download thread to clean up before we delete this file
                WaitForSingleObject(hDownloadThread, 1000);
                DeleteFile(gDownloader->getOutputFileName());
                if (! gDownloader->isOptionalUpdate() || !mIsFoundOrigin)
                {
                    // If this wasn't an optional update, canceling exits the application
                    LogService::releaseAndExitProcess();    //need to release LogService BEFORE exiting
                }
            }
            break;
        }
        if (dispatch)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        dispatch = true;    //restore for next time
    }

    return isUpdated;
}

bool MakeTempFilePath(wchar_t* tempFilePathBuf, int bufSize, wchar_t* tempFileName)
{
     wchar_t tempDirectoryPath[MAX_PATH];

     if (GetTempPath(bufSize, tempDirectoryPath))
     {
          wsprintfW(tempFilePathBuf, L"%s%s", tempDirectoryPath, tempFileName);
          return true;
     }
     else
     {
         return false;
     }
}

bool Updater::QueryUpdate(const wchar_t* version, const wchar_t* environment)
{
    HWND hQuery;
    if (gDownloader->isOptionalUpdate())
    {
        hQuery = CreateDialog(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_OPTIONALUPDATE), NULL, reinterpret_cast<DLGPROC>(DlgProcUpdate));
    }
    else
    {
        hQuery = CreateDialog(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_REQUIREDUPDATE), NULL, reinterpret_cast<DLGPROC>(DlgProcUpdate));
    }

    HDC hdc;
    long lfHeight;
    hdc = GetDC(NULL);
    lfHeight = -MulDiv(12, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    HFONT title = CreateFont(lfHeight, 0, 0, 0, FW_DONTCARE, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, 
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    ReleaseDC(NULL, hdc);
    SendMessage(GetDlgItem(hQuery, IDC_TITLE), WM_SETFONT, WPARAM(title), TRUE);

    CAxWindow dialogWindow(GetDlgItem(hQuery, IDC_PATCHNOTES));
    CComPtr<IUnknown> punkCtrl;
    dialogWindow.CreateControlEx ( L"{8856F961-340A-11D0-A96B-00C04FD705A2}", NULL, NULL, &punkCtrl );
    CComQIPtr<IWebBrowser2> pWB2;
    pWB2 = punkCtrl;
    if (pWB2)
    {
        wchar_t url[512];
        wstring patchVersion(version);
        int index = patchVersion.find_last_of(L".");
        patchVersion = patchVersion.substr(0, index);

        if (wcscmp(environment, L"production") == 0)
        {
            wsprintfW(url, L"https://secure.download.dm.origin.com/releasenotes/%s/%s.html", patchVersion.c_str(), Locale::GetInstance()->GetSystemLocale());
        }
        else
        {
            wsprintfW(url, L"https://stage.secure.download.dm.origin.com/releasenotes/%s/%s.html", patchVersion.c_str(), Locale::GetInstance()->GetSystemLocale());
        }

        wchar_t releaseNotesPath[MAX_PATH];

        if(!MakeTempFilePath(releaseNotesPath, MAX_PATH, L"relnotes.html"))
        {
            ORIGINBS_LOG_ERROR << "Failed to create temporary release notes file path [last error: " << GetLastError() << "]";
        }
        else
        {
            HANDLE tempFileHandle = CreateFile(releaseNotesPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            
            if(tempFileHandle == INVALID_HANDLE_VALUE)
            {
                ORIGINBS_LOG_ERROR << "Failed to create temporary release notes file [last error: " << GetLastError() << "]";
            }
            else
            {
                bool ignoreSSLErrors = (_wcsicmp(environment, L"production") != 0);
                bool downloaded = gDownloader->DownloadFile(url, tempFileHandle, ignoreSSLErrors);

                CloseHandle(tempFileHandle);

                if(!downloaded)
                {
                    ORIGINBS_LOG_ERROR << "Failed to download release notes, attempting to load directly.";

                    // Let's try once more with the old direct load approach:
                    HRESULT result = pWB2->Navigate(CComBSTR(url),0,0,0,0);

                    if (result != S_OK)
                    {
                        ORIGINBS_LOG_ERROR << "Unable to navigate to release notes url. err = " << result;
                    }
                }
                else
                {   
                    ORIGINBS_LOG_EVENT << "Successfully downloaded release notes from URL: " << url;

                    wsprintfW(url, L"file:///%s", releaseNotesPath);
                    HRESULT result = pWB2->Navigate(CComBSTR(url),0,0,0,0);

                    if (result != S_OK)
                    {
                        ORIGINBS_LOG_ERROR << "Unable to navigate to release notes url. err = " << result;
                    }
                }

                DeleteFile(releaseNotesPath);
            }
        }

    }

    SendMessage(hQuery, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)hIcon);

    // Localize text
    wchar_t* buffer = new wchar_t[BUFFER_LENGTH];
    wchar_t* retBuffer = new wchar_t[BUFFER_LENGTH];
    GetDlgItemText(hQuery, IDC_TITLE, buffer, BUFFER_LENGTH);
    Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
    SetDlgItemText(hQuery, IDC_TITLE, retBuffer);
    GetDlgItemText(hQuery, IDC_INSTALL, buffer, BUFFER_LENGTH);
    Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
    SetDlgItemText(hQuery, IDC_INSTALL, retBuffer);
	GetDlgItemText(hQuery, IDC_AUTO_UPDATE, buffer, BUFFER_LENGTH);
	Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
	SetDlgItemText(hQuery, IDC_AUTO_UPDATE, retBuffer);
	CheckDlgButton(hQuery, IDC_AUTO_UPDATE, gDownloader->IsAutoUpdate() ? BST_CHECKED : BST_UNCHECKED);

    if (gDownloader->isOptionalUpdate())
    {
        GetDlgItemText(hQuery, IDC_LATER, buffer, BUFFER_LENGTH);
        Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
        SetDlgItemText(hQuery, IDC_LATER, retBuffer);
    }
    else
    {
        GetDlgItemText(hQuery, IDC_OFFLINE, buffer, BUFFER_LENGTH);
        Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
        SetDlgItemText(hQuery, IDC_OFFLINE, retBuffer);
        GetDlgItemText(hQuery, IDC_EXIT, buffer, BUFFER_LENGTH);
        Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
        SetDlgItemText(hQuery, IDC_EXIT, retBuffer);
    }
    GetWindowText(hQuery, buffer, BUFFER_LENGTH);
    Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
    SetWindowText(hQuery, retBuffer);
    delete[] buffer;
    delete[] retBuffer;

    // The size and location of each button
    RECT laterRect   = {0, 0, 0, 0};
    RECT exitRect    = {0, 0, 0, 0};
    RECT offlineRect = {0, 0, 0, 0};
    RECT installRect = {0, 0, 0, 0};
    int offset = 0;

    // init to have ideal width and height returned
    SIZE size;
    size.cx = 0;
    size.cy = 0;
    BOOL ok = false;

    if (gDownloader->isOptionalUpdate())
    {
        GetWindowRect(GetDlgItem(hQuery, IDC_LATER), &laterRect);
        MapWindowPoints(NULL, hQuery, (LPPOINT)&laterRect, 2);
        ok = Button_GetIdealSize(GetDlgItem(hQuery, IDC_LATER), &size);

        if (ok && (size.cx > BUTTON_WIDTH_PIXELS))
        {
            // Need to adjust size
            int newWidth = size.cx + 20; // 20px border, 10 each side

            laterRect.left = laterRect.left - newWidth + BUTTON_WIDTH_PIXELS;
            laterRect.right = laterRect.left + newWidth;

            offset = newWidth - BUTTON_WIDTH_PIXELS;
        }
    }
    else
    {
        GetWindowRect(GetDlgItem(hQuery, IDC_EXIT), &exitRect);
        MapWindowPoints(NULL, hQuery, (LPPOINT)&exitRect, 2);
        ok = Button_GetIdealSize(GetDlgItem(hQuery, IDC_EXIT), &size);

        if (ok && (size.cx > BUTTON_WIDTH_PIXELS))
        {
            // Need to adjust size
            int newWidth = size.cx + 20; // 20px border, 10 each side

            exitRect.left = exitRect.left - newWidth + BUTTON_WIDTH_PIXELS;
            exitRect.right = exitRect.left + newWidth;
            offset = newWidth - BUTTON_WIDTH_PIXELS;
        }

        GetWindowRect(GetDlgItem(hQuery, IDC_OFFLINE), &offlineRect);
        MapWindowPoints(NULL, hQuery, (LPPOINT)&offlineRect, 2);
        ok = Button_GetIdealSize(GetDlgItem(hQuery, IDC_OFFLINE), &size);

        if (ok && (size.cx > BUTTON_WIDTH_PIXELS))
        {
            // Need to adjust size
            int newWidth = size.cx + 20; // 20px border, 10 each side

            offlineRect.left = offlineRect.left - newWidth - offset + BUTTON_WIDTH_PIXELS;
            offlineRect.right = offlineRect.left + newWidth;
            offset += newWidth - BUTTON_WIDTH_PIXELS;
        }
        else if (offset != 0)
        {
            // Need to move by offset
            offlineRect.left -= offset;
            offlineRect.right -= offset;
        }
    }

    GetWindowRect(GetDlgItem(hQuery, IDC_INSTALL), &installRect);
    MapWindowPoints(NULL, hQuery, (LPPOINT)&installRect, 2);
    ok = Button_GetIdealSize(GetDlgItem(hQuery, IDC_INSTALL), &size);

    if (ok && (size.cx > BUTTON_WIDTH_PIXELS))
    {
        // Need to adjust size
        int newWidth = size.cx + 20; // 20px border, 10 each side

        installRect.left = installRect.left - newWidth - offset + BUTTON_WIDTH_PIXELS;
        installRect.right = installRect.left + newWidth;
    }
    else if (offset != 0)
    {
        // Need to move by offset
        installRect.left -= offset;
        installRect.right -= offset;
    }

    if ( installRect.left < 14 ) // at least 14 pixels to the left of the leftmost button
    {
        int shift = 14 - installRect.left; // left is usually negative here
        RECT rect;
        GetWindowRect(hQuery, &rect);
        SetWindowPos(hQuery, NULL, rect.left - shift/2, rect.top, rect.right - rect.left + shift, rect.bottom - rect.top, NULL);

        GetWindowRect(GetDlgItem(hQuery, IDC_BOTTOM), &rect);
        MapWindowPoints(NULL, hQuery, (LPPOINT)&rect, 2);
        SetWindowPos(GetDlgItem(hQuery, IDC_BOTTOM), NULL, rect.left, rect.top, rect.right - rect.left + shift, rect.bottom - rect.top, NULL);

        GetWindowRect(GetDlgItem(hQuery, IDC_PATCHNOTES), &rect);
        MapWindowPoints(NULL, hQuery, (LPPOINT)&rect, 2);
        SetWindowPos(GetDlgItem(hQuery, IDC_PATCHNOTES), NULL, rect.left, rect.top, rect.right - rect.left + shift, rect.bottom - rect.top, NULL);

        GetWindowRect(GetDlgItem(hQuery, IDC_TITLE), &rect);
        MapWindowPoints(NULL, hQuery, (LPPOINT)&rect, 2);
        SetWindowPos(GetDlgItem(hQuery, IDC_TITLE), NULL, rect.left, rect.top, rect.right - rect.left + shift, rect.bottom - rect.top, NULL);

		GetWindowRect(GetDlgItem(hQuery, IDC_AUTO_UPDATE), &rect);
        MapWindowPoints(NULL, hQuery, (LPPOINT)&rect, 2);
        SetWindowPos(GetDlgItem(hQuery, IDC_AUTO_UPDATE), NULL, rect.left, rect.top, rect.right - rect.left + shift, rect.bottom - rect.top, NULL);

        laterRect.left += shift;
        laterRect.right += shift;
        exitRect.left += shift;
        exitRect.right += shift;
        offlineRect.left += shift;
        offlineRect.right += shift;
        installRect.left += shift;
        installRect.right += shift;
    }

    if (gDownloader->isOptionalUpdate())
    {
        SetWindowPos(GetDlgItem(hQuery, IDC_LATER) ,GetDlgItem(hQuery, IDC_BOTTOM), 
            laterRect.left, laterRect.top, laterRect.right - laterRect.left, laterRect.bottom - laterRect.top, NULL);
    }
    else
    {
        SetWindowPos(GetDlgItem(hQuery, IDC_EXIT) ,GetDlgItem(hQuery, IDC_BOTTOM), 
            exitRect.left, exitRect.top, exitRect.right - exitRect.left, exitRect.bottom - exitRect.top, NULL);
        SetWindowPos(GetDlgItem(hQuery, IDC_OFFLINE) ,GetDlgItem(hQuery, IDC_BOTTOM), 
            offlineRect.left, offlineRect.top, offlineRect.right - offlineRect.left, offlineRect.bottom - offlineRect.top, NULL);
    }
    SetWindowPos(GetDlgItem(hQuery, IDC_INSTALL) ,GetDlgItem(hQuery, IDC_BOTTOM), 
        installRect.left, installRect.top, installRect.right - installRect.left, installRect.bottom - installRect.top, NULL);

    int nCmdShow = SW_SHOW;
    if (gClientStartupState == kClientStartupMinimized)
    {
        nCmdShow = SW_MINIMIZE;
    }
    ShowWindow(hQuery, nCmdShow);

    MSG msg;
    bool done = false;
    bool downloadUpdate = false;
    bool dispatch = true;
	bool exitProcess = false;

    while(!done && GetMessage( &msg, NULL, 0, 0 ) > 0)
    {
		if (msg.message == WM_SYSCOMMAND)
		{
			if ( msg.wParam == SC_RESTORE) //maximizing window
			{
                if(gClientStartupState == kClientStartupMinimized)
                {
                    gClientStartupState = kClientStartupNormal;    //so that subsequent windows are no longer minimized
			}
			}
			else if (msg.wParam == SC_MINIMIZE)
			{
				//the initital detection happens in the dlgproc itself, and that posts a message so that we can set the member flag in this class
				//but we don't want to dispatch the message again down to the dlgproc, so use the kludge dispatch flag
				gClientStartupState = kClientStartupMinimized;
				dispatch = false;       //so it doesn't get passed down to the dlg proc which then passes it back up here
			}
			if (dispatch)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			dispatch = true;
		}
		else if (!IsDialogMessage(hQuery, &msg))
		{
			if (dispatch)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			dispatch = true;
		}
		else
		{
			switch (msg.message)
			{
			case UPDATE_INSTALL:
				{
					ORIGINBS_LOG_EVENT << "QueryUpdate: User chose to install update";
					downloadUpdate = true;
					done = true;
				}
				break;
			case UPDATE_EXIT:
				{
					ORIGINBS_LOG_EVENT << "QueryUpdate: User chose to exit Origin";
					exitProcess = true;
					done = true;
				}
				break;
			case UPDATE_LATER:
				{
					downloadUpdate = false;
					done = true;
				}
				break;
			case UPDATE_OFFLINE:
				{
					ORIGINBS_LOG_EVENT << "QueryUpdate: User chose to login offline mode";
					mIsOfflineMode = true;
					downloadUpdate = false;
					done = true;
				}
				break;
			}
		}
    }

	// Always update autoUpdate flag
	bool autoUpdate = IsDlgButtonChecked(hQuery, IDC_AUTO_UPDATE);
    EndDialog(hQuery, 0);
	gDownloader->SetAutoUpdate(autoUpdate);

	if (exitProcess)
		LogService::releaseAndExitProcess();    //need to release LogService BEFORE exiting

    return downloadUpdate;
}

bool Updater::Unpack(const wchar_t* unpackLocation)
{
    hUnpackDialog = CreateDialog(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_EXTRACTING), NULL, reinterpret_cast<DLGPROC>(DlgProcUnpack));
    SendMessage(hUnpackDialog, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)hIcon);
    HWND extractBar = GetDlgItem(hUnpackDialog, IDC_EXTRACTBAR);
    SetWindowLong (extractBar, GWL_STYLE, GetWindowLong(extractBar, GWL_STYLE) | PBS_MARQUEE);
    SendMessage(extractBar, PBM_SETMARQUEE, 1, 0);

    HDC hdc;
    long lfHeight;
    hdc = GetDC(NULL);
    lfHeight = -MulDiv(12, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    HFONT title = CreateFont(lfHeight, 0, 0, 0, FW_DONTCARE, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, 
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    ReleaseDC(NULL, hdc);
    SendMessage(GetDlgItem(hUnpackDialog, IDC_TITLE), WM_SETFONT, WPARAM(title), TRUE);

    // Localize text
    wchar_t* buffer = new wchar_t[BUFFER_LENGTH];
    wchar_t* retBuffer = new wchar_t[BUFFER_LENGTH];
    GetDlgItemText(hUnpackDialog, IDC_TITLE, buffer, BUFFER_LENGTH);
    Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
    SetDlgItemText(hUnpackDialog, IDC_TITLE, retBuffer);
    GetDlgItemText(hUnpackDialog, IDC_TEXT, buffer, BUFFER_LENGTH);
    Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
    SetDlgItemText(hUnpackDialog, IDC_TEXT, retBuffer);
    GetWindowText(hUnpackDialog, buffer, BUFFER_LENGTH);
    Locale::GetInstance()->Localize(buffer, &retBuffer, BUFFER_LENGTH);
    SetWindowText(hUnpackDialog, retBuffer);
    delete[] retBuffer;
    delete[] buffer;

    int nCmdShow = SW_SHOW;
    if (gClientStartupState == kClientStartupMinimized)
    {
        nCmdShow = SW_MINIMIZE;
    }
    ShowWindow(hUnpackDialog, nCmdShow);

    CreateThread(NULL, 0, unpackAsynch, &unpackLocation, 0, NULL);

    MSG msg;
    bool done = false;
    bool unpacked = false;
    bool dispatch = true;
    while(!done && GetMessage( &msg, NULL, 0, 0 ) > 0)
    {
        switch (msg.message)
        {
        case WM_SYSCOMMAND:
            {
                if ( msg.wParam == SC_RESTORE) //maximizing window
                {
                    if(gClientStartupState == kClientStartupMinimized)
                    {
                        gClientStartupState = kClientStartupNormal;    //so that subsequent windows are no longer minimized
                    }
                }
                else if (msg.wParam == SC_MINIMIZE)
                {
                    //the initital detection happens in the dlgproc itself, and that posts a message so that we can set the member flag in this class
                    //but we don't want to dispatch the message again down to the dlgproc, so use the kludge dispatch flag
                    gClientStartupState = kClientStartupMinimized;
                    dispatch = false;       //so it doesn't get passed down to the dlg proc which then passes it back up here
                }
            }
            break;

        case UNPACK_FAILED:
            {
                ORIGINBS_LOG_ERROR << "Unpack: failed";
                unpacked = false;
                done = true;
            }
            break;
        case UNPACK_SUCCEEDED:
            {
                ORIGINBS_LOG_EVENT << "Unpack: success";
                unpacked = true;
                done = true;
            }
            break;
        }

        if (dispatch)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        dispatch = true;    //restore for next time
    }

    // Delete the staged file
    DeleteFile(unpackLocation);

    EndDialog(hUnpackDialog, 0);
    return unpacked;
}

bool Updater::isOkWinHttpReadData() const
{
    return gDownloader->isOkWinHttpReadData();
}

bool Updater::isOkQuerySession() const
{
    return gDownloader->isOkQuerySession();
}

LRESULT CALLBACK DlgProcUnpack(HWND hWndDlg, UINT Msg,
    WPARAM wParam, LPARAM lParam)
{
    switch(Msg)
    {
    case WM_CTLCOLORSTATIC:
        {
            if(GetDlgItem(hWndDlg, IDC_TITLE) == (HWND)lParam)
            {
                SetTextColor((HDC)wParam, 0x993300);
            }
            else if(GetDlgItem(hWndDlg, IDC_BOTTOM) == (HWND)lParam)
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
    case WM_SYSCOMMAND:
        {
            if (wParam == SC_MINIMIZE)  //user has minimized the dialog
            {
                //need to post the message back up to the window so that the member flag can be set in the class
                PostMessage (hWndDlg, Msg, wParam, lParam);
            }
        }
        break;
    }

    return false;
}

LRESULT CALLBACK DlgProcProgress(HWND hWndDlg, UINT Msg,
    WPARAM wParam, LPARAM lParam)
{
    switch(Msg)
    {
    case WM_COMMAND:
        switch(wParam)
        {
        case IDCANCEL:
            {
                PostMessage(hWndDlg, DOWNLOAD_CANCELED, 0, 0);
                return true;
            }
        }
        break;
    case WM_CTLCOLORSTATIC:
        {
            if(GetDlgItem(hWndDlg, IDC_TITLE) == (HWND)lParam)
            {
                SetTextColor((HDC)wParam, 0x993300);
            }
            else if(GetDlgItem(hWndDlg, IDC_BOTTOM) == (HWND)lParam)
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
        break;

    case WM_SYSCOMMAND:
        {
            if (wParam == SC_MINIMIZE)  //user has minimized the dialog
            {
                //need to post the message back up to the window so that the member flag can be set in the class
                PostMessage (hWndDlg, Msg, wParam, lParam);
            }
        }
        break;
    }
    return false;
}

LRESULT CALLBACK DlgProcUpdate(HWND hWndDlg, UINT Msg,
    WPARAM wParam, LPARAM lParam)
{
    switch(Msg)
    {
    case WM_COMMAND:
        switch(wParam)
        {
        case IDCANCEL:
            {
                if (gDownloader->isOptionalUpdate())
                {
                    PostMessage(hWndDlg, UPDATE_LATER, 0, 0);
                }
                else
                {
                    PostMessage(hWndDlg, UPDATE_EXIT, 0, 0);
                }
                return true;
            }
        case IDC_INSTALL:
            {
                PostMessage(hWndDlg, UPDATE_INSTALL, 0, 0);
                return true;
            }
        case IDC_LATER:
            {
                PostMessage(hWndDlg, UPDATE_LATER, 0, 0);
                return true;
            }
        case IDC_EXIT:
            {
                PostMessage(hWndDlg, UPDATE_EXIT, 0, 0);
                return true;
            }
        case IDC_OFFLINE:
            {
                PostMessage(hWndDlg, UPDATE_OFFLINE, 0, 0);
                return true;
            }
        }
        break;
    case WM_CTLCOLORSTATIC:
        {
            if(GetDlgItem(hWndDlg, IDC_TITLE) == (HWND)lParam)
            {
                SetTextColor((HDC)wParam, 0x993300);
            }
            else if(GetDlgItem(hWndDlg, IDC_BOTTOM) == (HWND)lParam)
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
        break;

    case WM_SYSCOMMAND:
        {
            if (wParam == SC_MINIMIZE)  //user has minimized the dialog
            {
                //need to post the message back up to the window so that the member flag can be set in the class
                PostMessage (hWndDlg, Msg, wParam, lParam);
            }
        }
        break;
    }

    return false;
}

LRESULT CALLBACK DlgProcEULA(HWND hWndDlg, UINT Msg,
    WPARAM wParam, LPARAM lParam)
{
    switch(Msg)
    {
    case WM_COMMAND:
        switch(wParam)
        {
        case IDCANCEL:
            {
                PostMessage(hWndDlg, EULA_DECLINED, 0, 0);
                return true;
            }
        case IDOK:
            {
                PostMessage(hWndDlg, EULA_ACCEPTED, 0, 0);
                return true;
            }
        case IDC_CHECK1:
            {
                if (SendMessage(GetDlgItem(hWndDlg, IDC_CHECK1), BM_GETCHECK, 0, 0) == BST_CHECKED)
                {
                    PostMessage(hWndDlg, EULA_CHECKED, 0, 0);
                }
                else
                {
                    PostMessage(hWndDlg, EULA_UNCHECKED, 0, 0);
                }
                return true;
            }
        case IDC_PRINT:
            {
                PostMessage(hWndDlg, EULA_PRINT, 0, 0);
                return true;
            }
            break;
        }
        break;
    case WM_CTLCOLORSTATIC:
        {
            if(GetDlgItem(hWndDlg, IDC_TITLE) == (HWND)lParam)
            {
                SetTextColor((HDC)wParam, 0x993300);
            }
            else if(GetDlgItem(hWndDlg, IDC_BOTTOM) == (HWND)lParam)
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
        break;
    }

    return false;
}

void Updater::setUseEmbeddedEULA (bool embedded)
{
    mUseEmbeddedEULA = embedded;
}

bool Updater::DownloadConfiguration()
{
    // Read environment from EACore.ini

    wchar_t path[MAX_PATH];
    ::GetModuleFileName(::GetModuleHandle(NULL), path, MAX_PATH);

    // Need to communicate with our service to see if we need to update
    // First off, what version are we, or is there a staged version here?
    wstring location = path;
    // Remove executable
    int lastSlash = location.find_last_of(L"\\");
    location = location.substr(0, lastSlash + 1);

    wchar_t iniLocation[256];
    wsprintfW(iniLocation, L"%s%s", location.c_str(), L"EACore.ini");

    // Debug info from EACore.ini
    wchar_t environment[256];
    DetermineEnvironment(iniLocation, environment, 256);

    char version[32] = EALS_VERSION_P_DELIMITED;
    version[strlen(EALS_VERSION_P_DELIMITED)] = 0;
    
    // check for client version override
    wchar_t* overrideVersion = wcsstr(GetCommandLine(), L"/version ");
    if (overrideVersion != 0)
    {
        overrideVersion += wcslen(L"/version ");

        wstring version_ = overrideVersion;
        int endOfVersion = version_.find_first_of(L" ");
        version_ = version_.substr(0, endOfVersion);
        sprintf(version, "%S", version_.c_str()); // convert 'wchar_t' to 'char'
    }

    wchar_t wVersion[BUFFER_LENGTH];
    size_t convertedChars = 0;
    mbstowcs_s(&convertedChars, wVersion, _countof(wVersion), version, _TRUNCATE);
    wVersion[convertedChars] = 0;

    bool success = gDownloader->DownloadOriginConfig(environment, wVersion);
    mDownloaderErrorText = gDownloader->getErrorMessage();

	return success;
}
