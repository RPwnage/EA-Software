/*************************************************************************************************/
/*!
    \file   bvupsertcategory_command.cpp


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
#include "arson/rpc/arsonslave/bvupsertcategory_stub.h"
#include "proxycomponent/bytevault/rpc/bytevault_defines.h"
#include "proxycomponent/bytevault/rpc/bytevaultslave.h"

namespace Blaze
{
namespace Arson
{
class BvUpsertCategoryCommand : public BvUpsertCategoryCommandStub
{
public:
    BvUpsertCategoryCommand(
        Message* message, Blaze::ByteVault::UpsertCategoryRequest* request, ArsonSlaveImpl* componentImpl)
        : BvUpsertCategoryCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~BvUpsertCategoryCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    BvUpsertCategoryCommandStub::Errors execute() override
    {
        HttpStatusCode httpStatusCode = 0;
        BlazeRpcError err = mComponent->sendHttpRequest(Blaze::ByteVault::ByteVaultSlave::COMPONENT_ID, Blaze::ByteVault::ByteVaultSlave::CMD_UPSERTCATEGORY, &httpStatusCode, &mRequest, &mResponse);
        if(httpStatusCode == 201 && mResponse.getResourceCreated() == false)
        {
            mResponse.setResourceCreated(true);
        }        
        return arsonErrorFromByteVaultError(err);
    }

    static Errors arsonErrorFromByteVaultError(BlazeRpcError error);
};

DEFINE_BVUPSERTCATEGORY_CREATE()

BvUpsertCategoryCommandStub::Errors BvUpsertCategoryCommand::arsonErrorFromByteVaultError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
        case BYTEVAULT_AUTHENTICATION_REQUIRED : result = ARSON_ERR_BYTEVAULT_AUTHENTICATION_REQUIRED; break;
        case BYTEVAULT_INVALID_CONTEXT: result =  ARSON_ERR_BYTEVAULT_INVALID_CONTEXT; break;
        case BYTEVAULT_MISSING_CONTEXT: result = ARSON_ERR_BYTEVAULT_MISSING_CONTEXT; break;
        case BYTEVAULT_INVALID_USER: result = ARSON_ERR_BYTEVAULT_INVALID_USER; break;
        case BYTEVAULT_MISSING_CATEGORY: result = ARSON_ERR_BYTEVAULT_MISSING_CATEGORY; break;
        case BYTEVAULT_MISSING_DESCRIPTION: result = ARSON_ERR_BYTEVAULT_MISSING_DESCRIPTION; break;
        case BYTEVAULT_INVALID_MAX_RECORD_COUNT: result = ARSON_ERR_BYTEVAULT_INVALID_MAX_RECORD_COUNT; break;
        case BYTEVAULT_INVALID_MAX_RECORD_SIZE: result = ARSON_ERR_BYTEVAULT_INVALID_MAX_RECORD_SIZE; break;
        case BYTEVAULT_INVALID_TRUSTED_SOURCE : result = ARSON_ERR_BYTEVAULT_INVALID_TRUSTED_SOURCE; break;
        case BYTEVAULT_INVALID_TOKEN_TYPE : result = ARSON_ERR_BYTEVAULT_INVALID_TOKEN_TYPE; break;
        case BYTEVAULT_NOT_ALLOWED: result = ARSON_ERR_BYTEVAULT_NOT_ALLOWED; break;
        case BYTEVAULT_INVALID_MAX_HISTORY_RECORD_COUNT : result = ARSON_ERR_BYTEVAULT_INVALID_MAX_HISTORY_RECORD_COUNT; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[BvUpsertCategoryCommand].arsonErrorFromByteVaultError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
