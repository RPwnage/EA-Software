///////////////////////////////////////////////////////////////////////////////
// InstallerFlow.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef CLIENTINSTALLERFLOW_H
#define CLIENTINSTALLERFLOW_H

#include <QMap>
#include <QSharedPointer>
#include "flows/AbstractFlow.h"
#include "engine/downloader/ContentInstallFlowContainers.h"
#include "engine/downloader/ContentInstallFlowState.h"
#include "engine/cloudsaves/RemoteSync.h"
#include "engine/content/ContentTypes.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
class InstallerViewController;

/// \brief Handles all high-level actions related to the installer flow.
class ORIGIN_PLUGIN_API InstallerFlow : public AbstractFlow
{
    friend class MenuFlowWindowViewController;
	Q_OBJECT

public:
    /// \brief The InstallerFlow constructor.
	InstallerFlow(Engine::Content::EntitlementRef entitlement);

    /// \brief The InstallerFlow destructor.
    virtual ~InstallerFlow();

    /// \brief Public interface for starting the installer flow.
	virtual void start();


protected slots:
    void onPresentInstallInfo(const Origin::Downloader::InstallArgumentRequest&);
    void onPresentEulaLanguageSelection(const Origin::Downloader::InstallLanguageRequest&);
    void onPresentEula(const Origin::Downloader::EulaStateRequest&);
    void onPresentCancelDialog(const Origin::Downloader::CancelDialogRequest&, const bool& updateOnly = false);
    void onInstallError(qint32, qint32, QString, QString);
    void onPresent3PDDInstallDialog(const Origin::Downloader::ThirdPartyDDRequest&);
    /// \brief Shows notification telling us that the game is playble
    void onPlayable();
    /// \brief Called when the content install flow cannot continue while the content is still running.  (For updates)
    void onPendingContentExit();
    void onStateChanged(Origin::Downloader::ContentInstallFlowState);
    void onInstallWarning(qint32 code, QString description);

    void onInstallArgumentsCreated(const QString& productId, Origin::Downloader::InstallArgumentResponse args);
    void onLanguageSelected(const QString& productId, Origin::Downloader::EulaLanguageResponse response);
    void onEulaStateDone(const QString& productId, Origin::Downloader::EulaStateResponse response);
    void onViewStopFlow();
    void onViewStopFlowForSelfAndDependents();
    void onFlowCanceled();
    void onFlowComplete();


    void onClearRTPLaunch();
    void onContinueInstall();
    void updateView();


signals:
    /// \brief Emitted when the user cancels an auto-update for an entitlement.
    /// \param productId The product ID that identifies the entitlement.
    void autoUpdateCanceled (const QString& productId);
    void finished(InstallerFlowResult result);

private:
    QList<Engine::Content::EntitlementRef> dependentActiveOperationChildren() const;

    ///< Pointer to the installer view controller.
    InstallerViewController* mVC;
    Engine::Content::EntitlementRef mEntitlement;
};
}
}
#endif // CLIENTINSTALLERFLOW_H