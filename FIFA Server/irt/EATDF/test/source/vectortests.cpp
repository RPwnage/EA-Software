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
int testStructVectorImpl()
{
    int nErrorCount = 0;

    {
        //equality
        T list1, list2;
        typename T::value_type list1V = list1.pull_back();
        EA::TDF::TdfGenericReference list1VR(*list1V);
        fillTestData(list1VR);

        typename T::value_type list2V = list2.pull_back();
        EA::TDF::TdfGenericReference list2VR(*list2V);
        fillTestData(list2VR);

        EXPECT_TRUE(list1VR.equalsValue(list2VR));
        EXPECT_TRUE(list1.equalsValue(list2));        
    }

    {
        //copy
        T list1, list2, list3, list4, list5;
        typename T::value_type list1V = list1.pull_back();
        EA::TDF::TdfGenericReference list1VR(*list1V);
        fillTestData(list1VR);

        typename T::size_type value = 1;

        list1.copyInto(list2);
        list3.insert(list3.begin(), *list1.begin());
        list4.insert(list4.begin(), value, *list1.begin());
        list5.insert(list5.begin(), list1.begin(), list1.end());
        EXPECT_TRUE(list1.equalsValue(list2));        
        EXPECT_TRUE(list1.equalsValue(list3));
        EXPECT_TRUE(list1.equalsValue(list4));
        EXPECT_TRUE(list1.equalsValue(list5));

        // Verify that data isn't just a ref:
        fillTestData(list1VR, altTestData);
        EXPECT_FALSE(list1.equalsValue(list2));
        EXPECT_FALSE(list1.equalsValue(list3));
        EXPECT_FALSE(list1.equalsValue(list4));
        EXPECT_FALSE(list1.equalsValue(list5));
    }

    return nErrorCount;
}


TEST(TestVectors, TdfStructVectors)
{
    ASSERT_EQ(0, testStructVectorImpl<AllListsComplexClass::AllPrimitivesClassList>());
    ASSERT_EQ(0, testStructVectorImpl<AllListsComplexClass::AllPrimitivesUnionList>());
    ASSERT_EQ(0, testStructVectorImpl<AllListsComplexClass::VariableList>());
    ASSERT_EQ(0, testStructVectorImpl<AllListsComplexClass::TdfBlobList>());
    ASSERT_EQ(0, testStructVectorImpl<BoolListList>());
    ASSERT_EQ(0, testStructVectorImpl<ClassListList>());
    ASSERT_EQ(0, testStructVectorImpl<IntBoolMapList>());
    ASSERT_EQ(0, testStructVectorImpl<StringBoolMapList>());
    ASSERT_EQ(0, testStructVectorImpl<IntClassMapList>());
    ASSERT_EQ(0, testStructVectorImpl<StringClassMapList>());

}



template <class T>
int testPrimitiveVectorImpl()
{
    int nErrorCount = 0;

    {
        //equality
        T list1, list2;
        list1.push_back();
        EA::TDF::TdfGenericReference list1VR;
        list1.getReferenceByIndex(0, list1VR);
        fillTestData(list1VR);

        list2.push_back();
        EA::TDF::TdfGenericReference list2VR;
        list2.getReferenceByIndex(0, list2VR);
        fillTestData(list2VR);

        EXPECT_TRUE(list1VR.equalsValue(list2VR));
        EXPECT_TRUE(list1.equalsValue(list2));        
    }

    {
        //copy
        T list1, list2, list3, list4, list5;
        list1.push_back();
        EA::TDF::TdfGenericReference list1VR;
        list1.getReferenceByIndex(0, list1VR);
        fillTestData(list1VR);
        
        typename T::size_type value = 1;

        list1.copyInto(list2);
        list3.insert(list3.begin(), *list1.begin());
        list4.insert(list4.begin(), value, *list1.begin());
        list5.insert(list5.begin(), list1.begin(), list1.end());
        EXPECT_TRUE(list1.equalsValue(list2));
        EXPECT_TRUE(list1.equalsValue(list3));
        EXPECT_TRUE(list1.equalsValue(list4));
        EXPECT_TRUE(list1.equalsValue(list5));

        // Verify that data isn't just a ref:
        fillTestData(list1VR, altTestData);
        EXPECT_FALSE(list1.equalsValue(list2));
        EXPECT_FALSE(list1.equalsValue(list3));
        EXPECT_FALSE(list1.equalsValue(list4));
        EXPECT_FALSE(list1.equalsValue(list5));
    }

    return nErrorCount;
}


TEST(TestVectors, TdfPrimitiveVectors)
{

    ASSERT_EQ(0, testPrimitiveVectorImpl<AllListsPrimitivesClass::BoolList>());
    ASSERT_EQ(0, testPrimitiveVectorImpl<AllListsPrimitivesClass::Int8_tList>());
    ASSERT_EQ(0, testPrimitiveVectorImpl<AllListsPrimitivesClass::Int16_tList>());
    ASSERT_EQ(0, testPrimitiveVectorImpl<AllListsPrimitivesClass::Int32_tList>());
    ASSERT_EQ(0, testPrimitiveVectorImpl<AllListsPrimitivesClass::Int64_tList>());
    ASSERT_EQ(0, testPrimitiveVectorImpl<AllListsPrimitivesClass::Uint8_tList>());
    ASSERT_EQ(0, testPrimitiveVectorImpl<AllListsPrimitivesClass::Uint16_tList>());
    ASSERT_EQ(0, testPrimitiveVectorImpl<AllListsPrimitivesClass::Uint32_tList>());
    ASSERT_EQ(0, testPrimitiveVectorImpl<AllListsPrimitivesClass::Uint64_tList>());
    ASSERT_EQ(0, testPrimitiveVectorImpl<AllListsPrimitivesClass::InClassEnumList>());
    ASSERT_EQ(0, testPrimitiveVectorImpl<AllListsPrimitivesClass::OutOfClassEnumList>());
    ASSERT_EQ(0, testPrimitiveVectorImpl<AllListsPrimitivesClass::FloatList>());
    ASSERT_EQ(0, testPrimitiveVectorImpl<AllListsPrimitivesClass::StringStringList>());


    ASSERT_EQ(0, testPrimitiveVectorImpl<AllListsComplexClass::ABitFieldList>());
    ASSERT_EQ(0, testPrimitiveVectorImpl<AllListsComplexClass::ObjectTypeList>());
    ASSERT_EQ(0, testPrimitiveVectorImpl<AllListsComplexClass::ObjectIdList>());
    ASSERT_EQ(0, testPrimitiveVectorImpl<AllListsComplexClass::TimeValueList>());

}


