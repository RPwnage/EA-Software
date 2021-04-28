//  Platform.cpp
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#include "services/platform/PlatformService.h"
#include "services/platform/EnvUtils.h"
#include "services/debug/DebugService.h"
#include "SystemInformation.h"
#include "services/log/LogService.h"
#include "version/version.h"
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QtCore>
#include <QWidget>
#include "EAIO/EAFileUtil.h"

#ifdef Q_OS_WIN
#include <Windows.h>
#include <Sddl.h>
#include <ShlObj.h>
#include <AclAPI.h>
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "version.lib")
#include <mmdeviceapi.h>
#include "Endpointvolume.h"
#include <Audiopolicy.h>
#include <Audioclient.h>
#include "atlbase.h"
#include "cstringt.h"
#include "atlstr.h"
#endif

#if defined(ORIGIN_MAC)
#include "services/settings/SettingsManager.h"
#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>
#include <Security/Security.h>
#include <Security/SecCode.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#include <sys/param.h>
#include <sys/mount.h>
#endif

#include "services/escalation/IEscalationClient.h"

namespace Origin {
namespace Services {
namespace PlatformService {
    extern bool isBeta();
    extern QString resourcesDirectoryMac();
}
}
}
using namespace Origin::Services;

namespace
{
    bool sInitialized = false;
    quint64 sMachineHash = 0;
    QString sMachineHashString;
    QString sMachineHashSHA1;
    PlatformService::OriginState sCurrentState;

    // Maps SystemResultCode enum to string representation
    QMap<PlatformService::SystemResultCode, QString> sSystemResultCodeStrings;

    /*!
        \brief Returns string representation of EA::IO::DriveType

        \sa EA::IO::DriveType
    */
    QString GetDriveTypeAsQString(const EA::IO::DriveType& driveType)
    {
        switch (driveType)
        {
            case EA::IO::kDriveTypeFixed:
                return "FIXED";
                break;

            case EA::IO::kDriveTypeCD:
                return "CD";
                break;

            case EA::IO::kDriveTypeDVD:
                return "DVD";
                break;

            case EA::IO::kDriveTypeMemoryCard:
                return "MEMORY_CARD";
                break;

            case EA::IO::kDriveTypeRAM:
                return "RAM_DISK";
                break;

            case EA::IO::kDriveTypeRemovable:
                return "REMOVABLE";
                break;
            
            case EA::IO::kDriveTypeRemote:
                return "REMOTE";
                break;
            
            default:
                break;
        }

        // This fall-through case also handles EA::IO::kDriveTypeUnknown
        return "UNKNOWN";
    }

    void initSystemResultCodeStrings()
    {
#define SYSTEM_RESULT_MAP_INSERT(systemResult) sSystemResultCodeStrings.insert(PlatformService::systemResult, #systemResult)

        SYSTEM_RESULT_MAP_INSERT(ErrorNone);
        SYSTEM_RESULT_MAP_INSERT(ErrorNotOpen);
        SYSTEM_RESULT_MAP_INSERT(ErrorNoMemory);
        SYSTEM_RESULT_MAP_INSERT(ErrorInvalidName);
        SYSTEM_RESULT_MAP_INSERT(ErrorNameTooLong);
        SYSTEM_RESULT_MAP_INSERT(ErrorFileNotFound);
        SYSTEM_RESULT_MAP_INSERT(ErrorPathNotFound);
        SYSTEM_RESULT_MAP_INSERT(ErrorAccessDenied);
        SYSTEM_RESULT_MAP_INSERT(ErrorWriteProtect);
        SYSTEM_RESULT_MAP_INSERT(ErrorSharingViolation);
        SYSTEM_RESULT_MAP_INSERT(ErrorDiskFull);
        SYSTEM_RESULT_MAP_INSERT(ErrorFileAlreadyExists);
        SYSTEM_RESULT_MAP_INSERT(ErrorDeviceNotReady);
        SYSTEM_RESULT_MAP_INSERT(ErrorDataCRCError);

#undef SYSTEM_RESULT_MAP_INSERT
    }
}

// helpers

enum CommonPath
{
    ProgramData,
    AppResources,
    LocalAppData,
};

#if defined(Q_OS_WIN)
// Auto-close Win32 Handle
class Auto_HANDLE
{
public:
    Auto_HANDLE() : _handle(0)
    {
    }
    Auto_HANDLE(HANDLE handle) : _handle(handle) 
    { 
    }
    ~Auto_HANDLE() 
    {
        if (_handle)
        {
            CloseHandle(_handle);
        }
    }

    operator HANDLE()
    {
        return _handle;
    }

    Auto_HANDLE& operator=(const HANDLE rhs)
    {
        _handle = rhs;
        return *this;
    }

    HANDLE* operator&()
    {
        return &_handle;
    }

    HANDLE release()
    {
        HANDLE tmp = _handle;
        _handle = 0;
        return tmp;
    }

private:
    HANDLE _handle;
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

    static QString directoryFor(int csidl) 
    {
        wchar_t pathBuffer[MAX_PATH + 1] = {0};
        // Needs to be per-user as game saves can be stored in the user's directory
        // Shouldn't be roaming as it only applies to the local computer
        SHGetFolderPath(NULL, csidl, NULL, SHGFP_TYPE_CURRENT, pathBuffer);
        return QDir(QString::fromWCharArray(pathBuffer)).canonicalPath();
    }	

    // This is Vista+ only
    static QString directoryForX(KNOWNFOLDERID folderId)
    {
        typedef HRESULT (WINAPI *SGKFP)(REFKNOWNFOLDERID, DWORD, HANDLE, PWSTR*);
        SGKFP pGetKnownFolderPath = (SGKFP)GetProcAddress(GetModuleHandle(L"shell32"), "SHGetKnownFolderPath");
        QString resPath;

        if (pGetKnownFolderPath)
        {
            PWSTR path;
            if (pGetKnownFolderPath(folderId, 0, NULL, &path) == S_OK)
            {
                resPath = QString::fromUtf16(path);
                CoTaskMemFree(path);
            }
        }
        return resPath;
    }

    static HKEY getHKEY(QString key)
    {
        HKEY hkey = 0;

        // check "HKEY_LOCAL_MACHINE" first since it's the most common case
        if( key == "HKEY_LOCAL_MACHINE" )
            hkey = HKEY_LOCAL_MACHINE;
        else if( key == "HKEY_CLASSES_ROOT" )
            hkey = HKEY_CLASSES_ROOT;
        else if( key == "HKEY_CURRENT_CONFIG" )
            hkey = HKEY_CURRENT_CONFIG;
        else if( key == "HKEY_CURRENT_USER" )
            hkey = HKEY_CURRENT_USER;
        else if( key == "HKEY_USERS" )
            hkey = HKEY_USERS;

        return hkey;
    }

    // GetCertificateContext
    //
    // Given an path and exe grab the certificate info
    void getCertificateContext(LPCWSTR szFileName,  PCCERT_CONTEXT &pCertContext)
    {
        HCERTSTORE hStore = NULL;
        HCRYPTMSG hMsg = NULL; 

        bool fResult;   
        DWORD dwEncoding, dwContentType, dwFormatType;
        PCMSG_SIGNER_INFO pSignerInfo = NULL;
        DWORD dwSignerInfo;    
        CERT_INFO CertInfo;

        // Get message handle and store handle from the signed file.
        fResult = CryptQueryObject(CERT_QUERY_OBJECT_FILE,
            szFileName,
            CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
            CERT_QUERY_FORMAT_FLAG_BINARY,
            0,
            &dwEncoding,
            &dwContentType,
            &dwFormatType,
            &hStore,
            &hMsg,
            NULL);
        if (!fResult)
            goto cert_cleanup;

        // Get signer information size.
        fResult = CryptMsgGetParam(hMsg, 
            CMSG_SIGNER_INFO_PARAM, 
            0, 
            NULL, 
            &dwSignerInfo);
        if (!fResult)
            goto cert_cleanup;

        // Allocate memory for signer information.
        pSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, dwSignerInfo);
        if (!pSignerInfo)
            goto cert_cleanup;

        // Get Signer Information.
        fResult = CryptMsgGetParam(hMsg, 
            CMSG_SIGNER_INFO_PARAM, 
            0, 
            (PVOID)pSignerInfo, 
            &dwSignerInfo);
        if (!fResult)
            goto cert_cleanup;

        // Search for the signer certificate in the temporary certificate store.
        CertInfo.Issuer = pSignerInfo->Issuer;
        CertInfo.SerialNumber = pSignerInfo->SerialNumber;

        pCertContext = CertFindCertificateInStore(hStore,
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            0,
            CERT_FIND_SUBJECT_CERT,
            (PVOID)&CertInfo,
            NULL);

cert_cleanup:
        //the calling function is responsible for cleaning up pCertContext

        //clean up
        if (pSignerInfo != NULL) 
            LocalFree(pSignerInfo);

        if (hStore != NULL) 
            CertCloseStore(hStore, 0);

        if (hMsg != NULL) 
            CryptMsgClose(hMsg);

    }

