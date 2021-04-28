#include <limits>
#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QDesktopServices>

#if defined(ORIGIN_PC)
#include <Windows.h>
#include <AclAPI.h>
#include <shlobj.h> 
#elif defined(ORIGIN_MAC)
#include <sys/time.h>
#include <sys/types.h>
#include <utime.h>
#endif

#include "services/debug/DebugService.h"
#include "engine/cloudsaves/FilesystemSupport.h"
#include "services/platform/PlatformService.h"

namespace
{
#if defined(ORIGIN_PC)
    FILETIME datetimeToFiletime(const QDateTime &datetime)
    {
        SYSTEMTIME epochStart;
        // Start of Unix epoch
        epochStart.wYear = 1970;
        epochStart.wMonth = 1;
        epochStart.wDayOfWeek = 4; // Thursday
        epochStart.wDay = 1;
        epochStart.wHour = 0;
        epochStart.wMinute = 0;
        epochStart.wSecond = 0;
        epochStart.wMilliseconds = 0;

        FILETIME fileTime;
        // This is expecting epochStart in UTC
        SystemTimeToFileTime(&epochStart, &fileTime);

        // Convert to a uint64
        ULARGE_INTEGER integerFileTime;
        integerFileTime.LowPart = fileTime.dwLowDateTime;
        integerFileTime.HighPart = fileTime.dwHighDateTime;

        // FILETIME is in 100ns intervals - add the actual datetime on
        integerFileTime.QuadPart += datetime.toMSecsSinceEpoch() * 10000;

        // Convert back
        fileTime.dwLowDateTime = integerFileTime.LowPart;
        fileTime.dwHighDateTime = integerFileTime.HighPart;

        return fileTime;
    }
#elif defined(ORIGIN_MAC)
    timeval datetimeToTimeval(const QDateTime &datetime)
    {
        struct timeval ret;
        qint64 msecsSinceEpoch = datetime.toMSecsSinceEpoch();

        ret.tv_sec = msecsSinceEpoch / 1000;
        ret.tv_usec = (msecsSinceEpoch % 1000) * 1000;

        return ret;
    }
#endif
}

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{    
    bool FilesystemSupport::setFileMetadata(const LocalInstance &instance)
    {
#if defined(ORIGIN_PC)
        
        bool success = true;
        
        // Convert to a native path
        QString nativePath = QDir::toNativeSeparators(instance.path());
        wchar_t* lpPath = const_cast<wchar_t*>(nativePath.utf16());
        
        // Open the file
        HANDLE hFile = CreateFile(lpPath, GENERIC_WRITE | WRITE_DAC, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        
        if (hFile == INVALID_HANDLE_VALUE)
        {
            // Nothing to do
            return false;
        }
        
        // Create any empty ACL
        ACL emptyAcl;
        InitializeAcl(&emptyAcl, sizeof(emptyAcl), ACL_REVISION);
        
        DWORD result = SetSecurityInfo(hFile, 
                                       SE_FILE_OBJECT, 
                                       UNPROTECTED_DACL_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                       NULL, 
                                       NULL, 
                                       &emptyAcl, 
                                       NULL);
        
        if (result != ERROR_SUCCESS)
        {
            ORIGIN_ASSERT(0);
            success = false;
            // Keep on trying
        }
        
        // Now set our file times
        FILETIME lastModifiedTime;
        FILETIME creationTime;
        
        // Default to not setting if we don't have the timestamps
        FILETIME *pLastModifiedTime = NULL;
        FILETIME *pCreationTime = NULL;
        
        if (!instance.lastModified().isNull())
        {
            lastModifiedTime = datetimeToFiletime(instance.lastModified());
            pLastModifiedTime = &lastModifiedTime;
        }
        
        if (!instance.created().isNull())
        {
            creationTime = datetimeToFiletime(instance.created());
            pCreationTime = &creationTime;
        }
        
        if (pLastModifiedTime || pCreationTime)
        {
            SetFileTime(hFile, pCreationTime, NULL, pLastModifiedTime);
            
            if (result != ERROR_SUCCESS)
            {
                ORIGIN_ASSERT(0);
                success = false;
            }
        }
        
        CloseHandle(hFile);
        
        return success;
        
#elif defined(ORIGIN_MAC)

        bool success = true;

        // There's no way to set the creation time using the POSIX API    
        if (!instance.lastModified().isNull())
        {
            struct timeval times[2];
            
            // Make the modification and access date the same
            times[0] = times[1] = datetimeToTimeval(instance.lastModified()); 

            // Get an OS X path
            QString nativePath = QDir::toNativeSeparators(instance.path());

            if (utimes(QFile::encodeName(nativePath), times) != 0)
            {
                success = false;
            }
        }

        return success;
        
#endif
    }
        
    QDir FilesystemSupport::cloudSavesDirectory(CloudSavesDirectory dir, bool subscriptionBackup)
    {
		QString leafFolder = "\\Origin\\Cloud Saves";
		if(subscriptionBackup) 
		{
			leafFolder = "\\Origin\\Subscription Cloud Save Backups";
		}

#if defined(ORIGIN_PC)
        wchar_t folderPath[MAX_PATH + 1];

        int folderId = (dir == FilesystemSupport::LocalRoot) ? CSIDL_LOCAL_APPDATA : CSIDL_APPDATA;

        // Needs to be per-user as game saves can be stored in the user's directory
        // Shouldn't be roaming as it only applies to the local computer
        SHGetFolderPath(NULL, folderId, NULL, SHGFP_TYPE_CURRENT, folderPath);
        QDir cloudSavesRoot(QString::fromUtf16(folderPath) + leafFolder);
        if (!cloudSavesRoot.exists())
        {
            QDir().mkdir(cloudSavesRoot.absolutePath());
        }
#else
        QDir cloudSavesRoot(Origin::Services::PlatformService::getStorageLocation(QStandardPaths::DataLocation) + "/Cloud Saves");
#endif
        if (!cloudSavesRoot.exists())
        {
            QDir().mkdir(cloudSavesRoot.absolutePath());
        }

        return cloudSavesRoot;
    }
}
}
}
