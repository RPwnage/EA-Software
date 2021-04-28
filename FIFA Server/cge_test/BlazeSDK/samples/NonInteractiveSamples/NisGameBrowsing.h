
#ifndef NISGAMEBROWSING_H
#define NISGAMEBROWSING_H

#include "NisSample.h"

namespace NonInteractiveSamples
{

class NisGameBrowsing
    : public NisSample,
      protected Blaze::GameManager::GameManagerAPIListener,
      protected Blaze::GameManager::GameListener
{
    public:
        NisGameBrowsing(NisBase &nisBase);
        virtual ~NisGameBrowsing();

    protected:
        virtual void onCreateApis();
        virtual void onCleanup();

        virtual void onRun();

    protected:
        virtual void onPlayerTeamUpdated(Blaze::GameManager::Player *player, Blaze::GameManager::TeamIndex teamIndex) {}
        virtual void onGameTeamIdUpdated(Blaze::GameManager::Game *game, Blaze::GameManager::TeamId oldTeamId, Blaze::GameManager::TeamId newTeamId) {}

        virtual void onGameDestructing(Blaze::GameManager::GameManagerAPI* gameManagerAPI, Blaze::GameManager::Game* game, Blaze::GameManager::GameDestructionReason reason)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameManagerAPIListener::onGameDestructing\n");}

        virtual void onGameBrowserListUpdated(Blaze::GameManager::GameBrowserList* gameBrowserList,
        const Blaze::GameManager::GameBrowserList::GameBrowserGameVector* removedGames,
        const Blaze::GameManager::GameBrowserList::GameBrowserGameVector* updatedGames);

        virtual void onGameBrowserListUpdateTimeout(Blaze::GameManager::GameBrowserList* gameBrowserList);

        virtual void onGameBrowserListDestroy(Blaze::GameManager::GameBrowserList *gameBrowserList)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameManagerAPIListener::onGameBrowserListDestroy\n");}

        virtual void onMatchmakingScenarioFinished(Blaze::GameManager::MatchmakingResult matchmakingResult, 
        const Blaze::GameManager::MatchmakingScenario* matchmakingScenario, 
        Blaze::GameManager::Game* game)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameManagerAPIListener::onMatchmakingScenarioFinished\n");}

        virtual void onMatchmakingScenarioAsyncStatus(const Blaze::GameManager::MatchmakingScenario* matchmakingScenario,
        const Blaze::GameManager::MatchmakingAsyncStatusList* matchmakingAsyncStatusList)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameManagerAPIListener::onMatchmakingScenarioAsyncStatus\n");}

        virtual void onUserGroupJoinGame(Blaze::GameManager::Game* game, Blaze::GameManager::Player* player, Blaze::GameManager::UserGroupId groupId)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameManagerAPIListener::onUserGroupJoinGame\n");}

        virtual void onPlayerJoinGameQueue(Blaze::GameManager::Game *game)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameManagerAPIListener::onPlayerJoinGameQueue\n");}

        // Blaze::GameManager::Game state related methods
        virtual void onGameGroupInitialized(Blaze::GameManager::Game* game)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameListener::onGameGroupInitialized\n");}
        virtual void onPreGame(Blaze::GameManager::Game* game)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameListener::onPreGame\n");}
        virtual void onGameStarted(Blaze::GameManager::Game* game)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameListener::onGameStarted\n");}
        virtual void onGameEnded(Blaze::GameManager::Game* game)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameListener::onGameEnded\n");}
        virtual void onGameReset(Blaze::GameManager::Game* game)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameListener::onGameReset\n");}
        virtual void onReturnDServerToPool(Blaze::GameManager::Game* game)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameListener::onReturnDServerToPool\n");}

        // Blaze::GameManager::Game Settings and Attribute updates
        virtual void onGameAttributeUpdated(Blaze::GameManager::Game* game, const Blaze::Collections::AttributeMap * changedAttributeMap)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameListener::onGameAttributeUpdated\n");}
        virtual void onAdminListChanged(Blaze::GameManager::Game *game, const Blaze::GameManager::Player *adminPlayer, Blaze::GameManager::UpdateAdminListOperation operation, Blaze::GameManager::PlayerId updaterId)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameListener::onAdminListChanged\n");}
        virtual void onGameSettingUpdated(Blaze::GameManager::Game* game)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameListener::onGameSettingUpdated\n");}
        virtual void onPlayerCapacityUpdated(Blaze::GameManager::Game *game)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameListener::onPlayerCapacityUpdated\n");}

        // Blaze::GameManager::Player related methods
        virtual void onPlayerJoining(Blaze::GameManager::Player* player)
        {getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "BaseGameListener::onPlayerJoining %s\n", player->getName());}
        virtual void onPlayerJoiningQueue(Blaze::GameManager::Player *player)
        {getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "BaseGameListener::onPlayerJoiningQueue %s\n", player->getName());}
        virtual void onPlayerJoinComplete(Blaze::GameManager::Player* player)
        {getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "BaseGameListener::onPlayerJoinComplete %s\n", player->getName());}
        virtual void onQueueChanged(Blaze::GameManager::Game *game)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameListener::onQueueChanged\n");}
        virtual void onPlayerRemoved(Blaze::GameManager::Game *game, const Blaze::GameManager::Player *removedPlayer, 
        Blaze::GameManager::PlayerRemovedReason playerRemovedReason, Blaze::GameManager::PlayerRemovedTitleContext titleContext)
        {getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "BaseGameListener::onPlayerRemoved %s for Blaze reason %s\n", removedPlayer->getName(), PlayerRemovedReasonToString(playerRemovedReason));}
        virtual void onPlayerAttributeUpdated(Blaze::GameManager::Player* player, const Blaze::Collections::AttributeMap *changedAttributeMap)
        {getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "BaseGameListener::onPlayerAttributeUpdated %s\n", player->getName());}
        virtual void onPlayerCustomDataUpdated(Blaze::GameManager::Player* player)
        {getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "BaseGameListener::onPlayerCustomDataUpdated %s\n", player->getName());}

        // Networking(Host Migration) related methods
        virtual void onHostMigrationStarted(Blaze::GameManager::Game* game)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameListener::onHostMigrationStarted\n");}
        virtual void onHostMigrationEnded(Blaze::GameManager::Game* game)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameListener::onHostMigrationEnded\n");}
        virtual void onNetworkCreated(Blaze::GameManager::Game *game)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameListener::onNetworkCreated\n");}
        virtual void onPlayerJoinedFromQueue(Blaze::GameManager::Player *player)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameListener::onPlayerJoinedFromQueue\n");}

        /* TO BE DEPRECATED - will never be notified if you don't use GameManagerAPI::receivedMessageFromEndpoint() */
        virtual void onMessageReceived(Blaze::GameManager::Game *game, const void *buf, size_t bufSize,
            Blaze::BlazeId senderId, Blaze::BlazeNetworkAdapter::NetworkMeshAdapter::MessageFlagsBitset messageFlags) {};

        virtual void onUnresponsive(Blaze::GameManager::Game *game, Blaze::GameManager::GameState previousGameState)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameListener::onUnresponsive\n");}
        virtual void onBackFromUnresponsive(Blaze::GameManager::Game *game)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameListener::onBackFromUnresponsive\n");}

    private:
        void            CreateGameCb(Blaze::BlazeError error, Blaze::JobId jobId, Blaze::GameManager::Game* game);
        void            BrowseGames(void);
        void            CreateGameBrowserListCb(Blaze::BlazeError error, Blaze::JobId jobId, Blaze::GameManager::GameBrowserList* gameBrowserList, const char8_t* errDetails);

        void InitGameCompleteCb(Blaze::BlazeError err, Blaze::GameManager::Game * game)
        {
            getUiDriver().addLog(Pyro::ErrorLevel::NONE, "NisGameBrowsing::InitGameCb");
        }

};

}

#endif
