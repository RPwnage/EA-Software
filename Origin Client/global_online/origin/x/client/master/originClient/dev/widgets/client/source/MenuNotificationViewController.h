/////////////////////////////////////////////////////////////////////////////
// MenuNotificationViewController.h
//
// Copyright (c) 2014, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef MENMUNOTIFICATIONVIEWCONTROLLER_H
#define MENMUNOTIFICATIONVIEWCONTROLLER_H

#include <QObject>

#include "services/plugin/PluginAPI.h"

class QMenu;
class QAction;

namespace Origin
{

namespace Client
{

/// \brief Controller for 
class ORIGIN_PLUGIN_API MenuNotificationViewController : public QObject
{
    Q_OBJECT

public:
    /// \brief Constructor
    /// \param parent - The parent of the View Controller.
    MenuNotificationViewController(QObject* parent = 0);

    /// \brief Destructor
    ~MenuNotificationViewController();
    
    QMenu* menu() const {return mMenu;}

signals:
    void nextStep();

private slots:

    // Notification options
    void showToasts();
    void showFinishDownloadToast();
    void showReadyToPlayToast();
	void showReadyToInstallUpdateToast();
    void showPDLCRestartGameToast();
    void showPDLCReloadSaveToast();
    void showPDLCHotDeployToast();
    void showDynamicContentPlayableToast();
    void showDownloadErrorToast();
    void showFriendsSigningInToast();
    void showFriendsSigningOutToast();
    void showFriendsStartGameToast();
    void showFriendsQuitGameToast();
    void showFriendsStartBroadcastToast();
    void showFriendsStopBroadcastToast();
    void showIncomingTextChatToast();
	void showLongIncomingTextChatToast();
    void showIGOToast();
    void showAchievementToast();
    void showBroadcastStoppedToast();
    void showBroadcastNotPermittedToast();
    void showIncommingChatRoomMessageToast();
    void showInviteToChatRoomToast();
    void showKickedOutOfGroupToast();
    void showGameInviteToast();
    void showKoreanTimerSingular();
    void showKoreanTimerPlural();
    void showGameTimeSingular();
    void showGameTimePlural();
    void showUnPinToast();
    void showRePinToast();
#if ENABLE_VOICE_CHAT
    void showIncommingVoIPCallToast();
    void showVoIPToast();
    void show3VoIPToast();
    void showUserVoIPToast();
    void showUserMuted();
    void removeVoIPToast();
    void remove3VoIPToast();
    void removeUserVoIPToast();
    void removeUserMuted();
#endif
    
    // delayed toasts
    void delayedShowToasts();
	void delayedFinishDownloadToast();
	void delayedShowReadyToPlayToast();
    void delayedShowReadyToInstallUpdateToast();
    void delayedShowPDLCRestartGameToast();
    void delayedShowPDLCReloadSaveToast();
    void delayedShowPDLCHotDeployToast();
    void delayedShowDynamicContentPlayableToast();
    void delayedDownloadErrorToast();
    void delayedFriendsSigningInToast();
    void delayedFriendsSigningOutToast();
    void delayedFriendsStartGameToast();
    void delayedFriendsQuitGameToast();
    void delayedFriendsStartBroadcastToast();
    void delayedFriendsStopBroadcastToast();
    void delayedIncomingTextChatToast();
	void delayedLongIncomingTextChatToast();
	void delayedIGOToast();
    void delayedAchievementToast();
    void delayedBroadcastStoppedToast();
    void delayedBroadcastNotPermittedToast();
    void delayedShowIncommingChatRoomMessageToast();
    void delayedShowInviteToChatRoomToast();
    void delayedShowKickedOutOfGroupToast();
    void delayedShowGameInviteToast();
    void delayedShowKoreanTimerSingular();
    void delayedShowKoreanTimerPlural();
    void delayedShowGameTimeSingular();
    void delayedShowGameTimePlural();
    void delayedShowUnPinToast();
    void delayedShowRePinToast();
#if ENABLE_VOICE_CHAT
  	void delayedShowIncommingVoIPCallToast();
    void delayedShowVoIPToast();
    void delayedShow3VoIPToast();
    void delayedShowUserVoIPToast();
    void delayedShowUserMuted();
    void delayedRemoveVoIPToast();
    void delayedRemove3VoIPToast();
    void delayedRemoveUserVoIPToast();
    void delayedRemoveUserMuted();
#endif

    // Slots for Incoming call testing 
    void onAccepted();
    void onRejected();
    void onClicked();
    
private:
    void addAction(QAction* newAction, const QString& title, const char* slot);
    void addAction(QMenu* menuParent, QAction* newAction, const QString& title, const char* slot);

    const QString mProductID;
    QMenu* mMenu;
};

}
}
#endif
