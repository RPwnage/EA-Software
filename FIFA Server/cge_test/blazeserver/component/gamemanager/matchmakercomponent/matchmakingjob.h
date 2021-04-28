/*! ************************************************************************************************/
/*!
    \file   matchmakingjob.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKER_MATCHMAKINGJOB_H
#define BLAZE_MATCHMAKER_MATCHMAKINGJOB_H

#include "gamemanager/matchmaker/matchmaker.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/rete/retenetwork.h"

namespace Blaze
{
    namespace GameManager
    {
        namespace Matchmaker
        {
            class MatchmakingSession;
        }

   }

namespace Matchmaker
{
     /*! ************************************************************************************************/
     /*!
        \brief base class/ interface for matchmaking async jobs (calls into GameManager Master)
    *************************************************************************************************/
    class MatchmakingJob
    {
        NON_COPYABLE(MatchmakingJob);

    public:

         /*! ************************************************************************************************/
         /*!
            \brief MatchmakingJob base component constructor
        
            \param[in] matchmakerSlaveComponent - reference the mm slave owning this job
            \param[in] owningUserSessionId - the userSessionId of the owner of the associated (finalizing) matchmaking session
            \param[in] owningMatchmakingSession - pointer to the associated MM session

            \return new MatchmakingJob
        *************************************************************************************************/
        MatchmakingJob(MatchmakerSlaveImpl &matchmakerSlaveComponent, const UserSessionId &owningUserSessionId,
            const Blaze::GameManager::MatchmakingSessionId &owningMatchmakingSessionId);

        /*! ************************************************************************************************/
        /*!
            \brief cancels job timer, if valid
        *************************************************************************************************/
        virtual ~MatchmakingJob();

        // implemented by subclasses
        virtual void start() = 0;

        /*! ************************************************************************************************/
        /*!
            \brief checks the status of the owning matchmaking session

            \return true if the owningMatchmakingSession pointer is still viable
        *************************************************************************************************/
        bool isOwningSessionStillValid() const;

        // accessors
        const UserSessionId getOwningUserSessionId() const { return mOwningUserSessionId; }
        const Blaze::GameManager::MatchmakingSessionId getOwningMatchmakingSessionId() const { return mOwningMatchmakingSessionId; }

         /*! ************************************************************************************************/
         /*!
            \brief Called when we're ready to evaluate the connectivity of the game session.
                   Will dispatch to client when complete and finish matchmaking.
                   Will update QoS rule information, and complete matchmaking if the validation fails.
        *************************************************************************************************/
        BlazeRpcError onMatchQosDataAvailable(const Blaze::GameManager::GameSessionConnectionComplete &request, Blaze::GameManager::QosEvaluationResult &response);

        virtual void sendNotifyMatchmakingSessionConnectionValidated( const Blaze::GameManager::GameId gameId, bool dispatchClientListener ) = 0;

        //any qos connection-validated result received before we're in awaiting qos state will get delayed, instead of being sent to client immediately
        void setAwaitingQos() { mState = MMJOBSTATE_AWAITING_QOS; }
        void sendDelayedNotifyMatchmakingSessionConnectionValidatedIfAvailable();

        // store off pin submissions to be sent once QOS validation is complete
        void setPinSubmission(const PINSubmission &pinSubmission, const GameManager::Matchmaker::MatchmakingSession* session);

    protected:

        bool areAllUsersConnected(const Blaze::GameManager::GameSessionConnectionComplete &request) const;
        
        virtual bool validateQos(const Blaze::GameManager::GameSessionConnectionComplete &request, Blaze::GameManager::QosEvaluationResult &response) const = 0;

        /*! ************************************************************************************************/
        /*!
            \brief returns sessions that failed to join or create back to the matchmaker to try again/expire
        *************************************************************************************************/
        void returnSessionsToMatchmaker(Blaze::Search::MatchmakingSessionIdList sessionIdList, bool updatedQosCriteria, GameManager::Matchmaker::Matchmaker::ReinsertionActionType reinsertionAction = GameManager::Matchmaker::Matchmaker::NO_ACTION, GameManager::Rete::ProductionNodeId productionNodeId = GameManager::Rete::INVALID_PRODUCTION_NODE_ID);

        bool validateQosForMatchmakingSessions(const Blaze::Search::MatchmakingSessionIdList &matchmakingSessionIds, const Blaze::GameManager::GameSessionConnectionComplete &request, Blaze::GameManager::QosEvaluationResult &response) const;

        MatchmakerSlaveImpl &mComponent; 
        const UserSessionId mOwningUserSessionId; // either the MM session owner
        const Blaze::GameManager::MatchmakingSessionId mOwningMatchmakingSessionId; // MM session owning this job
        enum MatchmakingJobState
        {
            MMJOBSTATE_PRE_START,
            MMJOBSTATE_STARTED,
            MMJOBSTATE_AWAITING_QOS
        };
        MatchmakingJobState mState;

        // in rare edge cases a single Create mode MM session might immediately return qos data before the job's game creation/join even completes. This caches off the result until it completes.
        struct DelayedQosResult 
        {
            DelayedQosResult() : mGameId(Blaze::GameManager::INVALID_GAME_ID), mDispatchClientListener(false) {}//INVALID_GAME_ID if unset
            Blaze::GameManager::GameId mGameId;
            bool mDispatchClientListener;
        };
        DelayedQosResult mDelayedConnectionValidatedResult;
        PINSubmissionPtr mPinSubmissionPtr;
    };

    class FinalizeFindGameMatchmakingJob : public MatchmakingJob
    {
        NON_COPYABLE(FinalizeFindGameMatchmakingJob);

    public:

        FinalizeFindGameMatchmakingJob(MatchmakerSlaveImpl &matchmakerSlaveComponent, const UserSessionId &owningUserSessionId,
            const Blaze::GameManager::MatchmakingSessionId &owningMatchmakingSessionId) :
            MatchmakingJob(matchmakerSlaveComponent, owningUserSessionId, owningMatchmakingSessionId)
            {}

        void sendNotifyMatchmakingSessionConnectionValidated( const Blaze::GameManager::GameId gameId, bool dispatchClientListener ) override;

        void start() override;

    protected:

        bool validateQos(const Blaze::GameManager::GameSessionConnectionComplete &request, Blaze::GameManager::QosEvaluationResult &response) const override;
    };

    /*! ************************************************************************************************/
     /*!
        \brief job to track finialization in create game mode (create game session, join matching sessions to the game)
    *************************************************************************************************/
    class FinalizeCreateGameMatchmakingJob : public MatchmakingJob
    {
        NON_COPYABLE(FinalizeCreateGameMatchmakingJob);

    public:

        /*! ************************************************************************************************/
        /*!
            \brief MatchmakingJob base component constructor
        
            \param[in] matchmakerSlaveComponent - reference the mm slave owning this job
            \param[in] owningUserSessionId - the userSessionId of the owner of the associated (finalizing) matchmaking session
            \param[in] owningMatchmakingSession - pointer to the associated MM session

            \return new MatchmakingJob
        *************************************************************************************************/
        FinalizeCreateGameMatchmakingJob(Blaze::GameManager::Matchmaker::Matchmaker &matchmaker, const UserSessionId &owningUserSessionId,
            const Blaze::GameManager::MatchmakingSessionId &owningMatchmakingSessionId, 
            Blaze::GameManager::Matchmaker::CreateGameFinalizationSettings *createGameFinalizationSettings);

         /*! ************************************************************************************************/
         /*!
            \brief destructor, destroys any matchmaking sessions that are still valid, but were not returned to the matchmaker.
                The destroyed sessions are typically the sessions that successfully joined or created a game
        *************************************************************************************************/
        ~FinalizeCreateGameMatchmakingJob() override;


         /*! ************************************************************************************************/
         /*!
            \brief schedules a fiber to attempt create game.
        *************************************************************************************************/
        void start() override;

    private:
        /*! ************************************************************************************************/
        /*!
            \brief sends MatchmakerCreateGame RPC to GM master, if successful, follows up by joining remaining sessions
        *************************************************************************************************/
        void attemptCreateGame();

        /*! ************************************************************************************************/
        /*!
            \brief terminates sessions that failed QoS validation
        *************************************************************************************************/
        void terminateSessionsAsFailedQosValidation(Blaze::Search::MatchmakingSessionIdList failedQosList);

        /*! ************************************************************************************************/
        /*!
            \brief sessions that managed to join the created game are destroyed, based on the response from the join game RPC
        *************************************************************************************************/
        void cleanupSuccessfulSessions(GameManager::GameId gameId);

        /*! ************************************************************************************************/
        /*!
            \brief sessions that managed to join the created game are destroyed, based on the response from the join game RPC
        *************************************************************************************************/
        void joinRemainingSessions(Blaze::GameManager::GameId gameId, Blaze::GameManager::Matchmaker::NotifyMatchmakingReservedExternalPlayersMap& pendingNotifications);

        /*! ************************************************************************************************/
        /*!
            \brief removes sessions from the job if their associated user session has logged out (which deletes the session)
            \return true if sessions were removed
        *************************************************************************************************/
        bool removeInvalidSessions();

        /*! ************************************************************************************************/
        /*!
            \brief clears and rebuilds the session list in the finalization settings.
        *************************************************************************************************/
        void rebuildMatchedSessions();

        /*! ************************************************************************************************/
        /*!
            \returns true if the auto join matchmaking session still exists
        *************************************************************************************************/
        bool isAutoJoinMemberStillValid() const;

        // checks that all MM sessions for this job are valid by confirming the owning user sessions are still logged in
        bool areMatchmakingSessionsStillValid() const;

        

        /*! ************************************************************************************************/
        /*! \brief Create or Reset a game for this matchmaking session.
            The supplied host/members vote on the appropriate game values, and reservations are made for all joining players.

        WARNING: ONLY THE CREATING OR RESETTING PLAYER JOINS THE GAME, ASSOCIATED GROUP MEMBERS ARE NOT JOINED

            \param[in] finalizationSettings struct containing the host, and matchmaking sessions that should join the game explicitly
            \param[in,out] response TDF object containing the create game response
            \param[in] networkTopology the topology to use for the created game (supplied by the finalized session)
            \param[in] privledgedGameIdPreference dedicated server override game id
            \return error code from create attempt. ERR_OK if the game was created.
        ***************************************************************************************************/
        BlazeRpcError createOrResetRemoteGameSession(Blaze::GameManager::Matchmaker::CreateGameFinalizationSettings &finalizationSettings, Blaze::GameManager::CreateGameMasterResponse &response, GameNetworkTopology networkTopology, Blaze::GameManager::GameId privledgedGameIdPreference);

        /*! ************************************************************************************************/
        /*! \brief joins the MatchmakingSessions listed in createGameFinalizationSettings.mMMSessionList
            into the supplied game.  Group members are joined as well.  NOTE: we also join group members
            of createGameFinalizationSettings.mAutomaticallyJoinedMMSession.

            \param[in] gameId gameId we're joining.
            \param[in] createGameFinalizationSettings contains a listing of the matchmaking sessions to be joined
                        (as well as the session automatically joined during game creation/reset)
            \param[in,out] joinGameResponse contains a list of matchmaking session ids that joined the game session
            \return error code resulting from join request.
        ***************************************************************************************************/
        BlazeRpcError joinSessionsToRemoteGameSession(Blaze::GameManager::GameId& gameId, const Blaze::GameManager::Matchmaker::CreateGameFinalizationSettings &createGameFinalizationSettings, Blaze::GameManager::MatchmakingJoinGameResponse &joinGameResponse) const;
   
        void cleanupPlayersFromGame(Blaze::GameManager::GameId gameId, const ExternalUserJoinInfoList& externalUserInfos, bool requiresSuperUser) const;

        void sendNotifyMatchmakingSessionConnectionValidated( const Blaze::GameManager::GameId gameId, bool dispatchClientListener ) override;

    protected:

        bool validateQos(const Blaze::GameManager::GameSessionConnectionComplete &request, Blaze::GameManager::QosEvaluationResult &response) const override;

    private:
        typedef eastl::map<Blaze::GameManager::MatchmakingSessionId, const Blaze::GameManager::Matchmaker::MatchmakingSession*> MatchmakingSessionIdToMatchmakingSessionMap;

        const UserSessionId mAutoJoinUserSessionId; // the session id of the new game's host
        const BlazeId mAutoJoinUserBlazeId; // the blaze id of the new games host.
        const Blaze::GameManager::MatchmakingSessionId mAutoJoinMatchmakingSessionId; // MM session containing the auto-join user
        Blaze::Search::MatchmakingSessionIdList mMatchmakingSessionIdList;
        Blaze::Search::MatchmakingSessionIdList mJoinedMatchmakingSessionIdList;
        Blaze::Search::MatchmakingSessionIdList mFailedGameJoinMatchmakingSessionIdList;
        Blaze::GameManager::Matchmaker::CreateGameFinalizationSettings *mCreateGameFinalizationSettings;

        // this list contains the user session ids of all users joining the game via matchmaking.
        // if ANY of the sessions have logged out or been canceled before creating the game, we log out
        MatchmakingSessionIdToMatchmakingSessionMap mMatchmakingSessionIdToMatchmakingSessionMap; 

        Blaze::GameManager::Matchmaker::Matchmaker& mMatchmaker;
    };

} // namespace Matchmaker
} // namespace Blaze
#endif // BLAZE_MATCHMAKER_MATCHMAKINGJOB_H
