///////////////////////////////////////////////////////////////////////////////
// ConfigIngestController.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _CONFIGFILECONTROLLER_H_
#define _CONFIGFILECONTROLLER_H_

#include <QObject>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{
namespace Config
{
/// \brief Checks for an EACore.ini file on the user's desktop and ingests it if desired.
class ORIGIN_PLUGIN_API ConfigIngestController : public QObject
{
	Q_OBJECT

public:
    enum ConfigIngestError
    {
        kConfigIngestNoError, ///< Success
        kConfigIngestDeleteBackupFailed, ///< Could not delete the existing EACore.ini.bak
        kConfigIngestDeleteConfigFailed, ///< Could not delete the desktop EACore.ini
        kConfigIngestRenameFailed, ///< Could not rename the existing EACore.ini to EACore.ini.bak
        kConfigIngestCopyFailed ///< Could not copy the desktop file to the Origin directory
    };

public:
    /// \brief Constructor
    /// \param parent The parent.
    ConfigIngestController(QObject* parent = NULL);

    /// \brief Destructor
	~ConfigIngestController();

    /// \brief Checks the desktop for an EACore.ini.
    /// \return True if an EACore.ini exists on the desktop.
    bool desktopConfigExists() const;

    /// \brief Backs up the user's EACore.ini and replaces it with the desktop one.
    /// \return The error encountered during the operation.
    ConfigIngestError ingestDesktopConfig() const;
    
    /// \brief Backs up the user's EACore.ini and deletes it.
    /// \return The error encountered during the operation.
    ConfigIngestError revertToDefaultConfig() const;
};

} // namespace Config
} // namespace Engine
} // namespace Origin

#endif