#elif defined(ORIGIN_MAC)

    /// \brief returns true if the sha1 values of the file contents are the same, otherwise false.
    /// Note that simple implementation loads the entire files, so try not to use it on large files!
    /// Use the debugEnabled parameter to log more information to the console.
    bool PlatformService::haveIdenticalContents(const char* fileName1, const char* fileName2, bool debugEnabled)
    {
        if (!fileName1 || !fileName2)
        {
            if (debugEnabled)
                LogConsoleMessage("Invalid parameters");
            
            return false;
        }
        
        QFile file1(fileName1);
        if (!file1.open(QIODevice::ReadOnly))
        {
            if (debugEnabled)
                LogConsoleMessage("Unable to open file '%s' (%d)", fileName1, file1.error());
            
            return false;
        }
        
        QFile file2(fileName2);
        if (!file2.open(QIODevice::ReadOnly))
        {
            if (debugEnabled)
                LogConsoleMessage("Unable to open file '%s' (%d)", fileName2, file2.error());
            
            return false;
        }
        
        QByteArray fileData = file1.readAll();
        QByteArray hashData1 = QCryptographicHash::hash(fileData, QCryptographicHash::Sha1);
        
        fileData = file2.readAll();
        QByteArray hashData2 = QCryptographicHash::hash(fileData, QCryptographicHash::Sha1);

        if (debugEnabled)
        {
            LogConsoleMessage("Hash1: %s", qPrintable(hashData1.toHex()));
            LogConsoleMessage("Hash2: %s", qPrintable(hashData2.toHex()));
        }
        
        return hashData1 == hashData2;
    }

    QString CFtoQString(CFStringRef str)
    {
        if (str == NULL)
            return QString();

        CFIndex length = CFStringGetLength(str);
        if (length == 0)
            return QString();

        QString string(length, Qt::Uninitialized);
        CFStringGetCharacters(str, CFRangeMake(0, length), reinterpret_cast<UniChar *>(const_cast<QChar *>(string.unicode())));

        return string;
    }

    /// \brief Retrieves the bundle ID from a catalog-formatted path string (e.g. [com.ea.somegame]/Contents/MacOS/SomeGame)
    ///
    /// \param key TBD.
    /// \param bundle TBD.
    /// \param suffixPath TBD.
    static bool decodeBundleCatalogFormat(const QString& key, QString& bundle, QString& suffixPath)
    {
        if (key.isEmpty())
            return false;
     
        // Start with the entire key as the suffix path.
        suffixPath = key;

        // Prepare to find an optional Bundle ID inside bracket delimiters.
        QRegExp bundleExpression( "\\[([^[]*)\\]" ); // openbracket, capture any number of non-openbrackets, and a closebracket
        
        // Find the location of the start and end Bundle ID delimiters.
        int bundleStartIndex = bundleExpression.indexIn( key );
        QStringList matches = bundleExpression.capturedTexts();
        
        // If a Bundle ID was found and captured along with trailing text,
        if ( bundleStartIndex != -1 && matches.size() == 2 )
        {
            // Bail if the Bundle ID does not start at the beginning of the string.
            if ( bundleStartIndex != 0 )
                return false;
            
            // Set the bundle part equal to the enclosed bundle ID
            QString bundlePart = bundleExpression.capturedTexts().at(1);
        
            // This should throw out Windows-style registry keys
            if (bundlePart.contains("\\"))
                return false;
        
            // Assign the bundle id.
            bundle = bundlePart;
            
            // Remove the bundle portion from the beginning of the key.
            suffixPath.remove( 0, bundleExpression.matchedLength() );
        }
        
        //ORIGIN_LOG_EVENT << "Got Bundle: " << bundle << " Got Suffix: " << suffixPath;
        
        return true;
    }

    /// \brief Translates an app bundle ID into a file system path.
    QString lookupPathForAppBundleId( const QString& bundleId );

    /// Retrieves the complete file system path from a bundle catalog formatted string (e.g. [com.ea.somegame]/Contents/MacOS/SomeGame).
    /// Because Mac app bundles can't be as easily created as Windows registry keys, if the first charactger in the bundle ID
    /// is a forward-slash, it will not perform a bundle ID lookup, but instead will treat it as an absolute path.
    static QString fileFromBundleCatalogFormat(const QString& key)
    {
        QString bundleId;
        QString suffixPath;
        
        // If the bundle catalog format was malformed,
        if (!decodeBundleCatalogFormat(key, bundleId, suffixPath))
        {
            // Return an empty path.
            return QString();
        }
        
        QString appPath;
        
        // If the first character of the bundleId is a forward slash,
        if ( bundleId.startsWith("/") )
        {
            // Take the bundle ID portion as an absolute app path.
            appPath = bundleId;
        }
        // Otherwise,
        else
        {
            // Lookup the app path.
            appPath = lookupPathForAppBundleId( bundleId );
        }
        
        //ORIGIN_LOG_EVENT << "fileFromBundleCatalogFormat(): Got the following data: bundleID=" << bundleId << "; app path=" << appPath << "; suffix=" << suffixPath;

        // Return the app path, appending the suffix.
        return QDir::cleanPath(appPath + suffixPath);
    }

    /// Retrieves the base directory from a bundle catalog formatted string (e.g. [com.ea.somegame]/Contents/MacOS/SomeGame).
    /// Because Mac app bundles can't be as easily created as Windows registry keys, if the first charactger in the bundle ID
    /// is a forward-slash, it will not perform a bundle ID lookup, but instead will treat it as an absolute path.
    static QString dirFromBundleCatalogFormat(const QString& key)
    {
        QString bundleId;
        QString suffixPath;
        
        // If the bundle catalog format was malformed,
        if (!decodeBundleCatalogFormat(key, bundleId, suffixPath))
        {
            // Return an empty path.
            return QString();
        }
        
        QString appPath;
        
        // If the first character of the bundleId is a forward slash,
        if ( bundleId.startsWith("/") )
        {
            // Take the bundle ID portion as an absolute app path.
            appPath = bundleId;
        }
        // Otherwise,
        else
        {
            // Lookup the app path.
            appPath = lookupPathForAppBundleId( bundleId );
        }

        //ORIGIN_LOG_EVENT << "dirFromBundleCatalogFormat(): Got the following data: bundleID=" << bundleId << "; app path=" << appPath << "; suffix=" << suffixPath;
        
        // Return the app path discarding the suffix.
        return QDir::cleanPath(appPath);
    }

    static bool bundleIdExists(const QString& bundleId)
    {
        QString str = lookupPathForAppBundleId( bundleId );
        if ( str.isNull() ) return false;
        else return true;
    }

    static void getOSXPowerStatus(PlatformService::BatteryFlag& batteryStatus, int& batteryPercent)
    {
        bool foundGoodPercentage = false;
        bool isOnACPower = false;
        CFTypeRef psBlob = IOPSCopyPowerSourcesInfo();
        CFArrayRef powerSources = IOPSCopyPowerSourcesList(psBlob);
        
        CFDictionaryRef pSource = NULL;
        const void* psValue;
        int sourcesCount = CFArrayGetCount(powerSources);
        if (sourcesCount == 0)
        {
            CFRelease(psBlob);
            CFRelease(powerSources);
            
            batteryPercent = 0;//none found
            batteryStatus = PlatformService::NoBattery;
            return;
        }
        
        char strbuff[100];
        CFStringRef psName = (CFStringRef)IOPSGetProvidingPowerSourceType(psBlob);
        CFStringGetCString(psName, strbuff, 100, kCFStringEncodingUTF8);
        if (strncmp(strbuff,"AC Power", 4) == 0)
        {
            isOnACPower = true;
        }

        
        for (int i=0; i<sourcesCount; i++)
        {
            pSource = IOPSGetPowerSourceDescription(psBlob, CFArrayGetValueAtIndex(powerSources, i));
            psValue = (CFStringRef) CFDictionaryGetValue(pSource, CFSTR(kIOPSNameKey));
            psValue = CFRetain(psValue);
            CFRelease(psValue);
            
            int curCapacity = 0;
            int maxCapacity = 0;
            int percent = 0;
            
            psValue = CFDictionaryGetValue(pSource, CFSTR(kIOPSCurrentCapacityKey));
            psValue = CFRetain(psValue);
            CFNumberGetValue((CFNumberRef)psValue, kCFNumberSInt32Type, &curCapacity);
            CFRelease(psValue);
            
            psValue = CFDictionaryGetValue(pSource, CFSTR(kIOPSMaxCapacityKey));
            psValue = CFRetain(psValue);
            CFNumberGetValue((CFNumberRef)psValue, kCFNumberSInt32Type, &maxCapacity);
            CFRelease(psValue);
            percent = (int)((double)curCapacity / (double)maxCapacity * 100);
            
            if (percent > 0 && percent <= 100)
            {
                batteryPercent = percent;
                foundGoodPercentage = true;
                break;
            }
        }
        
        if(foundGoodPercentage)
        {
            char strbuff[100];
            CFStringRef psName = (CFStringRef)IOPSGetProvidingPowerSourceType(psBlob);
            CFStringGetCString(psName, strbuff, 100, kCFStringEncodingUTF8);
            
            batteryStatus = isOnACPower == true ? PlatformService::PluggedIn : PlatformService::UsingBattery;
            
            CFRelease(psBlob);
            CFRelease(powerSources);
            
            return;
        }
        else
        {
            CFRelease(psBlob);
            CFRelease(powerSources);
            
            // if we are he there is probably a problem with the users battery,
            // but they are obviously running the MacBook, so the must be plugged in???
            batteryStatus = PlatformService::PluggedIn;
        }
    }

