///////////////////////////////////////////////////////////////////////////////
// DiPManifest.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _DIPMANIFEST_H_
#define _DIPMANIFEST_H_

#include <list>
#include <map>
#include "services/common/VersionInfo.h"
#include <QString>
#include <QStringList>
#include <QFile>
#include <QMap>
#include <QList>
#include <QDomElement>

#include "services/publishing/SoftwareBuildMetadata.h"
#include "engine/content/LocalContent.h"
#include "services/plugin/PluginAPI.h"
#include "engine/content/DynamicContentTypes.h"

namespace Origin
{
    namespace Downloader
    {
        /// \brief Contains basic information about a EULA.
        class ORIGIN_PLUGIN_API EulaInfo
        {
        public:
            /// \brief The name of the EULA.
            QString mName;

            /// \brief The file path to the EULA.
            QString mFileName;

            /// \brief The installed size of the component.
            qint64 mInstalledSize;
        };

        /// \brief Contains basic information about any optional components that the user may install.
        class ORIGIN_PLUGIN_API OptComponentInfo
        {
        public:
            /// \brief The name of the flag to pass to the touchup installer.
            QString mFlagName;

            /// \brief The name of the EULA.
            QString mEulaName;

            /// \brief The name of the component.
            QString mInstallName;

            /// \brief The file name of the EULA.
            QString mEulaFileName;

            /// \brief The warning to display if the user chooses to cancel the optional install.
            QString mCancelWarning;

            /// \brief Extra information about the content this EULA is for.
            QString mToolTip;

            /// \brief The installed size of the component.
            qint64 mInstalledSize;
        };

        typedef QList<EulaInfo> EulaInfoList;
        typedef QList<OptComponentInfo> OptComponentInfoList;

        /// \brief Locale-specific information about content.
        class ORIGIN_PLUGIN_API LocalizedInfo
        {
        public:
            /// \brief The LocalizedInfo constructor.
            LocalizedInfo();
            
            /// \brief The LocalizedInfo copy constructor.
            /// \param other The LocalizedInfo object to copy from.
            LocalizedInfo(const LocalizedInfo& other);

            /// \brief Overridden "=" operator.
            /// \param other The LocalizedInfo object to copy from.
            LocalizedInfo& operator=(const LocalizedInfo& other);

            /// \brief Checks to make sure the locale is valid.
            /// \return True if locale is valid.
            bool IsEmpty() const { return msLocale.isEmpty(); }

            /// \brief The locale (i.e. "en_US").
            QString msLocale;

            /// \brief The localized game title.
            QString msTitle;

            /// \brief A list of all EULAs for this content.
            EulaInfoList mEulas;

            /// \brief A list of optional components to install.
            OptComponentInfoList mOptComponents;

            /// \brief A wildcard matches to exclude.(DiP 3.x and previous)
            QStringList mWildcardExcludes;

            /// \brief A wildcard matches to include (DiP 4.0+)
            QStringList mWildcardIncludes;

            /// \brief Whether an additional download is required for this locale
            bool mDownloadRequired;
        };

        class ProgressiveInstallChunk
        {
        public:
            ProgressiveInstallChunk();

            /// \brief The ProgressiveInstallChunk copy constructor.
            /// \param other The ProgressiveInstallChunk object to copy from.
            ProgressiveInstallChunk(const ProgressiveInstallChunk& other);

            /// \brief Overridden "=" operator.
            /// \param other The ProgressiveInstallChunk object to copy from.
            ProgressiveInstallChunk& operator=(const ProgressiveInstallChunk& other);

            bool SetChunkID(const QString& id);

            int GetChunkID() { return mChunkID; }

            bool SetChunkRequirement(const QString& requirement);

            Origin::Engine::Content::DynamicDownloadChunkRequirementT GetChunkRequirement() { return mChunkRequirement; }

            void SetChunkName(const QString& name) { mChunkName = name; }

            QString GetChunkName() { return mChunkName; }

            void AddFilePattern(const QString& pattern) { mFilePatterns.insert(pattern); }

            QSet<QString> GetFilePatterns() { return mFilePatterns; }

            bool HasPatterns() { return (mFilePatterns.size() == 0); }

        private:

            int mChunkID;

            Origin::Engine::Content::DynamicDownloadChunkRequirementT mChunkRequirement;

            QString mChunkName;

            QSet<QString> mFilePatterns;
        };

        typedef QMap<int,ProgressiveInstallChunk> tProgressiveInstallChunks;

