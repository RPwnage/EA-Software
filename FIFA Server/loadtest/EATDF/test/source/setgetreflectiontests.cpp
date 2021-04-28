/////////////////////////////////////////////////////////////////////////////
//  setgetrelectiontests.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
// Written by Jacob Trakhtenberg.
/////////////////////////////////////////////////////////////////////////////

#include <test.h>
#include <testdata.h>
#include <test/tdf/alltypes.h>

using namespace EA::TDF::Test;

template <class T>
static int testSetGetReflection()
{
    int nErrorCount = 0;

    const EA::TDF::TypeDescriptionClass& classDesc = static_cast<const EA::TDF::TypeDescriptionClass&>(EA::TDF::TypeDescriptionSelector<T>::get());
    const char8_t* name = classDesc.name;

    for (uint32_t idx = 0; idx < classDesc.memberCount; ++idx)
    {
        EA::TDF::TdfMemberInfoIterator itr(classDesc);
        itr.moveTo(idx);

        EA::TDF::TdfGenericValue val;
        fillTestData(val, *(itr.getInfo()->getTypeDescription()));

        {
            T tdf;
            EA::TDF::TdfGenericReferenceConst getVal;
            EXPECT_TRUE(tdf.setValueByName(itr.getInfo()->getMemberName(), val))
                << "class = " << name << " member = " << itr.getInfo()->getMemberName();
            EXPECT_TRUE(tdf.getValueByName(itr.getInfo()->getMemberName(), getVal))
                << "class = " << name << " member = " << itr.getInfo()->getMemberName();
            EXPECT_TRUE(val.equalsValue(getVal))
                << "class = " << name << " member = " << itr.getInfo()->getMemberName();
        }

        {
            T tdf;
            EA::TDF::TdfGenericReferenceConst getVal;
            EXPECT_TRUE(tdf.setValueByTag(itr.getInfo()->getTag(), val))
                << "class = " << name << " member = " << itr.getInfo()->getMemberName();
            EXPECT_TRUE(tdf.getValueByTag(itr.getInfo()->getTag(), getVal))
                << "class = " << name << " member = " << itr.getInfo()->getMemberName();
            EXPECT_TRUE(val.equalsValue(getVal))
                << "class = " << name << " member = " << itr.getInfo()->getMemberName();
        }

    }
    
    return nErrorCount;
}



template <class T>
static int testSetGetReflecctionUnion()
{
    int nErrorCount = 0;

    const EA::TDF::TypeDescriptionClass& classDesc = static_cast<const EA::TDF::TypeDescriptionClass&>(EA::TDF::TypeDescriptionSelector<T>::get());
    const char8_t* name = classDesc.name;

    for (uint32_t idx = 0; idx < classDesc.memberCount; ++idx)
    {
        EA::TDF::TdfMemberInfoIterator itr(classDesc);
        itr.moveTo(idx);

        EA::TDF::TdfGenericValue val;
        fillTestData(val, *(itr.getInfo()->getTypeDescription()));

        {
            T tdf;
            tdf.switchActiveMember(idx);

            EA::TDF::TdfGenericReferenceConst getVal;
            EXPECT_TRUE(tdf.setValueByName(itr.getInfo()->getMemberName(), val))
                << "class = " << name << " member = " << itr.getInfo()->getMemberName();
            EXPECT_TRUE(tdf.getValueByName(itr.getInfo()->getMemberName(), getVal))
                << "class = " << name << " member = " << itr.getInfo()->getMemberName();
            EXPECT_TRUE(val.equalsValue(getVal))
                << "class = " << name << " member = " << itr.getInfo()->getMemberName();
        }

        {
            T tdf;
            tdf.switchActiveMember(idx);

            EA::TDF::TdfGenericReference getVal;
            EXPECT_TRUE(tdf.setValueByTag(itr.getInfo()->getTag(), val))
                << "class = " << name << " member = " << itr.getInfo()->getMemberName();
            EXPECT_TRUE(tdf.getValueByTag(itr.getInfo()->getTag(), getVal))
                << "class = " << name << " member = " << itr.getInfo()->getMemberName();
            EXPECT_TRUE(val.equalsValue(getVal))
                << "class = " << name << " member = " << itr.getInfo()->getMemberName();
        }
    }

    return nErrorCount;
}



template <class T>
static int testSetGetReflectionUnionNoTags()
{
    int nErrorCount = 0;

    const EA::TDF::TypeDescriptionClass& classDesc = static_cast<const EA::TDF::TypeDescriptionClass&>(EA::TDF::TypeDescriptionSelector<T>::get());
    const char8_t* name = classDesc.name;

    for (size_t idx = 0; idx < classDesc.memberCount; ++idx)
    {
        EA::TDF::TdfMemberInfoIterator itr(classDesc);
        itr.moveTo(idx);

        EA::TDF::TdfGenericValue val;
        fillTestData(val, *(itr.getInfo()->typeDesc));

        {            
            T tdf;
            tdf.switchActiveMember(idx);

            EA::TDF::TdfGenericReferenceConst getVal;
            EXPECT_TRUE(tdf.setValueByName(itr.getInfo()->getMemberName(), val))
                << "class = " << name << " member = " << itr.getInfo()->getMemberName();
            EXPECT_TRUE(tdf.getValueByName(itr.getInfo()->getMemberName(), getVal))
                << "class = " << name << " member = " << itr.getInfo()->getMemberName();
            EXPECT_TRUE(tdf.getValueByTag(itr.getInfo()->getTag(), getVal))
                << "class = " << name << " member = " << itr.getInfo()->getMemberName();
            EXPECT_TRUE(val.equalsValue(getVal))
                << "class = " << name << " member = " << itr.getInfo()->getMemberName();
        }
      
    

    }

    return nErrorCount;
}


TEST_ALL_TDFS(testSetGetReflection, testSetGetReflecctionUnion);


