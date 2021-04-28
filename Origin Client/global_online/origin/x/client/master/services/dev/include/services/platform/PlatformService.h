//  Platform.h
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#ifndef PLATFORM_H
#define PLATFORM_H

#include <limits>
#include <QDateTime>
#include <QString>
#include <QByteArray>
#include <QMap>
#include <QVector>
#include <QIcon>
#include <QFileIconProvider>
#include <QUrl>
#include <QStandardPaths>
#include <QList>
#include <QSslCertificate>
#include <QWindow>

#include "services/common/VersionInfo.h"
#include "services/plugin/PluginAPI.h"

#ifdef ORIGIN_MAC
#include <errno.h>
#endif
namespace Origin
{
    namespace Services
    {
        namespace PlatformService
        { 
            /// \enum OriginPlatform
            /// \brief These represent the various platforms that Origin supports.
            ///
            /// The special value 'PlatformThis' is used in various Origin APIs to indicate that we would like 
            /// the results of the call to reflect the currently running platform.  In this case, the APIs would
            /// generally then internally call runningPlatform() to determine the current platform.
            ///
            /// The special value 'PlatformOther' is the opposite of 'PlatformThis'.  It will give results for
            /// PC when the running platform is Mac, and vice versa.
            enum OriginPlatform
            {
                PlatformUnknown = -1,
                PlatformThis,
                PlatformWindows,
                PlatformMacOS,
                PlatformOther
            };

            enum OriginProcessorArchitecture
            {
                ProcessorArchitectureUnknown = 0,
                ProcessorArchitecture32bit = 1,
                ProcessorArchitecture64bit = 1<<1,
                ProcessorArchitecture3264bit = ProcessorArchitecture32bit | ProcessorArchitecture64bit
            };
            
            /// \enum BatteryFlag
            /// \brief These represent the different states we have for battery status.
            ///
            enum BatteryFlag
            {
                UsingBattery,
                PluggedIn,
                NoBattery
            };

            /// \enum OriginState
            /// \brief These represent the different states we have for the Origin application.
            ///
            enum OriginState
            {
                Running,
                ShutDown,
                Unknown
            };

            /// \brief Get the currently running Origin platform.
            ///
            /// This function is mainly used by APIs that can accept 'PlatformThis' as a default platform.
            ORIGIN_PLUGIN_API OriginPlatform runningPlatform();

            /// \brief Resolve PlatformThis and PlatformOther to their proper platform.
            ///
            /// This function is mainly used by APIs that cannot accept 'PlatformThis' as a default platform.
            ORIGIN_PLUGIN_API OriginPlatform resolvePlatform(OriginPlatform platform);

            /// \brief Get the currently known Origin platforms and their string suffixes.
            ///
            /// This function is used mainly by the entitlement parsing APIs
            ORIGIN_PLUGIN_API QMap<OriginPlatform, QString> knownPlatformSuffixes();

            /// \brief Get the supported Origin platforms from an encoded string
            ///
            /// This function is used mainly by the entitlement parsing APIs.  Parses strings like 'pc', 'pcwin', 'mac/pc', etc.
            ORIGIN_PLUGIN_API QVector<OriginPlatform> supportedPlatformsFromCatalogString(const QString& catalogPlatformString);
            
            /// \brief Get the catalog string from a supported Origin platform.  Defaults to the current platform.
            ///
            /// This function returns either PCWIN, MAC, or UNKNOWN.
            ORIGIN_PLUGIN_API QString catalogStringFromSupportedPlatfom(const PlatformService::OriginPlatform platform = Origin::Services::PlatformService::runningPlatform());

            /// \brief Get the string name of the known Origin Platform Id
            ///
            /// This function is offered for convenience reasons
            ORIGIN_PLUGIN_API QString stringForPlatformId(OriginPlatform platformId);

            /// \brief Map a string name returned by stringForPlatformId back to an Origin Platform Id
            ///
            /// This function is offered for convenience reasons
            ORIGIN_PLUGIN_API OriginPlatform platformIdForString(const QString &platform);

            /// \brief Get the string name of the Origin Processor Architecture
            ///
            /// This function is offered for convenience reasons
            ORIGIN_PLUGIN_API QString stringForProcessorArchitecture(OriginProcessorArchitecture architectureId);

