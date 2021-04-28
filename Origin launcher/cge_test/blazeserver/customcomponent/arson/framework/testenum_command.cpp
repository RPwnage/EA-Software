/*************************************************************************************************/
/*!
    \file   testenum_commands.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"

#include "arson/tdf/arson.h"
#include "arson/rpc/arsonslave/testenum_stub.h"
#include "arsonslaveimpl.h"

namespace Blaze
{
namespace Arson
{

////////////////////////////////////////////////////////
class TestEnumCommand : public TestEnumCommandStub
{
public:
    TestEnumCommand(Message* message, TestEnumRequest* request, ArsonSlaveImpl* componentImpl)
        : TestEnumCommandStub(message, request), mComponent(componentImpl)
    { }

    //virtual ~TestEnumCommand() { }

    /* Private methods *******************************************************************************/
private:

    TestEnumCommandStub::Errors execute() override
    {
        mResponse.setTestEnumResp(mRequest.getTestEnum());
        
        return ERR_OK;
    }

private:
    // Not owned memory.
    ArsonSlaveImpl* mComponent;
};

// static factory method impl
DEFINE_TESTENUM_CREATE()

} // Arson
} // Blaze
