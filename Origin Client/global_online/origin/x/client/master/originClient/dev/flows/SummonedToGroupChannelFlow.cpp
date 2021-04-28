#include "SummonedToGroupChannelFlow.h"

#include <QDebug>
#include <chat/OriginConnection.h>
#include <engine/social/SocialController.h>
#include <engine/social/ConversationManager.h>
#include <engine/igo/IGOController.h>
#include <services/debug/DebugService.h>
#include "services/rest/GroupServiceClient.h"
#include "chat/RemoteUser.h"
#include "ToVisibleFlow.h"


namespace Origin
{
namespace Client
{

SummonedToGroupChannelFlow::SummonedToGroupChannelFlow(Engine::UserRef user, const QString& from, const QString& groupId, const QString& channelId)
: mUser(user)
, mGroupId(groupId)
, mChannelId(channelId)
, mFromUserId(from)
, mFromUsername("")
, mChannelToJoin(NULL)
, mDialog(NULL)
{
    Engine::Social::SocialController* socialController = mUser->socialControllerInstance();
    Chat::OriginConnection* chatConnection = socialController->chatConnection();
    Chat::ConnectedUser* currentUser = chatConnection->currentUser();
    const Chat::JabberID currentUserJid = currentUser->jabberId().asBare();
    const Chat::JabberID fromUserJid(from, currentUserJid.domain());
    Chat::RemoteUser* remoteUser = chatConnection->remoteUserForJabberId(fromUserJid);
    if (remoteUser)
    {
        mFromUsername = remoteUser->originId();
    }
}

void SummonedToGroupChannelFlow::closeDialog()
{
    if(mDialog)
    {
        mDialog->deleteLater();
        mDialog = NULL;
    }
}

Chat::ChatGroups* SummonedToGroupChannelFlow::chatGroups() const
{
    Engine::Social::SocialController* socialController = mUser->socialControllerInstance();
    Chat::OriginConnection* chatConnection = socialController->chatConnection();
    Chat::ConnectedUser* currentUser = chatConnection->currentUser();
    return currentUser->chatGroups();
}

void SummonedToGroupChannelFlow::start()
{
    // If we are invisible bail immediately and let the invite just show up in their list
    Engine::Social::SocialController* socialController = mUser->socialControllerInstance();
    Chat::OriginConnection* chatConnection = socialController->chatConnection();
    Chat::ConnectedUser* currentUser = chatConnection->currentUser();
    if (currentUser->visibility() == Chat::ConnectedUser::Invisible)
    {
        emit succeeded();
        return;
    }

    // Handle the invitation
    Chat::ChatGroups* groups = chatGroups();
    if (groups)
    {
        Chat::ChatGroup* group = groups->chatGroupForGroupGuid(mGroupId);
        if (group)
        {
            // Awesome, we are already a member of the group.
            if (group->isInviteGroup())
            {
                // We already have an unaccepted invitation to this group
                onGroupAdded(group);
            }
            else
            {
                // User is already a subscribed member of the group - prompt them to join the conversation
                promptUserForGroup(group);
            }

            // Send inviteToChatGroup signal which will show notification
            emit socialController->inviteToChatGroup(mFromUserId, mGroupId);
        }
        else
        {
            // Not a member of the group - wait for the group invitation to come in
            ORIGIN_VERIFY_CONNECT(groups, SIGNAL(groupAdded(Origin::Chat::ChatGroup*)), this, SLOT(onGroupAdded(Origin::Chat::ChatGroup*)));
        }
    }
}

void SummonedToGroupChannelFlow::onGroupAdded(Origin::Chat::ChatGroup* group)
{
    if (group)
    {
        if (group->isInviteGroup() && mGroupId.compare(group->getGroupGuid()) == 0)
        {
            // Okay, the group invitation has arrived - prompt the user
            ORIGIN_VERIFY_DISCONNECT(chatGroups(), SIGNAL(groupAdded(Origin::Chat::ChatGroup*)), this, SLOT(onGroupAdded(Origin::Chat::ChatGroup*)));
            promptUserForGroup(group);
        }
    }
}

void SummonedToGroupChannelFlow::updateFrom(const QString& from)
{
    using namespace Origin::UIToolkit;

    Engine::Social::SocialController* socialController = mUser->socialControllerInstance();
    Chat::OriginConnection* chatConnection = socialController->chatConnection();
    Chat::ConnectedUser* currentUser = chatConnection->currentUser();
    const Chat::JabberID currentUserJid = currentUser->jabberId().asBare();
    const Chat::JabberID fromUserJid(from, currentUserJid.domain());
    Chat::RemoteUser* remoteUser = chatConnection->remoteUserForJabberId(fromUserJid);
    if (remoteUser)
    {
        mFromUsername = remoteUser->originId();

        Chat::ChatGroups* groups = chatGroups();
        if (groups)
        {
            Chat::ChatGroup* group = groups->chatGroupForGroupGuid(mGroupId);
            if (group)
            {
                QString dialogText = tr("ebisu_client_your_friend_has_invited_you_to_join_the_chat_room");
                Chat::ChatGroup* existingGroup = chatGroups()->chatGroupForGroupGuid(group->getGroupGuid());
                if (existingGroup && !existingGroup->isInviteGroup())
                {
                    dialogText = tr("ebisu_client_your_friend_has_invited_existing_member_to_join_the_chat_room");
                }

                if (mDialog)
                {
                    mDialog->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_you_have_been_invited_to_a_chat_room_title"),
                        dialogText.arg(mFromUsername).arg(group->getName()));
                }

                emit socialController->inviteToChatGroup(from, mGroupId);
            }
        }
    }
}

void SummonedToGroupChannelFlow::promptUserForGroup(Chat::ChatGroup* group)
{
    using namespace Origin::UIToolkit;

    const QString groupName = group->getName();

    
    if (!mDialog) 
    {
        // Can't use alertNonModal because it only returns accept|reject and we need accept|reject|close
        mDialog = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close,
            NULL, OriginWindow::MsgBox, QDialogButtonBox::Yes|QDialogButtonBox::No);

        QString dialogText = tr("ebisu_client_your_friend_has_invited_you_to_join_the_chat_room");
        Chat::ChatGroup* existingGroup = chatGroups()->chatGroupForGroupGuid(group->getGroupGuid());
        if (existingGroup && !existingGroup->isInviteGroup())
        {
            dialogText = tr("ebisu_client_your_friend_has_invited_existing_member_to_join_the_chat_room");
        }

        mDialog->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_you_have_been_invited_to_a_chat_room_title"),
            dialogText.arg(mFromUsername).arg(groupName));
        mDialog->setButtonText(QDialogButtonBox::Yes, tr("ebisu_client_action_accept_request"));
        mDialog->setButtonText(QDialogButtonBox::No, tr("ebisu_client_action_ignore_request"));
        mDialog->setDefaultButton(QDialogButtonBox::Yes);
        mDialog->manager()->setupButtonFocus();

