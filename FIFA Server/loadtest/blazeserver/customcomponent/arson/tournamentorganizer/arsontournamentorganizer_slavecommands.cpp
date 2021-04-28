/*! ************************************************************************************************/
/*!
    \file arsontournamentorganizer_slavecommands.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "arson/tournamentorganizer/rpc/arsontournamentorganizerslave/createtournament_stub.h"
#include "arson/tournamentorganizer/rpc/arsontournamentorganizerslave/createtournamentteam_stub.h"
#include "arson/tournamentorganizer/rpc/arsontournamentorganizerslave/gettournament_stub.h"
#include "arson/tournamentorganizer/rpc/arsontournamentorganizerslave/removetournament_stub.h"
#include "arson/tournamentorganizer/rpc/arsontournamentorganizerslave/updatetournament_stub.h"
#include "arson/tournamentorganizer/rpc/arsontournamentorganizerslave/gettournamentteam_stub.h"
#include "arson/tournamentorganizer/rpc/arsontournamentorganizerslave/gettournamentteams_stub.h"
#include "arson/tournamentorganizer/rpc/arsontournamentorganizerslave/jointournamentteam_stub.h"
#include "arson/tournamentorganizer/rpc/arsontournamentorganizerslave/leavetournamentteam_stub.h"
#include "arson/tournamentorganizer/rpc/arsontournamentorganizerslave/settournamentteammatch_stub.h"
#include "arson/tournamentorganizer/rpc/arsontournamentorganizerslave/submittournamentmatchresult_stub.h"
#include "arson/tournamentorganizer/rpc/arsontournamentorganizerslave/updatetournamentteam_stub.h"
#include "arson/tournamentorganizer/rpc/arsontournamentorganizerslave/handlegameeventstart_stub.h"
#include "arson/tournamentorganizer/rpc/arsontournamentorganizerslave/handlegameeventend_stub.h"
#include "arson/tournamentorganizer/rpc/arsontournamentorganizerslave/testsendgameeventstart_stub.h"
#include "arson/tournamentorganizer/rpc/arsontournamentorganizerslave/testsendgameeventend_stub.h"
#include "arson/tournamentorganizer/rpc/arsontournamentorganizerslave/gettournaments_stub.h"
#include "arson/tournamentorganizer/rpc/arsontournamentorganizerslave/cleanuptournaments_stub.h"
#include "arson/tournamentorganizer/tdf/arsontournamentorganizer_server.h"
#include "arson/tournamentorganizer/arsontournamentorganizerslaveimpl.h"
#include "component/gamemanager/rpc/gamemanager_defines.h" // for PERMISSION_TOURNAMENT_PROVIDER
#include "gamemanager/externalsessions/externaluserscommoninfo.h" //for hasFirstPartyId in SubmitTournamentMatchResultCommand
#include "arsonslaveimpl.h"

namespace Blaze
{
namespace Arson
{

    /*! ************************************************************************************************/
    /*! \brief Create tournament
    ***************************************************************************************************/
    class CreateTournamentCommand : public CreateTournamentCommandStub
    {
    public:
        CreateTournamentCommand(Message* message, CreateTournamentRequest *request, ArsonTournamentOrganizerSlaveImpl* componentImpl) : CreateTournamentCommandStub(message, request), mComponent(*componentImpl) {}
        ~CreateTournamentCommand() override {}

    private:
        CreateTournamentCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_TOURNAMENT_PROVIDER))
            {
                return commandErrorFromBlazeError(ERR_AUTHORIZATION_REQUIRED);
            }
            if (EA_UNLIKELY(mComponent.getExternalTournamentUtil() == nullptr))
            {
                ERR_LOG("[CreateTournamentCommand].execute: internal error: External service util not initialized, cannot complete request.");
                return ERR_SYSTEM;
            }
            mRequest.getTournamentIdentification().copyInto(mResponse.getTournamentIdentification());
            // TBA: possible enhancement: use db or master tracking

            // Update external tournament service
            BlazeRpcError err = mComponent.getExternalTournamentUtil()->createTournament(mRequest, &mResponse.getArsonExternalTournamentInfo());
            return commandErrorFromBlazeError(err);
        }
    private:
        ArsonTournamentOrganizerSlaveImpl &mComponent;
    };
    DEFINE_CREATETOURNAMENT_CREATE()

    /*! ************************************************************************************************/
    /*! \brief Create tournament team
    ***************************************************************************************************/
    class CreateTournamentTeamCommand : public CreateTournamentTeamCommandStub
    {
    public:
        CreateTournamentTeamCommand(Message* message, CreateTournamentTeamRequest *request, ArsonTournamentOrganizerSlaveImpl* componentImpl) : CreateTournamentTeamCommandStub(message, request), mComponent(*componentImpl) {}
        ~CreateTournamentTeamCommand() override {}

    private:
        CreateTournamentTeamCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_TOURNAMENT_PROVIDER))
            {
                return commandErrorFromBlazeError(ERR_AUTHORIZATION_REQUIRED);
            }
            if (mRequest.getMembers().empty())
            {
                WARN_LOG("[CreateTournamentTeamCommand].execute: request did not specify any players to join.");
                return commandErrorFromBlazeError(ARSON_TO_ERR_INVALID_TOURNAMENT_PARAMETERS);
            }
            // TBA: possible enhancement: use db or master tracking

            const UserIdentification& creator = *mRequest.getMembers().front();
            BlazeRpcError err = createAndJoinTeam(creator);
            return commandErrorFromBlazeError(err);
        }
    private:
        ArsonTournamentOrganizerSlaveImpl &mComponent;

        BlazeRpcError createAndJoinTeam(const UserIdentification& creator)
        {
            eastl::string sessionName;
            BlazeRpcError err = mComponent.getNextTeamExternalSessionName(sessionName, mComponent.getConfig());
            if (err != ERR_OK)
                return err;

            // MS now requires specifying a registration date
            char8_t timeBuf[64];
            TimeValue::getTimeOfDay().toAccountString(timeBuf, sizeof(timeBuf), true);

            // create team
            CreateTournamentTeamParameters createParams;
            creator.copyInto(*createParams.getMembers().pull_back());
            createParams.getTeamIdentification().setDisplayName(mRequest.getDisplayName());
            createParams.getTeamIdentification().setUniqueName(sessionName.c_str());
            
            createParams.setRegistrationState(mRequest.getRegistrationState());
            createParams.setRegistrationDate(timeBuf);
            mRequest.getTournamentIdentification().copyInto(createParams.getTournamentIdentification());
            mRequest.getStanding().copyInto(createParams.getStanding());

            err = mComponent.getExternalTournamentUtil()->createTeam(createParams, &mResponse);
            if (err != ERR_OK)
                return err;

            // join remaining members to team
            JoinTournamentTeamParameters joinParams;
            createParams.getTeamIdentification().copyInto(joinParams.getTeamIdentification());
            createParams.getTournamentIdentification().copyInto(joinParams.getTournamentIdentification());
            UserIdentificationList::const_iterator itr = mRequest.getMembers().begin();
            ++itr;
            for (UserIdentificationList::const_iterator end = mRequest.getMembers().end(); itr != end; ++itr)
            {
                (*itr)->copyInto(joinParams.getUser());
                err = mComponent.getExternalTournamentUtil()->joinTeam(joinParams, &mResponse);
                if (err != ERR_OK)
                {
                    ExternalId extId = ArsonSlaveImpl::getExternalIdFromPlatformInfo(joinParams.getUser().getPlatformInfo());
                    ERR_LOG("[CreateTournamentTeamCommand].createAndJoinTeam: cleaning up team(" << sessionName.c_str() << ":displayName=" <<
                        mRequest.getDisplayName() << ") from external service, on failure to join member(" << joinParams.getUser().getBlazeId() << ":extId=" << extId << ").");

                    // cleanup by leaving users
                    for (UserIdentificationList::const_iterator lvItr = mRequest.getMembers().begin(), lvEnd = mRequest.getMembers().end(); (lvItr != itr && lvItr != lvEnd); ++lvItr)
                    {
                        LeaveTournamentTeamParameters leaveParams;
                        createParams.getTeamIdentification().copyInto(leaveParams.getTeamIdentification());
                        createParams.getTournamentIdentification().copyInto(leaveParams.getTournamentIdentification());
                        (*lvItr)->copyInto(leaveParams.getUser());

                        mComponent.getExternalTournamentUtil()->leaveTeam(leaveParams);
                    }
                    break;
                }
            }
            return err;
        }
    };
    DEFINE_CREATETOURNAMENTTEAM_CREATE()

    /*! ************************************************************************************************/
    /*! \brief Get tournament info
    ***************************************************************************************************/
    class GetTournamentCommand : public GetTournamentCommandStub
    {
    public:
        GetTournamentCommand(Message* message, GetTournamentRequest *request, ArsonTournamentOrganizerSlaveImpl* componentImpl) : GetTournamentCommandStub(message, request), mComponent(*componentImpl){}
        ~GetTournamentCommand() override{}

    private:
        GetTournamentCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_TOURNAMENT_PROVIDER))
            {
                return commandErrorFromBlazeError(ERR_AUTHORIZATION_REQUIRED);
            }
            if (EA_UNLIKELY(mComponent.getExternalTournamentUtil() == nullptr))
            {
                ERR_LOG("[GetTournamentCommand].execute: internal error: External service util not initialized, cannot complete request.");
                return ERR_SYSTEM;
            }

            mRequest.getTournamentIdentification().copyInto(mResponse.getTournamentIdentification());

            // TBA: possible enhancement: use db or master tracking

            BlazeRpcError err = mComponent.getExternalTournamentUtil()->getTournament(mRequest, mResponse.getArsonExternalTournamentInfo());
            return commandErrorFromBlazeError(err);
        }
    private:
        ArsonTournamentOrganizerSlaveImpl &mComponent;
    };
    DEFINE_GETTOURNAMENT_CREATE()

    /*! ************************************************************************************************/
    /*! \brief Get tournament team
    ***************************************************************************************************/
    class GetTournamentTeamCommand : public GetTournamentTeamCommandStub
    {
    public:
        GetTournamentTeamCommand(Message* message, GetTournamentTeamRequest *request, ArsonTournamentOrganizerSlaveImpl* componentImpl) : GetTournamentTeamCommandStub(message, request), mComponent(*componentImpl) {}
        ~GetTournamentTeamCommand() override {}

    private:
        GetTournamentTeamCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_TOURNAMENT_PROVIDER))
            {
                return commandErrorFromBlazeError(ERR_AUTHORIZATION_REQUIRED);
            }
            if (EA_UNLIKELY(mComponent.getExternalTournamentUtil() == nullptr))
            {
                ERR_LOG("[GetTournamentTeamCommand].execute: internal error: External service util not initialized, cannot complete request.");
                return ERR_SYSTEM;
            }
            // TBA: possible enhancement: use db or master tracking

            // get external service's tournament team
            BlazeRpcError err = ERR_OK;
            
            if (mRequest.getTournamentTeamIdentification().getUniqueName()[0] != '\0')
            {
                err = mComponent.getExternalTournamentUtil()->getTeamByName(mRequest, mResponse);
            }
            else if (GameManager::hasFirstPartyId(mRequest.getUser()))
            {
                err = mComponent.getExternalTournamentUtil()->getTeamForUser(mRequest, mResponse);
            }
            else
            {
                ERR_LOG("[GetTournamentTeamCommand].execute: test error: missing value user/team to get in request.");
                err = GetTournamentTeamCommandStub::ARSON_TO_ERR_INVALID_TOURNAMENT_TEAM;
            }
            return commandErrorFromBlazeError(err);
        }

    private:
        ArsonTournamentOrganizerSlaveImpl &mComponent;
    };
    DEFINE_GETTOURNAMENTTEAM_CREATE()

    /*! ************************************************************************************************/
    /*! \brief remove tournament
    ***************************************************************************************************/
    class RemoveTournamentCommand : public RemoveTournamentCommandStub
    {
    public:
        RemoveTournamentCommand(Message* message, RemoveTournamentRequest *request, ArsonTournamentOrganizerSlaveImpl* componentImpl) : RemoveTournamentCommandStub(message, request), mComponent(*componentImpl) {}
        ~RemoveTournamentCommand() override {}

    private:
        RemoveTournamentCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_TOURNAMENT_PROVIDER))
            {
                return commandErrorFromBlazeError(ERR_AUTHORIZATION_REQUIRED);
            }
            if (EA_UNLIKELY(mComponent.getExternalTournamentUtil() == nullptr))
            {
                ERR_LOG("[RemoveTournamentCommand].execute: internal error: External service util not initialized, cannot complete request.");
                return ERR_SYSTEM;
            }
            // TBA: possible enhancement: use db or master tracking

            BlazeRpcError err = mComponent.getExternalTournamentUtil()->removeTournament(mRequest);
            return commandErrorFromBlazeError(err);
        }
    private:
        ArsonTournamentOrganizerSlaveImpl &mComponent;
    };
    DEFINE_REMOVETOURNAMENT_CREATE()

    /*! ************************************************************************************************/
    /*! \brief update tournament
    ***************************************************************************************************/
    class UpdateTournamentCommand : public UpdateTournamentCommandStub
    {
    public:
        UpdateTournamentCommand(Message* message, UpdateTournamentRequest *request, ArsonTournamentOrganizerSlaveImpl* componentImpl) : UpdateTournamentCommandStub(message, request), mComponent(*componentImpl) {}
        ~UpdateTournamentCommand() override{}

    private:
        UpdateTournamentCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_TOURNAMENT_PROVIDER))
            {
                return commandErrorFromBlazeError(ERR_AUTHORIZATION_REQUIRED);
            }
            // TBA: possible enhancement: use db or master tracking

            // update external service's tournament team
            if (EA_UNLIKELY(mComponent.getExternalTournamentUtil() == nullptr))
            {
                ERR_LOG("[UpdateTournamentCommand].execute: internal error: External service util not initialized, cannot complete request.");
                return ERR_SYSTEM;
            }
            return commandErrorFromBlazeError(mComponent.getExternalTournamentUtil()->updateTournament(mRequest, &mResponse.getArsonExternalTournamentInfo()));
        }
    private:
        ArsonTournamentOrganizerSlaveImpl &mComponent;
    };
    DEFINE_UPDATETOURNAMENT_CREATE()

    /*! ************************************************************************************************/
    /*! \brief get tournament teams for user
    ***************************************************************************************************/
    class GetTournamentTeamsCommand : public GetTournamentTeamsCommandStub
    {
    public:
        GetTournamentTeamsCommand(Message* message, GetTournamentTeamsRequest *request, ArsonTournamentOrganizerSlaveImpl* componentImpl) : GetTournamentTeamsCommandStub(message, request), mComponent(*componentImpl) {}
        ~GetTournamentTeamsCommand() override {}

    private:
        GetTournamentTeamsCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_TOURNAMENT_PROVIDER))
            {
                return commandErrorFromBlazeError(ERR_AUTHORIZATION_REQUIRED);
            }
            if (EA_UNLIKELY(mComponent.getExternalTournamentUtil() == nullptr))
            {
                ERR_LOG("[GetTournamentTeamsCommand].execute: internal error: External service util not initialized, cannot complete request.");
                return ERR_SYSTEM;
            }
            // TBA: possible enhancement: use db or master tracking

            // get external service's tournament team
            BlazeRpcError err = mComponent.getExternalTournamentUtil()->getTeamsForUser(mRequest, mResponse.getTeams());
            return commandErrorFromBlazeError(err);
        }

    private:
        ArsonTournamentOrganizerSlaveImpl &mComponent;
    };
    DEFINE_GETTOURNAMENTTEAMS_CREATE()

    /*! ************************************************************************************************/
    /*! \brief join tournament team
    ***************************************************************************************************/
    class JoinTournamentTeamCommand : public JoinTournamentTeamCommandStub
    {
    public:
        JoinTournamentTeamCommand(Message* message, JoinTournamentTeamRequest *request, ArsonTournamentOrganizerSlaveImpl* componentImpl) : JoinTournamentTeamCommandStub(message, request), mComponent(*componentImpl) {}
        ~JoinTournamentTeamCommand() override {}

    private:
        JoinTournamentTeamCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_TOURNAMENT_PROVIDER))
            {
                return commandErrorFromBlazeError(ERR_AUTHORIZATION_REQUIRED);
            }
            if (EA_UNLIKELY(mComponent.getExternalTournamentUtil() == nullptr))
            {
                ERR_LOG("[JoinTournamentTeamCommand].execute: internal error: External service util not initialized, cannot complete request.");
                return ERR_SYSTEM;
            }
            // TBA: possible enhancement: use db or master tracking

            BlazeRpcError err = mComponent.getExternalTournamentUtil()->joinTeam(mRequest, &mResponse);
            return commandErrorFromBlazeError(err);
        }

    private:
        ArsonTournamentOrganizerSlaveImpl &mComponent;
    };
    DEFINE_JOINTOURNAMENTTEAM_CREATE()

    /*! ************************************************************************************************/
    /*! \brief leave tournament team
    ***************************************************************************************************/
    class LeaveTournamentTeamCommand : public LeaveTournamentTeamCommandStub
    {
    public:
        LeaveTournamentTeamCommand(Message* message, LeaveTournamentTeamRequest *request, ArsonTournamentOrganizerSlaveImpl* componentImpl) : LeaveTournamentTeamCommandStub(message, request), mComponent(*componentImpl) {}
        ~LeaveTournamentTeamCommand() override {}

    private:
        LeaveTournamentTeamCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_TOURNAMENT_PROVIDER))
            {
                return commandErrorFromBlazeError(ERR_AUTHORIZATION_REQUIRED);
            }
            if (EA_UNLIKELY(mComponent.getExternalTournamentUtil() == nullptr))
            {
                ERR_LOG("[LeaveTournamentTeamCommand].execute: internal error: External service util not initialized, cannot complete request.");
                return ERR_SYSTEM;
            }
            // TBA: possible enhancement: use db or master tracking

            return commandErrorFromBlazeError(mComponent.getExternalTournamentUtil()->leaveTeam(mRequest));
        }
    private:
        ArsonTournamentOrganizerSlaveImpl &mComponent;
    };
    DEFINE_LEAVETOURNAMENTTEAM_CREATE()

    /*! ************************************************************************************************/
    /*! \brief set the tournament team's current/pending match
    ***************************************************************************************************/
    class SetTournamentTeamMatchCommand : public SetTournamentTeamMatchCommandStub
    {
    public:
        SetTournamentTeamMatchCommand(Message* message, SetTournamentTeamMatchRequest *request, ArsonTournamentOrganizerSlaveImpl* componentImpl) : SetTournamentTeamMatchCommandStub(message, request), mComponent(*componentImpl) {}
        ~SetTournamentTeamMatchCommand() override {}

    private:
        SetTournamentTeamMatchCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_TOURNAMENT_PROVIDER))
            {
                return commandErrorFromBlazeError(ERR_AUTHORIZATION_REQUIRED);
            }
            if (EA_UNLIKELY(mComponent.getExternalTournamentUtil() == nullptr))
            {
                ERR_LOG("[SetTournamentTeamMatchCommand].execute: internal error: External service util not initialized, cannot complete request.");
                return ERR_SYSTEM;
            }

            SetTournamentTeamMatchParameters params;

            // having a bye should not also specify a match to set
            if ((mRequest.getMatchId() != GameManager::INVALID_GAME_ID) && mRequest.getBye())
            {
                ERR_LOG("[SetTournamentTeamMatchCommand].execute: request specified a match but had bye 'true'. bye true and must be mutually exclusive.");
                return commandErrorFromBlazeError(ARSON_TO_ERR_INVALID_TOURNAMENT_PARAMETERS);
            }

            if (mRequest.getMatchId() != GameManager::INVALID_GAME_ID)
            {
                // get the game
                Blaze::GameManager::ReplicatedGameData gameData;
                BlazeRpcError getErr = mComponent.getMatchData(mRequest.getMatchId(), gameData);
                if (getErr != ERR_OK)
                {
                    ERR_LOG("[SetTournamentTeamMatchCommand].execute: failed to get game(" << mRequest.getMatchId() << "), Error(" << ErrorHelp::getErrorName(getErr) << ").");
                    return commandErrorFromBlazeError(getErr);
                }
                gameData.getExternalSessionIdentification().copyInto(params.getMatchIdentification());
            }
            TRACE_LOG("[SetTournamentTeamMatchCommand].execute: setting team(" << mRequest.getTeamIdentification().getDisplayName() << ")'s game to (" << ((mRequest.getMatchId() != GameManager::INVALID_GAME_ID) ? eastl::string().sprintf("%" PRIu64 "", mRequest.getMatchId()).c_str() : "<no game>") << ").");

            // possible enhancement: use db for teams tracking

            // Update first party tournaments as needed:
            mRequest.getCaller().copyInto(params.getCaller());
            mRequest.getTournamentIdentification().copyInto(params.getTournamentIdentification());
            mRequest.getTeamIdentification().copyInto(params.getTeamIdentification());
            mRequest.getLabel().copyInto(params.getLabel());
            params.setBye(mRequest.getBye());

            BlazeRpcError sessErr = mComponent.getExternalTournamentUtil()->setTeamMatch(params, &mResponse);
            return commandErrorFromBlazeError(sessErr);
        }
    private:
        ArsonTournamentOrganizerSlaveImpl &mComponent;
    };
    DEFINE_SETTOURNAMENTTEAMMATCH_CREATE()
    
    /*! ************************************************************************************************/
    /*! \brief post results for the tournament match
    ***************************************************************************************************/
    class SubmitTournamentMatchResultCommand : public SubmitTournamentMatchResultCommandStub
    {
    public:
        SubmitTournamentMatchResultCommand(Message* message, SubmitTournamentMatchResultRequest *request, ArsonTournamentOrganizerSlaveImpl* componentImpl) : SubmitTournamentMatchResultCommandStub(message, request), mComponent(*componentImpl) {}
        ~SubmitTournamentMatchResultCommand() override {}

    private:
        SubmitTournamentMatchResultCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_TOURNAMENT_PROVIDER))
            {
                return commandErrorFromBlazeError(ERR_AUTHORIZATION_REQUIRED);
            }
            if (EA_UNLIKELY(mComponent.getExternalTournamentUtil() == nullptr))
            {
                ERR_LOG("[SubmitTournamentMatchResultCommand].execute: internal error: External service util not initialized, cannot complete request.");
                return ERR_SYSTEM;
            }
            if (!GameManager::hasFirstPartyId(mRequest.getReporter()))
            {
                eastl::string buf;
                ERR_LOG("[SubmitTournamentMatchResultCommand].execute: error: invalid reporter(" << GameManager::externalUserIdentificationToString(mRequest.getReporter(), buf) << ") specified, cannot complete request.");
                return commandErrorFromBlazeError(ARSON_TO_ERR_INVALID_USER);
            }
            // get the game
            Blaze::GameManager::ReplicatedGameData gameData;
            BlazeRpcError getErr = mComponent.getMatchData(mRequest.getMatchId(), gameData);
            if (getErr != ERR_OK)
            {
                ERR_LOG("[SubmitTournamentMatchResultCommand].execute: failed to get game(" << mRequest.getMatchId() << "), Error(" << ErrorHelp::getErrorName(getErr) << ").");
                return commandErrorFromBlazeError(getErr);
            }
            // TBA: possible enhancement: use db or master tracking

            // Update first party tournaments as needed:
            SubmitTournamentMatchResultParameters params;
            mRequest.getReporter().copyInto(params.getReporter());
            mRequest.getTeamResults().copyInto(params.getTeamResults());
            gameData.getExternalSessionIdentification().copyInto(params.getMatchIdentification());
            gameData.getTournamentIdentification().copyInto(params.getTournamentIdentification());

            BlazeRpcError sessErr = mComponent.getExternalTournamentUtil()->submitResult(params);
            if (sessErr != ERR_OK)
            {
                ExternalId extId = ArsonSlaveImpl::getExternalIdFromPlatformInfo(params.getReporter().getPlatformInfo());
                ERR_LOG("[SubmitTournamentMatchResultCommand].execute: failed to submit result for game(" << mRequest.getMatchId() << "), reporter(" << params.getReporter().getBlazeId() << ":extId=" << extId << "), Error(" << ErrorHelp::getErrorName(sessErr) << ").");
            }
            return commandErrorFromBlazeError(sessErr);
        }
    private:
        ArsonTournamentOrganizerSlaveImpl &mComponent;
    };
    DEFINE_SUBMITTOURNAMENTMATCHRESULT_CREATE()

    /*! ************************************************************************************************/
    /*! \brief update tournament team
    ***************************************************************************************************/
    class UpdateTournamentTeamCommand : public UpdateTournamentTeamCommandStub
    {
    public:
        UpdateTournamentTeamCommand(Message* message, UpdateTournamentTeamRequest *request, ArsonTournamentOrganizerSlaveImpl* componentImpl) : UpdateTournamentTeamCommandStub(message, request), mComponent(*componentImpl) {}
        ~UpdateTournamentTeamCommand() override {}

    private:
        UpdateTournamentTeamCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_TOURNAMENT_PROVIDER))
            {
                return commandErrorFromBlazeError(ERR_AUTHORIZATION_REQUIRED);
            }
            if (EA_UNLIKELY(mComponent.getExternalTournamentUtil() == nullptr))
            {
                ERR_LOG("[UpdateTournamentTeamCommand].execute: internal error: External service util not initialized, cannot complete request.");
                return ERR_SYSTEM;
            }
            return commandErrorFromBlazeError(mComponent.getExternalTournamentUtil()->updateTeam(mRequest, &mResponse));
        }
    private:
        ArsonTournamentOrganizerSlaveImpl &mComponent;
    };
    DEFINE_UPDATETOURNAMENTTEAM_CREATE()

    /*! ************************************************************************************************/
    /*! \brief handle notificaton sent from Blaze Server to the Tournament Organizer, when a match is starting
    ***************************************************************************************************/
    class HandleGameEventStartCommand : public HandleGameEventStartCommandStub
    {
    public:
        HandleGameEventStartCommand(Message* message, Blaze::GameManager::ExternalHttpGameEventData *request, ArsonTournamentOrganizerSlaveImpl* componentImpl) : HandleGameEventStartCommandStub(message, request), mComponent(*componentImpl) {}
        ~HandleGameEventStartCommand() override {}

    private:
        HandleGameEventStartCommandStub::Errors execute() override
        {
            TRACE_LOG("[HandleGameEventStartCommand].execute: ARSON recieved request: " << mRequest);
            return commandErrorFromBlazeError(ERR_OK);
        }
    private:
        ArsonTournamentOrganizerSlaveImpl &mComponent;
    };
    DEFINE_HANDLEGAMEEVENTSTART_CREATE()

    /*! ************************************************************************************************/
    /*! \brief handle notificaton sent from Blaze Server to the Tournament Organizer, when a match is ending
    ***************************************************************************************************/
    class HandleGameEventEndCommand : public HandleGameEventEndCommandStub
    {
    public:
        HandleGameEventEndCommand(Message* message, Blaze::GameManager::ExternalHttpGameEventData *request, ArsonTournamentOrganizerSlaveImpl* componentImpl) : HandleGameEventEndCommandStub(message, request), mComponent(*componentImpl) {}
        ~HandleGameEventEndCommand() override {}

    private:
        HandleGameEventEndCommandStub::Errors execute() override
        {
            TRACE_LOG("[HandleGameEventEndCommand].execute: ARSON recieved request: " << mRequest);
            return commandErrorFromBlazeError(ERR_OK);
        }
    private:
        ArsonTournamentOrganizerSlaveImpl &mComponent;
    };
    DEFINE_HANDLEGAMEEVENTEND_CREATE()

    /*! ************************************************************************************************/
    /*! \brief test artificially sending the game event to its currently assigned URL
    ***************************************************************************************************/
    class TestSendGameEventStartCommand : public TestSendGameEventStartCommandStub
    {
    public:
        TestSendGameEventStartCommand(Message* message, Blaze::Arson::TestSendTournamentGameEventRequest *request, ArsonTournamentOrganizerSlaveImpl* componentImpl) : TestSendGameEventStartCommandStub(message, request), mComponent(*componentImpl) {}
        ~TestSendGameEventStartCommand() override {}

    private:
        TestSendGameEventStartCommandStub::Errors execute() override
        {
            Blaze::Arson::TestSendTournamentGameEventResponse response;
            BlazeRpcError err = mComponent.testSendTournamentGameEvent(mRequest, response, true);
            response.copyInto(err == ERR_OK ? mResponse : mErrorResponse);
            return commandErrorFromBlazeError(err);
        }
    private:
        ArsonTournamentOrganizerSlaveImpl &mComponent;
    };
    DEFINE_TESTSENDGAMEEVENTSTART_CREATE()

    /*! ************************************************************************************************/
    /*! \brief test artificially sending the game event to its currently assigned URL
    ***************************************************************************************************/
    class TestSendGameEventEndCommand : public TestSendGameEventEndCommandStub
    {
    public:
        TestSendGameEventEndCommand(Message* message, Blaze::Arson::TestSendTournamentGameEventRequest *request, ArsonTournamentOrganizerSlaveImpl* componentImpl) : TestSendGameEventEndCommandStub(message, request), mComponent(*componentImpl) {}
        ~TestSendGameEventEndCommand() override {}

    private:
        TestSendGameEventEndCommandStub::Errors execute() override
        {
            Blaze::Arson::TestSendTournamentGameEventResponse response;
            BlazeRpcError err = mComponent.testSendTournamentGameEvent(mRequest, response, false);
           response.copyInto(err == ERR_OK ? mResponse : mErrorResponse);
           return commandErrorFromBlazeError(err);
        }
    private:
        ArsonTournamentOrganizerSlaveImpl &mComponent;
    };
    DEFINE_TESTSENDGAMEEVENTEND_CREATE()


    /*! ************************************************************************************************/
    /*! \brief Get all tournaments' infos
    ***************************************************************************************************/
    class GetTournamentsCommand : public GetTournamentsCommandStub
    {
    public:
        GetTournamentsCommand(Message* message, GetTournamentsRequest *request, ArsonTournamentOrganizerSlaveImpl* componentImpl) : GetTournamentsCommandStub(message, request), mComponent(*componentImpl) {}
        ~GetTournamentsCommand() override {}

    private:
        GetTournamentsCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_TOURNAMENT_PROVIDER))
            {
                return commandErrorFromBlazeError(ERR_AUTHORIZATION_REQUIRED);
            }
            if (EA_UNLIKELY(mComponent.getExternalTournamentUtil() == nullptr))
            {
                ERR_LOG("[GetTournamentsCommand].execute: internal error: External service util not initialized, cannot complete request.");
                return ERR_SYSTEM;
            }

            // TBA: possible enhancement: use db or master tracking

            BlazeRpcError err = mComponent.getExternalTournamentUtil()->getTournaments(mRequest, mResponse);
            return commandErrorFromBlazeError(err);
        }
    private:
        ArsonTournamentOrganizerSlaveImpl &mComponent;
    };
    DEFINE_GETTOURNAMENTS_CREATE()

    /*! ************************************************************************************************/
    /*! \brief Cleanup/cancel tournaments by title/organizer
    ***************************************************************************************************/
    class CleanupTournamentsCommand : public CleanupTournamentsCommandStub
    {
    public:
        CleanupTournamentsCommand(Message* message, CleanupTournamentsRequest *request, ArsonTournamentOrganizerSlaveImpl* componentImpl)
            : CleanupTournamentsCommandStub(message, request), mComponent(*componentImpl), mCleanAttempts(0), mCleanFails(0) {}
        ~CleanupTournamentsCommand() override {}

    private:
        CleanupTournamentsCommandStub::Errors execute() override
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_TOURNAMENT_PROVIDER))
            {
                return commandErrorFromBlazeError(ERR_AUTHORIZATION_REQUIRED);
            }
            if (EA_UNLIKELY(mComponent.getExternalTournamentUtil() == nullptr))
            {
                ERR_LOG("[CleanupTournamentsCommand].execute: internal error: External service util not initialized, fail.");
                return ERR_SYSTEM;
            }

            // TBA: possible enhancement: use db or master tracking

            BlazeRpcError err = ERR_OK;
            for (const auto& itr : mRequest.getTournaments())
            {
               if (itr->getTournamentId()[0] != '\0')
               {
                    RemoveTournamentParameters removeParams;
                    mRequest.getUser().copyInto(removeParams.getUser());
                    itr->copyInto(removeParams.getTournamentIdentification());
                    err = mComponent.getExternalTournamentUtil()->removeTournament(removeParams);
                    continue;
                }
                // Else, no tournamentId specified. Remove all tournaments for the title/organizer/club etc

                // Just in case, to throttle, batch these calls
                uint64_t batchSize = 20;
                bool shouldCallAgain = false;
                do
                {
                    err = getAndRemoveTournamentsBatch(batchSize, itr->getTournamentOrganizer(), shouldCallAgain);

                } while (shouldCallAgain && (err == ERR_OK));
            }

            TRACE_LOG("[CleanupTournamentsCommand].execute: cleanup succeeded on(" << (mCleanAttempts - mCleanFails) <<
                "), and failed on(" << mCleanFails << ") tournaments, out of total(" << mCleanAttempts << ") attempts.");

            return commandErrorFromBlazeError(ERR_OK);
        }
    private:
        ArsonTournamentOrganizerSlaveImpl &mComponent;

        uint64_t mCleanAttempts;
        uint64_t mCleanFails;

    private:
        BlazeRpcError getAndRemoveTournamentsBatch(uint64_t batchSize, const char8_t* organizer, bool& shouldCallAgain)
        {
            // first get all tournaments for the title and/or mRequest's organizer
            Blaze::Arson::GetTournamentsParameters getParams;
            Blaze::Arson::GetTournamentsResult getResult;
            mRequest.getUser().copyInto(getParams.getUser());
            mRequest.getMemberFilter().copyInto(getParams.getMemberFilter());
            getParams.setTournamentOrganizer(organizer);
            getParams.setClubIdFilter(mRequest.getClubIdFilter());
            getParams.setStateFilter(mRequest.getStateFilter());
            getParams.setNamePrefixFilter(mRequest.getNamePrefixFilter());
            getParams.setMaxResults(batchSize);

            BlazeRpcError err = mComponent.getExternalTournamentUtil()->getTournaments(getParams, getResult);
            if (err != ERR_OK)
            {
                return commandErrorFromBlazeError(err);
            }
            TRACE_LOG("[CleanupTournamentsCommand].getAndRemoveNextXTournaments: Found (" << getResult.getTournamentDetailsList().size() << ") more tournaments to remove.");

            // to avoid failing on repeated ones

            // now remove
            for (auto itr : getResult.getTournamentDetailsList())
            {
                ++mCleanAttempts;
                Blaze::Arson::RemoveTournamentParameters removeParams;
                mRequest.getUser().copyInto(removeParams.getUser());
                removeParams.setPostRemoveNameSuffix(mRequest.getPostRemoveNameSuffix());
                removeParams.getTournamentIdentification().setTournamentId(itr->getId());
                removeParams.getTournamentIdentification().setTournamentOrganizer(itr->getOrganizerId());

                err = mComponent.getExternalTournamentUtil()->removeTournament(removeParams);
                if (err != ERR_OK)
                {
                    ++mCleanFails;
                    WARN_LOG("[CleanupTournamentsCommand].getAndRemoveNextXTournaments: couldn't remove tournament(" << removeParams.getTournamentIdentification().getTournamentId() << "), check error logs. Skipping..");
                }
            }

            // If potentially more after this batch, caller should continue calling this, as needed
            shouldCallAgain = ((getResult.getTournamentDetailsList().size() >= batchSize) &&
                ((mRequest.getMaxTournamentsToProcess() == 0) || (mCleanAttempts < mRequest.getMaxTournamentsToProcess())));

            return ERR_OK;
        }
    };
    DEFINE_CLEANUPTOURNAMENTS_CREATE()

} // namespace
} // namespace Blaze