#endif			
#ifdef Q_OS_WIN
static void getPCPowerStats(PlatformService::BatteryFlag& batteryStatus, int& batteryPercent)
    {
        batteryStatus = PlatformService::NoBattery;
        batteryPercent = 0;

        // SYSTEM_POWER_STATUS documentation:
        // http://msdn.microsoft.com/en-us/library/windows/desktop/aa373232(v=vs.85).aspx
        SYSTEM_POWER_STATUS status;
        if(GetSystemPowerStatus(&status))
        {
            static const int DefaultBatteryValues[5] = 
                {   50, // Between 33% and 66%
                    80, // Over 66%
                    20, // Under 33%
                    0, // Not used
                    2  // Under 5%
                };

            const int flag = status.BatteryFlag;
            switch(flag)
            {
                case 0: // Battery is not being charged and the battery capacity is between low and high.
                case 1: // High > 66%
                case 2: // Low < 33%
                case 4: // Critical < 5%
                    // AC LINE STATUS: 0 == offline, 1 == online, 255 == unknown
                    // If the AC is offline then we are using the laptops battery
                    batteryStatus = (status.ACLineStatus == 0) ? PlatformService::UsingBattery : PlatformService::PluggedIn;
                    batteryPercent = status.BatteryLifePercent;
                    if (batteryPercent > 100) // ie 255 = unknown status
                    {
                        static bool usingBatteryLogError = true;
                        if (usingBatteryLogError)
                        {
                            usingBatteryLogError = false;
                            if (batteryStatus == PlatformService::UsingBattery)
                                ORIGIN_LOG_EVENT << "PC Power reported invalid battery percentage (" << batteryPercent << ") while status = " << flag << "(using battery)";
                            else
                                ORIGIN_LOG_EVENT << "PC Power reported invalid battery percentage (" << batteryPercent << ") while status = " << flag << "(computer plugged in)";
                        }

                        // Let's use our "representative values
                        batteryPercent = DefaultBatteryValues[flag];
                    }
                    break;
                case 8: // Battery charging
                case 9: // Why does it go to 9? Weird.
                case 10: // Why does it go to 10? Weird.
                    batteryStatus = PlatformService::PluggedIn;
                    batteryPercent = status.BatteryLifePercent;
                    if (batteryPercent > 100) // ie 255 = unknown status
                    {
                        static bool batteryChargingLogError = true;
                        if (batteryChargingLogError)
                        {
                            batteryChargingLogError = false;
                            ORIGIN_LOG_EVENT << "PC Power reported invalid battery percentage (" << batteryPercent << ") while status = " << flag << "(battery charging)";
                        }

                        batteryPercent = 0;
                    }
                    break;
                case 128: // No battery
                case 255: // Unknown status
                default:
                    break;
            }
        }
    }

    bool PlatformService::isBeta()
    {
        HKEY key = NULL;
        QString isBeta;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Origin", 0, KEY_READ, &key) == ERROR_SUCCESS)
        {
            char output[16];
            DWORD size = sizeof(output)/sizeof(output[0]);
            RegQueryValueExA(key, "IsBeta", NULL, NULL, (BYTE*)output, &size);
            isBeta = output;
        }

        RegCloseKey(key);

        if (isBeta.compare("true") == 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
#endif

    QString commonPath(CommonPath commonFolderPath)
    {
        QString pathRes;
        switch (commonFolderPath)
        {
        default:
        case ProgramData:
            {

#if defined(Q_OS_WIN)
                pathRes = directoryForX(FOLDERID_ProgramData);
                if (pathRes.isEmpty())
                {
                    pathRes = directoryFor(CSIDL_COMMON_APPDATA);
                }
                pathRes += QDir::separator() + qApp->applicationName() + QDir::separator();
#elif defined(ORIGIN_MAC)
                pathRes = PlatformService::getStorageLocation(QStandardPaths::DataLocation) + QDir::separator();
#else
#error "Need platform-specific info"
#endif			
            }
            break;

        case AppResources:
#if defined(Q_OS_WIN)
            pathRes = QCoreApplication::applicationDirPath();
#elif defined(ORIGIN_MAC)
            pathRes = PlatformService::resourcesDirectoryMac();
#else
#error "Need platform-specific info"
#endif			
            break;

        case LocalAppData:
#if defined(Q_OS_WIN)
            pathRes = directoryForX(FOLDERID_LocalAppData);
            if (pathRes.isEmpty())
            {
                pathRes = directoryFor(CSIDL_LOCAL_APPDATA);
            }
            pathRes += QString("%1%2%3").arg(QDir::separator(), qApp->applicationName(), QDir::separator());
#elif defined(ORIGIN_MAC)
            pathRes = PlatformService::getStorageLocation(QStandardPaths::DataLocation) + QDir::separator();
#else
#error "Need platform-specific info"
#endif
            break;
        }
        QDir qdir(pathRes);
        if (!qdir.exists())
        {
            ORIGIN_ASSERT(qdir.mkpath(pathRes));
        }
        return pathRes;
    }

void PlatformService::init()
{
    ORIGIN_ASSERT(!sInitialized);
    if (!sInitialized)
    {
#if defined(ORIGIN_MAC)
        QDir().mkpath(tempDataPath());
#endif 
        sMachineHash = NonQTOriginServices::SystemInformation::instance().GetMachineHash();
        sMachineHashString.setNum(sMachineHash);
        sMachineHashSHA1 = QCryptographicHash::hash(sMachineHashString.toLatin1(), QCryptographicHash::Sha1).toHex().toUpper();
        sCurrentState = PlatformService::Unknown; // OriginState
        initSystemResultCodeStrings();

        sInitialized = true;
    }
}

void PlatformService::release()
{
    ORIGIN_ASSERT(sInitialized);
    if (sInitialized)
    {
        sInitialized = false;
    }
}

PlatformService::OriginPlatform PlatformService::runningPlatform()
{
#if defined(Q_OS_WIN)
    return PlatformWindows;
#elif defined(ORIGIN_MAC)
    return PlatformMacOS;
#else
    return PlatformUnknown;
#endif
}

PlatformService::OriginPlatform PlatformService::resolvePlatform(OriginPlatform platform)
{
    switch (platform)
    {
    case PlatformService::PlatformThis:
        return runningPlatform();

    case PlatformService::PlatformOther:
        switch (runningPlatform())
        {
        case PlatformService::PlatformWindows:
            return PlatformService::PlatformMacOS;

        case PlatformService::PlatformMacOS:
            return PlatformService::PlatformWindows;

        default:
            return PlatformService::PlatformUnknown;
        }

    default:
        return platform;
    }
}

QMap<PlatformService::OriginPlatform, QString> PlatformService::knownPlatformSuffixes()
{
    QMap<PlatformService::OriginPlatform, QString> knownPlatforms;

    knownPlatforms[PlatformWindows] = "";  // No suffix for Windows
    knownPlatforms[PlatformMacOS] = "_Mac";  // _Mac for MacOS

#if !defined(Q_OS_WIN) && !defined(ORIGIN_MAC)
#warning "Update this list for new platforms"
#endif

    return knownPlatforms;
}

QVector<PlatformService::OriginPlatform> PlatformService::supportedPlatformsFromCatalogString(const QString& catalogPlatformString)
{
    QVector<PlatformService::OriginPlatform> supportedPlatforms;

    QStringList platforms = catalogPlatformString.toLower().split("/", QString::SkipEmptyParts);

    foreach(QString platformId, platforms)
    {
        // Mac supported
        if (platformId == "mac")
        {
            supportedPlatforms.push_back(Services::PlatformService::PlatformMacOS);
        }
        // PC supported
        else if (platformId == "pc" || platformId == "pcwin")
        {
            supportedPlatforms.push_back(Services::PlatformService::PlatformWindows);
        }
        else
        {
            // Unsupported platform?
            ORIGIN_LOG_EVENT << "Received unsupported platform ID in consolidated entitlements: " << platformId;
            supportedPlatforms.push_back(Services::PlatformService::PlatformUnknown);
        }
    }

    return supportedPlatforms;
}

QString PlatformService::catalogStringFromSupportedPlatfom(const PlatformService::OriginPlatform platform) {
    switch (platform)
    {
        case Origin::Services::PlatformService::PlatformWindows:
            return "PCWIN";
            
        case Origin::Services::PlatformService::PlatformMacOS:
            return "MAC";
            
        default:
            ORIGIN_LOG_WARNING << "Unknown platform detected...";
            return "UNKNOWN";
    }
}

QString PlatformService::stringForPlatformId(PlatformService::OriginPlatform platformId)
{
    switch (platformId)
    {
    case PlatformService::PlatformWindows:
        return "pc";
    case PlatformService::PlatformMacOS:
        return "mac";
    default:
        return "unknown";
    }
}

PlatformService::OriginPlatform PlatformService::platformIdForString(const QString &platform)
{
    const QString &lowerPlatform = platform.toLower();

    if ("pc" == lowerPlatform)
    {
        return PlatformService::PlatformWindows;
    }
    else if ("mac" == lowerPlatform)
    {
        return PlatformService::PlatformMacOS;
    }
    else
    {
        return PlatformService::PlatformUnknown;
    }
}

/// \brief Get the string name of the Origin Processor Architecture
///
/// This function is offered for convenience reasons
QString PlatformService::stringForProcessorArchitecture(PlatformService::OriginProcessorArchitecture architectureId)
{
    if (architectureId == PlatformService::ProcessorArchitecture3264bit)
    {
        return QString("32/64");
    }
    else if (architectureId == PlatformService::ProcessorArchitecture32bit)
    {
        return QString("32-bit");
    }
    else if (architectureId == PlatformService::ProcessorArchitecture64bit)
    {
        return QString("64-bit");
    }
    return QString("unknown");
}

/// \brief Get a bitfield enumeration describing the processor architectures supported by this machine.
///
/// Returns a bitfield enumeration of the processor architectures supported by this machine.
PlatformService::OriginProcessorArchitecture PlatformService::supportedArchitecturesOnThisMachine()
{
    PlatformService::OriginProcessorArchitecture thisMachineSupported = PlatformService::ProcessorArchitectureUnknown;

    // If we support 64-bit, we by definition support 32-bit also (for now, this may change someday)
    if (EnvUtils::GetIS64BitOS())
    {
        thisMachineSupported = PlatformService::ProcessorArchitecture3264bit;
    }
    else
    {
        // We are 32-bit only
        thisMachineSupported = PlatformService::ProcessorArchitecture32bit;
    }

    return thisMachineSupported;
}

/// \brief Get a boolean flag indicating whether this machine supports at least one of the architectures in the query.
///
/// Mainly used to determine if this machine supports running a particular game, which supports some/all of our architectures.
bool PlatformService::queryThisMachineCanExecute(PlatformService::OriginProcessorArchitecture availableArchitectures)
{
    // Do a bitwise AND to see if we support at least one of the architectures being queried for
    if ((supportedArchitecturesOnThisMachine() & availableArchitectures) != PlatformService::ProcessorArchitectureUnknown)
        return true;
    return false;
}

/// \brief Get a bitfield enumeration describing the processor architectures available in a piece of content by the catalog-formatted string.  (For a game, usually)
///
/// This function is used mainly by the entitlement parsing APIs.  Parses strings like '32-bit', '32/64', '64-bit', etc.
PlatformService::OriginProcessorArchitecture PlatformService::availableArchitecturesFromCatalogString(const QString& catalogProcArchString)
{
    if (catalogProcArchString == "32/64")
    {
        return PlatformService::ProcessorArchitecture3264bit;
    }
    else if (catalogProcArchString == "32-bit")
    {
        return PlatformService::ProcessorArchitecture32bit;
    }
    else if (catalogProcArchString == "64-bit")
    {
        return PlatformService::ProcessorArchitecture64bit;
    }
    return PlatformService::ProcessorArchitectureUnknown;
}

Origin::VersionInfo PlatformService::currentOSVersion()
{
    return Origin::VersionInfo(EnvUtils::GetOSVersion());
}

Origin::VersionInfo PlatformService::currentClientVersion()
{
    return Origin::VersionInfo(EALS_VERSION_P_DELIMITED);
}

bool PlatformService::querySupportedByCurrentOS(Origin::VersionInfo minimumOSVersion, Origin::VersionInfo maximumOSVersion)
{
    Origin::VersionInfo thisOS = currentOSVersion();

    // Do we meet the minimum spec?
    if (thisOS < minimumOSVersion)
        return false;

    // Do we exceed the maximum spec (optional)?
    if (!(maximumOSVersion == Origin::VersionInfo(0,0,0,0)) && maximumOSVersion < thisOS)
        return false;

    return true;
}

QString PlatformService::displayNameFromOSVersion(Origin::VersionInfo osVersion)
{
    QString osName;
    
#if defined(ORIGIN_PC)
    switch(osVersion.GetMajor() * 10 + osVersion.GetMinor())
    {
    case 51:
        osName = QObject::tr("ebisu_client_windows_xp");
        break;
    case 60:
        osName = QObject::tr("ebisu_client_windows_vista");
        break;
    case 61:
        osName = QObject::tr("ebisu_client_windows_7");
        break;
    case 62:
        osName = QObject::tr("ebisu_client_windows_8");
        break;
    default:
        osName = QObject::tr("--");
        break;
    }
#elif defined(ORIGIN_MAC)
    osName = QObject::tr("ebisu_client_osx");
#else
#error "require platform-specific mapping"
#endif
    return osName;
}

unsigned int PlatformService::OSMajorVersion()
{
#if defined(Q_OS_WIN)
    OSVERSIONINFO versionInfo;
    memset(&versionInfo, 0, sizeof(OSVERSIONINFO));
    versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    bool isOk = GetVersionEx(&versionInfo);
    ORIGIN_ASSERT(isOk);
    Q_UNUSED(isOk);
    return versionInfo.dwMajorVersion;
#elif defined(ORIGIN_MAC)
    SInt32 majorVersion;
    if (Gestalt(gestaltSystemVersionMajor, &majorVersion))
    {
        return static_cast<unsigned int>(majorVersion);
    }
    return 10;
#else
#error "Need platform-specific info"
    return 0;
#endif			
}

unsigned int PlatformService::OSMinorVersion()
{
#if defined(Q_OS_WIN)
    OSVERSIONINFO versionInfo;
    memset(&versionInfo, 0, sizeof(OSVERSIONINFO));
    versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    bool isOk = GetVersionEx(&versionInfo);
    ORIGIN_ASSERT(isOk);
    Q_UNUSED(isOk);
    return versionInfo.dwMinorVersion;
#elif defined(ORIGIN_MAC)
    SInt32 minorVersion;
    if (Gestalt(gestaltSystemVersionMinor, &minorVersion))
    {
        return static_cast<unsigned int>(minorVersion);
    }
    return 0;
#else
#error "Need platform-specific info"
    return 0;
#endif			
}

QString PlatformService::commonAppDataPath()
{
    return commonPath(ProgramData);
}

QString PlatformService::cloudSaveBackupPath()
{
    const QString BACKUP_DIR_NAME("Subscription Cloud Save Backups");
    return QString("%1%2").arg(commonPath(LocalAppData), BACKUP_DIR_NAME);
}

quint64 PlatformService::machineHash()
{
    return sMachineHash;
}

QString const& PlatformService::machineHashAsString()
{
    return sMachineHashString;
}

QString const& PlatformService::machineHashAsSHA1()
{
    return sMachineHashSHA1;
}

PlatformService::OriginState PlatformService::originState()
{
    return sCurrentState;
}

void PlatformService::setOriginState(PlatformService::OriginState state)
{
    sCurrentState = state;
}

PlatformService::SystemResultCode PlatformService::lastSystemResultCode()
{
#ifdef Q_OS_WIN
    return static_cast<SystemResultCode>(GetLastError());
#endif
#ifdef ORIGIN_MAC
    return static_cast<SystemResultCode>(errno);
#endif
}

#ifdef Q_OS_WIN

// Key format:      KEY\SUBKEY1\...\SUBKEYn\VALUENAME
bool PlatformService::registryQueryValue(QString path, QString& result, bool b64BitPath, bool bFailIfNotFound)
{
    QString key = "";
    QString subKey = "";
    QString valueName = "";

    // break up the path into 'key' and 'subkey'
    int firstSlash = path.indexOf('\\');
    int lastSlash = path.lastIndexOf('\\');

    if (firstSlash < 0 || lastSlash < 0)
    {
        ORIGIN_ASSERT(false);
        return false;
    }

    key = path.left(firstSlash);                    // everything before the first slash
    valueName = path.mid(lastSlash + 1);            // everything after last slash
    subKey = path.mid(firstSlash + 1, lastSlash - firstSlash-1);   // everything between first and last slashes

    HKEY preDefKey = getHKEY(key);
    HKEY handleKey = 0;

    REGSAM samDesired = KEY_QUERY_VALUE;

    // If requesting to access a 64 bit path explicitly
    if (b64BitPath)
        samDesired |=  KEY_WOW64_64KEY;

    if(RegOpenKeyExW(preDefKey, (LPCWSTR)subKey.utf16(), 0L, samDesired, &handleKey) == ERROR_SUCCESS)
    {

        // Okay we have the mRegHandle 
        // Next, get the string result out of the key
        DWORD registryType = REG_NONE;
        DWORD dwSize = 0;

        // Get size
        bool retVal = true;
        if (RegQueryValueExW(handleKey, (LPCWSTR)valueName.utf16(), NULL, &registryType, NULL, &dwSize) == ERROR_SUCCESS)
        {
            // Check that something is there
            if (dwSize > sizeof(wchar_t)) 
            {
                // Type needs to be TCHAR because this code is shared.
                // Registry paths can be wide chars.
                wchar_t* pBuf = (wchar_t*)_malloca(dwSize*sizeof(wchar_t));

                // Retrieve the result from the registry            
                if (RegQueryValueExW( handleKey, (LPCWSTR)valueName.utf16(), NULL, &registryType, (LPBYTE) pBuf , &dwSize) == ERROR_SUCCESS)
                {
                    result = QString::fromWCharArray((wchar_t*) pBuf);

                    _freea(pBuf);
                }
            }
        }         

        else
        {
            // Do we need to know whether the entry exists (<> empty)?
            if (bFailIfNotFound)
                retVal = false;
        }

        RegCloseKey(handleKey);
        return retVal;
    }
    return false;
}

QString PlatformService::pathFromRegistry(QString key, bool b64BitPath)
{
    QStringList parts = key.split(QRegExp("[\\[\\]]"), QString::SkipEmptyParts);

    if(parts.size() == 2)
    {
        QString path = "";
        registryQueryValue(parts[0], path, b64BitPath);

        if(path.isEmpty())
            return "";

        //sometimes the path is stored in the registry with quotes around it, so we want to remove that, otherwise
        //it won't be able to find the file
        if (path.endsWith ("\""))
        {
            path.chop (1);  //removes the ending quote
            if (path.startsWith ("\""))
            {
                path = path.right (path.size() - 1);
            }
        }

        if(!path.endsWith("\\"))
            path += "\\";

        path += parts.at(1);

        //remove the double slashes and end white spaces if any
        //windows network paths start with \\ so leave them alone
        bool isNetworkPath = path.startsWith("\\\\");
        path.replace("\\\\", "\\");
        if (isNetworkPath)
        {
            path.prepend("\\");
        }
        path = path.trimmed();

        return path;
    }

    // If there is no registry key passed in it could be just a fully qualified path
    return key;
}

QString PlatformService::dirFromRegistry(QString key, bool b64BitPath)
{
    QStringList parts = key.split(QRegExp("[\\[\\]]"), QString::SkipEmptyParts);

    if (parts.size() > 0)
    {
        QString path;
        if (registryQueryValue(parts[0], path, b64BitPath))
            return path;
    }

    return "";
}

static const DWORD FLASH_TIMEOUT = 730; // milliseconds per flash (same as value internally used in QApplication::alert() function)
void PlatformService::flashTaskbarIcon(QWidget* widget, int count /*= 0*/)
{
    if(widget)
    {
        flashTaskbarIcon(widget->winId());
    }
}

void PlatformService::flashTaskbarIcon(quintptr winId, int count /*= 0*/)
{
    if (winId)
    {
        BOOL success = false;
        if (count)
        {
            FLASHWINFO fwi;
            ZeroMemory(&fwi, sizeof(FLASHWINFO));
            fwi.cbSize = sizeof(FLASHWINFO);
            fwi.hwnd = (HWND)winId;
            fwi.dwTimeout = FLASH_TIMEOUT;
            fwi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
            fwi.uCount = (UINT)count;
            success = FlashWindowEx(&fwi);
        }
        else
        {
            success = FlashWindow((HWND)winId, true);
        }
        if (success)
        {
            flashTaskbarIcon(winId, count);
        }
    }
}
#endif // Q_OS_WIN

bool PlatformService::OSPathExists(QString path)
{
#ifdef Q_OS_WIN
    QString value;
    return registryQueryValue(path, value);
#endif
#ifdef ORIGIN_MAC
    return bundleIdExists(path);
#endif
}

QString PlatformService::localFilePathFromOSPath(QString key)
{
    if(key.startsWith("%PluginsDir%", Qt::CaseInsensitive))
    {
        QString path = key.replace("%PluginsDir%", pluginsDirectory(), Qt::CaseInsensitive);
        return QFileInfo(path).absoluteFilePath();
    }
    else
    {
#ifdef Q_OS_WIN
        return pathFromRegistry(key);
#endif
#ifdef ORIGIN_MAC
        return fileFromBundleCatalogFormat(key);
#endif
    }
}

QString PlatformService::localDirectoryFromOSPath(QString key, bool b64BitPath)
{
    if(key.startsWith("%PluginsDir%", Qt::CaseInsensitive))
    {
        QString path = key.replace("%PluginsDir%", pluginsDirectory(), Qt::CaseInsensitive);
        return QFileInfo(path).absolutePath();
    }
    else
    {
#ifdef Q_OS_WIN
        return dirFromRegistry(key, b64BitPath).replace('"',"");
#endif
#ifdef ORIGIN_MAC
        return dirFromBundleCatalogFormat(key);
#endif
    }
}

void PlatformService::powerStatus(BatteryFlag& batteryStatus, int& batteryPercent)
{
#ifdef Q_OS_WIN
    getPCPowerStats(batteryStatus, batteryPercent);
#endif
#ifdef ORIGIN_MAC
    getOSXPowerStatus(batteryStatus, batteryPercent);
#endif
}

// Mac version defined in PlatformService_Mac.mm
QString PlatformService::resourcesDirectory()
{
    return commonPath(AppResources);
}

QString PlatformService::pluginsDirectory()
{
    return QCoreApplication::applicationDirPath() + "/plugins";
}

QString PlatformService::odtDirectory()
{
    QString odtFolder = commonAppDataPath();
    odtFolder += "/..";
#ifdef ORIGIN_MAC
    odtFolder += "/ea.com";
#endif
    odtFolder += "/";
    odtFolder += ODT_FOLDER_NAME;
    return QDir::cleanPath(odtFolder);
}

const QString PlatformService::ODT_FOLDER_NAME("Origin Developer Tool");

bool PlatformService::createFolderElevated(const QString &path, const QString &acl,
    bool *uacExpired, int *elevationResult, int *elevationError)
{
    if (elevationResult)
    {
        *elevationResult = Origin::Escalation::kCommandErrorNone;
    }

    if (elevationError)
    {
        *elevationError = 0;
    }

    ORIGIN_ASSERT(!path.isEmpty());
    if (path.isEmpty())
    {
        ORIGIN_LOG_WARNING << "createFolderElevated called with empty directory.";
        return false;
    }

    QString escalationReasonStr = QString("createFolderElevated (%1)").arg(path);
    QSharedPointer<Escalation::IEscalationClient> escalationClient;
    int escalationError = Escalation::kCommandErrorNone;

    if (!Escalation::IEscalationClient::quickEscalate(escalationReasonStr, escalationError, escalationClient))
    {
        if (elevationResult)
        {
            *elevationResult = escalationError;
        }
        return false;
    }

    escalationError = escalationClient->createDirectory(path, acl);

    if (uacExpired)
    {
        *uacExpired = escalationClient->hasUACExpired();
    }

    if (elevationResult)
    {
        *elevationResult = escalationError;
    }

    if (elevationError)
    {
        *elevationError = escalationClient->getSystemError();
    }

    return Escalation::IEscalationClient::evaluateEscalationResult(escalationReasonStr,
        "createDirectory", escalationError, escalationClient->getSystemError());
}

QString PlatformService::eacoreIniDirectory()
{
    QString retval = "";
#if defined(Q_OS_WIN)
    retval = QCoreApplication::applicationDirPath();
#elif defined(ORIGIN_MAC)
    retval = PlatformService::commonAppDataPath();
#else
#error "Needs platform-specific implementation."
#endif

    // ensure there's always a trailing separator
    QString trailChar = retval.right(1);
    if (!(trailChar == "/" || trailChar == "\\"))
        retval += QDir::separator();

    return retval;
}

QString PlatformService::eacoreIniFilename()
{
    return eacoreIniDirectory() + "EACore.ini";
}

QString PlatformService::logDirectory()
{
    return commonAppDataPath() + "Logs" + QDir::separator();
}

QString PlatformService::clientLogFilename()
{
    return logDirectory() + "Client_Log.txt";
}

QString PlatformService::bootstrapperLogFilename()
{
    return logDirectory() + "Bootstrapper_Log.txt";
}

QString PlatformService::machineStoragePath()
{
#if defined(Q_OS_WIN)
    QString storagePath;
    WCHAR buffer[MAX_PATH];
    ZeroMemory(buffer, sizeof(buffer));
    
    SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, buffer );
    storagePath = QString::fromWCharArray(buffer);
    storagePath += "\\Origin\\";
    
    return storagePath;
#elif defined(ORIGIN_MAC)
    return "/Library/Application Support/Origin/";
#else
#error "Needs platform-specific implementation."
#endif
}

bool PlatformService::compareFileSigning(QString file1, QString file2)
{
    bool same = false;
    
#if defined(Q_OS_WIN)
    PCCERT_CONTEXT file1Cert=NULL, file2Cert=NULL;
    
    memset(&file2Cert, 0, sizeof(file2Cert));
    memset(&file1Cert, 0, sizeof(file1Cert));
    
    //get the certificate context for the Origin.exe and OriginClientService.exe
    //this will allocate memory for the returned context so we must free it below
    getCertificateContext((LPCWSTR)file1.toUpper().utf16(), file1Cert);
    getCertificateContext((LPCWSTR)file2.utf16(), file2Cert);
    
    if(file1Cert && file2Cert)
    {
        //see if they are the same certificate
        same = CertCompareCertificate(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, file1Cert->pCertInfo, file2Cert->pCertInfo);
    }
    
    //free the context
    if(file1Cert)
        CertFreeCertificateContext(file1Cert);
    if(file2Cert)
        CertFreeCertificateContext(file2Cert);
#elif defined(ORIGIN_MAC)
    CFStringRef cfsFile1 = NULL;
    CFStringRef cfsFile2 = NULL;
    CFURLRef cfuFile1 = NULL;
    CFURLRef cfuFile2 = NULL;
    SecStaticCodeRef sscFile1 = NULL;
    SecStaticCodeRef sscFile2 = NULL;
    CFDictionaryRef cfdFile1 = NULL;
    CFDictionaryRef cfdFile2 = NULL;
    
    // Convert our file paths to SecStaticCodeRefs
    cfsFile1 = CFStringCreateWithBytes (kCFAllocatorDefault, (const UInt8*)file1.toUtf8().constData(), file1.toUtf8().size(), kCFStringEncodingUTF8, false);
    cfuFile1 = CFURLCreateWithFileSystemPath (kCFAllocatorDefault, cfsFile1, kCFURLPOSIXPathStyle, false);
    SecStaticCodeCreateWithPath(cfuFile1, kSecCSDefaultFlags, &sscFile1);
    
    cfsFile2 = CFStringCreateWithBytes (kCFAllocatorDefault, (const UInt8*)file2.toUtf8().constData(), file2.toUtf8().size(), kCFStringEncodingUTF8, false);
    cfuFile2 = CFURLCreateWithFileSystemPath (kCFAllocatorDefault, cfsFile2, kCFURLPOSIXPathStyle, false);
    SecStaticCodeCreateWithPath(cfuFile2, kSecCSDefaultFlags, &sscFile2);
    
    if(sscFile1 && sscFile2)
    {
        // Gather dictionary of signing information
        SecCodeCopySigningInformation(sscFile1, kSecCSSigningInformation, &cfdFile1);
        SecCodeCopySigningInformation(sscFile2, kSecCSSigningInformation, &cfdFile2);
        
        if(cfdFile1 && cfdFile2)
        {
            // Pull certs out of signing info dict and compare.
            CFArrayRef file1Certs = (CFArrayRef)CFDictionaryGetValue(cfdFile1, kSecCodeInfoCertificates);
            CFArrayRef file2Certs = (CFArrayRef)CFDictionaryGetValue(cfdFile2, kSecCodeInfoCertificates);
            
            same = CFEqual(file1Certs, file2Certs);
        }
    }
    
    CFRelease (cfsFile1);
    CFRelease (cfsFile2);
    CFRelease (cfuFile1);
    CFRelease (cfuFile2);
    
    if(sscFile1)
        CFRelease (sscFile1);
    if(sscFile2)
        CFRelease (sscFile2);
    
    if(cfdFile1)
        CFRelease (cfdFile1);
    if(cfdFile2)
        CFRelease (cfdFile2);
#else
#error "Needs platform-specific implementation."
#endif
    
    return same;
}

bool PlatformService::getPluginVersionInfo(const QString& pluginPath, Origin::VersionInfo& pluginVersion, Origin::VersionInfo& compatibleClientVersion)
{
    bool versionsFound = false;

    pluginVersion = VersionInfo();
    compatibleClientVersion = VersionInfo();

#if defined(Q_OS_WIN)
    LPCWSTR path = pluginPath.utf16();
    DWORD verHandle = NULL;
    UINT size = 0;
    LPBYTE buffer = NULL;
    DWORD  verSize = GetFileVersionInfoSize(path, &verHandle);

    if(verSize)
    {
        LPSTR verData = new char[verSize];
        if (GetFileVersionInfo(path, verHandle, verSize, verData))
        {
            if (VerQueryValue(verData, L"\\", (VOID FAR* FAR*)&buffer, &size))
            {
                if (size)
                {
                    VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)buffer;
                    if (verInfo->dwSignature == 0xfeef04bd)
                    {
                        int major = HIWORD(verInfo->dwFileVersionMS);
                        int minor = LOWORD(verInfo->dwFileVersionMS);
                        int build = HIWORD(verInfo->dwFileVersionLS);
                        int rev = LOWORD(verInfo->dwFileVersionLS);
                        pluginVersion = VersionInfo(major, minor, build, rev);
                        
                        major = HIWORD(verInfo->dwProductVersionMS);
                        minor = LOWORD(verInfo->dwProductVersionMS);
                        build = HIWORD(verInfo->dwProductVersionLS);
                        rev = LOWORD(verInfo->dwProductVersionLS);
                        compatibleClientVersion = VersionInfo(major, minor, build, rev);

                        versionsFound = true;
                    }
                }
            }
        }
        delete[] verData;
    }
#elif defined(ORIGIN_MAC)
    QString bundlePath = getContainerBundle(pluginPath, ".plugin");
    
    CFStringRef cfsBundlePath = NULL;
    CFURLRef cfuBundlePath = NULL;
    CFBundleRef bundle = NULL;
    CFStringRef cfsClientVersionKey = NULL;
    
    cfsBundlePath = CFStringCreateWithBytes (kCFAllocatorDefault, (const UInt8*)bundlePath.toUtf8().constData(), bundlePath.toUtf8().size(), kCFStringEncodingUTF8, false);
    cfuBundlePath = CFURLCreateWithFileSystemPath (kCFAllocatorDefault, cfsBundlePath, kCFURLPOSIXPathStyle, false);
    
    if(cfuBundlePath != NULL)
    {
        bundle = CFBundleCreate(NULL, cfuBundlePath);
        
        // Get plugin version
        CFStringRef cfsPluginVersion = (CFStringRef)CFBundleGetValueForInfoDictionaryKey(bundle, kCFBundleVersionKey);        
        if(cfsPluginVersion != NULL)
        {
            pluginVersion = VersionInfo(CFtoQString(cfsPluginVersion));
        }
        
        // Get compatible client version using custom OriginClientVersion key
        cfsClientVersionKey = CFStringCreateWithCString(kCFAllocatorDefault, "OriginClientVersion", kCFStringEncodingUTF8);
        CFStringRef cfsCompatibleClientVersion = (CFStringRef)CFBundleGetValueForInfoDictionaryKey(bundle, cfsClientVersionKey);
        if(cfsCompatibleClientVersion != NULL)
        {
            compatibleClientVersion = VersionInfo(CFtoQString(cfsCompatibleClientVersion));
        }

    }
    
    if(cfsBundlePath)
        CFRelease (cfsBundlePath);
    if(cfuBundlePath)
        CFRelease (cfuBundlePath);
    if(bundle)
        CFRelease (bundle);
    if(cfsClientVersionKey)
        CFRelease (cfsClientVersionKey);
#else
#error "Needs platform-specific implementation."
#endif

    return versionsFound;
}

