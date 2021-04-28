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
            ERR_LOG(logPrefix() << ".execute: internal mock service error, inputs specifed to use the gCurrentUser, for the external session call,  but gCurrentUser is nullptr.");
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
            ERR_LOG(logPrefix() << ".execute: internal mock service error, no valid external id specified");
            return ERR_SYSTEM;
        }

        //fetch the external session call params
        Blaze::ExternalUserAuthInfo myExtToken;
        Blaze::XblContractVersion contractVersion;
        Blaze::GameManager::GetExternalSessionInfoMasterResponse externalSessionInfo;

        if (ERR_OK != mComponent->getXblExternalSessionParams(myExtToken, externalSessionInfo, contractVersion, mRequest.getEntityId(), ident, mRequest.getEntityType()))
        {
            ERR_LOG(logPrefix() << ".execute: internal mock service error, failed to load external session url parameters");
            mResponse.setBlazeErrorCode(ARSON_ERR_INVALID_PARAMETER);
            return ARSON_ERR_INVALID_PARAMETER;
        }
        // use sessionName from Blaze Game, if not in mRequest
        const char8_t* sessionTemplateName = (!mRequest.getSessionTemplateNameAsTdfString().empty() ? mRequest.getSessionTemplateName() : externalSessionInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().getXone().getTemplateName());
        const char8_t* sessionName = (!mRequest.getSessionNameAsTdfString().empty() ? mRequest.getSessionName() : externalSessionInfo.getExternalSessionCreationInfo().getUpdateInfo().getSessionIdentification().getXone().getSessionName());

        // retrieve an external session
        if (externalSessionInfo.getExternalSessionCreationInfo().getUpdateInfo().getXblLarge())
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
            TRACE_LOG(logPrefix() << ".execute: got a game with external session service. Found pre-existing external session named " << sessionName << ", with " << mResponse.getMultiplayerSessionResponse().getMembers().size() << " pre-existing members. Proceeding to clear them from game.");
            if ((mRequest.getEntityType() == Blaze::GameManager::ENTITY_TYPE_GAME) || (mRequest.getEntityType() == Blaze::GameManager::ENTITY_TYPE_GAME_GROUP))
            {
                //must have a game id cached on the external session. use it to fetch and validate the actual blaze game.
                Blaze::GameManager::ReplicatedGameData gameData;
                BlazeRpcError getErr = mComponent->getGameDataFromSearchSlaves(mRequest.getEntityId(), gameData);
                if (getErr != ERR_OK)
                {
                    ERR_LOG(logPrefix() << ".execute: (Arson) Fetch of GM game data got error code " << ErrorHelp::getErrorName(getErr));
                    return ARSON_ERR_EXTERNAL_SESSION_OUT_OF_SYNC;
                }
            }

            // delete them from the external session directory
            Arson::ArsonMembers::const_iterator itr = mResponse.getMultiplayerSessionResponse().getMembers().begin();
            Arson::ArsonMembers::const_iterator end = mResponse.getMultiplayerSessionResponse().getMembers().end();
            for (; itr != end; ++itr)
            {
                Blaze::ExternalXblAccountId xblId = (*itr).second->getConstants().getSystem().getXuid();
                doMpsLeave(xblId, sessionTemplateName, sessionName, xone);
                doMpsLeave(xblId, sessionTemplateName, sessionName, xbsx); //(in case one or other xbox platforms utils isn't enabled)
            }
        }
        else
        {
            WARN_LOG(logPrefix() << ".execute: did not get a game MPS named(" << sessionName << "), with external session service.");
            result = ERR_OK; // assume external session may have been already deleted (given other validations before using the getexternalmultiplayersession command succeeded).
        }
        
        return commandErrorFromBlazeError(result);
    }

private:
    ArsonSlaveImpl* mComponent;
    const char8_t* logPrefix() const { return (mLogPrefix.empty() ? mLogPrefix.sprintf("[GetXblActivityForUserCommand]").c_str() : mLogPrefix.c_str()); }
    mutable eastl::string mLogPrefix;

    //\param[in] clientPlatform The ExternalSessionUtil platform to use to cleanup with. If both platfortms enabled you may need to try both to ensure all MPSs cleaned up.
    Blaze::BlazeRpcError doMpsLeave(ExternalXblAccountId xblId, const char8_t* sessionTemplateName, const char8_t* sessionName, ClientPlatformType clientPlatform)
    {
        // on caps, for our tests, if player has an external id warn here
        if (xblId != INVALID_XBL_ACCOUNT_ID)
        {
            ERR_LOG(logPrefix() << ".doMpsLeave: trying to leave game MPS(" << sessionName << "), player with xuid " << xblId << " has invalid xuid. Skipping");
            return ERR_SYSTEM;
        }
        // leave:
        Blaze::LeaveGroupExternalSessionParameters params;
        ExternalUserLeaveInfo& userLeaveInfo = *params.getExternalUserLeaveInfos().pull_back();
        convertToPlatformInfo(userLeaveInfo.getUserIdentification().getPlatformInfo(), xblId, nullptr, INVALID_ACCOUNT_ID, clientPlatform);
        params.getSessionIdentification().getXone().setTemplateName(sessionTemplateName);
        params.getSessionIdentification().getXone().setSessionName(sessionName);
        auto* extSessUtil = (mComponent ? mComponent->getExternalSessionServiceXbox(userLeaveInfo.getUserIdentification().getPlatformInfo().getClientPlatform()) : nullptr);
        if (!extSessUtil)
        {
            eastl::string logBuf;
            ERR_LOG(logPrefix() << ".doMpsLeave: not extsessutil for platform(" << ClientPlatformTypeToString(userLeaveInfo.getUserIdentification().getPlatformInfo().getClientPlatform()) << "), not using that platform's util to try leaving game MPS(" << sessionName << "), for player with xuid " << xblId << ". Skipping. ");
            return USER_ERR_DISALLOWED_PLATFORM; //assume unsupported for this platform
        }
        BlazeRpcError err = extSessUtil->leave(params);
        if (ERR_OK != err)
        {
            ERR_LOG(logPrefix() << ".doMpsLeave: failed to remove from game MPS(" << sessionName << "), player with xuid " << xblId << ". Skipping.");
        }
        else
        {
            TRACE_LOG(logPrefix() << ".doMpsLeave: removed from game MPS(" << sessionName << "), player with xuid " << xblId << ".");
        }
        return err;
    }

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
