#include <limits>
#include <QDateTime>
#include <QDebug>
#include <QThread>
#include <QMetaType>
#include <QUrl>
#include <QRegExp>


#ifdef ORIGIN_PC
#include <Shlwapi.h>
#endif

#ifdef ORIGIN_MAC
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "engine/downloader/ContentServices.h"
#include "engine/downloader/ContentInstallFlowDiP.h"
#include "engine/downloader/ContentInstallFlowNonDiP.h"
#include "engine/content/Entitlement.h"
#include "engine/content/LocalContent.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentController.h"
#include "ContentProtocols.h"
#include "services/platform/EnvUtils.h"
#include "login/User.h"
#include "services/downloader/File.h"
#include "services/downloader/FilePath.h"
#include "services/downloader/StringHelpers.h"
#include "services/settings/SettingsManager.h"
#include "services/log/LogService.h"
#include "TelemetryAPIDLL.h"

namespace Origin
{
namespace Downloader
{
    const QString ContentServices::EAD_REG_STAGING_KEY = "SOFTWARE\\Electronic Arts\\EA Core\\Staging";
    const QString ContentServices::EAD_REG_CODE = "ergc";
    const QString ContentServices::EAD_REG_MEMBER_NAME = "MemberName";
    const QString ContentServices::EAD_REG_ACCESS_STAGING_KEY = "SOFTWARE\\Origin Games";
    const QString ContentServices::EAD_REG_DISPLAY_NAME = "DisplayName";
    const QString ContentServices::EAD_REG_LOCALE = "Locale";
	const QString ContentServices::SREGFILE = "staging.reg";

IContentInstallFlow*  ContentServices::CreateInstallFlow (Origin::Engine::Content::EntitlementRef item, Engine::UserRef user )
{
    IContentInstallFlow *flow = NULL;

    if (item->contentConfiguration()->dip())
    {
        flow = new ContentInstallFlowDiP(item, user);
    }
    else if ((item->contentConfiguration()->packageType() == Origin::Services::Publishing::PackageTypeSingle) ||
             (item->contentConfiguration()->packageType() == Origin::Services::Publishing::PackageTypeUnpacked))
    {
        flow = new ContentInstallFlowNonDiP(item, user);
    }

    return flow;
}


void ContentServices::InitializeInstallManifest(const Origin::Engine::Content::EntitlementRef item, Parameterizer& installManifest)
{
    installManifest.ClearAll();

    QString downloaderVersion = "9.0.0.0";
    installManifest.SetString("downloaderversion", downloaderVersion);

    installManifest.SetString("id", item->contentConfiguration()->productId());
    installManifest.SetString("locale", "");
    installManifest.SetBool("ispreload", false);
    installManifest.SetBool("isLocalSource", false);
    installManifest.SetBool("isITOFlow", false);
    installManifest.SetBool("paused", false);
    installManifest.SetBool("autoresume", false);
    installManifest.SetBool("autostart", false);
    installManifest.SetBool("downloading", false);
    installManifest.SetBool("isrepair", false);
    installManifest.Set64BitInt("totalbytes", 0);
    installManifest.Set64BitInt("savedbytes", 0);
    installManifest.SetString("contentversion", item->contentConfiguration()->version());

    installManifest.SetString("currentstate", ContentInstallFlowState(ContentInstallFlowState::kInvalid).toString());
    installManifest.SetString("previousstate", ContentInstallFlowState(ContentInstallFlowState::kInvalid).toString());
}

bool ContentServices::LoadInstallManifest (const Origin::Engine::Content::EntitlementRef item, Parameterizer& installManifest)
{
    QString path;
    if (GetInstallManifestPath(item, path))
    {    
        QFile installManifestFile(path);
        if (!installManifestFile.exists(path))
        {
            return false;
        }

        if (installManifestFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream instream(&installManifestFile);         
            QString stateInfo = instream.readLine();

            // Temporary legacy code for loading in case existing manifest isn't loaded correctly using QTextStream.
            if (stateInfo.isEmpty())
            {
                if (!StringHelpers::LoadQStringFromFile(path, stateInfo))
                {
                    return false;
                }
            }

            installManifest.ClearAll();
            installManifest.Parse(stateInfo);
        }
        else
        {
            ORIGIN_LOG_WARNING << "Unable to open install manifest: " << path;
            return false;
        }

        return true;
    }

    return false;
}

bool ContentServices::SaveInstallManifest(const Origin::Engine::Content::EntitlementRef item, const Parameterizer& installManifest)
{
    QString path;
    if (GetInstallManifestPath(item, path))
    {
        QString stateInfo;
        installManifest.GetPackedParameterString(stateInfo);

        QFile installManifest(path);
        if (installManifest.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&installManifest);
            out << stateInfo;
            out.flush();

            if (out.status() == QTextStream::WriteFailed)
            {
                ORIGIN_LOG_WARNING << "Unable to write to install flow manifest: " << path;
            }

#ifdef ORIGIN_MAC
            
            // Explicitly set relaxed permissions to allow resuming downloads by other users.
            installManifest.setPermissions( installManifest.permissions() | QFile::WriteOwner|QFile::WriteGroup|QFile::WriteUser|QFile::WriteOther );

#endif
            
            installManifest.close();
        }
        else
        {
            ORIGIN_LOG_WARNING << "Unable to open install flow manifest: " << path;
        }
    }