            /// \brief Get a bitfield enumeration describing the processor architectures supported by this machine.
            ///
            /// Returns a bitfield enumeration of the processor architectures supported by this machine.
            ORIGIN_PLUGIN_API OriginProcessorArchitecture supportedArchitecturesOnThisMachine();

            /// \brief Get a boolean flag indicating whether this machine supports at least one of the architectures in the query.
            ///
            /// Mainly used to determine if this machine supports running a particular game, which supports some/all of our architectures.
            ORIGIN_PLUGIN_API bool queryThisMachineCanExecute(OriginProcessorArchitecture availableArchitectures);

            /// \brief Get a bitfield enumeration describing the processor architectures available in a piece of content by the catalog-formatted string.  (For a game, usually)
            ///
            /// This function is used mainly by the entitlement parsing APIs.  Parses strings like '32', '32/64', '64', etc.
            ORIGIN_PLUGIN_API OriginProcessorArchitecture availableArchitecturesFromCatalogString(const QString& catalogProcArchString);
            
            ORIGIN_PLUGIN_API VersionInfo currentOSVersion();

            ORIGIN_PLUGIN_API bool querySupportedByCurrentOS(VersionInfo minimumOSVersion, VersionInfo maximumOSVersion = VersionInfo());
            ORIGIN_PLUGIN_API VersionInfo currentClientVersion();

            ORIGIN_PLUGIN_API QString displayNameFromOSVersion(VersionInfo osVersion);

            /// \brief Examine a widget's properties to determine if it is an OIG window
            ///
            /// This function is offered for convenience reasons
            ORIGIN_PLUGIN_API bool isOIGWindow(QWidget*);

#ifdef Q_OS_WIN
            /// \brief Examine window properties and compare to desktop properties to determine if the window is fullscreen
            ///
            /// This function will only check for PC
            ORIGIN_PLUGIN_API bool isWindowFullScreen(HWND topWin);

            /// \brief Examine if Origin.exe is running in Windows compatibility mode
            ///
            /// This function will only check for PC
            ORIGIN_PLUGIN_API QString const getCompatibilityMode();
#endif
            /// \brief Grab current audio endpoint and query the volume that has been set for the application
            ///
            /// The funtion returns an float 0.0(MUTE)-1.0(FULL)
            /// This will only return an actual value for Windows OS greater than XP others will get an automatic 1
            ORIGIN_PLUGIN_API float GetCurrentApplicationVolume();

