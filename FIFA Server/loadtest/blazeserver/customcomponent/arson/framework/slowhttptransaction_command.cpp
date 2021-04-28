/*************************************************************************************************/
/*!
    \file   slowtxngetdata_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/slowhttptransaction_stub.h"
#include "framework/connection/outboundhttpservice.h"

#include "testservice/tdf/testservice.h"
#include "testservice/rpc/testserviceslave.h"
#include "EAStdC/EAString.h"

namespace Blaze
{
namespace Arson
{
class SlowHttpTransactionCommand : public SlowHttpTransactionCommandStub
{
public:
    SlowHttpTransactionCommand(
        Message* message, Blaze::Arson::SlowTxnObject* request, ArsonSlaveImpl* componentImpl)
        : SlowHttpTransactionCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~SlowHttpTransactionCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    SlowHttpTransactionCommandStub::Errors execute() override
    {

        BlazeRpcError err = ERR_SYSTEM;

        Blaze::TestService::TestServiceSlave * testServiceSlave = (Blaze::TestService::TestServiceSlave *)Blaze::gOutboundHttpService->getService(Blaze::TestService::TestServiceSlave::COMPONENT_INFO.name);

        if (testServiceSlave == nullptr)
        {
            ERR_LOG("[ArsonComponent] Failed to instantiate testServiceSlave object.");
            return ERR_SYSTEM;
        }

        Blaze::TestService::SlowTxnRequest req;
        Blaze::TestService::SlowTxnResponse rsp;
        req.setDelaySecs(mRequest.getDelaySecs());

        if(0 == blaze_strcmp(mRequest.getMethod(), "GET"))
        {
            err = testServiceSlave->GetData(req, rsp);
        }
        else if(0 == blaze_strcmp(mRequest.getMethod(), "PUT"))
        {
            err = testServiceSlave->PutData(req, rsp);
        }
        else if(0 == blaze_strcmp(mRequest.getMethod(), "POST"))
        {
            err = testServiceSlave->PostData(req, rsp);
        }    
        else if(0 == blaze_strcmp(mRequest.getMethod(), "DELETE"))
        {
            err = testServiceSlave->DeleteData(req, rsp);
        }
        else
        {
            ERR_LOG("[SlowHttpTransaction].execute: failed to send http request. NO MATCH METHOD in GET, PUT, POST, DELETE");
            return ERR_SYSTEM;
        }

        if(err == Blaze::ERR_OK)
        {
            mResponse.setDelaySecs(EA::StdC::AtoU32(rsp.getDelaySecs()));
            mResponse.setMethod(rsp.getMethod());

            return ERR_OK;
        }
        else
        {
            ERR_LOG("[SlowHttpTransaction].execute: returns error code" << err << "instead of ERR_OK");
            return ERR_SYSTEM;
        }
    }

};

DEFINE_SLOWHTTPTRANSACTION_CREATE()

} //Arson
} //Blaze