#ifdef ORIGIN_MAC
bool PlatformService::extractExecutableFromBundle(const QString& appBundlePath, QString& actualExePath)
{
    QByteArray utf8BundlePath = appBundlePath.toUtf8();
    
    CFURLRef bundleUrl = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8 *)utf8BundlePath.data(), utf8BundlePath.length(), true);
    
    actualExePath = "";
    
    if(bundleUrl != NULL)
    {
        CFBundleRef bundle = CFBundleCreate(NULL, bundleUrl);
        CFRelease(bundleUrl);
        if(bundle != NULL)
        {
            CFStringRef cfExecutable =(CFStringRef)CFBundleGetValueForInfoDictionaryKey(bundle, kCFBundleExecutableKey);
            QString extractedExePath;
            
            if(cfExecutable != NULL)
            {
                extractedExePath = CFStringGetCStringPtr(cfExecutable, kCFStringEncodingMacRoman);
            }
            
            CFRelease(bundle);
            
            if(!extractedExePath.isEmpty())
            {
                actualExePath = appBundlePath + "/Contents/MacOS/" + extractedExePath;
            }
            
            return (!actualExePath.isEmpty());
        }
    }
    
    return false;
}

QString PlatformService::tempDataPath()
{
    return QString("/var/tmp/Origin/");
}

