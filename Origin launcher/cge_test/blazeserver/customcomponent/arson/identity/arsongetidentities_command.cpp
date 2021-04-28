/*************************************************************************************************/
/*!
    \file   arsongetidentities_command.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"

// arson includes
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/arsongetidentities_stub.h"
#include "framework/rpc/identityslave.h"
#include "arson/identity/tdf/arsonidentitytypes.h"

namespace Blaze
{
namespace Arson
{

/*! ************************************************************************************************/
/*! \brief Calls the Blaze Server side only RPC, allowing ARSON client to call it using a typed interface.
*************************************************************************************************/
class ArsonGetIdentitiesCommand : public ArsonGetIdentitiesCommandStub
{
public:
    ArsonGetIdentitiesCommand(Message* message,
        Blaze::Arson::IdentitiesRequest* request, ArsonSlaveImpl* componentImpl)
        : ArsonGetIdentitiesCommandStub(message, request),
        mComponent(componentImpl)
    {
    }
    ~ArsonGetIdentitiesCommand() override { }

private:
    ArsonSlaveImpl* mComponent;

private:

    ArsonGetIdentitiesCommandStub::Errors execute() override
    {
        BlazeRpcError err = Blaze::ERR_OK;
        Blaze::IdentitySlave* identityComp = static_cast<Blaze::IdentitySlave*>(gController->getComponent(Blaze::IdentitySlave::COMPONENT_ID, false, true, &err));
        if (identityComp == nullptr)
        {
            return arsonErrorFromBlazeError(ARSON_ERR_COMPONENT_NOT_FOUND);
        }
        UserSession::SuperUserPermissionAutoPtr autoPtr(true);
        Blaze::IdentitiesRequest blazeRequest;
        Blaze::IdentitiesResponse blazeResponse;
        toBlazeReq(blazeRequest, mRequest);

        err = identityComp->getIdentities(blazeRequest, blazeResponse);
        toArsonRsp(mResponse, blazeResponse);

        if (err != Blaze::ERR_OK)
        {
            WARN_LOG("[ArsonGetIdentitiesCommand].execute: error(" << ErrorHelp::getErrorName(err) << ") returned from Blaze call");
        }
        return arsonErrorFromBlazeError(err);
    }

    static void toBlazeReq(Blaze::IdentitiesRequest& blazeRequest, const Blaze::Arson::IdentitiesRequest& arsonRequest);
    static void toArsonRsp(Blaze::Arson::IdentitiesResponse& arsonResponse, const Blaze::IdentitiesResponse& blazeResponse);
    static Errors arsonErrorFromBlazeError(BlazeRpcError error);
};

DEFINE_ARSONGETIDENTITIES_CREATE()


void ArsonGetIdentitiesCommand::toBlazeReq(Blaze::IdentitiesRequest& blazeRequest, const Blaze::Arson::IdentitiesRequest& arsonRequest)
{
    arsonRequest.getEntityIds().copyInto(blazeRequest.getEntityIds());
    blazeRequest.setBlazeObjectType(arsonRequest.getBlazeObjectType());
}

void ArsonGetIdentitiesCommand::toArsonRsp(Blaze::Arson::IdentitiesResponse& arsonResponse, const Blaze::IdentitiesResponse& blazeResponse)
{
    for (auto it : blazeResponse.getIdentityInfoByEntityIdMap())
    {
        auto* rspMapItem = arsonResponse.getIdentityInfoByEntityIdMap().allocate_element();
        rspMapItem->setBlazeObjectId(it.second->getBlazeObjectId());
        rspMapItem->setIdentityName(it.second->getIdentityName());
    }
}

ArsonGetIdentitiesCommandStub::Errors ArsonGetIdentitiesCommand::arsonErrorFromBlazeError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
        case ARSON_ERR_COMPONENT_NOT_FOUND: result = ARSON_ERR_COMPONENT_NOT_FOUND; break;
        default:
        {
            result = ERR_SYSTEM;
            ERR_LOG("[ArsonGetIdentitiesCommand].arsonErrorFromBlazeError: unexpected error(0x" << SbFormats::HexLower(error) << ":" << ErrorHelp::getErrorName(error) << "): return as (" << ErrorHelp::getErrorName(error) << ")");
            break;
        }
    };
    return result;
}

} //Arson
} //Blaze
