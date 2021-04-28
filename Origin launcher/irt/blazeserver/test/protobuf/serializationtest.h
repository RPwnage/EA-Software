/*************************************************************************************************/
/*!
\file serializationtest.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_PROTOBUF_SERIALIZATION_TEST_H
#define BLAZE_PROTOBUF_SERIALIZATION_TEST_H

#include "gtest/gtest.h"

namespace Test
{
namespace protobuf
{

    class SerializationTest : public testing::Test
    {

    protected:
        SerializationTest()
        {

        }
        ~SerializationTest() override
        {

        }

        void SetUp() override;
        void TearDown() override;
    };
} // namespace protobuf
}  // namespace Test

#endif