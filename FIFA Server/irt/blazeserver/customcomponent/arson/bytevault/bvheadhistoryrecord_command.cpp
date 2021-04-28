/*************************************************************************************************/
/*!
    \file   bvgetcontexts_command.cpp


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
#include "arson/rpc/arsonslave/bvheadhistoryrecord_stub.h"

#include "framework/connection/outboundhttpservice.h"
#include "proxycomponent/bytevault/rpc/bytevault_defines.h"
#include "proxycomponent/bytevault/rpc/bytevaultslave.h"

namespace Blaze
{
namespace Arson
{
class BvHeadHistoryRecordCommand : public BvHeadHistoryRecordCommandStub
{
public:
    BvHeadHistoryRecordCommand(
        Message* message, Blaze::ByteVault::GetHistoryRecordRequest* request, ArsonSlaveImpl* componentImpl)
        : BvHeadHistoryRecordCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~BvHeadHistoryRecordCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    BvHeadHistoryRecordCommandStub::Errors execute() override
    {
        BlazeRpcError err = ERR_SYSTEM;

        Blaze::ByteVault::ByteVaultSlave* byteVault = (Blaze::ByteVault::ByteVaultSlave*)gOutboundHttpService->getService("bytevault");
        if (byteVault != nullptr)
        {
            err = byteVault->headHistoryRecord(mRequest, mResponse);
        }
        return arsonErrorFromByteVaultError(err);
    }

    static Errors arsonErrorFromByteVaultError(BlazeRpcError error);
};

DEFINE_BVHEADHISTORYRECORD_CREATE()

BvHeadHistoryRecordCommandStub::Errors BvHeadHistoryRecordCommand::arsonErrorFromByteVaultError(BlazeRpcError error)
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
        case BYTEVAULT_INVALID_CATEGORY: result =  ARSON_ERR_BYTEVAULT_INVALID_CATEGORY; break;
        case BYTEVAULT_MISSING_CONTEXT: result = ARSON_ERR_BYTEVAULT_MISSING_CONTEXT; break;
        case BYTEVAULT_MISSING_CATEGORY: result = ARSON_ERR_BYTEVAULT_MISSING_CATEGORY; break;
        case BYTEVAULT_MISSING_RECORD_NAME: result = ARSON_ERR_BYTEVAULT_MISSING_RECORD_NAME; break;
        case BYTEVAULT_NO_MATCHING_RECORD: result = ARSON_ERR_BYTEVAULT_NO_MATCHING_RECORD; break;
        case BYTEVAULT_INVALID_USER: result = ARSON_ERR_BYTEVAULT_INVALID_USER; break;
        case BYTEVAULT_INVALID_TOKEN_TYPE : result = ARSON_ERR_BYTEVAULT_INVALID_TOKEN_TYPE; break;
        case BYTEVAULT_INVALID_LASTUPDATETIME: result = ARSON_ERR_BYTEVAULT_INVALID_LASTUPDATETIME; break;
        case BYTEVAULT_INVALID_USER_TYPE: result = ARSON_ERR_BYTEVAULT_INVALID_USER; break;
        
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[BvHeadHistoryRecordCommand].arsonErrorFromByteVaultError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
