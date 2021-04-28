/*************************************************************************************************/
/*!
    \file   bvupsertrecord_command.cpp


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
#include "arson/rpc/arsonslave/bvupsertrecord_stub.h"
#include "proxycomponent/bytevault/rpc/bytevault_defines.h"
#include "proxycomponent/bytevault/rpc/bytevaultslave.h"

namespace Blaze
{
namespace Arson
{
class BvUpsertRecordCommand : public BvUpsertRecordCommandStub
{
public:
    BvUpsertRecordCommand(
        Message* message, Blaze::ByteVault::UpsertRecordRequest* request, ArsonSlaveImpl* componentImpl)
        : BvUpsertRecordCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~BvUpsertRecordCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    BvUpsertRecordCommandStub::Errors execute() override
    {
        HttpStatusCode httpStatusCode = 0;
        
        BlazeRpcError err = mComponent->sendHttpRequest(Blaze::ByteVault::ByteVaultSlave::COMPONENT_ID, Blaze::ByteVault::ByteVaultSlave::CMD_UPSERTRECORD, &httpStatusCode, &mRequest, nullptr, &mResponse, nullptr);
        
        if(httpStatusCode == 201 && mResponse.getResourceCreated() == false)
        {
            mResponse.setResourceCreated(true);
        }

        return arsonErrorFromByteVaultError(err);
    }

    static Errors arsonErrorFromByteVaultError(BlazeRpcError error);
};

DEFINE_BVUPSERTRECORD_CREATE()

BvUpsertRecordCommandStub::Errors BvUpsertRecordCommand::arsonErrorFromByteVaultError(BlazeRpcError error)
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
        case BYTEVAULT_INVALID_CATEGORY: result =  ARSON_ERR_BYTEVAULT_INVALID_CATEGORY; break;
        case BYTEVAULT_MISSING_CATEGORY: result = ARSON_ERR_BYTEVAULT_MISSING_CATEGORY; break;
        case BYTEVAULT_INVALID_TOKEN_TYPE: result = ARSON_ERR_BYTEVAULT_INVALID_TOKEN_TYPE; break;
        case BYTEVAULT_INVALID_PAYLOAD: result = ARSON_ERR_BYTEVAULT_INVALID_PAYLOAD; break;
        case BYTEVAULT_MISSING_CONTENT_TYPE: result = ARSON_ERR_BYTEVAULT_MISSING_CONTENT_TYPE; break; 
        case BYTEVAULT_MISSING_CONTENT_LENGTH: result = ARSON_ERR_BYTEVAULT_MISSING_CONTENT_LENGTH; break;
        case BYTEVAULT_MISSING_RECORD_NAME: result = ARSON_ERR_BYTEVAULT_MISSING_RECORD_NAME; break;
        case BYTEVAULT_MAX_RECORD_SIZE_EXCEEDED: result = ARSON_ERR_BYTEVAULT_MAX_RECORD_SIZE_EXCEEDED; break;
        case BYTEVAULT_MAX_RECORD_COUNT_EXCEEDED: result = ARSON_ERR_BYTEVAULT_MAX_RECORD_COUNT_EXCEEDED; break;
        case BYTEVAULT_NOT_ALLOWED: result = ARSON_ERR_BYTEVAULT_NOT_ALLOWED; break;
        case BYTEVAULT_INVALID_USER_TYPE: result = ARSON_ERR_BYTEVAULT_INVALID_USER_TYPE; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[BvUpsertRecordCommand].arsonErrorFromByteVaultError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