        // Connect the Yes/No buttons clicked signals
        ORIGIN_VERIFY_CONNECT(mDialog->button(QDialogButtonBox::Yes), SIGNAL(clicked()), this, SLOT(onAccept()));
        ORIGIN_VERIFY_CONNECT(mDialog->button(QDialogButtonBox::No), SIGNAL(clicked()), this, SLOT(onIgnore()));

        ORIGIN_VERIFY_CONNECT(mDialog->button(QDialogButtonBox::Yes), SIGNAL(clicked()), mDialog, SLOT(accept()));
        ORIGIN_VERIFY_CONNECT(mDialog->button(QDialogButtonBox::No), SIGNAL(clicked()), mDialog, SLOT(reject()));

        // Handle closeWindow signal
        ORIGIN_VERIFY_CONNECT(mDialog, SIGNAL(closeWindow()), mDialog, SLOT(reject()));

        // Also need to listen to the group and see if the user clicks accept or ignore from the friends list and dismiss ourselves if so
        ORIGIN_VERIFY_CONNECT(group, SIGNAL(groupInviteAccepted()), mDialog, SLOT(reject()));
        ORIGIN_VERIFY_CONNECT(group, SIGNAL(groupInviteDeclined()), mDialog, SLOT(reject()));

        // In addition to dismissing ourselves, fire the handled signal so our parent can keep track of instances
        ORIGIN_VERIFY_CONNECT(group, SIGNAL(groupInviteAccepted()), this, SIGNAL(handled()));
        ORIGIN_VERIFY_CONNECT(group, SIGNAL(groupInviteDeclined()), this, SIGNAL(handled()));
        ORIGIN_VERIFY_CONNECT(mDialog, SIGNAL(closeWindow()), this, SIGNAL(handled()));