            /// \enum SystemResultCode
            /// \brief These represent the most common file system errors.
            ///
            /// There are additional errors which may be returned by the system APIs which are different from these. 
            /// You should be prepared to receive any value for an error code.
            ///
            /// WIN: We hard-code the values here because otherwise we'd have to include Microsoft headers from 
            /// a header file; and that is something we avoid at all costs.
            ///
            /// \warning Always update PlatformService::initSystemResultCodeStrings() when this enum changes!
            enum SystemResultCode
            {
#ifdef Q_OS_WIN
                ErrorNone                = 0,   ///< ERROR_NONE -- No error was found
                ErrorNotOpen           =  6,    ///< ERROR_INVALID_HANDLE -- Attempt to read a stream that hasn't been opened
                ErrorNoMemory          =   8,   ///< ERROR_NOT_ENOUGH_MEMORY -- Insufficient memory to perform operation
                ErrorInvalidName       = 123,   ///< ERROR_INVALID_NAME -- Invalid file name
                ErrorNameTooLong       = 111,   ///< ERROR_BUFFER_OVERFLOW -- File name/path is too long
                ErrorFileNotFound      =   2,   ///< ERROR_FILE_NOT_FOUND -- Attempt to open a nonexistent file for reading
                ErrorPathNotFound      =   3,   ///< ERROR_PATH_NOT_FOUND -- Attempt to access a file in a non-existent directory
                ErrorAccessDenied      =   5,   ///< ERROR_ACCESS_DENIED -- Insufficient privileges to open the file
                ErrorWriteProtect      =  19,   ///< ERROR_WRITE_PROTECT -- Attempt to open a read-only file for writing
                ErrorSharingViolation  =  16,   ///< ERROR_CURRENT_DIRECTORY -- Attempt to modify a file that is in use
                ErrorDiskFull          = 112,   ///< ERROR_DISK_FULL -- Out of space on the device
                ErrorFileAlreadyExists =  80,   ///< ERROR_FILE_EXISTS -- Attempt to create a new file with the same name as existing file
                ErrorDeviceNotReady    =  21,   ///< ERROR_NOT_READY -- Attempt to access a hardware device that isn't ready
                ErrorDataCRCError      =  23,   ///< ERROR_CRC -- The data read off of the disk was corrupted in some way
#endif // Q_OS_WIN
#ifdef ORIGIN_MAC
                ErrorNone                = 0,            // No error was found
                ErrorNotOpen           =  EBADF,         // Attempt to read a stream that hasn't been opened.
                ErrorNoMemory          =   ENOMEM,       // Insufficient memory to perform operation
                ErrorInvalidName       = ENOENT,         // Invalid file name.
                ErrorNameTooLong       = ENAMETOOLONG,   // File name/path is too long
                ErrorFileNotFound      =   ENOENT,       // Attempt to open a nonexistent file for reading
                ErrorPathNotFound      =   ENOTDIR,      // Attempt to access a file in a non-existent directory
                ErrorAccessDenied      =   EACCES,       // Insufficient privileges to open the file
                ErrorWriteProtect      =  EPERM,         // Attempt to open a read-only file for writing
                ErrorSharingViolation  =  EBUSY,         // Attempt to modify a file that is in use
                ErrorDiskFull          = ENOSPC,         // Out of space on the device
                ErrorFileAlreadyExists =  EEXIST,        // Attempt to create a new file with the same name as existing file
                ErrorDeviceNotReady    =  EWOULDBLOCK,   // Attempt to access a hardware device that isn't ready
                ErrorDataCRCError      =  EIO,           // The data read off of the disk was corrupted in some way
#endif // ORIGIN_MAC
            };


            ORIGIN_PLUGIN_API void init();
            ORIGIN_PLUGIN_API void release();

            ORIGIN_PLUGIN_API quint64 machineHash();
            ORIGIN_PLUGIN_API QString const& machineHashAsString();
            ORIGIN_PLUGIN_API QString const& machineHashAsSHA1();
            ORIGIN_PLUGIN_API PlatformService::OriginState originState();
            ORIGIN_PLUGIN_API void setOriginState(PlatformService::OriginState state);
            
            /// \brief Returns the last system result code.
            ORIGIN_PLUGIN_API SystemResultCode lastSystemResultCode();

            /// \brief Returns the platform's data location.
            ORIGIN_PLUGIN_API QString commonAppDataPath();

            /// \brief Returns the directory where cloud save backups are stored
            ORIGIN_PLUGIN_API QString cloudSaveBackupPath();

            /// \brief Returns the OS major version.
            ORIGIN_PLUGIN_API unsigned int OSMajorVersion();

            /// \brief Returns the OS minor version.
            ORIGIN_PLUGIN_API unsigned int OSMinorVersion();

            /// \brief Checks to see if an OS-specifc key exists (like a windows registry key, on Mac the bundle itself).
            /// \param path The key to check.
            /// \return bool True if the key exists, false if it doesn't.
            ORIGIN_PLUGIN_API bool OSPathExists(QString path);

            /// \brief Returns a full path given an OS specific key (like windows registry) with appended folder/filename. 
            ///
            /// \param key OS-specific; on Windows: [HKEY_LOCAL_MACHINE\\SOFTWARE\\EA Sports\\FIFA 12\\Install Dir]\\Game\\fifa.exe ; on Mac: [com.ea.fifa12]/Contents/MacOS/fifa
            /// \return QString The full local path from the key.
            ORIGIN_PLUGIN_API QString localFilePathFromOSPath(QString key);

            /// \brief returns a the directory portion given an OS specifc key
            ///
            /// \param key OS-specific; on Windows: [HKEY_LOCAL_MACHINE\\SOFTWARE\\EA Sports\\FIFA 12\\Install Dir]\\Game\\fifa.exe ; on Mac: [com.ea.fifa12]/Contents/MacOS/fifa
            /// \param b64BitPath: On Windows this forces reading from the 64 bit registry (i.e. non-virtualized path)
            /// \return QString The directory of the key that is passed in; in this case, the local path of the installDir.
            ORIGIN_PLUGIN_API QString localDirectoryFromOSPath(QString key, bool b64BitPath = false);

