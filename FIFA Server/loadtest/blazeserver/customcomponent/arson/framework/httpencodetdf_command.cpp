/*************************************************************************************************/
/*!
    \file   httpencodetdf.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/protocol/shared/httpencoder.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/httpencodetdf_stub.h"

namespace Blaze
{
namespace Arson
{
class HttpEncodeTDFCommand : public HttpEncodeTDFCommandStub
{
public:
    HttpEncodeTDFCommand(Message* message, HttpEncodeTDFRequest* request, ArsonSlaveImpl* componentImpl)
        : HttpEncodeTDFCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~HttpEncodeTDFCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    HttpEncodeTDFCommandStub::Errors execute() override
    {
        BlazeRpcError err = Blaze::ARSON_ERR_XML_ENCODING_FAILED;

        RawBuffer buf(4096);
        HttpEncoder encoder;
        RpcProtocol::Frame frame;
        if (encoder.encode(buf, mRequest, &frame))
        {
            mResponse.setEncodedTdf(reinterpret_cast<char8_t*>(buf.data()));
            err = Blaze::ERR_OK;
        }

        return commandErrorFromBlazeError(err);
    }
};

DEFINE_HTTPENCODETDF_CREATE()


} //Arson
} //Blaze
