
#include "NisGameBrowsing.h"

#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/loginmanager/loginmanager.h"

namespace NonInteractiveSamples
{

NisGameBrowsing::NisGameBrowsing(NisBase &nisBase)
    : NisSample(nisBase)
{
}

NisGameBrowsing::~NisGameBrowsing()
{
}

void NisGameBrowsing::onCreateApis()
{
    getUiDriver().addDiagnosticCallFunc(getBlazeHub()->getGameManagerAPI()->addListener(this));
}

void NisGameBrowsing::onCleanup()
{
    getUiDriver().addDiagnosticCallFunc(getBlazeHub()->getGameManagerAPI()->removeListener(this));
}

//------------------------------------------------------------------------------
// Create a peer-hosted game
void NisGameBrowsing::onRun()
{
    if (!getBlazeHub()->getConnectionManager()->initialQosPingSiteLatencyRetrieved())
    {
        getBlazeHub()->getScheduler()->scheduleMethod("NisGameBrowsingOnRun", this, &NisGameBrowsing::onRun);
        return;
    }

    getUiDriver().addDiagnosticFunction();

    Blaze::GameManager::Game::CreateGameParameters params;
    params.mGameName            = getBlazeHub()->getLoginManager(0)->getPersonaName();
    params.mNetworkTopology     = Blaze::CLIENT_SERVER_PEER_HOSTED;
    params.mVoipTopology        = Blaze::VOIP_DISABLED;
    params.mMaxPlayerCapacity   = 4;
    params.mPlayerCapacity[Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT] = 4;

    // We'll allow all methods of joining this game, even though some are not available yet.
    params.mGameSettings.setOpenToBrowsing();
    params.mGameSettings.setOpenToMatchmaking();
    params.mGameSettings.setOpenToInvites();
    params.mGameSettings.setOpenToJoinByPlayer();
    params.mGameSettings.clearHostMigratable(); // Not available yet so we'll disable

    params.mGameSettings.setRanked();

    params.mGameAttributes["Mode"] = "tdeathmatch";
    params.mGameAttributes["Map"] = "map_1";

    getBlazeHub()->getGameManagerAPI()->createGame(params, Blaze::GameManager::GameManagerAPI::CreateGameCb(this, &NisGameBrowsing::CreateGameCb));
}

//------------------------------------------------------------------------------
// The result of the createGame call. If successful, we'll put the game into PRE_GAME and browse for it.
void NisGameBrowsing::CreateGameCb(Blaze::BlazeError error, Blaze::JobId jobId, Blaze::GameManager::Game* game)
{
    getUiDriver().addDiagnosticCallback();

    if (error == Blaze::ERR_OK)
    {
        getUiDriver().addDiagnosticCallFunc(game->addListener(this));

        getUiDriver().addDiagnosticCallFunc(
            game->initGameComplete(
                Blaze::GameManager::Game::ChangeGameStateCb(Blaze::MakeFunctor(this, &NisGameBrowsing::InitGameCompleteCb))));

        // Now let's browse for this game
        BrowseGames();
    }
    else
    {
        reportBlazeError(error);
    }
}

//------------------------------------------------------------------------------
// Fire off a single browse request to get the list of games; should include ours
void NisGameBrowsing::BrowseGames()
{
    getUiDriver().addDiagnosticFunction();

    Blaze::GameManager::GameBrowserList::GameBrowserListParameters gameListParams;
    gameListParams.mListConfigName = "default";
    gameListParams.mListCapacity   = Blaze::GameManager::GAME_LIST_CAPACITY_UNLIMITED;
    gameListParams.mListType       = Blaze::GameManager::GameBrowserList::LIST_TYPE_SNAPSHOT;
    gameListParams.mListCriteria.getPlayerCountRuleCriteria().setMinPlayerCount(1);
    gameListParams.mListCriteria.getPlayerCountRuleCriteria().setDesiredPlayerCount(8);
    gameListParams.mListCriteria.getPlayerCountRuleCriteria().setMaxPlayerCount(16);
    gameListParams.mListCriteria.getPlayerCountRuleCriteria().setRangeOffsetListName("matchAny");
    gameListParams.mListCriteria.getHostBalancingRulePrefs().setMinFitThresholdName("matchAny");

    getUiDriver().addDiagnosticCallFunc(
        getBlazeHub()->getGameManagerAPI()->createGameBrowserList(gameListParams, 
            Blaze::GameManager::GameManagerAPI::CreateGameBrowserListCb(this, &NisGameBrowsing::CreateGameBrowserListCb)));
}

//------------------------------------------------------------------------------
// The result of the createGameBrowserList call
void NisGameBrowsing::CreateGameBrowserListCb(Blaze::BlazeError error, Blaze::JobId jobId,
                                              Blaze::GameManager::GameBrowserList* gameBrowserList, const char8_t* errDetails)
{
    getUiDriver().addDiagnosticCallback();

    if (error != Blaze::ERR_OK)
    {
        reportBlazeError(error);
    }
}

//------------------------------------------------------------------------------
// This listener callback gives us the list of games
void NisGameBrowsing::onGameBrowserListUpdated(Blaze::GameManager::GameBrowserList* gameBrowserList,
                                           const Blaze::GameManager::GameBrowserList::GameBrowserGameVector* removedGames,
                                           const Blaze::GameManager::GameBrowserList::GameBrowserGameVector* updatedGames)
{
    getUiDriver().addDiagnosticCallback();

    Blaze::GameManager::GameBrowserList::GameBrowserGameVector::iterator iter    = gameBrowserList->getGameVector().begin();
    Blaze::GameManager::GameBrowserList::GameBrowserGameVector::iterator iterEnd = gameBrowserList->getGameVector().end();

    while (iter != iterEnd)
    {
        Blaze::GameManager::GameBrowserGame* game = *iter;
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  game: name=%s, state=%s\n", game->getName(), GameStateToString(game->getGameState()));
        iter++;
    }

    done();
}

void NisGameBrowsing::onGameBrowserListUpdateTimeout(Blaze::GameManager::GameBrowserList* gameBrowserList)
{
    getUiDriver().addDiagnosticFunction();
    getUiDriver().addLog(Pyro::ErrorLevel::ERR, "Timeout fetching game snapshot list");
}

}
