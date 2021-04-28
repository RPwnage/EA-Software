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
int testComparisonTdf()
{
    int nErrorCount = 0;

    const EA::TDF::TypeDescriptionClass& classDesc = static_cast<const EA::TDF::TypeDescriptionClass&>(EA::TDF::TypeDescriptionSelector<T>::get());
    const char8_t* name = classDesc.name;
    
    T c1, c2;
    fillTestData(c1);
    fillTestData(c2);
    EXPECT_TRUE(c1.equalsValue(c2))
        << "class = " << name; //validate equality
    EXPECT_TRUE(c2.equalsValue(c1))
        << "class = " << name;

    //Now test inequality on each member
    TdfMemberIterator itr(c2);
    while (itr.next())
    {
        fillTestData(c2); //return to stock
        fillTestData(itr, altTestData); //set this member to alternate value.

        EXPECT_FALSE(c1.equalsValue(c2))
            << "class = " << name;
        EXPECT_FALSE(c2.equalsValue(c1))
            << "class = " << name;
    }

    return nErrorCount;
}


template <class T>
int testComparisonUnion()
{
    int nErrorCount = 0;

    const EA::TDF::TypeDescriptionClass& classDesc = static_cast<const EA::TDF::TypeDescriptionClass&>(EA::TDF::TypeDescriptionSelector<T>::get());
    const char8_t* name = classDesc.name;

    T c1, c2;
    EXPECT_TRUE(c1.equalsValue(c2))
        << "class = " << name; //validate equality with unset
    EXPECT_TRUE(c2.equalsValue(c1))
        << "class = " << name;

    for (uint32_t memberIndex = 0; memberIndex < TdfUnion::INVALID_MEMBER_INDEX ; ++memberIndex)
    {
        c1.switchActiveMember(memberIndex);
        if (c1.getActiveMemberIndex() == TdfUnion::INVALID_MEMBER_INDEX)
            break;
        
        c2.switchActiveMember(TdfUnion::INVALID_MEMBER_INDEX);
        EXPECT_FALSE(c1.equalsValue(c2))
            << "class = " << name << " memberIndex = " << memberIndex;
        EXPECT_FALSE(c2.equalsValue(c1))
            << "class = " << name << " memberIndex = " << memberIndex; //validate inequality with unset
        
        c2.switchActiveMember(memberIndex);

        fillTestData(c1);
        fillTestData(c2);

        //validate equality
        EXPECT_TRUE(c1.equalsValue(c2))
            << "class = " << name << " memberIndex = " << memberIndex;
        EXPECT_TRUE(c2.equalsValue(c1))
            << "class = " << name << " memberIndex = " << memberIndex;

        fillTestData(c2, altTestData); //set this member to alternate value.
        EXPECT_FALSE(c1.equalsValue(c2))
            << "class = " << name << " memberIndex = " << memberIndex;
        EXPECT_FALSE(c2.equalsValue(c1))
            << "class = " << name << " memberIndex = " << memberIndex;
    }

    return nErrorCount;
}

TEST_ALL_TDFS(testComparisonTdf, testComparisonUnion);