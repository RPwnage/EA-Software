// *********************************************************************
//  SocialViewController.h
//  Copyright (c) 2012-2013 Electronic Arts Inc. All rights reserved.
// **********************************************************************

#ifndef _SOCIALVIEWCONTROLLER_H
#define _SOCIALVIEWCONTROLLER_H

#include <QObject>
#include <QString>
#include <QSet>

#include <engine/login/User.h>
#include <engine/multiplayer/Invite.h>
#include "ChatWindowManager.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace UIToolkit {
    class OriginWindow;
}

namespace Chat
{
class Conversable;
class RemoteUser;
}

namespace Client
{

/// \brief Controls high-level social flows not related to a particular widget
class ORIGIN_PLUGIN_API SocialViewController : public QObject
{
    Q_OBJECT
public:
    SocialViewController(QWidget* parent, Engine::UserRef user);
    virtual ~SocialViewController();

    /// \brief Blocks a Nucleus ID after requesting user confirmation
    /// \param nucleusId of the user the block is requested for
    void blockUserWithConfirmation(quint64 nucleusId);

    /// \brief Blocks a contact after requesting user confirmation
    /// \param remoteUser the block is requested for
    void blockContactWithConfirmation(const Chat::RemoteUser* remoteUser);

    /// \brief Blocks a Nucleus ID (should only be used by the Origin SDK)
    /// \param nucleusId of the user the block is requested for
    void blockUser(quint64 nucleusId);

    /// \brief Unsubscribes from a remote user after requesting user confirmation
    void removeContactWithConfirmation(const Chat::RemoteUser *);

    /// \brief Show the dialog to report a user for a violation
    void reportTosViolation(qint64 nucleusId, const QString& masterTitleId = "", Engine::IIGOCommandController::CallOrigin callOrigin = Engine::IIGOCommandController::CallOrigin_CLIENT);
    void reportTosViolation(const Chat::RemoteUser *);

    /// \brief Shows a dialog indicating we've entered an empty MUC room
    void showEnteredEmptyMucRoomDialog();

    /// \brief Returns a pointer to the chat window manager
    ChatWindowManager* chatWindowManager() { return mChatWindowManager; }
    
    /// \brief Sets the last notified conversation id
    void setLastNotifiedConversationId(QString id);

    /// \brief Returns the id of the last notified conversation
    QString findLastNotifiedId(){return mLastNotifiedConversationId;}
    
    /// \brief Returns the last notified conversation
    QSharedPointer<Engine::Social::Conversation> findLastNotifiedConversation();

    void acceptInviteFailed();
    void rejectInviteFailed();
    void createErrorDialog(const QString& heading, const QString& description, const QString& title);

    void onMessageGroupRoomInviteFailed();

    void showGroupInviteSentDialog(const QObjectList& users);


signals:
    void reportSubmitted();
    void conflictAccepted(const QString& channel);
    void conflictDeclined();
    void showInviteAlertDialog(const QString& groupGuid);

public slots:
    void doReportUserDialogDelete();
    void raiseMainChatWindow();
    void showChatWindowForFriend(QObject*);

    /// \brief Perform logic to display voice quality survey after a voice chat disconnects.
    /// \sa Conversation::displaySurvey
    void onDisplaySurvey();

    /// \brief Called when the user accepts or rejects the voice quality survey
    void onSurveyPromptClosed(const int& result);

    /// \brief Called when the user fails to enter a room
    void onFailedToEnterRoom();

    void onInviteToRoom(const QObjectList& users, const QString&, const QString&);

    /// \brief triggered from JS VoiceBridge to show a toasty for incoming calls
    void showToast_IncomingVoiceCall(const QString& originId, const QString& conversationId);

