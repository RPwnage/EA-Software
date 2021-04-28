/*! *********************************************************************************************/
/*!
    \file 


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#include <test.h>
#include <testdata.h>
#include <test/tdf/alltypes.h>

using namespace EA::TDF;
using namespace EA::TDF::Test;


template <class T>
static int hashTdf(const char8_t* name, bool isCollection)
{
    int nErrorCount = 0;
    T sampleTdf;
    TestData testData;
    uint32_t hashValue, prevHashValue;

    FillTestDataOpts fillTestDataOpts;
    fillTestDataOpts.overwriteCollections = false;

    fillTestData(sampleTdf, testData);

    EXPECT_TRUE(sampleTdf.computeHash(prevHashValue));
    EXPECT_TRUE(sampleTdf.computeHash(hashValue));

    EXPECT_TRUE(hashValue == prevHashValue);

    TdfMemberIterator itr(sampleTdf);
    while (itr.next())
    {
        prevHashValue = hashValue;
        fillTestData(itr, altTestData);
        
        EXPECT_TRUE(sampleTdf.computeHash(hashValue));
        EXPECT_TRUE(hashValue != prevHashValue);

        if(isCollection)
        {
            // If container, run another fillTestData with fillTestDataOpts with overwriteCollections set to false
            // this is going to add another element to the container instead of clearing it.

            prevHashValue = hashValue;
            fillTestData(itr, testData, fillTestDataOpts);

            EXPECT_TRUE(sampleTdf.computeHash(hashValue));
            EXPECT_TRUE(hashValue != prevHashValue);
        }
    }

    return nErrorCount;
}


template <class T>
static int hashUnion(const char8_t* name, bool isCollection)
{
    int nErrorCount = 0;
    T sampleTdf;

    TestData testData;

    FillTestDataOpts fillTestDataOpts;
    fillTestDataOpts.overwriteCollections = false;

    uint32_t hashValue, prevHashValue;

    for (uint32_t memberIndex = 0; memberIndex < TdfUnion::INVALID_MEMBER_INDEX ; ++memberIndex)
    {   

        sampleTdf.switchActiveMember(memberIndex);
        if (sampleTdf.getActiveMemberIndex() == TdfUnion::INVALID_MEMBER_INDEX)
            break;
        
        fillTestData(sampleTdf, testData);
        EXPECT_TRUE(sampleTdf.computeHash(prevHashValue));

        fillTestData(sampleTdf, altTestData);
        EXPECT_TRUE(sampleTdf.computeHash(hashValue));

        EXPECT_TRUE(hashValue != prevHashValue);

        prevHashValue = hashValue;

        if (isCollection)
        {

            fillTestData(sampleTdf, testData, fillTestDataOpts);
            EXPECT_TRUE(sampleTdf.computeHash(hashValue));

            EXPECT_TRUE(hashValue != prevHashValue);
        }
    }
    return nErrorCount;
}

#define MAKE_STRING(_val) #_val
#define TEST_HASH_TDF(_name, _iscollection) \
    TEST(TestHashTDF, _name) { ASSERT_EQ(0, hashTdf<_name>(#_name, _iscollection)); }

#define TEST_HASH_UNION(_name, _iscollection) \
    TEST(TestHashUnion, _name) { ASSERT_EQ(0, hashUnion<_name>(#_name, _iscollection)); } 

TEST_HASH_TDF(AllPrimitivesClass, false);
TEST_HASH_TDF(AllListsPrimitivesClass, true);
TEST_HASH_TDF(AllMapsPrimitivesClass,true);
TEST_HASH_TDF(AllComplexClass, false);
TEST_HASH_TDF(AllListsComplexClass, true);
TEST_HASH_TDF(AllMapsComplexClass, true);

TEST_HASH_UNION(AllMapsPrimitivesUnion, true);
TEST_HASH_UNION(AllMapsPrimitivesUnionAllocInPlace, true);
TEST_HASH_UNION(AllListsPrimitivesUnion, true);
TEST_HASH_UNION(AllListsPrimitivesUnionAllocInPlace, true);
TEST_HASH_UNION(AllPrimitivesUnion, false);
TEST_HASH_UNION(AllPrimitivesUnionAllocInPlace, false);

TEST_HASH_UNION(AllComplexUnion, false);
TEST_HASH_UNION(AllComplexUnionAllocInPlace, false);
TEST_HASH_UNION(AllListsComplexUnion, true);
TEST_HASH_UNION(AllListsComplexUnionAllocInPlace, true);
TEST_HASH_UNION(AllMapsComplexUnion, true);
TEST_HASH_UNION(AllMapsComplexUnionAllocInPlace, true);