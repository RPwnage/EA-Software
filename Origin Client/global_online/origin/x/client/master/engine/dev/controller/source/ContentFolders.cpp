#include "ContentFolders.h"

#include "services/settings/SettingsManager.h"
#include "engine/content/ContentController.h"
#include <EAStdC/EAString.h>

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

#ifdef ORIGIN_PC    
#include <shlobj.h>
#endif

#include "services/escalation/IEscalationClient.h"
#include "TelemetryAPIDLL.h"

using namespace Origin::Engine;

// The maximum path length against which path lengths are checked.
// This should be less than the max path available on each supported platform.
#define MAXIMUM_PATH_LENGTH 260

namespace Origin
{
    namespace Engine
    {
        namespace Content
        {
            static const int kValidateIntervalTimeoutMs = 5000;
            
            namespace Detail
            {
                // useful values
                static const uchar MAX_CHAR = 126;
                static const uchar MIN_CHAR = 32;
                
                ///
                /// returns true if the char is not valid ASCII and false otherwise
                inline bool isAscii(const char chr)
                {
                    return chr <= MAX_CHAR && chr >= MIN_CHAR;
                }

                /// \fn containsSymbols(const QString& str)
                /// \brief Detects if the passed string contains symbols.
                ///
                /// \param str The string to test.
                /// \return bool Returns true if symbols are found.
                static bool containsSymbols(const QString& str)
                {
                    bool isContainSymbols = false;
#if defined(ORIGIN_PC)
                    for ( int i = 0; i < str.length(); i++)
                    {
                        if ( str.at(i).isSymbol() || !isAscii(str.at(i).toLatin1()) )
                        {
                            isContainSymbols = true;
                            break;
                        }
                    }
#elif defined(ORIGIN_MAC)
                    // disallow non-ASCII characters for the time being - we'll see for how long
                    for ( int i = 0; i < str.length(); i++)
                    {
                        if ( str.at(i).isSymbol() || !isAscii(str.at(i).toLatin1()) )
                        {
                            isContainSymbols = true;
                            break;
                        }
                    }
#else
#error "Determine platform-specific path symbol restrictions"
#endif
                    if (isContainSymbols)
                    {
                        // Telemetry for recording when users try to use paths that contain non-ASCII characters, or symbols.
                        // Server-side telemetry reporting can not display any symbols or non-ASCII characters from attribute strings.
                        // The best we can do is to URL percent encode the string, so that it will be possible for someone who wishes to
                        // analyze the invalid paths to decode them from the telemetry reports.  The encoding's percent signs will appear
                        // as underscores in these reports.
                        GetTelemetryInterface()->Metric_INVALID_SETTINGS_PATH_CHAR((const char*)(QUrl::toPercentEncoding(str, "", "-._~:/?#[]@!$&'()*+,;=")));
                    }
                    
                    return isContainSymbols;
                }
            } // namespace Detail
            
#if defined(ORIGIN_PC)
            static const size_t kPathCapacity = 4096;
#endif
            ContentFolders::ContentFolders(QObject *parent)
                :QObject(parent),
                mSuppressFolderValidationError(false),
                mAsyncFolderCheckIsRunning(false)
            {
                mInstallerCacheLocation = Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOAD_CACHEDIR).toString();
                mDownloadInPlaceLocation = Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOADINPLACEDIR).toString();
                mRemoveOldCache = Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOAD_CACHEDIRREMOVAL);

                mDownloadInPlaceFolderValid = validateDIPFolder(mDownloadInPlaceLocation);
                mIsCacheFolderValid = validateCacheFolder(mInstallerCacheLocation);

                ORIGIN_VERIFY_CONNECT(
                    Origin::Services::SettingsManager::instance(), SIGNAL(settingChanged(const QString&, const Origin::Services::Variant&)),
                    this, SLOT(onSettingChanged(const QString&, const Origin::Services::Variant&)));
                
                ORIGIN_VERIFY_CONNECT(
                    &mValidateFoldersTimer, 
                    SIGNAL(timeout()),
                    this,
                    SLOT(onValidateFoldersTimeout()));

