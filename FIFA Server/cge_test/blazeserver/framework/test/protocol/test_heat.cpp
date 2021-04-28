/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include <stdio.h>
#include <time.h>
#include "framework/blaze.h"
#include "framework/protocol/shared/heatencoder.h"
#include "framework/protocol/shared/heatdecoder.h"
#include "framework/protocol/shared/heatutil.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/test/blazeunittest.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
using namespace Blaze;

/*** Test Classes ********************************************************************************/
class TestHeat
{
public:
    typedef TdfString String;

    TestHeat()
    {
    }

    void testIncompleteUnionDecode4()
    {
    }
};



// The above class(es) used to be instantiated in the context of a unit test framework.
// This framework not being used anymore, we now instantiate them in a dedicated main()
// Martin Clouatre - April 2009
int main(void)
{
    std::cout << "** BLAZE UNIT TEST SUITE **\n";
    std::cout << "test name: TEST_HEAT\n";
    std::cout << "\nPress <enter> to exit...\n\n\n";
    
    TestHeat testHeat;
    testHeat.testIncompleteUnionDecode4();
    
    //Wait for user to hit enter before quitting
    char8_t ch = 0;
    while (!std::cin.get(ch)) {}
}

