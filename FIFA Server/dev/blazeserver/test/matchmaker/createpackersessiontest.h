/*************************************************************************************************/
/*!
    \file createpackersessiontest.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVER_TEST_CREATE_PACKER_SESSION_TEST_H
#define BLAZE_SERVER_TEST_CREATE_PACKER_SESSION_TEST_H

#include "test/matchmaker/mmtestfixtureutil.h"

namespace BlazeServerTest
{
namespace Matchmaker
{
    class MockMatchmakerSlaveImpl;

    class CreatePackerSessionTest : public testing::Test, public MMTestFixtureUtil
    {
    protected:
        CreatePackerSessionTest();
        ~CreatePackerSessionTest() override;

        void SetUp() override
        {
        }

        void TearDown() override
        {
        }

        // Objects declared here can be used by all tests in the test case for this fixture.
    };

} // namespace Matchmaker
} // namespace BlazeServerTest

#endif