                mValidateFoldersTimer.setInterval(kValidateIntervalTimeoutMs);
                mValidateFoldersTimer.start();

            }

            ContentFolders::~ContentFolders()
            {
                mValidateFoldersTimer.stop();
                QMutexLocker lock(&mThreadRunningMutex);
            }

            void ContentFolders::onValidateFoldersTimeout()
            {
                if (!mAsyncFolderCheckIsRunning)
                {
                    ValidateFoldersAsync* validateAsync = new ValidateFoldersAsync(this);
                    validateAsync->setAutoDelete(true);
                    QThreadPool::globalInstance()->start(validateAsync);
                }
            }

            bool ContentFolders::verifyMountPoint(const QString &path)
            {
#ifndef ORIGIN_MAC
                // we only need to verify network folder mount points on Mac
                return true;
#else
                if (!path.startsWith("/Volumes/"))
                {
                    // all Mac network mount points will be under /Volumes
                    return true;
                }
                if (!QDir(path).exists())
                {
                    // if the Mac network folder doesn't exist, we assume that it is not valid (probably unmounted).
                    // if we don't make this assumption, then we run into a tricky problem: when a network folder
                    // is unmounted, the mount point folder is removed...but we are still allowed to re-create the
                    // mount point folder; it just doesn't actually map to a network drive.  to avoid this confusion
                    // we just won't force create Mac network drive folders...
                    // note: we have already established that we are on a Mac, and the path begins with /Volumes/
                    return false;
                }
                
                // check if the network folder is truly writeable.  EBIBUGS-18578.
                QFile testFile(path + QDir::separator() + "mountTestFile");
                bool res = testFile.open(QIODevice::ReadWrite);
                QDir(path).remove("mountTestFile");
                return res;
#endif
            }
            
            void ContentFolders::periodicFolderValidation()
            {
                QMutexLocker lock(&mThreadRunningMutex);
                
                // Just going to do simple exists check if we are already valid
                if(mDownloadInPlaceFolderValid)
                {
                    QFileInfo dipLocation(mDownloadInPlaceLocation);
                    mDownloadInPlaceFolderValid = (dipLocation.isDir() && dipLocation.isWritable() && verifyMountPoint(mDownloadInPlaceLocation));
                }
                
                if(!mDownloadInPlaceFolderValid && !mDownloadInPlaceLocation.isEmpty())
                {
                    // We supress error signals during this periodic check because
                    // it runs continously and if the SettingsDialog is open, the errors that are
                    // emitted causes error dialogs to be raised over and over
                    mSuppressFolderValidationError = true;
                    mDownloadInPlaceFolderValid = validateDIPFolder(mDownloadInPlaceLocation);
                    mSuppressFolderValidationError = false;
                }
                
                if(mIsCacheFolderValid)
                {
                    QFileInfo cacheLocation(mInstallerCacheLocation);
                    mIsCacheFolderValid = (cacheLocation.isDir() && cacheLocation.isWritable() && verifyMountPoint(mInstallerCacheLocation));
                }
                
                if(!mIsCacheFolderValid && !mInstallerCacheLocation.isEmpty())
                {
                    // We supress error signals during this periodic check because
                    // it runs continously and if the SettingsDialog is open, the errors that are
                    // emitted causes error dialogs to be raised over and over
                    mSuppressFolderValidationError = true;
                    mIsCacheFolderValid = validateCacheFolder(mInstallerCacheLocation);
                    mSuppressFolderValidationError = false;
                }
            }

            void ContentFolders::setRemoveOldCache(bool s) 
            { 
                mRemoveOldCache = s; 

                // do not set it yet, it is set if the cacheDir is changed!
            }

            void ContentFolders::setInstallerCacheLocation(const QString &newLocation, bool isDefaultCache)
            { 
                mInstallerCacheLocation = newLocation;
                QString pTempCacheLocation = mInstallerCacheLocation;
                const QString slashCache = QDir::separator() + QString("cache");

                // We used to add the cache tag to the UI layer but it does not need to know that there is an extra 
                // folder so that the user can have it deleted without losing any other data so add it here instead
                if (!isDefaultCache && mInstallerCacheLocation.endsWith(slashCache, Qt::CaseInsensitive) == false)
                {
                    pTempCacheLocation += slashCache;
                    
                    // In most cases, the new /cache folder will be created the first time it is needed.  But there
                    // is at least one exception: on Mac, if it is a network drive folder and it does not exist,
                    // then we have to assume that is has been unmounted, so we will not create it.  So let's
                    // go ahead and make this folder now to avoid dealing with edge cases like that one.
                    // EBIBUGS-022105
                    createFolderElevated (pTempCacheLocation);
                }

                // do not send CacheDirRemoval && CacheDir separately via "WriteSettings", because is reset automatically by the middleware for every settings change event! 

                Origin::Services::writeSetting(Origin::Services::SETTING_DOWNLOAD_CACHEDIR, pTempCacheLocation);
                Origin::Services::writeSetting(Origin::Services::SETTING_DOWNLOAD_CACHEDIRREMOVAL, mRemoveOldCache);
                mIsCacheFolderValid = true;
            }

            void ContentFolders::setDownloadInPlaceLocation(const QString &newLocation)
            { 
                QString location = newLocation;
                if (mDownloadInPlaceLocation != newLocation)
                {
                    //this function will automatically move the "program files" location to the "program files(x86)" location
                    // on 64-bit OS -- this function does nothing on Mac and does nothing if its a 32 bit system
                    adjustFolderFor32bitOn64BitOS(location);
           
                    Origin::Services::writeSetting(Origin::Services::SETTING_DOWNLOADINPLACEDIR, location);
                    mDownloadInPlaceFolderValid = true;
                }
            }

            // Slots

            void ContentFolders::onSettingChanged(const QString& settingName, const Origin::Services::Variant& value)
            {
                mRemoveOldCache = false;    // this is false by default & has to be set every time we change the cache dir

                if(settingName == Origin::Services::SETTING_DOWNLOAD_CACHEDIR.name())
                {
                    mInstallerCacheLocation = value.toString();
                    mIsCacheFolderValid = true;

                    if(mDownloadInPlaceFolderValid)
                    {
                        emit(contentFoldersValid());
                    }
                }
                else if (settingName == Origin::Services::SETTING_DOWNLOADINPLACEDIR.name())
                {
                    mDownloadInPlaceLocation = value.toString();
                    mDownloadInPlaceFolderValid = true;

                    if(mIsCacheFolderValid)
                    {
                        emit(contentFoldersValid());
                    }
                }
            }

            bool ContentFolders::validateFolder(const QString& folder, bool isCacheFolder /*=false*/)
            {
                // This function might be called from multiple threads...
                QMutexLocker locker(&mValidateFolderMutex);
                
                if (!checkFolders(folder, isCacheFolder))
                {
                    emit validateFailed();
                    return false;
                }
                else if (Detail::containsSymbols(folder))
                {
                    emit validateFailed();
                    
                    if(!mSuppressFolderValidationError)
                    {
                        emit folderValidationError(FolderErrors::CHAR_INVALID);
                    }
                    return false;
                }
                return true;
            }

            bool ContentFolders::validateDIPFolder(const QString & dip)
            {
                return validateFolder(dip);
            }

            bool ContentFolders::validateCacheFolder(const QString & cache)
            {
                return validateFolder(cache, true);
            }


            bool ContentFolders::validateAllFolders(const QString &dipFolder, const QString &cacheFolder)
            {
                // does all checks but does not show a UI
                // the only reason we got here is because if we write an invalid path
                // into the QLineEdit, then click the close button on the top left, UI fires two events - close & editchangned
                // we only want editchanged to show error dialogs and close should be ignored silently

                bool status;

                status = validateCacheFolder(cacheFolder);

                if (status)
                    status = validateDIPFolder(dipFolder);

                if (status)
                    // as a final validation make sure that the paths do not contain invalid characters
                    status = FolderErrors::ERROR_NONE == validatePaths();

                return status;
            }

            bool ContentFolders::createFolderElevated(const QString & sDir )
            {
                return Services::PlatformService::createFolderElevated(sDir, "D:(A;OICI;GA;;;WD)");
            }

            // do not call checkFolders directly, use the wrapper above
            bool ContentFolders::checkFolders(const QString & cleanFolder, const bool isCacheFolder)
            {
                if(cleanFolder.isEmpty())
                    return false;

                const FolderErrors::ErrorMessages invalidCacheOrDip = (isCacheFolder) ? FolderErrors::CACHE_INVALID : FolderErrors::DIP_INVALID;

                // check if this is an invalid Mac network folder mount point.  EBIBUGS-18578.
                if (!verifyMountPoint(cleanFolder))
                {
                    if(!mSuppressFolderValidationError)
                    {
                        emit folderValidationError(invalidCacheOrDip);
                    }
                    return false;
                }
                
                QDir cleanFolderDir = cleanFolder;

                if (cleanFolderDir.isRoot())
                {
                    if(!mSuppressFolderValidationError)
                    {
                        emit folderValidationError(invalidCacheOrDip);
                    }
                    return false;
                }

                if (!cleanFolderDir.exists())
                {
                    if (!createFolderElevated(cleanFolder))
                    {
                        if(!mSuppressFolderValidationError)
                        {
                            emit folderValidationError(invalidCacheOrDip);
                        }
                        return false;
                    }
                }

                // Check that the directory is not the same.
                const QString cacheOrDIPSettingFolder = isCacheFolder ?
                    Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOADINPLACEDIR, Origin::Services::Session::SessionService::currentSession()) : 
                    Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOAD_CACHEDIR, Origin::Services::Session::SessionService::currentSession());

                if(cleanFolder.compare(cacheOrDIPSettingFolder, Qt::CaseInsensitive) == 0)
                {
                    if(!mSuppressFolderValidationError)
                    {
                        emit folderValidationError(FolderErrors::FOLDERS_HAVE_SAME_NAME);
                    }
                    return false;
                }

                // Check that the provided dir doesn't match any of the forbidden dirs.
                QStringList forbiddenDirs;
                getForbiddenDirs(forbiddenDirs);
                if (forbiddenDirs.contains(cleanFolder))
                {
                    if (!mSuppressFolderValidationError)
                    {
                        emit folderValidationError(invalidCacheOrDip);
                    }
                    return false;
                }

                // Some of the forbidden directories do not allow using any subdirectories below them as well.  This is the case for all of the Mac forbidden dirs.
                // Check if the folder path begins with one of these forbidden dirs (forbiddenSubDirs).
                QStringList forbiddenSubDirs;
                getForbiddenSubDirs(forbiddenSubDirs);
                
