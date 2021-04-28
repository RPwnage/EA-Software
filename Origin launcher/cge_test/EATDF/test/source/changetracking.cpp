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
int testSettingChangeBits()
{
    int nErrorCount = 0;

    const EA::TDF::TypeDescriptionClass& classDesc = static_cast<const EA::TDF::TypeDescriptionClass&>(EA::TDF::TypeDescriptionSelector<T>::get());
    const char8_t* name = classDesc.name;


    {
        T orig, marked, cleared;
       
        EXPECT_TRUE(!orig.isSet());

        fillTestData(orig);
        fillTestData(cleared);
        fillTestData(marked);
        marked.markSetRecursive();
        cleared.clearIsSetRecursive();
        
        EXPECT_TRUE(compareChangeBits(orig, marked))
            << "class = " << name;

        EXPECT_TRUE(!compareChangeBits(orig, cleared))
            << "class = " << name;

        orig.clearIsSetRecursive();

        EXPECT_TRUE(!orig.isSet());

        EXPECT_TRUE(compareChangeBits(orig, cleared))
            << "class = " << name;

    }


    return nErrorCount;
}


template <class T>
int testSettingChangeBitsUnion()
{
    int nErrorCount = 0;

    const EA::TDF::TypeDescriptionClass& classDesc = static_cast<const EA::TDF::TypeDescriptionClass&>(EA::TDF::TypeDescriptionSelector<T>::get());
    const char8_t* name = classDesc.name;


    for (uint32_t memberIndex = 0; memberIndex < TdfUnion::INVALID_MEMBER_INDEX ; ++memberIndex)
    {
        T orig, marked, cleared;

        EXPECT_TRUE(!orig.isSet());

        orig.switchActiveMember(memberIndex);
        if (orig.getActiveMemberIndex() == TdfUnion::INVALID_MEMBER_INDEX)
            break;
        
        TdfMemberInfoIterator itr(orig);
        itr.moveTo(memberIndex);

        marked.switchActiveMember(memberIndex);
        cleared.switchActiveMember(memberIndex);

        fillTestData(orig);
        fillTestData(marked);
        fillTestData(cleared);
        marked.markSetRecursive();
        cleared.clearIsSetRecursive();

        EXPECT_TRUE(compareChangeBits(orig, marked))
            << "class = " << name << " member = " << itr.getInfo()->getMemberName();

        EXPECT_TRUE(!compareChangeBits(orig, cleared))
            << "class = " << name << " member = " << itr.getInfo()->getMemberName();

        orig.clearIsSetRecursive();

        EXPECT_TRUE(!orig.isSet());

        EXPECT_TRUE(compareChangeBits(orig, cleared))
            << "class = " << name << " member = " << itr.getInfo()->getMemberName();

    }


    return nErrorCount;
}

TEST_ALL_TDFS(testSettingChangeBits, testSettingChangeBitsUnion);
