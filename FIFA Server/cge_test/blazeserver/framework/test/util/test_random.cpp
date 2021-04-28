/*************************************************************************************************/
/*!
    \file test_random.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include <stdio.h>
#include <time.h>
#include "framework/blaze.h"
#include "framework/util/random.h"
#include "framework/test/blazeunittest.h"
#include "time.h"

class TestRandom
{
public:
    TestRandom() {}
    
    // two things going on here:
    // 1) check that getRandomNumber's range is correct (inclusive between 0 & range-1)
    // 2) a quick 'n dirty check that the distribution is normal (ish)
    void testRandomRange()
    {
        const int32_t range = 100;

        int32_t resultCount[range];
        memset(resultCount, 0, range * sizeof(int32_t)); // zero the array

        srand((uint32_t)Blaze::TimeValue::getTimeOfDay().getMillis());

        const int32_t numLoops = 10000000; // 10 mil
        for( int32_t i=0; i<numLoops; ++i )
        {
            int32_t randomNum = Blaze::Random::getRandomNumber(range);
            if (randomNum < 0 || randomNum >= range)
            {
                printf("Range Error: expected random number to be within range [0..%d), but is actually %d!\n", range, randomNum);
                BUT_ASSERT(false);
                return;
            }

            resultCount[randomNum] += 1;
        }

        // now check the distribution; we expect each entry in the range to have roughly the same percentage.
        // for 100 slots, we expect each one to be around 1%
        for (int32_t i=0; i<range; ++i)
        {
            float p = resultCount[i] / (float)numLoops;
            //printf("resultCount[%d] : %d -> p = %f\n", i, resultCount[i], p);
            if (p < 0.0095f || p > 0.0105f)
            {
                // TODO: On Windows this test is currently failing to produce a even distribution for index 99.
                // Distribution Problem: distribution for index 99 is outside +/-5% of expected value of 1%. (p = 0.007891)
                printf("  Distribution Problem: distribution for index %d is outside +/-5%% of expected value of 1%%. (p = %f)\n", i, p);
                BUT_ASSERT( p > 0.0095f);
                BUT_ASSERT( p < 0.0105f);
            }
        }
    }

    // Same test as above but for smaller range of numbers.
    // 1) check that getRandomNumber's range is correct (inclusive between 0 & range-1)
    // 2) a quick 'n dirty check that the distribution is normal (ish)
    void testSmallRandomRange()
    {
        const int32_t range = 3;

        int32_t resultCount[range];
        memset(resultCount, 0, range * sizeof(int32_t)); // zero the array

        srand((uint32_t)Blaze::TimeValue::getTimeOfDay().getMillis());

        const int32_t numLoops = 10000000; // 10 mil
        for( int32_t i=0; i<numLoops; ++i )
        {
            int32_t randomNum = Blaze::Random::getRandomNumber(range);
            if (randomNum < 0 || randomNum >= range)
            {
                printf("Range Error: expected random number to be within range [0..%d), but is actually %d!\n", range, randomNum);
                BUT_ASSERT(false);
                return;
            }

            resultCount[randomNum] += 1;
        }

        const float EXPECTED_PERCENT = 0.3333f;
        const float VARIANCE = 0.0010f;
        const float UPPER_BOUND = EXPECTED_PERCENT + VARIANCE;
        const float LOWER_BOUND = EXPECTED_PERCENT - VARIANCE;
        // now check the distribution; we expect each entry in the range to have roughly the same percentage.
        // for 3 slots, we expect each one to be around 33.3333%
        for (int32_t i=0; i<range; ++i)
        {
            float p = resultCount[i] / (float)numLoops;
            //printf("resultCount[%d] : %d -> p = %f\n", i, resultCount[i], p);
            if (p < LOWER_BOUND || p > UPPER_BOUND)
            {
                printf("  Distribution Problem: distribution for index %d is outside +/-10%% of expected value of 30%%. (p = %f)\n", i, p);
                BUT_ASSERT( p > LOWER_BOUND);
                BUT_ASSERT( p < UPPER_BOUND);
            }
        }
    }

};


// The above class(es) used to be instantiated in the context of a unit test framework.
// This framework not being used anymore, we now instantiate them in a dedicated main()
// Martin Clouatre - April 2009
int main(void)
{
    std::cout << "** BLAZE UNIT TEST SUITE **\n";
    std::cout << "test name: TEST_RANDOM\n";
    std::cout << "\nPress <enter> to exit...\n\n\n";
    
    TestRandom testRandom;
    testRandom.testRandomRange();
    testRandom.testSmallRandomRange();
    
    //Wait for user to hit enter before quitting
    char8_t ch = 0;
    while (!std::cin.get(ch)) {}
}

