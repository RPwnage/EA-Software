///////////////////////////////////////////////////////////////////////////////
// Downloader.cpp
// 
// Created by Kevin Moore
// Copyright (c) 2011 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "Downloader.h"
#include "Common.h"
#include "Resource.h"
#include "Locale.h"
#include "version/version.h"
#include "LogService_win.h"
#include <string>
#include <ShlObj.h>
#include <time.h>
#include <comutil.h>

using namespace std;

static const size_t MAX_BUFFER_SIZE = 256;

namespace
{
	const wchar_t* STAGED_CONFIG_FILE = L"%s\\%s.wad.staged";
	const wchar_t* CONFIG_FILE = L"%s\\%s.wad";
}

Downloader::Downloader()
    : hSession(NULL)
    , hConnect(NULL)
    , hRequest(NULL)
    , mhDownloadedFile(NULL)
    , mURLOverride(false)
    , mIsBetaOptIn(false)
    , mIsAutoUpdate(false)
    , mAutoPatch(false)
    , mUserCancelledDownload(false)
    , mInternetConnection(true)
    , mServiceAvailable(true)
	, mTelemetryOptOut(false)
    , mSettingsFileFound(false)
    , mCurEULAVersion(0)
    , mIsOkWinHttpReadData(false)
    , mIsOkQuerySession(false)
{
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig;
    WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig);

    LPCWSTR agent = L"Mozilla/5.0 EA Download Manager Origin/" _T(EALS_VERSION_P_DELIMITED);
    if ( proxyConfig.lpszProxy )
    {
        ORIGINBS_LOG_EVENT << "Using proxy " << proxyConfig.lpszProxy;
        hSession = WinHttpOpen(agent, WINHTTP_ACCESS_TYPE_NAMED_PROXY, 
            proxyConfig.lpszProxy, proxyConfig.lpszProxyBypass, 0);
    }
    else
    {
        hSession = WinHttpOpen(agent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    }



    mVersionNumber = new wchar_t[MAX_BUFFER_SIZE];
    mVersionNumber[0] = NULL;
    mUrl = new wchar_t[MAX_BUFFER_SIZE];
    mUrl[0] = NULL;
    mEulaUrl = new wchar_t[MAX_BUFFER_SIZE];
    mEulaUrl[0] = NULL;
    mEulaVersion = new wchar_t[MAX_BUFFER_SIZE];
    mEulaVersion[0] = NULL;
    mEulaLatestMajorVersion = new wchar_t[MAX_BUFFER_SIZE];
    mEulaLatestMajorVersion[0] = NULL;
    mType = new wchar_t[MAX_BUFFER_SIZE];
    mType[0] = NULL;
    mUpdateRule = new wchar_t[MAX_BUFFER_SIZE];
    mUpdateRule[0] = NULL;
	mOriginConfigFileName = new wchar_t[MAX_PATH];
	mOriginConfigFileName[0] = NULL;
    mOutputFileName = new wchar_t[MAX_PATH];
    mOutputFileName[0] = NULL;
    mDownloadOverride = new wchar_t[MAX_PATH];
    mDownloadOverride[0] = NULL;
    mTempEulaFilePath = new wchar_t[MAX_PATH];
    mTempEulaFilePath[0] = NULL;

    mSettingsFileFound = QuerySettingsFile();
}

Downloader::~Downloader() 
{
    if (hSession)
    {
        WinHttpCloseHandle(hSession);
        hSession = NULL;
    }

    // Make sure we aren't leaving a temporary file behind just in case abberrant logic occurred.
    DeleteTempEULAFile();

    delete[] mVersionNumber;
    delete[] mUrl;
    delete[] mEulaUrl;
    delete[] mEulaVersion;
    delete[] mEulaLatestMajorVersion;
    delete[] mType;
    delete[] mUpdateRule;
    delete[] mOutputFileName;
    delete[] mTempEulaFilePath;
}

void Downloader::Download(const wchar_t* url)
{
    if (OpenSession(url))
    {
        DownloadUpdate();
    }
}

void Downloader::setDownloadOverride(const wchar_t* overrideURL)
{
    wcscpy_s(mDownloadOverride, MAX_BUFFER_SIZE, overrideURL);
    mURLOverride = true;
}

bool Downloader::QueryUpdateService(const wchar_t* environment, const wchar_t* version, 
    const wchar_t* type, wchar_t** buffer)
{
    wchar_t url[1024];
    bool connection;

    // Get OS version and Service Pack version.
    wchar_t osVersion[32];
    wchar_t servicePackVersion[32];
    osVersion[0] = 0;
    servicePackVersion[0] = 0;

    bool validOSAndServicePackVersion = false;
    OSVERSIONINFOEX	osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (GetVersionEx(LPOSVERSIONINFOW(&osvi)))
    {
        wsprintfW(osVersion, L"%d.%d.%d", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
        wsprintfW(servicePackVersion, L"%d.%d", osvi.wServicePackMajor, osvi.wServicePackMinor);
        validOSAndServicePackVersion = true;
    }

    // check for OS version override - format should be {major}.{minor}.{build}
    wchar_t* overrideOSVersion = wcsstr(GetCommandLine(), L"/osVersion ");
    if (overrideOSVersion != 0)
    {
        overrideOSVersion += wcslen(L"/osVersion ");

        wstring osVersion_ = overrideOSVersion;
        int endOfOSVersion = osVersion_.find_first_of(L" ");
        osVersion_ = osVersion_.substr(0, endOfOSVersion);
        wsprintfW(osVersion, L"%s", osVersion_.c_str()); // replace 'osVersion' with override
    }

    // check for service pack version override - format should be {major}.{minor}
    wchar_t* overrideServicePackVersion = wcsstr(GetCommandLine(), L"/servicePackVersion ");
    if (overrideServicePackVersion != 0)
    {
        overrideServicePackVersion += wcslen(L"/servicePackVersion ");

        wstring servicePackVersion_ = overrideServicePackVersion;
        int endOfServicePackVersion = servicePackVersion_.find_first_of(L" ");
        servicePackVersion_ = servicePackVersion_.substr(0, endOfServicePackVersion);
        wsprintfW(servicePackVersion, L"%s", servicePackVersion_.c_str()); // replace 'servicePackVersion' with override
    }

    ORIGINBS_LOG_EVENT << "Querying update service for environment " << environment;
    if (mURLOverride)
    {
        if (validOSAndServicePackVersion)
        {
            wsprintfW(url, L"%s/%s/%s/%s?platform=PCWIN&osVersion=%s&servicePackVersion=%s", mDownloadOverride, 
                version, Locale::GetInstance()->GetEulaLocale(), type, osVersion, servicePackVersion);
        }
        else
        {
            wsprintfW(url, L"%s/%s/%s/%s?platform=PCWIN", mDownloadOverride, 
                version, Locale::GetInstance()->GetEulaLocale(), type);
        }
        connection = OpenSession(url);
    }
    else if (_wcsicmp(environment, L"production") == 0)
    {
        if (validOSAndServicePackVersion)
        {
            wsprintfW(url, L"https://secure.download.dm.origin.com/autopatch/2/upgradeFrom/%s/%s/%s?platform=PCWIN&osVersion=%s&servicePackVersion=%s", 
                version, Locale::GetInstance()->GetEulaLocale(), type, osVersion, servicePackVersion);
        }
        else
        {
            wsprintfW(url, L"https://secure.download.dm.origin.com/autopatch/2/upgradeFrom/%s/%s/%s?platform=PCWIN", 
                version, Locale::GetInstance()->GetEulaLocale(), type);
        }
        connection = OpenSession(url);
    }
    // QA sometimes use fc-qa in their testing so we should explicitly handle it.
    else if (_wcsicmp(environment, L"fc.qa") == 0 || _wcsicmp(environment, L"fc-qa") == 0)
    {
        if (validOSAndServicePackVersion)
        {
            wsprintfW(url, L"https://stage.secure.download.dm.origin.com/fc-qa/autopatch/2/upgradeFrom/%s/%s/%s?platform=PCWIN&osVersion=%s&servicePackVersion=%s", 
                version, Locale::GetInstance()->GetEulaLocale(), type, osVersion, servicePackVersion);
        }
        else
        {
            wsprintfW(url, L"https://stage.secure.download.dm.origin.com/fc-qa/autopatch/2/upgradeFrom/%s/%s/%s?platform=PCWIN", 
                version, Locale::GetInstance()->GetEulaLocale(), type);
        }
        connection = OpenSession(url);
    }
    else
    {
        if (validOSAndServicePackVersion)
        {
            wsprintfW(url, L"https://stage.secure.download.dm.origin.com/%s/autopatch/2/upgradeFrom/%s/%s/%s?platform=PCWIN&osVersion=%s&servicePackVersion=%s", 
                environment, version, Locale::GetInstance()->GetEulaLocale(), type, osVersion, servicePackVersion);
        }
        else
        {
            wsprintfW(url, L"https://stage.secure.download.dm.origin.com/%s/autopatch/2/upgradeFrom/%s/%s/%s?platform=PCWIN", 
                environment, version, Locale::GetInstance()->GetEulaLocale(), type);
        }
        connection = OpenSession(url, true);
    }

    ORIGINBS_LOG_EVENT << "URL: " << url;
    if (connection)
    {
        ORIGINBS_LOG_EVENT << "Query connection successful";
        
        mIsOkQuerySession = true;

        connection = DownloadUpdateXML(buffer);
    }
    else
    {
        ORIGINBS_LOG_ERROR << "Query connection failed";
    }

    return connection;
}

bool Downloader::OpenSession(const wchar_t* url, bool ignoreSSLErrors /* = false */)
{
    wchar_t host[MAX_BUFFER_SIZE];
    URL_COMPONENTS urlComp;
    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.lpszHostName = host;
    urlComp.dwHostNameLength = sizeof(host) / sizeof(host[0]);

    // set required component lengths to non-zero so that they are cracked.
	urlComp.dwUrlPathLength = (DWORD)-1;
	urlComp.dwSchemeLength = (DWORD)-1;

    if (!WinHttpCrackUrl(url, 0, 0, &urlComp))
    {
        ORIGINBS_LOG_ERROR << "OpenSession: WinHttpCrackUrl failed";
        SetErrorMessage("OpenSession:WinHttpCrackUrl", GetLastError());
        CancelDownload(DOWNLOAD_FAILED);
        return false;
    }

    // Open an HTTP session
    hConnect = WinHttpConnect(hSession, host, urlComp.nPort, 0);

    if (hConnect == NULL)
    {
        ORIGINBS_LOG_ERROR << "OpenSession: failed to connect";
        SetErrorMessage("OpenSession:WinHttpConnect", GetLastError());
        CancelDownload(DOWNLOAD_FAILED);
        return false;
    }

    hRequest = WinHttpOpenRequest(hConnect, L"GET", urlComp.lpszUrlPath, 
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 
        (INTERNET_SCHEME_HTTPS == urlComp.nScheme) ? WINHTTP_FLAG_SECURE : 0);

    if (hRequest == NULL)
    {
        ORIGINBS_LOG_ERROR << "OpenSession:: failed to open request";
        SetErrorMessage("OpenSession:WinHttpOpenRequest", GetLastError());
        CancelDownload(DOWNLOAD_FAILED);
        return false;
    }

    // Since this thread blocks the main thread, we only want to wait a 
    // limited amount of time to avoid having the user wait too 
    // long to access the Origin login screen (EBIBUGS-26188).
    static const int TIMEOUT = 5000;
    bool success = WinHttpSetTimeouts(hRequest, TIMEOUT, TIMEOUT, TIMEOUT, TIMEOUT) == TRUE;

    if (!success)
    {
        DWORD lastError = GetLastError();
        ORIGINBS_LOG_WARNING << "Unable to set WinHTTP timeouts due to error " << lastError;
        SetErrorMessage("OpenSession:WinHttpSetTimeouts", lastError);
    }

    if (ignoreSSLErrors)
    {
        DWORD dwFlags = SECURITY_FLAG_IGNORE_CERT_CN_INVALID|
            SECURITY_FLAG_IGNORE_CERT_DATE_INVALID|
            SECURITY_FLAG_IGNORE_UNKNOWN_CA;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(DWORD));
    }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA,
        0, 0, 0))
    {
        ORIGINBS_LOG_ERROR << "OpenSession: failed to send request";
        SetErrorMessage("OpenSession:WinHttpSendRequest", GetLastError());
        // If we couldn't send a request then we don't have a connection
        mInternetConnection = false;
        CancelDownload(DOWNLOAD_FAILED);
        return false;
    }

    if (!WinHttpReceiveResponse(hRequest, NULL))
    {
        ORIGINBS_LOG_ERROR << "OpenSession: failed to receive response";
        SetErrorMessage("OpenSession:WinHttpReceiveResponse", GetLastError());
        // If we didn't get a response then the service isn't available
        mServiceAvailable = false;
        CancelDownload(DOWNLOAD_FAILED);
        return false;
    }

    return true;
}