    /// \brief triggered from JS VoiceBridge to show a voice survey
    void sendSurveyPromptForConversation(QString const& voiceChatChannelId);

protected:
    void closeConflictWindow();
    //
    QString runningGameMasterTitle();

private slots:
    void onBlockDialogClosed();
    void onGroupRoomInviteReceived(const QString& from, const QString& groupId, const QString& channelId);
    void showInviteNotification(const QString& from, const QString& groupId);
    void onReportUserClosed();
    void onReportUser(quint64 /*mId*/, QString /*reasonStr*/, QString /*whereStr*/, QString /*masterTitle*/, QString /*comments*/);
    void onStartConversation(QSharedPointer<Origin::Engine::Social::Conversation>);
    void onInvitedToRemoteSession(const Origin::Engine::MultiplayerInvite::Invite &invite);
    void onContentLookupSucceeded();
    void onContentLookupFailed();
    // This is used as a callback function for MUC invite notifications
    void showChatWindowForConversation();
    void onGroupRoomInviteFinished();
    void onGroupRoomInviteFailed();
    void onGroupRoomInviteFailedAlertClosed();
#if ENABLE_VOICE_CHAT
    void onVoiceChatConflict(const QString& channel);
    void onConflictButtonClicked();
    void onCloseConflictWindow();
    void onVoiceUserTalking(const QString&);
    void onVoiceUserStoppedTalking(const QString&);
    void onVoiceLocalUserMuted();
    void onVoiceLocalUserUnMuted();
    void onGameAdded(uint32_t gameId);
    void onMutedByAdmin(QSharedPointer<Origin::Engine::Social::Conversation> , const Origin::Chat::JabberID&);
    void onVoiceClientDisconnectCompleted(bool joiningVoice);

    /// \brief Slot that gets called when a voice connection fails due to an error.
    void onVoiceConnectError(int connectError);

    /// \brief
    void onVoiceChatConnected(Origin::Engine::Voice::VoiceClient* voiceClient);

    /// \brief Slot that gets called when a voice chat ends
    void onVoiceChatDisconnected();

#endif
    void onEnterMucRoomFailed(Origin::Chat::ChatroomEnteredStatus);
    void acceptIncomingVoiceCall();
    void rejectIncomingVoiceCall();
    void onVoiceCallStateChanged(const QString& id, int newVoiceCallState);
    void onGroupChatMessageRecieved(QSharedPointer<Origin::Engine::Social::Conversation>, const Origin::Chat::Message &);
    void onGroupRoomDestroyed(QSharedPointer<Origin::Engine::Social::Conversation>, const QString&);
    void onGroupDestroyed(QString, qint64);
    void onKickedFromRoom(QSharedPointer<Origin::Engine::Social::Conversation>, const QString&);
    void onKickedFromGroup(QString, QString, qint64);
    void onInviteUserToRoomFinished();
    void onGroupRoomInviteHandled();
    void onInviteAlertClicked(QObject*);
    void onShowReportUser(qint64 target, const QString& masterTitle, Origin::Engine::IIGOCommandController::CallOrigin callOrigin);

private:
    /// \brief Deletes the survey window and sets the window pointer to NULL.
    void deleteSurveyWindow(); 

    Origin::Engine::MultiplayerInvite::Invite mInvite;
    Engine::UserRef mUser;
    ChatWindowManager* mChatWindowManager;
    QString mLastNotifiedConversationId;
    QMap<QObject *, quint64> mBlockWindowToUserId;
    QMap<QObject *, quint64> mReportWindowToUserId;
    UIToolkit::OriginWindow* mNoMoreFriendsAlert;
    QPointer<Origin::Engine::Voice::VoiceClient> mVoiceClient;
    bool mCurrentUserMuted;
    UIToolkit::OriginWindow* mVoiceConflictWindow;
    UIToolkit::OriginWindow* mSurveyPromptWindow;
    UIToolkit::OriginWindow* mGroupRoomInviteFailedAlert;
    QMap<QString, QObject*> mSummonedToRoomFlowList;
    bool mVoiceConflictHandled;
};

class ORIGIN_PLUGIN_API BlockReceiver : public QObject
{
    Q_OBJECT
public:
    BlockReceiver(QObject* parent, Engine::UserRef user, quint64 nucleusId);

public slots:
    void checkConnection();
    void performBlock();

private:
    Engine::UserRef mUser;
    quint64 mNucleusId;
};

}
}

#endif

