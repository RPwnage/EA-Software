/////////////////////////////////////////////////////////////////////////////
// main.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include <gtest/gtest.h>

#include "test/common/gtest/gtestfixtureutil.h" 

int main(int argc, char **argv)
{
    // https://github.com/google/googletest/blob/master/googletest/docs/advanced.md 
    // TODO:
    // We should look at removing everything that is not GT Menu based from this.
    // -> We should investigate setting the environment - a class derived from ::testing::Environment 
    // -> We can use complete custom handling of logging output
    // -> --gtest_list_tests, --gtest_filter  
    // -> There are a bunch of command line options at the end that can let us customize the behavior of
    //      gTest the way we like. These can also be exposed as GUI options to help users.
    TestingUtils::GtestFixtureUtil::preInitGoogleTest(argc, argv);

    ::testing::InitGoogleTest(&argc, argv);

    // Adds thread safety to death tests. May not be strictly needed in Blaze tests.
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";

    return RUN_ALL_TESTS();
}