#ifdef ORIGIN_PC
                for (int i = 0; i < forbiddenSubDirs.size(); i++)
                {
                    if (cleanFolder.startsWith(forbiddenSubDirs[i], Qt::CaseInsensitive))
                        {
                            if (!mSuppressFolderValidationError)
                            {   
                                emit folderValidationError(invalidCacheOrDip);
                            }
                            return false;
                        }
                }
#elif defined(ORIGIN_MAC)
                // We need ensure that cleanFolder ends with a slash on Mac for the forbiddenSubDirs check, but since it is a const &, we'll need another variable
                QString cleanFolderMac(cleanFolder);
                if (!cleanFolderMac.endsWith(QDir::separator()))
                {
                    cleanFolderMac.append(QDir::separator());
                }
                
                for (int i = 0; i < forbiddenSubDirs.size(); i++)
                {
                    if (cleanFolderMac.startsWith(forbiddenSubDirs[i], Qt::CaseInsensitive))
                    {
                        // On Mac, we allow the cache directory to be a subdirectory of /Library/
                        if (isCacheFolder && (forbiddenSubDirs[i] == "/Library/"))
                        {
                            continue;
                        }
                        if (!mSuppressFolderValidationError)
                        {   
                            emit folderValidationError(invalidCacheOrDip);
                        }
                        return false;
                    }
                }
