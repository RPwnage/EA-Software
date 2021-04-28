/*************************************************************************************************/
/*!
    \file
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "arson/rpc/arsonslave/getxblactivityforuser_stub.h"
#include "arsonslaveimpl.h"
#include "xblclientsessiondirectory/tdf/xblclientsessiondirectory.h" // for HandlesGetActivityResult in execute()
#include "gamemanager/tdf/gamemanagerconfig_server.h"

namespace Blaze
{
namespace Arson
{

//      (side: we have this in ARSON, because blazeserver xblservicesconfigs.rpc doesn't actually have a 'get' external session method/rpc)
class GetXblActivityForUserCommand : public GetXblActivityForUserCommandStub
{
public:
    GetXblActivityForUserCommand(Message* message, ArsonGetXblActivityForUserRequest* request, ArsonSlaveImpl* componentImpl)
        : GetXblActivityForUserCommandStub(message, request), mComponent(componentImpl)
    {
    }
    ~GetXblActivityForUserCommand() override {}

private:

    GetXblActivityForUserCommandStub::Errors execute() override
    {
        convertToPlatformInfo(mRequest.getExternalIdent().getPlatformInfo(), mRequest.getExternalIdent().getExternalId(), nullptr, mRequest.getExternalIdent().getAccountId(), xone);

        eastl::string xuidBuf;
        UserIdentification ident;
        mRequest.getExternalIdent().copyInto(ident);

        Blaze::GameManager::GameManagerServerConfig gameManagerServerConfig;
        BlazeRpcError rc = gController->getConfigTdf("gamemanager", false, gameManagerServerConfig);
        if (rc != ERR_OK)
        {
            ERR_LOG("[ArsonSlaveImpl].GetXblActivityForUserCommand (Arson): Failure fetching config for game manager, error=" << ErrorHelp::getErrorName(rc));
            return ERR_SYSTEM;
        }

        XBLServices::HandlesGetActivityResult currentPrimary;
        BlazeRpcError restErr = mComponent->retrieveCurrentPrimarySessionXbox(ident, currentPrimary);
        if (restErr != ERR_OK)
        {
            ERR_LOG("[GetXblActivityForUserCommand] (Arson): failed XBL REST call on error " << ErrorHelp::getErrorName(restErr));
            return commandErrorFromBlazeError(restErr);
        }

        if (currentPrimary.getId()[0] == '\0')
        {
            TRACE_LOG("[GetXblActivityForUserCommand] (Arson): XBL REST call returned empty results, no primary external session assigned to external id " << mRequest.getExternalIdent().getPlatformInfo().getExternalIds().getXblAccountId() << ".");
            return ERR_OK;
        }
        //store the found handle
        mResponse.setHandleId(currentPrimary.getId());

        //store the entire MPS for test also
        Arson::GetXblMultiplayerSessionResponse mps;
        BlazeRpcError result = mComponent->retrieveExternalSessionXbox(
            currentPrimary.getSessionRef().getScid(),
            currentPrimary.getSessionRef().getTemplateName(),
            currentPrimary.getSessionRef().getName(),
            mComponent->getExternalSessionServiceXbox(ident.getPlatformInfo().getClientPlatform())->getConfig().getContractVersion(), mps);
        if (ERR_OK != result)
        {
            ERR_LOG("[GetXblActivityForUserCommand] (Arson): Got error code " << ErrorHelp::getErrorName(result));
            return ARSON_ERR_EXTERNAL_SESSION_FAILED;
        }

        mps.getMultiplayerSessionResponse().copyInto(mResponse.getMultiplayerSessionResponse());
        TRACE_LOG("[GetXblActivityForUserCommand] (Arson): XBL REST call got primary external session for external id " << mRequest.getExternalIdent().getPlatformInfo().getExternalIds().getXblAccountId() << ": Scid: " <<
            currentPrimary.getSessionRef().getScid() << ", template: " <<
            currentPrimary.getSessionRef().getTemplateName() << ", sessionName: " <<
            currentPrimary.getSessionRef().getName() << ", externalSessionId: " <<
            mps.getMultiplayerSessionResponse().getConstants().getCustom().getExternalSessionId() << ", externalSessionType: " <<
            Blaze::ExternalSessionTypeToString((Blaze::ExternalSessionType)mps.getMultiplayerSessionResponse().getConstants().getCustom().getExternalSessionType()) << ".");
        return ERR_OK;
    }

private:
    ArsonSlaveImpl* mComponent;
};

DEFINE_GETXBLACTIVITYFORUSER_CREATE()

} // Arson
} // Blaze
