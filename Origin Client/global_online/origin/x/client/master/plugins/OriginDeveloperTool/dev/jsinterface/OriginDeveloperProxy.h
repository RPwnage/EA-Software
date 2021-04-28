/////////////////////////////////////////////////////////////////////////////
// OriginDeveloperProxy.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef _ORIGINDEVELOPERPROXY_H
#define _ORIGINDEVELOPERPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QStringList>
#include <QVariantMap>

#include "engine/config/ConfigFile.h"
#include "services/plugin/PluginAPI.h"
#include "services/publishing/CatalogDefinition.h"
#include "TelemetryDebugView.h"

namespace Origin
{

namespace Plugins
{

namespace DeveloperTool
{
    class CalculateCrcHelper;
    
namespace JsInterface
{

class OriginDeveloperProxy : public QObject
{
    Q_OBJECT
public:
    OriginDeveloperProxy();
    ~OriginDeveloperProxy();

    // User's nucleus ID.
    Q_PROPERTY(qint64 userId READ userId);
    qint64 userId();

    // ODT Version
    Q_PROPERTY(QString odtVersion READ odtVersion);
    QString odtVersion();

    // Unowned IDs.
    Q_PROPERTY(QStringList unownedIds READ unownedIds);
    QStringList unownedIds();

    Q_INVOKABLE bool importFile();
    Q_INVOKABLE bool exportFile();
    Q_INVOKABLE void removeContent(QString id);

    Q_INVOKABLE QStringList cloudConfiguration(QString id);
    Q_INVOKABLE void clearRemoteArea(QString id);

    Q_INVOKABLE QVariantList softwareBuildMap(QString id);
    Q_INVOKABLE void downloadBuildInBrowser(QString offerId, QString version);
    Q_INVOKABLE void downloadCrc(QString offerId, QString version);
    Q_INVOKABLE void downloadBuildInstallerXml(QString offerId, QString version, QString localOverride);

    Q_INVOKABLE QString storeEnvironment();
    Q_INVOKABLE QVariant storeUrls(const QString& environment);
    
    Q_INVOKABLE QString selectFile(QString startingDirectory, QString filter, QString operation, QString telemetryType = "");
    Q_INVOKABLE QString selectFolder();
    
    Q_INVOKABLE void launchCodeRedemptionDebugger();
    Q_INVOKABLE void launchTelemetryLiveViewer();
    Q_INVOKABLE void launchTrialResetTool(const QString& contentId);

    Q_INVOKABLE void viewODTDocumentation();

    // Developer tools
    
    /// \brief Opens the client log file in a text editor.
    /// Convenience functionality to allow ODT users to quickly access the client log.
    Q_INVOKABLE void openClientLog();
    
    Q_INVOKABLE void openBootstrapperLog();

    /// \brief Forcefully restarts the client.
    Q_INVOKABLE void restartClient();

    /// \brief Performs ODT machine de-activation
    Q_INVOKABLE void deactivate();

private slots:
    void onUrlFetchForDownloadFinished();
    void onUrlFetchForManifestFinished();
    void onUrlFetchForCrcFinished();
    void onCrcFinished();

private:
    Q_DISABLE_COPY(OriginDeveloperProxy);

    bool fetchAndDisplayManifest(const QString& offerId, const QString& dipUrl);

    void getDownloadUrl(const QString& offerId, const QString& buildId, const char* myResultSlot);

    QVariantList convertSoftwareBuildMap(const Services::Publishing::SoftwareBuildMap &softwareBuilds);

    QSet<QString> mManifestFileCleanup;
    CalculateCrcHelper* mCrcHelper;
    TelemetryDebugView mTelemetryDebugView;
    QMap<QString, QVariantMap> mStoreUrls;

    /// \brief True if the user has modified any override that would require a restart during this session.
    bool mRestartRequired;
};

} // namespace JsInterface

} // namespace DeveloperTool

} // namespace Plugins

} // namespace Origin

#endif
