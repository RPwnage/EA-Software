/*************************************************************************************************/
/*!
    \file   bvupsertcontext_command.cpp


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
#include "arson/rpc/arsonslave/bvupsertcontext_stub.h"
#include "proxycomponent/bytevault/rpc/bytevault_defines.h"
#include "proxycomponent/bytevault/rpc/bytevaultslave.h"

namespace Blaze
{
namespace Arson
{
class BvUpsertContextCommand : public BvUpsertContextCommandStub
{
public:
    BvUpsertContextCommand(
        Message* message, Blaze::ByteVault::UpsertContextRequest* request, ArsonSlaveImpl* componentImpl)
        : BvUpsertContextCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~BvUpsertContextCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    BvUpsertContextCommandStub::Errors execute() override
    {
        HttpStatusCode httpStatusCode = 0;
        BlazeRpcError err = mComponent->sendHttpRequest(Blaze::ByteVault::ByteVaultSlave::COMPONENT_ID, Blaze::ByteVault::ByteVaultSlave::CMD_UPSERTCONTEXT, &httpStatusCode, &mRequest, &mResponse);
        if(httpStatusCode == 201 && mResponse.getResourceCreated() == false)
        {
            mResponse.setResourceCreated(true);
        }
        return arsonErrorFromByteVaultError(err);
    }

    static Errors arsonErrorFromByteVaultError(BlazeRpcError error);
};

DEFINE_BVUPSERTCONTEXT_CREATE()

BvUpsertContextCommandStub::Errors BvUpsertContextCommand::arsonErrorFromByteVaultError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
        case BYTEVAULT_AUTHENTICATION_REQUIRED : result = ARSON_ERR_BYTEVAULT_AUTHENTICATION_REQUIRED; break;
        case BYTEVAULT_MISSING_LABEL : result = ARSON_ERR_BYTEVAULT_MISSING_LABEL; break;
        case BYTEVAULT_MISSING_CONTEXT : result = ARSON_ERR_BYTEVAULT_MISSING_CONTEXT; break;
        case BYTEVAULT_MISSING_DESCRIPTION : result = ARSON_ERR_BYTEVAULT_MISSING_DESCRIPTION; break;
        case BYTEVAULT_INVALID_USER : result = ARSON_ERR_BYTEVAULT_INVALID_USER; break;
        case BYTEVAULT_INVALID_TOKEN_TYPE : result = ARSON_ERR_BYTEVAULT_INVALID_TOKEN_TYPE; break;
        case BYTEVAULT_NOT_ALLOWED: result = ARSON_ERR_BYTEVAULT_NOT_ALLOWED; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[BvUpsertContextCommand].arsonErrorFromByteVaultError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
