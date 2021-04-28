/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/getxblmultiplayersession_stub.h"
#include "arsonslaveimpl.h"

namespace Blaze
{
namespace Arson
{

//      (side: we have this in ARSON, because blazeserver xblservicesconfigs.rpc doesn't actually have a 'get' external session method/rpc)
class GetXblMultiplayerSessionCommand : public GetXblMultiplayerSessionCommandStub
{
public:
    GetXblMultiplayerSessionCommand(Message* message, GetXblMultiplayerSessionRequest* request, ArsonSlaveImpl* componentImpl)
        : GetXblMultiplayerSessionCommandStub(message, request), mComponent(componentImpl)
    {
    }
    ~GetXblMultiplayerSessionCommand() override {}

private:

    GetXblMultiplayerSessionCommandStub::Errors execute() override
    {
        //fetch the external id for the external session call
        if (mRequest.getUseCurrentUserForExternalSessionCall() && (gCurrentUserSession == nullptr))
        {
            ERR_LOG("[GetXblMultiplayerSessionCommandStub].execute: (Arson) internal mock service error, inputs specifed to use the gCurrentUser, for the external session call,  but gCurrentUser is nullptr.");
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
            ERR_LOG("[GetXblMultiplayerSessionCommandStub].execute: (Arson) internal mock service error, no valid external id specified, default platform(" <<
                ClientPlatformTypeToString(gController->getDefaultClientPlatform()) << "), resolved ident: " << ident);
            return ERR_SYSTEM;
        }

        // depending if its pg or game, set parameters for external session
        Blaze::ExternalUserAuthInfo extToken;
        Blaze::XblContractVersion contractVersion;
        Blaze::GameManager::GetExternalSessionInfoMasterResponse externalSessionInfo;

        if (ERR_OK != mComponent->getXblExternalSessionParams(extToken, externalSessionInfo, contractVersion, mRequest.getEntityId(), ident, mRequest.getEntityType()))
        {
            ERR_LOG("[GetXblMultiplayerSessionCommandStub].execute: internal mock service error, failed to load external session url parameters");
            return ARSON_ERR_INVALID_PARAMETER;
        }
        // use sessionName from Blaze Game, if not in mRequest
        const char8_t* sessionTemplateName = (!mRequest.getSessionTemplateNameAsTdfString().empty() ? mRequest.getSessionTemplateName() : externalSessionInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().getXone().getTemplateName());
        const char8_t* sessionName = (!mRequest.getSessionNameAsTdfString().empty() ? mRequest.getSessionName() : externalSessionInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().getXone().getSessionName());

        // retrieve an external session
        BlazeRpcError result = ERR_OK;
        if (externalSessionInfo.getExternalSessionCreationInfo().getUpdateInfo().getXblLarge() || !mRequest.getOnBehalfOfUserIdentifications().empty())
        {
            UserIdentificationList onBehalfUsers = mRequest.getOnBehalfOfUserIdentifications(); //To populate members from Large MPS need onBehalfUsers headers
            if (onBehalfUsers.empty())
                ident.copyInto(*onBehalfUsers.pull_back());
            result = mComponent->retrieveExternalSessionXbox(sessionTemplateName, sessionName, contractVersion, mResponse, onBehalfUsers);
        }
        else
        {
            result = mComponent->retrieveExternalSessionXbox(sessionTemplateName, sessionName, contractVersion, mResponse, nullptr);
        }

        if (ERR_OK == result)
        {
            TRACE_LOG("[GetXblMultiplayerSessionCommandStub].execute: (Arson) got a game with external session service. Found pre-existing external session named(" << sessionName <<
                "), id " << mResponse.getMultiplayerSessionResponse().getConstants().getCustom().getExternalSessionId() << ", with " << mResponse.getMultiplayerSessionResponse().getMembers().size() <<
                " pre-existing members, Visibility " << mResponse.getMultiplayerSessionResponse().getConstants().getSystem().getVisibility() << ", Closed=" << (mResponse.getMultiplayerSessionResponse().getProperties().getSystem().getClosed()? "true" : "false") << ", JoinRestriction " << mResponse.getMultiplayerSessionResponse().getProperties().getSystem().getJoinRestriction() << ".");
        }
        else
        {
            ERR_LOG("[GetXblMultiplayerSessionCommandStub].execute: (Arson) Got error code " << ErrorHelp::getErrorName(result));
            return (commandErrorFromBlazeError(result) != GetXblMultiplayerSessionCommandStub::ERR_SYSTEM ? commandErrorFromBlazeError(result) : ARSON_ERR_EXTERNAL_SESSION_FAILED);
        }

        // boiler plate checks to ensure external session, and GameManager are in sync etc.
        Blaze::GameManager::GameId expectedGameId = ((mRequest.getEntityId() != Blaze::GameManager::INVALID_GAME_ID) ? mRequest.getEntityId() : mResponse.getMultiplayerSessionResponse().getConstants().getCustom().getExternalSessionId());
        if ((mRequest.getEntityType() == Blaze::GameManager::ENTITY_TYPE_GAME) || (mRequest.getEntityType() == Blaze::GameManager::ENTITY_TYPE_GAME_GROUP))
        {
            //must have a game id cached on the external session. use itUser to fetch and validate the actual blaze game.
            Blaze::GameManager::ReplicatedGameData gameData;
            BlazeRpcError getErr = mComponent->getGameDataFromSearchSlaves(expectedGameId, gameData);
            if (getErr != ERR_OK)
            {
                ERR_LOG("[GetXblMultiplayerSessionCommandStub].execute: (Arson) Fetch of GM game(" << expectedGameId << ") data got error code " << ErrorHelp::getErrorName(getErr));
                return ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC;
            }
        }

        return ERR_OK;
    }

private:
    ArsonSlaveImpl* mComponent;
};

DEFINE_GETXBLMULTIPLAYERSESSION_CREATE()

} // Arson
} // Blaze
