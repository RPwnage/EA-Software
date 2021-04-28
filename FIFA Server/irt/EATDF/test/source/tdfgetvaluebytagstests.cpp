/*! *********************************************************************************************/
/*!
    \file tdfgetvaluebytagstests.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#include <test.h>
#include <testdata.h>
#include <test/tdf/alltypes.h>

// Verify a single call to getValueByTags
int testGetValueByTagsHelper(const EA::TDF::Tdf& tdf, const uint32_t tags[], size_t tagSize, EA::TDF::TdfMemberIterator& memberItr, bool expectMemberSet)
{
    int  nErrorCount = 0;

    EA::TDF::TdfGenericReferenceConst tdfResult;
    const EA::TDF::TdfMemberInfo* memberInfo;

    bool memberSet = !expectMemberSet;
    EXPECT_TRUE(tdf.getValueByTags(tags, tagSize, tdfResult, &memberInfo, &memberSet) == true)
        << "member = " << memberItr.getFullName();
    EXPECT_TRUE(memberItr.equalsValue(tdfResult) == true)
        << "member = " << memberItr.getFullName();
    EXPECT_TRUE(memberItr.getInfo() == memberInfo)
        << "member = " << memberItr.getFullName();
    EXPECT_TRUE(expectMemberSet == memberSet)
        << "member = " << memberItr.getFullName();

    return nErrorCount;
}

// Verify calls to getValueByTags that fetch members of variable (isVariable==true) or generic (isVariable==false) types.
// Test the case where the variable or generic is set as AllPrimitivesClass, the case where it's set as AllPrimitivesUnion,
// and the case where it's invalid.
int testGetValueByTagsHelperVariableAndGeneric(bool isVariable)
{
    int  nErrorCount = 0;

    EA::TDF::Test::AllComplexClass allComplexClass;
    uint32_t tags[2];

    if (isVariable)
        allComplexClass.getTagByMemberName("variable", tags[0]);
    else
        allComplexClass.getTagByMemberName("generic", tags[0]);

    {
        // Verify that getValueByTags gracefully handles requests to fetch members of invalid variable or generic types.
        EA::TDF::TdfGenericReferenceConst tdfResult;
        allComplexClass.getClass().getTagByMemberName("bool", tags[1]); // set an arbitrary tag. getValueByTags should return failure before it sees this tag.

        EXPECT_TRUE(allComplexClass.getValueByTags(tags, 2, tdfResult, nullptr, nullptr) == false)
            << "tag = " << tags[1];

        if (!isVariable)
        {
            EA::TDF::GenericTdfType genericSet, genericUnset;
            EA::TDF::Test::AllPrimitivesClass allPrimitivesClass;
            genericSet.set(allPrimitivesClass);

            allComplexClass.getGeneric().set(genericUnset);
            EXPECT_TRUE(allComplexClass.getValueByTags(tags, 2, tdfResult, nullptr, nullptr) == false)
                << "tag = " << tags[1];

            allComplexClass.getGeneric().set(genericSet);
            EXPECT_TRUE(allComplexClass.getValueByTags(tags, 2, tdfResult, nullptr, nullptr) == false)
                << "tag = " << tags[1];
        }
    }

    {
        EA::TDF::Test::AllPrimitivesClass allPrimitivesClassSet, allPrimitivesClassUnset;
        fillTestData(allPrimitivesClassSet);
        EA::TDF::TdfMemberIterator memberItrSet(allPrimitivesClassSet);
        EA::TDF::TdfMemberIterator memberItrUnset(allPrimitivesClassUnset);

        // It's sufficient to fetch one member of AllPrimitivesClass. 
        // Use mFloat because its default value is user-defined.
        memberItrSet.find("float");
        memberItrUnset.find("float");

        tags[1] = memberItrSet.getTag();

        if (isVariable)
        {
            allComplexClass.setVariable(allPrimitivesClassSet);
        }
        else
        {
            allComplexClass.getGeneric().set(allPrimitivesClassSet);
        }
        EXPECT_EQ(0, testGetValueByTagsHelper(allComplexClass, tags, 2, memberItrSet, true))
            << "member = " << memberItrSet.getFullName();

        if (isVariable)
        {
            allComplexClass.setVariable(allPrimitivesClassUnset);
            EXPECT_EQ(0, testGetValueByTagsHelper(allComplexClass, tags, 2, memberItrUnset, false))
                << "member = " << memberItrUnset.getFullName();
        }
        else
        {
            allComplexClass.getGeneric().set(allPrimitivesClassUnset);
            EXPECT_EQ(0, testGetValueByTagsHelper(allComplexClass, tags, 2, memberItrUnset, true))
                << "member = " << memberItrUnset.getFullName(); // A generic is considered to be set even if the value it has been set with is unset.
        }
    }

    {
        EA::TDF::Test::AllPrimitivesUnion allPrimitivesUnionSet, allPrimitivesUnionUnset;
        TestData testData;
        FillTestDataOpts opts;
        opts.setIndex = true;
        fillTestData(allPrimitivesUnionSet, testData, opts);
        EA::TDF::TdfMemberIterator memberItrSet(allPrimitivesUnionSet);
        EA::TDF::TdfMemberIterator memberItrUnset(allPrimitivesUnionUnset);

        memberItrSet.moveTo(allPrimitivesUnionSet.getActiveMemberIndex());
        memberItrUnset.moveTo(allPrimitivesUnionSet.getActiveMemberIndex());

        tags[1] = memberItrSet.getTag();

        if (isVariable)
        {
            allComplexClass.setVariable(allPrimitivesUnionSet);
        }
        else
        {
            allComplexClass.getGeneric().set(allPrimitivesUnionSet);
        }
        EXPECT_EQ(0, testGetValueByTagsHelper(allComplexClass, tags, 2, memberItrSet, true))
            << "member = " << memberItrSet.getFullName();

        if (isVariable)
        {
            allComplexClass.setVariable(allPrimitivesUnionUnset);
            EXPECT_EQ(0, testGetValueByTagsHelper(allComplexClass, tags, 2, memberItrUnset, false))
                << "member = " << memberItrUnset.getFullName();
        }
        else
        {
            allComplexClass.getGeneric().set(allPrimitivesUnionUnset);
            EXPECT_EQ(0, testGetValueByTagsHelper(allComplexClass, tags, 2, memberItrUnset, true))
                << "member = " << memberItrUnset.getFullName(); // A generic is considered to be set even if the value it has been set with is unset.
        }
    }

    return nErrorCount;
}

TEST(GetValueByTags, AllComplexClass)
{
    EA::TDF::Test::AllComplexClass allComplexClassSet, allComplexClassUnset;
    uint32_t tags[2];
    // The default tag to use in tests that attempt to fetch a member of a type that doesn't have subtags.
    // (Note: in these tests, getValueByTags is expected to return failure before it sees the default tag.)
    uint32_t defaultTag = 0;
    allComplexClassSet.getClass().getTagByMemberName("bool", defaultTag);

    fillTestData(allComplexClassSet);

    for (uint32_t memberCounter = 0; ; ++memberCounter)
    {
        EA::TDF::TdfMemberIterator memberItrSet(allComplexClassSet);
        EA::TDF::TdfMemberIterator memberItrUnset(allComplexClassUnset);

        if (!memberItrSet.moveTo(memberCounter) || !memberItrUnset.moveTo(memberCounter))
            break;
        tags[0] = memberItrSet.getTag();

        switch (memberItrSet.getType())
        {
        case EA::TDF::TDF_ACTUAL_TYPE_TDF:
            {
                EA::TDF::TdfMemberIterator submemberItrSet(memberItrSet.asTdf());
                EA::TDF::TdfMemberIterator submemberItrUnset(memberItrUnset.asTdf());

                for (uint32_t submemberCounter = 0; ; ++submemberCounter)
                {
                    if (!submemberItrSet.moveTo(submemberCounter) || !submemberItrUnset.moveTo(submemberCounter))
                        break;

                    tags[1] = submemberItrSet.getTag();
                    ASSERT_EQ(0, testGetValueByTagsHelper(allComplexClassSet, tags, 2, submemberItrSet, true))
                        << "member = " << memberItrSet.getFullName() << " submember = " << submemberItrSet.getFullName();
                    ASSERT_EQ(0, testGetValueByTagsHelper(allComplexClassUnset, tags, 2, submemberItrUnset, false))
                        << "member = " << memberItrSet.getFullName() << " submember = " << submemberItrSet.getFullName();
                }
            }
            break;
        case EA::TDF::TDF_ACTUAL_TYPE_UNION:
            {
                for (uint32_t memberIndex = 0; memberIndex < EA::TDF::TdfUnion::INVALID_MEMBER_INDEX ; ++memberIndex)
                {
                    // Note: if a union has an active member, that member is considered to be set.
                    memberItrSet.asUnion().switchActiveMember(memberIndex);
                    uint32_t activeMemberIndex = memberItrSet.asUnion().getActiveMemberIndex();

                    if (activeMemberIndex == EA::TDF::TdfUnion::INVALID_MEMBER_INDEX)
                        break;

                    EA::TDF::TdfMemberIterator submemberItrSet(memberItrSet.asUnion());
                    EA::TDF::TdfMemberIterator submemberItrUnset(memberItrUnset.asUnion());
                    const char8_t* submemberName = nullptr;
                    memberItrSet.asUnion().getMemberNameByIndex(activeMemberIndex, submemberName);

                    memberItrSet.asUnion().getIteratorByName(submemberName, submemberItrSet);
                    memberItrUnset.asUnion().getIteratorByName(submemberName, submemberItrUnset);
                    memberItrUnset.asUnion().clearIsSet(); // unset the member, since it was set by fetching the iterator

                    tags[1] = submemberItrSet.getTag();
                    ASSERT_EQ(0, testGetValueByTagsHelper(allComplexClassSet, tags, 2, submemberItrSet, true))
                        << "member = " << memberItrSet.getFullName() << " submember = " << submemberItrSet.getFullName();
                    ASSERT_EQ(0, testGetValueByTagsHelper(allComplexClassUnset, tags, 2, submemberItrUnset, false))
                        << "member = " << memberItrSet.getFullName() << " submember = " << submemberItrSet.getFullName();
                }
            }
            break;
        default:
            {
                if (memberItrSet.getType() != EA::TDF::TDF_ACTUAL_TYPE_VARIABLE && memberItrSet.getType() != EA::TDF::TDF_ACTUAL_TYPE_GENERIC_TYPE)
                {
                    // Verify that getValueByTags gracefully handles requests to fetch members of types that don't have subtags.
                    EA::TDF::TdfGenericReferenceConst tdfResult;
                    tags[1] = defaultTag;
                    EXPECT_TRUE(allComplexClassSet.getValueByTags(tags, 2, tdfResult, nullptr, nullptr) == false)
                        << "member = " << memberItrSet.getFullName() << " tag = " << tags[1];
                }

                // Don't fetch submembers of a variable or generic member;
                // save that for the GetValueByTagsVariable and GetValueByTagsGeneric tests
                ASSERT_EQ(0, testGetValueByTagsHelper(allComplexClassSet, tags, 1, memberItrSet, true))
                    << "member = " << memberItrSet.getFullName();
                ASSERT_EQ(0, testGetValueByTagsHelper(allComplexClassUnset, tags, 1, memberItrUnset, false))
                    << "member = " << memberItrSet.getFullName();
            }
            break;
        }
    }
}

TEST(GetValueByTags, Variable)
{
    ASSERT_EQ(0, testGetValueByTagsHelperVariableAndGeneric(true));
}

TEST(GetValueByTags, Generic)
{
    ASSERT_EQ(0, testGetValueByTagsHelperVariableAndGeneric(false));
}


