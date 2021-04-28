/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "framework/blaze.h"
#include "framework/connection/inetaddress.h"
#include "framework/test/blazeunittest.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
using namespace Blaze;

#define TEST_ADDR1 "10.20.30.40:123"
#define TEST_ADDR2 "12.34.56.78:45678"


/*** Test Classes ********************************************************************************/
class TestAddress
{
public:
    void testInetCopyConstructor()
    {
        char buf[64];
        InetAddress* addr1 = new InetAddress(TEST_ADDR1);
        BUT_ASSERT(strcmp(addr1->toString(buf, sizeof(buf)), TEST_ADDR1) == 0);

        InetAddress* addr2(addr1);
        BUT_ASSERT(strcmp(addr2->toString(buf, sizeof(buf)), TEST_ADDR1) == 0);
    }

    void testInetAssignment()
    {
        char buf[64];
        InetAddress* addr1 = new InetAddress(TEST_ADDR1);
        BUT_ASSERT(strcmp(addr1->toString(buf, sizeof(buf)), TEST_ADDR1) == 0);

        InetAddress* addr2 = new InetAddress(TEST_ADDR2);
        BUT_ASSERT(strcmp(addr2->toString(buf, sizeof(buf)), TEST_ADDR2) == 0);

        *addr2 = *addr1;
        BUT_ASSERT(strcmp(addr2->toString(buf, sizeof(buf)), TEST_ADDR1) == 0);
    }

    void testInetAddress()
    {
        int32_t x = 2;
        BUT_ASSERT(x == 2);
    }
};


// The above class(es) used to be instantiated in the context of a unit test framework.
// This framework not being used anymore, we now instantiate them in a dedicated main()
// Martin Clouatre - April 2009
int main(void)
{
    std::cout << "** BLAZE UNIT TEST SUITE **\n";
    std::cout << "test name: TEST_ADDRESS\n";
    std::cout << "\nPress <enter> to exit...\n\n\n";
    
    TestAddress testAddress;
    testAddress.testInetCopyConstructor();
    testAddress.testInetAssignment();
    testAddress.testInetAddress();
    
    //Wait for user to hit enter before quitting
    char8_t ch = 0;
    while (!std::cin.get(ch)) {}
}

