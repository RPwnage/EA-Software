/////////////////////////////////////////////////////////////////////////////
// CloudSyncViewController.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CLOUDSYNCVIEWCONTROLLER_H
#define CLOUDSYNCVIEWCONTROLLER_H

#include <QObject>
#include <QSharedPointer>
#include <QJsonObject>
#include "engine/content/ContentTypes.h"
#include "engine/cloudsaves/RemoteSync.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
class ORIGIN_PLUGIN_API CloudSyncViewController : public QObject
{
    Q_OBJECT

public:
    /// \brief Constructor
    /// \param parent - The parent of the CloudSyncViewController.
    CloudSyncViewController(Engine::Content::EntitlementRef entitlement, const bool& inDebug = false, QObject* parent = 0);

    /// \brief Destructor
    ~CloudSyncViewController();

    void showQuotaExceededFailure(const Origin::Engine::CloudSaves::SyncFailure& aFailure);
    void showNoSpaceFailure(const Origin::Engine::CloudSaves::SyncFailure& aFailure);
    void showManifestLockedFailure();
    void showErrorPlay();
    void showErrorRetry();
    void showCloudSync();
    bool isWindowShowing() const;

public slots:
    void showExternalSavesWarning();
    void showUntrustedRemoteFilesDetected();
    void showEmptyCloudWarning();
    void showSyncConflictDetected(const QDateTime& localLastModified, const QDateTime& remoteLastModified);
    void emitStopFlow();
    void onCommandbuttonsDone(QJsonObject);
    void onContinueOnAccept(QJsonObject);
    void onRetryPushOnAccept(QJsonObject);
    void onRetryPullOnAccept(QJsonObject);
    void onGlobalConnectionStateChanged(const bool&);
    void onCloudProgressChanged(const float& progress, const qint64& completedBytes, const qint64& totalBytes);
    void onRemoteSyncFinished();

signals:
	void stopFlow();
	void continueFlow();
    void retryPush();
    void retryPull();
    void disableCloudStorage();
    void stateChosen(const int& response);


private:
    QString rejectGroup() const;
    Engine::Content::EntitlementRef mEntitlement;
    const bool mInDebug;
    QString mWindowShowing;
};
}
}
#endif
