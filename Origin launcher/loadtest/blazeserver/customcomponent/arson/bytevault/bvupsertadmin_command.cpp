/*************************************************************************************************/
/*!
    \file   bvupsertadmin_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/bvupsertadmin_stub.h"
#include "proxycomponent/bytevault/rpc/bytevault_defines.h"
#include "proxycomponent/bytevault/rpc/bytevaultslave.h"

namespace Blaze
{
namespace Arson
{
class BvUpsertAdminCommand : public BvUpsertAdminCommandStub
{
public:
    BvUpsertAdminCommand(
        Message* message, Blaze::ByteVault::UpsertAdminRequest* request, ArsonSlaveImpl* componentImpl)
        : BvUpsertAdminCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~BvUpsertAdminCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    BvUpsertAdminCommandStub::Errors execute() override
    {
        HttpStatusCode httpStatusCode = 0;
        BlazeRpcError err = mComponent->sendHttpRequest(Blaze::ByteVault::ByteVaultSlave::COMPONENT_ID, Blaze::ByteVault::ByteVaultSlave::CMD_UPSERTADMIN, &httpStatusCode, &mRequest, &mResponse);
        if(httpStatusCode == 201 && mResponse.getResourceCreated() == false)
        {
            mResponse.setResourceCreated(true);
        }

        return arsonErrorFromByteVaultError(err);
    }

    static Errors arsonErrorFromByteVaultError(BlazeRpcError error);
};

DEFINE_BVUPSERTADMIN_CREATE()

BvUpsertAdminCommandStub::Errors BvUpsertAdminCommand::arsonErrorFromByteVaultError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
        case BYTEVAULT_AUTHENTICATION_REQUIRED : result = ARSON_ERR_BYTEVAULT_AUTHENTICATION_REQUIRED; break;
        case BYTEVAULT_NOT_ALLOWED : result = ARSON_ERR_BYTEVAULT_NOT_ALLOWED; break;
        case BYTEVAULT_INVALID_USER : result = ARSON_ERR_BYTEVAULT_INVALID_USER; break;
        case BYTEVAULT_INVALID_TOKEN_TYPE : result = ARSON_ERR_BYTEVAULT_INVALID_TOKEN_TYPE; break;
        case BYTEVAULT_MISSING_CONTEXT : result = ARSON_ERR_BYTEVAULT_MISSING_CONTEXT; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[BvUpsertAdminCommand].arsonErrorFromByteVaultError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}


} //Arson
} //Blaze