#endif // ORIGIN_MAC
                
#ifdef ORIGIN_PC
                // Awful check for MSOCache dir.
                static const QString MSOCacheDir = ":\\MSOCache";
                if(cleanFolder.endsWith(MSOCacheDir, Qt::CaseInsensitive) || cleanFolder.contains(MSOCacheDir + "\\", Qt::CaseInsensitive))
                {
                    if(!mSuppressFolderValidationError)
                    {
                        emit folderValidationError(invalidCacheOrDip);
                    }
                    return false;
                }

                // mark ";" as an illegal character for the DIP folder,
                // because it breaks DLL loading for some games
                if (!isCacheFolder && cleanFolder.contains(";"))
                {
                    if(!mSuppressFolderValidationError)
                    {
                        emit folderValidationError(invalidCacheOrDip);
                    }
                    return false;
                }

                //check if system folder
                QDir sysCheckFolderPath = cleanFolderDir;

                //    Win32:  Check if folder is in the c:\windows\* folder
                char16_t buffer[kPathCapacity + 1] = {0};
                if (GetEnvironmentVariableW(L"windir", buffer, kPathCapacity) != 0)
                {
                    QString windowsDirectoryEnvVar(QString::fromUtf16(buffer));
                    if (cleanFolder.startsWith(windowsDirectoryEnvVar, Qt::CaseInsensitive) == true)
                    {
                        if(!mSuppressFolderValidationError)
                        {
                            emit folderValidationError(invalidCacheOrDip);
                        }
                        return false;
                    }
                }

                //make sure that a system folder is not a part of the path
                while(!sysCheckFolderPath.isRoot())
                {
                    QString pathname = sysCheckFolderPath.path();

                    char16_t folderToCheck[kPathCapacity+1] = {0};
                    pathname.toWCharArray(folderToCheck);
                    const DWORD attributes = GetFileAttributes(folderToCheck);

                    if (attributes == INVALID_FILE_ATTRIBUTES)
                    {
                        if(!mSuppressFolderValidationError)
                        {
                            emit folderValidationError(invalidCacheOrDip);
                        }
                        return false;    // prevent an endless loop for invalid/non-existing folder/drives !!!
                    }

                    if (attributes & FILE_ATTRIBUTE_SYSTEM)
                    {
                        if(!mSuppressFolderValidationError)
                        {
                            emit folderValidationError(invalidCacheOrDip);
                        }
                        return false;
                    }

                    //move up the directory path
                    sysCheckFolderPath.cdUp();
                }
                
                //check if root is cd room
                if (isCdRom(sysCheckFolderPath))
                {
                    if(!mSuppressFolderValidationError)
                    {
                        emit folderValidationError(invalidCacheOrDip);
                    }
                    return false;
                }
