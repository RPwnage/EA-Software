/*************************************************************************************************/
/*!
    \file   bvsoftdeleterecords_command.cpp


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
#include "arson/rpc/arsonslave/bvarchiverecords_stub.h"

#include "proxycomponent/bytevault/rpc/bytevault_defines.h"
#include "proxycomponent/bytevault/rpc/bytevaultslave.h"
#include "framework/connection/outboundhttpservice.h"

namespace Blaze
{
namespace Arson
{
class BvArchiveRecordsCommand : public BvArchiveRecordsCommandStub
{
public:
    BvArchiveRecordsCommand(
        Message* message, Blaze::ByteVault::ArchiveRecordsRequest* request, ArsonSlaveImpl* componentImpl)
        : BvArchiveRecordsCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~BvArchiveRecordsCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    BvArchiveRecordsCommandStub::Errors execute() override
    {
        BlazeRpcError err = ERR_SYSTEM;

        Blaze::ByteVault::ByteVaultSlave* byteVault = (Blaze::ByteVault::ByteVaultSlave*)gOutboundHttpService->getService("bytevault");
        if (byteVault != nullptr)
        {
            err = byteVault->archiveRecords(mRequest);
        }

        return arsonErrorFromByteVaultError(err);
    }

    static Errors arsonErrorFromByteVaultError(BlazeRpcError error);
};

DEFINE_BVARCHIVERECORDS_CREATE()

BvArchiveRecordsCommandStub::Errors BvArchiveRecordsCommand::arsonErrorFromByteVaultError(BlazeRpcError error)
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
        case BYTEVAULT_MISSING_RECORD_NAME: result =  ARSON_ERR_BYTEVAULT_MISSING_RECORD_NAME; break;
        case BYTEVAULT_INVALID_TOKEN_TYPE: result = ARSON_ERR_BYTEVAULT_INVALID_TOKEN_TYPE; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[BvArchiveRecordCommand].arsonErrorFromByteVaultError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
