///////////////////////////////////////////////////////////////////////////////
// PlayFlow.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef CLIENTPLAYFLOW_H
#define CLIENTPLAYFLOW_H

#include "flows/AbstractFlow.h"
#include <QSharedPointer>
#include "engine/content/ContentTypes.h"
#include "PlayViewController.h"
#include "engine/content/localcontent.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{

/// \brief Handles all high-level actions related to playing a game.
class ORIGIN_PLUGIN_API PlayFlow : public AbstractFlow
{
    Q_OBJECT

public:
    /// \brief The PlayFlow constructor.
    /// \param productId The product ID of the game to play.
    PlayFlow(const QString& productId);

    /// \brief The PlayFlow destructor.
    virtual ~PlayFlow();

    /// \brief Public interface to start the play flow.
    virtual void start();

    /// \brief Sets whether or not we are playing without updates.
    /// \param without_updates True if we are playing without updates.
    void setPlayNowWithoutUpdates(const bool without_updates) { mWithoutUpdates = without_updates; }

    /// \brief Sets any launch parameters.
    /// \param launchParams The parameters to pass to the game executable.
    void setLaunchParameters (const QString& launchParams) {mLaunchParameters = launchParams;}

    bool isPlayGameWhileUpdatingWindowVisible() const {return mPlayGameWhileUpdatingWindowVisible;}

    void setCloudSyncFinished(const bool& proceedToPlay);

protected slots:

    /// \brief Slot that is triggered when a game executable launches.
    void onGameLaunched(Origin::Engine::Content::LocalContent::PlayResult result);

    /// \brief Slot that is triggered when a alternate executable or url launches.
    /// \param productId Product ID of the game that has launched the alternate exe or url
    /// \param alternateSoftwareId Software ID configured as an alternate launcher for this offer
    /// \param launchParams Parameters to launch alternate software with
    void onAlternateGameLaunched(const QString& productId, const QString &alternateSoftwareId, const QString &launchParams);

    /// \brief Slot that is triggered when the user cancels a game launch.
    void onGameLaunchCanceled();

    void onPromptUserPlayWhileUpdating();
    void onPlayGameWhileUpdatingAnswered(int result);
    void onUpdatePromptAnswered(const Origin::Client::PlayViewController::UpdateExistAnswers&);
    void onPlay3PDDGameAnswered(bool result);
    void onPdlcDownloadingWarningAnswered(int result);
    void onLocalStateChanged();
    void onInstallFlowStateChanged();
    void onRestoreBackupSaveFiles();

#if defined(ORIGIN_MAC)
    void onCheckIGOSettings();
    void onOigSettingsAnswer(int result, bool oigEnabled);
#endif

signals:
    /// \brief Emitted when the play flow has finished.
    /// \param result The result of the play flow.
    void finished(PlayFlowResult result);
    void successfulLaunch();

private:
    void sendCancelTelemetry();

    Engine::Content::EntitlementRef mEntitlement;
    PlayViewController* mPlayViewController; ///< Pointer to the play view controller.
    bool mWithoutUpdates; ///< True if the game is being played without updates.
    QString mLaunchParameters; ///< Any launch parameters to pass to the game executable.
    bool mPlayGameWhileUpdatingWindowVisible;
};

}
}
#endif // CLIENTPLAYFLOW_H