        /// \brief Converts a string value to a boolean.
        /// \param sValue A string that represents a boolean value.
        /// \return True if a XML flag string is "1" or "true" as per w3 spec: www.w3.org/TR/xmlschema-2/\#boolean
        bool FlagStringToBool(const QString& sValue);

        typedef std::map<QString,LocalizedInfo> tLocalizedInfoMap;

        /// \brief Download In Place Manifest information class
        ///
        /// <b>DiP 1.0</b> Initial implementation
        /// 
        /// <b>DiP 1.1</b> Language Excludes
        /// 
        /// <b>DiP 2.0</b> Autopatching support, Shortcuts Enabled, Ignores, Deletes
        /// 
        /// <b>DiP 2.1</b> Consolodated EULAs
        ///
        /// <b>DiP 2.2 (Proposed, NYI)</b> Game Version In Manifest, explicit feature enabling flags.
        /// The various features would default to the behavior previously defined above, but could be explicitly overridden if needed.
        ///
        /// <b>DiP 3.0</b> Differential updates
        ///
        /// <b>See also:</b> <a style="font-weight:bold;" href="https://developer.origin.com/documentation/display/EBI/DiP+Manifest+2.2+Schema">DiP Manifest 2.2 Schema</a>
        /// in the %Origin Internal wiki.
        ///
        /// <b>Important Note:</b> Attribute names are case sensitive!
        /// 
        /// <b>Example:</b>
        ///
        /// \code
        /// <game gameVersion="string" manifestVersion="2.2">
        /// <metadata>
        /// <featureFlags>
        /// \endcode
        class ORIGIN_PLUGIN_API DiPManifest
        {
        public:
            /// \brief The DiPManifest constructor.
            DiPManifest();

            /// \brief The DiPManifest copy constructor.
            /// \param other The DiPManifest object to copy from.
            DiPManifest(const DiPManifest& other);

            /// \brief Overridden "=" operator
            /// \param other The DiPManifest object to copy from.
            /// \return A DiPManifest object identical to other.
            DiPManifest& operator=(const DiPManifest& other);

            /// \brief Clears all information contained in this manifest.
            void Clear();

            /// \brief Checks to see if there is any information contained in this manifest.
            /// \return True if this manifest contains no information.
            bool IsEmpty() const;

            /// \brief Checks to see if the given locale is supported by the content.
            /// \param lang A string representing the locale (i.e. "en_US").
            /// \return True if the locale is supported.
            bool IsLocaleSupported(const QString& lang) const;

            /// \brief Checks to see if given local requires an additional download
            /// \param lang A string representing the locale (i.e. "en_US").
            /// \return True if the locale is supported and requires an additional download
            bool CheckIfLocaleRequiresAdditionalDownload(const QString& lang) const;

            /// \brief Gets a list of all locales supported by the content.
            /// \return A list of all locales supported by the content.
            QStringList GetSupportedLocales() const;

            /// \brief Parses an existing manifest file.
            /// \param manifestFile A QFile object created from an existing manifest file.
            /// \param isDLC A boolean flag indicating whether the content is DLC
            /// \return True if no errors were encountered during parsing.
            bool Parse(QFile & manifestFile, bool isDLC);

            /// \brief Loads the manifest file from disk.
            /// \param sFileName The path to the file to load.
            /// \param isDLC A boolean flag indicating whether the content is DLC
            /// \return True if the file was loaded successfully.
            bool Load(const QString& sFileName, bool isDLC);

            /// \brief Gets a list of all content IDs installable from this.
            /// \return A list of all content IDs this manifest supports.
            const QStringList& GetContentIDs() { return mContentIDs; }

            /// \brief Gets a list of wildcards slated for deletion.
            /// \return A list of wildcards slated for deletion.
            const QStringList& GetWildcardDeletes() { return mWildcardDeletes; }

            /// \brief Gets a wildcard list that should be excluded from a download.
            /// \return A list of wildcards that should be excluded from a download.
            const QStringList& GetWildcardExcludes(const QString& sLocale) { return GetSelectedInfo(sLocale).mWildcardExcludes; }

            const QStringList& GetWildcardIncludes(const QString& sLocale) { return GetSelectedInfo(sLocale).mWildcardIncludes; }

            const QStringList& GetAllLanguageIncludes() { return mAllLanguageFiles; }

            /// \brief Gets a list of wildcards that should be ignored.
            /// \return A list of wildcards that should be ignored.
            const QStringList& GetWildcardIgnores() { return mWildcardIgnores; }

            /// \brief Gets a list of wildcards that should be excluded from differential update.
            /// \return A list of wildcards that should be excluded from differential update.
            const QStringList& GetWildcardExcludeFromDifferential() { return mWildcardExcludesFromDifferential; }
            