            /// \brief returns the OS-specific battery life
            ///
            /// \param batteryStatus the variable where the current batteryStatus enum flag will be stored.
            /// \param batteryPercent - the variable where the currect battery percentage will be stored.
            ORIGIN_PLUGIN_API void powerStatus(BatteryFlag& batteryStatus, int &batteryPercent);
            
            /// \brief returns a full path to the directory where the eacore.ini
            /// file lives, since it's in a different place between PC and Mac.
            ///
            /// \return QString The directory containing eacore.ini
            ORIGIN_PLUGIN_API QString eacoreIniDirectory();

            /// \brief returns a full path to the eacore.ini file, including the
            /// name, so it needs no modification.
            ///
            /// \return QString The full path and filename of eacore.ini
            ORIGIN_PLUGIN_API QString eacoreIniFilename();
            
            /// \brief returns a full path to the directory where the log
            /// files live, since it's in a different place between PC and Mac.
            ///
            /// \return QString The directory containing log files
            ORIGIN_PLUGIN_API QString logDirectory();
            
            /// \brief returns a full path to the client log file, including the
            /// name, so it needs no modification.
            ///
            /// \return QString The full path and filename of client_log.txt
            ORIGIN_PLUGIN_API QString clientLogFilename();
            
            /// \brief returns a full path to the bootstrapper log file, including the
            /// name, so it needs no modification.
            ///
            /// \return QString The full path and filename of bootstrapper_log.txt
            ORIGIN_PLUGIN_API QString bootstrapperLogFilename();
            
            /// \brief returns the file system path for global storage
            ORIGIN_PLUGIN_API QString machineStoragePath();

            /// \brief returns whether currently installed client is a beta version
            ORIGIN_PLUGIN_API bool isBeta();

            /// \brief returns whether two files are signed with the same certificate
            ORIGIN_PLUGIN_API bool compareFileSigning(QString file1, QString file2);

            /// \brief Gets version info from a plug-in's binary data (PC) or plist (Mac)
            /// \param pluginPath The path to the plug-in.  This is the DLL on PC or the dylib on Mac.
            /// \param pluginVersion [out] The plug-in version.
            /// \param compatibleClientVersion [out] The client version this plug-in is compatible with.
            /// \return True if versions were found.
            ORIGIN_PLUGIN_API bool getPluginVersionInfo(const QString& pluginPath, Origin::VersionInfo& pluginVersion, Origin::VersionInfo& compatibleClientVersion);

            /// \brief returns a QIcon with all icons contained in the given executable or library file.
            /// \param executableAbsolutePath Absolute path to the executable or library file to extract from.
            /// \return The QIcon containing all relevant icons.
            ORIGIN_PLUGIN_API QIcon iconForFile(const QString& executableAbsolutePath);
            
            ORIGIN_PLUGIN_API QString pathToConfigurationFile();
            
            ORIGIN_PLUGIN_API bool setAutoStart(bool autoStartEnabled);

            ORIGIN_PLUGIN_API bool isAutoStart();

            ORIGIN_PLUGIN_API bool isGuestUser();

            ORIGIN_PLUGIN_API bool IsUserAdmin();

            /// \brief Returns true if the given string is a SimCity product id
            /// This function is only here for the use by the few SimCity hacks that we need in the client until 9.4
            /// removes the need for the hacks.
            ORIGIN_PLUGIN_API bool isSimCityProductId(QString const& productId);

#ifdef Q_OS_WIN
			ORIGIN_PLUGIN_API void openUrlWithInternetExplorer(const QUrl& url);

			// Helpers to access registry
			ORIGIN_PLUGIN_API bool registryQueryValue(QString path, QString& result, bool b64BitPath = false, bool bFailIfNotFound = false);
		    ORIGIN_PLUGIN_API QString pathFromRegistry(QString key, bool b64BitPath = false);
			ORIGIN_PLUGIN_API QString dirFromRegistry(QString key, bool b64BitPath = false);

