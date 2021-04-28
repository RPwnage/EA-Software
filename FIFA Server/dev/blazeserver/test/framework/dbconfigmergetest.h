/*************************************************************************************************/
/*!
\file dbconfigmergetest.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVER_TEST_DBCONFIG_MERGE_TEST_H
#define BLAZE_SERVER_TEST_DBCONFIG_MERGE_TEST_H


#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test/mock/mocklogger.h"



namespace BlazeServerTest
{

namespace frameworktest
{
    class DbConfigMergeTest : public testing::Test
    {

    protected:
        DbConfigMergeTest();

        ~DbConfigMergeTest() override;

        // gTest functions

        // Code here will be called immediately after the constructor (right
        // before each test).
        void SetUp() override;

        // Code here will be called immediately after each test (right
        // before the destructor).
        void TearDown() override {}

    };

} //
} // namespace BlazeServerTest

#endif
