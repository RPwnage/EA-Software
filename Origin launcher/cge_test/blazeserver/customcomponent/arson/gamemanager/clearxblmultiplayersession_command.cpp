/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/clearxblmultiplayersession_stub.h"
#include "arsonslaveimpl.h"

#include "gamemanager/gamemanagerslaveimpl.h" // for GameManagerSlaveImpl in validateBlazeEntityForExternalSessionRemoved
#include "gamemanager/tdf/externalsessiontypes_server.h" //for LeaveGroupExternalSessionParameters in execute()

namespace Blaze
{
namespace Arson
{

class ClearXblMultiplayerSessionCommand : public ClearXblMultiplayerSessionCommandStub
{
public:
    ClearXblMultiplayerSessionCommand(Message* message, GetXblMultiplayerSessionRequest* request, ArsonSlaveImpl* componentImpl)
        : ClearXblMultiplayerSessionCommandStub(message, request), mComponent(componentImpl)
    {
    }
    ~ClearXblMultiplayerSessionCommand() override {}

private:

    ClearXblMultiplayerSessionCommandStub::Errors execute() override
    {
        BlazeRpcError result = validateBlazeEntityForExternalSessionRemoved(mRequest.getEntityType(), mRequest.getEntityId());
        if (result != ERR_OK)
        {
            mResponse.setBlazeErrorCode(result);
            return commandErrorFromBlazeError(result); // Err logged.
        }

        //fetch the external id for the external session call
        if (mRequest.getUseCurrentUserForExternalSessionCall() && (gCurrentUserSession == nullptr))
        {
            ERR_LOG("[ClearXblMultiplayerSessionCommandStub].execute: internal mock service error, inputs specifed to use the gCurrentUser, for the external session call,  but gCurrentUser is nullptr.");
            return ARSON_ERR_INVALID_PARAMETER;
        }
        Blaze::UserIdentification ident;
        mRequest.getExternalIdent().copyInto(ident);
        if (mRequest.getUseCurrentUserForExternalSessionCall())
        {
            UserInfo::filloutUserIdentification(gCurrentLocalUserSession->getUserInfo(), ident);
        }

        ExternalId extId = mComponent->getExternalIdFromPlatformInfo(ident.getPlatformInfo());
        if (extId == INVALID_EXTERNAL_ID)
        {
            ERR_LOG("[ClearXblMultiplayerSessionCommandStub].execute: internal mock service error, no valid external id specified");
            return ERR_SYSTEM;
        }

        //fetch the external session call params
        Blaze::ExternalUserAuthInfo myExtToken;
        Blaze::XblContractVersion contractVersion;

        if (ERR_OK != mComponent->getXblExternalSessionParams(mRequest.getEntityType(), mRequest.getEntityId(), ident, myExtToken, contractVersion))
        {
            ERR_LOG("[ClearXblMultiplayerSessionCommandStub].execute: internal mock service error, failed to load external session url parameters");
            mResponse.setBlazeErrorCode(ARSON_ERR_INVALID_PARAMETER);
            return ARSON_ERR_INVALID_PARAMETER;
        }

        // retrieve in external session
        result = mComponent->retrieveExternalSessionXbox(mRequest.getScid(), mRequest.getSessionTemplateName(), mRequest.getSessionName(), contractVersion, mResponse);
        if (ERR_OK == result)
        {
            TRACE_LOG("[ClearXblMultiplayerSessionCommandStub].execute: got a game with external session service. Found pre-existing external session named " << mRequest.getSessionName() << ", with " << mResponse.getMultiplayerSessionResponse().getMembers().size() << " pre-existing members. Proceeding to clear them from game.");
            if ((mRequest.getEntityType() == Blaze::GameManager::ENTITY_TYPE_GAME) || (mRequest.getEntityType() == Blaze::GameManager::ENTITY_TYPE_GAME_GROUP))
            {
                //must have a game id cached on the external session. use it to fetch and validate the actual blaze game.
                Blaze::GameManager::ReplicatedGameData gameData;
                BlazeRpcError getErr = mComponent->getGameDataFromSearchSlaves(mRequest.getEntityId(), gameData);
                if (getErr != ERR_OK)
                {
                    ERR_LOG("[ClearXblMultiplayerSessionCommandStub].execute: (Arson) Fetch of GM game data got error code " << ErrorHelp::getErrorName(getErr));
                    return ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC;
                }
            }

            // delete them from the external session directory
            Arson::ArsonMembers::const_iterator itr = mResponse.getMultiplayerSessionResponse().getMembers().begin();
            Arson::ArsonMembers::const_iterator end = mResponse.getMultiplayerSessionResponse().getMembers().end();
            for (; itr != end; ++itr)
            {
                Blaze::ExternalXblAccountId xblId = (*itr).second->getConstants().getSystem().getXuid();
                // on caps, for our tests, if player has an external id warn here
                if (xblId != INVALID_XBL_ACCOUNT_ID)
                {
                    ERR_LOG("[ClearXblMultiplayerSessionCommandStub].execute: trying to leave game " << mRequest.getSessionName() << ", player with xuid " << xblId << " has invalid xuid. Skipping");
                    continue;
                }

                Blaze::LeaveGroupExternalSessionParameters params;
                ExternalUserLeaveInfo& userLeaveInfo = *params.getExternalUserLeaveInfos().pull_back();
                convertToPlatformInfo(userLeaveInfo.getUserIdentification().getPlatformInfo(), xblId, nullptr, INVALID_ACCOUNT_ID, xone);
                params.getSessionIdentification().getXone().setTemplateName(mRequest.getSessionTemplateName());
                params.getSessionIdentification().getXone().setSessionName(mRequest.getSessionName());
                if (ERR_OK != mComponent->getExternalSessionServiceXbox(userLeaveInfo.getUserIdentification().getPlatformInfo().getClientPlatform())->leave(params))
                {
                    ERR_LOG("[ClearXblMultiplayerSessionCommandStub].execute: failed to remove from game " << mRequest.getSessionName() << ", player with xuid " << xblId << ". Skipping");
                }
                else
                {
                    TRACE_LOG("[ClearXblMultiplayerSessionCommandStub].execute: removed from game " << mRequest.getSessionName() << ", player with xuid " << xblId << ".");
                }
            }
        }
        else
        {
            WARN_LOG("[ClearXblMultiplayerSessionCommandStub].execute: did not get a game named " << mRequest.getSessionName() << ", with external session service.");
            result = ERR_OK; // assume external session may have been already deleted (given other validations before using the getexternalmultiplayersession command succeeded).
        }
        
        return commandErrorFromBlazeError(result);
    }

private:
    ArsonSlaveImpl* mComponent;

