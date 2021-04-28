///////////////////////////////////////////////////////////////////////////////
// Downloader.h
// 
// Created by Kevin Moore
// Copyright (c) 2011 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <atlcomcli.h>
#include <MsXml.h>

#include <string>
#include <sstream>

#define DOWNLOAD_FAILED  WM_USER
#define DOWNLOAD_SUCCEEDED WM_USER + 1
#define DOWNLOAD_CANCELED WM_USER + 2
#define DOWNLOAD_NOT_ENOUGH_SPACE WM_USER + 14

class Downloader
{
public:
	Downloader();
	~Downloader();

	void Download(const wchar_t* url);
	void UserCancelDownload();
	bool UpdateCheck(const wchar_t* environment, const wchar_t* currentVersion);
	void ReadDebugResponse(const wchar_t* version, const wchar_t* type, const wchar_t* downloadLocation,
        const wchar_t* rule, const wchar_t* eulaVersion, const wchar_t* eulaLocation, const wchar_t* eulaLatestMajorVersion);

	bool IsAutoUpdate() const {return mIsAutoUpdate;}
	void SetAutoUpdate(bool autoUpdate);
    bool IsAutoPatch() const { return mAutoPatch; }
    bool IsTelemetryOptOut() const { return mTelemetryOptOut; }
    bool IsBetaOptIn() const { return mIsBetaOptIn; }
	bool GetInternetConnection() const {return mInternetConnection;}
    bool SettingsFileFound() const { return mSettingsFileFound; }

	bool GetServiceAvailable() const {return mServiceAvailable;}

	int GetAcceptedEULAVersion() const {return mCurEULAVersion;}
	void WriteSettings(bool writeEULAVersion, wchar_t* eulaVersion, wchar_t* eulaLocation, bool writeInstallerSettings, bool autoUpdate, bool autoPatch, bool telemOO, bool writeTimestamp);

	wchar_t* getDownloadUrl() const {return mUrl;}
	wchar_t* getVersionNumber() const {return mVersionNumber;}
	wchar_t* getDownloadType() const {return mType;}
	wchar_t* getEulaURL() const {return mEulaUrl;}
	wchar_t* getEulaVersion() const {return mEulaVersion;}
	wchar_t* getEulaLatestMajorVersion() const {return mEulaLatestMajorVersion;}
	wchar_t* getUpdateRule() const {return mUpdateRule;}
	wchar_t* getOutputFileName() const {return mOutputFileName;}
    wchar_t* getTempEULAFilePath() const {return mTempEulaFilePath;}

    /// \brief Returns a copy of an error message that should be reported to the Origin client (if an error occurred).
    const std::string getErrorMessage() const {return mErrorMessage.str();}

    bool DownloadFile(wchar_t* fileUrl, HANDLE localFileHandle, bool ignoreSSLErrors);
    bool DownloadEULA(bool ignoreSSLErrors = false);
    void DeleteTempEULAFile();

	bool DownloadOriginConfig(const wchar_t* environment, const wchar_t* version, bool ignoreSSLErrors = false);
	bool CopyStagedConfigFile(const wchar_t* environment);
	void RemoveStagedConfigFile(const wchar_t* environment);

	/// \brief returns true if the update is optional
	bool isOptionalUpdate() const { return wcscmp(mUpdateRule, L"OPTIONAL") == 0;}

	/// \brief to allow parent to know if there was a data error
	bool isOkWinHttpReadData() const { return mIsOkWinHttpReadData; }

    /// \brief to allow parent to know if there was a connection error
    bool isOkQuerySession() const { return mIsOkQuerySession; }

    void setDownloadOverride(const wchar_t* overrideURL);

private:
	Downloader(const Downloader &obj);
    HANDLE CreateTempEULAFile();
	HANDLE CreateStagedOriginConfigFile(const wchar_t* environment);

    bool CheckAvailableDiskSpace(DWORD sizeNeeded);

	bool OpenSession(const wchar_t* url, bool ignoreSSLErrors = false);
	void DownloadUpdate();
	bool DownloadUpdateXML(wchar_t** buffer);
	void GetVersion(wchar_t* version, int* versionArray);
	bool QueryUpdateService(const wchar_t* environment, const wchar_t* version, const wchar_t* type, wchar_t** buffer);
	void CancelDownload(int failureCode);
	bool ParseXML(CComPtr<IXMLDOMDocument> iXMLDoc);
    void SetErrorMessage(const char* errorText, const DWORD errorCode = ERROR_SUCCESS);
    void SetErrorMessage(const DWORD errorCode = ERROR_SUCCESS);

	bool QuerySettingsFile();
    bool IsOldSettingsFileOptIn();

	/// \brief Close internet handles
	void closeWinHttpHandles();

	HINTERNET hSession;
	HINTERNET hConnect;
	HINTERNET hRequest;

    /// \brief handler for the downloaded file
	HANDLE mhDownloadedFile;

    /// \brief Error message to pass to Origin client via command-line parameter (if an error occurred)
    std::ostringstream mErrorMessage;

	wchar_t* mUrl;
	wchar_t* mVersionNumber;
	wchar_t* mType;
	wchar_t* mEulaUrl;
	wchar_t* mEulaVersion;
    wchar_t* mEulaLatestMajorVersion;
	wchar_t* mUpdateRule;
	wchar_t* mOutputFileName;
	wchar_t* mDownloadOverride;
    wchar_t* mTempEulaFilePath;
	wchar_t* mOriginConfigFileName;

	bool mURLOverride;
	bool mIsBetaOptIn;
	bool mIsAutoUpdate;
    bool mAutoPatch;
	bool mUserCancelledDownload;
	bool mInternetConnection;
	bool mServiceAvailable;
    bool mTelemetryOptOut;
    bool mSettingsFileFound;

	int mCurEULAVersion;

	/// \brief to know if there was a data error
	bool mIsOkWinHttpReadData;

    /// \brief to know if there was a connection
    bool mIsOkQuerySession;

};

#endif //DOWNLOADER_H