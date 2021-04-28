/*! ************************************************************************************************/
/*!
    \file gamemanagerapijobs.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGERAPI_JOBS_H
#define BLAZE_GAMEMANAGERAPI_JOBS_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"

namespace Blaze
{
namespace GameManager
{

    /*! **************************************************************************************************************/
    /*! 
        \class LeaveGameJob

        \brief
        Specialized Job for Game::leaveGame() callbacks issued without a RPC call (pre-RPC errors.)
    ******************************************************************************************************************/
    class LeaveGameJob: public FunctorCallJob3<BlazeError, Game*, JobId>
    {
    public:
        LeaveGameJob(const Game::LeaveGameJobCb& cb, BlazeError err, Game *game, JobId jobId) : FunctorCallJob3<BlazeError, Game*, JobId>(cb, err, game, jobId) { setAssociatedTitleCb(cb); }
    };

    /*! **************************************************************************************************************/
    /*! 
        \class GameManagerApiJob

        \brief
        GameManagerApiJob is a base class for wrapper jobs used to dispatch the title callback in case of cancel requests
        \details
        GameManagerApiJob does nothing during idle() or execute(), inheriting classes should implement the cancel class
        to trigger the title callback with SDK_ERR_RPC_CANCELED.

    ******************************************************************************************************************/
    class GameManagerApiJob : public Job
    {
    public:
        GameManagerApiJob(GameManagerAPI* api, const FunctorBase& cb );
        GameManagerApiJob(GameManagerAPI* api, const GameSetupReason& reason);
        GameManagerApiJob(): mAPI(nullptr), mFunctorCb(), mGameId(GameManager::INVALID_GAME_ID), mUserIndex(0), mConnectionValidated(false), mWaitingOnConnectionValidation(false) {}    

        virtual ~GameManagerApiJob();

        /*! ***************************************************************************/
        /*! \brief idler - no-op
        *******************************************************************************/
        void idle() {}

        /*! ***************************************************************************/
        /*! \brief execute 
            \returns wether or not the job can be deleted.
        *******************************************************************************/
        virtual void execute() {
            // only dispatch the finishing setup cb if *not* pending MM qos validation.
            if (isMatchmakingConnectionValidated())
                dispatch(ERR_OK, getGame());
        }

        /*! ***************************************************************************/
        /*! \brief This function runs when the job is canceled.
        *******************************************************************************/
        virtual void cancel(BlazeError err) = 0;

        /*! ***************************************************************************/
        /*! \brief Dispatches a callback based on the current setup reason
        *******************************************************************************/
        virtual void dispatch(BlazeError err, Game *game = nullptr);

        /*! ***************************************************************************/
        /*! \brief Returns the game ID associated with this job.
        *******************************************************************************/
        GameId getGameId() const {
            return mGameId;
        }

        /*! ***************************************************************************/
        /*! \brief Links the game ID to this job
        *******************************************************************************/
        void setGameId(uint32_t userIndex, GameId gameId);

        /*! ***************************************************************************/
        /*! \brief Stores the fact that matchmaking validated the connection on the
                the job.
        *******************************************************************************/
        void setConnectionValidated() { mConnectionValidated = true; }

        /*! ***************************************************************************/
        /*! \brief Stores the fact that matchmaking validation is the last thing required
            before dispatching the job.
        *******************************************************************************/
        void setWaitingForConnectionValidation() { mWaitingOnConnectionValidation = true; }

        /*! ***************************************************************************/
        /*! \brief Returns if matchmaking validation is the last thing required
            before dispatching the job.
        *******************************************************************************/
        bool isWaitingForConnectionValidation() const { return mWaitingOnConnectionValidation; }

        /*! ***************************************************************************/
        /*! \brief Returns the game object linked to this job (or nullptr if none.)
        *******************************************************************************/
        Game* getGame() const;

        /*! ***************************************************************************/
        /*! \brief Returns the game setup reason for the job
        *******************************************************************************/
        const GameSetupReason& getSetupReason() const {
            return mReason;
        }

        /*! ***************************************************************************/
        /*! \brief Sets the game setup reason for the job
        *******************************************************************************/
        void setSetupReason(const GameSetupReason& reason);

        /*! ***************************************************************************/
        /*! \brief Returns whether the matchmaking session associated with this job was
                cancelled.
        *******************************************************************************/
        bool isMatchmakingScenarioCanceled() const;

        /*! ***************************************************************************/
        /*! \brief Returns whether the matchmaking session associated with this job was
                has completed connection validation.
        *******************************************************************************/
        bool isMatchmakingConnectionValidated() const;

        /*! ***************************************************************************/
        /*! \brief Clears the title callback - this is for cases where the job may persist
                but the title callback has already been executed and is no longer needed.
        *******************************************************************************/
        void clearTitleCb();

        void setReservedExternalPlayerIdentifications(const Blaze::UserIdentificationList& externalPlayers) { externalPlayers.copyInto(mReservedPlayerIdentifications); }

        bool isCreatingHostlessGame() const;
    protected:
        GameManagerAPI* getAPI() {
            return mAPI;
        }
        void getTitleCb(FunctorBase& cb) const;

        virtual uint32_t getUserIndex() const { return mUserIndex; }

        void dispatchOnReservedExternalPlayersCallback(BlazeError err);

        GameManagerAPI* mAPI;

    private:
        FunctorBase mFunctorCb;
        GameId mGameId;
        GameSetupReason mReason;
        uint32_t mUserIndex;
        UserIdentificationList mReservedPlayerIdentifications;
        bool mConnectionValidated;
        bool mWaitingOnConnectionValidation;
    };

    /*! **************************************************************************************************************/
    /*! \class JoinGameJob

        \brief
         JoinGameJob is a wrapper job to dispatch the title callback in case of a GameManagerAPI::JoinGameByXXX() request is
            canceled.
        \details
            JoinGameJob does nothing during idle() or execute(), but triggers the title callback with SDK_ERR_RPC_CANCELED
            if the job is canceled.

    ******************************************************************************************************************/
    class JoinGameJob : public GameManagerApiJob
    {
    public:

        JoinGameJob(GameManagerAPI *api, const GameManagerAPI::JoinGameCb &titleCb, uint32_t userIndex);
        
        virtual void cancel(BlazeError err);

        virtual void dispatch(BlazeError err, Game *game = nullptr);

        void getTitleCb(GameManagerAPI::JoinGameCb &cb) {
            GameManagerApiJob::getTitleCb(cb);
        }

        uint32_t getUserIndex() const { return mUserIndex; }

    private:
        uint32_t mUserIndex;
    };

    /*! **************************************************************************************************************/
    /*! \class JoinGameByUserListJob
    ******************************************************************************************************************/
    class JoinGameByUserListJob : public JoinGameJob
    {
    public:
        JoinGameByUserListJob(GameManagerAPI *api, const GameManagerAPI::JoinGameCb &titleCb, uint32_t userIndex);
        virtual void cancel(BlazeError err);
    };

    /*! **************************************************************************************************************/
    /*! 
        \class CreateGameJob

        \brief
            JoinGameJob is a wrapper job to dispatch the title callback in case of a GameManagerAPI::CreateGame() request is
            canceled.
        \details
            CreateGameJob does nothing during idle() or execute(), but triggers the title callback with SDK_ERR_RPC_CANCELED
            if the job is canceled.

    ******************************************************************************************************************/
    class CreateGameJob : public GameManagerApiJob
    {
    public:

        CreateGameJob(GameManagerAPI *api, const GameManagerAPI::CreateGameCb &titleCb);

        virtual void cancel(BlazeError err);

        virtual void dispatch(BlazeError err, Game *game = nullptr);

        void getTitleCb(GameManagerAPI::CreateGameCb &cb) {
            GameManagerApiJob::getTitleCb(cb);
        }
    };

    /*! **************************************************************************************************************/
    /*! 
        \class MatchmakingScenarioJob

        \brief 'fake' job used to track a matchmaking scenario.  Allows title to cancel the overall matchmaking job,
                which spans an RPC and an async notification.
    ******************************************************************************************************************/
    class MatchmakingScenarioJob : public GameManagerApiJob
    {
    public:

        MatchmakingScenarioJob(GameManagerAPI *api, const GameManagerAPI::StartMatchmakingScenarioCb &titleCb)
            : GameManagerApiJob(api, titleCb)
        {}

        virtual void cancel(BlazeError err) 
        {
            //trigger title callback with cancel error.
            dispatch(err, getGame());
        }

        //  NOTE: only used for cancellation - goal is to move all API logic into the job so that callback can return correct values.
        virtual void dispatch(BlazeError err, Game *game = nullptr)
        {
            GameManagerAPI::StartMatchmakingScenarioCb cb;
            getTitleCb(cb);
            cb(err, getId(), nullptr, "");
        }

        void getTitleCb(GameManagerAPI::StartMatchmakingScenarioCb &cb) {
            GameManagerApiJob::getTitleCb(cb);
        }
    };

    /*! **************************************************************************************************************/
    /*! 
        \class CreateGameBrowserListJob

        \brief
         CreateGameBrowserListJob is a wrapper job to dispatch the title callback in the case of a GameManagerAPI::createGameBrowserList() 
         request is being canceled.
        \details
            CreateGameBrowserListJob does nothing during idle() or execute(), but triggers the title callback with SDK_ERR_RPC_CANCELED
            if the job is canceled.
    ******************************************************************************************************************/
    class CreateGameBrowserListJob : public GameManagerApiJob
    {
    public:

        CreateGameBrowserListJob(GameManagerAPI *api, const GameManagerAPI::CreateGameBrowserListCb &titleCb, GetGameListRequestPtr getGameListRequest)
            : GameManagerApiJob(api, titleCb), mGetGameListRequest(getGameListRequest)
        {}

        virtual void cancel(BlazeError err) 
        {
            //trigger title callback with cancel error.
            dispatch(err, getGame());         
        }

        //  NOTE: only used for cancellation - goal is to move all API logic into the job so that callback can return correct values.
        virtual void dispatch(BlazeError err, Game *game = nullptr)
        {
            GameManagerAPI::CreateGameBrowserListCb cb;
            getTitleCb(cb);
            Job::setExecuting(true);   
            cb(err, getId(), nullptr, "");
            Job::setExecuting(false);   
        }

        void getTitleCb(GameManagerAPI::CreateGameBrowserListCb &cb) {
            GameManagerApiJob::getTitleCb(cb);
        }

        GetGameListRequestPtr mGetGameListRequest;
    };

    /*! **************************************************************************************************************/
    /*!
    \class GameBrowserScenarioListJob

    \brief
    GameBrowserScenarioListJob is a wrapper job to dispatch the title callback in the case of a GameManagerAPI::createGameBrowserListScenario()
    request is being canceled.
    \details
    GameBrowserScenarioListJob does nothing during idle() or execute(), but triggers the title callback with SDK_ERR_RPC_CANCELED
    if the job is canceled.
    ******************************************************************************************************************/
    class GameBrowserScenarioListJob : public GameManagerApiJob
    {
    public:

        GameBrowserScenarioListJob(GameManagerAPI *api, const GameManagerAPI::CreateGameBrowserListCb &titleCb, GetGameListScenarioRequestPtr getGameListScenarioRequest)
            : GameManagerApiJob(api, titleCb), mGetGameListScenarioRequest(getGameListScenarioRequest)
        {}

        virtual void cancel(BlazeError err)
        {
            //trigger title callback with cancel error.
            dispatch(err, getGame());
        }

        //  NOTE: only used for cancellation - goal is to move all API logic into the job so that callback can return correct values.
        virtual void dispatch(BlazeError err, Game *game = nullptr)
        {
            GameManagerAPI::CreateGameBrowserListCb cb;
            getTitleCb(cb);
            Job::setExecuting(true);
            cb(err, getId(), nullptr, "");
            Job::setExecuting(false);
        }

        void getTitleCb(GameManagerAPI::CreateGameBrowserListCb &cb) {
            GameManagerApiJob::getTitleCb(cb);
        }

        GetGameListScenarioRequestPtr mGetGameListScenarioRequest;
    };

    /*! **************************************************************************************************************/
    /*! 
        \class GameSetupJob

        \brief
            Initiated as a result of an onNotifyGameSetup for a server-initiated game join/create where no title callback
            is needed.      
    ******************************************************************************************************************/
    class GameSetupJob : public GameManagerApiJob
    {
    public:

        GameSetupJob(GameManagerAPI *api, const GameSetupReason& reason);

        virtual void cancel(BlazeError err);
    };

} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGERAPI_JOBS_H
