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
int testCopyTdf(const char8_t* name, uint32_t maxFillDataDepth)
{
    int nErrorCount = 0;
    TestData testData;

    // Test basic copy
    {
        T orig, copy;
        fillTestData(orig);
        orig.copyInto(copy);

        EXPECT_TRUE(orig.equalsValue(copy))
            << "class = " << name;
    }

    // Now fill just one field and its subfields up to depth maxFillDataDepth, and copy the Tdf with change tracking
    for (uint32_t fillDataDepth = 0; fillDataDepth <= maxFillDataDepth; ++fillDataDepth)
    {
        FillTestDataOpts ftdOpts;
        ftdOpts.fillDataDepth = fillDataDepth;
        for (uint32_t counter = 0; ; ++counter)
        {
            T orig, copy;

            TdfMemberIterator itr(orig);
            if (!itr.moveTo(counter)) //if we've reached the end, bail
                break;

            fillTestData(itr, testData, ftdOpts); //fill just the one field

            EA::TDF::MemberVisitOptions mvOpts;
            mvOpts.onlyIfSet = true;
            orig.copyInto(copy, mvOpts);

            if (fillDataDepth == 0)
            {
                EXPECT_TRUE(orig.equalsValue(copy))
                    << "class = " << name << " member = " << itr.getInfo()->getMemberName();

                EXPECT_TRUE(compareChangeBits(orig, copy))
                    << "class = " << name << " member = " << itr.getInfo()->getMemberName();
            }
            else
            {
                EXPECT_TRUE(orig.equalsValue(copy))
                    << "class = " << name << " member = " << itr.getInfo()->getMemberName();

                EXPECT_TRUE(compareChangeBits(orig, copy))
                    << "class = " << name << " member = " << itr.getInfo()->getMemberName();
            }
        }
    }

    // Now fill just one field and copy the Tdf with default value comparisons
    // Skip this test for AllComplexClass because it has no members with defined default values

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4127)  // conditional expression is constant.
#endif
    if (T::TDF_ID != AllComplexClass::TDF_ID)
    {
#ifdef _MSC_VER
    #pragma warning(pop)
#endif
        FillTestDataOpts ftdOpts;
        // Use testNonDefaultCopy = true and default TEST_NON_DEFAULT_COPY_INDEX/TEST_NON_DEFAULT_COPY_TDF_ID
        // to fill just the first non-bool field of any member of type AllPrimitivesClass (unless this type is AllPrimitivesClass).
        // This verifies that the onlyIfNotDefault option is respected at the level of the innermost nested class (where
        // possible - see comment above regarding AllComplexClass)

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4127)  // conditional expression is constant.
#endif
        if (T::TDF_ID != AllPrimitivesClass::TDF_ID)
        {
#ifdef _MSC_VER
#pragma warning(pop)
#endif
            ftdOpts.testNonDefaultCopy = true;
        }

        for (uint32_t counter = 0; ; ++counter)
        {
            T orig, copy, expected;
            fillTestData(copy, altTestData);

            TdfMemberIterator itr(orig);
            TdfMemberIterator copyItr(copy);
            TdfMemberIterator expectedItr(expected);

            if (!itr.moveTo(counter) || !copyItr.moveTo(counter) || !expectedItr.moveTo(counter)) //if we've reached the end, bail
                break;

            // If this is a collections class, all members should be changed by copyInto only if the source collection is non-empty.
            // If source collection is non-emtpy, primitive collections should be copied in their entirety;cComplex collections should be re-populated,
            // respecting onlyIfNotDefault for each field of every element copied.
            fillTestData(expected, altTestData);

            fillTestData(itr, testData, ftdOpts);
            fillTestData(expectedItr, testData, ftdOpts);

            EA::TDF::MemberVisitOptions mvOpts;
            mvOpts.onlyIfNotDefault = true;
            orig.copyInto(copy, mvOpts);

            EXPECT_TRUE(copy.equalsValue(expected))
                << "class = " << name << " member = " << itr.getInfo()->getMemberName();
        }
    }

    return nErrorCount;
}


