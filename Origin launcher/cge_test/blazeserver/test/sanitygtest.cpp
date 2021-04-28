/*************************************************************************************************/
/*!
\file sanitygtest.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "sanitygtest.h"

using namespace BlazeServerTest;

void SanityGtest::dummyFailFn()
{
    eastl::string msg = "Intentional failing non empty message";
    EXPECT_STREQ("", msg.c_str());
}

// Sanity tests the gt framework works with disabled tests, error logging, etc. Usable for manually checking UI menu
TEST_F(SanityGtest, DISABLED_DisabledFailingSanityTest)
{
    int i = 123;

    // purposely fail this
    EXPECT_TRUE(i == 321);

    // to test UI tracing, call a method that purposely fails
    dummyFailFn();
}