bool Downloader::DownloadUpdateXML(wchar_t** buffer)
{
    DWORD statusCode = 0;
    wchar_t* test;
    DWORD size = 32;
    wchar_t szResponse[64] = {0};
    bool result = false;

    // Check the status code
    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE, WINHTTP_HEADER_NAME_BY_INDEX,
        &szResponse, &size, WINHTTP_NO_HEADER_INDEX))
    {
        statusCode = _wtoi(szResponse);
    }

    if (statusCode == 200)
    {
		DWORD dwDownloaded = 0;
		DWORD dwTotalSize = 0;
		char* pszOutBuffer;
        size = 32;
        if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX,
            &szResponse, &size, WINHTTP_NO_HEADER_INDEX))
        {
            dwTotalSize = _wtoi(szResponse);
        }

        // Allocate space for the buffer.
        pszOutBuffer = new char[dwTotalSize+1];
        test = new wchar_t[dwTotalSize+1];
        ZeroMemory(pszOutBuffer, dwTotalSize+1);

        if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwTotalSize, &dwDownloaded))
        {
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pszOutBuffer, dwDownloaded, test, dwDownloaded);
            test[dwDownloaded] = 0;
            delete[] pszOutBuffer;
            *buffer = test;
            result = true;
        }	
        else
        {
            ORIGINBS_LOG_ERROR << "Failed to read update XML data from server: " << GetLastError();
        }
    }
    else
    {
        ORIGINBS_LOG_ERROR << "UpdateXML status code: " << statusCode;
    }

    closeWinHttpHandles();

    return result;
}

HANDLE Downloader::CreateTempEULAFile()
{
    TCHAR szPath[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, szPath))) 
    {
        ORIGINBS_LOG_ERROR << "Failed to get appdata path to create staged config file [last error: " << GetLastError() << "]";
        return INVALID_HANDLE_VALUE;
    }

    std::wstring strProgramDataOrigin (szPath);
    strProgramDataOrigin.append (L"\\Origin\\SelfUpdate");

    wsprintfW(mTempEulaFilePath, L"%s\\tempEULA_%s.html", strProgramDataOrigin.c_str(), mEulaVersion);
    HANDLE file = CreateFile(mTempEulaFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        
    if(file == INVALID_HANDLE_VALUE)
    {
        ORIGINBS_LOG_ERROR << "Failed to create temporary EULA file [last error: " << GetLastError() << "]";
        mTempEulaFilePath[0] = NULL;
    }

    return file;
}

HANDLE Downloader::CreateStagedOriginConfigFile(const wchar_t* environment)
{	
    TCHAR szPath[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, szPath))) 
    {
        ORIGINBS_LOG_ERROR << "Failed to get appdata path to create staged config file [last error: " << GetLastError() << "]";
        return INVALID_HANDLE_VALUE;
    }

    std::wstring strProgramDataOrigin (szPath);
    strProgramDataOrigin.append (L"\\Origin");

	wchar_t path[MAX_PATH];

	wsprintfW(path, STAGED_CONFIG_FILE, strProgramDataOrigin.c_str(), environment);

    HANDLE file = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        
    if(file == INVALID_HANDLE_VALUE)
    {
        ORIGINBS_LOG_ERROR << "Failed to create staged config file [last error: " << GetLastError() << "]";
    }

    return file;
}

bool Downloader::CheckAvailableDiskSpace(DWORD sizeNeeded)
{
    ULARGE_INTEGER freeBytes;
    ULARGE_INTEGER totalNumberOfBytes;
    ULARGE_INTEGER totalNumberOfFreeBytes;
    GetDiskFreeSpaceEx(NULL, &freeBytes, &totalNumberOfBytes, &totalNumberOfFreeBytes);

    if(sizeNeeded > freeBytes.QuadPart)
    {
        ORIGINBS_LOG_ERROR << "Insufficient disk space to download update";
        // Not enough space
        wchar_t* text = new wchar_t[512];
        wchar_t* title = new wchar_t[512];
        Locale::GetInstance()->Localize(L"ebisu_client_not_enough_disk_space_uppercase", &title, 512);
        Locale::GetInstance()->Localize(L"ebisu_client_not_enough_disk_space", &text, 512);
        MessageBoxEx(NULL, text, title, MB_OK, NULL);
        delete[] text;
        delete[] title;
        return false;
    }
    else
    {
        return true;
    }
}

