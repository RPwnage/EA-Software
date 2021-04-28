/*************************************************************************************************/
/*!
    \file 
    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVER_TEST_PACKER_SILO_TEST_H
#define BLAZE_SERVER_TEST_PACKER_SILO_TEST_H

#include "test/common/gtest/gtestfixtureutil.h"

namespace BlazeServerTest
{
namespace GamePacker
{
    class PackerSiloTest : public testing::Test
    {
    protected:
        PackerSiloTest() {}
        ~PackerSiloTest() override {}

        void SetUp() override {}

        void TearDown() override {}

        // Objects declared here can be used by all tests in the test case for this fixture.
    };

} // namespace 
} // namespace BlazeServerTest

#endif