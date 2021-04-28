#include "EABase/eabase.h"  //include eabase.h before any std lib includes for VS2012 (see EABase/2.02.00/doc/FAQ.html#Info.17 for details)

#include <stdlib.h>
#include <iostream>

#ifndef ENABLE_BLAZE_MEM_SYSTEM
#define ENABLE_BLAZE_MEM_SYSTEM 1
#endif

#include "framework/blaze.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/util/shared/rawbufferistream.h"
#include "framework/logger.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/rpc/blazecontrollerslave.h"
#include "framework/controller/controller.h"
#include "framework/protocol/shared/tdfdecoder.h"

#include "framework/protocol/shared/encoderfactory.h"
#include "framework/protocol/shared/decoderfactory.h"

#include "EAJson/Json.h"


#include "xblclientsessiondirectory/tdf/xblclientsessiondirectory.h"

#include "EAStdC/EAString.h"
#include "EAIO/PathString.h"
#include "EAIO/EAFileDirectory.h"

// arson includes
#ifdef TARGET_arson
#include "arson/tdf/arson.h"
#include "arson/rpc/arsonslave/jsonencodetdf_stub.h"
#include "arson/rpc/arsonslave/testxml2encoder_stub.h"
#endif

#ifdef EA_PLATFORM_WINDOWS
#include <io.h>
#include <fcntl.h>
# define SET_BINARY_MODE(handle) _setmode(_fileno(handle), _O_BINARY)
#else
# define SET_BINARY_MODE(handle) ((void)0)
#endif


#define EA_TDF_GET_DEFAULT_ICOREALLOCATOR *::Blaze::Allocator::getAllocator(DEFAULT_BLAZE_MEMGROUP)

// imported from allocation.cpp
extern void initPerThreadMemTracker();
extern void shutdownPerThreadMemTracker();

