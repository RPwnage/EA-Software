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
int testStructMapImpl()
{
    int nErrorCount = 0;

    {
        //equality
        T map1, map2;
        typename T::ValType* map1V = map1.allocate_element();
        EA::TDF::TdfGenericReference map1VR(*map1V);
        fillTestData(map1VR);
        
        typename T::key_type map1K;
        EA::TDF::TdfGenericReference map1KR(map1K);
        fillTestData(map1KR);

        map1[map1K] = map1V;


        typename T::ValType* map2V = map2.allocate_element();
        EA::TDF::TdfGenericReference map2VR(*map2V);
        fillTestData(map2VR);

        typename T::key_type map2K;
        EA::TDF::TdfGenericReference map2KR(map2K);
        fillTestData(map2KR);

        map2[map2K] = map2V;

        EXPECT_TRUE(map1VR.equalsValue(map2VR));
        EXPECT_TRUE(map1.equalsValue(map2));        
    }

    {
        //copy
        T map1, map2;
        typename T::ValType* map1V = map1.allocate_element();
        EA::TDF::TdfGenericReference map1VR(*map1V);
        fillTestData(map1VR);

        typename T::key_type map1K;
        EA::TDF::TdfGenericReference map1KR(map1K);
        fillTestData(map1KR);

        map1[map1K] = map1V;


        map1.copyInto(map2);
        EXPECT_TRUE(map1.equalsValue(map2));        
    }

    return nErrorCount;
}


TEST(TestMaps, StructMaps)
{

    ASSERT_EQ(0, testStructMapImpl<AllMapsComplexClass::StringClassMap>());
    ASSERT_EQ(0, testStructMapImpl<AllMapsComplexClass::IntClassMap>());
    ASSERT_EQ(0, testStructMapImpl<AllMapsComplexClass::StringUnionMap>());
    ASSERT_EQ(0, testStructMapImpl<AllMapsComplexClass::IntUnionMap>());
    ASSERT_EQ(0, testStructMapImpl<AllMapsComplexClass::StringVariableMap>());
    ASSERT_EQ(0, testStructMapImpl<AllMapsComplexClass::IntVariableMap>());
    ASSERT_EQ(0, testStructMapImpl<AllMapsComplexClass::StringBlobMap>());
    ASSERT_EQ(0, testStructMapImpl<AllMapsComplexClass::IntBlobMap>());

    ASSERT_EQ(0, testStructMapImpl<IntBoolListMap>());
    ASSERT_EQ(0, testStructMapImpl<StringBoolListMap>());
    ASSERT_EQ(0, testStructMapImpl<IntClassListMap>());
    ASSERT_EQ(0, testStructMapImpl<StringClassListMap>());
    ASSERT_EQ(0, testStructMapImpl<IntIntBoolMapMap>());
    ASSERT_EQ(0, testStructMapImpl<StringIntBoolMapMap>());
    ASSERT_EQ(0, testStructMapImpl<IntIntClassMapMap>());
    ASSERT_EQ(0, testStructMapImpl<StringIntClassMapMap>());
}


template <class T>
struct TestPrimitiveMapImpl
{
    static int doTest()
    {
        typedef typename T::key_type K;
        typedef typename T::ValType V;

        int nErrorCount = 0;

        {
            //equality
            T map1, map2;
            V map1V;
            fillTestDataRaw<V>::fill(map1V);

            K map1K;
            fillTestDataRaw<K>::fill(map1K);

            map1[map1K] = map1V;


            V map2V;
            fillTestDataRaw<V>::fill(map2V);

            K map2K;
            fillTestDataRaw<K>::fill(map2K);

            map2[map2K] = map2V;

            EXPECT_TRUE(map1.equalsValue(map2));        
        }

        {
            //copy
            T map1, map2;
            V map1V;
            fillTestDataRaw<V>::fill(map1V);

            K map1K;
            fillTestDataRaw<K>::fill(map1K);

            map1[map1K] = map1V;


            map1.copyInto(map2);
            EXPECT_TRUE(map1.equalsValue(map2));        
        }

        return nErrorCount;
    }
};



TEST(TestMaps, PrimitiveMaps)
{

    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::StringBoolMap>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::StringInt8Map>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::StringInt16Map>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::StringInt32Map>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::StringInt64Map>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::StringUInt8Map>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::StringUInt16Map>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::StringUInt32Map>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::StringUInt64Map>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::StringInClassEnumMap>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::StringOutOfClassEnumMap>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::StringFloatMap>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::StringStringMap>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsComplexClass::StringBitFieldMap>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsComplexClass::StringObjectTypeMap>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsComplexClass::StringObjectIdMap>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsComplexClass::StringTimeValueMap>::doTest());

    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::IntBoolMap>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::IntInt8Map>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::IntInt16Map>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::IntInt32Map>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::IntInt64Map>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::IntUInt8Map>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::IntUInt16Map>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::IntUInt32Map>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::IntUInt64Map>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::IntInClassEnumMap>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::IntOutOfClassEnumMap>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::IntFloatMap>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsPrimitivesClass::IntStringMap>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsComplexClass::IntBitFieldMap>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsComplexClass::IntObjectTypeMap>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsComplexClass::IntObjectIdMap>::doTest());
    ASSERT_EQ(0, TestPrimitiveMapImpl<AllMapsComplexClass::IntTimeValueMap>::doTest());
   
}


