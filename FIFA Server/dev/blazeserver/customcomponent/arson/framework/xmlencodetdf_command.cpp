/*************************************************************************************************/
/*!
    \file   xmlencodetdf.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/protocol/xml2encoder.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/xmlencodetdf_stub.h"

namespace Blaze
{
namespace Arson
{
class XmlEncodeTDFCommand : public XmlEncodeTDFCommandStub
{
public:
    XmlEncodeTDFCommand(Message* message, XMLEncodeTDFRequest* request, ArsonSlaveImpl* componentImpl)
        : XmlEncodeTDFCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~XmlEncodeTDFCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    XmlEncodeTDFCommandStub::Errors execute() override
    {
        BlazeRpcError err = Blaze::ARSON_ERR_XML_ENCODING_FAILED;

        RawBuffer buf(4096);
        Xml2Encoder encoder;
        RpcProtocol::Frame frame;
        if (encoder.encode(buf, *mRequest.getTdf(), &frame))
        {
            mResponse.setEncodedTdf(reinterpret_cast<char8_t*>(buf.data()));
            err = Blaze::ERR_OK;
        }

        return commandErrorFromBlazeError(err);
    }
};

DEFINE_XMLENCODETDF_CREATE()


} //Arson
} //Blaze
