/////////////////////////////////////////////////////////////////////////////
// PlayViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef PLAYVIEWCONTROLLER_H
#define PLAYVIEWCONTROLLER_H

#include <QWidget>
#include <QSharedPointer>
#include <QTimer>
#include <QJsonObject>
#include "engine/content/ContentTypes.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
class ORIGIN_PLUGIN_API PlayViewController : public QObject
{
    Q_OBJECT

public:
    enum UpdateExistAnswers
    {
        PlayWithoutUpdate,
        PlayWhileUpdating,
        UpdateCancelLaunch,
        CancelLaunch
    };


    PlayViewController(const QString& productId, const bool& inDebug = false, QObject *parent = 0);
    ~PlayViewController();

    /// \brief Shown a dialog that is used for external launched games.
    void showOIGSettings();

public slots:
    void onUpdatePromptDone(QJsonObject);
    void onUpdateRequiredPromptDone(QJsonObject);
    void onPdlcDownloadingWarningAnswer(QJsonObject);
    void onStartingTimedTrialDone(QJsonObject);
    void onPlayGameWhileUpdatingDone(QJsonObject);
    void show3PDDPlayDialog();
    void showPDLCDownloadingWarning();
    void showStartingLimitedTrialDialog();
    void showPDLCDownloadsComplete(const QString& overrideId = "");
    void showStartingTimedTrialDialog();
    void showFirstLaunchMessageDialog();
    void showOnlineRequiredForTrialDialog();
    void showGameUpdatingWarning();
    void showMultipleLaunchDialog();
    void showDefaultLaunchDialog();
    void showInstallFlowResuming();
    void showUpdateCompleteDialog(const QString& overrideId = "");
    void showUpdateRequiredDialog();
    void showUpdateExistsDialog();
    void showUpgradeSaveGameWarning();
    void showPluginIncompatible();
    void showSystemRequirementsNotMet(const QString& info);
    void closeWindow();
    void emitStopFlow();
    void onMultiLaunchTitleChoosen(QJsonObject);
    void onOigSettingsDone(QJsonObject);
    void on3PDDPlayDone(QJsonObject);
    
signals:
    void launchQueueEmpty();
    void launchTitleSelected(const QString& title);
    void stopFlow();
    void trialCheckComplete();
    void updateExistAnswer(const Origin::Client::PlayViewController::UpdateExistAnswers&);
    void play3PDDGameAnswer(bool);
    void playGameWhileUpdatingAnswer(int);
    void oigSettingsAnswer(int result, bool oigEnabled);
    void pdlcDownloadingWarningAnswer(int);
    void upgradeSaveWarningComplete();
    void firstLaunchMessageComplete();

private slots:
    void showDownloadDebugDialog();
    void showUpdateDebugDialog();
    void onLinkActivated(const QString& link);
    void onUpgradeSaveGameWarningDone(const int& result);
    void onFirstLaunchMessageComplete(const int& result);
#ifdef Q_OS_WIN 
    void onFlashTimer();
#endif
    void onUpdateDownloadProgress(const QString&, const QJsonObject&);

private:
    QString determineTitle();
    QString rejectGroup() const;
    
    Engine::Content::EntitlementRef mEntitlement;
    const bool mInDebug;
#ifdef Q_OS_WIN 
    bool mFlashState;
    QTimer mFlashTimer;
#endif
};
}
}
#endif