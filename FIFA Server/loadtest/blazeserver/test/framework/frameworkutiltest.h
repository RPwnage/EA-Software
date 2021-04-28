/*************************************************************************************************/
/*!
\file frameworkutiltest.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVER_TEST_FRAMEWORK_UTIL_TEST_H
#define BLAZE_SERVER_TEST_FRAMEWORK_UTIL_TEST_H

#include <gtest/gtest.h>

namespace BlazeServerTest
{
namespace Framework
{
    class FrameworkUtilTest : public testing::Test
    {
    public:
        FrameworkUtilTest()
        {
        }

        ~FrameworkUtilTest() override
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

} //namespace Framework
} //namespace BlazeServerTest

#endif