bool Downloader::DownloadEULA(bool ignoreSSLErrors)
{
    if(_wcsicmp(mEulaUrl, L"") != 0)
    {
        HANDLE eulaFile = CreateTempEULAFile();
        bool downloadSuccess = false;

        if(eulaFile != INVALID_HANDLE_VALUE)
        {
            downloadSuccess = DownloadFile(mEulaUrl, eulaFile, ignoreSSLErrors);
            CloseHandle(eulaFile);
        }

        if(!downloadSuccess)
        {                
            DeleteTempEULAFile();
        }
    }

    return (_wcsicmp(mTempEulaFilePath, L"") != 0);
}

bool Downloader::DownloadFile(wchar_t* fileUrl, HANDLE localFileHandle, bool ignoreSSLErrors)
{        
    bool errorOccurred = false;

    if(OpenSession(fileUrl, ignoreSSLErrors))
    {
        DWORD statusCode = 0;
        DWORD size = 32;
        wchar_t szResponse[64] = {0};

        // Check the status code
        if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE, WINHTTP_HEADER_NAME_BY_INDEX,
            &szResponse, &size, WINHTTP_NO_HEADER_INDEX))
        {
            statusCode = _wtoi(szResponse);
        }

        if (statusCode == 200)
        {
            DWORD dwMaxChunkSize = 8192;
            DWORD dwAvailable= 0;
            DWORD dwDownloaded = 0;

            // Allocate space for the buffer.
            char* pszOutBuffer = new char[dwMaxChunkSize+1];

            if(!pszOutBuffer)
            {
                ORIGINBS_LOG_ERROR << "Failed to allocate memory for download [last error: " << GetLastError() << "]";
                errorOccurred = true;
            }
            else
            {
                do 
                {
					DWORD dwChunkSize = 0;
                    dwAvailable = 0;

                    if(!WinHttpQueryDataAvailable(hRequest, &dwAvailable))
                    {
                        ORIGINBS_LOG_ERROR << "Failed to determine how much data is available [last error: " << GetLastError() << "]";
                        errorOccurred = true;
                        break;
                    }

                    dwChunkSize = (dwAvailable > dwMaxChunkSize) ? dwMaxChunkSize : dwAvailable;

                    if (CheckAvailableDiskSpace(dwChunkSize))
                    {
                        ZeroMemory(pszOutBuffer, dwChunkSize+1);

                        if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwChunkSize, &dwDownloaded))
                        {
                            DWORD dwWriteSize = 0;
                            BOOL writeOk = WriteFile(localFileHandle, pszOutBuffer, dwDownloaded, &dwWriteSize, NULL);

                            if (!writeOk || dwWriteSize != dwDownloaded)
                            {
                                int err = GetLastError ();
                                ORIGINBS_LOG_ERROR << "DownloadUpdate: failed to write out downloaded file from URL [" << fileUrl << "]. err = " << err;
                                errorOccurred = true;
                                break;
                            }
                        }	
                    }
                    else
                    {
                        errorOccurred = true;
                    }

                } while (dwAvailable > 0 && !errorOccurred);

                delete[] pszOutBuffer;
            }
        }
        else
        {
            errorOccurred = true;
            ORIGINBS_LOG_ERROR << "Download URL failed. [url: " << fileUrl << "]. Request status code: " << statusCode;
        }

        closeWinHttpHandles();
    }

    return !errorOccurred;
}

void Downloader::DeleteTempEULAFile()
{
    if(_wcsicmp(mTempEulaFilePath, L"") != 0)
    {
        DeleteFile(mTempEulaFilePath);
        mTempEulaFilePath[0] = NULL;
    }
}

void Downloader::DownloadUpdate()
{	
    DWORD dwSize = 128 * 1024;
    DWORD dwDownloaded = 0;
    DWORD dwTotalSize = 0;
    DWORD statusCode = 0;
    DWORD dwTotalDownloaded = 0;
    char* pszOutBuffer;
    DWORD size = 32;
    wchar_t szResponse[64] = {0};

	ORIGINBS_LOG_EVENT << "DownloadUpdate: downloading version " << mVersionNumber << " from server.";

    // Check the status code
    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE, WINHTTP_HEADER_NAME_BY_INDEX,
        &szResponse, &size, WINHTTP_NO_HEADER_INDEX))
    {
        statusCode = _wtoi(szResponse);
    }

    if (statusCode != 200)
    {
        ORIGINBS_LOG_ERROR << "DownloadUpdate failed. status code = " << statusCode;
        CancelDownload(DOWNLOAD_FAILED);
        return;
    }

    size = 32;
    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX,
        &szResponse, &size, WINHTTP_NO_HEADER_INDEX))
    {
        dwTotalSize = _wtoi(szResponse);
    }

    if (!CheckAvailableDiskSpace(dwTotalSize))
    {
		ORIGINBS_LOG_WARNING << "DownloadUpdate failed.  Could not download update due to lack of disk space.";
        CancelDownload(DOWNLOAD_NOT_ENOUGH_SPACE);
        return;
    }

    wchar_t szProgramDataPath[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, szProgramDataPath);

    std::wstring strProgramDataOrigin (szProgramDataPath);
    strProgramDataOrigin.append (L"\\Origin\\SelfUpdate\\");
    wstring updateLocation = strProgramDataOrigin;

    // Ensure our SelfUpdate working folder exists
    int dirResult = SHCreateDirectoryEx(NULL, updateLocation.c_str(), NULL);
    if (dirResult != ERROR_SUCCESS && dirResult != ERROR_ALREADY_EXISTS)
    {
        ORIGINBS_LOG_ERROR << "DownloadUpdate: failed to create selfupdate folder: " << updateLocation.c_str() << " err = " << dirResult;
    }

    int* version = new int[4];
    GetVersion(mVersionNumber, version);
    wchar_t tempOutputName[MAX_PATH];
    wsprintfW(tempOutputName, L"%sOriginUpdate_%i_%i_%i_%i.zip.part", updateLocation.c_str(), version[0], version[1], version[2], version[3]);
    wsprintfW(mOutputFileName, L"%sOriginUpdate_%i_%i_%i_%i.zip", updateLocation.c_str(), version[0], version[1], version[2], version[3]);
    mhDownloadedFile = CreateFile(tempOutputName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    delete[] version;

    // Allocate space for the buffer.
    pszOutBuffer = new char[dwSize+1];
    ZeroMemory(pszOutBuffer, dwSize+1);

    clock_t timerStart = clock();
    int curSecondsElapsed = 1;
    bool windowVisible = false;

    do
    {
        // Only send this every second to keep this from spamming
        int secondsElapsed = ((clock() - timerStart) / CLOCKS_PER_SEC) + 1;
        if (curSecondsElapsed != secondsElapsed)
        {
            // Don't show the UI until there is data to show
            if (!windowVisible)
            {
                int nCmdShow = SW_SHOW;
                if (gClientStartupState == kClientStartupMinimized)
                {
                    nCmdShow = SW_MINIMIZE;
                }
                ShowWindow(hDownloadDialog, nCmdShow);
                windowVisible = true;
            }
            int kbDownloaded = dwTotalDownloaded / 1024;
            int downloadSpeed = kbDownloaded/secondsElapsed;
            int secondsRemaining = ((dwTotalSize - dwTotalDownloaded) / 1024) / (downloadSpeed + 1);
            int minutesRemaining = secondsRemaining / 60;
            int hoursRemaining = minutesRemaining / 60;
            secondsRemaining %= 60;
            minutesRemaining %= 60;
            wchar_t buffer[256];
            wchar_t* locBuffer = new wchar_t[256];
            Locale::GetInstance()->Localize(L"origin_bootstrap_KB", &locBuffer, 256);
            wsprintfW(buffer, L"%d %s", downloadSpeed, locBuffer);
            SetDlgItemText(hDownloadDialog, IDC_DOWNLOAD_SPEED, buffer);
            buffer[0] = 0;
            if (hoursRemaining != 0)
            {
                Locale::GetInstance()->Localize(L"origin_bootstrap_hour", &locBuffer, 256);
                wsprintfW(buffer, L"%d %s ", hoursRemaining, locBuffer);
            }
            if (minutesRemaining != 0)
            {
                Locale::GetInstance()->Localize(L"origin_bootstrap_min", &locBuffer, 256);
                wsprintfW(buffer, L"%s%d %s ", buffer, minutesRemaining, locBuffer);
            }
            if (secondsRemaining != 0)
            {
                Locale::GetInstance()->Localize(L"origin_bootstrap_sec", &locBuffer, 256);
                wsprintfW(buffer, L"%s%d %s", buffer, secondsRemaining, locBuffer);
            }
            Locale::GetInstance()->Localize(L"origin_bootstrap_copied", &locBuffer, 256);
            wsprintfW(locBuffer, locBuffer, dwTotalDownloaded/1024/1024, dwTotalSize/1024/1024);
            wsprintfW(buffer, L"%s (%s)", buffer, locBuffer);
            SetDlgItemText(hDownloadDialog, IDC_TIMELEFTVALUE, buffer);
            curSecondsElapsed = secondsElapsed;
        }

        int progress = (int)(((float)dwTotalDownloaded / (float)dwTotalSize) * 100);
        SendNotifyMessage(GetDlgItem(hDownloadDialog, IDC_PROGRESS), PBM_SETPOS, progress, 0);

        // Read the Data.
        //NOTE: it seems that even if we lose internet connection, WinHttpReadData doesn't seem to return an error
        //it always returns true and when you call GetLastError(), the error is not set
        //BUT it does seem to return dwDownloaded = 0 tho we have no way to distinguish that from when there is no more data to download
        //unfortunately, it looks like we have to rely on unpack to fail when the file isn't completely downloaded
        mIsOkWinHttpReadData = WinHttpReadData( hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)?true:false;

        if (mIsOkWinHttpReadData)
        {
            dwTotalDownloaded += dwDownloaded;

            DWORD dwWriteSize = 0;
            BOOL writeOk = WriteFile(mhDownloadedFile, pszOutBuffer, dwDownloaded, &dwWriteSize, NULL);
            if (!writeOk)
            {
                int err = GetLastError ();
                ORIGINBS_LOG_ERROR << "DownloadUpdate: failed to write out downloaded data. err = " << err;
            }
        }
        else
        {
            int err = GetLastError ();
            ORIGINBS_LOG_ERROR << "DownloadUpdate: http read error = " << err;
        }

    } while (dwDownloaded > 0 && !mUserCancelledDownload && mIsOkWinHttpReadData);

    CloseHandle(mhDownloadedFile);

    // Free the memory allocated to the buffer.
    delete[] pszOutBuffer;

    if (mUserCancelledDownload || !mIsOkWinHttpReadData)
    {
        // The downloaded file is faulty. Erase it
        DeleteFile(tempOutputName);
        CancelDownload(DOWNLOAD_FAILED);
    }
    else
    {
        // Once we're done the download, move the file over to a new name
        MoveFile(tempOutputName, mOutputFileName);
        closeWinHttpHandles();
        PostMessage(hDownloadDialog, DOWNLOAD_SUCCEEDED, 0, 0);
    }
}