    return false;
}

void ContentServices::DeleteInstallManifest(const Origin::Engine::Content::EntitlementRef item)
{
    QString path;
    if (GetInstallManifestPath(item, path))
    {
        QFile installManifest(path);
        if (installManifest.remove() == false)
        {
            GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("ContentServices_DeleteInstallManifest", path.toUtf8().data(), installManifest.error());
        }
    }
}

bool ContentServices::GetInstallManifestPath(const Origin::Engine::Content::EntitlementRef item_param,  QString& path, bool createIfNotExists /*= false*/)
{
    if (!item_param.isNull())
    {
        Origin::Engine::Content::EntitlementRef item = item_param;
        QString fileNameSafeId = item->contentConfiguration()->productId();
        Origin::Downloader::StringHelpers::StripReservedCharactersFromFilename(fileNameSafeId);

        if (item->contentConfiguration()->isPDLC())
        {
            if (item->parent().isNull())
            {
                return false;
            }

            item = item->parent();
        }

        QString installPath;
        if (GetContentInstallCachePath(item, installPath, createIfNotExists))
        {
            if (!installPath.endsWith("\\"))
            {
                installPath.append("\\");
            }
            QString installManifestFilename = QString("%1.mfst").arg(fileNameSafeId);
            path = CFilePath::Absolutize(installPath, installManifestFilename).ToString();
            return true;
        }
    }

    return false;
}

bool ContentServices::GetContentInstallCachePath(const Origin::Engine::Content::EntitlementRef content, QString& path, bool createIfNotExists /*= false*/, int *contentDownloadErrorType /*= NULL */, int *elevation_error /*= NULL*/)
{
    if (contentDownloadErrorType)
        *contentDownloadErrorType = 0;

    if (elevation_error)
        *elevation_error = 0;

    if (content != NULL)
    {
        Origin::Engine::Content::LocalContent *localContent;

        if (content->contentConfiguration()->isPDLC())
        {
            if (content->parent().isNull())
            {
                return false;
            }

            localContent = content->parent()->localContent();
        }
        else
        {
            localContent = content->localContent();
        }

        int elevation_result = 0;
        path = localContent->cacheFolderPath(createIfNotExists, false, &elevation_result, elevation_error);

        // Must translate the protocol error to a download error
        if (contentDownloadErrorType)
        {
            if (elevation_result != Origin::Escalation::kCommandErrorNone)
            {
                *contentDownloadErrorType = DownloadErrorFromEscalationError(elevation_result, ProtocolError::ContentFolderError);
            }
        }
    }
    return !path.isEmpty();
}

void ContentServices::ClearDownloadCache(const Origin::Engine::Content::EntitlementRef content)
{
    QString cachePath;
    GetContentDownloadCachePath(content, cachePath);

    if (!cachePath.isEmpty())
    {
        EnvUtils::DeleteFolderIfPresent(cachePath);
    }
}

bool ContentServices::GetContentDownloadCachePath(const Origin::Engine::Content::EntitlementRef content, QString& path, bool createIfNotExists /*= false*/)
{
    path = content->localContent()->nonDipInstallerPath(createIfNotExists);
    return !path.isEmpty();
}

QString ContentServices::GetTempFolderPath()
{
    // Get the temp folder
    QString tempDir = QDir::toNativeSeparators(QDir::tempPath());

    // If the temp dir is empty, fall back to the download cache dir
    if (tempDir.isEmpty() || !QDir(tempDir).exists())
    {
        ORIGIN_LOG_WARNING << "Invalid QDir::tempPath() [" << tempDir << "]; Using download cache.";
        QString originCachePath(QDir::toNativeSeparators(Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOAD_CACHEDIR)));
        if (originCachePath.isEmpty() || !QDir(originCachePath).exists())
        {
#ifdef ORIGIN_PC
            tempDir = "C:\\Windows\\Temp";      
#else
            tempDir = "/tmp";
#endif

            ORIGIN_LOG_ERROR << "Invalid download cache dir [" << originCachePath << "]; Using default fallback: " << tempDir;
        }
        else
        {
            tempDir = originCachePath;
        }
    }

