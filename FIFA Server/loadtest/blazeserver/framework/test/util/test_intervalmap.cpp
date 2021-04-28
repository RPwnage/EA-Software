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
#include "framework/util/intervalmap.h"
#include "framework/util/shared/blazestring.h"
#include "framework/test/blazeunittest.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
using namespace Blaze;

/*** Test Classes ********************************************************************************/
class TestIntervalMap
{
public:

    void TestSimpleAllocateAndRelease()
    {
        IntervalMap imap;
        char8_t buf[1024];

        // Allocate 6 identifiers
        BUT_ASSERT(imap.allocate() == 0);
        BUT_ASSERT(imap.allocate() == 1);
        BUT_ASSERT(imap.allocate() == 2);
        BUT_ASSERT(imap.allocate() == 3);
        BUT_ASSERT(imap.allocate() == 4);
        BUT_ASSERT(imap.allocate() == 5);

        // Make sure the intervals are okay
        imap.print(buf, sizeof(buf));
        BUT_ASSERT_MSG(buf, strcmp(buf, "{0-5}") == 0);

        // Release the first id
        BUT_ASSERT(imap.release(0));
        imap.print(buf, sizeof(buf));
        BUT_ASSERT_MSG(buf, strcmp(buf, "{1-5}") == 0);

        // Release the last id
        BUT_ASSERT(imap.release(5));
        imap.print(buf, sizeof(buf));
        BUT_ASSERT_MSG(buf, strcmp(buf, "[1-4]{6}") == 0);

        // Release an id in the middle
        BUT_ASSERT(imap.release(3));
        imap.print(buf, sizeof(buf));
        BUT_ASSERT_MSG(buf, strcmp(buf, "[1-2][4-4]{6}") == 0);
    }

    void TestOneAllocateAndOneRelease()
    {
        IntervalMap imap;
        char8_t buf[1024];

        // Allocate a single identifier
        BUT_ASSERT(imap.allocate() == 0);

        // Make sure the intervals are okay
        imap.print(buf, sizeof(buf));
        BUT_ASSERT_MSG(buf, strcmp(buf, "{0-0}") == 0);

        // Release the first id
        BUT_ASSERT(imap.release(0));
        imap.print(buf, sizeof(buf));
        BUT_ASSERT_MSG(buf, strcmp(buf, "{1}") == 0);

        BUT_ASSERT(imap.allocate() == 1);
        imap.print(buf, sizeof(buf));
        BUT_ASSERT_MSG(buf, strcmp(buf, "{1-1}") == 0);
    }

    void TestReserveConstructor()
    {
        char8_t buf[1024];

        IntervalMap imap0(0, 100);
        imap0.print(buf, sizeof(buf));
        BUT_ASSERT_MSG(buf, strcmp(buf, "{0}") == 0);

        IntervalMap imap1(800, 1000);
        imap1.print(buf, sizeof(buf));
        BUT_ASSERT_MSG(buf, strcmp(buf, "{800}") == 0);
        
        IntervalMap imap2(1000, 2147483647);
        imap2.print(buf, sizeof(buf));
        BUT_ASSERT_MSG(buf, strcmp(buf, "{1000}") == 0);
    }

    void TestFull()
    {
        IntervalMap imap(0,5);

        // Fill up the map
        BUT_ASSERT(imap.allocate() == 0);
        BUT_ASSERT(imap.allocate() == 1);
        BUT_ASSERT(imap.allocate() == 2);
        BUT_ASSERT(imap.allocate() == 3);
        BUT_ASSERT(imap.allocate() == 4);
        BUT_ASSERT(imap.allocate() == 5);

        // No more room
        BUT_ASSERT(imap.allocate() == -1);
        BUT_ASSERT(imap.allocate() == -1);

        // Release an element in the middle
        BUT_ASSERT(imap.release(3));

        // Consume the released element
        BUT_ASSERT(imap.allocate() == 3);

        // Ensure we're full again
        BUT_ASSERT(imap.allocate() == -1);

        // Release an element at the head
        BUT_ASSERT(imap.release(0));

        // Consume the release element
        BUT_ASSERT(imap.allocate() == 0);

        // Ensure we're full again
        BUT_ASSERT(imap.allocate() == -1);

        // Release the first two elements
        BUT_ASSERT(imap.release(0));
        BUT_ASSERT(imap.release(1));

        char8_t buf[1024];
        imap.print(buf, sizeof(buf));
        BUT_ASSERT(imap.allocate() == 0);
        imap.print(buf, sizeof(buf));
        BUT_ASSERT(imap.allocate() == 1);
        imap.print(buf, sizeof(buf));
        BUT_ASSERT(imap.allocate() == -1);
    }

