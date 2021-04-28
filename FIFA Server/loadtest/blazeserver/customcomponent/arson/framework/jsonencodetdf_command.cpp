/*************************************************************************************************/
/*!
    \file   jsonencodetdf.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/protocol/shared/jsonencoder.h"
// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/jsonencodetdf_stub.h"

namespace Blaze
{
namespace Arson
{
class JsonEncodeTDFCommand : public JsonEncodeTDFCommandStub
{
public:
    JsonEncodeTDFCommand(Message* message, JSONEncodeTDFRequest* request, ArsonSlaveImpl* componentImpl)
        : JsonEncodeTDFCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~JsonEncodeTDFCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    JsonEncodeTDFCommandStub::Errors execute() override
    {
        BlazeRpcError err = Blaze::ARSON_ERR_JSON_ENCODING_FAILED;
        if (mRequest.getTdf() == nullptr)
        {
            ERR_LOG("[JsonEncodeTDFCommand].execute arson request's variable tdf invalid.");
            return ERR_SYSTEM;
        }

        RawBuffer buf(4096);
        JsonEncoder encoder;
        RpcProtocol::Frame frame;
        if (encoder.encode(buf, *mRequest.getTdf(), &frame))
        {
            mResponse.setEncodedTdf(reinterpret_cast<char8_t*>(buf.data()));
            err = Blaze::ERR_OK;
        }

        return commandErrorFromBlazeError(err);
    }
};

DEFINE_JSONENCODETDF_CREATE()


} //Arson
} //Blaze