    return tempDir;
}

QString ContentServices::GetTempFilename(const QString& offerId, const QString& pattern)
{
    // Get product ID for constructing temp filenames
    QString productId = offerId;

    // Strip special chars
    productId.replace(QRegExp("[^a-zA-Z0-9]"), "_");

    // Get the temporary path
    QString tempDir = GetTempFolderPath();

    // Construct a file prefix
    QString prefix = pattern + "_" + productId;

    // Generate the random part from a new UUID -> SHA-1 -> Truncate to 6 digits
    QString randomPart = QString(QCryptographicHash::hash(QUuid::createUuid().toByteArray(), QCryptographicHash::Sha1).toHex()).right(6);

    return tempDir + QDir::separator() + prefix + "_" + randomPart + ".tmp";
}

#ifdef ORIGIN_MAC
void ContentServices::CreateAccessPlist(const Origin::Engine::Content::EntitlementRef content, const QString& locale, const QString& bundlePath)
{
    // Create the plist dictionary.
    CFMutableDictionaryRef dict = CFDictionaryCreateMutable (kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    // Create dictionary entries for the display name and the locale.
    // Use class constants for the dictionary keys.
    // Display name
    CFStringRef cfsKey = CFStringCreateWithBytes (kCFAllocatorDefault, (const UInt8*)EAD_REG_DISPLAY_NAME.toUtf8().constData(), EAD_REG_DISPLAY_NAME.toUtf8().size(), kCFStringEncodingUTF8, false);
    QString sDisplayName(content->contentConfiguration()->displayName());
    CFStringRef cfsDisplayName = CFStringCreateWithBytes (kCFAllocatorDefault, (const UInt8*)sDisplayName.toUtf8().constData(), sDisplayName.toUtf8().size(), kCFStringEncodingUTF8, false);
    CFDictionarySetValue (dict, cfsKey, cfsDisplayName);
    
    // Locale
    CFRelease (cfsKey);
    cfsKey = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8*)EAD_REG_LOCALE.toUtf8().constData(), EAD_REG_LOCALE.toUtf8().size(), kCFStringEncodingUTF8, false);
    CFStringRef cfsLocale = CFStringCreateWithBytes (kCFAllocatorDefault, (const UInt8*)locale.toUtf8().constData(), locale.toUtf8().size(), kCFStringEncodingUTF8, false);
    CFDictionarySetValue (dict, CFSTR("Locale"), cfsLocale);

    // Create the directory for the plist inside the game bundle.
    QString path = bundlePath + "/__Installer/OOA/";
    if (!QDir().mkpath(path))
    {
        ORIGIN_LOG_ERROR << "Could not create the path for the plist inside the game bundle: " << path;
    }
    
    // Set the plist path to <game_title>.app/__Installer/OOA/<content_id>.plist
    QString id(content->contentConfiguration()->contentId());
    path += id + ".plist";
    
    // Write the plist file.
    CFStringRef cfsPath = CFStringCreateWithBytes (kCFAllocatorDefault, (const UInt8*)path.toUtf8().constData(), path.toUtf8().size(), kCFStringEncodingUTF8, false);
    CFURLRef fileURL = CFURLCreateWithFileSystemPath (kCFAllocatorDefault, cfsPath, kCFURLPOSIXPathStyle, false);
    CFDataRef xmlData = CFPropertyListCreateXMLData (kCFAllocatorDefault, (CFPropertyListRef)dict);
    SInt32 errorCode;
    if (!CFURLWriteDataAndPropertiesToResource (fileURL, xmlData, NULL, &errorCode))
    {
        ORIGIN_LOG_ERROR << "Could not write the plist file into the game bundle: " << path;
    }
    
    // Release CF resources.
    CFRelease (cfsKey);
    CFRelease (cfsDisplayName);
    CFRelease (cfsLocale);
    CFRelease (cfsPath);
    CFRelease (xmlData);
    CFRelease (dict);
    CFRelease (fileURL);
}
#endif