QString PlatformService::translationsPath()
{
    return QCoreApplication::applicationDirPath() + "/../Resources/lang";
}

QString PlatformService::getContainerBundle(QString path, QString extension)
{
    int idx = 0;
    int startIdx = 0;
    while ((idx = path.indexOf(extension, startIdx)) >= 0)
    {
        int len = idx + extension.count();
        if (len == path.count() || path[len] == QDir::separator())
        {
            QString bundlePath = path.left(len);

            QFileInfo info(bundlePath);
            if (info.isBundle())
                return bundlePath;
        }
        
        startIdx = len;
    }
    
    return QString();
}

#endif

#ifdef Q_OS_WIN
// QFileIconProvider only returns the OS's "default" image size, which is 32x32 for Windows.
// We want to display the highest resolution icon in the game library, which is usually much
// larger than 32x32 so we need to use the Windows API.
// http://msdn.microsoft.com/en-us/library/ms997538.aspx
// TODO: Find a better way to do this using Qt?  Patch QFileIconProvider to use this method?

// #pragmas are used here to ensure that the structure's
// packing in memory matches the packing of the EXE or DLL.
#pragma pack( push )
#pragma pack( 2 )
typedef struct
{
   BYTE   bWidth;               // Width, in pixels, of the image
   BYTE   bHeight;              // Height, in pixels, of the image
   BYTE   bColorCount;          // Number of colors in image (0 if >=8bpp)
   BYTE   bReserved;            // Reserved
   WORD   wPlanes;              // Color Planes
   WORD   wBitCount;            // Bits per pixel
   DWORD  dwBytesInRes;         // how many bytes in this resource?
   WORD   nID;                  // the ID
} GRPICONDIRENTRY, *LPGRPICONDIRENTRY;
#pragma pack( pop )

#pragma pack( push )
#pragma pack( 2 )
typedef struct 
{
   WORD            idReserved;   // Reserved (must be 0)
   WORD            idType;       // Resource type (1 for icons)
   WORD            idCount;      // How many images?
   GRPICONDIRENTRY idEntries[1]; // The entries for each image
} GRPICONDIR, *LPGRPICONDIR;
#pragma pack( pop )

typedef struct
{
   BITMAPINFOHEADER icHeader;      // DIB header
   RGBQUAD          icColors[1];   // Color table
   BYTE             icXOR[1];      // DIB bits for XOR mask
   BYTE             icAND[1];      // DIB bits for AND mask
} ICONIMAGE, *LPICONIMAGE;

// forward declaration
QPixmap qt_pixmapFromWinHICON(HICON icon);

BOOL EnumIconNamesFunc(HANDLE hModule, LPCTSTR lpType, LPTSTR lpName, LONG_PTR lParam)
{
    HRSRC hRsrc = FindResource( (HMODULE)hModule, lpName, lpType );
    if(hRsrc)
    {
        HGLOBAL hGroup = LoadResource( (HMODULE)hModule, hRsrc );
        if(hGroup)
        {
            GRPICONDIR *lpGrpIconDir = (LPGRPICONDIR)LockResource( hGroup );
            if(lpGrpIconDir)
            {
                QIcon *icon = (QIcon*)lParam;
                if(icon)
                {
                    for(int i = 0; i < lpGrpIconDir->idCount; ++i)
                    {
                        GRPICONDIRENTRY * e = &lpGrpIconDir->idEntries[i];
                        hRsrc = FindResource((HMODULE)hModule, MAKEINTRESOURCE( e->nID ), RT_ICON);
                        HGLOBAL hGlobal = LoadResource((HMODULE)hModule, hRsrc);
                        ICONIMAGE *lpIconImage = (LPICONIMAGE)LockResource(hGlobal);
                        HICON hicon = CreateIconFromResourceEx((PBYTE)lpIconImage,
                                                                e->dwBytesInRes,
                                                                TRUE,
                                                                0x00030000,
                                                                e->bWidth,
                                                                e->bHeight,
                                                                0 );
                
                        icon->addPixmap(qt_pixmapFromWinHICON(hicon));

                        DestroyIcon(hicon);
                    }
                }
            }
        }
    }

    return TRUE;
}
#endif

QIcon PlatformService::iconForFile(const QString& executableAbsolutePath)
{
    QIcon icon;

#if defined(Q_OS_WIN)
    QString backslashPath(executableAbsolutePath);
    backslashPath.replace("/", "\\");
        
    HMODULE hLib = LoadLibraryEx(backslashPath.utf16(), NULL, LOAD_LIBRARY_AS_DATAFILE);
    if(hLib)
    {
        EnumResourceNames(hLib, RT_GROUP_ICON, (ENUMRESNAMEPROC)EnumIconNamesFunc, (LONG_PTR)&icon);
        FreeLibrary( hLib );
    }
#elif defined(ORIGIN_MAC)
    QFileIconProvider iconProvider;
    icon = iconProvider.icon(QFileInfo(executableAbsolutePath));
#else
#error "Need platform-specific info"
#endif

    // Fall back on the Qt functionality.  It gives us the wrong size but is better than nothing.
    if(icon.isNull())
    {
        QFileIconProvider iconProvider;
        icon = iconProvider.icon(QFileInfo(executableAbsolutePath));
    }

    return icon;
}

QString PlatformService::pathToConfigurationFile()
{
    QString path;
    
#if defined (Q_OS_WIN)
    
    path = commonAppDataPath();

#elif defined (ORIGIN_MAC)
    
    // On Mac we write into the app bundle's resource folder
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    PlatformService::ScopedCFRef<CFURLRef> originUrl(CFBundleCopyBundleURL(mainBundle));
    CFStringRef originUrlPath = CFURLCopyFileSystemPath(*originUrl, kCFURLPOSIXPathStyle);
    
    CFIndex length = CFStringGetLength(originUrlPath);
    CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8);
    
    char *buffer = (char *) malloc(maxSize);
    
    if (CFStringGetCString(originUrlPath, buffer, maxSize, kCFStringEncodingUTF8))
    {
        path.append(buffer);
        path += "/Contents/Resources/";
    }
    else 
    {
        ORIGIN_LOG_ERROR << "Failed to get bundle path while building path to configuration file";
    }
    
    free(buffer);
    
#endif
    
    return path;
}

void PlatformService::sleep(quint32 msecs)
{
#ifdef Q_OS_WIN
    ::Sleep(msecs);
#else
    struct timespec ts;
    ts.tv_sec = msecs / 1000;
    ts.tv_nsec = (msecs % 1000) * 1000 * 1000;
    nanosleep(&ts, NULL);
#endif
}

QString PlatformService::systemResultCodeString(const SystemResultCode code)
{
    QMap<SystemResultCode, QString>::const_iterator iter(sSystemResultCodeStrings.constFind(code));

    if (iter != sSystemResultCodeStrings.end())
    {
        return iter.value();
    }

    return QString("Unknown");
}

QString PlatformService::getLastSystemResultCodeString()
{
    return systemResultCodeString(lastSystemResultCode());
}


