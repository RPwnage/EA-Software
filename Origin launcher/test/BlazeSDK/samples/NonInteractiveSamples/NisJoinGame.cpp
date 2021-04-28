
#include "NisJoinGame.h"

#include "EAAssert/eaassert.h"

using namespace Blaze::GameManager;

namespace NonInteractiveSamples
{

/*
  PLEASE READ:
  The SampleJoinGame sample can work in conjuction with the Integrated Sample.
  The Integrated Sample will create a game using the persona of the currently logged
  in user as the name of the game.  You can then set the JOIN_GAME_NAME to the persona
  used to sign in with the Integrated Sample.

  So, when you create a game with the Integrated Sample, the game will be created with
  a name e1qual to the persona of the currently logged in user.  Then, set the JOIN_GAME_NAME
  to the persona used to start the Integrated Sample, and this sample will then try to join that
  game.
*/
#define JOIN_GAME_NAME "nexus001"

NisJoinGame::NisJoinGame(NisBase &nisBase)
    : NisSample(nisBase)
{
}

NisJoinGame::~NisJoinGame()
{
}

void NisJoinGame::onCreateApis()
{
    getUiDriver().addDiagnosticCallFunc(getBlazeHub()->getGameManagerAPI()->addListener(this));
}

void NisJoinGame::onCleanup()
{
    getUiDriver().addDiagnosticCallFunc(getBlazeHub()->getGameManagerAPI()->removeListener(this));
}

//------------------------------------------------------------------------------
// Base class calls this.
void NisJoinGame::onRun()
{
    getUiDriver().addDiagnosticFunction();

    // create the new list
    Blaze::GameManager::GameBrowserList::GameBrowserListParameters gameListParams;
    gameListParams.mListConfigName = "default";
    gameListParams.mListCapacity   = Blaze::GameManager::GAME_LIST_CAPACITY_UNLIMITED;

    // Let's use the LIST_TYPE_SUBSCRIPTION type.  With this type we get updates via the listener
    gameListParams.mListType       = Blaze::GameManager::GameBrowserList::LIST_TYPE_SUBSCRIPTION;
    //gameListParams.mListType       = Blaze::GameManager::GameBrowserList::LIST_TYPE_SNAPSHOT;

    // These criteria are used to limit which games will show up in this GameBrowserList.
    gameListParams.mListCriteria.getPlayerCountRuleCriteria().setMinPlayerCount(1);
    gameListParams.mListCriteria.getPlayerCountRuleCriteria().setDesiredPlayerCount(8);
    gameListParams.mListCriteria.getPlayerCountRuleCriteria().setMaxPlayerCount(30);
    gameListParams.mListCriteria.getPlayerCountRuleCriteria().setRangeOffsetListName( "matchAny" );
    gameListParams.mListCriteria.getHostBalancingRulePrefs().setMinFitThresholdName( "matchAny" );
    gameListParams.mListCriteria.getHostViabilityRulePrefs().setMinFitThresholdName( "hostViability" );

    // Let's create the list
    getBlazeHub()->getGameManagerAPI()->createGameBrowserList(gameListParams,
        Blaze::GameManager::GameManagerAPI::CreateGameBrowserListCb(this, &NisJoinGame::createGameBrowserListCb));
}

// The callback we registered with createGameBrowserList
void NisJoinGame::createGameBrowserListCb(
    Blaze::BlazeError blazeError,
    Blaze::JobId jobId,
    Blaze::GameManager::GameBrowserList *gameBrowserList,
    const char8_t *errorMsg)
{
    getUiDriver().addDiagnosticCallback();

    // The list we're interested in will be provided to us in the onGameBrowserListUpdated
    // listener callback

    // Check for errors
    if (blazeError != Blaze::ERR_OK)
    {
        if (strlen(errorMsg) > 0)
        {
            getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "Error Message: %s", errorMsg);
        }

        reportBlazeError(blazeError);
    }
}

// This is an override of the GameManagerAPIListener method
void NisJoinGame::onGameBrowserListUpdated(Blaze::GameManager::GameBrowserList* gameBrowserList,
    const Blaze::GameManager::GameBrowserList::GameBrowserGameVector* removedGames,
    const Blaze::GameManager::GameBrowserList::GameBrowserGameVector* updatedGames)
{
    getUiDriver().addDiagnosticFunction();
    bool gameFound = false;
    // This isn't a typical use-case. But, for the purposes of this sample, we want to
    // join a game that has a name of JOIN_GAME_NAME.  Usually, you wouldn't even join a game
    // based on it's name.  But here, we're looking for a game that has the same name
    // as one created by our Integrated Sample, and we'll try to join that one.

    // Let's iterate through the list
    for (Blaze::GameManager::GameBrowserList::GameBrowserGameVector::iterator i = gameBrowserList->getGameVector().begin();
        i != gameBrowserList->getGameVector().end();
        i++)
    {
        Blaze::GameManager::GameBrowserGame *game = (*i);

        // If this game is the one we're looking for, then join it.
        if (strcmp(game->getName(), JOIN_GAME_NAME) == 0)
        {
            gameFound = true;
            getBlazeHub()->getGameManagerAPI()->joinGameById(game->getId(),
                Blaze::GameManager::JOIN_BY_BROWSING, Blaze::GameManager::GameManagerAPI::JoinGameCb(this, &NisJoinGame::joinGameCb));
        }
    }

    if (!gameFound)
    {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "Game not found.");
        done();
    }
}

void NisJoinGame::onGameBrowserListUpdateTimeout(Blaze::GameManager::GameBrowserList* gameBrowserList)
{
    getUiDriver().addDiagnosticFunction();
    getUiDriver().addLog(Pyro::ErrorLevel::ERR, "Timeout fetching game snapshot list");
}

// Called on success/failure of our call to joinGameById
void NisJoinGame::joinGameCb(Blaze::BlazeError blazeError, Blaze::JobId, Blaze::GameManager::Game *game, const char8_t *errorMsg)
{
    getUiDriver().addDiagnosticCallback();

    // Check for errors
    if (blazeError == Blaze::ERR_OK)
    {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "Game joined successfully");
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "Players:");

        // List all the players currently in this game
        for (uint16_t a = 0; a < game->getPlayerCount(); a++)
        {
            getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "    %s", game->getPlayerByIndex(a)->getName());
        }

        done();
    }
    else
    {
        if (strlen(errorMsg) > 0)
        {
            getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "Error Message: %s", errorMsg);
        }

        // Report any errors
        reportBlazeError(blazeError);
    }
}

}
