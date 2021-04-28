#include "SummonToGroupChannelFlow.h"

#include <QDebug>
#include <chat/OriginConnection.h>
#include <engine/social/SocialController.h>
#include <engine/social/ConversationManager.h>
#include <engine/igo/IGOController.h>
#include <services/debug/DebugService.h>
#include "services/rest/GroupServiceClient.h"

#include "chat/ChatGroup.h"
#include "chat/ChatChannel.h"

#include "Qt/originwindow.h"                                                                                            
#include "Qt/originwindowmanager.h"  
    

namespace Origin
{
namespace Client
{
    
SummonToGroupChannelFlow::SummonToGroupChannelFlow(Chat::ChatChannel* channel, Engine::UserRef user, const Chat::JabberID& toJid, const QString& groupId, const QString& channelId)
: mChannel(channel)
, mUser(user)
, mToJid(toJid)
, mGroupId(groupId)
, mChannelId(channelId)
{
}

void SummonToGroupChannelFlow::start()
{
    // We can't bring someone into a group without a valid invitation, so first send a group invitation
    // This also covers the case where the remote user is offline - they will see the group invite when they log in next.
    Engine::Social::SocialController *socialController = mUser->socialControllerInstance();
    Origin::Services::GroupInviteMemberResponse* resp = Services::GroupServiceClient::inviteGroupMember(Services::Session::SessionService::currentSession(),
                                                        mGroupId,
                                                        QString::number(socialController->chatConnection()->jabberIdToNucleusId(mToJid)));

    // We need to wait for this response to finish before we do the rest (no pun intended).
    // Failure is expected if the user is already a member or has already been invited, so
    // on failure just proceed to send the invite anyway
    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(onGroupInviteSuccess()));
    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(failure()), this, SLOT(onGroupInviteSuccess()));
    resp->setDeleteOnFinish(true);
}

void SummonToGroupChannelFlow::onGroupInviteSuccess()
{
    Origin::Services::GroupInviteMemberResponse* resp = dynamic_cast<Origin::Services::GroupInviteMemberResponse*>(sender());
    ORIGIN_ASSERT(resp);

    ORIGIN_VERIFY_DISCONNECT(resp, SIGNAL(success()), this, SLOT(onGroupInviteSuccess()));
    ORIGIN_VERIFY_DISCONNECT(resp, SIGNAL(failure()), this, SLOT(onGroupInviteSuccess()));

    // The group invite completed - summon the user into the requested channel
    QString groupId = mChannel->getGroupGuid();
    QString channelId = mChannel->getChannelId();
    mChannel->inviteUser(mToJid, groupId, channelId);

    emit finished();
}


}
}
