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
#include "arson/rpc/arsonslave/slowhttpsensitiveinfotxn2_stub.h"
#include "framework/connection/outboundhttpservice.h"

#include "testservice/tdf/testservice.h"
#include "testservice/rpc/testserviceslave.h"
#include "EAStdC/EAString.h"

namespace Blaze
{
namespace Arson
{
class SlowHttpSensitiveInfoTxn2Command : public SlowHttpSensitiveInfoTxn2CommandStub
{
public:
    SlowHttpSensitiveInfoTxn2Command(
        Message* message, Blaze::Arson::SlowTxnSensitiveInfoObject* request, ArsonSlaveImpl* componentImpl)
        : SlowHttpSensitiveInfoTxn2CommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~SlowHttpSensitiveInfoTxn2Command() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    SlowHttpSensitiveInfoTxn2CommandStub::Errors execute() override
    {

        BlazeRpcError err = ERR_SYSTEM;

        Blaze::TestService::TestServiceSlave * testServiceSlave = (Blaze::TestService::TestServiceSlave *)Blaze::gOutboundHttpService->getService(Blaze::TestService::TestServiceSlave::COMPONENT_INFO.name);

        if (testServiceSlave == nullptr)
        {
            ERR_LOG("[ArsonComponent] Failed to instantiate testServiceSlave object.");
            return ERR_SYSTEM;
        }

        Blaze::TestService::SlowTxnSensitiveInfoRequest req;
        Blaze::TestService::SlowTxnSensitiveInfoResponse rsp;
        req.setDelaySecs(mRequest.getDelaySecs());
        req.setEmail(mRequest.getEmail());
        req.setPassword(mRequest.getPassword());

        err = testServiceSlave->GetSensitiveInfo(req, rsp);

        if(err == Blaze::ERR_OK)
        {
            mResponse.setDelaySecs(EA::StdC::AtoU32(rsp.getDelaySecs()));
            mResponse.setMethod(rsp.getMethod());
            mResponse.setEmail(rsp.getUserEmail());
            mResponse.setPassword(rsp.getUserPassword());

            return ERR_OK;
        }
        else
        {
            ERR_LOG("[SlowHttpSensitiveInfoTxn2].execute: returns error code" << err << "instead of ERR_OK");
            return ERR_SYSTEM;
        }
    }

};

DEFINE_SLOWHTTPSENSITIVEINFOTXN2_CREATE()

} //Arson
} //Blaze
