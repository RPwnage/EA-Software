/////////////////////////////////////////////////////////////////////////////
// CloudSyncFlow.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CLOUDSYNCFLOW_H
#define CLOUDSYNCFLOW_H

#include "flows/AbstractFlow.h"
#include <QSharedPointer>
#include "engine/content/ContentTypes.h"
#include "engine/cloudsaves/RemoteSync.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
class CloudSyncViewController;

/// \brief TBD
class ORIGIN_PLUGIN_API CloudSyncFlow : public AbstractFlow
{
    Q_OBJECT

public:
    CloudSyncFlow(Engine::Content::EntitlementRef entitlement);
    ~CloudSyncFlow();

    void start();

    /// \brief Returns if we should try to do a cloud sync.
    static bool canPullCloudSync(const QString& productID);

public slots:
	void flowFailed();

	void flowSuccess();

signals:
    /// \brief Emitted when there is a cloud sync error.
    void cloudSyncFailed();
    /// \brief Emitted when we want to hide the cloud sync launch dialogs content.
    void showContent(const bool& showContent);
	/// \brief Emitted when the flow is complete.
	void finished(CloudSyncFlowResult);


private slots:
    void onPullFailed(Origin::Engine::CloudSaves::SyncFailure);
    void onPushFailed(Origin::Engine::CloudSaves::SyncFailure, QString, QString);
    void onDisableCloud();
    /// \brief Calls to the server to start a cloud push
    void startPush();
    void startPull();

private:
    void finish();
	
    Engine::Content::EntitlementRef mEntitlement;
    CloudSyncViewController* mVC;
};
}
}
#endif