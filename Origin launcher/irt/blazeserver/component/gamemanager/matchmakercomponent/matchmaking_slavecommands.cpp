/*! ************************************************************************************************/
/*!
    \file startmatchmaking_slavecommand.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/connectutil.h"
#include "framework/usersessions/usersessionmanager.h"
#include "gamemanager/rpc/matchmakerslave/startmatchmakinginternal_stub.h"
#include "gamemanager/rpc/matchmakerslave/cancelmatchmakinginternal_stub.h"
#include "gamemanager/matchmakercomponent/matchmakerslaveimpl.h"
#include "gamemanager/gamemanagerhelperutils.h" // for getHostSessionInfo
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"

namespace Blaze
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*! \brief Start a matchmaking session; on success we register for async notifications from the session.
    ***************************************************************************************************/
    class StartMatchmakingInternalCommand : public StartMatchmakingInternalCommandStub
    {
    public:
        StartMatchmakingInternalCommand(Message* message, Blaze::GameManager::StartMatchmakingInternalRequest* request, MatchmakerSlaveImpl* componentImpl)
            :   StartMatchmakingInternalCommandStub(message, request), mComponent(*componentImpl)
        {
        }

        ~StartMatchmakingInternalCommand() override {}

    private:
        
        StartMatchmakingInternalCommandStub::Errors execute() override
        {
            auto sessionId = mRequest.getMatchmakingSessionId();

            auto* ownerUserSessionInfo = Blaze::GameManager::getHostSessionInfo(mRequest.getUsersInfo());
            if (ownerUserSessionInfo != nullptr && !gUserSessionManager->getSessionExists(ownerUserSessionInfo->getSessionId()))
            {
                // Bail out early if the user session owning the matchmaking session is no longer available.
                TRACE_LOG("[StartMatchmakingInternalCommand].execute: Matchmaking session owner user session(" << ownerUserSessionInfo->getSessionId() << ") not found");
                return StartMatchmakingInternalCommandStub::MATCHMAKER_ERR_MATCHMAKING_USERSESSION_NOT_FOUND;
            }

            Blaze::ExternalSessionIdentification externalSessionIdentification;
            BlazeRpcError err = mComponent.getMatchmaker()->startSession(mRequest, externalSessionIdentification, mErrorResponse);

            if (err == Blaze::ERR_OK) 
            {
                mResponse.getStartMatchmakingResponse().setSessionId(sessionId); // send right back

                auto mmSession = mComponent.getMatchmaker()->findSession(sessionId);
                if (mmSession != nullptr)
                {
                    mResponse.setEstimatedTimeToMatch(mmSession->getEstimatedTimeToMatch());
                }

                // before sending the result down, fill out the instance status
                mComponent.getMatchmaker()->filloutMatchmakingStatus(mResponse.getMatchmakingStatus());
                
                // Fill the external session data if applicable. Currently, only used on xone. However, note that the client makes no use of this data currently. Just that MS
                // forces us to have an external session for mm session. 
                // This is Xbox MPSD sessions specific. Not expected used on other platforms
                if (!externalSessionIdentification.getXone().getCorrelationIdAsTdfString().empty()) // if the session had xone users
                {
                    mResponse.getStartMatchmakingResponse().setExternalSessionTemplateName(externalSessionIdentification.getXone().getTemplateName());
                    mResponse.getStartMatchmakingResponse().setExternalSessionName(externalSessionIdentification.getXone().getSessionName());
                    mResponse.getStartMatchmakingResponse().setExternalSessionCorrelationId(externalSessionIdentification.getXone().getCorrelationId());

                    mComponent.getExternalSessionScid(mResponse.getStartMatchmakingResponse().getScidAsTdfString());
                }
            }

            return commandErrorFromBlazeError(err);
        }

    private:
        MatchmakerSlaveImpl &mComponent;

    };

    //! \brief static creation factory method for command (from stub)
    DEFINE_STARTMATCHMAKINGINTERNAL_CREATE()


    /*! ************************************************************************************************/
    /*! \brief Cancel a matchmaking session;
    ***************************************************************************************************/
    class CancelMatchmakingInternalCommand : public CancelMatchmakingInternalCommandStub
    {
    public:
        CancelMatchmakingInternalCommand(Message* message, Blaze::GameManager::CancelMatchmakingInternalRequest* request, MatchmakerSlaveImpl* componentImpl)
            :   CancelMatchmakingInternalCommandStub(message, request), mComponent(*componentImpl)
        {
        }

        ~CancelMatchmakingInternalCommand() override {}

    private:

        CancelMatchmakingInternalCommandStub::Errors execute() override
        {
            BlazeRpcError err = mComponent.getMatchmaker()->cancelSession(mRequest.getMatchmakingSessionId(), mRequest.getOwnerUserSessionId());
            return commandErrorFromBlazeError(err);
        }

    private:
        MatchmakerSlaveImpl &mComponent;
    };

    //! \brief static creation factory method for command (from stub)
    DEFINE_CANCELMATCHMAKINGINTERNAL_CREATE()

} // namespace GameManager
} // namespace Blaze