            // Starts flashing the task bar icon.
            // If count == 0, the icon continously flashes until the app is focused.
            // If count > 0, the icon flashes for 'count' times, then holds steady until stopped or the app is focused.
            ORIGIN_PLUGIN_API void flashTaskbarIcon(QWidget* widget, int count = 0);
            ORIGIN_PLUGIN_API void flashTaskbarIcon(quintptr winId, int count = 0);
#endif

            ORIGIN_PLUGIN_API void asyncOpenUrl(const QUrl &url);

            ORIGIN_PLUGIN_API void asyncOpenUrlAndSwitchToApp(const QUrl &url);

            
            /// \brief returns the file system path for a well-known standard location
            ORIGIN_PLUGIN_API QString getStorageLocation(QStandardPaths::StandardLocation location);
            
#ifdef ORIGIN_MAC
            
            /// \brief Returns the current version of OS X.
            ORIGIN_PLUGIN_API QString getOSXVersion();
            
            /// \brief Add the supplied window to Expose (i.e., Mission Control).
            ORIGIN_PLUGIN_API void addWindowToExpose( QWidget* window );
            
            ORIGIN_PLUGIN_API bool isReadOnlyPath(QString path);
            
            /// \brief returns the file system path to the root of the Origin application bundle
            ORIGIN_PLUGIN_API QString applicationBundleRootPath();
            
            /// \brief returns the file system path to the system preferences directory
            ORIGIN_PLUGIN_API QString systemPreferencesPath();
            
            /// \brief returns the file system path to the user preferences directory
            ORIGIN_PLUGIN_API QString userPreferencesPath();
            
            /// \brief returns the file system path to temporary data storage
            ORIGIN_PLUGIN_API QString tempDataPath();
            
            /// \brief returns the file system path to the Qt translations directory in the app bundle
            ORIGIN_PLUGIN_API QString translationsPath();

            /// \brief finds the bundle containing the specified path
            /// \param path the path to start the search for a bundle
            /// \param extension the extension to search for
            /// \return the path to the first containing bundle (returns 'path' if it is a bundle) or an empty string.
            ORIGIN_PLUGIN_API QString getContainerBundle(QString path, QString extension = ".app");
            
            /// \brief returns the unique identifier of the bundle specified by its path
            /// \param path the path to the bundle
            /// \return the bundle identifier or an empty string if the path doesn't point to a valid bundle
            ORIGIN_PLUGIN_API QString getBundleIdentifier(QString path);

            /// \brief returns the unique identifier of the bundle specified by its running executable pid
            /// \param pid the pid of the bundle's currently running executable
            /// \return the bundle identifier or an empty string if the pid doesn't correspond to a process with a known bundle identifier
            ORIGIN_PLUGIN_API QString getBundleIdentifier(pid_t pid);

            /// \brief logs a message to the system console
            ORIGIN_PLUGIN_API void LogConsoleMessage(const char* format, ...);

            /// \brief returns true if the sha1 values of the file contents are the same, otherwise false.
            /// Note that simple implementation loads the entire files, so try not to use it on large files!
            /// Use the debugEnabled parameter to log more information to the console.
            ORIGIN_PLUGIN_API bool haveIdenticalContents(const char* fileName1, const char* fileName2, bool debugEnabled);
            
            /// \brief install the Escalation Services helper tool if necessary.
            ///
            /// \param errorMsg A buffer to store the reason for the failure to install the helper tool.
            /// \param maxErrorMsg The length of the error buffer.
            /// \return bool True if the helper tool was up-to-date/it installed correctly, otherwise false.
            /// The conditions we use to install the current bundle version are:
            /// 1) first-time install
            /// 2) the sha1 values of the service file contents don't match
            ORIGIN_PLUGIN_API bool installEscalationServiceHelperDaemon(char* errorMsg, size_t maxErrorLen);

            /// \brief run an AppleScript scenario.
            /// \param script The actual text version of the AppleScript code to run.
            /// \param errorMsg A buffer to store the reason why the script failed to run.
            /// \param maxErrorMsg The length of the error buffer.
            /// \return bool True if the script compiled and ran, otherwise false.
            ORIGIN_PLUGIN_API bool runAppleScript(const char* script, char* errorMsg, size_t maxErrorLen);
            
