/*! ************************************************************************************************/
/*!
    \file gamemanagerapijobs.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/blazehub.h"

#include "BlazeSDK/gamemanager/gamemanagerapijobs.h"
#include "BlazeSDK/component/gamereportingcomponent.h"

namespace Blaze
{
namespace GameManager
{

GameManagerApiJob::GameManagerApiJob(GameManagerAPI* api, const FunctorBase& cb)
    : mAPI(api), mFunctorCb(cb), mGameId(GameManager::INVALID_GAME_ID), mUserIndex(INVALID_USER_INDEX), mConnectionValidated(false), mWaitingOnConnectionValidation(false)
{
    setAssociatedTitleCb(mFunctorCb);
}


GameManagerApiJob::GameManagerApiJob(GameManagerAPI* api, const GameSetupReason& reason)
    : mAPI(api), mFunctorCb(), mGameId(GameManager::INVALID_GAME_ID), mUserIndex(INVALID_USER_INDEX), mConnectionValidated(false), mWaitingOnConnectionValidation(false)
{
    setSetupReason(reason);
}

GameManagerApiJob::~GameManagerApiJob()
{
    if (mUserIndex != INVALID_USER_INDEX)
    {
        mAPI->unlinkGameFromJob(mUserIndex, mGameId);
    }
}

//  NOTE: should this code move to the GameSetupJob if it'll only happen for that job (excepting matchmaking...)
void GameManagerApiJob::dispatch(BlazeError err, Game *game)
{
    if (game != nullptr)
    {
        Job::setExecuting(true);
        const GameSetupReason& reason = getSetupReason();
        mAPI->dispatchGameSetupCallback(*game, err, reason, mUserIndex);

        // Side: we ensure dispatch of onReservedExternalPlayers is *after* any successful game setup
        // in case title code wants to use their local joined game object,
        // in their implementation of the onReservedExternalPlayers handler.
        dispatchOnReservedExternalPlayersCallback(err);

        Job::setExecuting(false);
    }
}

void GameManagerApiJob::clearTitleCb()
{
    mFunctorCb.clear();
    setAssociatedTitleCb(mFunctorCb);   // Updates (clears) the callback object association
}

void GameManagerApiJob::getTitleCb(FunctorBase& cb) const
{
    cb = mFunctorCb;
}

void GameManagerApiJob::setGameId(uint32_t userIndex, GameId gameId)
{
    if (mUserIndex != INVALID_USER_INDEX)
        mAPI->unlinkGameFromJob(mUserIndex, mGameId);

    mGameId = gameId;
    mUserIndex = userIndex;
    mAPI->linkGameToJob(userIndex, gameId, this);
}

Game* GameManagerApiJob::getGame() const
{
    if (mGameId == GameManager::INVALID_GAME_ID)
        return nullptr;

    return mAPI->getGameById(mGameId);
}

void GameManagerApiJob::setSetupReason(const GameSetupReason& reason)
{
    reason.copyInto(mReason);
}


bool GameManagerApiJob::isMatchmakingConnectionValidated() const
{
    if ( (mReason.getMatchmakingSetupContext() == nullptr) && (mReason.getIndirectMatchmakingSetupContext() == nullptr) && ((mReason.getDatalessSetupContext() == nullptr) || (mReason.getDatalessSetupContext()->getSetupContext() != INDIRECT_JOIN_GAME_FROM_RESERVATION_CONTEXT)) )
    {
        // we don't have a possible job from a matchmaking session, connection is always considered validated, because no validation happens.
        return true;
    }

    if ( (mReason.getMatchmakingSetupContext() != nullptr) && (mReason.getMatchmakingSetupContext()->getSessionId() == INVALID_MATCHMAKING_SESSION_ID) )
    {
        // a matchmaking job, with no matchmaking session..., bad state, default state is verified.
        return true;
    }

    return mConnectionValidated;
}



bool GameManagerApiJob::isMatchmakingScenarioCanceled() const
{
    // GM_TODO: in the 3.0 timeframe, we'd like to rework Matchmaking so that the job spans all the way to the notification.

    if (mReason.getMatchmakingSetupContext() == nullptr || mReason.getMatchmakingSetupContext()->getSessionId() == INVALID_MATCHMAKING_SESSION_ID)
    {
        // we don't have an associated MM session (nothing to cancel)
        return false;
    }

    MatchmakingScenario *matchmakingResults = mAPI->getMatchmakingScenarioById(mReason.getMatchmakingSetupContext()->getScenarioId());
    if ((matchmakingResults == nullptr) || matchmakingResults->isCanceled())
    {
        return true;
    }

    return false;
}

/*! ***************************************************************************/
/*! \brief Dispatch onReservedExternalPlayers.
*******************************************************************************/
void GameManagerApiJob::dispatchOnReservedExternalPlayersCallback(BlazeError err)
{    
    if (!mReservedPlayerIdentifications.empty() && (getAPI() != nullptr))
    {
        getAPI()->dispatchOnReservedPlayerIdentifications(getGame(), mReservedPlayerIdentifications, err, getUserIndex());
    }
}