template <class T>
int testCopyUnion(const char8_t* name, uint32_t maxFillDataDepth)
{
    int nErrorCount = 0;
    TestData testData;

    // Test basic copy
    {
        T orig, copy;
        for (uint32_t memberIndex = 0; memberIndex < TdfUnion::INVALID_MEMBER_INDEX ; ++memberIndex)
        {
            orig.switchActiveMember(memberIndex);
            if (orig.getActiveMemberIndex() == TdfUnion::INVALID_MEMBER_INDEX)
                break;

            fillTestData(orig);
            orig.copyInto(copy);

            EXPECT_TRUE(orig.equalsValue(copy))
                << "class = " << name << " memberIndex = " << memberIndex;
        }
    }

    // Now set the union member and its subfields up to depth maxFillDataDepth, and copy the union with change tracking
    for (uint32_t fillDataDepth = 0; fillDataDepth <= maxFillDataDepth; ++fillDataDepth)
    {
        T orig, copy;
        FillTestDataOpts ftdOpts;
        ftdOpts.fillDataDepth = fillDataDepth;

        for (uint32_t memberIndex = 0; memberIndex < TdfUnion::INVALID_MEMBER_INDEX ; ++memberIndex)
        {
            orig.switchActiveMember(memberIndex);
            if (orig.getActiveMemberIndex() == TdfUnion::INVALID_MEMBER_INDEX)
                break;

            fillTestData(orig, testData, ftdOpts);

            EA::TDF::MemberVisitOptions mvOpts;
            mvOpts.onlyIfSet = true;
            orig.copyInto(copy, mvOpts);

            if (fillDataDepth == 0)
            {
                EXPECT_TRUE(copy.equalsValue(orig))
                    << "class = " << name << "memberIndex = " << memberIndex;

                EXPECT_TRUE(compareChangeBits(orig, copy))
                    << "class = " << name << "memberIndex = " << memberIndex;
            }
            else
            {
                EXPECT_TRUE(copy.equalsValue(orig))
                    << "class = " << name << "memberIndex = " << memberIndex;

                EXPECT_TRUE(compareChangeBits(orig, copy))
                    << "class = " << name << "memberIndex = " << memberIndex;
            }
        }
    }

    // Now set the union member and copy the union with default value comparisons
    {
        T copy, expected, orig;
        fillTestData(copy, altTestData);
        fillTestData(expected, altTestData);

        FillTestDataOpts ftdOpts;
        ftdOpts.testNonDefaultCopy = true;
        for (uint32_t memberIndex = 0; memberIndex < TdfUnion::INVALID_MEMBER_INDEX ; ++memberIndex)
        {
            EA::TDF::MemberVisitOptions mvOpts;
            mvOpts.onlyIfNotDefault = true;

            orig.switchActiveMember(memberIndex);
            if (orig.getActiveMemberIndex() == TdfUnion::INVALID_MEMBER_INDEX)
                break;
            expected.switchActiveMember(memberIndex);

            fillTestData(orig, testData, ftdOpts);
            fillTestData(expected, testData, ftdOpts);
            orig.copyInto(copy, mvOpts);

            EXPECT_TRUE(orig.equalsValue(copy))
                << "class = " << name << "memberIndex = " << memberIndex;
        }
    }

    return nErrorCount;
}

#define MAKE_STRING(_val) #_val
#define TEST_COPY_FUNC_TDF(_name, _num) \
    TEST(TestCopy, Tdf##_name) { testCopyTdf<_name>(#_name, _num); } 

#define TEST_COPY_FUNC_UNION(_name, _num) \
    TEST(TestCopy, Union##_name) { testCopyUnion<_name>(#_name, _num); } 


TEST_COPY_FUNC_TDF(AllPrimitivesClass, 0);
TEST_COPY_FUNC_TDF(AllComplexClass, 1);
TEST_COPY_FUNC_UNION(AllPrimitivesUnion, 0);
TEST_COPY_FUNC_UNION(AllPrimitivesUnionAllocInPlace, 0);
TEST_COPY_FUNC_UNION(AllComplexUnion, 1);
TEST_COPY_FUNC_UNION(AllComplexUnionAllocInPlace, 1);

TEST_COPY_FUNC_TDF(AllListsPrimitivesClass, 1);
TEST_COPY_FUNC_TDF(AllListsComplexClass, 2);
TEST_COPY_FUNC_UNION(AllListsPrimitivesUnion, 1);
TEST_COPY_FUNC_UNION(AllListsPrimitivesUnionAllocInPlace, 1);
TEST_COPY_FUNC_UNION(AllListsComplexUnion, 2);
TEST_COPY_FUNC_UNION(AllListsComplexUnionAllocInPlace, 2);

TEST_COPY_FUNC_TDF(AllMapsPrimitivesClass, 1);
TEST_COPY_FUNC_TDF(AllMapsComplexClass, 2);
TEST_COPY_FUNC_UNION(AllMapsPrimitivesUnion, 1);
TEST_COPY_FUNC_UNION(AllMapsPrimitivesUnionAllocInPlace, 1);
TEST_COPY_FUNC_UNION(AllMapsComplexUnion, 2);
TEST_COPY_FUNC_UNION(AllMapsComplexUnionAllocInPlace, 2);

