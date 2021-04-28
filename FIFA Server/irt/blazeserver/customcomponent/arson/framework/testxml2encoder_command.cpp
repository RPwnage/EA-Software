/*************************************************************************************************/
/*!
    \file   testxml2encoder_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/protocol/shared/encoderfactory.h"
#include "framework/protocol/xml2encoder.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/testxml2encoder_stub.h"

namespace Blaze
{
namespace Arson
{
class TestXml2EncoderCommand : public TestXml2EncoderCommandStub
{
public:
    TestXml2EncoderCommand(
        Message* message, ArsonSlaveImpl* componentImpl)
        : TestXml2EncoderCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~TestXml2EncoderCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    TestXml2EncoderCommandStub::Errors execute() override
    {
        TestXml2EncoderRequest request;

        TdfEncoder* encoder = (TdfEncoder*)EncoderFactory::createDefaultDifferenceEncoder(Encoder::XML2);
        
        request.getStringMap()["1"] = "";
        request.getStringMap()["2"] = "map string 1";
        request.getStringMap()["3"] = "";
        request.getStringMap()[""] = "";
        request.getStringList().push_back("");
        request.getStringList().push_back("list string 1");
        request.getStringList().push_back("");
        
        MyUnion* u = request.getUnionList().pull_back();
        u = request.getUnionList().pull_back();
        u->setInt(0);
        u = request.getUnionList().pull_back();
        u->setString("");
        u = request.getUnionList().pull_back();
        {
            Bar b;
            b.setBarInt(0);
            b.getBarFoo().setFooInt(0);
            u->setBar(&b);
        }
        
        u = &request.getUnion1();
        u->setInt(0);
        u = &request.getUnion2();
        u->setString("");
        u = &request.getUnion3();
        {
            Bar b;
            b.setBarInt(0);
            b.getBarFoo().setFooInt(0);
            u->setBar(&b);
        }
        
        Nested* nested = request.getNestedMap().allocate_element();
        request.getNestedMap()["nested item"] = nested;
        nested->getStringList().push_back("");
        nested->getStringList().push_back("list string 2");
        nested->getStringList().push_back("");
        nested->getStringMap()["1"] = "";
        nested->getStringMap()["2"] = "map string 2";
        nested->getStringMap()["3"] = "";
        
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
        
        RpcProtocol::Frame frame;
        frame.xmlResponseFormat = request.getUseAttributes() ? RpcProtocol::XML_RESPONSE_FORMAT_ATTRIBUTES : RpcProtocol::XML_RESPONSE_FORMAT_ELEMENTS;
        RawBuffer rb(0);
        encoder->encode(rb, request, &frame, false);

        delete encoder;

        return ERR_OK;
    }
};

DEFINE_TESTXML2ENCODER_CREATE()

} //Arson
} //Blaze
