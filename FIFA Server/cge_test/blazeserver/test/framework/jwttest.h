/*************************************************************************************************/
/*!
\file jwttest.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVER_TEST_JWT_TEST_H
#define BLAZE_SERVER_TEST_JWT_TEST_H

#include <gtest/gtest.h>

namespace Blaze
{
namespace OAuth
{
    class JwtTest : public testing::Test
    {
    public:
        JwtTest()
        {
        }

        ~JwtTest() override
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

} //namespace OAuth
} //namespace Blaze

#endif //BLAZE_SERVER_TEST_JWT_TEST_H