#ifdef ORIGIN_PC
void ContentServices::setHKeyCurrentUser(const Origin::Engine::Content::EntitlementRef content, bool &isHKCU)
{
    QString cdKey(content->contentConfiguration()->cdKey());
    
    //initialize isHKCU to have a value even if cdKey is Empty
    isHKCU = true;

    if (!cdKey.isEmpty())
    {
        QString baseSubkey(content->contentConfiguration()->stagingKeyPath());
        HKEY hive = HKEY_CURRENT_USER;

        if (baseSubkey.isEmpty())
        {
            baseSubkey = EAD_REG_STAGING_KEY;
            baseSubkey.append("\\");
            baseSubkey.append(content->contentConfiguration()->contentId());
            baseSubkey.append("\\");
        }
        else
        {

            QString subkey, rval;
            if (ParseCMSRegistryString(baseSubkey, hive, rval, subkey))
            {
                baseSubkey = rval;
                baseSubkey.append(subkey);
                baseSubkey.append("\\");
            }

        }

        isHKCU = (hive == HKEY_CURRENT_USER);
        
        //only if its HKCU do we write the reg entry directly
        //otherwise, we use the Export functions to write to the staging.reg file and have
        //the proxy installer import the .reg file so that the reg entries will appear even with UAC on
        if(isHKCU)
        {
            QString key(baseSubkey);
            key.append(EAD_REG_CODE);
            WriteRegistryString(hive, key, "", cdKey);


            key = baseSubkey;
            key.append(EAD_REG_MEMBER_NAME);
            WriteRegistryString(hive, key, "", "");

        }
    }
}

bool ContentServices::ExportStagingKeys()
{
    const QString SPATH   ("REG"); 
    const QString SDIR    ("");
    const QString SVERB   ("open");

    SHELLEXECUTEINFO shellExecuteInfo;
    QString	sRegBranch  ( QString( "HKCU\\" ) + EAD_REG_STAGING_KEY ); // Fully qualified registry branch.
    QString	sCacheDir(Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOAD_CACHEDIR));
    sCacheDir.replace("/", "\\");
    if (!sCacheDir.endsWith("\\"))
    {
        sCacheDir.append("\\");
    }
    QString fullStagingFilePath = sCacheDir + SREGFILE;
    QString	sCmdLineArgs(QString( "EXPORT \"" ) + sRegBranch + "\" \"" + fullStagingFilePath + "\""); // For REG.

    memset(&shellExecuteInfo, 0, sizeof(shellExecuteInfo));
    shellExecuteInfo.cbSize		  = sizeof( SHELLEXECUTEINFO );
    shellExecuteInfo.fMask		  = SEE_MASK_NOCLOSEPROCESS; // Don't close the process.
    shellExecuteInfo.hwnd		  = 0;
    shellExecuteInfo.lpVerb		  = SVERB.utf16();
    shellExecuteInfo.lpFile		  = SPATH.utf16();
    shellExecuteInfo.lpParameters = sCmdLineArgs.utf16();
    shellExecuteInfo.lpDirectory  = SDIR.utf16();
    shellExecuteInfo.hProcess	  = 0;
    shellExecuteInfo.nShow		  = SW_HIDE;

    QFile staging(fullStagingFilePath);
    if (staging.exists())
    {
        if (staging.remove() == false)
        {
            GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("ContentServices_ExportStagingKeys", fullStagingFilePath.toUtf8().data(), staging.error());
        }
    }

    if  (!::ShellExecuteEx(&shellExecuteInfo))
    {
        return false;
    }

    DWORD result = WAIT_OBJECT_0;
    if (shellExecuteInfo.hProcess)
    {
        result = ::WaitForSingleObject(shellExecuteInfo.hProcess, 10000); // Wait or timeout
        ::CloseHandle(shellExecuteInfo.hProcess);	// Cleanup.
    }

    return result == WAIT_OBJECT_0;
}

