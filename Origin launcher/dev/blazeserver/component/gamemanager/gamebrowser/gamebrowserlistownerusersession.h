/*! ************************************************************************************************/
/*!
    \file gamebrowserlistownerusersession.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAME_BROWSER_LIST_OWNER_USERSESSION_H
#define BLAZE_GAMEMANAGER_GAME_BROWSER_LIST_OWNER_USERSESSION_H

#include "component/gamemanager/gamebrowser/gamebrowserlistowner.h"
#include "framework/tdf/userdefines.h"

namespace Blaze
{
namespace GameManager
{

class GameBrowser;
class GameBrowserListOwnerUserSession : public IGameBrowserListOwner
{
public:
    GameBrowserListOwnerUserSession(GameBrowserListId listId, GameBrowser& gameBrowser, UserSessionId userSessionId, uint32_t msgNum)
        : IGameBrowserListOwner(GAMEBROWSERLIST_OWNERTYPE_USERSESSION, listId)
        , mGameBrowser(gameBrowser)
        , mUserSessionId(userSessionId)
        , mMsgNum(msgNum)
    {

    }

    bool canUpdate() const override;
    void onUpdate(const NotifyGameListUpdate& listUpdate, uint32_t msgNum)  override;
    void onDestroy() override;

    UserSessionId getOwnerSessionId() const
    {
        return mUserSessionId;
    }

    uint32_t getMsgNum() const
    {
        return mMsgNum;
    }
private:
    GameBrowser& mGameBrowser;
    UserSessionId mUserSessionId;
    uint32_t mMsgNum;
};
    
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_GAME_BROWSER_LIST_OWNER_USERSESSION_H
