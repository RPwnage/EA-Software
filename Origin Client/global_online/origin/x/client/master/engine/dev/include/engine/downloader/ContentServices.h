#ifndef CONTENTSERVICES_H
#define CONTENTSERVICES_H

#include "services/downloader/Common.h"
#include "services/downloader/Parameterizer.h"
#include "engine/downloader/IContentInstallFlow.h"
#include "engine/login/User.h"
#include "services/escalation/IEscalationClient.h"
#include "engine/content/ContentTypes.h"
#include "services/plugin/PluginAPI.h"

#ifdef WIN32
#include <windows.h>
#endif

#include <QMap>
#include <QObject>
#include <QSharedPointer>
#include <QVector>
#include <QUrl>

namespace Origin
{
    namespace Downloader
    {
        class File;

        /// \brief TBD.
        class ORIGIN_PLUGIN_API ContentServices : public QObject
        {
            Q_OBJECT
            // Intended to be a factory which creates a pair of Flow/Protocol objects
            
        public:

            /// \brief TBD.
            static const QString EAD_REG_STAGING_KEY;

            /// \brief TBD.
            static const QString EAD_REG_CODE;

            /// \brief TBD.
            static const QString EAD_REG_MEMBER_NAME;

            /// \brief TBD.
            static const QString EAD_REG_ACCESS_STAGING_KEY;

            /// \brief TBD.
            static const QString EAD_REG_DISPLAY_NAME;

            /// \brief TBD.
            static const QString EAD_REG_LOCALE;

			/// \brief TBD.
            static const QString SREGFILE;

            /// \brief Deletes the unpack folder of non DiP content, i.e. C:/ProgramData/Orgin/DownloadCache/cache/crysis_se1/
            ///
            /// Checks for folder existance prior to removal.
            ///
            /// \param content The non DiP content whose unpack folder should be deleted.
            static void ClearDownloadCache(const Origin::Engine::Content::EntitlementRef content);

            /// \brief Creates an IContentInstallFlow object for a specified entitlement.
            ///
            /// Determines which subclass of Downloader::IContentInstallFlow to use by examining the content's package type.
            /// - Services::Publishing::PackageTypeDownloadInPlace -> ContentInstallFlowDiP
            /// - Services::Publishing::PackageTypeSingle -> ContentInstallFlowNonDiP
            /// - Services::Publishing::PackageTypeUnpacked -> ContentInstallFlowNonDiP
            /// \param item The entitlement for which a content install flow object will be created.
            /// \param user The current user.
            /// \return IContentInstallFlow A pointer to the created IContentInstallFlow or NULL for unknown package types.
            static IContentInstallFlow*  CreateInstallFlow (Origin::Engine::Content::EntitlementRef item, Engine::UserRef user );

            /// \brief Builds the absolute path to a specified entitlement's LocalContent folder, i.e. C:/ProgramData/Origin/LocalContent/Shank/
            ///
            /// Will attempt to escalate the Origin client process in order to create the folder.
            ///
            /// \param content The entitlement whose LocalContent folder is desired.
            /// \param path A reference string in which to store the contructed path.
            /// \param createIfNotExists If true, wiil create the folder if it does not already exist.
            /// \param elevation_result If the folder is created, the result of the escalation attempt is stored here.
            /// \return bool False if the path string could not be built or escalation failed during folder creation.
            static bool GetContentInstallCachePath(const Origin::Engine::Content::EntitlementRef content,  QString& path, bool createIfNotExists = false, int *contentDownloadErrorType = NULL, int *elevation_error = NULL);

            /// \brief Builds the absolute path string for the unpack folder of non DiP content, i.e. C:/ProgramData/Orgin/DownloadCache/cache/crysis_se1/
            ///
            /// \param content The non DiP content whose unpack folder path is desired.
            /// \param path The built path string is stored in this reference.
            /// \param createIfNotExists If true, the folder will be created if it does not already exist.
            /// \return bool False if the path could not be built.
            static bool GetContentDownloadCachePath(const Origin::Engine::Content::EntitlementRef content, QString& path, bool createIfNotExists = false);

            static QString GetTempFolderPath();

            static QString GetTempFilename(const QString& offerId, const QString& pattern);

            /// \brief Initializes the common install manifest entries for all entitlement package types.
            ///
            /// Manifest entry keys that are initialized:
            /// - QString downloaderversion
            /// - QString id (Engine::Content::ContentConfiguration::key)
            /// - QString locale
            /// - bool ispreload
            /// - bool autoresume
            /// - bool autostart
            /// - bool downloading
            /// - bool needsrepair
            /// - qint64 totalbytes
            /// - qint64 savedbytes
            /// - qint64 contentversion
            /// - QString currentstate
            /// - QString previousstate
            ///
            /// \param item The entitlement which owns the install manifest.
            /// \param installManifest An install manifest reference.
            static void InitializeInstallManifest(const Origin::Engine::Content::EntitlementRef item, Parameterizer& installManifest);

