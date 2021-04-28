/*************************************************************************************************/
/*!
\file sampletests.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "sample/sampletests.h"
#include <gmock/gmock.h>

using ::testing::AtLeast;
using ::testing::Return;
using ::testing::_;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::NiceMock;

//////////////////////////////////////////////////////////////////
// Documentation:
//
// gTest, gMock documentation is located at
// https://github.com/google/googletest 
// More specifically, familiarize yourself with 
// 1. https://github.com/google/googletest/blob/master/googletest/docs/primer.md 
// 2. https://github.com/google/googletest/blob/master/googlemock/docs/ForDummies.md 
// 3. https://github.com/google/googletest/blob/master/googlemock/docs/CheatSheet.md 
//
// Advanced docs - Hopefully the unit tests don't use advanced features which may result
// in difficulty to understand the tests themselves!
// 1. https://github.com/google/googletest/blob/master/googletest/docs/advanced.md 
//
//////////////////////////////////////////////////////////////////

namespace Test
{
namespace Sample 
{
    ////////////////////////////////////////////////////////////////
    // TEST macro usage that does not need any other special set up
    // Written as TEST(testName, subTestName)
    //
    //
    ////////////////////////////////////////////////////////////////
    TEST(TestMacroUsage, ComparisonOperators)
    {
        int i = 42;
        
        // Equality operators
        EXPECT_EQ(i, 42);
        EXPECT_NE(i, 0);
        
        // GreaterThan operators
        EXPECT_GE(i, 42);
        EXPECT_GT(i, 41);

        // LessThan operators
        EXPECT_LE(i, 42);
        EXPECT_LT(i, 43);
    }

    TEST(TestMacroUsage, StringComparisonOperators)
    {
        // Strings use special operators because you are not comparing for value(pointer in this case) equality
        const char* str = "foo";

        // Equality operators - Default is case sensitive
        EXPECT_STREQ(str, "foo");
        EXPECT_STRNE(str, "bar");

        //Case insensitive comparison
        EXPECT_STRCASEEQ(str, "FOO");
        EXPECT_STRCASENE(str, "bar");
    }

    TEST(TestMacroUsage, BooleanExpressions)
    {
        // Some times, you may want to compare for equality outside (For example, tdf code does not generate == operator
        // but relies on "bool equalsValue(...)" call. So you can evaluate your expression outside the comparison operators
        // and just pass the result.

        // Note - Prefer EXPECT_EQ if you can use it because it'll display the value of argument if the check fails.
        int i = 42;

        EXPECT_TRUE(i == 42);
        EXPECT_FALSE(i != 42);
    }

    TEST(TestMacroUsage, CustomMessage)
    {
        // If you want to log custom messages, you can simply using the stream operator. 
        int i = 42;

        EXPECT_TRUE(i == 42) << "'i' was not found to be 42";
    }

    TEST(TestMacroUsage, AssertUsage)
    {
        // The ASSERT_* counterparts of the EXPECT macros allow us to skip the remainder of the test if the statement
        // fails. It will NOT cause a real assert/debug break that you are more familiar with. 

        const char* str = "foo";
        ASSERT_TRUE(str != nullptr);

        //
        // If above line were to fail, the code here will be skipped. 
        // 
    }

    TEST(TestMacroUsage, DISABLED_disabledTest)
    {
        // Prefixing a test with DISABLED_ will disable the execution of the test (the code still compiles). Use it as
        // a rare measure.

        // As this test will not be called, following failure will never be reported.
        ASSERT_TRUE(false);
    }






    ////////////////////////////////////////////////////////////////
    // TEST_F macro usage that allows for setting up a fixture that can 
    // help you share Initialize/Shutdown for a family of tests.
    // 
    // Written as TEST_F(testFixtureClassName, subTestName)
    //
    // testFixtureClassName needs to derive from testing::Test
    ////////////////////////////////////////////////////////////////

    int64_t gSampleCounter = 0;
    class IncrementTestFixture : public testing::Test
    {
    public:
        IncrementTestFixture() { }
        ~IncrementTestFixture(){ }

        void SetUp() override { gSampleCounter = 0; }
        void TearDown() override {} // handle if necessary (to free a memory allocation or uninit some other state perhaps
    };

    TEST_F(IncrementTestFixture, IncrementBy5)
    {
        gSampleCounter += 5;
        EXPECT_TRUE(gSampleCounter == 5);
    }

    TEST_F(IncrementTestFixture, IncrementBy10)
    {
        // Even though the "IncrementBy5" test incremented the global variable gSampleCounter by 5, this
        // test still passes because SetUp would reset it back to 0.

        gSampleCounter += 10;
        EXPECT_EQ(gSampleCounter, 10);
    }


    // TestFixture with an internal state
    class TestFixtureWithState : public testing::Test
    {
    public:
        TestFixtureWithState() { mName = "foo"; }
        ~TestFixtureWithState() { }

        void SetUp() override { }
        void TearDown() override {} // handle if necessary (to free a memory allocation or uninit some other state perhaps

        const char* getName() const { return mName; }
    private:
        const char* mName;
    };

    TEST_F(TestFixtureWithState, CheckState)
    {
        EXPECT_STREQ(getName(), "foo");
    }




    ////////////////////////////////////////////////////////////////
    // Mock testing sample to mock the interfaces that your test code
    // depends on.
    // 
    ////////////////////////////////////////////////////////////////

    
    const int INVALID_ACCOUNT_ID = 0;
    struct AccountInfo
    {
        int64_t accountId;
        bool superUser;

        AccountInfo(int64_t accountId_, bool superUser_)
            : accountId(accountId_)
            , superUser(superUser_)
        {

        }
    };

    // Real class that is used in the class to be tested. However, note that this is a dummy implementation of a real class
    // for illustration purpose.
    class NucleusService
    {
    public:
        virtual ~NucleusService() {}
        virtual AccountInfo getAccountInfo(const char* token) { return AccountInfo(rand(), false); }
    };

    // Mock class. Note that no implementation of the api is needed.
    class MockNucleusService : public NucleusService
    {
    public:
        MOCK_METHOD1(getAccountInfo, AccountInfo(const char* token));
    };


    // The system to be tested which uses the NucleusService. 
    class OAuthImpl
    {
    public:
        OAuthImpl(NucleusService& nucleusService) : mNucleusService(nucleusService) {}

        AccountInfo getAccountInfo(const char* token)
        {
            if (token == nullptr || token[0] == '\0')
                return AccountInfo(INVALID_ACCOUNT_ID, false);
            
            return mNucleusService.getAccountInfo(token);
        }
    private:
        NucleusService& mNucleusService;
    };


    TEST(OAuthImplTests, testNullToken)
    {
        MockNucleusService nucleusService;
        OAuthImpl oauthImpl(nucleusService);

        // Check handling of invalid input data 
        EXPECT_EQ(oauthImpl.getAccountInfo(nullptr).accountId, INVALID_ACCOUNT_ID);
        EXPECT_EQ(oauthImpl.getAccountInfo("").accountId, INVALID_ACCOUNT_ID);
    }

    TEST(OAuthImplTests, testValidToken)
    {
        MockNucleusService nucleusService;
        OAuthImpl oauthImpl(nucleusService);

        // Check handling of a valid input.
        // Instruct mock service to return number - 42 when the token is "xZJ".

        // Important Quirk: If we simply hardcode "xZJ" string literal to two different places below, gmock fails our unit test
        // because it is testing for equality of those string literals by pointer comparison. So we create a const string literal
        // to use at both the places. 
        // An alternative would be to use std::string in the api. 
        const char* kMockToken = "xZJ";
        EXPECT_CALL(nucleusService, getAccountInfo(kMockToken))
            .Times(1)
            .WillOnce(Return(AccountInfo(42, false)));


        EXPECT_EQ(oauthImpl.getAccountInfo(kMockToken).accountId, 42);
    }

    TEST(OAuthImplTests, testUserIdUsingWildCardForArgs)
    {
        // gmock compares args specified in the EXPECT_CALL and actual args passed to your test class. If they don't match,
        // the test would fail. This may be a hassle in some cases. As we saw in above test case, we had to encapsulate the token
        // string in kMockToken and then use it at 2 different places even though the actual value for it was just arbitrary and could
        // be anything. So gmock allows you to use "_" for the args you don't really care about. 
        
        // In addition to using "_", we also use EXPECT_GE rather than EXPECT_EQ to show further flexibility. 
        
        MockNucleusService nucleusService;
        OAuthImpl oauthImpl(nucleusService);
        EXPECT_CALL(nucleusService, getAccountInfo(_))
            .Times(1)
            .WillOnce(Return(AccountInfo(42, false)));
        
        
        EXPECT_GE(oauthImpl.getAccountInfo("xZJ").accountId, 0);
    }

    TEST(OAuthImplTests, testUserIdForNucleusIssue)
    {
        // This is a test case which tests that after we give what we think is a valid token id (a non-empty input), 
        // if nucleus rejects it and say provides an INVALID_ACCOUNT_ID, OAuthImpl should be able to handle it.

        MockNucleusService nucleusService;
        OAuthImpl oauthImpl(nucleusService);
        EXPECT_CALL(nucleusService, getAccountInfo(_))
            .Times(1)
            .WillOnce(Return(AccountInfo(INVALID_ACCOUNT_ID, false)));

        
        EXPECT_EQ(oauthImpl.getAccountInfo("xZJ").accountId, INVALID_ACCOUNT_ID);
    }

    TEST(OAuthImplTests, testUserIdUsingAtleastForCardinality)
    {
        // At times, the code you are unit testing would take multiple branches or call the api different times and you don't want
        // to have a maintenance hassle (like adjusting unit test code every time you made some change to the internal implementation.
        // So rather than specifying an exact call number, you can specify the least number of times. 
        
        MockNucleusService nucleusService;
        OAuthImpl oauthImpl(nucleusService);
        EXPECT_CALL(nucleusService, getAccountInfo(_))
            .Times(AtLeast(0)) // In this particular test, AtLeast(1) will also pass.
            .WillOnce(Return(AccountInfo(42, false)));

        
        EXPECT_EQ(oauthImpl.getAccountInfo("xZJ").accountId, 42);
    }

    TEST(OAuthImplTests, testUserIdUsingRepeatedlyForResults)
    {
        // At times, you may want to test multiple inputs but expect the same output for each of them.
        // For example, a token can be made of 1. just alphabets 2. alphanumerics so let's try both inputs.
        
        MockNucleusService nucleusService;
        OAuthImpl oauthImpl(nucleusService);
        EXPECT_CALL(nucleusService, getAccountInfo(_))
            .Times(AtLeast(0)) 
            .WillRepeatedly(Return(AccountInfo(42, false)));
       
        
        EXPECT_EQ(oauthImpl.getAccountInfo("xZJ").accountId, 42);
        EXPECT_EQ(oauthImpl.getAccountInfo("x123ZJ").accountId, 42);
    }
    
    TEST(OAuthImplTests, testUserIdByChainingMultipleMockResults)
    {
        // At times, you may want to test multiple inputs but expect a different output for each of them.
        // For example, different tokens should return different account ids to trigger different flow in 
        // your code.

        MockNucleusService nucleusService;
        OAuthImpl oauthImpl(nucleusService);
        EXPECT_CALL(nucleusService, getAccountInfo(_))
            .Times(AtLeast(0))
            .WillOnce(Return(AccountInfo(42, false)))
            .WillOnce(Return(AccountInfo(4242, false))); // You can even chain WillRepeatedly after this.
        

        EXPECT_EQ(oauthImpl.getAccountInfo("xZJ").accountId, 42);
        EXPECT_EQ(oauthImpl.getAccountInfo("x123ZJ").accountId, 4242);
    }


    TEST(OAuthImplTests, testUserIdUsingOnCall)
    {
        // At times, you don't want to be too specific if the code flow would call the external function at all.
        // So instead of EXPECT_CALL, you call ON_CALL.

        // Noted Quirk: We use NiceMock here to prevent gTest from logging a warning about uninteresting calls.
        // The current default behavior is Warn about calls that don't get validated, but we're OK with that here.
        
        NiceMock<MockNucleusService> nucleusService;
        OAuthImpl oauthImpl(nucleusService);
        ON_CALL(nucleusService, getAccountInfo(_)).WillByDefault(Return(AccountInfo(42, false)));

        EXPECT_EQ(oauthImpl.getAccountInfo("").accountId, INVALID_ACCOUNT_ID);
        EXPECT_EQ(oauthImpl.getAccountInfo("xZJ").accountId, 42);
        EXPECT_EQ(oauthImpl.getAccountInfo("x123ZJ").accountId, 42);
    }

    TEST(OAuthImplTests, testUserIdUsingActualCall)
    {
        // At times, you may want to reach out to the original implementation from your mock class (or some other concrete implementation). 
        // It should be rare(or something to be avoided) but it is possible.

        NucleusService nucleusServiceActual;
        MockNucleusService nucleusService;
        OAuthImpl oauthImpl(nucleusService);
        EXPECT_CALL(nucleusService, getAccountInfo(_)).Times(AtLeast(0)).WillRepeatedly(Invoke(&nucleusServiceActual, &NucleusService::getAccountInfo));


        EXPECT_GE(oauthImpl.getAccountInfo("xZJ").accountId, 0);
    }

    AccountInfo return42AsAccountId()
    {
        return AccountInfo(42, false);
    }

    TEST(OAuthImplTests, testUserIdUsingACallWithoutArgs)
    {
        // The above example but using a straight up function call that has no args
        MockNucleusService nucleusService;
        OAuthImpl oauthImpl(nucleusService);
        EXPECT_CALL(nucleusService, getAccountInfo(_)).Times(AtLeast(0)).WillRepeatedly(InvokeWithoutArgs(return42AsAccountId));

        EXPECT_EQ(oauthImpl.getAccountInfo("xZJ").accountId, 42);
    }

}
}