void Downloader::closeWinHttpHandles()
{
    if (NULL != hRequest)
    {
        WinHttpCloseHandle(hRequest);
        hRequest = NULL;
    }
    if (NULL != hConnect)
    {
        WinHttpCloseHandle(hConnect);
        hConnect = NULL;
    }
}

void Downloader::CancelDownload(int failureCode)
{
    closeWinHttpHandles();
    PostMessage(hDownloadDialog, failureCode, 0, 0);
}

void Downloader::UserCancelDownload()
{
    mUserCancelledDownload = true;
}

void Downloader::GetVersion(wchar_t* version, int* versionArray)
{
    // Copy this to a temp since we don't want to alter the version
    wchar_t* tempVer = new wchar_t[MAX_BUFFER_SIZE];
    wcscpy_s(&tempVer[0], MAX_BUFFER_SIZE, version);

    wchar_t* context;
    wchar_t* p = wcstok_s(tempVer, L".", &context);

    while (p != NULL)
    {
        *versionArray = _wtoi(p);
        versionArray++;
        p = wcstok_s(NULL, L".", &context);
    }
    delete[] tempVer;
}

bool Downloader::UpdateCheck(const wchar_t* environment, const wchar_t* currentVersion)
{
    // Need to check the version online
    // Need to query our service to see if we need to update
    wchar_t* buffer = NULL;
    wchar_t* type = (mIsBetaOptIn ? L"BETA" : L"PROD");

    // Did we succeed?
    if (!QueryUpdateService(environment, currentVersion, type, &buffer))
        return false;

    if (buffer != NULL)
    {
        CComPtr<IXMLDOMDocument> iXMLDoc;
        HRESULT hr = iXMLDoc.CoCreateInstance(__uuidof(DOMDocument));

        VARIANT_BOOL bLoadSuccess = false;
        bool bParseSuccess = false;
        if (iXMLDoc)
        {
            hr = iXMLDoc->loadXML(buffer, &bLoadSuccess);
            if (bLoadSuccess)
            {
                bParseSuccess = ParseXML(iXMLDoc);
            }
            else
            {
                ORIGINBS_LOG_ERROR << "UpdateCheck: loadXML failed. err = " << hr;
            }
        }

        return bParseSuccess;
    }
    else
    {
        ORIGINBS_LOG_ERROR << "QueryUpdateService returned empty update data.";
        return false;
    }
}

void Downloader::ReadDebugResponse(const wchar_t* version, const wchar_t* type, const wchar_t* downloadLocation,
    const wchar_t* rule, const wchar_t* eulaVersion, const wchar_t* eulaLocation, const wchar_t* eulaLatestMajorVersion)
{
    wcscpy_s(&mUrl[0], MAX_BUFFER_SIZE, downloadLocation);
    wcscpy_s(&mVersionNumber[0], MAX_BUFFER_SIZE, version);
    wcscpy_s(&mType[0], MAX_BUFFER_SIZE, type);
    wcscpy_s(&mEulaUrl[0], MAX_BUFFER_SIZE, eulaLocation);
    wcscpy_s(&mEulaVersion[0], MAX_BUFFER_SIZE, eulaVersion);
    wcscpy_s(&mUpdateRule[0], MAX_BUFFER_SIZE, rule);
    wcscpy_s(&mEulaLatestMajorVersion[0], MAX_BUFFER_SIZE, eulaLatestMajorVersion);

    ORIGINBS_LOG_EVENT << "ReadDebugResponse: url [" << mUrl << "], version [" << mVersionNumber << "], type [" << mType << "], eulaUrl [" << mEulaUrl << "], updateRule [" << mUpdateRule << "], eulaLatestMajorVersion [" << mEulaLatestMajorVersion << "]";
}

bool Downloader::ParseXML(CComPtr<IXMLDOMDocument> iXMLDoc)
{
    bool parseSuccess = true;

    CComPtr<IXMLDOMElement> iRootElm;
    CComPtr<IXMLDOMNodeList> iNodeList;
    iXMLDoc->get_documentElement(&iRootElm);
    if (iRootElm)
    {
        iRootElm->get_childNodes(&iNodeList);

        if (iNodeList)
        {
            long length = 0;
            iNodeList->get_length(&length);

            for (int i = 0; i < length; i++)
            {
                CComPtr<IXMLDOMNode> iElem;
                iNodeList->nextNode(&iElem);
                CComVariant val(VT_EMPTY);
                if (iElem)
                {
                    iElem->get_nodeTypedValue(&val);
                    LPWSTR nodeName;
                    iElem->get_nodeName(&nodeName);

                    if (wcscmp(nodeName, L"downloadURL") == 0)
                    {
                        wcscpy_s(&mUrl[0], MAX_BUFFER_SIZE, val.bstrVal);
                    }
                    else if (wcscmp(nodeName, L"version") == 0)
                    {
                        wcscpy_s(&mVersionNumber[0], MAX_BUFFER_SIZE, val.bstrVal);
                    }
                    else if (wcscmp(nodeName, L"buildType") == 0)
                    {
                        wcscpy_s(&mType[0], MAX_BUFFER_SIZE, val.bstrVal);
                    }
                    else if (wcscmp(nodeName, L"eulaURL") == 0)
                    {
                        wcscpy_s(&mEulaUrl[0], MAX_BUFFER_SIZE, val.bstrVal);
                    }
                    else if (wcscmp(nodeName, L"eulaVersion") == 0)
                    {
                        wcscpy_s(&mEulaVersion[0], MAX_BUFFER_SIZE, val.bstrVal);
                    }
                    else if (wcscmp(nodeName, L"updateRule") == 0)
                    {
                        wcscpy_s(&mUpdateRule[0], MAX_BUFFER_SIZE, val.bstrVal);
                    }
                    else if (wcscmp(nodeName, L"eulaLatestMajorVersion") == 0)
                    {
                        wcscpy_s(&mEulaLatestMajorVersion[0], MAX_BUFFER_SIZE, val.bstrVal);
                    }
                }
            }
        }
        else
        {
            parseSuccess = false;
        }
    }
    else
    {
        parseSuccess = false;
    }

    if (!parseSuccess)
    {
        ORIGINBS_LOG_ERROR << "Downloader: parsing of XML failed";
    }

    return parseSuccess;
}

