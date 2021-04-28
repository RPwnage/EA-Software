/*! ************************************************************************************************/
/*!
    \file gamebrowserlistownergbservice.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEBROWSERSERVICE_V1_GAMEBROWSERLISTOWNER_GBSERVICE_H
#define BLAZE_GAMEBROWSERSERVICE_V1_GAMEBROWSERLISTOWNER_GBSERVICE_H

#include "component/gamemanager/gamebrowser/gamebrowserlistowner.h"
#include "framework/tdf/userdefines.h"

namespace Blaze
{
namespace GameBrowserService
{
namespace v1
{

class GetGameListStreamingCommand;

class GameBrowserListOwnerGBService : public Blaze::GameManager::IGameBrowserListOwner
{
public:
    GameBrowserListOwnerGBService(Blaze::GameManager::GameBrowserListId listId, GetGameListStreamingCommand* cmd);
    ~GameBrowserListOwnerGBService();
    bool canUpdate() const override;
    void onUpdate(const Blaze::GameManager::NotifyGameListUpdate& listUpdate, uint32_t)  override;
    void onDestroy() override;

    void clearOwner() { mOwnerRpc = nullptr; }
private:
    GetGameListStreamingCommand* mOwnerRpc;
};

} // namespace v1
} // namespace GameBrowserService
} // namespace Blaze

#endif // BLAZE_GAMEBROWSERSERVICE_V1_GAMEBROWSERLISTOWNER_GBSERVICE_H
