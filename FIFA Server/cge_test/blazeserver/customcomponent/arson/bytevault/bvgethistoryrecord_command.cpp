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
#include "arson/rpc/arsonslave/bvgethistoryrecord_stub.h"

#include "framework/connection/outboundhttpservice.h"
#include "proxycomponent/bytevault/rpc/bytevault_defines.h"
#include "proxycomponent/bytevault/rpc/bytevaultslave.h"

namespace Blaze
{
namespace Arson
{
class BvGetHistoryRecordCommand : public BvGetHistoryRecordCommandStub
{
public:
    BvGetHistoryRecordCommand(
        Message* message, Blaze::ByteVault::GetHistoryRecordRequest* request, ArsonSlaveImpl* componentImpl)
        : BvGetHistoryRecordCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~BvGetHistoryRecordCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    BvGetHistoryRecordCommandStub::Errors execute() override
    {
        BlazeRpcError err = ERR_SYSTEM;

        Blaze::ByteVault::ByteVaultSlave* byteVault = (Blaze::ByteVault::ByteVaultSlave*)gOutboundHttpService->getService("bytevault");
        if (byteVault != nullptr)
        {
            err = byteVault->getHistoryRecord(mRequest, mResponse);
        }  
        return arsonErrorFromByteVaultError(err);
    }

    static Errors arsonErrorFromByteVaultError(BlazeRpcError error);
};

DEFINE_BVGETHISTORYRECORD_CREATE()

BvGetHistoryRecordCommandStub::Errors BvGetHistoryRecordCommand::arsonErrorFromByteVaultError(BlazeRpcError error)
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
        //case BYTEVAULT_INVALID_DELETIONTIME: result = ARSON_ERR_BYTEVAULT_INVALID_DELETIONTIME; break;
        case BYTEVAULT_INVALID_USER_TYPE: result = ARSON_ERR_BYTEVAULT_INVALID_USER; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[BvGetHistoryRecordCommand].arsonErrorFromByteVaultError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