            /// \brief Loads the install manifest of a specified entitlement into a Downloader::Parameterizer.
            ///
            /// \param item The entitlement whose install manifest will be loaded.
            /// \param installManifest A reference to a Downloader::Parameterizer in which to load the install manifest.
            /// \return bool True if the load was successful.
            static bool LoadInstallManifest (const Origin::Engine::Content::EntitlementRef item, Parameterizer& installManifest);

            /// \brief Save the install manifest of a specified entitlement.
            ///
            /// The install manifest gets saved to the path returned by ContentServices::GetInstallManifestPath.
            ///
            /// \param item The entitlement whose manifest will be saved.
            /// \param installManifest The install manifest to save.
            /// \return bool True if the save was successful.
            static bool SaveInstallManifest (const Origin::Engine::Content::EntitlementRef item, const Parameterizer& installManifest);

            /// \brief Removes the install manifest file for a specified entitlement if it exists. 
            ///
            /// \param item The entitlement whose install manifest file should be removed.
            static void DeleteInstallManifest (const Origin::Engine::Content::EntitlementRef item);

            /// \brief Builds the path string where a specified entitlement's install manifest will be saved.
            ///
            /// ContentServices::GetContentInstallCachePath + "/" + ID.mfst where ID = Engine::Content::ContentConfiguration::productId
            ///
            /// \param item The entitlement whose install manifest file path is desired.
            /// \param path The built path string is stored in this reference.
            /// \param createIfNotExists If true and it does not exist, the folder for the install manifest file will be created.
            /// \return bool False, if the path string could not be built or folder creation failed.
            static bool GetInstallManifestPath(const Origin::Engine::Content::EntitlementRef item,  QString& path, bool createIfNotExists = false);
            
#ifdef ORIGIN_MAC
            /// \brief Content information required by EA Access that is saved to the registry on Windows, is instead written to a plist and saved in the game bundle, on Mac.
            /// \param content The entitlement whose information is being written to a plist.
            /// \param locale The user's locale.
            /// \param bundlePath Path to the game bundle - in which the plist is saved.
            static void CreateAccessPlist(const Origin::Engine::Content::EntitlementRef content, const QString& locale, const QString& bundlePath);
#endif

#ifdef ORIGIN_PC
            /// \brief Sets registry key/value, HKCU version.
            ///
            /// \param content The entitlement whose keys/values need to be created.
            /// \param isHKCU Set to true if the created key's root is HKEY_CURRENT_USER.
            static void setHKeyCurrentUser(const Origin::Engine::Content::EntitlementRef content, bool &isHKCU);

            /// \brief TBD.
            ///
            /// \return bool True if the export was successful.
            static bool ExportStagingKeys();

            /// \brief Sets registry key/value, Local Machine version.
            /// \param content The entitlement whose access staging keys need to be writen.
            /// \param Origin locale.
            static void setHKeyLocalMachine(const Origin::Engine::Content::EntitlementRef content, const QString& locale);
            
            /// \brief Set registry key/value based on content.
            ///
            /// \param content The entitlement whose staging keys need to be writen.
            static void setContentHKey(const Origin::Engine::Content::EntitlementRef content);

        private:

            /// \brief TBD.
            ///
            /// \param preDefKey TBD.
            /// \param subkey TBD.
            /// \param valueName TBD.
            /// \param dataString TBD.
            /// \return bool True if ???.
            static bool WriteRegistryString(const HKEY preDefKey, const QString& subkey, const QString& valueName, const QString& dataString);

            /// \brief TBD.
            ///
            /// \param preDefKey TBD.
            /// \param subkey TBD.
            /// \param valueName TBD.
            /// \param dataString TBD.
            /// \return bool True if ???.
            static bool WriteRegistryStringEx(const HKEY preDefKey, const QString& subkey, const wchar_t* valueName, QString dataString);

            /// \brief TBD.
            ///
            /// \param sCMSRegistryString TBD.
            /// \param hHiveKey TBD.
            /// \param sRegistryPath TBD.
            /// \param sSubKey TBD.
            /// \return bool True if ???.
            static bool ParseCMSRegistryString(const QString& sCMSRegistryString, HKEY &hHiveKey, QString &sRegistryPath, QString &sSubKey);
#endif // ORIGIN_PC
        };
        
    } // namespace Downloader
} // namespace Origin

#endif // CONTENTSERVICES_H
