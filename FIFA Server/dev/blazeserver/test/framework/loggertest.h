/*************************************************************************************************/
/*!
    \file loggertest.cpp

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVER_TEST_LOGGER_TEST_H
#define BLAZE_SERVER_TEST_LOGGER_TEST_H

#include <gtest/gtest.h>

namespace BlazeServerTest
{
namespace Framework
{
    class LoggerTest : public testing::Test
    {
    public:
        LoggerTest()
        {
        }

        ~LoggerTest() override
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
