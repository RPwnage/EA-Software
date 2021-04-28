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
        ClientPlatformType platform = (mRequest.getExternalIdent().getPlatformInfo().getClientPlatform() == INVALID ? xone : mRequest.getExternalIdent().getPlatformInfo().getClientPlatform());
        if ((mRequest.getExternalIdent().getPlatformInfo().getExternalIds().getXblAccountId() == INVALID_XBL_ACCOUNT_ID)) //for back compat:
            convertToPlatformInfo(mRequest.getExternalIdent().getPlatformInfo(), mRequest.getExternalIdent().getExternalId(), nullptr, mRequest.getExternalIdent().getAccountId(), platform);

        eastl::string xuidBuf;
        UserIdentification ident;
        mRequest.getExternalIdent().copyInto(ident);

        XBLServices::HandlesGetActivityResult currentPrimary;
        BlazeRpcError restErr = mComponent->retrieveCurrentPrimarySessionXbox(ident, currentPrimary);
        if (restErr != ERR_OK)
        {
            ERR_LOG(logPrefix() << ".execute:  (Arson): failed XBL REST call on error " << ErrorHelp::getErrorName(restErr));
            return commandErrorFromBlazeError(restErr);
        }

        if (currentPrimary.getId()[0] == '\0')
        {
            TRACE_LOG(logPrefix() << ".execute:  (Arson): XBL REST call returned empty results, no primary external session assigned to external id " << mRequest.getExternalIdent().getPlatformInfo().getExternalIds().getXblAccountId() << ".");
            return ERR_OK;
        }
        //store the found handle
        mResponse.setHandleId(currentPrimary.getId());

        auto* extSessUtil = mComponent->getExternalSessionServiceXbox(ident.getPlatformInfo().getClientPlatform());
        if (extSessUtil == nullptr)
        {
            eastl::string logBuf;
            ERR_LOG(logPrefix() << ".execute: ExternalSession Util null, check test setup code. " << (mComponent ? mComponent->toCurrPlatformsHavingExtSessConfigLogStr(logBuf, nullptr) : "<ArsonComponent null>"));
            return ERR_SYSTEM;
        }

        //store the entire MPS for test also
        UserIdentificationList onBehalfOfUserList;
        ident.copyInto(*onBehalfOfUserList.pull_back());

        Arson::GetXblMultiplayerSessionResponse mps;
        BlazeRpcError result = mComponent->retrieveExternalSessionXbox(
            currentPrimary.getSessionRef().getTemplateName(),
            currentPrimary.getSessionRef().getName(),
            extSessUtil->getConfig().getContractVersion(), mps, onBehalfOfUserList);
        if (ERR_OK != result)
        {
            ERR_LOG(logPrefix() << ".execute:  (Arson): Got error code " << ErrorHelp::getErrorName(result));
            return ARSON_ERR_EXTERNAL_SESSION_FAILED;
        }

        mps.getMultiplayerSessionResponse().copyInto(mResponse.getMultiplayerSessionResponse());
        TRACE_LOG(logPrefix() << ".execute:  (Arson): XBL REST call got primary external session for external id " << mRequest.getExternalIdent().getPlatformInfo().getExternalIds().getXblAccountId() << ": Scid: " <<
            currentPrimary.getSessionRef().getScid() << ", template: " <<
            currentPrimary.getSessionRef().getTemplateName() << ", sessionName: " <<
            currentPrimary.getSessionRef().getName() << ", externalSessionId: " <<
            mps.getMultiplayerSessionResponse().getConstants().getCustom().getExternalSessionId() << ", externalSessionType: " <<
            Blaze::ExternalSessionTypeToString((Blaze::ExternalSessionType)mps.getMultiplayerSessionResponse().getConstants().getCustom().getExternalSessionType()) << ".");
        return ERR_OK;
    }

private:
    ArsonSlaveImpl* mComponent;
    const char8_t* logPrefix() const { return (mLogPrefix.empty() ? mLogPrefix.sprintf("[GetXblActivityForUserCommand]").c_str() : mLogPrefix.c_str()); }
    mutable eastl::string mLogPrefix;
};

DEFINE_GETXBLACTIVITYFORUSER_CREATE()

} // Arson
} // Blaze