#endif //ORIGIN_PC
#ifdef ORIGIN_MAC
                //check if root is cd room
                if (isCdRom(cleanFolderDir))
                {
                    if(!mSuppressFolderValidationError)
                    {
                        emit folderValidationError(invalidCacheOrDip);
                    }
                    return false;
                }
#endif // ORIGIN_MAC

                // Kind of hacky... The UI does not know that we append \cache to our cache directory so sometimes a path that it thinks is not too long
                // will get checked by Core which returns that it was too long, causing tons of issues. All of this setting stuff really needs to be cleaned up
                bool endsWithSeparator = cleanFolder.endsWith(QDir::separator());
                const QString realFolder = cleanFolder + (endsWithSeparator ? "cache" : "/cache");
                if(realFolder.size() > (MAXIMUM_PATH_LENGTH - 160))
                {
                    if(!mSuppressFolderValidationError)
                    {
                        emit folderValidationError(isCacheFolder ? FolderErrors::CACHE_TOO_LONG : FolderErrors::DIP_TOO_LONG);
                    }
                    return false;
                }

                if(isCacheFolder)
                {
                    return verifyWritableCache(cleanFolder);
                }

                return true;
            }

            // There was no good reason for forbidding most of these directories.  The best explanation I could find was 
            // "There may be bugs when we try to uninstall stuff, so just to make sure we don't accidently hurt any 'nearby' files..."
            void ContentFolders::getForbiddenDirs(QStringList &forbiddenDirs)
            {
#ifdef ORIGIN_PC
                // Reference: http://msdn.microsoft.com/en-us/library/ms683188(v=vs.85).aspx
                // An environment variable has a maximum size limit of 32,767 characters, including the null-terminating character.
                // 4096 is still quite conservative. JRivero
                char16_t     _osDir[kPathCapacity+1] = {0};
                char16_t     _appDir[kPathCapacity+1] = {0};
                char16_t     _appDirX64[kPathCapacity+1] = {0};
                char16_t     _desktopDir[kPathCapacity+1] = {0};
                char16_t     _programDataDir[kPathCapacity+1] = {0};
                char16_t     _userDir[kPathCapacity+1] = {0};
                char16_t     _perfLogsDir[kPathCapacity+1] = {0};
                char16_t     _bootDir[kPathCapacity+1] = {0};

                GetEnvironmentVariableW( L"ProgramFiles", _appDir, kPathCapacity );
                GetEnvironmentVariableW( L"ProgramW6432", _appDirX64, kPathCapacity );
                GetEnvironmentVariableW( L"windir", _osDir, kPathCapacity );
                GetEnvironmentVariableW( L"SystemDrive", _perfLogsDir, kPathCapacity );
                GetEnvironmentVariableW( L"SystemDrive", _bootDir, kPathCapacity );
                SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, _desktopDir);
                SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, _programDataDir);
                SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, _userDir);

                // Hardcode check for the MSOCacheDir at the root any drive
                const QString perfLogsDir = (EA::StdC::Strlen(_perfLogsDir) > 0) ? QString::fromWCharArray(_perfLogsDir).append("\\PerfLogs") : QString("");
                const QString bootDir = (EA::StdC::Strlen(_bootDir) > 0) ? QString::fromWCharArray(_bootDir).append("\\Boot") : QString("");

                forbiddenDirs.push_back(QString::fromWCharArray(_desktopDir));
                forbiddenDirs.push_back(QString::fromWCharArray(_appDir));
                forbiddenDirs.push_back(QString::fromWCharArray(_appDirX64));
                forbiddenDirs.push_back(QString::fromWCharArray(_osDir));
                forbiddenDirs.push_back(QString::fromWCharArray(_programDataDir));
                forbiddenDirs.push_back(QString::fromWCharArray(_userDir));
                forbiddenDirs.push_back(perfLogsDir);
                forbiddenDirs.push_back(bootDir);
