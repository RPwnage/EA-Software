/*************************************************************************************************/
/*!
    \file   jsondecodetdf.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/protocol/shared/jsondecoder.h"
#include "framework/protocol/shared/decoderfactory.h"
// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/jsondecodetdf_stub.h"

namespace Blaze
{
namespace Arson
{
class JsonDecodeTDFCommand : public JsonDecodeTDFCommandStub
{
public:
    JsonDecodeTDFCommand(Message* message, JSONDecodeTDFRequest* request, ArsonSlaveImpl* componentImpl)
        : JsonDecodeTDFCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~JsonDecodeTDFCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    JsonDecodeTDFCommandStub::Errors execute() override
    {
        BlazeRpcError result = Blaze::ARSON_ERR_JSON_ENCODING_FAILED;
        if (mRequest.getDecodedTdf() == nullptr)
        {
            ERR_LOG("[JsonDecodeTDFCommand].execute arson request's string tdf invalid.");
            return ERR_SYSTEM;
        }

        Blaze::TdfDecoder* decoder = static_cast<Blaze::TdfDecoder*>(DecoderFactory::create(Decoder::JSON));
        //JsonDecoder decoder;
        RawBuffer buffer(4096);

        size_t len = strlen(mRequest.getDecodedTdf());

        uint8_t* outputBuffer = buffer.acquire(len + 1);
        if (outputBuffer != nullptr)
        {
            memcpy(outputBuffer, mRequest.getDecodedTdf(), len);
            outputBuffer[len] = '\0';
            buffer.put(len);
        }
        EA::TDF::TdfPtr responseTdf = mRequest.getTdf()->getClassInfo().createInstance(EA_TDF_GET_DEFAULT_ALLOCATOR_PTR, "JsonDecodeTDFCommand::Execute");
        result = decoder->decode(buffer, *responseTdf);
        mResponse.setTdf(*responseTdf);

        delete decoder;
        return commandErrorFromBlazeError(result);
    }
};

DEFINE_JSONDECODETDF_CREATE()


} //Arson
} //Blaze
