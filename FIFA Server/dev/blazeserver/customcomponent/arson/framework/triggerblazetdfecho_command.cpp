/*************************************************************************************************/
/*!
    \file   triggerblazetdfecho_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/protocol/shared/encoderfactory.h"
#include "framework/protocol/shared/decoderfactory.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/triggerblazetdfecho_stub.h"

namespace Blaze
{
namespace Arson
{
class TriggerBlazeTdfEchoCommand : public TriggerBlazeTdfEchoCommandStub
{
public:
    TriggerBlazeTdfEchoCommand( Message* message, Blaze::Arson::TriggerTdfEchoRequest* request, ArsonSlaveImpl* componentImpl)
        : TriggerBlazeTdfEchoCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~TriggerBlazeTdfEchoCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    
    TriggerBlazeTdfEchoCommandStub::Errors execute() override
    {
        TRACE_LOG("[TriggerBlazeTdfEchoCommand].start:");

        BlazeRpcError result = ERR_SYSTEM;
        if(mRequest.getEncodingType() == TriggerTdfEchoRequest::EncodingType::JSON_ENCODING)
        {
            RawBuffer buf(4096);
            RpcProtocol::Frame frame;
            Blaze::TdfEncoder* encoder = static_cast<Blaze::TdfEncoder*>(EncoderFactory::create(Encoder::JSON));
            if (encoder->encode(buf, *mRequest.getTdf(), &frame))
            {
                mResponse.setEncodedTdf(reinterpret_cast<char8_t*>(buf.data()));
            }

            Blaze::TdfDecoder* decoder = static_cast<Blaze::TdfDecoder*>(DecoderFactory::create(Decoder::JSON));
            EA::TDF::TdfPtr responseTdf = mRequest.getTdf()->getClassInfo().createInstance(EA_TDF_GET_DEFAULT_ALLOCATOR_PTR, "TriggerBlazeTdfEchoCommandStub::Execute");
            result = decoder->decode(buf, *responseTdf);
            mResponse.setDecodedTdf(*responseTdf);

            delete encoder;
            delete decoder;
        }
        else if(mRequest.getEncodingType() == TriggerTdfEchoRequest::EncodingType::XML_ENCODING)
        {
            RawBuffer buf(4096);
            RpcProtocol::Frame frame;
            
            if(mRequest.getXMLEncUseCommonListEntryName())
            {
                INFO_LOG("[TriggerBlazeTdfEchoCommand].execute: Setting XML Encoded TDF frame parameter useCommonListEntryName to 'true'.");
                frame.useCommonListEntryName = true;
            }
            
            Blaze::TdfEncoder* encoder = static_cast<Blaze::TdfEncoder*>(EncoderFactory::create(Encoder::XML2));
            if (encoder->encode(buf, *mRequest.getTdf(), &frame)) 
            {
                mResponse.setEncodedTdf(reinterpret_cast<char8_t*>(buf.data()));
                result = Blaze::ERR_OK;
            }

            INFO_LOG("[TriggerBlazeTdfEchoCommand].execute: XML Encoded TDF " << reinterpret_cast<char8_t*>(buf.data()));

            Blaze::TdfDecoder* decoder = static_cast<Blaze::TdfDecoder*>(DecoderFactory::create(Decoder::XML2));

            EA::TDF::TdfPtr responseTdf = mRequest.getTdf()->getClassInfo().createInstance(EA_TDF_GET_DEFAULT_ALLOCATOR_PTR, "TriggerBlazeTdfEchoCommandStub::Execute");
            result = decoder->decode(buf, *responseTdf);
            mResponse.setDecodedTdf(*responseTdf);

            delete encoder;
            delete decoder;
        }
        else
        {
            WARN_LOG("[TriggerBlazeTdfEchoCommand].execute: Encoding Type " << mRequest.getEncodingType() << " not recognized!");
            return ERR_SYSTEM;
        }
        return commandErrorFromBlazeError(result);
    }
};

DEFINE_TRIGGERBLAZETDFECHO_CREATE()

} //Arson
} //Blaze