    void TestAddAddRemoveAddWithReservedRegion()
    {
        IntervalMap imap(0, 20);

        // Allocate some ids and then give one back and ensure that it isn't reused until all the
        // other ids have been used first
        BUT_ASSERT(imap.allocate() == 0);
        BUT_ASSERT(imap.allocate() == 1);
        BUT_ASSERT(imap.release(0));
        BUT_ASSERT(imap.allocate() == 2);
    }

    void TestAddRemoveAddWithReservedRegion()
    {
        IntervalMap imap(0, 20);

        // Allocate some ids and then give one back and ensure that it isn't reused until all the
        // other ids have been used first
        BUT_ASSERT(imap.allocate() == 0);
        BUT_ASSERT(imap.release(0));
        BUT_ASSERT(imap.allocate() == 1);
    }

    void TestFragmentation()
    {
        char8_t buf[1024];
        IntervalMap imap(0,20);

        // Fill up the map
        for(int32_t i = 0; i < 18; ++i)
        {
            blaze_snzprintf(buf, sizeof(buf), "%d", i);
            BUT_ASSERT_MSG(buf, imap.allocate() == i);
        }

        // Remove blocks in the middle
        BUT_ASSERT(imap.release(4));
        BUT_ASSERT(imap.release(6));
        BUT_ASSERT(imap.release(10));
        BUT_ASSERT(imap.release(15));

        // Allocate a few more to trigger a wrap around
        BUT_ASSERT(imap.allocate() == 18);
        BUT_ASSERT(imap.allocate() == 19);
        BUT_ASSERT(imap.allocate() == 20);
        BUT_ASSERT(imap.allocate() == 4);
        BUT_ASSERT(imap.allocate() == 6);

        // Remove from the head
        BUT_ASSERT(imap.release(0));

        // Add some more
        BUT_ASSERT(imap.allocate() == 10);
        BUT_ASSERT(imap.allocate() == 15);

        // Make sure we wrap around
        BUT_ASSERT(imap.allocate() == 0);

        // Make sure we're full now
        BUT_ASSERT(imap.allocate() == -1);

        // Remove the last item
        BUT_ASSERT(imap.release(20));

        // Try and re-use it
        BUT_ASSERT(imap.allocate() == 20);

        // Make sure we're full agai
        BUT_ASSERT(imap.allocate() == -1);
    }

    void TestAllocate1()
    {
        IntervalMap imap(0,5);

        BUT_ASSERT(imap.allocate() == 0);
        BUT_ASSERT(imap.allocate() == 1);
        BUT_ASSERT(imap.allocate() == 2);
        BUT_ASSERT(imap.allocate() == 3);
        BUT_ASSERT(imap.allocate() == 4);
        BUT_ASSERT(imap.allocate() == 5);
        BUT_ASSERT(imap.release(0));
        BUT_ASSERT(imap.allocate() == 0);

        BUT_ASSERT(imap.allocate() == -1);
        BUT_ASSERT(imap.release(3));
        BUT_ASSERT(imap.allocate() == 3);
    }

    void TestAllocate2()
    {
        IntervalMap imap(0,5);

        BUT_ASSERT(imap.allocate() == 0);
        BUT_ASSERT(imap.allocate() == 1);
        BUT_ASSERT(imap.allocate() == 2);
        BUT_ASSERT(imap.allocate() == 3);
        BUT_ASSERT(imap.allocate() == 4);
        BUT_ASSERT(imap.allocate() == 5);
        BUT_ASSERT(imap.release(0));
        BUT_ASSERT(imap.release(1));
        BUT_ASSERT(imap.release(2));
        BUT_ASSERT(imap.allocate() == 0);
        BUT_ASSERT(imap.allocate() == 1);
        BUT_ASSERT(imap.allocate() == 2);
        BUT_ASSERT(imap.allocate() == -1);
        BUT_ASSERT(imap.release(4));
        BUT_ASSERT(imap.allocate() == 4);
    }

    void TestAllocate3()
    {
        IntervalMap imap(0,5);

        BUT_ASSERT(imap.allocate() == 0);
        BUT_ASSERT(imap.allocate() == 1);
        BUT_ASSERT(imap.allocate() == 2);
        BUT_ASSERT(imap.allocate() == 3);
        BUT_ASSERT(imap.allocate() == 4);
        BUT_ASSERT(imap.allocate() == 5);
        BUT_ASSERT(imap.allocate() == -1);
        BUT_ASSERT(imap.release(4));
        BUT_ASSERT(imap.allocate() == 4);
    }