            /// \brief enables the System Preferences' Enable Assistive Devices setting.
            ///
            /// \return bool true if the setting was already on or we did change it successfully, otherwise false.
            ORIGIN_PLUGIN_API bool enableAssistiveDevices();

            /// \brief Interrogates the application bundle at appBundlePath and reads Executable from bundle Info.plist and provide a full bundle path.
            /// \param appBundlePath Path to the Mac OS X application bundle (/Applications/Chess.app)
            /// \param actualExePath Reference to string where executable path will be placed.  Value will be relative to bundle path (/Applications/Chess.app/Contents/MacOS/Chess)
            /// \return bool True if executable was successfully extracted, false otherwise
            ORIGIN_PLUGIN_API bool extractExecutableFromBundle(const QString& appBundlePath, QString& actualExePath);

            /// \brief requests the certificate requirements to use when injected a dylib (IGO).
            /// \param buffer the buffer to store the certificate requirements.
            /// \param bufferSize the size of the buffer.
            /// \return bool true if the buffer was large enough to return the certificate requirements, otherwise false.
            ORIGIN_PLUGIN_API bool getIGOCertificateRequirements(char* buffer, size_t bufferSize);
            
            /// \brief reads the certificates from the local keychain.
            /// \param a list to insert the certificate data.
            ORIGIN_PLUGIN_API void readLocalCertificates(QList<QSslCertificate>* certs);
            
            /// \brief delete a specific directory (doesn't need to be empty).
            /// \param dirName the name of the directory to delete.
            /// \param errorMsg A buffer to store the reason why the script failed to run.
            /// \param maxErrorMsg The length of the error buffer.
            /// \return bool true if the directory was successfully delete, false otherwise.
            ORIGIN_PLUGIN_API bool deleteDirectory(const char* dirName, char* errorMsg, size_t maxErrorLen);

            ORIGIN_PLUGIN_API bool restoreApplicationFromTrash(QString oldPath, QString newPath, QString appPath);
            
            // Setup a tap to watch the CTRL+Tab/CTRL+SHIFT+Tab combos to bypass the Mac Keyboard Interface Control
            // behavior for the key view loop.
            class ORIGIN_PLUGIN_API KICWatchListener
            {
                public:
                    virtual ~KICWatchListener() {}
                
                    virtual void KICWatchEvent(int key, Qt::KeyboardModifiers modifiers) = 0;
            };            
            ORIGIN_PLUGIN_API void* setupKICWatch(void* viewId, KICWatchListener* listener);
            
            // Remove tap used to watch CTRL+Tab/CTRL+SHIFT+Tab combos.
            ORIGIN_PLUGIN_API void cleanupKICWatch(void* tap);

            /// \brief Make a widget window invisible.
            /// \param winId the identifier returned from the widget winId() method.
            /// \param pushOnTop whether the window should be pushed on top of all other windows.
            ORIGIN_PLUGIN_API void MakeWidgetInvisible(int winId, bool pushOnTop);
            
            /// \brief Retrieves the LSMinimumSystemVersion value from the Info.plist pointed to by a bundle path.
            /// \param bundlePath The path to the bundle.
            ORIGIN_PLUGIN_API QString getRequiredOSVersionFor(QString const& bundlePath);
            
#if 0 // Keeping this code around for when I figure out to properly/entirely disable App Nap using the NSProcessInfo API
            /// \brief Notify the OS that the process is performing some user-triggered activity and should
            /// allow the process to go to "sleep" / minimize its CPU/energy-consumption (App Nap)
            /// \return an unique ID for the started activity.
            ORIGIN_PLUGIN_API void* beginUserActivity(const char* reason);
            
            /// \brief Notify the OS that the previously declared activity is over and the system can put the
            /// the process to "sleep" / minimize its CPU/energy-consumption (App Nap) if desired.
            ORIGIN_PLUGIN_API void endUserActivity(void* activityID);
#endif
#endif

            /// \brief Returns the file system path to the directory holding external (non-Qt) application resources.
            ORIGIN_PLUGIN_API QString resourcesDirectory();

            /// \brief Returns the file system path to the directory for Origin plug-ins.
            ORIGIN_PLUGIN_API QString pluginsDirectory();
            