void ContentServices::setContentHKey(const Origin::Engine::Content::EntitlementRef content)
{
    if (content->contentConfiguration()->stagingKeyPath().isEmpty())
    {
        ORIGIN_LOG_ERROR << "stagingKeyPath is empty";
        return;
    }

    //stagingKeyPath will have [ ] around it, so we need to parse out what's in between
    //and pass that to WriteRegistryString
    QString baseSubkey(content->contentConfiguration()->stagingKeyPath());
    HKEY hive = HKEY_CURRENT_USER;

    QString subkey, rval;
    if (ParseCMSRegistryString(baseSubkey, hive, rval, subkey))
    {
        baseSubkey = rval;
        baseSubkey.append(subkey);
        baseSubkey.append("\\");
    }
    else
    {
        ORIGIN_LOG_ERROR << "Failure to parse stagingKeyPath: " << content->contentConfiguration()->stagingKeyPath();
    }

    ORIGIN_ASSERT(hive);

    if (NULL==hive)
        return;

    baseSubkey.append(EAD_REG_CODE);
    WriteRegistryString(hive, baseSubkey, "", content->contentConfiguration()->cdKey());
}

void ContentServices::setHKeyLocalMachine(const Origin::Engine::Content::EntitlementRef content, const QString& locale)
{
    //MY: Export keys needed by EA Access
    //since WriteRegistryString fails for HKLM keys, we can't export them the same way as the regular staging keys
    //so just append the registry entries manually to staging.reg which is created in ExportStagingKeys
    //that is what will get passed to the ProxyInstaller to so it can run reg edit in elevated mode

    //Software\Electronic Arts\<content id>
    QString base_subkey (EAD_REG_ACCESS_STAGING_KEY);

    QString sActualContentID(content->contentConfiguration()->contentId());

    base_subkey.append("\\");
    base_subkey.append(sActualContentID);
    base_subkey.append("\\");

    //Add DisplayName to the registry
    QString sValueName (EAD_REG_DISPLAY_NAME); //DisplayName
    //TODO: Grab the localized display name
    QString sDataString(content->contentConfiguration()->displayName());
    // The DataString needs to have double slashes for directory separators
    sDataString.replace("\\", "\\\\");
    WriteRegistryString(HKEY_LOCAL_MACHINE, base_subkey, sValueName, sDataString);

    //Add Locale to the registry
    sValueName = QString (EAD_REG_LOCALE);
    WriteRegistryString(HKEY_LOCAL_MACHINE, base_subkey, sValueName, locale);
}

namespace internal_util
{
    int openOrCreateKeyForWriting(const HKEY& predefKey, const QString& subKeyName, HKEY& key)
    {
        int iResult = RegOpenKeyEx(
                predefKey
            , subKeyName.toStdWString().c_str()
            , 0
            , KEY_ALL_ACCESS|KEY_WOW64_32KEY
            , &key);

        if (ERROR_SUCCESS != iResult) // key doesn't exist? Try creating it.
        {
            DWORD disposition = 0;
            iResult = RegCreateKeyEx(
                predefKey,
                subKeyName.toStdWString().c_str(),
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS|KEY_WOW64_32KEY,
                NULL,
                &key,
                &disposition
                );
    
            ORIGIN_LOG_EVENT << "RegCreateKeyEx [" << iResult << " : " << disposition << "]";
        }
        return iResult ;
    }
}
    
