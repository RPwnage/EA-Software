///////////////////////////////////////////////////////////////////////////////
// EnvUtils.cpp
// Environmental Utilities
// Copyright (c) 2010-2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef _ENVUTILS_H_INCLUDED_
#define _ENVUTILS_H_INCLUDED_

#include <QString>
#include <QDir>
#include <QFile>

#include "services/plugin/PluginAPI.h"

/// \brief Contains various helper functions relating to the OS environment.
class ORIGIN_PLUGIN_API EnvUtils
{
public:
    /// \brief Converts a "long" path to a Windows short path.
    /// \param sPath The path to shorten.
    /// \return A Windows short path if called on Windows, otherwise the original path.
    static QString ConvertToShortPath(const QString& sPath);

    /// \brief Deletes a folder if it exists.
    /// \param sFolderName A path to the folder to delete.
    /// \param bSafety Deprecated.
    /// \param bIncludingRoot If true, the function will also delete the folder specified by sFolderName.
    /// \param pProgressBar A pointer to a progress bar object.
    /// \param pfcnCallback A pointer to a callback function that will update pProgressBar.
    /// \return True if deletion was successful.
    static bool DeleteFolderIfPresent( const QString& sFolderName, bool bSafety = false, bool bIncludingRoot = false, void *pProgressBar = NULL, void (*pfcnCallback)(void*) = NULL );
    
    /// \brief Gets the version of the current OS in a platform-specific format, typically "major.minor" on Windows, "major.minor.fix" on OS X.
    ///
    /// More info on versions available here: http://msdn.microsoft.com/en-us/library/ms724833(v=vs.85).aspx
    /// \return A QString representing the OS version.
    static QString GetOSVersion();

    /// \brief Whether the running OS is 64 bit or not
    /// \return True if 64 bit OS
    static bool GetIS64BitOS();

    /// \brief Whether this is Windows XP or older 
    static bool GetISWindowsXPOrOlder();

    /// \brief Whether the running process is elevated or not (presumed to be true for XP users who are administrators)
    /// \return True if the process is elevated (has administrative permissions)
    static bool GetIsProcessElevated();

    /// \brief Recursively scans a folder tree and returns all entries matching a pattern
    /// \param path - base path to scan
    /// \param files - returned set of entries
    static void ScanFolders(QString path, QFileInfoList& files); 

    /// \brief Returns true if the parent process that launched this client is a browser
    static bool GetParentProcessIsBrowser();

#ifdef ORIGIN_PC
    static QString GetOriginBootstrapVersionString();

    struct FileLockProcessInfo
    {
        QString toString()
        {
            return QString("%1 [Name: %2 ; Type: %3 ; PID: %4]").arg(processPath).arg(processDisplayName).arg(processType).arg(processPid);
        }

        QString processPath;
        QString processDisplayName;
        QString processType;
        int processPid;
    };

    static bool GetFileInUseDetails(const QString& filePath, QString& rebootReason, QString& lockingProcessesSummary, QList<FileLockProcessInfo>& processes);
#endif

    static bool GetFileExistsNative(const QString& filePath);
    static bool GetFileDetailsNative(const QString& filePath, qint64& fileSize);
    static bool IsDriveAvailable( const QString& sPath );
};


#endif

