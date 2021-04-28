#include "AcceptInviteAndOpenChannelFlow.h"

#include <QDebug>
#include <chat/OriginConnection.h>
#include <engine/social/SocialController.h>
#include <engine/social/ConversationManager.h>
#include <engine/igo/IGOController.h>
#include <services/debug/DebugService.h>
#include "services/rest/GroupServiceClient.h"

#include "Qt/originwindow.h"                                                                                            
#include "Qt/originwindowmanager.h"  
    

namespace Origin
{
namespace Client
{

AcceptInviteAndOpenChannelFlow::AcceptInviteAndOpenChannelFlow(Chat::ChatGroup* group)
    : mGroup(group)
{
}

void AcceptInviteAndOpenChannelFlow::start()
{
    if (mGroup && mGroup->isInviteGroup())
    {
        ORIGIN_VERIFY_CONNECT(mGroup, SIGNAL(groupInviteAccepted()), this, SLOT(onGroupInviteAcceptSuccess()));

        mGroup->acceptGroupInvite();        
    }

}

void AcceptInviteAndOpenChannelFlow::onGroupInviteAcceptSuccess()
{
    if (mGroup->getChannelCount() > 0) 
    {
        onChannelAdded(mGroup->getDefaultChannel());
    }
    else
    {
        mGroup->getGroupChannelInformation();
        ORIGIN_VERIFY_CONNECT(mGroup, SIGNAL(channelAdded(Origin::Chat::ChatChannel*)), this, SLOT(onChannelAdded(Origin::Chat::ChatChannel*)));
    }
}

void AcceptInviteAndOpenChannelFlow::onChannelAdded(Origin::Chat::ChatChannel* channel)
{
    if (channel)
    {
        mGroup->joinChannel(channel->getChannelId(), "", true);
    }
}

}
}
