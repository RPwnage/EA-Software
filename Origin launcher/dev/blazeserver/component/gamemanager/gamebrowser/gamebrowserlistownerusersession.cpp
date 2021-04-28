/*! ************************************************************************************************/
/*!
\file gamebrowserlistownerusersession.cpp


\attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/usersessions/usersessionmaster.h"
#include "framework/usersessions/usersessionsmasterimpl.h"
#include "component/gamemanager/gamebrowser/gamebrowserlistownerusersession.h"
#include "component/gamemanager/gamebrowser/gamebrowser.h"

namespace Blaze
{
namespace GameManager
{
    
bool GameBrowserListOwnerUserSession::canUpdate() const 
{
    UserSessionMasterPtr ownerSession = gUserSessionMaster->getLocalSession(mUserSessionId);
    if (ownerSession == nullptr)
    {
        TRACE_LOG("[GameBrowserListOwnerUserSession].canUpdate: owner(" << mUserSessionId << ") of this GameBrowserList("
            << mListId << ") has logged out.");
        return false;
    }

    if (ownerSession->isBlockingNonPriorityNotifications())
    {
        TRACE_LOG("[GameBrowserListOwnerUserSession].canUpdate: owner(" << mUserSessionId << ") of this GameBrowserList("
            << mListId << ") is blocking non-priority notifications.");
        return false;
    }

    return true;
}

void GameBrowserListOwnerUserSession::onUpdate(const NotifyGameListUpdate& listUpdate, uint32_t msgNum)  
{
    mGameBrowser.sendNotifyGameListUpdateToUserSessionById(mUserSessionId, &listUpdate, false, msgNum);
}

void GameBrowserListOwnerUserSession::onDestroy() 
{
    // nothing to do, GameBrowser drives this for the GameBrowserListOwnerUserSession
}

} // namespace GameManager
} // namespace Blaze
