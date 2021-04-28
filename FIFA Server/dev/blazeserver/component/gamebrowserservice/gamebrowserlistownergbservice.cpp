/*! ************************************************************************************************/
/*!
\file gamebrowserlistownergbservice.cpp


\attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "component/gamebrowserservice/gamebrowserlistownergbservice.h"
#include "component/gamebrowserservice/getgameliststreaming_command.h"

namespace Blaze
{
namespace GameBrowserService
{
namespace v1
{
    
GameBrowserListOwnerGBService::GameBrowserListOwnerGBService(Blaze::GameManager::GameBrowserListId listId, GetGameListStreamingCommand* cmd)
    : IGameBrowserListOwner(Blaze::GameManager::GAMEBROWSERLIST_OWNERTYPE_GBSERVICE, listId)
    , mOwnerRpc(cmd)
{
    TRACE_LOG("[GameBrowserListOwnerGBService].ctor: created owner for listId: " << mListId);
}

GameBrowserListOwnerGBService::~GameBrowserListOwnerGBService()
{
    TRACE_LOG("[GameBrowserListOwnerGBService].dtor: destroyed owner for listId: " << mListId);
}

bool GameBrowserListOwnerGBService::canUpdate() const
{
    return true;
}

void GameBrowserListOwnerGBService::onUpdate(const Blaze::GameManager::NotifyGameListUpdate& listUpdate, uint32_t)
{
    if (mOwnerRpc != nullptr)
        mOwnerRpc->onListUpdate(listUpdate);
}

void GameBrowserListOwnerGBService::onDestroy()
{
    if (mOwnerRpc != nullptr)
        mOwnerRpc->onListDestroy();
}

} // namespace v1
} // namespace GameBrowserService
} // namespace Blaze
