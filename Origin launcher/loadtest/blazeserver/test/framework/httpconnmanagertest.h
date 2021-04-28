/*************************************************************************************************/
/*!
    \file httpconnmanagertest.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVER_TEST_HTTP_CONN_MANAGER_TEST_H
#define BLAZE_SERVER_TEST_HTTP_CONN_MANAGER_TEST_H

#include "test/common/gtest/gtestfixtureutil.h"

namespace BlazeServerTest
{
namespace Framework
{
    class HttpConnManagerTest : public testing::Test, public TestingUtils::GtestFixtureUtil
    {
    public:
        HttpConnManagerTest(){}
        ~HttpConnManagerTest() override {}

        void SetUp() override
        {
        }

        void TearDown() override
        {
        }

        // Objects declared here can be used by all tests in the test case for HttpConnManagerTest.
    };

} // namespace
} // namespace

#endif