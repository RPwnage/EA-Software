///////////////////////////////////////////////////////////////////////////////
// SoftwareBuildMetadata.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _SOFTWAREBUILDMETADATA_H_
#define _SOFTWAREBUILDMETADATA_H_

#include <QString>
#include <QStringList>
#include <QFile>
#include <QMap>
#include <QList>
#include <QtXml/QXmlSimpleReader>

#include "services/common/VersionInfo.h"

namespace Origin
{

namespace Services
{

namespace Publishing
{

class ORIGIN_PLUGIN_API SoftwareBuildMetadata
{
public:
    SoftwareBuildMetadata();

    /// \brief The SoftwareBuildMetadata copy constructor.
    /// \param other The SoftwareBuildMetadata object to copy from.
    SoftwareBuildMetadata(const SoftwareBuildMetadata& other);

    /// \brief Overridden "=" operator
    /// \param other The SoftwareBuildMetadata object to copy from.
    /// \return A SoftwareBuildMetadata object identical to other.
    SoftwareBuildMetadata& operator=(const SoftwareBuildMetadata& other);

    /// \brief Clears the contents of the build metadata block.
    void Clear();

    /// \brief Parses the build metadata block (same in DiP manifest and in catalog)
    bool ParseSoftwareBuildMetadataBlock(const QString& xmlData, const QString& dataSource);

    /// \brief Sets up the default feature flags for DiP Manifest versions <= 2.1.  
    /// 
    /// Feature flag reading was implemented for 2.2 and above.
    void SetFeatureFlagDefaultsBasedOnManifestVersion(VersionInfo manifestVersion, bool isDLC);

    /// \brief Arbitrary string describing the game version.
    VersionInfo mGameVersion;

    /// \brief If set, don't allow downloading if the OS version isn't greater than or equal to this.
    VersionInfo mMinimumRequiredOS;

    /// \brief If set, don't allow downloading if the Origin client version isn't equal to this.
    VersionInfo mRequiredClientVersion;

    /// \brief True if game should check for published updates.
    bool mbAutoUpdateEnabled;

    /// \brief True if game should use the ignore list when calculating updates.
    bool mbIgnoresEnabled;

    /// \brief True if game should use the delete list when applying an update.
    bool mbDeletesEnabled;

    /// \brief True if game should use the consolidated EULA UI flow.
    bool mbConsolidatedEULAsEnabled;

    /// \brief True if game should show a checkbox in the installation flow to let a user install shortcuts.
    bool mbShortcutsEnabled;

    /// \brief True if game should use the language specific excludes.
    bool mbLanguageExcludesEnabled;

    /// \brief True if game should use the language specific includes.  (DiP 4.0+)
    bool mbLanguageIncludesEnabled;

    /// \brief True if the content manager should use the gameVersion in the manifest to compare against the version in the entitlement response in checking whether an update has been published.
    bool mbUseGameVersionFromManifestEnabled;

    /// \brief True if game should always run the touchup installer after performing an update or repair.
    bool mbForceTouchupInstallerAfterUpdate;

    /// \brief True if game should be run elevated.
    bool mbExecuteGameElevated;

    /// \brief True if a detected update is mandatory. (i.e. don't allow launch before updating.)
    bool mbTreatUpdatesAsMandatory;

    /// \brief True if we can launch this content even if it's already playing (e.g. Sparta).
    bool mbAllowMultipleInstances;

    /// \brief True if differential updates are enabled (available starting DiP 3.0 - defaults to true for DiP 3.0 and false for < 3.0).
    bool mbEnableDifferentialUpdate;

    /// \brief True if dynamic download (progressive install) is enabled (available starting DiP 4.0 - defaults to false).
    bool mbDynamicContentSupportEnabled;

    /// \brief True if the game (all launchers) requires a 64-bit OS. (DiP 4.0+)
    bool mbGameRequires64BitOS;

    /// \brief True if the game supports changing its install language post-install.
    bool mbLanguageChangeSupportEnabled;

    /// \brief True if the DLC requires the parent (base game) to be up to date.
    bool mbDLCRequiresParentUpToDate;

    /// \brief If set, defines the lower bound of the range of game versions that Origin should automatically attempt to repair (DiP 4.0+)
    VersionInfo mAutoRepairVersionMin;

    /// \brief If set, defines the upper bound of the range of game versions that Origin should automatically attempt to repair (DiP 4.0+)
    VersionInfo mAutoRepairVersionMax;

    /// \brief If set, defines the minimum required version for online play.  (DiP 4.0+)
    VersionInfo mMinimumRequiredVersionForOnlineUse;
};

} //namespace Publishing

} //namespace Services

} //namespace Origin

#endif