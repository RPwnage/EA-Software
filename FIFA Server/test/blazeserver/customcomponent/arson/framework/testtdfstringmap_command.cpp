/*************************************************************************************************/
/*!
    \file   TestTdfStringMapCommand_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/testtdfstringmap_stub.h"

namespace Blaze
{
namespace Arson
{
class TestTdfStringMapCommand : public TestTdfStringMapCommandStub
{
public:
    TestTdfStringMapCommand(
        Message* message, ArsonSlaveImpl* componentImpl)
        : TestTdfStringMapCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~TestTdfStringMapCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    TestTdfStringMapCommandStub::Errors execute() override
    {
        // Testing for GOS-25474: Fix cases where copy sets strings without initializing the buffer ptr
        TdfStringMap::StringsMap strMap;
        strMap["fred"] = "bob";
        EA::TDF::TdfGenericReferenceConst value;
        value.setRef(strMap);

        TdfStringMap strMap2;
        strMap2.setValueByName("strings", value);

        TdfStringMap::StringsMap::const_iterator iter = strMap2.getStrings().find("fred");

        if ( iter != strMap2.getStrings().end() )
        {
            if ( blaze_strcmp(iter->second.c_str(), "bob") == 0 )
            {
                return ERR_OK;
            }
        }

        return ERR_SYSTEM;
    }
};

DEFINE_TESTTDFSTRINGMAP_CREATE()

} //Arson
} //Blaze