bool ContentServices::WriteRegistryString(const HKEY preDefKey, const QString& subkey, const QString& valueName, const QString& dataString)
{
    // Current user keys can be written directly by Origin (since they won't require privileges)
    if (preDefKey == HKEY_CURRENT_USER)
    {
        ORIGIN_LOG_EVENT << "WriteRegistryString (HKCU; key: " << subkey << " value: " << (valueName.isEmpty() ? QString("(default)") : dataString) << " data: " << dataString << ")";
        
        HKEY key = NULL;
        int iResult = internal_util::openOrCreateKeyForWriting(preDefKey, subkey, key);
        if (ERROR_SUCCESS != iResult)
        {
            ORIGIN_LOG_ERROR << "Unable to create/open HKCU registry key.";
            return false;
        }

        iResult = RegSetValueEx(key, valueName.toStdWString().c_str(), 0, REG_SZ, (BYTE*) dataString.toStdWString().c_str(), wcslen(dataString.toStdWString().c_str()) * sizeof(wchar_t));

        // Clean up
        RegCloseKey(key);

        if (ERROR_SUCCESS != iResult)
        {
            ORIGIN_LOG_ERROR << "Unable to write HKCU registry key.";
            return false;
        }

        ORIGIN_LOG_EVENT << "Successfully wrote registry key [HKCU\\" << subkey << "\\" << valueName << "] : '" << dataString << "'";

        return true;
    }
    else
    {
        QString escalationReasonStr = "WriteRegistryString (key: " + subkey + " value: " + valueName + ")";

        int escalationError = 0;
        QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;
        if (!Origin::Escalation::IEscalationClient::quickEscalate(escalationReasonStr, escalationError, escalationClient))
            return false;

        int commandResult = escalationClient->setRegistryStringValue(Origin::Escalation::IEscalationClient::RegSz, preDefKey, subkey, valueName, dataString);
        return Origin::Escalation::IEscalationClient::evaluateEscalationResult(escalationReasonStr, "setRegistryStringValue", commandResult, escalationClient->getSystemError());
    }
}

bool ContentServices::WriteRegistryStringEx(const HKEY preDefKey, const QString& subkey, const wchar_t* valueName, QString dataString)
{
    /*bool	rval = false;

    // Create a file with the key inside
    QString	sPreDef;
    switch((ULONG_PTR)preDefKey)
    {
    case HKEY_LOCAL_MACHINE : sPreDef = "HKEY_LOCAL_MACHINE"; break;
    case HKEY_CURRENT_USER  : sPreDef = "HKEY_CURRENT_USER"; break;
    case HKEY_CLASSES_ROOT  : sPreDef = "HKEY_CLASSES_ROOT"; break;
    case HKEY_USERS         : sPreDef = "HKEY_USERS"; break;
    }

    // ValueName
    QString sValueName;
    // Add the field name
    if (wcslen(valueName) > 0)
        sValueName = QString::fromWCharArray(valueName); 
    else
        sValueName = "@";

    // Create the registry contents
    QString	fileData;
    // The DataString needs to have double slashes for directory separators
    QString sDataString = dataString;
    sDataString.replace("\\", "\\\\");
    fileData = QString("Windows Registry Editor Version 5.00\n\n[%1\\%2]\n\"%3\"=\"%4\"\n")
        .arg(sPreDef).arg(subkey).arg(sValueName).arg(sDataString);


    // Dump the registry data out to a file
    QString sFilename;
    wchar_t logDirPath[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_DEFAULT, logDirPath);

    sFilename = QString::fromWCharArray(logDirPath);

    // Create a temp name, just in case of clashes with other users and multi-threading
    wchar_t	*tmpName = L"\\regtemp_XXXXX";
    // why doesn't this work?
    //_wmktemp_s (tmpName, wcslen(tmpName)+1);
    sFilename.append(QString::fromWCharArray(tmpName));
    sFilename.append(".reg");


    // Open the file and write the registry instructions
    File tempRegFile(sFilename);
    if( tempRegFile.open(QIODevice::WriteOnly) )
    {
        tempRegFile.write((const char*)fileData.utf16(), fileData.length());
        tempRegFile.close();
    }
    else
    {
#ifdef _DEBUG
        QString message;
        message = QString("Can not open file %1 for creation/writing.").arg(sFilename);
        MessageBox(NULL, message.utf16(), L"WriteRegistryString", MB_OK);
#endif
        return rval;
    }

    // Execute: regedit /s <filename>
    QString	command ( "regedit.exe" );
    wchar_t	dirPath[MAX_PATH];
    QString sParams;
    if (GetCurrentDirectory(MAX_PATH, dirPath) != 0)
    {
        sParams = QString("/s \"%1\"").arg(sFilename);
        CoreProcess_Windows::OpenPath(command, sParams, ".", "", false);
        int totalSleep = 5000;

        // ensure the registry entry is recorded before deleting the instructions (or wait n seconds)
        while (totalSleep > 0 && ReadRegistryString(PreDefKey, SubKey, QString::fromWCharArray(ValueName)) != DataString )
        {
            Sleep(100);
            totalSleep -= 100;
        }

        rval = totalSleep > 0;
    }
    QFile::remove(sFilename);

    return rval;*/

        return false;
}

