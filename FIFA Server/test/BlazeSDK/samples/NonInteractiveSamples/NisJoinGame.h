
#ifndef NISJOINGAME_H
#define NISJOINGAME_H

#include "NisSample.h"

namespace NonInteractiveSamples
{

class NisJoinGame 
    : public NisSample,
      protected Blaze::GameManager::GameManagerAPIListener
{
    public:
        NisJoinGame(NisBase &nisBase);
        virtual ~NisJoinGame();

    protected:
        virtual void onCreateApis();
        virtual void onCleanup();

        virtual void onRun();

    protected:
        virtual void onGameDestructing(Blaze::GameManager::GameManagerAPI* gameManagerAPI, Blaze::GameManager::Game* game, Blaze::GameManager::GameDestructionReason reason)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameManagerAPIListener::onGameDestructing\n");}

        virtual void onGameBrowserListDestroy(Blaze::GameManager::GameBrowserList *gameBrowserList)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameManagerAPIListener::onGameBrowserListDestroy\n");}

        virtual void onUserGroupJoinGame(Blaze::GameManager::Game* game, Blaze::GameManager::Player* player, Blaze::GameManager::UserGroupId groupId)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameManagerAPIListener::onUserGroupJoinGame\n");}

        virtual void onMatchmakingScenarioAsyncStatus(const Blaze::GameManager::MatchmakingScenario* matchmakingScenario,
        const Blaze::GameManager::MatchmakingAsyncStatusList* matchmakingAsyncStatusList)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameManagerAPIListener::onMatchmakingScenarioAsyncStatus\n");}

        virtual void onMatchmakingScenarioFinished(Blaze::GameManager::MatchmakingResult matchmakingResult, 
        const Blaze::GameManager::MatchmakingScenario* matchmakingScenario, 
        Blaze::GameManager::Game* game)
        {getUiDriver().addLog(Pyro::ErrorLevel::NONE, "BaseGameManagerAPIListener::onMatchmakingScenarioFinished\n");}

    private:
        void onGameBrowserListUpdated(Blaze::GameManager::GameBrowserList* gameBrowserList,
            const Blaze::GameManager::GameBrowserList::GameBrowserGameVector* removedGames,
            const Blaze::GameManager::GameBrowserList::GameBrowserGameVector* updatedGames);

        void onGameBrowserListUpdateTimeout(Blaze::GameManager::GameBrowserList* gameBrowserList);

        void createGameBrowserListCb(
            Blaze::BlazeError blazeError,
            Blaze::JobId jobId,
            Blaze::GameManager::GameBrowserList *gameBrowserList,
            const char8_t *errorMsg);

        void joinGameCb(Blaze::BlazeError blazeError, Blaze::JobId, Blaze::GameManager::Game *game, const char8_t *errorMsg);

};

}

#endif // NISJOINGAME_H