    void TestAllocate4()
    {
        IntervalMap imap(0,5);

        BUT_ASSERT(imap.allocate() == 0);
        BUT_ASSERT(imap.allocate() == 1);
        BUT_ASSERT(imap.allocate() == 2);
        BUT_ASSERT(imap.allocate() == 3);
        BUT_ASSERT(imap.allocate() == 4);
        BUT_ASSERT(imap.allocate() == 5);
        BUT_ASSERT(imap.release(3));
        BUT_ASSERT(imap.release(4));
        BUT_ASSERT(imap.allocate() == 3);
        BUT_ASSERT(imap.allocate() == 4);
        BUT_ASSERT(imap.allocate() == -1);
    }

    void TestAllocate5()
    {
        char8_t buf[1024];
        IntervalMap imap(0,5);

        BUT_ASSERT(imap.allocate() == 0);
        BUT_ASSERT(imap.allocate() == 1);
        BUT_ASSERT(imap.allocate() == 2);
        BUT_ASSERT(imap.allocate() == 3);
        BUT_ASSERT(imap.allocate() == 4);
        BUT_ASSERT(imap.allocate() == 5);
        BUT_ASSERT(imap.release(3));
        imap.print(buf, sizeof(buf));
        BUT_ASSERT(imap.allocate() == 3);
        BUT_ASSERT(imap.allocate() == -1);
    }

    void TestRelease1()
    {
        IntervalMap imap(10,20);

        BUT_ASSERT(imap.allocate() == 10);
        BUT_ASSERT(imap.allocate() == 11);
        BUT_ASSERT(imap.allocate() == 12);

        BUT_ASSERT(!imap.release(5));
        BUT_ASSERT(!imap.release(9));
        BUT_ASSERT(!imap.release(21));
        BUT_ASSERT(!imap.release(30));

        BUT_ASSERT(imap.release(10));
        BUT_ASSERT(imap.release(11));
        BUT_ASSERT(imap.release(12));
    }

    void TestRelease2()
    {
        IntervalMap imap(10,20);

        BUT_ASSERT(imap.allocate() == 10);
        BUT_ASSERT(imap.allocate() == 11);
        BUT_ASSERT(imap.allocate() == 12);
        BUT_ASSERT(imap.allocate() == 13);
        BUT_ASSERT(imap.allocate() == 14);

        BUT_ASSERT(imap.release(10));
        BUT_ASSERT(imap.release(11));

        BUT_ASSERT(!imap.release(10));
        BUT_ASSERT(!imap.release(11));
    }

    void TestRelease3()
    {
        IntervalMap imap(10,20);

        BUT_ASSERT(imap.allocate() == 10);
        BUT_ASSERT(imap.allocate() == 11);
        BUT_ASSERT(imap.allocate() == 12);
        BUT_ASSERT(imap.allocate() == 13);
        BUT_ASSERT(imap.allocate() == 14);
        BUT_ASSERT(imap.allocate() == 15);

        BUT_ASSERT(imap.release(12));
        BUT_ASSERT(imap.release(13));

        BUT_ASSERT(imap.release(14));
        BUT_ASSERT(imap.release(15));

        BUT_ASSERT(imap.allocate() == 16);
    }

    void TestRelease4()
    {
        IntervalMap imap(0,5);

        BUT_ASSERT(imap.allocate() == 0);
        BUT_ASSERT(imap.allocate() == 1);
        BUT_ASSERT(imap.allocate() == 2);
        BUT_ASSERT(imap.allocate() == 3);
        BUT_ASSERT(imap.allocate() == 4);
        BUT_ASSERT(imap.allocate() == 5);

        BUT_ASSERT(imap.release(5));
        BUT_ASSERT(imap.allocate() == 5);
        BUT_ASSERT(imap.allocate() == -1);
        BUT_ASSERT(imap.release(5));
        BUT_ASSERT(imap.allocate() == 5);
        BUT_ASSERT(imap.allocate() == -1);
    }