    //ARSON_TODO: change entityid to union, and incorp mm session id.
    //Validate the game/mmsession does not exist in blaze, before attempting to remove from the external session directory.
    Blaze::BlazeRpcError validateBlazeEntityForExternalSessionRemoved(EA::TDF::ObjectType entityType, Blaze::EntityId gameId)
    {
        if ((entityType == Blaze::GameManager::ENTITY_TYPE_GAME) || (entityType == Blaze::GameManager::ENTITY_TYPE_GAME_GROUP))
        {
            Blaze::GameManager::GameManagerSlaveImpl* gameMgr = (Blaze::GameManager::GameManagerSlaveImpl*)gController->getComponent(Blaze::GameManager::GameManagerSlave::COMPONENT_ID);
            if (gameMgr == nullptr)
                return ERR_OK;

            Blaze::GameManager::GetGameDataFromIdRequest request;
            Blaze::GameManager::GameBrowserDataList response;
            request.getGameIds().push_back(gameId);
            gameMgr->getGameDataFromId(request, response);
            if (response.getGameData().empty())
                return ERR_OK;
        }
        else
        {
            ERR_LOG("[ArsonSlaveImpl].validateBlazeEntityForExternalSessionRemoved: Unhandled entity type.");
            return ERR_SYSTEM;
        }

        ERR_LOG("[ArsonSlaveImpl].validateBlazeEntityForExternalSessionRemoved: found non-removed Blaze object " << entityType.toString().c_str() << ", id " << gameId);
        return Blaze::ARSON_ERR_ARSON_UNEXPECTED_EXISTING_ENTITY;
    }
};

DEFINE_CLEARXBLMULTIPLAYERSESSION_CREATE()

} // Arson
} // Blaze