            /// \brief Gets the title of the content.
            /// \return The title of the content.
            const QString& GetTitle(const QString& sLocale) const { return GetSelectedInfo(sLocale).msTitle; }
            
            /// \brief Gets a list of EulaInfo objects.
            /// \return A list of EulaInfo objects.
            const EulaInfoList& GetEulas(const QString& sLocale) const { return GetSelectedInfo(sLocale).mEulas; }
            
            /// \brief Gets a list of optional components to install.
            /// \return A list of optional components to install.
            const OptComponentInfoList& GetOptionalCompInfo (const QString& sLocale) const { return GetSelectedInfo(sLocale).mOptComponents; }
            
            /// \brief Gets the path to the touchup installer (relative to the DiP install directory).
            /// \return The path to the touchup installer.
            const QString& GetInstallerPath() const { return msInstallerPath; }
            
            /// \brief Gets any parameters that should be used with the touchup installer.
            /// \return The parameters to use with the touchup installer.
            const QString& GetInstallerParams() const { return msInstallerParams; }

            /// \brief Gets any parameters that should be used with the touchup installer only during an update operation.
            /// \return The repair parameters.
            const QString& GetUpdateParams() const { return msUpdateParams; }

            /// \brief Gets any parameters that should be used with the touchup installer only during a repair operation.
            /// \return The repair parameters.
            const QString& GetRepairParams() const { return msRepairParams; }

            /// \brief Gets the game version (set by game teams).
            /// \return A VersionInfo object representing the game version.
            const VersionInfo& GetGameVersion() const { return mSoftwareBuildMetadata.mGameVersion; }
            
            /// \brief Gets the manifest version (DiP version, i.e. 1.1, 2.0, etc.).
            /// \return A VersionInfo object representing the manifest version.
            const VersionInfo GetManifestVersion() const { return mManifestVersion; }

            /// \brief Gets the game's minimum OS version (5.1 for Win XP, 7.0 for Win7, etc.).
            /// \return A VersionInfo object representing the game's minimum OS version.
            const VersionInfo GetMinimumRequiredOS() const { return mSoftwareBuildMetadata.mMinimumRequiredOS; }
            
            /// \brief Gets the game's required Origin Client version (e.g. 9.5.0.1234).  This is only used by plug-ins.
            /// \return A VersionInfo object representing the game's required Origin Client version.
            const VersionInfo GetRequiredClientVersion() const { return mSoftwareBuildMetadata.mRequiredClientVersion; }

            /// \brief Checks if auto updates are enabled for this content.
            /// \return True if auto updates are enabled for this content.
            bool GetAutoUpdateEnabled() { return mSoftwareBuildMetadata.mbAutoUpdateEnabled; }

            /// \brief Checks if ignores are enabled for this content.
            /// \return True if ignores are enabled for this content.
            bool GetIgnoresEnabled() { return mSoftwareBuildMetadata.mbIgnoresEnabled; }

            /// \brief Checks if deletes are enabled for this content.
            /// \return True if deletes are enabled for this content.
            bool GetDeletesEnabled() { return mSoftwareBuildMetadata.mbDeletesEnabled; }

            /// \brief Checks if consolidated EULAs are enabled for this content.
            /// \return True if consolidated EULAs are enabled for this content.
            bool GetConsolidatedEULAsEnabled() { return mSoftwareBuildMetadata.mbConsolidatedEULAsEnabled; }

            /// \brief Checks if shortcuts are enabled for this content.
            /// \return True if shortcuts are enabled for this content.
            bool GetShortcutsEnabled() { return mSoftwareBuildMetadata.mbShortcutsEnabled; }

            /// \brief Checks if language excludes are enabled for this content.
            /// \return True if language excludes are enabled for this content.
            bool GetLanguageExcludesEnabled() { return mSoftwareBuildMetadata.mbLanguageExcludesEnabled; }

            /// \brief Checks if language includes are enabled for this content.  (DiP 4+)
            /// \return True if language includes are enabled for this content.
            bool GetLanguageIncludesEnabled() { return mSoftwareBuildMetadata.mbLanguageIncludesEnabled; }

            /// \brief Checks if the content should use the game version from the manifest.
            /// \return True if the content should use the game version from the manifest.
            bool GetUseGameVersionFromManifestEnabled() { return mSoftwareBuildMetadata.mbUseGameVersionFromManifestEnabled; }

            /// \brief Checks if the content should always re-run the touch-up installer after an update.
            /// \return True if the content should always re-run the touch-up installer after an update.
            bool GetForceTouchupInstallerAfterUpdate() { return mSoftwareBuildMetadata.mbForceTouchupInstallerAfterUpdate; }