bool PlatformService::setAutoStart(bool autoStartEnabled)
{
#if defined(Q_OS_WIN)
    HKEY key = NULL;
    QString keyPath = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    RegOpenKeyExW(HKEY_CURRENT_USER, (LPCWSTR)keyPath.utf16(), 0L, KEY_SET_VALUE, &key);

    if (key)
    {
        QString value = EALS_PRODUCT_NAME_STR;
        if (autoStartEnabled)
        {
            QString launcherPath = QCoreApplication::applicationFilePath();
            launcherPath.replace("/", "\\");
            launcherPath.append(" -AutoStart");
            RegSetValueEx(key, value.toStdWString().c_str(), 0, REG_SZ, 
                (BYTE*)launcherPath.toStdWString().c_str(), launcherPath.length() * sizeof(wchar_t));
        }
        else
        {
            RegDeleteValue(key, value.toStdWString().c_str());
        }

        RegCloseKey(key);
        return true;
    } 
    return false;

#elif defined(Q_OS_MAC)
    // Use the launch services API to add/remove Origin from the login items list
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef originUrl = CFBundleCopyBundleURL(mainBundle);
    CFStringRef originUrlPath = CFURLCopyFileSystemPath(originUrl, kCFURLPOSIXPathStyle);
    LSSharedFileListRef loginItems = LSSharedFileListCreate(NULL, kLSSharedFileListSessionLoginItems, NULL);

    if (loginItems) 
    {
        if (autoStartEnabled) 
        {
            CFMutableDictionaryRef inPropertiesToSet = CFDictionaryCreateMutable(NULL, 1, NULL, NULL);
            CFMutableArrayRef inPropertiesToClear = CFArrayCreateMutable(NULL, 1, NULL);
            
            bool autoUpdate = Origin::Services::readSetting(Origin::Services::SETTING_AUTOUPDATE);
            if (autoUpdate)
            {
                // Set the login item to hide on auto-start
                if(inPropertiesToSet)
                {
                    CFDictionaryAddValue(inPropertiesToSet, kLSSharedFileListLoginItemHidden, kCFBooleanFalse);
                }
                else
                {
                    ORIGIN_LOG_WARNING << "Cannot set auto-start behavior to hide on start.";
                }
            }
            else
            {
                // Remove the hide flag on auto-start
                if(inPropertiesToClear)
                {
                    CFArrayAppendValue(inPropertiesToClear, kLSSharedFileListLoginItemHidden);
                }
                else
                {
                    ORIGIN_LOG_WARNING << "Cannot clear auto-start behavior to hide on start.";
                }
            }

            // Insert Origin into the login items list
            LSSharedFileListItemRef item = LSSharedFileListInsertItemURL(loginItems, kLSSharedFileListItemLast, NULL, NULL, originUrl, inPropertiesToSet, inPropertiesToClear);

            if (item)
            {
                CFRelease(item);
            }

            if(inPropertiesToSet)
            {
                CFRelease(inPropertiesToSet);
            }
            
            if(inPropertiesToClear)
            {
                CFRelease(inPropertiesToClear);
            }
        }
        else
        {
            // Remove Origin from the login items list
            CFArrayRef loginItemsArray = (CFArrayRef)LSSharedFileListCopySnapshot(loginItems, NULL);
            CFURLRef itemUrl;
            for (int i = 0; i < CFArrayGetCount(loginItemsArray); i++)
            {
                LSSharedFileListItemRef itemRef = (LSSharedFileListItemRef)CFArrayGetValueAtIndex(loginItemsArray, i);
                if (LSSharedFileListItemResolve(itemRef, 0, (CFURLRef*) &itemUrl, NULL) == noErr) 
                {
                    CFStringRef itemUrlPath = CFURLCopyFileSystemPath(itemUrl, kCFURLPOSIXPathStyle);
                    if (CFStringCompare(originUrlPath, itemUrlPath, 0) == kCFCompareEqualTo)
                    {
                        LSSharedFileListItemRemove(loginItems, itemRef);
                    }
                    CFRelease(itemUrlPath);
                }
            }
            if (loginItemsArray)
            {
                CFRelease(loginItemsArray);
            }
        }
        CFRelease(originUrl);
        CFRelease(originUrlPath);
        CFRelease(loginItems);
        return true;
    }
    CFRelease(originUrl);
    CFRelease(originUrlPath);
    return false;
#endif
}

bool PlatformService::isAutoStart()
{
#if defined(Q_OS_WIN)
    QString path = "";
    QString key = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\\";
    key.append(EALS_PRODUCT_NAME_STR);
    registryQueryValue(key, path);

    // This path is the autostart executable. If it's not there then autostart is off
    return !path.isEmpty();

#elif defined(Q_OS_MAC)
    // Use the launch services API to check if Origin is in the login items list
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    PlatformService::ScopedCFRef<CFURLRef> originUrl(CFBundleCopyBundleURL(mainBundle));
    CFStringRef originUrlPath = CFURLCopyFileSystemPath(*originUrl, kCFURLPOSIXPathStyle);
    PlatformService::ScopedCFRef<LSSharedFileListRef> loginItems(LSSharedFileListCreate(NULL, kLSSharedFileListSessionLoginItems, NULL));
    bool result = false;

    if (*loginItems)
    {
        PlatformService::ScopedCFRef<CFArrayRef> loginItemsArray((CFArrayRef)LSSharedFileListCopySnapshot(*loginItems, NULL));
        PlatformService::ScopedCFRef<CFURLRef> itemUrl;
        for (int i = 0; i < CFArrayGetCount(*loginItemsArray); i++)
        {
            LSSharedFileListItemRef itemRef = (LSSharedFileListItemRef)CFArrayGetValueAtIndex(*loginItemsArray, i);
            if (LSSharedFileListItemResolve(itemRef, 0, (CFURLRef*) &*itemUrl, NULL) == noErr)
            {
                CFStringRef itemUrlPath = CFURLCopyFileSystemPath(*itemUrl, kCFURLPOSIXPathStyle);
                if (CFStringCompare(originUrlPath, itemUrlPath, 0) == kCFCompareEqualTo)
                {
                    result = true;
                }
                CFRelease(itemUrlPath);
            }
        }
    }

    CFRelease(originUrlPath);

    return result;
#endif
}


#ifdef Q_OS_WIN
void PlatformService::openUrlWithInternetExplorer(const QUrl &url)
{
    QString urlString = QString::fromUtf8(url.toEncoded().constData());

    if (SUCCEEDED(OleInitialize(NULL)))
    {
        IWebBrowser2*    pBrowser2 = NULL;

        CoCreateInstance(CLSID_InternetExplorer, NULL, CLSCTX_LOCAL_SERVER, 
            IID_IWebBrowser2, (void**)&pBrowser2);
        if (pBrowser2)
        {
            VARIANT vEmpty;
            VariantInit(&vEmpty);

            BSTR bstrURL = SysAllocString(urlString.utf16());

            HRESULT hr = pBrowser2->Navigate(bstrURL, &vEmpty, &vEmpty, &vEmpty, &vEmpty);
            if (SUCCEEDED(hr))
            {
                pBrowser2->put_Visible(VARIANT_TRUE);
            }
            else
            {
                pBrowser2->Quit();
            }

            SysFreeString(bstrURL);
            pBrowser2->Release();
        }

        OleUninitialize();
    }
}
#endif

void PlatformService::asyncOpenUrl(const QUrl &url)
{
#ifdef Q_OS_WIN
    QString urlString = QString::fromUtf8(url.toEncoded().constData());

    SHELLEXECUTEINFO execInfo;
    ZeroMemory(&execInfo, sizeof(execInfo));

    execInfo.cbSize = sizeof(execInfo);
    // Tell Windows we want this opened in the background
    // In unspecified circumstances it can still block but I haven't seen an
    // example of that
    execInfo.fMask = SEE_MASK_ASYNCOK;
    execInfo.lpFile = urlString.utf16();
    execInfo.nShow = SW_SHOWNORMAL;

    ShellExecuteEx(&execInfo);
#else
    // Close enough
    QDesktopServices::openUrl(url);
#endif

}

void PlatformService::asyncOpenUrlAndSwitchToApp(const QUrl &url)
{
#ifdef Q_OS_WIN
    QString urlString = QString::fromUtf8(url.toEncoded().constData());

    SHELLEXECUTEINFO execInfo;
    ZeroMemory(&execInfo, sizeof(execInfo));

    execInfo.cbSize = sizeof(execInfo);
    // tell Windows we want this opened in the foreground
    execInfo.fMask = SEE_MASK_NOASYNC;
    execInfo.lpFile = urlString.utf16();
    execInfo.nShow = SW_RESTORE;

    if (ShellExecuteEx(&execInfo))
    {
        SetForegroundWindow(execInfo.hwnd);
    }

#else
    // Close enough
    QDesktopServices::openUrl(url);
#endif

}

bool PlatformService::isGuestUser()
{
#ifdef ORIGIN_PC
    // Open the access token associated with the calling process
    HANDLE hToken = NULL;
    bool retVal = false;
    if ( OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ) ) 
    {
        // Get the size of the memory buffer needed for the SID
        DWORD dwBufferSize = 0;
        if (! GetTokenInformation( hToken, TokenUser, NULL, 0, &dwBufferSize ) )
        {
            // Allocate buffer for user token data
            std::vector<BYTE> buffer;
            buffer.resize( dwBufferSize );
            PTOKEN_USER pTokenUser = reinterpret_cast<PTOKEN_USER>( &buffer[0] );

            // Retrieve the token information in a TOKEN_USER structure
            if ( GetTokenInformation( 
                hToken, 
                TokenUser, 
                pTokenUser, 
                dwBufferSize,
                &dwBufferSize)) 
            {
                if ( IsValidSid( pTokenUser->User.Sid ) ) 
                {
                    PSID pSID = pTokenUser->User.Sid;

                    // Get string corresponding to SID
                    LPTSTR pszSID = NULL;
                    if (!ConvertSidToStringSid( pSID, &pszSID ))
                    {
                        return false;
                    }

                    // Deep copy result in a CString instance
                    QString strSID = QString::fromStdWString(pszSID);

                    // Release buffer allocated by ConvertSidToStringSid API
                    LocalFree( pszSID );
                    pszSID = NULL;

                    if(strSID.right(3) == "501")
                    {
                        retVal = true;
                    }
                }
            }
        }
        // Cleanup
        CloseHandle( hToken );
        hToken = NULL;
    }
    return retVal;
#elif defined(ORIGIN_MAC)
    return getuid() == 201;
#else
#error "Requires platform implementation"
#endif
}

bool PlatformService::IsUserAdmin()
/*++ 
Routine Description: This routine returns TRUE if the caller's
process is a member of the Administrators local group. Caller is NOT
expected to be impersonating anyone and is expected to be able to
open its own process and process token. 
Arguments: None. 
Return Value: 
    TRUE - Caller has Administrators local group. 
    FALSE - Caller does not have Administrators local group. --
*/ 
{
#if defined(ORIGIN_PC)

    BOOL b = TRUE;

    // Major version numbers of the Windows Operating Systems.
    const int NMAJOR_2003R2_2003_XP_2000 = 5;
    const int NMINOR_2003R2_2003_XP64	    = 2;
    const int NMINOR_XP					    = 1;

    OSVERSIONINFOEX	osvi;
    SYSTEM_INFO		si;

    bool isXP = false;

    // ----------------------------------------
    // Prepare the structs for data population
    //

    ZeroMemory( &osvi, sizeof( OSVERSIONINFOEX ) );
    ZeroMemory( &si, sizeof( SYSTEM_INFO ) );
    osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );


    // ---------------------------------------------------
    // Determine what *this* specific Operating System is
    //

    if ( ::GetVersionEx( ( OSVERSIONINFO * ) &osvi ) )
    {
        ::GetSystemInfo( &si ); // Has no return value and we assume it always works.

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
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup; 
    b = AllocateAndInitializeSid(
        &NtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &AdministratorsGroup); 

    if(b) 
    {
        if (!CheckTokenMembership( NULL, AdministratorsGroup, &b)) 
        {
                b = FALSE;
        } 
        FreeSid(AdministratorsGroup); 
    }
    }
    
    return(b);
#elif defined (ORIGIN_MAC)
    gid_t *groups = NULL;
    int numGroups = 0;

    numGroups = getgroups(0, groups);
    groups = new gid_t[numGroups];
    getgroups(numGroups, groups);

    for (int i = 0; i < numGroups; i++)
    {
        if ((int)groups[i] == 80) // 80 is the admin group in osx
        {
            delete[] groups;
            return true;
        }
    }
    delete[] groups;
    return false;
#else
#  warning "TODO: implement IsUserAdmin()" 
    return true;
#endif
}

