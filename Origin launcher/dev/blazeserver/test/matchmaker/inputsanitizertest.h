/*************************************************************************************************/
/*!
\file inputsanitizertest.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_INPUT_SANITIZER_TEST_H
#define BLAZE_INPUT_SANITIZER_TEST_H

#include "gtest/gtest.h"

namespace Test
{
namespace inputsanitizer
{

    class InputSanitizerTest : public testing::Test
    {

    protected:
        InputSanitizerTest()
        {

        }
        ~InputSanitizerTest() override
        {

        }

        void SetUp() override;
        void TearDown() override;
    };
} // namespace inputsanitizer
}  // namespace Test

#endif