#endif // ORIGIN_PC
            }

            // Some of the forbidden directories do not allow using any subdirectories below them as well.  This is the case for all of the Mac forbidden dirs.
            void ContentFolders::getForbiddenSubDirs(QStringList &forbiddenSubDirs)
            {
#ifdef ORIGIN_MAC
                forbiddenSubDirs << ("/.DocumentRevisions-V100/");
                forbiddenSubDirs << ("/.Spotlight-V100/");
                forbiddenSubDirs << ("/.Trashes/");
                forbiddenSubDirs << ("/.vol/");
                forbiddenSubDirs << ("/Applications/Utilities/");
                forbiddenSubDirs << ("/Library/");
                forbiddenSubDirs << ("/Network/");
                forbiddenSubDirs << ("/System/");
                forbiddenSubDirs << ("/bin/");
                forbiddenSubDirs << ("/cores/");
                forbiddenSubDirs << ("/dev/");
                forbiddenSubDirs << ("/home/");
                forbiddenSubDirs << ("/net/");
                forbiddenSubDirs << ("/private/");
                forbiddenSubDirs << ("/sbin/");
                forbiddenSubDirs << ("/usr/");
#endif // ORIGIN_MAC
            }

            bool ContentFolders::adjustFolderFor32bitOn64BitOS(QString &folderName)
            {
                bool adjusted = false;
#ifdef Q_OS_WIN
                char16_t     _appDirX64[kPathCapacity+1] = {0};
                char16_t     _appDir[kPathCapacity+1] = {0};
                //if this environment var exists, it means we are on a 64 bit system that has a Program Files folder for 64 bit apps
                //we currently do not allow a user to install to this folder because the windows installer doesn't allow 32 bit programs to be installed
                //in the Program Files folder on 64 bit machines
                //addresses this bug: https://developer.origin.com/support/browse/EBIBUGS-21961

                DWORD appDirlength = GetEnvironmentVariableW( L"ProgramFiles", _appDir, kPathCapacity );
                DWORD appDirX64length = GetEnvironmentVariableW( L"ProgramW6432", _appDirX64, kPathCapacity );

                //check if we got a valid "ProgramW6432" env var, if so, then we make sure we have enough space to add the \\
                //we should always have enough but this is just to make sure we don't overwrite the array
                if((appDirX64length != 0) && (appDirX64length < (kPathCapacity - 2) && (appDirlength < (kPathCapacity - 2))))
                {
                    //if the last char doesn't already have a '\\', add one so that when we do the "startswith" check against this folder
                    // Program Files(x86) won't be a match

                    //save off the string before we add the backslash for checking
                    QString display64String = QString::fromStdWString(_appDirX64);
                    QString display32String = QString::fromStdWString(_appDir);

                    if(_appDirX64[appDirX64length-1] != '\\')
                    {
                        _appDirX64[appDirX64length] = '\\';
                        _appDirX64[appDirX64length+1] = 0;
                    }

                    if(_appDir[appDirlength-1] != '\\')
                    {
                        _appDir[appDirlength] = '\\';
                        _appDir[appDirlength+1] = 0;
                    }

                    if(folderName.startsWith(QString::fromStdWString(_appDirX64), Qt::CaseInsensitive))
                    {
                        adjusted = true;
                        folderName.replace(QString::fromStdWString(_appDirX64), QString::fromStdWString(_appDir));
                        emit programFilesAdjustedFrom64bitTo32bitFolder(display64String, display32String);
                    }
                }
#endif
                return adjusted;
            }

            bool ContentFolders::isCdRom( QDir &sysCheckFolderPath)
            {
#ifdef ORIGIN_PC
                return (DRIVE_CDROM == GetDriveType(sysCheckFolderPath.path().toStdWString().c_str()));
#else
                return Origin::Services::PlatformService::isReadOnlyPath(sysCheckFolderPath.canonicalPath());

#endif // ORIGIN_PC
            }

            bool ContentFolders::verifyWritableCache(const QString &cleanFolder)
            {
                // Make sure the cache file is writable
                const QString writeTest(cleanFolder + QDir::separator() + "writetest");
                QFile checkFile(writeTest);

                if(!checkFile.open(QIODevice::ReadWrite))
                {
                    if(!mSuppressFolderValidationError)
                    {
                        emit folderValidationError(FolderErrors::CACHE_INVALID);
                    }
                    return false;
                }
                else
                {
                    const char * const testString = "teststring";
                    // Ensure they can actually write to the file
                    // It's possible to create an ACL that allows file creation but denies appending
                    if(checkFile.write(testString) != strlen(testString))
                    {
                        if(!mSuppressFolderValidationError)
                        {
                            emit folderValidationError(FolderErrors::CACHE_INVALID);
                        }
                        return false;
                    }

                    if (checkFile.remove() == false)
                    {
                        GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("ContentFolders_verifyWritableCache", writeTest.toUtf8().data(), checkFile.error());
                    }
                }

                return true;
            }

            int ContentFolders::validatePaths()
            {
                QMutexLocker locker(&mValidateFolderMutex);
                
                QString folder = Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOAD_CACHEDIR, Origin::Services::Session::SessionService::currentSession());
                if (Detail::containsSymbols(folder))
                {
                    if(!mSuppressFolderValidationError)
                    {
                        emit folderValidationError(FolderErrors::CHAR_INVALID);
                    }
                    return FolderErrors::CHAR_INVALID;
                }
                QString downloadInPlaceLocation = Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOADINPLACEDIR, Origin::Services::Session::SessionService::currentSession());
                if (Detail::containsSymbols(downloadInPlaceLocation)
#ifdef ORIGIN_PC
                    || downloadInPlaceLocation.contains(";")    // or do we have a forbidden character
                    // in our existing folder?
#endif
                    )
                {
                    if(!mSuppressFolderValidationError)
                    {
                        emit folderValidationError(FolderErrors::CHAR_INVALID);
                    }
                    return FolderErrors::CHAR_INVALID;
                }
                return FolderErrors::ERROR_NONE;
            }
            
            void ContentFolders::setAsyncFolderCheckIsRunning(bool running)
            {
                mAsyncFolderCheckIsRunning = running;
            }
            
            ContentFolders::ValidateFoldersAsync::ValidateFoldersAsync(ContentFolders* parent) :
                mParent(parent)
            {
                
            }
            
            void ContentFolders::ValidateFoldersAsync::run()
            {
                if (mParent)
                {
                    mParent->setAsyncFolderCheckIsRunning(true);
                    mParent->periodicFolderValidation();
                    mParent->setAsyncFolderCheckIsRunning(false);
                }
            }
            
        } // namespace Content
    } // namespace Engine
} // namespace Origin



