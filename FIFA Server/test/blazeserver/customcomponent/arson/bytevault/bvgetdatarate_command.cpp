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
#include "arson/rpc/arsonslave/bvgetdatarates_stub.h"

#include "framework/connection/outboundhttpservice.h"
#include "proxycomponent/bytevault/rpc/bytevaultslave.h"

namespace Blaze
{
namespace Arson
{
class BvGetDataRatesCommand : public BvGetDataRatesCommandStub
{
public:
    BvGetDataRatesCommand(
        Message* message, Blaze::ByteVault::GetDataRatesRequest* request, ArsonSlaveImpl* componentImpl)
        : BvGetDataRatesCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~BvGetDataRatesCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    BvGetDataRatesCommandStub::Errors execute() override
    {

        BlazeRpcError err = ERR_SYSTEM;


        Blaze::ByteVault::ByteVaultSlave* byteVault = (Blaze::ByteVault::ByteVaultSlave*)gOutboundHttpService->getService("bytevault");
        if (byteVault != nullptr)
        {
            err = byteVault->getDataRates(mRequest, mResponse);
        }
        
        return arsonErrorFromByteVaultError(err);
    }

    static Errors arsonErrorFromByteVaultError(BlazeRpcError error);
};

DEFINE_BVGETDATARATES_CREATE()

BvGetDataRatesCommandStub::Errors BvGetDataRatesCommand::arsonErrorFromByteVaultError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[BvGetDataRateCommand].arsonErrorFromByteVaultError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
