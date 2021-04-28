/*! ************************************************************************************************/
/*!
    \file matchmakingsession.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/gamemanager/matchmakingsession.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/gamemanager/player.h"

namespace Blaze
{
namespace GameManager
{
    MatchmakingScenario::MatchmakingScenario(GameManagerAPI* gameManager, const StartMatchmakingScenarioResponse *startMatchmakingScenarioResponse, MemoryGroupId memGroupId)
        : mGameManagerAPI(gameManager),
          mAssignedInstanceId(0),
          mFitScore(0), 
          mMaxPossibleFitScore(0),
          mFitScorePercent(0),
          mResult(GameManager::SUCCESS_CREATED_GAME),
          mMemGroup(memGroupId),
          mFinished(false),
          mIsCanceled(false),
          mTimeToMatch(0),
          mGameId(GameManager::INVALID_GAME_ID),
          mSessionAgeSeconds(0)
    {
        mScid = (startMatchmakingScenarioResponse->getScid());
        mExternalSessionTemplateName = (startMatchmakingScenarioResponse->getExternalSessionTemplateName());
        mExternalSessionName = (startMatchmakingScenarioResponse->getExternalSessionName());
        mExternalSessionCorrelationId = (startMatchmakingScenarioResponse->getExternalSessionCorrelationId());
        mScenarioVariant = startMatchmakingScenarioResponse->getScenarioVariant();
        mScenarioId = startMatchmakingScenarioResponse->getScenarioId();
        mEstimatedTimeToMatch = startMatchmakingScenarioResponse->getEstimatedTimeToMatch();
        mRecoTrackingTag = startMatchmakingScenarioResponse->getRecoTrackingTag();
    }
    
    /*! ************************************************************************************************/
    /*! \brief Issue a cancelMatchmaking RPC, which will destroy the session and dispatch 
        onMatchmakingScenarioFinished.  

        Note: it's possible that the matchmaking session has already finished, and the finished notification
        crosses the cancel rpc on the wire.  In this case, the cancel is ignored (as the session no longer
        exists on the server).  Also the onMatchmakingScenarioFinished will not be dispatched by
        the cancel.
    ***************************************************************************************************/
    void MatchmakingScenario::cancelScenario()
    {
        cancelInternal();
    }
    
    /*! ************************************************************************************************/
    /*! \brief internal common helper for canceling a scenario
    ***************************************************************************************************/
    void MatchmakingScenario::cancelInternal()
    {
        mIsCanceled = true;

        if (!isFinished())
        {
            // See if the session already put us in a game. There may still be a session on the server due to QoS validation
            if (mGameId != GameManager::INVALID_GAME_ID)
            {
                BLAZE_SDK_DEBUGF("[MatchmakingSession] Cancel request for scenario %" PRIu64 " while setting up game %" PRIu64 ".  Attempting to leave game.\n", mScenarioId, mGameId);
                Game* game = mGameManagerAPI->getGameById(mGameId);
                if (game != nullptr)
                {
                    // Leave game silently
                    game->leaveGame(Game::LeaveGameJobCb());
                    bool wasActive = (game->getLocalPlayer() != nullptr ? game->getLocalPlayer()->isActive() : true);
                    // note DON'T dispatchNotifyMatchmakingFinished in destroyLocalGame, passing in false here. We do the dispatch below.
                    mGameManagerAPI->destroyLocalGame(game, LOCAL_PLAYER_LEAVING, game->isTopologyHost(), wasActive, false);
                }
                finishMatchmaking(0, 0, SESSION_CANCELED, 0);

                // Since having a gameId doesn't mean we're done on the server, must always send the cancel
                CancelMatchmakingScenarioRequest cancelRequest;
                cancelRequest.setMatchmakingScenarioId(mScenarioId);
                mGameManagerAPI->getGameManagerComponent()->cancelMatchmakingScenario(cancelRequest);
                // dispatch now, because if we do have a game, the session may already be destroyed server-side
                mGameManagerAPI->dispatchNotifyMatchmakingScenarioFinished(*this, nullptr);
            }
            else
            {
                // Note: RPC is fire 'n forget; the server will send down an async 
                //  notifyMatchmakingFinished msg with session canceled on success
                CancelMatchmakingScenarioRequest cancelRequest;
                cancelRequest.setMatchmakingScenarioId(mScenarioId);
                mGameManagerAPI->getGameManagerComponent()->cancelMatchmakingScenario(cancelRequest);
            }
        }
    }

    void MatchmakingScenario::updateFitScorePercent(float fitScorePercent)
    {
        if (mFitScorePercent > 100.0f)
            mFitScorePercent = 100.0f;
        else if (fitScorePercent < 0.0f)
            mFitScorePercent = 0.0f;
        else
            mFitScorePercent = fitScorePercent;
    }

    /*! ************************************************************************************************/
    /*! \brief sets the fit scores and result for a matchmaking session. Should
            be called before dispatchNotifyMatchmakingFinished.

        Note: if maxPossibleFitScore is zero, we set the fitPercent to 100.0 (avoiding a divide by zero).
          If the max possible score is 0, and matching score is a perfect match.
    ***************************************************************************************************/
    void MatchmakingScenario::finishMatchmaking(FitScore fitScore, FitScore maxFitScore, MatchmakingResult result, const TimeValue& timeToMatch)
    {
        BlazeAssert(!isFinished());

        mFinished = true;
        mFitScore = fitScore;
        mMaxPossibleFitScore = maxFitScore;
        mResult = result;
        mTimeToMatch = timeToMatch;

        // calc the fit score percent
        if (mMaxPossibleFitScore > 0)
        {
            mFitScorePercent = (mFitScore / (float) mMaxPossibleFitScore) * 100;
        }
        else
        {
            // divide by zero -> 100%
            mFitScorePercent = 100.0;
        }
    }


    void MatchmakingScenario::addDebugInfo(const DebugFindGameResults& findGameResults, const DebugCreateGameResults& createGameResults)
    {
        createGameResults.copyInto(mCreateGameResults);
        findGameResults.copyInto(mFindGameResults);
    }


} //GameManager
} // namespace Blaze

