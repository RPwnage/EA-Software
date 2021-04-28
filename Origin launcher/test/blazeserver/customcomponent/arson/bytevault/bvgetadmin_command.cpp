/*************************************************************************************************/
/*!
    \file   bvgetadmin_command.cpp


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
#include "arson/rpc/arsonslave/bvgetadmin_stub.h"
#include "framework/connection/outboundhttpservice.h"
#include "proxycomponent/bytevault/rpc/bytevault_defines.h"
#include "proxycomponent/bytevault/rpc/bytevaultslave.h"

namespace Blaze
{
namespace Arson
{
class BvGetAdminCommand : public BvGetAdminCommandStub
{
public:
    BvGetAdminCommand(
        Message* message, Blaze::ByteVault::GetAdminRequest* request, ArsonSlaveImpl* componentImpl)
        : BvGetAdminCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~BvGetAdminCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    BvGetAdminCommandStub::Errors execute() override
    {

        BlazeRpcError err = ERR_SYSTEM;


        Blaze::ByteVault::ByteVaultSlave* byteVault = (Blaze::ByteVault::ByteVaultSlave*)gOutboundHttpService->getService("bytevault");
        if (byteVault != nullptr)
        {
            err = byteVault->getAdmin(mRequest, mResponse);
        }


        return arsonErrorFromByteVaultError(err);
    }

    static Errors arsonErrorFromByteVaultError(BlazeRpcError error);
};

DEFINE_BVGETADMIN_CREATE()

BvGetAdminCommandStub::Errors BvGetAdminCommand::arsonErrorFromByteVaultError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
        case BYTEVAULT_AUTHENTICATION_REQUIRED : result = ARSON_ERR_BYTEVAULT_AUTHENTICATION_REQUIRED; break;
        case BYTEVAULT_INVALID_USER : result = ARSON_ERR_BYTEVAULT_INVALID_USER; break;
        case BYTEVAULT_INVALID_TOKEN_TYPE : result = ARSON_ERR_BYTEVAULT_INVALID_TOKEN_TYPE; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[BvGetAdminCommand].arsonErrorFromByteVaultError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
