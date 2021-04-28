/*************************************************************************************************/
/*!
\file heat2serializationtest.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_HEAT2_SERIALIZATION_TEST_H
#define BLAZE_HEAT2_SERIALIZATION_TEST_H

#include "gtest/gtest.h"

namespace Test
{
namespace heat2
{

    class Heat2SerializationTest : public testing::Test
    {

    protected:
        Heat2SerializationTest()
        {

        }
        ~Heat2SerializationTest() override
        {

        }

        void SetUp() override;
        void TearDown() override;
    };
} // namespace heat2
}  // namespace Test

#endif