            /// \brief Returns the file system path to the ODT license file directory.
            ORIGIN_PLUGIN_API QString odtDirectory();
            
            /// \brief The name of the ODT license file directory.
            ORIGIN_PLUGIN_API extern const QString ODT_FOLDER_NAME;

            /// \brief Uses the escalation service to create a folder
            /// \param path The folder path to create
            /// \param acl The access control list string to use
            /// \param uacExpired Whether the UAC prompt has expired
            ORIGIN_PLUGIN_API bool createFolderElevated(const QString &path, const QString &acl,
                bool *uacExpired = nullptr, int *elevationResult = nullptr, int *elevationError = nullptr);

            /// \brief Sleeps the current thread for a given number of milliseconds.
            ORIGIN_PLUGIN_API void sleep(quint32 msecs);

#if defined(ORIGIN_MAC)

            /// \brief Handy template for scoping references to CoreFoundation entities that
            /// need get cleaned up before the return of the function.
            template <typename T>
            class ORIGIN_PLUGIN_API ScopedCFRef
            {
            public:
            
                ScopedCFRef()
                : _ref(NULL)
                {
                }
                
                explicit ScopedCFRef(T ref)
                : _ref(ref)
                {
                }
                
                ~ScopedCFRef()
                {
                    if (_ref != NULL)
                    {
                        CFRelease(_ref);
                    }
                }
                
                T& operator()()
                {
                    return _ref;
                }
                
                T& operator*()
                {
                    return _ref;
                }
                
                bool isNull()
                {
                    return _ref == NULL;
                }
                
            private:
            
                T _ref;
            };

#endif

            /// \brief Returns a string representation of the given SystemResultCode
            ///
            /// \return String representation of the given SystemResultCode, or a 
            //          default-constructed QString if the code couldn't be mapped.
            ORIGIN_PLUGIN_API QString systemResultCodeString(const SystemResultCode code);

            /// \brief Returns the string representation of the last error to occur.
            ///
            /// \sa PlatformService::lastSystemResultCode()
            ORIGIN_PLUGIN_API QString getLastSystemResultCodeString();

            // Disk functions
            class ORIGIN_PLUGIN_API DriveInfo
            {
            public:

                DriveInfo(
                    const QString& driveString, 
                    const QString& driveType, 
                    const QString& fileSystemType,
                    const quint64& totalBytes, 
                    const quint64& freeBytes);

                QString GetDriveString() const          { return mDriveString; }
                QString GetDriveTypeString() const      { return mDriveType; }
                QString GetFileSystemTypeString() const { return mFileSystemType; }
                quint64 GetTotalBytes() const           { return mTotalBytes; }
                quint64 GetFreeBytes() const            { return mFreeBytes; }
        
            private:

                //! Drive letter string 
                QString mDriveString; 

                //! Drive type
                /*!
                    \sa EA::IO::DriveType
                */
                QString mDriveType; 

                //! File system type
                QString mFileSystemType; 

                //! Total bytes available on the drive
                quint64 mTotalBytes;

                //! Free bytes available on the drive
                quint64 mFreeBytes;
            };

            ORIGIN_PLUGIN_API void PrintDriveInfoToLog();

            ORIGIN_PLUGIN_API void GetDriveInfo(QList<DriveInfo>& driveInfoList);

            ORIGIN_PLUGIN_API void GetDriveLetterStrings(QStringList& driveStringList);

            ORIGIN_PLUGIN_API QString GetFileSystemType(const QString& driveString);

            ORIGIN_PLUGIN_API qint64 GetFreeDiskSpace(QString const& path, quint64* pTotalNumberOfBytes=0, quint64* pTotalNumberOfFreeBytes=0);

            ORIGIN_PLUGIN_API bool GetVolumeWritableStatusFromPath(QString const& path);

            ORIGIN_PLUGIN_API bool GetDACLStringFromPath(QString const& path, bool useParentDirectory, QString& outDACL);

            QDataStream &operator << (QDataStream &out, const OriginPlatform &platform);
            QDataStream &operator >> (QDataStream &in, OriginPlatform &platform);
        };
    }
}

#endif //PLATFORM_H
