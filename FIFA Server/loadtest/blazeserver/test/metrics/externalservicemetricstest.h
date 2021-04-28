/*************************************************************************************************/
/*!
\file externalservicemetricstest.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVER_TEST_EXTERNAL_SERVICE_METRICS_TEST_H
#define BLAZE_SERVER_TEST_EXTERNAL_SERVICE_METRICS_TEST_H

#include <gtest/gtest.h>

namespace BlazeServerTest
{
    namespace Metrics
    {
        class ExternalServiceMetricsTest : public testing::Test
        {
        public:
            ExternalServiceMetricsTest()
            {
            }

            ~ExternalServiceMetricsTest() override
            {
            }

        protected:
            void SetUp() override
            {
            }

            void TearDown() override
            {
            }

        };
        /*
        // A subclass fixture for death tests. Google Test treats suites with a name
        // ending in "DeathTest" specially.
        class ExternalServiceMetricsDeathTest : public ExternalServiceMetricsTest
        {

        };
        */
    } //namespace Metrics
} //namespace BlazeServerTest

#endif
