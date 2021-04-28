/*************************************************************************************************/
/*!
    \file   xmldecodetdf.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/protocol/shared/httpdecoder.h"
#include "framework/protocol/shared/decoderfactory.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/xmldecodetdf_stub.h"

namespace Blaze
{
namespace Arson
{
class XmlDecodeTDFCommand : public XmlDecodeTDFCommandStub
{
public:
    XmlDecodeTDFCommand(Message* message, XMLDecodeTDFRequest* request, ArsonSlaveImpl* componentImpl)
        : XmlDecodeTDFCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~XmlDecodeTDFCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    XmlDecodeTDFCommandStub::Errors execute() override
    {
        RawBuffer buffer(4096);
        size_t len = strlen(mRequest.getEncodedTdf());
        
        uint8_t* outputBuffer = buffer.acquire(len + 1);
        if (outputBuffer != nullptr)
        {
            memcpy(outputBuffer, mRequest.getEncodedTdf(), len);
            outputBuffer[len] = '\0';
            buffer.put(len);
        }

        Blaze::Arson::MapofUnion* mapTest = BLAZE_NEW Blaze::Arson::MapofUnion;
        mResponse.setTdf(*mapTest);

        Blaze::TdfDecoder* decoder = static_cast<Blaze::TdfDecoder*>(DecoderFactory::create(Decoder::XML2));
        XmlDecodeTDFCommandStub::Errors result = commandErrorFromBlazeError(decoder->decode(buffer, *mResponse.getTdf()));
        delete decoder;
        return result;
    }
};

DEFINE_XMLDECODETDF_CREATE()


} //Arson
} //Blaze