    void TestRelease5()
    {
        IntervalMap imap(0,10);

        BUT_ASSERT(imap.allocate() == 0);
        BUT_ASSERT(imap.allocate() == 1);
        BUT_ASSERT(imap.allocate() == 2);
        BUT_ASSERT(imap.allocate() == 3);
        BUT_ASSERT(imap.allocate() == 4);
        BUT_ASSERT(imap.allocate() == 5);

        BUT_ASSERT(imap.release(5));

        BUT_ASSERT(imap.allocate() == 6);
        BUT_ASSERT(imap.allocate() == 7);
        BUT_ASSERT(imap.allocate() == 8);
    }

    void TestRelease6()
    {
        IntervalMap imap(0,10);

        BUT_ASSERT(imap.allocate() == 0);
        BUT_ASSERT(imap.allocate() == 1);
        BUT_ASSERT(imap.allocate() == 2);
        BUT_ASSERT(imap.allocate() == 3);
        BUT_ASSERT(imap.allocate() == 4);
        BUT_ASSERT(imap.allocate() == 5);
        BUT_ASSERT(imap.allocate() == 6);
        BUT_ASSERT(imap.allocate() == 7);

        BUT_ASSERT(imap.release(3));

        BUT_ASSERT(imap.allocate() == 8);
        BUT_ASSERT(imap.allocate() == 9);
        BUT_ASSERT(imap.allocate() == 10);
        BUT_ASSERT(imap.allocate() == 3);
        BUT_ASSERT(imap.allocate() == -1);
    }

    void TestDaveMScenario1()
    {
        IntervalMap imap(0,10);

        BUT_ASSERT(imap.allocate() == 0);
        BUT_ASSERT(imap.allocate() == 1);
        BUT_ASSERT(imap.allocate() == 2);

        BUT_ASSERT(imap.release(1));
        BUT_ASSERT(imap.release(0));

        char8_t buf[1024];
        imap.print(buf, sizeof(buf));
        BUT_ASSERT_MSG(buf, strcmp(buf, "{2-2}") == 0);
    }

    void TestDaveMScenario2()
    {
        IntervalMap imap(0,10);

        BUT_ASSERT(imap.allocate() == 0);
        BUT_ASSERT(imap.allocate() == 1);
        BUT_ASSERT(imap.allocate() == 2);
        BUT_ASSERT(imap.allocate() == 3);
        BUT_ASSERT(imap.allocate() == 4);
        BUT_ASSERT(imap.allocate() == 5);
        BUT_ASSERT(imap.allocate() == 6);
        BUT_ASSERT(imap.allocate() == 7);
        BUT_ASSERT(imap.allocate() == 8);
        BUT_ASSERT(imap.allocate() == 9);

        BUT_ASSERT(imap.release(3));
        BUT_ASSERT(imap.release(4));
        BUT_ASSERT(imap.release(5));

        BUT_ASSERT(imap.allocate() == 10);
        BUT_ASSERT(imap.release(10));

        char8_t buf[1024];
        imap.print(buf, sizeof(buf));
        BUT_ASSERT_MSG(buf, strcmp(buf, "{0-2}[6-9]") == 0);

        BUT_ASSERT(imap.allocate() == 3);
    }

};



// The above class(es) used to be instantiated in the context of a unit test framework.
// This framework not being used anymore, we now instantiate them in a dedicated main()
// Martin Clouatre - April 2009
int main(void)
{
    std::cout << "** BLAZE UNIT TEST SUITE **\n";
    std::cout << "test name: TEST_INTERVALMAP\n";
    std::cout << "\nPress <enter> to exit...\n\n\n";
    
    TestIntervalMap testIntervalMap;
    testIntervalMap.TestSimpleAllocateAndRelease();
    testIntervalMap.TestOneAllocateAndOneRelease();
    testIntervalMap.TestReserveConstructor();
    testIntervalMap.TestFull();
    testIntervalMap.TestAddAddRemoveAddWithReservedRegion();
    testIntervalMap.TestAddRemoveAddWithReservedRegion();
    testIntervalMap.TestFragmentation();
    testIntervalMap.TestAllocate1();
    testIntervalMap.TestAllocate2();
    testIntervalMap.TestAllocate3();
    testIntervalMap.TestAllocate4();
    testIntervalMap.TestRelease1();
    testIntervalMap.TestRelease2();
    testIntervalMap.TestRelease3();
    testIntervalMap.TestRelease4();
    testIntervalMap.TestRelease5();
    testIntervalMap.TestRelease6();   
    testIntervalMap.TestDaveMScenario1();
    testIntervalMap.TestDaveMScenario2(); 
    
    //Wait for user to hit enter before quitting
    char8_t ch = 0;
    while (!std::cin.get(ch)) {}
}