            /// \brief Checks if the content should not allow launching when the game is out of date.
            bool GetTreatUpdatesAsMandatory() { return mSoftwareBuildMetadata.mbTreatUpdatesAsMandatory; }

            /// \brief Check if differential updates are enabled (available starting DiP 3.0).
            bool GetEnableDifferentialUpdate() { return mSoftwareBuildMetadata.mbEnableDifferentialUpdate; }

            /// \brief Checks if the game must be run with elevated priveleges.  (i.e. through the ORiginClientService)
            /// \return True if the game should be run elevated.
            bool GetExecuteGameElevated() { return mSoftwareBuildMetadata.mbExecuteGameElevated; }

            /// \brief Checks if any chunks are defined for use with progressive installer.
            /// \return True if the game has defined progressive installer chunks.
            bool GetProgressiveInstallationEnabled() { return (mSoftwareBuildMetadata.mbDynamicContentSupportEnabled && !mProgressiveInstallChunks.empty()); }

            /// \brief Checks if the game supports changing its installation language post-install.
            /// \return True if the game has support for changing the installation language.
            bool GetLanguageChangeSupportEnabled() { return mSoftwareBuildMetadata.mbLanguageChangeSupportEnabled; }

            /// \brief If specified, this path should point to the registry location that contains the uninstall executable/parameters to run.
            const QString& GetUninstallPath() { return msUninstallPath; }

            /// \brief Launchers for this item (if available.)
            const Origin::Engine::Content::tLaunchers& GetLaunchers() { return mLaunchers; }

            /// \brief Gets the chunk definitions for progressive installation
            tProgressiveInstallChunks& GetProgressiveInstallerChunks() { return mProgressiveInstallChunks; }

            Origin::Services::Publishing::SoftwareBuildMetadata GetSoftwareBuildMetadata() { return mSoftwareBuildMetadata; }

        private:
            bool ParseManifestV31Earlier(const QDomElement& oElement_Game);

            bool ParseManifestV4(const QDomElement& oDiPManifestHeader);

            /// \brief Parses the feature flags element
            bool ParseFeatureFlags(const QDomElement& oFeatureFlagsElement);

            bool ParseManifestContentIds(const QDomElement& oRootElement);

            bool ParseRuntimeBlock(const QDomElement& oRootElement);

            bool ParseIgnoreDeletes(const QDomElement& oRootElement);

            bool ParseTouchupBlock(const QDomElement& oTouchupElement);

            /// \brief Gets the locale-specific information for this content.
            /// \return A LocalizedInfo object containing the locale-specific information for this content.
            const LocalizedInfo& GetSelectedInfo(const QString& sLocale) const;

        private:
            bool mIsDLC;

            /// \brief The on-disk filename this manifest was loaded from (if any)
            QString mFileName;

            /// \brief The current locale.
            QString msLocale;

            /// \brief The data structure that holds all locale specific configuration data.
            tLocalizedInfoMap mLocalizedInfoMap;

            tProgressiveInstallChunks mProgressiveInstallChunks;

            /// \brief Path to the touchup installer.
            QString msInstallerPath;

            /// \brief Parameters to pass to the touchup installer.
            QString msInstallerParams;

            /// \brief Update Parameters to pass to the touchup installer if it is a repair operation.
            QString msUpdateParams;

            /// \brief Repair Parameters to pass to the touchup installer if it is a repair operation.
            QString msRepairParams;

            /// \brief List of Launchers
            Origin::Engine::Content::tLaunchers mLaunchers;

            /// \brief Game build-specific metadata (mainly used for DiP 4.0+, but data structures shared for legacy DiP parsing)
            Origin::Services::Publishing::SoftwareBuildMetadata mSoftwareBuildMetadata;

            /// \brief DiP manifest version.
            VersionInfo mManifestVersion;
            
            /// \brief Uninstall registry key.  Follows same convention as msInstallerPath
            QString msUninstallPath;

            /// \brief All content IDs that this manifest can be used to install.
            QStringList mContentIDs;

            /// \brief List of wildcard files to be explicitly deleted during DiPUpdate.
            QStringList mWildcardDeletes;

            /// \brief List of wildcards to ignore during DiPUpdate
            QStringList mWildcardIgnores;
            
            /// \brief List of wildcard files to exclude from differential update.
            QStringList mWildcardExcludesFromDifferential;

            /// \brief Combined list of all language-specific include patterns (DiP 4.0+)
            QStringList mAllLanguageFiles;
        };
    }
}

#endif
