/*! ************************************************************************************************/
/*!
    \file matchmakingsession.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_SESSION
#define BLAZE_MATCHMAKING_SESSION
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/component/gamemanager/tdf/matchmaker.h"
#include "BlazeSDK/component/gamemanager/tdf/scenarios.h"
#include "BlazeSDK/memorypool.h"

namespace Blaze
{


namespace GameManager
{
    class GameManagerAPI;

    /*! ************************************************************************************************/
    /*! \brief Helper class for shared functionality of MatchmakingScenario/Session
    ***************************************************************************************************/
    class BLAZESDK_API MatchmakingScenario
    {
        friend class GameManagerAPI;
        friend class GameSetupContext;
        friend class MemPool<MatchmakingScenario>;
    public:

        /*! ************************************************************************************************/
        /*! \brief Return the actual fitScore to the matched game.  Note: not initialized until the session
                has finished (ie: inside GameManagerListener::onMatchmakingSessionFinished).
            \return returns the actual fitScore of my matchmaking session to the matched game.
        ***************************************************************************************************/
        FitScore getFitScore() const { return mFitScore; }

        uint16_t getAssignedInstanceId() const { return mAssignedInstanceId; }

        /*! ************************************************************************************************/
        /*! \brief Return the time to match of a matched game. Useful for re-entering matchmaking after a connection
            verification failure.
            \return returns time it took to discover the match, not counting QoS validation time, if any.
        ***************************************************************************************************/
        const TimeValue& getTimeToMatch() const { return mTimeToMatch; }

        /*! ************************************************************************************************/
        /*! \brief Return the estimated time to match sent by the Blaze server
            \return returns estimated time to match for this scenario (always 0 for legacy matchmaking)
        ***************************************************************************************************/
        const TimeValue& getEstimatedTimeToMatch() const { return mEstimatedTimeToMatch; }

        /*! ************************************************************************************************/
        /*! \brief Return the max possible fitScore.  Note: not initialized until the session 
                has finished (ie: inside GameManagerListener::onMatchmakingSessionFinished).
            \return returns the max possible fitScore as reported by the server.
        ***************************************************************************************************/
        FitScore getMaxPossibleFitScore() const { return mMaxPossibleFitScore; }

        /*! ************************************************************************************************/
        /*! \brief return the session's fitScore percent (from 0..100).  Note: not initialized until the session 
                has finished (ie: inside GameManagerListener::onMatchmakingSessionFinished).
                
                The fitScore percentage ranges from 0..100, and is calculated by (fitScore/maxPossibleFitScore)*100.
                If maxPossibleFitScore is 0, we set the score to 100% (any match is a perfect match).

            \return return the fitScore percent (0.0 to 100.0)
        ***************************************************************************************************/
        float getFitScorePercent() const { return mFitScorePercent; }

        /*! ************************************************************************************************/
        /*! \brief Set to true once the local session has been canceled.
        ***************************************************************************************************/
        bool isCanceled() const { return mIsCanceled; }

        /*! ************************************************************************************************/
        /*! \brief If the session was started with the pseudoRequest flag and finalized through create game,
                This will contain the debug information about the matchmaking session.
        ***************************************************************************************************/
        const DebugCreateGameResults& getDebugCreateGameResults() const { return mCreateGameResults; }

        /*! ************************************************************************************************/
        /*! \brief If the session was started with the pseudoRequest flag and finalized through find game,
                This will contain the debug information about the matchmaking session.
        ***************************************************************************************************/
        const DebugFindGameResults& getDebugFindGameResults() const { return mFindGameResults; }

        const char *getScid() const { return mScid.c_str(); }
        const char *getExternalSessionTemplateName() const { return mExternalSessionTemplateName.c_str(); }
        const char *getExternalSessionName() const { return mExternalSessionName.c_str(); }
        const char *getExternalSessionCorrelationId() const { return mExternalSessionCorrelationId.c_str(); }

        uint32_t getSessionAgeSeconds() const { return mSessionAgeSeconds; }


    public:

        /*! ************************************************************************************************/
        /*! \brief Return if the matchmaking helper represents a scenario or a session.
            \return Return true if this is a scenario. 
        ***************************************************************************************************/
        bool getIsScenario() const { return true; }

        /*! ************************************************************************************************/
        /*! \brief Return the matchmaking scenario's unique matchmaking scenario id.
            \return Return the matchmaking scenario's id.
        ***************************************************************************************************/
        MatchmakingScenarioId getScenarioId() const { return mScenarioId; }

        /*! ************************************************************************************************/
        /*! \brief Issue a cancelMatchmakingScenario RPC, which will destroy the scenario and dispatch 
            onMatchmakingScenarioFinished.  

            Note: it's possible that the matchmaking scenario has already finished, and the finished notification
            crosses the cancel RPC on the wire.  In this case, the cancel is ignored (as the scenario no longer
            exists on the server).  Also the onMatchmakingScenarioFinished will not be dispatched by
            the cancel.
        ***************************************************************************************************/
        void cancelScenario();

        /*! ************************************************************************************************/
        /*! \brief Return the matchmaking scenario's variant name.
        \return Return the matchmaking scenario's variant name.
        ***************************************************************************************************/
        const char8_t* getScenarioVariant() const { return mScenarioVariant.c_str(); }

        const char8_t* getRecoTrackingTag() const { return mRecoTrackingTag.c_str(); }

    private:
        NON_COPYABLE(MatchmakingScenario); // disable copy ctor & assignment operator

        // Note: don't create or delete matchmaking sessions explicitly; they're managed by the GameManagerAPI
        MatchmakingScenario(GameManagerAPI* gameManager, const StartMatchmakingScenarioResponse *startMatchmakingResponse, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
        ~MatchmakingScenario() {}

        void updateFitScorePercent(float fitScorePercent);
        void finishMatchmaking(FitScore fitScore,FitScore maxFitScore, MatchmakingResult result, const TimeValue& timeToMatch);
        bool isFinished() const { return mFinished; }
        void addDebugInfo(const DebugFindGameResults& findGameResults, const DebugCreateGameResults& createGameResults);

        // helper to override the result of a matchmaking session
        void setMatchmakingResult(MatchmakingResult matchmakingResult) { mResult = matchmakingResult; }
        void setSessionAgeSeconds(uint32_t sessionAgeSeconds) { mSessionAgeSeconds = sessionAgeSeconds; }
        void setAssignedInstanceId(uint16_t instanceId) { mAssignedInstanceId = instanceId; }


        /*! ************************************************************************************************/
        /*! \brief Sets the game id this matchmaking session was put into.  
        
            \param[in] gameId - the game id this matchmaking session put the player in.
        ***************************************************************************************************/
        void setGameId(GameId gameId) { mGameId = gameId; }
        /*! ************************************************************************************************/
        /*! \brief Sets the game id this matchmaking session was put into.  
        
            \return gameId - the game id this matchmaking session put the player in. Generally INVALID_GAME_ID except during QoS validation.
        ***************************************************************************************************/
        GameId getGameId() { return mGameId; }

        void cancelInternal();

        GameManagerAPI *mGameManagerAPI;
        
        uint16_t mAssignedInstanceId;
        FitScore mFitScore;
        FitScore mMaxPossibleFitScore;
        float mFitScorePercent;
        MatchmakingResult mResult;
        MemoryGroupId mMemGroup;   //!< memory group to be used by this class, initialized with parameter passed to constructor
        bool mFinished;
        bool mIsCanceled;
        TimeValue mTimeToMatch;
        TimeValue mEstimatedTimeToMatch;
        ScenarioVariantName mScenarioVariant;
        RecoTrackingTag mRecoTrackingTag;

        GameId mGameId;
        DebugCreateGameResults mCreateGameResults;
        DebugFindGameResults mFindGameResults;

        uint32_t mSessionAgeSeconds;

        MatchmakingScenarioId mScenarioId;

        XblScid mScid;
        XblSessionTemplateName mExternalSessionTemplateName;
        XblSessionName mExternalSessionName;
        XblCorrelationId mExternalSessionCorrelationId;
    };

} //Game Manager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_SESSION
