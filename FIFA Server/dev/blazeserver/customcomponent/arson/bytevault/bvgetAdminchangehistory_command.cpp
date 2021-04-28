/*************************************************************************************************/
/*!
    \file   bvgetadminchangehistory_command.cpp


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
#include "arson/rpc/arsonslave/bvgetadminchangehistory_stub.h"

#include "framework/connection/outboundhttpservice.h"
#include "proxycomponent/bytevault/rpc/bytevault_defines.h"
#include "proxycomponent/bytevault/rpc/bytevaultslave.h"

namespace Blaze
{
namespace Arson
{
class BvGetAdminChangeHistoryCommand : public BvGetAdminChangeHistoryCommandStub
{
public:
    BvGetAdminChangeHistoryCommand(
        Message* message, Blaze::ByteVault::GetAdminChangeHistoryRequest* request, ArsonSlaveImpl* componentImpl)
        : BvGetAdminChangeHistoryCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~BvGetAdminChangeHistoryCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    BvGetAdminChangeHistoryCommandStub::Errors execute() override
    {
        BlazeRpcError err = ERR_SYSTEM;


        Blaze::ByteVault::ByteVaultSlave* byteVault = (Blaze::ByteVault::ByteVaultSlave*)gOutboundHttpService->getService("bytevault");
        if (byteVault != nullptr)
        {
            err = byteVault->getAdminChangeHistory(mRequest, mResponse);
        }
        
        return arsonErrorFromByteVaultError(err);
    }

    static Errors arsonErrorFromByteVaultError(BlazeRpcError error);
};

DEFINE_BVGETADMINCHANGEHISTORY_CREATE()

BvGetAdminChangeHistoryCommandStub::Errors BvGetAdminChangeHistoryCommand::arsonErrorFromByteVaultError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
        case BYTEVAULT_AUTHENTICATION_REQUIRED : result = ARSON_ERR_BYTEVAULT_AUTHENTICATION_REQUIRED; break;
        case BYTEVAULT_MISSING_CONTEXT: result = ARSON_ERR_BYTEVAULT_MISSING_CONTEXT; break;
        case BYTEVAULT_INVALID_USER: result = ARSON_ERR_BYTEVAULT_INVALID_USER; break;
        case BYTEVAULT_MISSING_CATEGORY: result = ARSON_ERR_BYTEVAULT_MISSING_CATEGORY; break;
        case BYTEVAULT_INVALID_TOKEN_TYPE: result = ARSON_ERR_BYTEVAULT_INVALID_TOKEN_TYPE; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[BvGetAdminChangeHistoryCommand].arsonErrorFromByteVaultError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
