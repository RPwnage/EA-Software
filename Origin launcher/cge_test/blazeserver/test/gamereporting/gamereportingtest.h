/*************************************************************************************************/
/*!
\file gamereportingtest.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_PROTOBUF_GAME_REPORTING_TEST_H
#define BLAZE_PROTOBUF_GAME_REPORTING_TEST_H

#include "gtest/gtest.h"

namespace Test
{
namespace protobuf
{

    class GameReportingTest : public testing::Test
    {

    protected:
        GameReportingTest()
        {

        }
        ~GameReportingTest() override
        {

        }

        void SetUp() override;
        void TearDown() override;
    };
} // namespace protobuf
}  // namespace Test

#endif