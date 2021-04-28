/*************************************************************************************************/
/*!
    \file   arsongetidentitybyname_command.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"

// arson includes
#include "arsonslaveimpl.h"
#include "framework/rpc/identityslave.h"
#include "arson/identity/tdf/arsonidentitytypes.h"
#include "rpc/arsonslave/arsongetidentitybyname_stub.h"

namespace Blaze
{
namespace Arson
{

/*! ************************************************************************************************/
/*! \brief Calls the Blaze Server side only RPC, allowing ARSON client to call it using a typed interface.
*************************************************************************************************/
class ArsonGetIdentityByNameCommand : public ArsonGetIdentityByNameCommandStub
{
public:
    ArsonGetIdentityByNameCommand(Message* message,
        Blaze::Arson::IdentityByNameRequest* request, ArsonSlaveImpl* componentImpl)
        : ArsonGetIdentityByNameCommandStub(message, request),
        mComponent(componentImpl)
    {
    }
    ~ArsonGetIdentityByNameCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

private:

    ArsonGetIdentityByNameCommandStub::Errors execute() override
    {
        BlazeRpcError err = Blaze::ERR_OK;
        Blaze::IdentitySlave* identityComp = static_cast<Blaze::IdentitySlave*>(gController->getComponent(Blaze::IdentitySlave::COMPONENT_ID, false, true, &err));
        if (identityComp == nullptr)
        {
            return arsonErrorFromBlazeError(ARSON_ERR_COMPONENT_NOT_FOUND);
        }

        UserSession::SuperUserPermissionAutoPtr autoPtr(true);
        Blaze::IdentityByNameRequest blazeRequest;
        Blaze::IdentityInfo blazeResponse;
        toBlazeReq(blazeRequest, mRequest);

        err = identityComp->getIdentityByName(blazeRequest, blazeResponse);
        toArsonRsp(mResponse, blazeResponse);

        if (err != Blaze::ERR_OK)
        {
            WARN_LOG("[ArsonGetIdentityByNameCommand].execute: error(" << ErrorHelp::getErrorName(err) << ") returned from Blaze call");
        }
        return arsonErrorFromBlazeError(err);
    }

    static void toBlazeReq(Blaze::IdentityByNameRequest& blazeRequest, const Blaze::Arson::IdentityByNameRequest& arsonRequest);
    static void toArsonRsp(Blaze::Arson::IdentityInfo& arsonResponse, const Blaze::IdentityInfo& blazeResponse);

    static ArsonGetIdentityByNameCommandStub::Errors arsonErrorFromBlazeError(BlazeRpcError error);
    
};

DEFINE_ARSONGETIDENTITYBYNAME_CREATE()

void ArsonGetIdentityByNameCommand::toBlazeReq(Blaze::IdentityByNameRequest& blazeRequest, const Blaze::Arson::IdentityByNameRequest& arsonRequest)
{
    blazeRequest.setEntityName(arsonRequest.getEntityName());
    blazeRequest.setBlazeObjectType(arsonRequest.getBlazeObjectType());
}

void ArsonGetIdentityByNameCommand::toArsonRsp(Blaze::Arson::IdentityInfo& arsonResponse, const Blaze::IdentityInfo& blazeResponse)
{
    arsonResponse.setIdentityName(blazeResponse.getIdentityName());
    arsonResponse.setBlazeObjectId(blazeResponse.getBlazeObjectId());
}

ArsonGetIdentityByNameCommandStub::Errors ArsonGetIdentityByNameCommand::arsonErrorFromBlazeError(BlazeRpcError error)
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
            //got an error not defined in rpc definition, log it
            result = ERR_SYSTEM;
            ERR_LOG("[ArsonGetIdentityByNameCommand].arsonErrorFromBlazeError: unexpected error(0x" << SbFormats::HexLower(error) << ":" << ErrorHelp::getErrorName(error) << "): return as (" << ErrorHelp::getErrorName(error) << ")");
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