bool PlatformService::isSimCityProductId(QString const& productId)
{
    QString simCityProductIds(
      "OFB-EAST:50496 "     // Standard Edition (EN)
      "OFB-EAST:60108 "     // Standard Edition (WW)
      "OFB-EAST:50497 "     // Standard Edition (RU/PL/HU)
      "OFB-EAST:109547434 " // Standard Edition DVD (EN)
      "OFB-EAST:109547430 " // Standard Edition DVD (WW)
      "OFB-EAST:109547435 " // Standard Edition DVD (RU/PL/HU)
      "OFB-EAST:50534 "     // Limited Edition (EN)
      "OFB-EAST:48205 "     // Limited Edition (WW)
      "OFB-EAST:50536 "     // Limited Edition (RU/PL/HU)
      "OFB-EAST:109547432 " // Limited Edition DVD (EN)
      "OFB-EAST:109547436 " // Limited Edition DVD (WW)
      "OFB-EAST:109547433 " // Limited Edition DVD (RU/PL/HU)
      "OFB-EAST:50669 "     // Digital Deluxe Edition (EN)
      "OFB-EAST:48204 "     // Digital Deluxe Edition (WW)
      "OFB-EAST:50532 "     // Digital Deluxe Edition (RU/PL/HU)
      "OFB-EAST:109547456 " // Collectors Edition CIAB (EN+British City)
      "OFB-EAST:109547457 " // Collectors Edition CIAB (WW+British City)
      "OFB-EAST:109547459 " // Collectors Edition CIAB (WW+German City)
      "OFB-EAST:109547458 " // Collectors Edition CIAB (WW+French City)
    );
    
    return simCityProductIds.contains(productId, Qt::CaseInsensitive);
}

/*!
    \brief Creates DriveInfo object based on given parameters.

    \note This info is captured at the time of creation for the DriveInfo 
          object and doesn't necessarily reflect accurate data for the 
          "present", that is, this information should be considered stale 
          even at the time of DriveInfo object creation.

    \param driveString Drive-letter string (for example, "C:\" on Windows).

    \param driveType Drive type (fixed, removable, network, etc.). 
           See EA::IO::DriveType.

    \param fileSystemType String representation of file-system (for example,
           FAT, FAT32, NTFS, etc.)

    \param totalBytes Total number of bytes for this drive.

    \param freeBytes Free bytes available on the drive.
*/
PlatformService::DriveInfo::DriveInfo(
    const QString& driveString, 
    const QString& driveType, 
    const QString& fileSystemType,
    const quint64& totalBytes, 
    const quint64& freeBytes)
    : mDriveString(driveString),
      mDriveType(driveType),
      mFileSystemType(fileSystemType),
      mTotalBytes(totalBytes),
      mFreeBytes(freeBytes)
{
}


/*!
    \brief Prints drive info to the client log.

    The following drive information is printed to the client log:
    Logical drive string
    The drive type (e.g. fixed, remote, removable, etc.)
    Total bytes
    Free bytes
    File system type

    \note Currently isn't cross-platform.
*/
void PlatformService::PrintDriveInfoToLog()
{
    QList<DriveInfo> driveInfoList;

    // Populate the list with this machine's drive information
    GetDriveInfo(driveInfoList);

    for (qint32 i = 0; i < driveInfoList.size(); ++i)
    {
        DriveInfo& driveInfo = driveInfoList[i];

        // Dump drive info to the client log
        ORIGIN_LOG_EVENT << 
            driveInfo.GetDriveString() << " " <<
            driveInfo.GetDriveTypeString() << " " <<
            driveInfo.GetTotalBytes() << " " <<
            driveInfo.GetFreeBytes() << " " <<
            driveInfo.GetFileSystemTypeString();
    }
}

/*!
    \brief Populates the given list with DriveInfo objects for this machine.

    DriveInfo objects are appended to the given list.
*/
void PlatformService::GetDriveInfo(QList<DriveInfo>& driveInfoList)
{
    QStringList driveLetterStrings;

    GetDriveLetterStrings(driveLetterStrings);

    for (qint32 i = 0; i < driveLetterStrings.size(); ++i)
    {
        // Get the drive letter string representing the drive
        QString& driveString = driveLetterStrings[i];

        // Given the drive-letter string, get the drive type
        EA::IO::DriveType driveType = EA::IO::GetDriveTypeValue(driveString.toStdString().c_str());

        // Convert drive type to QString
        QString driveTypeString(GetDriveTypeAsQString(driveType));

        // Create variables to hold our total and free bytes for this drive
        quint64 totalBytes = 0;
        quint64 freeBytes = 0;

        // Get the total and free number of bytes for this drive
        GetFreeDiskSpace(driveString, &totalBytes, &freeBytes);

        // Get the file system type
        QString fileSystemType(GetFileSystemType(driveString));

        // Add the DriveInfo to our list of DriveInfo objects
        driveInfoList.append(
            DriveInfo(
                driveString, 
                driveTypeString, 
                fileSystemType,
                totalBytes, 
                freeBytes));
    }
}

/*!
    \brief Populate the given string list with all of the drive-letter 
           associated with this machine.

    Drive-letter strings are appended to the given QString list.
*/
void PlatformService::GetDriveLetterStrings(QStringList& driveStringList)
{

#ifdef Q_OS_WIN

    // Declare buffer size for holding buffer of drive letter strings
    static const quint32 BUFFER_SIZE = 512;

    // This buffer will store a list of null-terminated drive letters
    wchar_t pDriveStringBuffer[BUFFER_SIZE];
    pDriveStringBuffer[0] = L'\0';

    // Populate the buffer with the drive strings
    const quint32 driveStringLength = 
        GetLogicalDriveStrings(BUFFER_SIZE - 1, pDriveStringBuffer);

    if (driveStringLength > 0 && driveStringLength < BUFFER_SIZE)
    {
        // Get a pointer to the buffer that we can modify and use as an 
        // iterator
        wchar_t* pBufferIter = pDriveStringBuffer;

        // Keep track of where we are in the drive-list string to ensure we
        // don't venture out-of-bounds
        quint32 index = 0;

        do
        {
            // Note that drive-string list is a single string where 
            // drive-letters are separated by NULL terminating character.
            driveStringList.append(QString::fromWCharArray(pBufferIter));
            
            // Iterate through the buffer until we hit a NULL character,
            // however we post-increment to ensure we're ready for the next
            // drive-letter string if we are within bounds of the index.
            while (++index && *pBufferIter++);

        } while (index < driveStringLength);
    }

#else

    // TODO: determine whether to implement this

#endif // Q_OS_WIN

}

/*!
    \brief Given a drive letter string, return string representation of the
           drive type.
*/
QString PlatformService::GetFileSystemType(const QString& driveString)
{

#ifdef Q_OS_WIN

    // We need to represent the passed drive string as a wchar_t string
    wchar_t* pDriveString = new wchar_t[driveString.size() + 1];
    driveString.toWCharArray(pDriveString);

    // QString::toWCharArray doesn't append null-character to string
    pDriveString[driveString.size()] = L'\0';

    // Misc. flags that we'll need to get our desired info
    DWORD maxComponentLength;
    DWORD fileSystemFlags;

    // String to hold the file system type - MAX_PATH + 1 is the maximum size
    // of the buffer.
    wchar_t pFileSystemType[MAX_PATH + 1];

    // CD and floppy drives won't return a filesystem type if there's no disk
    // in the drive, so we initialize to null string to prevent returning 
    // garbage
    pFileSystemType[0] = '\0';

    // According to GetVolumeInformation documentation, attempting to 
    // retrieve volume info for a CD/floppy drive that doesn't have a disc
    // inserted will result in Windows prompting the user to insert a disc.
    // We can avoid this by setting the error mode to SEM_FAILCRITICALERRORS.
    UINT previousErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    // Get the file system type
    GetVolumeInformation(
        pDriveString, 
        NULL,
        0,
        NULL,
        &maxComponentLength,
        &fileSystemFlags,
        pFileSystemType,
        MAX_PATH + 1);

    // Restore the previous error mode
    SetErrorMode(previousErrorMode);

    // Free dynamically allocated memory
    delete[] pDriveString;

    // Return file system type as a QString
    return QString::fromWCharArray(pFileSystemType);

#else

    // TODO: determine whether to implement this
    return QString();

#endif // #ifdef Q_OS_WIN

}

bool PlatformService::isOIGWindow(QWidget* widget)
{
    return widget->property("OIG").isValid();
}

