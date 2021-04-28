/*************************************************************************************************/
/*!
\file metricstest.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVER_TEST_METRICS_TEST_H
#define BLAZE_SERVER_TEST_METRICS_TEST_H

#include <gtest/gtest.h>

#include "framework/blaze.h"
#include "framework/metrics/metrics.h"

namespace Blaze
{
namespace Metrics
{
    namespace Tag
    {
        extern TagInfo<const char*>* tag1;
        extern TagInfo<const char*>* tag2;
        extern TagInfo<const char*>* tag3;
    }
}
}

namespace BlazeServerTest
{
namespace Metrics
{

    // The fixture for testing class.
    class MetricsTest : public testing::Test
    {

    protected:
        // You can remove any or all of the following functions if its body
        // is empty.

        MetricsTest()
        {

        }

        ~MetricsTest() override
        {

        }

        // If the constructor and destructor are not enough for setting up
        // and cleaning up each test, you can define the following methods:

        void SetUp() override
        {
            // Code here will be called immediately after the constructor (right
            // before each test).
        }

        void TearDown() override
        {
            // Code here will be called immediately after each test (right
            // before the destructor).
        }

        // Objects declared here can be used by all tests in the test case for MetricsTest.

    };

    // A subclass fixture for death tests. Google Test treats suites with a name
    // ending in "DeathTest" specially.
    class MetricsDeathTest : public MetricsTest
    {

    };



} // namespace metrics
}  // namespace test

#endif