        ORIGIN_VERIFY_CONNECT(Services::Connection::ConnectionStatesService::instance(), SIGNAL(nowOfflineUser()), mDialog, SLOT(reject()));

        // In addition, listen for kicked from group, leaving group, group deleted
        ORIGIN_VERIFY_CONNECT(mUser->socialControllerInstance(), SIGNAL(kickedFromGroup(QString, QString, qint64)), mDialog, SLOT(reject()));
        ORIGIN_VERIFY_CONNECT(mUser->socialControllerInstance(), SIGNAL(groupDestroyed(QString, qint64, QString)), mDialog, SLOT(reject()));
        ORIGIN_VERIFY_CONNECT(chatGroups(), SIGNAL(groupRemoved(Origin::Chat::ChatGroup*)), this, SLOT(onGroupRemoved(Origin::Chat::ChatGroup*)));

        // Delete mDialog when finished
        ORIGIN_VERIFY_CONNECT(mDialog, SIGNAL(finished(int)), this, SLOT(closeDialog()));

    }

    if (Engine::IGOController::instance()->isAvailable())
    {
        mDialog->configForOIG();
        Engine::IGOController::instance()->igowm()->addPopupWindow(mDialog, Engine::IIGOWindowManager::WindowProperties());
    }
    else
    {
        mDialog->showWindow();
    }
}

void SummonedToGroupChannelFlow::onAccept()
{
    Chat::ChatGroups* groups = chatGroups();
    if (groups)
    {
        Chat::ChatGroup* group = groups->chatGroupForGroupGuid(mGroupId);
        if (group)
        {
            ORIGIN_VERIFY_DISCONNECT(group, SIGNAL(groupInviteAccepted()), this, SIGNAL(handled()));
            ORIGIN_VERIFY_DISCONNECT(group, SIGNAL(groupInviteDeclined()), this, SIGNAL(handled()));

            if (group->isInviteGroup())
            {
                // We already have an invitation to this group - we just need to accept it!
                ORIGIN_VERIFY_CONNECT(group, SIGNAL(groupInviteAccepted()), this, SLOT(onGroupInviteAcceptSuccess()));
                ORIGIN_VERIFY_CONNECT(group, SIGNAL(acceptInviteFailed()), this, SLOT(onGroupInviteAcceptFailed()));
                group->acceptGroupInvite();
            }
            else
            {
                // Find the correct channel
                Chat::ChatChannel* channel = group->channelForChannelId(mChannelId);
                if (channel)
                {
                    // Awesome we found the channel - join it now and we're done!
                    mChannelToJoin = channel;
                    ToVisibleFlow *flow = new ToVisibleFlow(mUser->socialControllerInstance());
                    ORIGIN_VERIFY_CONNECT(flow, SIGNAL(finished(const bool&)), this, SLOT(startJoinChannel(const bool&)));
                    flow->start();
                }
                else
                {
                    // Poke the group for channel information
                    onGroupInviteAcceptSuccess(group);
                }
            }
        }
    }
}

void SummonedToGroupChannelFlow::onIgnore()
{
    Chat::ChatGroups* groups = chatGroups();
    if (groups)
    {
        Chat::ChatGroup* group = groups->chatGroupForGroupGuid(mGroupId);
        if (group)
        {
            ORIGIN_VERIFY_DISCONNECT(group, SIGNAL(groupInviteAccepted()), this, SIGNAL(handled()));
            ORIGIN_VERIFY_DISCONNECT(group, SIGNAL(groupInviteDeclined()), this, SIGNAL(handled()));

            group->rejectGroupInvite();
            emit succeeded();
        }
    }
}