/*! ***************************************************************************/
/*! \brief return whether creating game that won't have a topology host assigned
*******************************************************************************/
bool GameManagerApiJob::isCreatingHostlessGame() const
{
    const Game* game = getGame();
    return (game != nullptr) && game->isHostless() && (((mReason.getDatalessSetupContext() != nullptr) && (mReason.getDatalessSetupContext()->getSetupContext() == CREATE_GAME_SETUP_CONTEXT)) ||
        ((mReason.getMatchmakingSetupContext() != nullptr) && (mReason.getMatchmakingSetupContext()->getMatchmakingResult() == SUCCESS_CREATED_GAME)));
}

///////////////////////////////////////////////////////////////////////////////////////
    
JoinGameJob::JoinGameJob(GameManagerAPI *api, const GameManagerAPI::JoinGameCb &titleCb, uint32_t userIndex)
            : GameManagerApiJob(api, titleCb), mUserIndex(userIndex)
{

}

void JoinGameJob::cancel(BlazeError err)
{
    Game *game = getGame();

    dispatch(err, game);

    if (game != nullptr)
    {
        //  leave game silently.
        game->leaveGame(Game::LeaveGameJobCb(), nullptr, game->getGameSettings().getDisconnectReservation());
    }
}

void JoinGameJob::dispatch(BlazeError err, Game *game)
{
    if (game == nullptr)
    {
        game = getGame();
    }

    //  validate whether to dispatch the title callback based on the context.
    GameManagerAPI::JoinGameCb titleCb;
    getTitleCb(titleCb);
    Job::setExecuting(true);
    titleCb(err, getId(), game, "");

    // Side: we ensure dispatch of onReservedExternalPlayers is *after* any successful game setup
    // (and join/create job's title cb) in case title code wants to use their local joined game object,
    // in their implementation of the onReservedExternalPlayers handler.
    dispatchOnReservedExternalPlayersCallback(err);

    Job::setExecuting(false);
}

JoinGameByUserListJob::JoinGameByUserListJob(GameManagerAPI *api, const GameManagerAPI::JoinGameCb &titleCb, uint32_t userIndex)
    : JoinGameJob(api, titleCb, userIndex)
{
}

// Override JoinGameJob to avoid dispatching a leave request for the caller,
// as the caller wasn't joining the game as part of this request
void JoinGameByUserListJob::cancel(BlazeError err)
{
    dispatch(err, getGame());
}

///////////////////////////////////////////////////////////////////////////////////////

CreateGameJob::CreateGameJob(GameManagerAPI *api, const GameManagerAPI::CreateGameCb &titleCb)
            : GameManagerApiJob(api, titleCb)
{
}

void CreateGameJob::cancel(BlazeError err)
{
    Game *game = getGame();

    dispatch(err, game);
 
    if (game != nullptr)
    {
        //  destroy game silently.
        game->destroyGame(HOST_LEAVING, Game::DestroyGameJobCb());
    }
}

void CreateGameJob::dispatch(BlazeError err, Game *game)
{
    if (game == nullptr)
    {
        game = getGame();
    }

    //  validate whether to dispatch the title callback based on the context.
    GameManagerAPI::CreateGameCb titleCb;
    getTitleCb(titleCb);
    Job::setExecuting(true);
    titleCb(err, getId(), game);

    dispatchOnReservedExternalPlayersCallback(err); 

    Job::setExecuting(false);
}

///////////////////////////////////////////////////////////////////////////////////////

GameSetupJob::GameSetupJob(GameManagerAPI *api, const GameSetupReason& reason)
            : GameManagerApiJob(api, reason)

{
}


void GameSetupJob::cancel(BlazeError err)
{
    Game *game = getGame();
    dispatch(err, game);

    if (game != nullptr)
    {
        const GameSetupReason& setupReason = getSetupReason();
        
        // silently roll back our action ( leave or destroy the game )
        if ( (setupReason.getDatalessSetupContext() != nullptr) && 
                  (setupReason.getDatalessSetupContext()->getSetupContext() == CREATE_GAME_SETUP_CONTEXT) )
        {
            // we destroy the game silently if we originated from a createGame call
            game->destroyGame(HOST_LEAVING, Game::DestroyGameJobCb());
        }
        else
        {
            // we leave the game silently
            game->leaveGame(Game::LeaveGameJobCb(), nullptr, game->getGameSettings().getDisconnectReservation());
            // GM_AUDIT: GOS-4488: if this was a PG game setup, we should probably do a PG leave game
        }
    }
}


} // namespace GameManager
} // namespace Blaze

