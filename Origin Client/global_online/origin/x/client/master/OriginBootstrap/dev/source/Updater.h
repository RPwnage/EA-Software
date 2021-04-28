///////////////////////////////////////////////////////////////////////////////
// Updater.h
// 
// Created by Kevin Moore
// Copyright (c) 2011 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef UPDATER_H
#define UPDATER_H

#include <windows.h>
#include <string>

class Downloader;

#define EULA_ACCEPTED WM_USER + 3
#define EULA_DECLINED WM_USER + 4
#define EULA_CHECKED WM_USER + 5
#define EULA_UNCHECKED WM_USER + 6
#define EULA_PRINT WM_USER + 7
#define UPDATE_INSTALL WM_USER + 8
#define UPDATE_LATER WM_USER + 9
#define UPDATE_EXIT WM_USER + 10
#define UPDATE_OFFLINE WM_USER + 11

class Updater
{
public:
	Updater();
	~Updater();

	void DoUpdates();
	bool RunOfflineMode() const {return mIsOfflineMode;}
	wchar_t* Updated() const {return mUpdated;}

	/// \brief returns whether the download was successful or not
	bool isDownloadSuccessful() const { return mIsDownloadSuccessful; }

    /// \brief returns whether the client was already up to date or not
    bool isUpToDate() const { return mIsUpToDate; }

	/// \brief to know if there was data error
	bool isOkWinHttpReadData() const;
    
    /// \brief to know if there was connection error
    bool isOkQuerySession() const;

    void setUseEmbeddedEULA (bool embedded);

	bool DownloadConfiguration();

    /// \brief Error text returned by the downloader that is reported to the client.
    const std::string& downloaderErrorText() const { return mDownloaderErrorText; }

private:
	Updater(const Updater &obj);
    void DetermineEnvironment(const wchar_t* iniLocation, wchar_t* environmentBuf, int envBufSize);

	bool QueryUpdate(const wchar_t* version, const wchar_t* environment);
	bool Unpack(const wchar_t* unpackLocation);
	bool DownloadUpdate();
	void ShowEULA(wchar_t* url);
    bool CheckForSameVersion(const wchar_t* location, const wchar_t* versionToCheck);

	HICON hIcon;

    /// \brief Error text returned by the downloader that is reported to the client.
    std::string mDownloaderErrorText;

	bool mIsOfflineMode;
	bool mIsHasInternetConnection;
	bool mIsFoundOrigin;

	wchar_t* mUpdated;

	/// \brief Variable to discern whether the operation was successful or not
	bool mIsDownloadSuccessful;
    bool mIsUpToDate;

    /// \brief Debug flag to force the use of embedded EULAs 
    bool mUseEmbeddedEULA;

    /// \brief the operation environment the client is configured for.
    wchar_t mEnvironment[256];
};

#endif
