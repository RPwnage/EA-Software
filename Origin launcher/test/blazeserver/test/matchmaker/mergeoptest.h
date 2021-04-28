/*************************************************************************************************/
/*!
\file mergeoptest.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_MERGE_OP_TEST_H
#define BLAZE_MERGE_OP_TEST_H

#include "gtest/gtest.h"

namespace Test
{
namespace mergeop
{

    class MergeOpTest : public testing::Test
    {

    protected:
        MergeOpTest()
        {

        }
        ~MergeOpTest() override
        {

        }

        void SetUp() override;
        void TearDown() override;
    };
} // namespace inputsanitizer
}  // namespace Test

#endif