#ifdef Q_OS_WIN
bool PlatformService::isWindowFullScreen(HWND topWin)
{
    RECT appBounds;
    HMONITOR topMonitor;
    MONITORINFO monInfo;
    bool ok = false;

    if (topWin)
    {
        topMonitor = ::MonitorFromWindow (topWin, MONITOR_DEFAULTTOPRIMARY);
		if (topMonitor)
		{
    		monInfo.cbSize = sizeof (MONITORINFO);
	    	ok = GetMonitorInfo (topMonitor, &monInfo);
            if (ok)
            {
                if (topWin != ::GetDesktopWindow() && topWin != ::GetShellWindow())
                {
                    ::GetWindowRect(topWin, &appBounds);
                    if (monInfo.rcMonitor.bottom == appBounds.bottom &&
                        monInfo.rcMonitor.left == appBounds.left &&
                        monInfo.rcMonitor.right == appBounds.right &&
                        monInfo.rcMonitor.top == appBounds.top)
                    {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}


QString const PlatformService::getCompatibilityMode()
{
    const HKEY registryRoots[2] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
    const int MAX_VALUE_NAME = 16383;

    for (int rootIndex = 0; rootIndex < _countof(registryRoots); rootIndex++)
    {
        HKEY     hKey;
        TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
        DWORD    cchClassName = MAX_PATH;  // size of class string 
        DWORD    cSubKeys = 0;               // number of subkeys 
        DWORD    cbMaxSubKey;              // longest subkey size 
        DWORD    cchMaxClass;              // longest class string 
        DWORD    cValues;              // number of values for key 
        DWORD    cchMaxValue;          // longest value name 
        DWORD    cbMaxValueData;       // longest value data 
        DWORD    cbSecurityDescriptor; // size of security descriptor 
        FILETIME ftLastWriteTime;      // last write time 

        DWORD retCode;

        TCHAR  achValue[MAX_VALUE_NAME];
        DWORD cchValue = MAX_VALUE_NAME;

        if (RegOpenKeyEx(registryRoots[rootIndex], TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            // Get the class name and the value count. 
            retCode = RegQueryInfoKey(
                hKey,                    // key handle 
                achClass,                // buffer for class name 
                &cchClassName,           // size of class string 
                NULL,                    // reserved 
                &cSubKeys,               // number of subkeys 
                &cbMaxSubKey,            // longest subkey size 
                &cchMaxClass,            // longest class string 
                &cValues,                // number of values for this key 
                &cchMaxValue,            // longest value name 
                &cbMaxValueData,         // longest value data 
                &cbSecurityDescriptor,   // security descriptor 
                &ftLastWriteTime);       // last write time 

            // Enumerate the key values. 

            if (cValues)
            {
                for (DWORD i = 0, retCode = ERROR_SUCCESS; i < cValues; i++)
                {
                    cchValue = MAX_VALUE_NAME;
                    achValue[0] = '\0';
                    retCode = RegEnumValue(hKey, i, achValue, &cchValue, NULL, NULL, NULL, NULL);
                    if (retCode == ERROR_SUCCESS)
                    {
                        // convert cstring to qstring and substring key for "Origin.exe"
                        CString s1 = achValue;
                        QString compatibilityKey;
    #ifdef _UNICODE
                        compatibilityKey = QString::fromWCharArray((LPCTSTR)s1, s1.GetLength());
    #else
                        compatibilityKey = QString((LPCTSTR)s1);
    #endif

                        // if Origin.exe exists in the value, then this is compatibility mode. Grab the value of this key
                        if (compatibilityKey.contains("Origin.exe", Qt::CaseInsensitive))
                        {
                            CRegKey k;
                            CString s;
                            ULONG len = 255;
                            if (k.Open(registryRoots[rootIndex], L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers") == ERROR_SUCCESS)
                            {
                                k.QueryStringValue(achValue, s.GetBufferSetLength(len), &len);
                                s.ReleaseBuffer();
                                QString compatibilityValue;
    #ifdef _UNICODE
                                compatibilityValue = QString::fromWCharArray((LPCTSTR)s, s.GetLength());
    #else
                                compatibilityValue = QString((LPCTSTR)s);
    #endif
                                k.Close();
                                return compatibilityValue;
                            }
                        }
                    }
                }
            }
        }
    }
    return QString();
}
#endif

float PlatformService::GetCurrentApplicationVolume()
{
    float volume = 1.0f;
#ifdef Q_OS_WIN
    // If we are on XP just return 100%
    if (Origin::Services::PlatformService::OSMajorVersion() < 6)
    {
        return (volume);
    }
    HRESULT hr = S_OK;
    IMMDeviceEnumerator *pDeviceEnumerator = NULL;
    // Grab an instance of our Device Enumerator
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pDeviceEnumerator);
    if (FAILED(hr))
        return (volume);

    IMMDevice *pDefaultEndpoint = NULL;
    // Grab the default audio endpoint from teh enumerator
    hr = pDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDefaultEndpoint);
    if (FAILED(hr))
        return (volume);

    IAudioClient *pAudioClient = NULL;
    // Get the audio client from the default device.
    hr = pDefaultEndpoint->Activate(
        __uuidof(IAudioClient), 
        CLSCTX_ALL,
        NULL, 
        (void**)&pAudioClient);
    if (FAILED(hr))
        return (volume);

    WAVEFORMATEX *pwfx = NULL;
    // Get the client's format.
    hr = pAudioClient->GetMixFormat(&pwfx);
    if (FAILED(hr))
        return (volume);

    REFERENCE_TIME hnsRequestedDuration = 10000000;
    // Open the stream and associate it with an audio session.
    hr = pAudioClient->Initialize( 
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 
        hnsRequestedDuration, 
        hnsRequestedDuration, 
        pwfx, 
        NULL);
    if (FAILED(hr))
        return (volume);

    ISimpleAudioVolume* obj = NULL;
    // From the intialized client grab the simpleAudoVolume object
    hr = pAudioClient->GetService(__uuidof(ISimpleAudioVolume), (void**)&obj);
    if (FAILED(hr))
        return (volume);
    
    // If we have an object grab the current volume
    if (obj)
    {
        obj->GetMasterVolume(&volume);
        //Clean up
        obj->Release();
    }
    pDefaultEndpoint->Release();
    // the volume returned is a float between 0-1
    return (volume);
#else
    return (volume);
#endif

}



#ifdef Q_OS_WIN
///
/// Helper function for removing . and .. elements in a path. 
/// All . elements are removed, and all .. elements are removed 
/// along with whatever element precedes it. This function is
/// only called with absolute paths, so .. should never be the 
/// first element.
///
void ResolveDots( QStringList& elements )
{
    while ( elements[0] == ".." )
        elements.removeAt(0);

    for ( int i = 0; i < elements.size(); ++i )
    {
        if ( elements[i] == "." )
        {
            elements.removeAt(i--);
        }
        else if ( elements[i] == ".." )
        {
            elements.removeAt(i--);
            elements.removeAt(i--);
        }
    }
}

///
/// Helper function resolves relative paths, . and .., symlinks, etc.
/// The returned value is either the empty string, or it is a path that
/// meets all of the following conditions: it is absolute, it exists, it 
/// is a directory, it does not end with /, it does not contain . or .. 
/// elements, it does not contain symbolic links, it may be an UNC path in 
/// which case it begins with /. Also, the returned path represents the 
/// same directory as the deepest existing directory in the argument path.
///
/// There are two Qt bugs related to dangling symlinks: QFileInfo::exists() 
/// returns true for such links, contrary to documentation, and 
/// QFileInfo::canonicalPath() returns the current working directory, 
/// rather than "". This function has been written to still work if these bugs 
/// are fixed, i.e. the two affected functions are never called on dangling 
/// links.
///
QString CanonicalPath( QString const& path )
{
    QString p(path);
    p.replace("/", "\\");

    QSet<QString> circularCheck;

    circularCheck << path;
    bool unc = p.startsWith("\\\\");

    if ( !unc && QFileInfo(p).isRelative() )
    {
        p = QDir::currentPath() + "\\" + p;
        p.replace("/", "\\");
    }
    // this call removes the UNC prefix, hence we need the bool
    QStringList elements = p.split("\\", QString::SkipEmptyParts);

    if (elements.size() == 0)
    {
        return "";
    }
    else 
    {
        // ".." is resolved before symlinks, so "dir2\dir22\link\.." 
        // means "dir2\dir22\" regardless of where link is pointing, 
        // verified using dir on the command line
        ResolveDots( elements );
        // iterate from the root towards the end of the path, until we
        // find an element that is either a file or non-existent, 
        // resolving symlinks as we go.
        for ( int i = 0; i < elements.size(); ++i )
        {
            // the first i+1 elements of the path
            QString head = QStringList(elements.mid(0, i+1)).join("\\");
            // the remainder of the path
            QString tail = QStringList(elements.mid(i+1)).join("\\");
            if ( unc )
                head = "\\\\" + head;
            QFileInfo headInfo(head);
            if ( headInfo.isSymLink() )
            {
                // target may or may not exist
                QString target = headInfo.symLinkTarget();

                // replace UNC/ prefix with \\ prefix
                if ( target.startsWith("UNC/") )
                    target = "\\\\" + target.mid(4);

                else if ( QFileInfo(target).isRelative() )
                    target = headInfo.absolutePath() + "\\" + target;

                if ( tail.size() )
                    target += "\\" + tail;

                target.replace("/", "\\");

                if ( circularCheck.contains(target) )
                    return "";

                circularCheck << target;
                unc = target.startsWith("\\\\");
                elements = target.split("\\", QString::SkipEmptyParts);
                // the sym link may have contained dots, resolve those now
                ResolveDots( elements );
                i = -1; // start over from the beginning of the path
            } 
            else if ( !headInfo.exists() || headInfo.isFile() )
            {
                // strip off non-existent path elements and files
                elements = elements.mid(0, i);
                break;
            }
        }
        if ( unc )
            elements.push_front("\\");

        return elements.join("\\");
    }
}

int64_t PlatformService::GetFreeDiskSpace( QString const& path, uint64_t* pTotalNumberOfBytes, uint64_t* pTotalNumberOfFreeBytes )
{
    QString canonical = CanonicalPath(path);

    if ( !canonical.isEmpty() )
    {
        canonical += "/"; // needed for UNC paths

        ULARGE_INTEGER freeBytesAvailable;
        ULARGE_INTEGER totalNumberOfBytes;
        ULARGE_INTEGER totalNumberOfFreeBytes;

        freeBytesAvailable.QuadPart = 0;
        totalNumberOfBytes.QuadPart = 0;
        totalNumberOfFreeBytes.QuadPart = 0;
        if ( GetDiskFreeSpaceEx(canonical.utf16(), &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes ) )
        {
            if ( NULL != pTotalNumberOfBytes )
                *pTotalNumberOfBytes = totalNumberOfBytes.QuadPart;

            if ( NULL != pTotalNumberOfFreeBytes)
                *pTotalNumberOfFreeBytes = totalNumberOfFreeBytes.QuadPart;

            return freeBytesAvailable.QuadPart;
        }
    }
    return -1;
}

#elif defined(Q_OS_MAC)

void cdUp(QString & path)
{
    int where = path.lastIndexOf(QDir::separator());
    if (where > 0)
        path = path.left(where);
    else
        path.clear();
}

qint64 PlatformService::GetFreeDiskSpace(
    QString const& path,
    quint64* pTotalNumberOfBytes, 
    quint64* pTotalNumberOfFreeBytes)
{
    // We may start from an invalid path (ie when we are about to download a new game, we haven't created the full path yet) but we are going
    // to put the data in a known/valid directory anyways, so we just need to find that guy
    // In Qt5, canonicalPath will return an empty path if it doesn't exist on disk
    // Final note: what I'm doing may be overkill, but better be safe than sorry!
    QString currentPath = path;
    QString canonicalPath = QDir(currentPath).canonicalPath();
    while (!currentPath.isEmpty() && canonicalPath.isEmpty())
    {
        cdUp(currentPath);
        canonicalPath = QDir(currentPath).canonicalPath();
    }
    
    if (currentPath.isEmpty())
        return 0;
    
    struct statfs64 fsinfo64;
    int result = statfs64(canonicalPath.toUtf8(), &fsinfo64);
    if (result == 0)
    {
        if ( NULL != pTotalNumberOfBytes )
            *pTotalNumberOfBytes = fsinfo64.f_bsize * fsinfo64.f_blocks;
        if ( NULL != pTotalNumberOfFreeBytes)
            *pTotalNumberOfFreeBytes = fsinfo64.f_bsize * fsinfo64.f_bfree;
        return fsinfo64.f_bsize * fsinfo64.f_bavail;
    }

    return -1;
}

#else
#error "Use statfs() to get free disk space on *nix"
#endif

/// \brief returns the file system path for a well-known standard location
QString PlatformService::getStorageLocation(QStandardPaths::StandardLocation location)
{
    QStringList paths = QStandardPaths::standardLocations(location);
    QStringList::const_iterator iter;
    for (iter = paths.constBegin(); iter != paths.constEnd(); ++iter)
    {
        if (!(*iter).isEmpty())
            return *iter;
    }
        
    ORIGIN_LOG_EVENT << "Failed to find standard direction location (" << location << ")";
        
    return QString();
}

bool PlatformService::GetVolumeWritableStatusFromPath(QString const& path)
{
#ifdef ORIGIN_PC
    // Minimum folder path is X:/
    if (path.length() < 3)
        return false;
    // We are only interested in drives here, not UNC paths
    if (path.at(1) != ':')
        return false;

    // Create the volume path
    QString volumePath = QString("\\\\.\\%1:").arg(path.at(0));

    Auto_HANDLE hStorageDevice = CreateFile((LPCWSTR)volumePath.utf16(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE,NULL, OPEN_EXISTING, 0, NULL);
    if (hStorageDevice == INVALID_HANDLE_VALUE)
    {
        ORIGIN_LOG_ERROR << "Unable to open volume handle for volume: " << volumePath << " Error: " << GetLastError();
        return false;
    }

    DWORD bytesReturned = 0;
    if (!DeviceIoControl(hStorageDevice,IOCTL_DISK_IS_WRITABLE, NULL, 0, NULL, 0, &bytesReturned, NULL))
    {
        // Per MSDN, read-only volumes return ERROR_WRITE_PROTECT here
        DWORD lastError = GetLastError();
        if (lastError == ERROR_WRITE_PROTECT)
        {
            ORIGIN_LOG_ERROR << "Volume: " << volumePath << " is not read-only.";
        }
        else
        {
            ORIGIN_LOG_ERROR << "Volume: " << volumePath << " is not ready. Error: " << lastError;
        }
        return false;
    }

    ORIGIN_LOG_EVENT << "Volume: " << volumePath << " is ready and writable.";
    return true;
#else
    // TODO: Figure out how to determine volume status on Mac
    return true;
#endif
}

bool PlatformService::GetDACLStringFromPath(QString const& path, bool useParentDirectory, QString& outDACL)
{
    QString pathToInvestigate = path;

    if (useParentDirectory)
    {
        QFileInfo fileInfo(pathToInvestigate);
        pathToInvestigate = fileInfo.absoluteDir().absolutePath();
    }

    if (pathToInvestigate.isEmpty())
    {
        ORIGIN_LOG_ERROR << "Path was empty, can't get DACL.";
        return false;
    }

#ifdef ORIGIN_PC
    AutoLocalHeap<PSECURITY_DESCRIPTOR> pSD;
    if (GetNamedSecurityInfo((LPCWSTR)pathToInvestigate.utf16(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &pSD) != ERROR_SUCCESS)
    {
        ORIGIN_LOG_ERROR << "Unable to get DACL for path: " << pathToInvestigate << " Error: " << GetLastError();
        return false;
    }
    
    AutoLocalHeap<LPWSTR> outputStr;
    ULONG len = 0;
    if (!ConvertSecurityDescriptorToStringSecurityDescriptor(pSD, SDDL_REVISION_1, DACL_SECURITY_INFORMATION, &outputStr, &len))
    {
        ORIGIN_LOG_ERROR << "Unable to convert security descriptor to string.  Error: " << GetLastError();
        return false;
    }

    outDACL = QString::fromUtf16(outputStr.data());
    return true;
#else
    return true;
#endif
}

QDataStream &PlatformService::operator << (QDataStream &out, const OriginPlatform &platformId)
{
    return out << stringForPlatformId(platformId);
}

QDataStream &PlatformService::operator >> (QDataStream &in, OriginPlatform &platformId)
{
    QString platform;

    in >> platform;
    platformId = platformIdForString(platform);

    return in;
}
