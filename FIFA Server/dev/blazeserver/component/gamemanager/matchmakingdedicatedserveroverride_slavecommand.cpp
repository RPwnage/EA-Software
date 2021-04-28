/*! ************************************************************************************************/
/*!
\file setprivelagedmatchmaker_slavecommand.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/matchmakingdedicatedserveroverride_stub.h"
#include "gamemanager/rpc/gamemanagerslave/getmatchmakingdedicatedserveroverrides_stub.h"
#include "gamemanager/rpc/gamemanagerslave/matchmakingfillserversoverride_stub.h"
#include "gamemanager/rpc/gamemanagerslave/getmatchmakingfillserversoverride_stub.h"
#include "gamemanager/gamemanagerslaveimpl.h"

#include "gamemanager/matchmaker/matchmakingsessionslave.h"

namespace Blaze
{
namespace GameManager
{
    /*! ************************************************************************************************/
    /*! \brief Debug helper to allow player to override matchmaking and always attempt to match into the provided game.
        
        Our use case is that you already know the id of the game (via high debug logging, through the event log or status page etc).
        The request takes a 'playerId' and 'gameId' and always attempts to matchmake that user into the specified priority game id.
        Override is stored only as long as the server is running. Setting the gameId to INVALID_GAME_ID clears override entry for the player.
    *************************************************************************************************/
    class MatchmakingDedicatedServerOverrideCommand : public MatchmakingDedicatedServerOverrideCommandStub
    {
    public:

        MatchmakingDedicatedServerOverrideCommand(
            Message* message, MatchmakingDedicatedServerOverrideRequest *request, GameManagerSlaveImpl* componentImpl)
            :   MatchmakingDedicatedServerOverrideCommandStub(message, request),
            mComponent(*componentImpl)
        {
        }

        ~MatchmakingDedicatedServerOverrideCommand() override {}

    private:

        MatchmakingDedicatedServerOverrideCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_MATCHMAKING_DEDICATED_SERVER_OVERRIDE))
            {
                WARN_LOG("[MatchmakingDedicatedServerOverrideCommand].execute: User " << UserSession::getCurrentUserBlazeId() 
                    << " attempted to matchmake player[" <<  mRequest.getPlayerId() << "] into game[" << mRequest.getGameId() << "], no permission!" );

                return MatchmakingDedicatedServerOverrideCommand::ERR_AUTHORIZATION_REQUIRED;
            }

            UserInfoPtr userInfo;
            if (gUserSessionManager->lookupUserInfoByBlazeId(mRequest.getPlayerId(), userInfo) != Blaze::ERR_OK)
            {
                return GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
            }

            //See if the game exists
            if (mRequest.getGameId() != INVALID_GAME_ID)
            {
                Search::GetGamesRequest ggReq;
                ggReq.setFetchOnlyOne(true);
                ggReq.getGameIds().push_back(mRequest.getGameId());
                ggReq.setResponseType(Search::GET_GAMES_RESPONSE_TYPE_FULL);

                Search::GetGamesResponse ggResp;
                BlazeRpcError fetchErr = mComponent.fetchGamesFromSearchSlaves(ggReq, ggResp);
                if (fetchErr == ERR_OK)
                {
                    if (ggResp.getFullGameData().empty())
                    {
                        return GAMEMANAGER_ERR_INVALID_GAME_ID;
                    }

                }
                else
                {
                    return commandErrorFromBlazeError(fetchErr);
                }
            }

            // tell all the mm slaves
            BlazeRpcError err = Blaze::ERR_OK;
            Blaze::Matchmaker::MatchmakerSlave* matchmakerComponent = 
                static_cast<Blaze::Matchmaker::MatchmakerSlave*>(gController->getComponent(Blaze::Matchmaker::MatchmakerSlave::COMPONENT_ID, false, true, &err));
            if (matchmakerComponent == nullptr)
            {
                ERR_LOG("[MatchmakingDedicatedServerOverrideCommand].execute() - Unable to resolve matchmaker component.");
                return MatchmakingDedicatedServerOverrideCommand::ERR_SYSTEM;
            }
            Component::InstanceIdList ids;
            matchmakerComponent->getComponentInstances(ids, false);

            if (EA_UNLIKELY(ids.empty()))
            {
                // this should only occur if we're shutting down or all the masters have gone down.
                err = Blaze::ERR_SYSTEM;
            }
            else
            {
                for (Component::InstanceIdList::const_iterator itr = ids.begin(), end = ids.end(); itr != end; ++itr)
                {
                    RpcCallOptions opts;
                    opts.routeTo.setInstanceId(*itr);
                    err = matchmakerComponent->matchmakingDedicatedServerOverride(mRequest, opts);                        
                }
            }

            if (err != Blaze::ERR_OK)
            {
                // something went wrong, bail out early
                return commandErrorFromBlazeError(err);
            }

            // tell all the search slaves, too
            Blaze::Search::SearchSlave* searchComponent = 
                static_cast<Blaze::Search::SearchSlave*>(gController->getComponent(Blaze::Search::SearchSlave::COMPONENT_ID, false, true, &err));
            if (searchComponent == nullptr)
            {
                ERR_LOG("[MatchmakingDedicatedServerOverrideCommand].execute() - Unable to resolve search component.");
                return MatchmakingDedicatedServerOverrideCommand::ERR_SYSTEM;
            }


            searchComponent->getComponentInstances(ids, false);

            if (EA_UNLIKELY(ids.empty()))
            {
                // this should only occur if we're shutting down or all the masters have gone down.
                err = Blaze::ERR_SYSTEM;
            }
            else
            {
                for (Component::InstanceIdList::const_iterator itr = ids.begin(), end = ids.end(); itr != end; ++itr)
                {
                    //don't really care about the error, just send it to everyone.
                    RpcCallOptions opts;
                    opts.routeTo.setInstanceId(*itr);
                    err = searchComponent->matchmakingDedicatedServerOverride(mRequest, opts);
                }
            }

            // PACKER_TODO: change mm & search slaves to redis as well
            err = mComponent.updateDedicatedServerOverride(mRequest.getPlayerId(), mRequest.getGameId());

            return commandErrorFromBlazeError(err);
        }

    private:

        GameManagerSlaveImpl &mComponent;
    };


    //static creation factory method of command's stub class
    DEFINE_MATCHMAKINGDEDICATEDSERVEROVERRIDE_CREATE()


    class GetMatchmakingDedicatedServerOverridesCommand : public GetMatchmakingDedicatedServerOverridesCommandStub
    {
    public:

        GetMatchmakingDedicatedServerOverridesCommand(
            Message* message, GameManagerSlaveImpl* componentImpl)
            :   GetMatchmakingDedicatedServerOverridesCommandStub(message),
            mComponent(*componentImpl)
        {
        }

        ~GetMatchmakingDedicatedServerOverridesCommand() override {}

    private:

        GetMatchmakingDedicatedServerOverridesCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_MATCHMAKING_DEDICATED_SERVER_OVERRIDE))
            {
                WARN_LOG("[GetMatchmakingDedicatedServerOverridesCommand].execute: User " << UserSession::getCurrentUserBlazeId() 
                    << " attempted to get the matchmaking dedicated server overrides, no permission!" );

                return GetMatchmakingDedicatedServerOverridesCommandStub::ERR_AUTHORIZATION_REQUIRED;
            }

            // Since the GameManager itself doesn't track the data, we just query a random matchmaker slave: 
            BlazeRpcError err = Blaze::ERR_OK;
            Blaze::Matchmaker::MatchmakerSlave* matchmakerComponent = 
                static_cast<Blaze::Matchmaker::MatchmakerSlave*>(gController->getComponent(Blaze::Matchmaker::MatchmakerSlave::COMPONENT_ID, false, true, &err));
            if (matchmakerComponent == nullptr)
            {
                ERR_LOG("[GetMatchmakingDedicatedServerOverridesCommand].execute() - Unable to resolve matchmaker component.");
                return GetMatchmakingDedicatedServerOverridesCommandStub::ERR_SYSTEM;
            }

            err = matchmakerComponent->getMatchmakingDedicatedServerOverrides(mResponse);
            return commandErrorFromBlazeError(err);
        }

    private:

        GameManagerSlaveImpl &mComponent;
    };


    //static creation factory method of command's stub class
    DEFINE_GETMATCHMAKINGDEDICATEDSERVEROVERRIDES_CREATE()

    /*! ************************************************************************************************/
    /*! \brief Debug helper to allow player to override matchmaking and always attempt to match into the provided game.
        
        Our use case is that you already know the id of the game (via high debug logging, through the event log or status page etc).
        The request takes a 'playerId' and 'gameId' and always attempts to matchmake that user into the specified priority game id.
        Override is stored only as long as the server is running. Setting the gameId to INVALID_GAME_ID clears override entry for the player.
    *************************************************************************************************/
    class MatchmakingFillServersOverrideCommand : public MatchmakingFillServersOverrideCommandStub
    {
    public:

        MatchmakingFillServersOverrideCommand(
            Message* message, MatchmakingFillServersOverrideList *request, GameManagerSlaveImpl* componentImpl)
            :   MatchmakingFillServersOverrideCommandStub(message, request),
            mComponent(*componentImpl)
        {
        }

        ~MatchmakingFillServersOverrideCommand() override {}

    private:

        MatchmakingFillServersOverrideCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_MATCHMAKING_DEDICATED_SERVER_OVERRIDE))
            {
                WARN_LOG("[MatchmakingFillServersOverrideCommand].execute: User " << UserSession::getCurrentUserBlazeId() 
                    << " attempted to setup game fill overrides, no permission!" );

                return MatchmakingFillServersOverrideCommandStub::ERR_AUTHORIZATION_REQUIRED;
            }

            UserInfoPtr userInfo;

            // See if the games exist (if all are missing, give an error. Otherwise, accept them?)
            if (!mRequest.getGameIdList().empty())
            {
                Search::GetGamesRequest ggReq;
                ggReq.setFetchOnlyOne(true);
                mRequest.getGameIdList().copyInto(ggReq.getGameIds());
                ggReq.setResponseType(Search::GET_GAMES_RESPONSE_TYPE_FULL);

                Search::GetGamesResponse ggResp;
                BlazeRpcError fetchErr = mComponent.fetchGamesFromSearchSlaves(ggReq, ggResp);
                if (fetchErr == ERR_OK)
                {
                    if (ggResp.getFullGameData().empty())
                    {
                        return GAMEMANAGER_ERR_INVALID_GAME_ID;
                    }
                }
                else
                {
                    return commandErrorFromBlazeError(fetchErr);
                }
            }

            // tell all the mm slaves
            BlazeRpcError err = Blaze::ERR_OK;
            Blaze::Matchmaker::MatchmakerSlave* matchmakerComponent = 
                static_cast<Blaze::Matchmaker::MatchmakerSlave*>(gController->getComponent(Blaze::Matchmaker::MatchmakerSlave::COMPONENT_ID, false, true, &err));
            if (matchmakerComponent == nullptr)
            {
                ERR_LOG("[MatchmakingFillServersOverrideCommand].execute() - Unable to resolve matchmaker component.");
                return MatchmakingFillServersOverrideCommandStub::ERR_SYSTEM;
            }
            Component::InstanceIdList ids;
            matchmakerComponent->getComponentInstances(ids, false);

            if (EA_UNLIKELY(ids.empty()))
            {
                // this should only occur if we're shutting down or all the masters have gone down.
                err = Blaze::ERR_SYSTEM;
            }
            else
            {
                for (Component::InstanceIdList::const_iterator itr = ids.begin(), end = ids.end(); itr != end; ++itr)
                {
                    RpcCallOptions opts;
                    opts.routeTo.setInstanceId(*itr);
                    err = matchmakerComponent->matchmakingFillServersOverride(mRequest, opts);                        
                }
            }

            if (err != Blaze::ERR_OK)
            {
                // something went wrong, bail out early
                return commandErrorFromBlazeError(err);
            }

            // tell all the search slaves, too
            Blaze::Search::SearchSlave* searchComponent = 
                static_cast<Blaze::Search::SearchSlave*>(gController->getComponent(Blaze::Search::SearchSlave::COMPONENT_ID, false, true, &err));
            if (searchComponent == nullptr)
            {
                ERR_LOG("[MatchmakingFillServersOverrideCommand].execute() - Unable to resolve search component.");
                return MatchmakingFillServersOverrideCommandStub::ERR_SYSTEM;
            }


            searchComponent->getComponentInstances(ids, false);

            if (EA_UNLIKELY(ids.empty()))
            {
                // this should only occur if we're shutting down or all the masters have gone down.
                err = Blaze::ERR_SYSTEM;
            }
            else
            {
                for (Component::InstanceIdList::const_iterator itr = ids.begin(), end = ids.end(); itr != end; ++itr)
                {
                    //don't really care about the error, just send it to everyone.
                    RpcCallOptions opts;
                    opts.routeTo.setInstanceId(*itr);
                    err = searchComponent->matchmakingFillServersOverride(mRequest, opts);
                }
            }

            return commandErrorFromBlazeError(err);
        }

    private:

        GameManagerSlaveImpl &mComponent;
    };


    //static creation factory method of command's stub class
    DEFINE_MATCHMAKINGFILLSERVERSOVERRIDE_CREATE()

    /*! ************************************************************************************************/
    /*! \brief Debug helper to allow player to override matchmaking and always attempt to match into the provided game.
        
        Our use case is that you already know the id of the game (via high debug logging, through the event log or status page etc).
        The request takes a 'playerId' and 'gameId' and always attempts to matchmake that user into the specified priority game id.
        Override is stored only as long as the server is running. Setting the gameId to INVALID_GAME_ID clears override entry for the player.
    *************************************************************************************************/
    class GetMatchmakingFillServersOverrideCommand : public GetMatchmakingFillServersOverrideCommandStub
    {
    public:

        GetMatchmakingFillServersOverrideCommand(
            Message* message, GameManagerSlaveImpl* componentImpl)
            :   GetMatchmakingFillServersOverrideCommandStub(message),
            mComponent(*componentImpl)
        {
        }

        ~GetMatchmakingFillServersOverrideCommand() override {}

    private:

        GetMatchmakingFillServersOverrideCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_MATCHMAKING_DEDICATED_SERVER_OVERRIDE))
            {
                WARN_LOG("[GetMatchmakingFillServersOverrideCommand].execute: User " << UserSession::getCurrentUserBlazeId() 
                    << " attempted to get the matchmaking fill server overrides, no permission!" );

                return GetMatchmakingFillServersOverrideCommandStub::ERR_AUTHORIZATION_REQUIRED;
            }

            // Since the GameManager itself doesn't track the data, we just query a random matchmaker slave: 
            BlazeRpcError err = Blaze::ERR_OK;
            Blaze::Matchmaker::MatchmakerSlave* matchmakerComponent = 
                static_cast<Blaze::Matchmaker::MatchmakerSlave*>(gController->getComponent(Blaze::Matchmaker::MatchmakerSlave::COMPONENT_ID, false, true, &err));
            if (matchmakerComponent == nullptr)
            {
                ERR_LOG("[GetMatchmakingFillServersOverrideCommand].execute() - Unable to resolve matchmaker component.");
                return GetMatchmakingFillServersOverrideCommandStub::ERR_SYSTEM;
            }

            err = matchmakerComponent->getMatchmakingFillServersOverride(mResponse);
            return commandErrorFromBlazeError(err);
        }

    private:

        GameManagerSlaveImpl &mComponent;
    };


    //static creation factory method of command's stub class
    DEFINE_GETMATCHMAKINGFILLSERVERSOVERRIDE_CREATE()

} // namespace GameManager
} // namespace Blaze
