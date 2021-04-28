/*! ************************************************************************************************/
/*!
    \file updateexternalsessionimage_slavecommand.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rpc/gamemanagerslave/updateexternalsessionimage_stub.h"
#include "gamemanagerslaveimpl.h"
#include "gamemanager/externalsessions/externalsessionutil.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"

#include "framework/usersessions/usersessionmanager.h"

namespace Blaze
{
namespace GameManager
{
    /*! ************************************************************************************************/
    /*! \brief Update a game's external session image.
    ***************************************************************************************************/
    class UpdateExternalSessionImageCommand : public UpdateExternalSessionImageCommandStub
    {
    public:
        UpdateExternalSessionImageCommand(Message* message, UpdateExternalSessionImageRequest *request, GameManagerSlaveImpl* componentImpl)
            : UpdateExternalSessionImageCommandStub(message, request), mComponent(*componentImpl)
        {
        }
        ~UpdateExternalSessionImageCommand() override
        {
        }

    private:

        // Note: for scalability we do these calls on slave. Centralization on master isn't needed, as these requests should be coming in serially via an admin
        UpdateExternalSessionImageCommandStub::Errors execute() override
        {
            // Get game's info, and verify caller is an admin
            GetExternalSessionInfoMasterRequest getReq;
            GetExternalSessionInfoMasterResponse getRsp;
            getReq.setGameId(mRequest.getGameId());
            UserInfo::filloutUserIdentification(gCurrentLocalUserSession->getUserInfo(), getReq.getCaller());

            BlazeRpcError err = mComponent.getMaster()->getExternalSessionInfoMaster(getReq, getRsp);
            if (err != ERR_OK)
            {
                if (err != GAMEMANAGER_ERR_INVALID_GAME_ID)
                    WARN_LOG("[UpdateExternalSessionImageCommand].execute: Failed to retrieve info for game(" << mRequest.getGameId() << "), on error(" << ErrorHelp::getErrorName(err) << "=" << ErrorHelp::getErrorDescription(err) << "). Not updating image.");
                return commandErrorFromBlazeError(err);
            }
            else if (!getRsp.getIsGameAdmin())
            {
                TRACE_LOG("[UpdateExternalSessionImageCommand].execute: user(" << getReq.getCaller().getBlazeId() << ") is not an admin for game(" << mRequest.getGameId() << "). No op.");
                return GAMEMANAGER_ERR_PERMISSION_DENIED;
            }
            else if (!isExtSessIdentSet(getRsp.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification()))
            {
                TRACE_LOG("[UpdateExternalSessionImageCommand].execute: Game(" << mRequest.getGameId() << ") external session(" << toLogStr(getRsp.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification()) << ") does not currently exist or is not advertised by any users. No op.");
                return GAMEMANAGER_ERR_INVALID_GAME_STATE_ACTION;
            }

            // send the request
            err = mComponent.getExternalSessionUtilManager().handleUpdateExternalSessionImage(mRequest, getRsp, mComponent, mErrorResponse);

            return commandErrorFromBlazeError(convertExternalServiceErrorToGameManagerError(err));
        }

    private:
        GameManagerSlaveImpl &mComponent;
    };

    DEFINE_UPDATEEXTERNALSESSIONIMAGE_CREATE()

} // namespace GameManager
} // namespace Blaze
