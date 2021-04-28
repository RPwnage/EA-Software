/*! ************************************************************************************************/
/*!
    \file updateprimaryexternalsessionforuser_slavecommand.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/updateprimaryexternalsessionforuser_stub.h"
#include "gamemanagerslaveimpl.h"
#include "component/gamemanager/externalsessions/externalsessionutil.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h" // for oldToNewExternalSessionIdentificationParams() in execute()
#include "gamemanager/tdf/externalsessiontypes_server.h" // for UpdatePrimaryExternalSessionForUserParameter

namespace Blaze
{
namespace GameManager
{
    /*! ************************************************************************************************/
    /*! \brief This command is called from client when its local users join or remove from games, to
        update user's game presence.
    ***************************************************************************************************/
    class UpdatePrimaryExternalSessionForUserCommand : public UpdatePrimaryExternalSessionForUserCommandStub
    {
    public:

        UpdatePrimaryExternalSessionForUserCommand(Message* message, UpdateExternalSessionPresenceForUserRequest *request, GameManagerSlaveImpl* componentImpl)
            : UpdatePrimaryExternalSessionForUserCommandStub(message, request), mComponent(*componentImpl)
        {
        }
        ~UpdatePrimaryExternalSessionForUserCommand() override
        {
        }

    private:

        UpdatePrimaryExternalSessionForUserCommandStub::Errors execute() override
        {
            convertToPlatformInfo(mRequest.getUserIdentification());

            // transfer deprecated GameActivity parameters to the new ones for mChangedGame
            oldToNewGameActivityParams(mRequest.getChangedGame());
            // transfer deprecated GameActivity parameters to the new ones for mCurrentGames
            for (GameActivityList::iterator itr = mRequest.getCurrentGames().begin(), end = mRequest.getCurrentGames().end(); itr != end; ++itr)
            {
                oldToNewGameActivityParams(**itr);
            }
            // transfer deprecated ChangedGameId as needed
            if (mRequest.getChangedGame().getGameId() == INVALID_GAME_ID)
            {
                mRequest.getChangedGame().setGameId(mRequest.getChangedGameId());
            }
            // transfer deprecated PsnPushContextId as needed
            if (mRequest.getPsnPushContextIdAsTdfString().empty() && (getPeerInfo() != nullptr))
            {
                mRequest.setPsnPushContextId(getPeerInfo()->getClientInfo()->getPsnPushContextId());
            }


            ExternalId extId = getExternalIdFromPlatformInfo(mRequest.getUserIdentification().getPlatformInfo());
            TRACE_LOG("[UpdatePrimaryExternalSessionForUserCommand].execute: updating for user(" << mRequest.getUserIdentification().getName() << ", id(" << mRequest.getUserIdentification().getBlazeId() << "), extId(" << extId << ")) on " << UpdateExternalSessionPresenceForUserReasonToString(mRequest.getChange()) << " for game(" << mRequest.getChangedGame().getGameId() << ").");

            UpdateExternalSessionPresenceForUserParameters mRequestParameters;
            mRequest.getUserIdentification().copyInto(mRequestParameters.getUserIdentification());
            mRequest.getCurrentGames().copyInto(mRequestParameters.getCurrentGames());
            mRequest.getChangedGame().copyInto(mRequestParameters.getChangedGame());
            mRequestParameters.setOldPrimaryGameId(mRequest.getOldPrimaryGameId());
            mRequestParameters.setChange(mRequest.getChange());
            mRequestParameters.setRemoveReason(mRequest.getRemoveReason());
            mRequestParameters.setPsnPushContextId(mRequest.getPsnPushContextId());

            if (getPeerInfo() != nullptr)
            {
                mRequestParameters.setTitleId(getPeerInfo()->getClientInfo()->getTitleId());
            }

            // Side: Blaze Server slave processes each RPC by same client and user index serially, ensuring order
            BlazeRpcError sessErr = mComponent.getExternalSessionUtilManager().updatePresenceForUser(mRequestParameters, mResponse, mErrorResponse);
            if (sessErr != ERR_OK)
                mComponent.incExternalSessionFailureNonImpacting(mRequest.getUserIdentification().getPlatformInfo().getClientPlatform());

            return commandErrorFromBlazeError(convertExternalServiceErrorToGameManagerError(sessErr));
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    DEFINE_UPDATEPRIMARYEXTERNALSESSIONFORUSER_CREATE()

} // namespace GameManager
} // namespace Blaze
