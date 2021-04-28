///////////////////////////////////////////////////////////////////////////////
// SharedNetworkOverride.h
//
// Copyright (c) 2015 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _SHAREDNETWORKOVERRIDE_H_
#define _SHAREDNETWORKOVERRIDE_H_

#include "engine/content/Entitlement.h"

namespace Origin
{
namespace Engine
{
namespace Shared
{

// Shared Network Override (SNO) is a feature (configured in ODT or EACore.ini) that allows users to designate a network (or local) folder
// from which download overrides can be automatically set for a particular title, when new builds appear in the folder.  This feature was a 
// game team request, to assist their QA by automating the process of setting new download overrides and updating, whenever a new build is
// available.  Core functionality: setup a SNO for a title in ODT; a new build is pushed to the folder; SNO updates the download override
// (in client memory, but not in the EACore.ini) to use that new build; SNO kicks off an update for that title.
class ORIGIN_PLUGIN_API SharedNetworkOverride : public QObject
{
	Q_OBJECT

public:
    SharedNetworkOverride(Content::EntitlementRef ent, QString folderPath, QString updateCheck, QString time, bool confirm, QObject* parent = 0);

    void start();

    const QString& folderPath() const { return m_folderPath; }
    const QString& updateCheck() const { return m_updateCheck; }
    const QString& time() const { return m_time; }
    const bool confirm() const { return m_confirm; }
    const QString& downloadOverride() const { return m_downloadOverridePath; }
    const QString& pendingOverride() const { return m_pendingOverridePath;  }

signals:
    void showConfirmWindow(Engine::Content::EntitlementRef ent, const QString overridePath, const QString lastModifiedTime);
    void showFolderErrorWindow(Content::EntitlementRef ent, const QString& folderPath);
    void showInvalidTimeWindow(Content::EntitlementRef ent);

public slots:
    // For the "confirm build" window
    void onConfirmAccepted(const QString& offerId);
    void onConfirmRejected(const QString& offerId);

    // For the "inaccessible network folder" error window
    void onErrorWindowRetry(const QString& offerId);
    void onErrorWindowDisable(const QString& offerId);

private slots:
    // Check the network folder for a new build to set a download override for
    void pollFolder();

    void onAsyncFolderCheckFinished();

private:
    // Updates the download override (in memory) and kicks off an update for the title.
    void updateDownloadOverride();
    
    // Check if the network folder is accessible on a background thread; inaccessible network folders may hang the main thread.
    bool asyncFolderCheck() const;

    QFuture<bool> m_folderCheckOperation;
    QFutureWatcher<bool> m_folderCheckOperationWatcher;

    Content::EntitlementRef m_entitlement;
    
    // The network location where we will check for new builds to set download overrides to.
    QString m_folderPath;
    
    // Either "latest" or "scheduled".  Are we always trying to set overrides for the most recent build,
    // or are we only checking for new builds at a scheduled time.
    QString m_updateCheck;
    
    // If we're doing "scheduled" update checking, then this is the time we check for new builds.
    QString m_time;

    // Do we prompt the user with a confirmation window before setting a new download override?
    bool m_confirm;

    // The currently set download override path for this SNO
    QString m_downloadOverridePath;
    QString m_pendingOverridePath;
    QTime m_parsedTime;
    
    // Timer to trigger polling of the SNO folder
    QTimer *m_timer;
    bool m_pollInProgress;

    // We keep track of builds that were rejected by the user in the confirmation window, so we don't repeatedly prompt for them.
    QSet<QString> m_rejectedDownloadOverrides;
};

} // namespace Shared
} // namespace Engine
} // namespace Origin

#endif