bool ContentServices::ParseCMSRegistryString(const QString& sCMSRegistryString, HKEY &hHiveKey, QString &sRegistryPath, QString &sSubKey)
 {
        bool	rval = false;

        sRegistryPath.clear();
        hHiveKey = HKEY_CURRENT_USER;

        if (sCMSRegistryString.isEmpty())
        {
            return rval;
        }

        int nStartBracketPos = sCMSRegistryString.indexOf('[');
        if (nStartBracketPos != 0)
        {
            //CORE_ASSERT(false);
            return rval;
        }

        int nEndBracketPos = sCMSRegistryString.indexOf(']', nStartBracketPos);
        if (nEndBracketPos == -1)
        {
            //CORE_ASSERT(false);
            return rval;
        }

        QString sToken = sCMSRegistryString.mid(1, nEndBracketPos-1);

        if (sToken.indexOf("HKEY") > 0 || sToken.indexOf("KEY") > 0) // parse reg key token if token is [HKEY...]
        {
            QString sSubKeyPath = sToken;

            // get rid of double backslashes (CMS has some entries like this)
            sSubKeyPath.replace("\\\\", "\\");

            // get rid of fwd slashes
            sSubKeyPath.replace("/", "\\");

            /*
             from HKEY_LOCAL_MACHINE\\SOFTWARE\\Electronic Arts\\EA Games\\Battlefield 2\\InstallDir ,
             we would get hHiveKey = HKEY_LOCAL_MACHINE
                            sRegKeySubPath = SOFTWARE\\Electronic Arts\\EA Games\\Battlefield 2\\
                            sKey = InstallDir
            */
            int nFirstSlashPos = sSubKeyPath.indexOf('\\');
            int nLastSlashPos = sSubKeyPath.lastIndexOf('\\');

            QString sKeyType = sSubKeyPath.left(nFirstSlashPos); // e.g. HKEY_LOCAL_MACHINE
            sRegistryPath	= sSubKeyPath.mid(nFirstSlashPos + 1, nLastSlashPos - nFirstSlashPos);
            sSubKey			= sSubKeyPath.mid(nLastSlashPos + 1);

            if (  (sKeyType == "HKEY_LOCAL_MACHINE") 
                || (sKeyType == "KEY_LOCAL_MACHINE"))
            {
                hHiveKey = HKEY_LOCAL_MACHINE;
                rval = true;
            }
            else if (   (sKeyType == "HKEY_CURRENT_USER") 
                || (sKeyType == "KEY_CURRENT_USER"))
            {
                hHiveKey = HKEY_CURRENT_USER;
                rval = true;
            }
            else
            {
                ORIGIN_LOG_ERROR << L"ParseCMSRegistryToken: Unsupported reg key type " << sKeyType;
            }
        }
        else // otherwise try to replace EA token
        {
            sRegistryPath = QString("<%1>").arg(sToken);
            //if (! CoreTokenManager::GetSingleton()->ReplaceTokens(sRegistryPath))
            //{
                //EBILOGERROR << L"ParseCMSRegistryString: failed to parse token " << sToken;
                //return rval;
            //}
        }

        return rval;
    } // ParseCMSRegistryString

#endif // ORIGIN_PC   
} // namespace Downloader
} // namespace Origin