void SummonedToGroupChannelFlow::onGroupInviteAcceptSuccess(Chat::ChatGroup* group/*=NULL*/)
{
    // Now we need to ask for the group info
    // TODO - we might want to check the group's channel array and see if perhaps we already have it by some chance

    if (!group) group = dynamic_cast<Chat::ChatGroup*>(sender());
    ORIGIN_ASSERT(group);

    ORIGIN_VERIFY_DISCONNECT(group, SIGNAL(groupInviteAccepted()), this, SLOT(onGroupInviteAcceptSuccess()));
    ORIGIN_VERIFY_DISCONNECT(group, SIGNAL(acceptInviteFailed()), this, SLOT(onGroupInviteAcceptFailed()));

    ORIGIN_VERIFY_CONNECT(group, SIGNAL(groupChannelInformationReceived()), this, SLOT(onGroupChannelInformationReceived()));
    ORIGIN_VERIFY_CONNECT(group, SIGNAL(groupChannelGetInfoFailed()), this, SLOT(onGroupChannelGetInfoFailed()));

    group->getGroupChannelInformation();
}

void SummonedToGroupChannelFlow::onGroupChannelInformationReceived()
{
    Chat::ChatGroup* group = dynamic_cast<Chat::ChatGroup*>(sender());
    ORIGIN_ASSERT(group);

    ORIGIN_VERIFY_DISCONNECT(group, SIGNAL(groupChannelInformationReceived()), this, SLOT(onGroupChannelInformationReceived()));
    ORIGIN_VERIFY_DISCONNECT(group, SIGNAL(groupChannelGetInfoFailed()), this, SLOT(onGroupChannelGetInfoFailed()));

    Origin::Chat::ChatChannel* channel = group->channelForChannelId(mChannelId);
    if (channel)
    {
        mChannelToJoin = channel;
        ToVisibleFlow *flow = new ToVisibleFlow(mUser->socialControllerInstance());
        ORIGIN_VERIFY_CONNECT(flow, SIGNAL(finished(const bool&)), this, SLOT(startJoinChannel(const bool&)));
        flow->start();
    }
}

void SummonedToGroupChannelFlow::onGroupInviteAcceptFailed()
{
    Chat::ChatGroup* group = dynamic_cast<Chat::ChatGroup*>(sender());
    ORIGIN_ASSERT(group);

    ORIGIN_VERIFY_DISCONNECT(group, SIGNAL(groupInviteAccepted()), this, SLOT(onGroupInviteAcceptSuccess()));
    ORIGIN_VERIFY_DISCONNECT(group, SIGNAL(acceptInviteFailed()), this, SLOT(onGroupInviteAcceptFailed()));
    emit failed();
}

void SummonedToGroupChannelFlow::onGroupChannelGetInfoFailed()
{
    Chat::ChatGroup* group = dynamic_cast<Chat::ChatGroup*>(sender());
    ORIGIN_ASSERT(group);

    ORIGIN_VERIFY_DISCONNECT(group, SIGNAL(groupChannelInformationReceived()), this, SLOT(onGroupChannelInformationReceived()));
    ORIGIN_VERIFY_DISCONNECT(group, SIGNAL(groupChannelGetInfoFailed()), this, SLOT(onGroupChannelGetInfoFailed()));

    emit failed();
}

void SummonedToGroupChannelFlow::onGroupRemoved(Origin::Chat::ChatGroup* group)
{
    if (getGroupId() == group->getGroupGuid()) 
    {
        closeDialog();
        emit succeeded();
    }
}

void SummonedToGroupChannelFlow::onShowInviteAlertDialog(const QString& groupGuid)
{
    if (this->getGroupId() == groupGuid)
    {
        if (mDialog)
        {
            // Only raise the dialog if we are not in IGO
            if (!Origin::Engine::IGOController::instance()->igowm()->visible())
            {
                mDialog->showWindow();
                mDialog->raise();
            }
        }
    }
}
void SummonedToGroupChannelFlow::startJoinChannel(const bool& success)
{
    sender()->deleteLater();

    if (success && mChannelToJoin)
    {
        mChannelToJoin->setInvitedBy(mFromUsername);
        mChannelToJoin->joinChannel();
    }

    emit succeeded();
}

}
}