/// \brief Set error text contents for reporting errors to the Origin client.
/// Also accepts an error code which is appended onto the given error text.
void Downloader::SetErrorMessage(const char* errorText, const DWORD errorCode)
{
    // Only store latest error message so we clear our contents
    mErrorMessage.clear();
    mErrorMessage.str("");

    if (errorText)
    {
        mErrorMessage << errorText << ":" << errorCode;
    }
    else
    {
        mErrorMessage << errorCode;
    }
}

/// \brief Overloaded method that sets the error message based on the given
/// error code alone (with no additional error text).
void Downloader::SetErrorMessage(const DWORD errorCode)
{
    SetErrorMessage(NULL, errorCode);
}

bool Downloader::IsOldSettingsFileOptIn()
{
    bool foundOldBetaSetting = false;
    wchar_t settingsFile[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, settingsFile);
    wcscat_s(settingsFile, MAX_PATH, L"\\Origin\\Settings.xml");

    HANDLE hOutputFile = CreateFile(settingsFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOutputFile != INVALID_HANDLE_VALUE)
    {
        DWORD fileSize = GetFileSize(hOutputFile, NULL);
        LPSTR pszOutBuffer;
        LPWSTR wBuffer;
        DWORD dwBytesRead;
        pszOutBuffer = new char[fileSize + 1];
        wBuffer = new wchar_t[fileSize + 1];
        ReadFile(hOutputFile, LPVOID(pszOutBuffer), fileSize, &dwBytesRead, NULL);
        wBuffer[MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pszOutBuffer, dwBytesRead, wBuffer, dwBytesRead)] = 0;
        CloseHandle(hOutputFile);

        CComPtr<IXMLDOMDocument> iXMLDoc;
        CComPtr<IXMLDOMElement> iRootElm;
        CComPtr<IXMLDOMNodeList> iNodeList;
        long length;
        VARIANT_BOOL bSuccess;

        iXMLDoc.CoCreateInstance(__uuidof(DOMDocument));

        if (iXMLDoc)
        {
            bool parseOk = true;

            iXMLDoc->loadXML(wBuffer, &bSuccess);
            if (!bSuccess)
            {
                ORIGINBS_LOG_ERROR << "Failure to load Legacy settings";
            }
            iXMLDoc->get_documentElement(&iRootElm);
            if (iRootElm)
            {
                iRootElm->get_childNodes(&iNodeList);
                if (iNodeList)
                {
                    iNodeList->get_length(&length);
                    for (int i = 0; i < length; i++)
                    {
                        CComPtr<IXMLDOMNode> iNode;
                        CComPtr<IXMLDOMNamedNodeMap> iAttrs;
                        iNodeList->nextNode(&iNode);
                        iNode->get_attributes(&iAttrs);
                        if (iAttrs)
                        {
                            CComPtr<IXMLDOMNode> item;
                            iAttrs->getNamedItem(L"key", &item);
                            if (item)
                            {
                                VARIANT val;
                                item->get_nodeValue(&val);
                                if (wcscmp(val.bstrVal, L"BetaOptIn") == 0)
                                {
                                    CComPtr<IXMLDOMNode> item2;
                                    iAttrs->getNamedItem(L"value", &item2);
                                    if (item2)
                                    {
                                        item2->get_nodeValue(&val);
                                        mIsBetaOptIn = (wcscmp(val.bstrVal, L"true") == 0);
                                        foundOldBetaSetting = true;
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    parseOk = false;
                }
            }
            else
            {
                parseOk = false;
            }

            if (!parseOk)
            {
                ORIGINBS_LOG_ERROR << "Parsing of legacy settings failed";
            }
        }
        else
        {
            ORIGINBS_LOG_ERROR << "Unable to create instance for legacy settings";
        }

        delete[] wBuffer;
        delete[] pszOutBuffer;
    }

    return foundOldBetaSetting;
}

bool Downloader::QuerySettingsFile()
{
    bool foundOldBetaSetting = IsOldSettingsFileOptIn();

    wchar_t settingsFile[MAX_PATH];
    wchar_t settingsFolder[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, settingsFile);
    wcscat_s(settingsFile, MAX_PATH, L"\\Origin\\local.xml");
    SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, settingsFolder);
    wcscat_s(settingsFolder, MAX_PATH, L"\\Origin");

    HANDLE hOutputFile = CreateFile(settingsFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOutputFile == INVALID_HANDLE_VALUE)
    {
        int error = GetLastError();
        if (error == ERROR_PATH_NOT_FOUND)
        {
            // Need to create this path
            CreateDirectory(settingsFolder, NULL);
        }
        // Write our settings file
        HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_SETTINGS), L"XML");
        hOutputFile = CreateFile(settingsFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

        LogServicePlatform::grantEveryoneAccessToFile(settingsFile);

        HGLOBAL hBytes = LoadResource(NULL, hRes);
        DWORD dSize = SizeofResource(NULL, hRes);
        wchar_t* buffer = reinterpret_cast<wchar_t*>(LockResource(hBytes));

        DWORD dWritten = 0;

        WriteFile(hOutputFile, buffer, dSize, &dWritten, NULL);
        CloseHandle(hOutputFile);
        FreeResource(hBytes);

        return false;   // file did not exist
    }
    else
    {
        DWORD fileSize = GetFileSize(hOutputFile, NULL);
        LPSTR pszOutBuffer;
        LPWSTR wBuffer;
        DWORD dwBytesRead;
        pszOutBuffer = new char[fileSize + 1];
        wBuffer = new wchar_t[fileSize + 1];
        ReadFile(hOutputFile, LPVOID(pszOutBuffer), fileSize, &dwBytesRead, NULL);
        wBuffer[MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pszOutBuffer, dwBytesRead, wBuffer, dwBytesRead)] = 0;
        CloseHandle(hOutputFile);

        CComPtr<IXMLDOMDocument> iXMLDoc;
        CComPtr<IXMLDOMElement> iRootElm;
        CComPtr<IXMLDOMNodeList> iNodeList;
        long length;
        VARIANT_BOOL bSuccess;

        iXMLDoc.CoCreateInstance(__uuidof(DOMDocument));

        if (iXMLDoc)
        {
            bool parseOk = true;

            iXMLDoc->loadXML(wBuffer, &bSuccess);
            if (!bSuccess)
            {
                ORIGINBS_LOG_ERROR << "Failure to load settings XML";
            }
            iXMLDoc->get_documentElement(&iRootElm);
            if (iRootElm)
            {
                iRootElm->get_childNodes(&iNodeList);
                if (iNodeList)
                {
                    iNodeList->get_length(&length);
                    for (int i = 0; i < length; i++)
                    {
                        CComPtr<IXMLDOMNode> iNode;
                        CComPtr<IXMLDOMNamedNodeMap> iAttrs;
                        iNodeList->nextNode(&iNode);
                        iNode->get_attributes(&iAttrs);
                        if (iAttrs)
                        {
                            CComPtr<IXMLDOMNode> item;
                            iAttrs->getNamedItem(L"key", &item);
                            if (item)
                            {
                                VARIANT val;
                                item->get_nodeValue(&val);
                                if (wcscmp(val.bstrVal, L"AutoUpdate") == 0)
                                {
                                    CComPtr<IXMLDOMNode> item2;
                                    iAttrs->getNamedItem(L"value", &item2);
                                    if (item2)
                                    {
                                        item2->get_nodeValue(&val);
                                        mIsAutoUpdate = (wcscmp(val.bstrVal, L"true") == 0);
                                    }
                                }
                                if (wcscmp(val.bstrVal, L"AutoPatchGlobal") == 0)
                                {
                                    CComPtr<IXMLDOMNode> item2;
                                    iAttrs->getNamedItem(L"value", &item2);
                                    if (item2)
                                    {
                                        item2->get_nodeValue(&val);
                                        mAutoPatch = (wcscmp(val.bstrVal, L"true") == 0);
                                    }
                                }
                                if (wcscmp(val.bstrVal, L"TelemetryHWSurveyOptOut") == 0)
                                {
                                    CComPtr<IXMLDOMNode> item2;
                                    iAttrs->getNamedItem(L"value", &item2);
                                    if (item2)
                                    {
                                        item2->get_nodeValue(&val);
                                        mTelemetryOptOut = (wcscmp(val.bstrVal, L"false") == 0);
                                    }
                                }
                                if (wcscmp(val.bstrVal, L"BetaOptIn") == 0 && !foundOldBetaSetting)
                                {
                                    CComPtr<IXMLDOMNode> item2;
                                    iAttrs->getNamedItem(L"value", &item2);
                                    if (item2)
                                    {
                                        item2->get_nodeValue(&val);
                                        mIsBetaOptIn = (wcscmp(val.bstrVal, L"true") == 0);
                                    }
                                }
                                if (wcscmp(val.bstrVal, L"AcceptedEULAVersion") == 0)
                                {
                                    CComPtr<IXMLDOMNode> item2;
                                    iAttrs->getNamedItem(L"value", &item2);
                                    if (item2)
                                    {
                                        item2->get_nodeValue(&val);
                                        mCurEULAVersion = _wtoi(val.bstrVal);
                                    }
                                }
                                if (wcscmp(val.bstrVal, L"Locale") == 0)
                                {
                                    CComPtr<IXMLDOMNode> item2;
                                    iAttrs->getNamedItem(L"value", &item2);
                                    if (item2)
                                    {
                                        item2->get_nodeValue(&val);
                                        Locale::CreateInstance(val.bstrVal);
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    parseOk = false;
                }
            }
            else
            {
                parseOk = false;
            }

            if (!parseOk)
            {
                ORIGINBS_LOG_ERROR << "Unable to parse settings";
            }
        }
        else
        {
            ORIGINBS_LOG_ERROR << "Unable to create instance of settings";
        }

        delete[] wBuffer;
        delete[] pszOutBuffer;
        
        return true;    // file does exist
    }
}

void Downloader::WriteSettings(bool writeEulaVersion, wchar_t* eulaVersion, wchar_t* eulaLocation, bool writeInstallerSettings, bool autoUpdate, bool autoPatch, bool telemOO, bool writeTimestamp)
{
    wchar_t settingsFile[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, settingsFile);
    wcscat_s(settingsFile, MAX_PATH, L"\\Origin\\local.xml");

	// Keep track of what the settings should be in case we fail to save the info or if we try to call this method multiple times with only partial data
	if (writeInstallerSettings)
	{
		mIsAutoUpdate = autoUpdate;
        mAutoPatch = autoPatch;
		mTelemetryOptOut = telemOO;
	}

    HANDLE hOutputFile = CreateFile(settingsFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    int error = GetLastError();
    if (error == ERROR_FILE_NOT_FOUND || hOutputFile == INVALID_HANDLE_VALUE)
    {
        // Since we would have written the file when we queried, we should never hit this
        ORIGINBS_LOG_ERROR << "WriteSettings: unable to find file";
        return;
    }
    else
    {
        DWORD fileSize = GetFileSize(hOutputFile, NULL);
        LPSTR pszOutBuffer;
        LPWSTR wBuffer;
        DWORD dwBytesRead;
        pszOutBuffer = new char[fileSize + 1];
        wBuffer = new wchar_t[fileSize + 1];
        ReadFile(hOutputFile, LPVOID(pszOutBuffer), fileSize, &dwBytesRead, NULL);
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pszOutBuffer, dwBytesRead, wBuffer, dwBytesRead);
        wBuffer[dwBytesRead] = 0;
        CloseHandle(hOutputFile);

        CComPtr<IXMLDOMDocument> iXMLDoc;
        CComPtr<IXMLDOMElement> iRootElm;
        CComPtr<IXMLDOMNodeList> iNodeList;
        long length;
        VARIANT_BOOL bSuccess;
        iXMLDoc.CoCreateInstance(__uuidof(DOMDocument));
        if (iXMLDoc)
        {
            iXMLDoc->loadXML(wBuffer, &bSuccess);
            if (!bSuccess)
            {
                ORIGINBS_LOG_ERROR << "WriteSettings: unable to load settings";
            }

            iXMLDoc->get_documentElement(&iRootElm);
            if (iRootElm)
            {
                iRootElm->get_childNodes(&iNodeList);
                if (iNodeList)
                {
                    bool foundEULAVersion = false;
                    bool foundEULALocation = false;
                    bool foundAutoUpdate = false;
                    bool foundAutoPatch = false;
                    bool foundTelemOO = false;
                    bool foundTimestamp = false;
                    iNodeList->get_length(&length);
                    for (int i = 0; i < length; i++)
                    {
                        CComPtr<IXMLDOMNode> iNode;
                        CComPtr<IXMLDOMNamedNodeMap> iAttrs;
                        iNodeList->nextNode(&iNode);
                        iNode->get_attributes(&iAttrs);
                        if (iAttrs)
                        {
                            CComPtr<IXMLDOMNode> item;
                            iAttrs->getNamedItem(L"key", &item);
                            if (item)
                            {
                                VARIANT val;
                                item->get_nodeValue(&val);
                                if (wcscmp(val.bstrVal, L"AcceptedEULAVersion") == 0 && writeEulaVersion)
                                {
                                    CComPtr<IXMLDOMAttribute> iAttribute;
                                    iXMLDoc->createAttribute(L"value", &iAttribute);
                                    iAttribute->put_nodeValue(_variant_t(eulaVersion));
                                    CComPtr<IXMLDOMNode> versionItem = NULL;
                                    HRESULT result = iAttrs->setNamedItem(iAttribute, &versionItem);
                                    foundEULAVersion = true;
                                    if (result != S_OK)
                                    {
                                        ORIGINBS_LOG_ERROR << "WriteSettings: unable to update EULA version[" << eulaVersion << "] err = " << result;
                                    }
                                }
                                if (wcscmp(val.bstrVal, L"AcceptedEULALocation") == 0 && writeEulaVersion)
                                {
                                    CComPtr<IXMLDOMAttribute> iAttribute;
                                    iXMLDoc->createAttribute(L"value", &iAttribute);
                                    iAttribute->put_nodeValue(_variant_t(eulaLocation));
                                    CComPtr<IXMLDOMNode> versionItem = NULL;
                                    HRESULT result = iAttrs->setNamedItem(iAttribute, &versionItem);
                                    foundEULALocation = true;
                                    if (result != S_OK)
                                    {
                                        ORIGINBS_LOG_ERROR << "WriteSettings: unable to update EULA location[" << eulaLocation << "] err = " << result;
                                    }
                                }
                                if (wcscmp(val.bstrVal, L"AutoUpdate") == 0 && writeInstallerSettings)
                                {
                                    CComPtr<IXMLDOMAttribute> iAttribute;
                                    iXMLDoc->createAttribute(L"value", &iAttribute);
                                    if (autoUpdate)
                                    {
                                        iAttribute->put_nodeValue(_variant_t(L"true"));
                                    }
                                    else
                                    {
                                        iAttribute->put_nodeValue(_variant_t(L"false"));
                                    }
                                    CComPtr<IXMLDOMNode> versionItem = NULL;
                                    HRESULT result = iAttrs->setNamedItem(iAttribute, &versionItem);
                                    foundAutoUpdate = true;
                                    if (result != S_OK)
                                    {
                                        ORIGINBS_LOG_ERROR << "WriteSettings: unable to update autoUpdate[" << autoUpdate << "] err = " << result;
                                    }
                                }
                                if (wcscmp(val.bstrVal, L"AutoPatchGlobal") == 0 && writeInstallerSettings)
                                {
                                    CComPtr<IXMLDOMAttribute> iAttribute;
                                    iXMLDoc->createAttribute(L"value", &iAttribute);
                                    if (autoPatch)
                                    {
                                        iAttribute->put_nodeValue(_variant_t(L"true"));
                                    }
                                    else
                                    {
                                        iAttribute->put_nodeValue(_variant_t(L"false"));
                                    }
                                    CComPtr<IXMLDOMNode> versionItem = NULL;
                                    HRESULT result = iAttrs->setNamedItem(iAttribute, &versionItem);
                                    foundAutoPatch = true;
                                    if (result != S_OK)
                                    {
                                        ORIGINBS_LOG_ERROR << "WriteSettings: unable to update AutoPatchGlobal[" << autoPatch << "] err = " << result;
                                    }
                                }
                                if (wcscmp(val.bstrVal, L"TelemetryHWSurveyOptOut") == 0 && writeInstallerSettings)
                                {
                                    CComPtr<IXMLDOMAttribute> iAttribute;
                                    iXMLDoc->createAttribute(L"value", &iAttribute);
                                    if (telemOO)
                                    {
                                        iAttribute->put_nodeValue(_variant_t(L"false"));
                                    }
                                    else
                                    {
                                        iAttribute->put_nodeValue(_variant_t(L"true"));
                                    }
                                    CComPtr<IXMLDOMNode> versionItem = NULL;
                                    HRESULT result = iAttrs->setNamedItem(iAttribute, &versionItem);
                                    foundTelemOO = true;
                                    if (result != S_OK)
                                    {
                                        ORIGINBS_LOG_ERROR << "WriteSettings: unable to update telemOO[" << telemOO << "] err = " << result;
                                    }
                                }
                                if (wcscmp(val.bstrVal, L"InstallTimestamp") == 0 && writeTimestamp)
                                {
                                    CComPtr<IXMLDOMAttribute> iAttribute;
                                    iXMLDoc->createAttribute(L"value", &iAttribute);
                                    time_t timestamp;
                                    time(&timestamp);
                                    iAttribute->put_nodeValue(_variant_t(timestamp));
                                    CComPtr<IXMLDOMNode> versionItem = NULL;
                                    HRESULT result = iAttrs->setNamedItem(iAttribute, &versionItem);
                                    foundTimestamp = true;
                                    if (result != S_OK)
                                    {
                                        ORIGINBS_LOG_ERROR << "WriteSettings: unable to update InstallTimestamp[" << (long)timestamp << "] err = " << result;
                                    }
                                }
                            }
                        }
                    }

                    // If we didn't find our EULA then we need to modify our XML since the tag isn't there
                    if (!foundEULAVersion && writeEulaVersion)
                    {
                        CComPtr<IXMLDOMNode> retNode;
                        CComVariant nodeType;
                        nodeType = NODE_ELEMENT;

                        CComPtr<IXMLDOMNode> newNode;
                        CComPtr<IXMLDOMNamedNodeMap> iAttrs;
                        iXMLDoc->createNode(nodeType, CComBSTR("Setting"), CComBSTR(""), &newNode);
                        newNode->get_attributes(&iAttrs);

                        CComPtr<IXMLDOMAttribute> key;
                        iXMLDoc->createAttribute(L"key", &key);
                        key->put_nodeValue(_variant_t("AcceptedEULAVersion"));
                        iAttrs->setNamedItem(key, NULL);

                        CComPtr<IXMLDOMAttribute> type;
                        iXMLDoc->createAttribute(L"type", &type);
                        type->put_nodeValue(_variant_t("2"));
                        iAttrs->setNamedItem(type, NULL);

                        CComPtr<IXMLDOMAttribute> val;
                        iXMLDoc->createAttribute(L"value", &val);
                        val->put_nodeValue(_variant_t(eulaVersion));
                        iAttrs->setNamedItem(val, NULL);

                        HRESULT result = iRootElm->appendChild(newNode, &retNode);
                        if (result != S_OK)
                        {
                            ORIGINBS_LOG_ERROR << "WriteSettings: unable to add EULA version[" << eulaVersion << "] err = " << result;
                        }
                    }

                    if (!foundEULALocation && writeEulaVersion)
                    {
                        CComPtr<IXMLDOMNode> retNode;
                        CComVariant nodeType;
                        nodeType = NODE_ELEMENT;

                        CComPtr<IXMLDOMNode> newNode;
                        CComPtr<IXMLDOMNamedNodeMap> iAttrs;
                        iXMLDoc->createNode(nodeType, CComBSTR("Setting"), CComBSTR(""), &newNode);
                        newNode->get_attributes(&iAttrs);

                        CComPtr<IXMLDOMAttribute> key;
                        iXMLDoc->createAttribute(L"key", &key);
                        key->put_nodeValue(_variant_t("AcceptedEULALocation"));
                        iAttrs->setNamedItem(key, NULL);

                        CComPtr<IXMLDOMAttribute> type;
                        iXMLDoc->createAttribute(L"type", &type);
                        type->put_nodeValue(_variant_t("10"));
                        iAttrs->setNamedItem(type, NULL);

                        CComPtr<IXMLDOMAttribute> val;
                        iXMLDoc->createAttribute(L"value", &val);
                        val->put_nodeValue(_variant_t(eulaLocation));
                        iAttrs->setNamedItem(val, NULL);

                        HRESULT result = iRootElm->appendChild(newNode, &retNode);
                        if (result != S_OK)
                        {
                            ORIGINBS_LOG_ERROR << "WriteSettings: unable to add EULA location [" << eulaLocation << "] err = " << result;
                        }
                    }

                    if (!foundAutoUpdate && writeInstallerSettings)
                    {
                        CComPtr<IXMLDOMNode> retNode;
                        CComVariant nodeType;
                        nodeType = NODE_ELEMENT;

                        CComPtr<IXMLDOMNode> newNode;
                        CComPtr<IXMLDOMNamedNodeMap> iAttrs;
                        iXMLDoc->createNode(nodeType, CComBSTR("Setting"), CComBSTR(""), &newNode);
                        newNode->get_attributes(&iAttrs);

                        CComPtr<IXMLDOMAttribute> key;
                        iXMLDoc->createAttribute(L"key", &key);
                        key->put_nodeValue(_variant_t("AutoUpdate"));
                        iAttrs->setNamedItem(key, NULL);

                        CComPtr<IXMLDOMAttribute> type;
                        iXMLDoc->createAttribute(L"type", &type);
                        type->put_nodeValue(_variant_t("1"));
                        iAttrs->setNamedItem(type, NULL);

                        CComPtr<IXMLDOMAttribute> val;
                        iXMLDoc->createAttribute(L"value", &val);
                        if (autoUpdate)
                        {
                            val->put_nodeValue(_variant_t(L"true"));
                        }
                        else
                        {
                            val->put_nodeValue(_variant_t(L"false"));
                        }
                        iAttrs->setNamedItem(val, NULL);

                        HRESULT result = iRootElm->appendChild(newNode, &retNode);
                        if (result != S_OK)
                        {
                            ORIGINBS_LOG_ERROR << "WriteSettings: unable to add autoUpdate[" << autoUpdate << "] err = " << result;
                        }
                    }

                    if (!foundAutoPatch && writeInstallerSettings)
                    {
                        CComPtr<IXMLDOMNode> retNode;
                        CComVariant nodeType;
                        nodeType = NODE_ELEMENT;

                        CComPtr<IXMLDOMNode> newNode;
                        CComPtr<IXMLDOMNamedNodeMap> iAttrs;
                        iXMLDoc->createNode(nodeType, CComBSTR("Setting"), CComBSTR(""), &newNode);
                        newNode->get_attributes(&iAttrs);

                        CComPtr<IXMLDOMAttribute> key;
                        iXMLDoc->createAttribute(L"key", &key);
                        key->put_nodeValue(_variant_t("AutoPatchGlobal"));
                        iAttrs->setNamedItem(key, NULL);

                        CComPtr<IXMLDOMAttribute> type;
                        iXMLDoc->createAttribute(L"type", &type);
                        type->put_nodeValue(_variant_t("1"));
                        iAttrs->setNamedItem(type, NULL);

                        CComPtr<IXMLDOMAttribute> val;
                        iXMLDoc->createAttribute(L"value", &val);
                        if (autoPatch)
                        {
                            val->put_nodeValue(_variant_t(L"true"));
                        }
                        else
                        {
                            val->put_nodeValue(_variant_t(L"false"));
                        }
                        iAttrs->setNamedItem(val, NULL);

                        HRESULT result = iRootElm->appendChild(newNode, &retNode);
                        if (result != S_OK)
                        {
                            ORIGINBS_LOG_ERROR << "WriteSettings: unable to add autoPatch[" << autoPatch << "] err = " << result;
                        }
                    }

                    if (!foundTelemOO && writeInstallerSettings)
                    {
                        CComPtr<IXMLDOMNode> retNode;
                        CComVariant nodeType;
                        nodeType = NODE_ELEMENT;

                        CComPtr<IXMLDOMNode> newNode;
                        CComPtr<IXMLDOMNamedNodeMap> iAttrs;
                        iXMLDoc->createNode(nodeType, CComBSTR("Setting"), CComBSTR(""), &newNode);
                        newNode->get_attributes(&iAttrs);

                        CComPtr<IXMLDOMAttribute> key;
                        iXMLDoc->createAttribute(L"key", &key);
                        key->put_nodeValue(_variant_t("TelemetryHWSurveyOptOut"));
                        iAttrs->setNamedItem(key, NULL);

                        CComPtr<IXMLDOMAttribute> type;
                        iXMLDoc->createAttribute(L"type", &type);
                        type->put_nodeValue(_variant_t("1"));
                        iAttrs->setNamedItem(type, NULL);

                        CComPtr<IXMLDOMAttribute> val;
                        iXMLDoc->createAttribute(L"value", &val);

                        if (autoUpdate)
                        {
                            val->put_nodeValue(_variant_t(L"false"));
                        }
                        else
                        {
                            val->put_nodeValue(_variant_t(L"true"));
                        }
                        iAttrs->setNamedItem(val, NULL);

                        HRESULT result = iRootElm->appendChild(newNode, &retNode);
                        if (result != S_OK)
                        {
                            ORIGINBS_LOG_ERROR << "WriteSettings: unable to add telemOO [" << telemOO << "] err = " << result;
                        }
                    }

                    if (!foundTimestamp && writeTimestamp)
                    {
                        CComPtr<IXMLDOMNode> retNode;
                        CComVariant nodeType;
                        nodeType = NODE_ELEMENT;

                        CComPtr<IXMLDOMNode> newNode;
                        CComPtr<IXMLDOMNamedNodeMap> iAttrs;
                        iXMLDoc->createNode(nodeType, CComBSTR("Setting"), CComBSTR(""), &newNode);
                        newNode->get_attributes(&iAttrs);

                        CComPtr<IXMLDOMAttribute> key;
                        iXMLDoc->createAttribute(L"key", &key);
                        key->put_nodeValue(_variant_t("InstallTimestamp"));
                        iAttrs->setNamedItem(key, NULL);

                        CComPtr<IXMLDOMAttribute> type;
                        iXMLDoc->createAttribute(L"type", &type);
                        type->put_nodeValue(_variant_t("5"));
                        iAttrs->setNamedItem(type, NULL);

                        CComPtr<IXMLDOMAttribute> val;

                        iXMLDoc->createAttribute(L"value", &val);
                        time_t timestamp;
                        time(&timestamp);

                        val->put_nodeValue(_variant_t(timestamp));
                        iAttrs->setNamedItem(val, NULL);

                        HRESULT result = iRootElm->appendChild(newNode, &retNode);
                        if (result != S_OK)
                        {
                            ORIGINBS_LOG_ERROR << "WriteSettings: unable to add InstallTimestamp[" << (long)timestamp << "] err = " << result;
                        }
                    }
                }
                else
                {
                    ORIGINBS_LOG_ERROR << "WriteSettings: Unable to retrive child nodes";
                }
            }
        }
        else
        {
            ORIGINBS_LOG_ERROR << "WriteSettings: unable to create instance";
        }

        iXMLDoc->save(_variant_t(settingsFile));

        delete[] wBuffer;
        delete[] pszOutBuffer;
    }
}

bool Downloader::DownloadOriginConfig(const wchar_t* environment, const wchar_t* version, bool ignoreSSLErrors)
{
	wchar_t url[1024];
	if (_wcsicmp(environment, L"production") == 0)
    {
        wsprintfW(url, L"https://secure.download.dm.origin.com/production/autopatch/2/init/%s", version);
    }
    else
    {
	    wsprintfW(url, L"https://stage.secure.download.dm.origin.com/%s/autopatch/2/init/%s", environment, version); 
    }

    LARGE_INTEGER frequency;
    LARGE_INTEGER startReceive, endReceive, elapsedMilliseconds;

    // Calculate when the response began
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&startReceive);

    const bool openSessionSuccess = OpenSession(url, ignoreSSLErrors);

    // Record how long the response took to complete.
    QueryPerformanceCounter(&endReceive);

	if (!openSessionSuccess)
	{
		ORIGINBS_LOG_ERROR << "Failed to open session for config fetch: " << GetLastError();
		return false;
	}

    // Calculate response duration
    elapsedMilliseconds.QuadPart = endReceive.QuadPart - startReceive.QuadPart;

    // In rare occurrences, frequency may be reported as zero
    if (frequency.QuadPart == 0)
    {
        // Note: when performing telemetry queries, responses of 0ms should
        // be omitted.
        SetErrorMessage(0UL);
    }

    else
    {
        // Convert to milliseconds first to prevent loss of precision
        elapsedMilliseconds.QuadPart *= 1000;
        elapsedMilliseconds.QuadPart /= frequency.QuadPart;

        SetErrorMessage((DWORD) elapsedMilliseconds.QuadPart);
    }

	// Check status code
	wchar_t szResponse[64] = {0};
	DWORD size = 32;
    DWORD statusCode = 0;
	
	WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE, WINHTTP_HEADER_NAME_BY_INDEX,
						&szResponse, &size, WINHTTP_NO_HEADER_INDEX);
	
	statusCode = _wtoi(szResponse);

	// Turn our dashes into underscores for the file on disk
	wchar_t normalizedEnvironment[MAX_PATH];
    wcscpy_s(&normalizedEnvironment[0], MAX_PATH, environment);

	wchar_t* dash = wcschr(normalizedEnvironment, L'-');
    while ( dash )
    {
        *dash = L'_';
        dash = wcschr(normalizedEnvironment, L'-');
    }

	HANDLE originConfigFile = CreateStagedOriginConfigFile(normalizedEnvironment);
	bool errorOccurred = false;

	// Allocate space for the buffer.
    char* pszOutBuffer = NULL;
  	if (statusCode == 200)
	{
        // Check for available data.
		DWORD dwMaxChunkSize = 4000;
        DWORD dwAvailable= 0;
        DWORD dwDownloaded = 0;

		pszOutBuffer = new char[dwMaxChunkSize+1];

        if(!pszOutBuffer)
        {
            ORIGINBS_LOG_ERROR << "Failed to allocate memory for config download [last error: " << GetLastError() << "]";
            errorOccurred = true;
        }
        else
        {
            do 
            {
				DWORD dwChunkSize = 0;
				dwAvailable = 0;

                if(!WinHttpQueryDataAvailable(hRequest, &dwAvailable))
                {
                    ORIGINBS_LOG_ERROR << "Failed to determine how much data is available from config response [last error: " << GetLastError() << "]";
                    errorOccurred = true;
                    break;
                }

                dwChunkSize = (dwAvailable > dwMaxChunkSize) ? dwMaxChunkSize : dwAvailable;

                if (CheckAvailableDiskSpace(dwChunkSize))
                {
                    ZeroMemory(pszOutBuffer, dwChunkSize+1);

                    if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwChunkSize, &dwDownloaded))
                    {
                        DWORD dwWriteSize = 0;
                        BOOL writeOk = WriteFile(originConfigFile, pszOutBuffer, dwDownloaded, &dwWriteSize, NULL);

                        if (!writeOk || dwWriteSize != dwDownloaded)
                        {
                            int err = GetLastError ();
                            ORIGINBS_LOG_ERROR << "DownloadOriginConfig: failed to write out downloaded init data. err = " << err;
                            errorOccurred = true;
                            break;
                        }
                    }	
                }
                else
                {
                    errorOccurred = true;
                }

            } while (dwDownloaded > 0 && !errorOccurred);
        }
	}
	else
	{
		ORIGINBS_LOG_ERROR << "DownloadOriginConfig status code: " << statusCode;
	}

	delete[] pszOutBuffer;

	CloseHandle(originConfigFile);

	if (!errorOccurred && statusCode == 200)
	{
		// Downloaded file successfully, copy staged file over
		return CopyStagedConfigFile(normalizedEnvironment);
	}
	else
	{
		RemoveStagedConfigFile(normalizedEnvironment);
        return false;
	}
}

bool Downloader::CopyStagedConfigFile(const wchar_t* environment)
{
    TCHAR szPath[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, szPath))) 
    {
        ORIGINBS_LOG_ERROR << "Failed to get appdata path to copy staged config file over cached file: " << GetLastError();
        return false;
    }

    std::wstring strProgramDataOrigin (szPath);
    strProgramDataOrigin.append (L"\\Origin");

	wchar_t path[MAX_PATH], staged_path[MAX_PATH];

	wsprintfW(staged_path, STAGED_CONFIG_FILE, strProgramDataOrigin.c_str(), environment);
	wsprintfW(path, CONFIG_FILE, strProgramDataOrigin.c_str(), environment);

	// Move and replace
	if (!MoveFileEx(staged_path, path, 1))
	{
		ORIGINBS_LOG_ERROR << "Failed to copy staged config file over cached file: " << GetLastError();

		RemoveStagedConfigFile(environment);
        return false;
	}
    else
    {
        return true;
    }
}

void Downloader::RemoveStagedConfigFile(const wchar_t* environment)
{
    TCHAR szPath[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, szPath))) 
    {
        ORIGINBS_LOG_ERROR << "Failed to get appdata path to delete staged config file: " << GetLastError();
        return;
    }

    std::wstring strProgramDataOrigin (szPath);
    strProgramDataOrigin.append (L"\\Origin");

	wchar_t path[MAX_PATH];

	wsprintfW(path, STAGED_CONFIG_FILE, strProgramDataOrigin.c_str(), environment);

	if (!DeleteFile(path))
	{
		ORIGINBS_LOG_ERROR << "Failed to remove staged config file: " << GetLastError();
	}
}

void Downloader::SetAutoUpdate(bool autoUpdate)
{
	if (autoUpdate == mIsAutoUpdate)
		return;

	bool writeEulaVersion = false;
	wchar_t* eulaVersion = NULL;
	wchar_t* eulaLocation = NULL;
	bool writeInstallerSettings = true;
    bool writeTimestamp = true;

    WriteSettings(writeEulaVersion, eulaVersion, eulaLocation, writeInstallerSettings, autoUpdate, mAutoPatch, mTelemetryOptOut, writeTimestamp);
}
