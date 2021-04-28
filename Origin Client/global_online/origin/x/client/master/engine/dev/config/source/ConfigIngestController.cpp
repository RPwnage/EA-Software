/////////////////////////////////////////////////////////////////////////////
// ConfigIngestController.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "engine/config/ConfigIngestController.h"
#include "services/platform/PlatformService.h"

#include "TelemetryAPIDLL.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QTextStream>

namespace Origin
{
namespace Engine
{
namespace Config
{

ConfigIngestController::ConfigIngestController(QObject *parent)
: QObject(parent)
{
}


ConfigIngestController::~ConfigIngestController()
{
}

QString desktopConfigPath()
{
    return Origin::Services::PlatformService::getStorageLocation(QStandardPaths::DesktopLocation) + QDir::separator() + QString("EACore.ini");
}

bool ConfigIngestController::desktopConfigExists() const
{
    return QFile::exists(desktopConfigPath());
}

ConfigIngestController::ConfigIngestError ConfigIngestController::ingestDesktopConfig() const
{
    QString eaCoreClientPath = Origin::Services::PlatformService::eacoreIniFilename();
    QFile eaCoreDesktop(desktopConfigPath());
    if (!eaCoreDesktop.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return kConfigIngestCopyFailed;
    }

    QByteArray desktopContents = eaCoreDesktop.readAll();
    eaCoreDesktop.close();

    ConfigIngestError error = revertToDefaultConfig();
    if (error != kConfigIngestNoError)
    {
        return error;
    }

    // Either there was no .ini, or we backed it up successfully.
    // Can't just rename in case Origin is installed on a
    // different drive than the one holding the desktop folder.
    QFile eaCoreClient(eaCoreClientPath);
    if (!eaCoreClient.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        return kConfigIngestCopyFailed;
    }

    QTextStream stream(&eaCoreClient);
    stream << desktopContents;
    eaCoreClient.close();

    if (!eaCoreDesktop.remove())
    {
        GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("ConfigIngestController_ingestDesktopConfig", eaCoreClientPath.toUtf8().data(), eaCoreDesktop.error());
        return kConfigIngestDeleteConfigFailed;
    }

    return kConfigIngestNoError;
}

ConfigIngestController::ConfigIngestError ConfigIngestController::revertToDefaultConfig() const
{
    QString eaCoreClientPath = Origin::Services::PlatformService::eacoreIniFilename();
    QFile eaCoreClient(eaCoreClientPath);

    if (eaCoreClient.exists())
    {
        QString eaCoreIniBakFilePath = Services::PlatformService::commonAppDataPath() + "EACore.ini.bak";
        QFile eaCoreBak(eaCoreIniBakFilePath);

        // We only keep one generation of backup.
        if (eaCoreBak.exists() && !eaCoreBak.remove())
        {
            GetTelemetryInterface()->Metric_ERROR_DELETE_FILE("ConfigIngestController_revertToDefaultConfig", eaCoreIniBakFilePath.toUtf8().data(), eaCoreBak.error());
            return kConfigIngestDeleteBackupFailed;
        }

        // Either there was no .bak, or we deleted it successfully.
        if (!eaCoreClient.copy(eaCoreIniBakFilePath))
        {
            return kConfigIngestRenameFailed;
        }

        // Clear EACore.ini contents
        eaCoreClient.resize(0);
    }

    return kConfigIngestNoError;
}

} // Config
} // Engine
} // Origin