namespace codecperformance
{

using namespace Blaze;
using EA::TDF::TimeValue;
using namespace Blaze::Arson;

const uint32_t SAMPLE_SIZE = 1000;

void fillJsonTdf(XBLServices::PutMultiplayerSessionLeaveGroupRequest &putRequest)
{
    putRequest.setScid("d2a649c9-5d53-4b5c-8c62-89d87d074cf6");
    putRequest.setSessionTemplateName("BlazeSample");
    putRequest.setSessionName("prodbattlefield-4-xone_30_36028797019534436");
    putRequest.getHeader().setContractVersion("104");

    for(uint32_t j = 0; j < 10; j++)
    {
        eastl::string memberName;
        const char8_t *memberIndexStr = memberName.sprintf("reserve_%u", 1).c_str();
        putRequest.getBody().getMembers()[memberIndexStr] = '\0';
    }
}

#ifdef TARGET_arson
void fillXmlTdf(Blaze::Arson::TestXml2EncoderRequest& request)
{
    //request.setNum(0);
    request.setText("mushroom");

    request.getStringMap()["1"] = "";
    request.getStringMap()["2"] = "map string 1";
    request.getStringMap()["3"] = "";
    request.getStringMap()[""] = "";
    request.getStringList().push_back("");
    request.getStringList().push_back("list string 1");
    request.getStringList().push_back("");

    Blaze::Arson::MyUnion* u2 = request.getUnionList().pull_back();
    u2->setInt(0);
    Blaze::Arson::MyUnion* u3 = request.getUnionList().pull_back();
    u3->setString("");
    Blaze::Arson::MyUnion* u4 = request.getUnionList().pull_back();
    {
        Bar b;
        b.setBarInt(0);
        b.getBarFoo().setFooInt(0);
        u4->setBar(&b);
    }
    Blaze::Arson::MyUnion* u7 = &request.getUnion3();

    Blaze::Arson::Bar b;
    b.setBarInt(15);
    b.getBarFoo().setFooInt(23);
    u7->setBar(&b);

    Blaze::Arson::Nested* nested;
    nested = request.getNestedMap().allocate_element();
    request.getNestedMap()["nested item"] = nested;
    nested->getStringList().push_back("");
    nested->getStringList().push_back("list string 2");
    nested->getStringList().push_back("");

    for(int i = 0; i < 100; i++)
    {
        char8_t idString[32];
        EA::StdC::Snprintf(idString, sizeof(idString), "%d", i);
        nested->getStringMap()[idString] = "test";
    }

    nested = request.getNestedList().pull_back();
    nested->getStringList().push_back("");
    nested->getStringList().push_back("list string 2.1");
    nested->getStringList().push_back("");
    nested->getStringMap()["1.1"] = "";
    nested->getStringMap()["2.1"] = "map string 2.1";
    nested->getStringMap()["3.1"] = "";

    ListOfListOfInt32* listOfList1 = request.getLoLoInt32List().pull_back();
    ListOfListOfInt32* listOfList2 = request.getLoLoInt32List().pull_back();
    typedef EA::TDF::TdfPrimitiveVector<int32_t> ListOfInt32;
    ListOfInt32* list0 = listOfList1->pull_back();
    list0->push_back(0);
    list0->push_back(7);
    ListOfInt32* list2 = listOfList2->pull_back();
    list2->push_back(22);

    ListOfInt32* intList1 = request.getLoLoInt32().pull_back();
    intList1->push_back(888);
    ListOfInt32* intList2 = request.getLoLoInt32().pull_back();
    intList2->push_back(999);
}
#endif

void encodeWith(EA::TDF::Tdf& tdf, Encoder::Type type)
{
    RpcProtocol::Frame frame;
    bool result = false;
    RawBuffer buf(4096);
    Blaze::TdfEncoder* encoder = static_cast<Blaze::TdfEncoder*>(EncoderFactory::create(type));
    double startTime = ((double)TimeValue::getTimeOfDay().getMicroSeconds())/1000000;
    for(uint32_t k = 0; k < SAMPLE_SIZE; k++)
    {
        result = encoder->encode(buf, tdf, &frame);
        buf.reset();
    }
    double endTime = ((double)TimeValue::getTimeOfDay().getMicroSeconds())/1000000;
    if (result)
        printf("The %s encoder calculated time is: %f\n", encoder->getName(), endTime - startTime);

    delete encoder;
}

void decodeWith(RawBuffer& buf, Decoder::Type type)
{
    BlazeRpcError decodeResult = Blaze::ERR_SYSTEM;
    Blaze::TdfDecoder* decoder = static_cast<Blaze::TdfDecoder*>(DecoderFactory::create(type));
    double startTime = ((double)TimeValue::getTimeOfDay().getMicroSeconds())/1000000;
    //for(uint32_t k = 0; k < SAMPLE_SIZE; k++)
    {
#ifdef TARGET_arson
        Blaze::Arson::TestXml2EncoderRequest tdf;
        decodeResult = decoder->decode(buf, tdf);

        RpcProtocol::Frame frame;
        RawBuffer buf1(4096);
        Blaze::TdfEncoder* encoder = static_cast<Blaze::TdfEncoder*>(EncoderFactory::create(Encoder::JSON));
        encoder->encode(buf1, tdf, &frame);
        RawBuffer buf2(4096);
        Blaze::TdfEncoder* encoder2 = static_cast<Blaze::TdfEncoder*>(EncoderFactory::create(Encoder::JSON));
        encoder2->encode(buf2, tdf, &frame);


        buf.resetToMark();
#endif
    }
    double endTime = ((double)TimeValue::getTimeOfDay().getMicroSeconds())/1000000;
    if (decodeResult == Blaze::ERR_OK)
        printf("The %s decoder calculated time is: %f\n", decoder->getName(), endTime - startTime);

    delete decoder;
}

int codecperformance_main(int argc, char** argv)
{
    initPerThreadMemTracker();
    
    eastl::string codecFormat = argv[1];
    eastl::string codecType = argv[2];    
#ifdef TARGET_arson        
    if (codecType == "encoder")
    {
        if (codecFormat == "XML")
        {
            Blaze::Arson::TestXml2EncoderRequest tdf;
            fillXmlTdf(tdf);
            encodeWith(tdf, Encoder::XML2);
        }
        else if (codecFormat == "JSON")
        {
            XBLServices::PutMultiplayerSessionLeaveGroupRequest tdf;
            fillJsonTdf(tdf);
            encodeWith(tdf, Encoder::JSON);
        }
    }
    else if (codecType == "decoder")
    {
        if (codecFormat == "XML")
        {
            RawBuffer buf(4096);
            Blaze::Arson::TestXml2EncoderRequest tdf;
            fillXmlTdf(tdf);
            RpcProtocol::Frame frame;
            Blaze::TdfEncoder* encoder = static_cast<Blaze::TdfEncoder*>(EncoderFactory::create(Encoder::XML2)); // construct the encoding format
            encoder->encode(buf, tdf, &frame);
            delete encoder;

            decodeWith(buf, Decoder::XML2);
        }
        else if (codecFormat == "JSON")
        {
            RawBuffer buf(4096);
            Blaze::Arson::TestXml2EncoderRequest tdf;
            fillXmlTdf(tdf);
            RpcProtocol::Frame frame;
            Blaze::TdfEncoder* encoder = static_cast<Blaze::TdfEncoder*>(EncoderFactory::create(Encoder::JSON)); // construct the encoding format
            encoder->encode(buf, tdf, &frame);
            delete encoder;

            decodeWith(buf, Decoder::JSON);
        }
    }
#endif
    shutdownPerThreadMemTracker();

    return 0;
}

}

