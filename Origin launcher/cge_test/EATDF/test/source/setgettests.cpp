/////////////////////////////////////////////////////////////////////////////
// BasicTest.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
// Written by Jacob Trakhtenberg.
/////////////////////////////////////////////////////////////////////////////

#include <test.h>
#include <testdata.h>
#include <test/tdf/alltypes.h>

TEST(SetGetPrimitives, BaseClass)
{
    EA::TDF::Test::AllPrimitivesClass allPrimitives;
    
    allPrimitives.setBool(true);
    EXPECT_TRUE(allPrimitives.getBool() == true);

    allPrimitives.setUInt8(UINT8_VAL);
    EXPECT_TRUE(allPrimitives.getUInt8() == UINT8_VAL);

    allPrimitives.setUInt16(UINT16_VAL);
    EXPECT_TRUE(allPrimitives.getUInt16() == UINT16_VAL);

    allPrimitives.setUInt32(UINT32_VAL);
    EXPECT_TRUE(allPrimitives.getUInt32() == UINT32_VAL);

    allPrimitives.setUInt64(UINT64_VAL);
    EXPECT_TRUE(allPrimitives.getUInt64() == UINT64_VAL);

    allPrimitives.setInt8(INT8_VAL);
    EXPECT_TRUE(allPrimitives.getInt8() == INT8_VAL);

    allPrimitives.setInt16(INT16_VAL);
    EXPECT_TRUE(allPrimitives.getInt16() == INT16_VAL);

    allPrimitives.setInt32(INT32_VAL);
    EXPECT_TRUE(allPrimitives.getInt32() == INT32_VAL);

    allPrimitives.setInt64(INT64_VAL);
    EXPECT_TRUE(allPrimitives.getInt64() == INT64_VAL);

    allPrimitives.setFloat(FLOAT_VAL);
    EXPECT_TRUE(allPrimitives.getFloat() == FLOAT_VAL);

    allPrimitives.setString(STRING_VAL);
    EXPECT_TRUE(EA::StdC::Strcmp(allPrimitives.getString(), STRING_VAL) == 0);

    allPrimitives.setOutOfClassEnum(EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2);
    EXPECT_TRUE(allPrimitives.getOutOfClassEnum() == EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2);

    allPrimitives.setInClassEnum(EA::TDF::Test::AllPrimitivesClass::IN_OF_CLASS_ENUM_VAL_2);
    EXPECT_TRUE(allPrimitives.getInClassEnum() == EA::TDF::Test::AllPrimitivesClass::IN_OF_CLASS_ENUM_VAL_2);
}

TEST(SetGetPrimitives, ConstDefaultClass)
{
    EA::TDF::Test::AllPrimitivesConstDefaultClass allPrimitives;

    EXPECT_TRUE(allPrimitives.getUInt8() == EA::TDF::Test::UINT8_T_CONST);
    allPrimitives.setUInt8(UINT8_VAL);
    EXPECT_TRUE(allPrimitives.getUInt8() == UINT8_VAL);

    EXPECT_TRUE(allPrimitives.getUInt16() == EA::TDF::Test::UINT16_T_CONST);
    allPrimitives.setUInt16(UINT16_VAL);
    EXPECT_TRUE(allPrimitives.getUInt16() == UINT16_VAL);

    EXPECT_TRUE(allPrimitives.getUInt32() == EA::TDF::Test::UINT32_T_CONST);
    allPrimitives.setUInt32(UINT32_VAL);
    EXPECT_TRUE(allPrimitives.getUInt32() == UINT32_VAL);

    EXPECT_TRUE(allPrimitives.getUInt64() == EA::TDF::Test::UINT64_T_CONST);
    allPrimitives.setUInt64(UINT64_VAL);
    EXPECT_TRUE(allPrimitives.getUInt64() == UINT64_VAL);

    EXPECT_TRUE(allPrimitives.getInt8() == EA::TDF::Test::INT8_T_CONST);
    allPrimitives.setInt8(INT8_VAL);
    EXPECT_TRUE(allPrimitives.getInt8() == INT8_VAL);

    EXPECT_TRUE(allPrimitives.getInt16() == EA::TDF::Test::INT16_T_CONST);
    allPrimitives.setInt16(INT16_VAL);
    EXPECT_TRUE(allPrimitives.getInt16() == INT16_VAL);

    EXPECT_TRUE(allPrimitives.getInt32() == EA::TDF::Test::INT32_T_CONST);
    allPrimitives.setInt32(INT32_VAL);
    EXPECT_TRUE(allPrimitives.getInt32() == INT32_VAL);

    EXPECT_TRUE(allPrimitives.getInt64() == EA::TDF::Test::INT64_T_CONST);
    allPrimitives.setInt64(INT64_VAL);
    EXPECT_TRUE(allPrimitives.getInt64() == INT64_VAL);

    EXPECT_TRUE(allPrimitives.getFloat() == EA::TDF::Test::FLOAT_CONST);
    allPrimitives.setFloat(FLOAT_VAL);
    EXPECT_TRUE(allPrimitives.getFloat() == FLOAT_VAL);

}


TEST(SetGetPrimitives, MaxDefaultClass)
{
    EA::TDF::Test::AllPrimitivesMaxDefaultClass allPrimitives;

    EXPECT_TRUE(allPrimitives.getUInt8() == UINT8_MAX);
    allPrimitives.setUInt8(UINT8_VAL);
    EXPECT_TRUE(allPrimitives.getUInt8() == UINT8_VAL);

    EXPECT_TRUE(allPrimitives.getUInt16() == UINT16_MAX);
    allPrimitives.setUInt16(UINT16_VAL);
    EXPECT_TRUE(allPrimitives.getUInt16() == UINT16_VAL);

    EXPECT_TRUE(allPrimitives.getUInt32() == UINT32_MAX);
    allPrimitives.setUInt32(UINT32_VAL);
    EXPECT_TRUE(allPrimitives.getUInt32() == UINT32_VAL);

    EXPECT_TRUE(allPrimitives.getUInt64() == UINT64_MAX);
    allPrimitives.setUInt64(UINT64_VAL);
    EXPECT_TRUE(allPrimitives.getUInt64() == UINT64_VAL);

    EXPECT_TRUE(allPrimitives.getInt8() == INT8_MAX);
    allPrimitives.setInt8(INT8_VAL);
    EXPECT_TRUE(allPrimitives.getInt8() == INT8_VAL);

    EXPECT_TRUE(allPrimitives.getInt16() == INT16_MAX);
    allPrimitives.setInt16(INT16_VAL);
    EXPECT_TRUE(allPrimitives.getInt16() == INT16_VAL);

    EXPECT_TRUE(allPrimitives.getInt32() == INT32_MAX);
    allPrimitives.setInt32(INT32_VAL);
    EXPECT_TRUE(allPrimitives.getInt32() == INT32_VAL);

    EXPECT_TRUE(allPrimitives.getInt64() == INT64_MAX);
    allPrimitives.setInt64(INT64_VAL);
    EXPECT_TRUE(allPrimitives.getInt64() == INT64_VAL);

    EXPECT_TRUE(allPrimitives.getFloat() == FLT_MAX);
    allPrimitives.setFloat(FLOAT_VAL);
    EXPECT_TRUE(allPrimitives.getFloat() == FLOAT_VAL);

}


TEST(SetGetPrimitives, MinDefaultClass)
{
    EA::TDF::Test::AllPrimitivesMinDefaultClass allPrimitives;

    EXPECT_TRUE(allPrimitives.getInt8() == INT8_MIN);
    allPrimitives.setInt8(INT8_VAL);
    EXPECT_TRUE(allPrimitives.getInt8() == INT8_VAL);

    EXPECT_TRUE(allPrimitives.getInt16() == INT16_MIN);
    allPrimitives.setInt16(INT16_VAL);
    EXPECT_TRUE(allPrimitives.getInt16() == INT16_VAL);

    EXPECT_TRUE(allPrimitives.getInt32() == INT32_MIN);
    allPrimitives.setInt32(INT32_VAL);
    EXPECT_TRUE(allPrimitives.getInt32() == INT32_VAL);

    EXPECT_TRUE(allPrimitives.getInt64() == INT64_MIN);
    allPrimitives.setInt64(INT64_VAL);
    EXPECT_TRUE(allPrimitives.getInt64() == INT64_VAL);

    EXPECT_TRUE(allPrimitives.getFloat() == FLT_MIN);
    allPrimitives.setFloat(FLOAT_VAL);
    EXPECT_TRUE(allPrimitives.getFloat() == FLOAT_VAL);
}


TEST(SetGetPrimitives, Union)
{
    EA::TDF::Test::AllPrimitivesUnion allPrimitives;

    allPrimitives.setBool(true);
    EXPECT_TRUE(allPrimitives.getBool());

    allPrimitives.setUInt8(UINT8_VAL);
    EXPECT_TRUE(allPrimitives.getUInt8() == UINT8_VAL);

    allPrimitives.setUInt16(UINT16_VAL);
    EXPECT_TRUE(allPrimitives.getUInt16() == UINT16_VAL);

    allPrimitives.setUInt32(UINT32_VAL);
    EXPECT_TRUE(allPrimitives.getUInt32() == UINT32_VAL);

    allPrimitives.setUInt64(UINT64_VAL);
    EXPECT_TRUE(allPrimitives.getUInt64() == UINT64_VAL);

    allPrimitives.setInt8(INT8_VAL);
    EXPECT_TRUE(allPrimitives.getInt8() == INT8_VAL);

    allPrimitives.setInt16(INT16_VAL);
    EXPECT_TRUE(allPrimitives.getInt16() == INT16_VAL);

    allPrimitives.setInt32(INT32_VAL);
    EXPECT_TRUE(allPrimitives.getInt32() == INT32_VAL);

    allPrimitives.setInt64(INT64_VAL);
    EXPECT_TRUE(allPrimitives.getInt64() == INT64_VAL);

    allPrimitives.setFloat(FLOAT_VAL);
    EXPECT_TRUE(allPrimitives.getFloat() == FLOAT_VAL);

    allPrimitives.setString(STRING_VAL);
    EXPECT_TRUE(EA::StdC::Strcmp(allPrimitives.getString(), STRING_VAL) == 0);

    allPrimitives.setOutOfClassEnum(EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2);
    EXPECT_TRUE(allPrimitives.getOutOfClassEnum() == EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2);
}


TEST(SetGetPrimitives, UnionAllocInPlace)
{
    EA::TDF::Test::AllPrimitivesUnionAllocInPlace allPrimitives;

    allPrimitives.setBool(true);
    EXPECT_TRUE(allPrimitives.getBool());

    allPrimitives.setUInt8(UINT8_VAL);
    EXPECT_TRUE(allPrimitives.getUInt8() == UINT8_VAL);

    allPrimitives.setUInt16(UINT16_VAL);
    EXPECT_TRUE(allPrimitives.getUInt16() == UINT16_VAL);

    allPrimitives.setUInt32(UINT32_VAL);
    EXPECT_TRUE(allPrimitives.getUInt32() == UINT32_VAL);

    allPrimitives.setUInt64(UINT64_VAL);
    EXPECT_TRUE(allPrimitives.getUInt64() == UINT64_VAL);

    allPrimitives.setInt8(INT8_VAL);
    EXPECT_TRUE(allPrimitives.getInt8() == INT8_VAL);

    allPrimitives.setInt16(INT16_VAL);
    EXPECT_TRUE(allPrimitives.getInt16() == INT16_VAL);

    allPrimitives.setInt32(INT32_VAL);
    EXPECT_TRUE(allPrimitives.getInt32() == INT32_VAL);

    allPrimitives.setInt64(INT64_VAL);
    EXPECT_TRUE(allPrimitives.getInt64() == INT64_VAL);

    allPrimitives.setFloat(FLOAT_VAL);
    EXPECT_TRUE(allPrimitives.getFloat() == FLOAT_VAL);

    allPrimitives.setString(STRING_VAL);
    EXPECT_TRUE(EA::StdC::Strcmp(allPrimitives.getString(), STRING_VAL) == 0);

    allPrimitives.setOutOfClassEnum(EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2);
    EXPECT_TRUE(allPrimitives.getOutOfClassEnum() == EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2);
}


TEST(SetGetComplex, BaseClass)
{
    EA::TDF::Test::AllComplexClass allComplex;
    
    allComplex.getClass().setBool(true);
    EXPECT_TRUE(allComplex.getClass().getBool() == true);

    allComplex.getUnion().setBool(true);
    EXPECT_TRUE(allComplex.getUnion().getBool() == true);

    allComplex.getBitField().setBits(BITFIELD_VAL);
    EXPECT_TRUE(allComplex.getBitField().getBits() == BITFIELD_VAL);

    allComplex.getBlob().setData(BLOB_VAL, BLOB_VAL_SIZE);
    EXPECT_TRUE(memcmp(allComplex.getBlob().getData(), BLOB_VAL, BLOB_VAL_SIZE) == 0);

    allComplex.setObjectType(OBJECT_TYPE_VAL);
    EXPECT_TRUE(allComplex.getObjectType() == OBJECT_TYPE_VAL);

    allComplex.setObjectId(OBJECT_ID_VAL);
    EXPECT_TRUE(allComplex.getObjectId() == OBJECT_ID_VAL);
    
    allComplex.setTime(TIME_VAL);
    EXPECT_TRUE(allComplex.getTime() == TIME_VAL);

    EA::TDF::Test::AllPrimitivesClass allPrims;
    allComplex.setVariable(allPrims);
    EXPECT_TRUE(allComplex.getVariable() == &allPrims);
}


TEST(SetGetComplex, Union)
{
    EA::TDF::Test::AllComplexUnion allComplex;

    allComplex.setClass(nullptr);
    allComplex.getClass()->setBool(true);
    EXPECT_TRUE(allComplex.getClass()->getBool() == true);

    allComplex.setUnion(nullptr);
    allComplex.getUnion()->setBool(true);
    EXPECT_TRUE(allComplex.getUnion()->getBool() == true);

    EA::TDF::Test::ABitField bits;
    bits.setBits(BITFIELD_VAL);
    allComplex.setBitField(&bits);
    EXPECT_TRUE(allComplex.getBitField()->getBits() == BITFIELD_VAL);

    EA::TDF::TdfBlob blob;
    blob.setData(BLOB_VAL, BLOB_VAL_SIZE);
    allComplex.setBlob(&blob);
    EXPECT_TRUE(memcmp(allComplex.getBlob()->getData(), BLOB_VAL, BLOB_VAL_SIZE) == 0);

    allComplex.setObjectType(OBJECT_TYPE_VAL);
    EXPECT_TRUE(*allComplex.getObjectType() == OBJECT_TYPE_VAL);

    allComplex.setObjectId(OBJECT_ID_VAL);
    EXPECT_TRUE(*allComplex.getObjectId() == OBJECT_ID_VAL);
    
    allComplex.setTime(TIME_VAL);
    EXPECT_TRUE(allComplex.getTime() == TIME_VAL);

    EA::TDF::Test::AllPrimitivesClass allPrims;
    allComplex.switchActiveMember(EA::TDF::Test::AllComplexUnion::MEMBER_VARIABLE);
    allComplex.getVariable()->set(allPrims);
    EXPECT_TRUE(allComplex.getVariable()->get() == &allPrims);
}



TEST(SetGetComplex, UnionAllocInPlace)
{
    EA::TDF::Test::AllComplexUnionAllocInPlace allComplex;

    allComplex.setClass(nullptr);
    allComplex.getClass()->setBool(true);
    EXPECT_TRUE(allComplex.getClass()->getBool() == true);

    allComplex.setUnion(nullptr);
    allComplex.getUnion()->setBool(true);
    EXPECT_TRUE(allComplex.getUnion()->getBool() == true);

    EA::TDF::Test::ABitField bits;
    bits.setBits(BITFIELD_VAL);
    allComplex.setBitField(&bits);
    EXPECT_TRUE(allComplex.getBitField()->getBits() == BITFIELD_VAL);

    EA::TDF::TdfBlob blob;
    blob.setData(BLOB_VAL, BLOB_VAL_SIZE);
    allComplex.setBlob(&blob);
    EXPECT_TRUE(memcmp(allComplex.getBlob()->getData(), BLOB_VAL, BLOB_VAL_SIZE) == 0);

    allComplex.setObjectType(OBJECT_TYPE_VAL);
    EXPECT_TRUE(*allComplex.getObjectType() == OBJECT_TYPE_VAL);

    allComplex.setObjectId(OBJECT_ID_VAL);
    EXPECT_TRUE(*allComplex.getObjectId() == OBJECT_ID_VAL);

    allComplex.setTime(TIME_VAL);
    EXPECT_TRUE(allComplex.getTime() == TIME_VAL);

    EA::TDF::Test::AllPrimitivesClass allPrims;
    allComplex.switchActiveMember(EA::TDF::Test::AllComplexUnion::MEMBER_VARIABLE);
    allComplex.getVariable()->set(allPrims);
    EXPECT_TRUE(allComplex.getVariable()->get() == &allPrims);
}


TEST(SetGetListPrimitives, BaseClass)
{
    EA::TDF::Test::AllListsPrimitivesClass allPrimitives;

    allPrimitives.getBool().push_back(true);
    EXPECT_TRUE(allPrimitives.getBool()[0] == true);

    allPrimitives.getUInt8().push_back(UINT8_VAL);
    EXPECT_TRUE(allPrimitives.getUInt8()[0] == UINT8_VAL);

    allPrimitives.getUInt16().push_back(UINT16_VAL);
    EXPECT_TRUE(allPrimitives.getUInt16()[0] == UINT16_VAL);

    allPrimitives.getUInt32().push_back(UINT32_VAL);
    EXPECT_TRUE(allPrimitives.getUInt32()[0] == UINT32_VAL);

    allPrimitives.getUInt64().push_back(UINT64_VAL);
    EXPECT_TRUE(allPrimitives.getUInt64()[0] == UINT64_VAL);

    allPrimitives.getInt8().push_back(INT8_VAL);
    EXPECT_TRUE(allPrimitives.getInt8()[0] == INT8_VAL);

    allPrimitives.getInt16().push_back(INT16_VAL);
    EXPECT_TRUE(allPrimitives.getInt16()[0] == INT16_VAL);

    allPrimitives.getInt32().push_back(INT32_VAL);
    EXPECT_TRUE(allPrimitives.getInt32()[0] == INT32_VAL);

    allPrimitives.getInt64().push_back(INT64_VAL);
    EXPECT_TRUE(allPrimitives.getInt64()[0] == INT64_VAL);

    allPrimitives.getFloat().push_back(FLOAT_VAL);
    EXPECT_TRUE(allPrimitives.getFloat()[0] == FLOAT_VAL);
    
    allPrimitives.getString().push_back(STRING_VAL);
    EXPECT_TRUE(EA::StdC::Strcmp(allPrimitives.getString()[0], STRING_VAL) == 0);

    allPrimitives.getOutOfClassEnum().push_back(EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2);
    EXPECT_TRUE(allPrimitives.getOutOfClassEnum()[0] == EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2);

    allPrimitives.getInClassEnum().push_back(EA::TDF::Test::AllListsPrimitivesClass::IN_OF_CLASS_ENUM_VAL_2);
    EXPECT_TRUE(allPrimitives.getInClassEnum()[0] == EA::TDF::Test::AllListsPrimitivesClass::IN_OF_CLASS_ENUM_VAL_2);
}



TEST(SetGetListPrimitives, Union)
{
    EA::TDF::Test::AllListsPrimitivesUnion allPrimitives;

    allPrimitives.setBool(nullptr);
    allPrimitives.getBool()->push_back(true);
    EXPECT_TRUE((*allPrimitives.getBool())[0] == true);

    allPrimitives.setUInt8(nullptr);
    allPrimitives.getUInt8()->push_back(UINT8_VAL);
    EXPECT_TRUE((*allPrimitives.getUInt8())[0] == UINT8_VAL);

    allPrimitives.setUInt16(nullptr);
    allPrimitives.getUInt16()->push_back(UINT16_VAL);
    EXPECT_TRUE((*allPrimitives.getUInt16())[0] == UINT16_VAL);

    allPrimitives.setUInt32(nullptr);
    allPrimitives.getUInt32()->push_back(UINT32_VAL);
    EXPECT_TRUE((*allPrimitives.getUInt32())[0] == UINT32_VAL);

    allPrimitives.setUInt64(nullptr);
    allPrimitives.getUInt64()->push_back(UINT64_VAL);
    EXPECT_TRUE((*allPrimitives.getUInt64())[0] == UINT64_VAL);

    allPrimitives.setInt8(nullptr);
    allPrimitives.getInt8()->push_back(INT8_VAL);
    EXPECT_TRUE((*allPrimitives.getInt8())[0] == INT8_VAL);

    allPrimitives.setInt16(nullptr);
    allPrimitives.getInt16()->push_back(INT16_VAL);
    EXPECT_TRUE((*allPrimitives.getInt16())[0] == INT16_VAL);

    allPrimitives.setInt32(nullptr);
    allPrimitives.getInt32()->push_back(INT32_VAL);
    EXPECT_TRUE((*allPrimitives.getInt32())[0] == INT32_VAL);

    allPrimitives.setInt64(nullptr);
    allPrimitives.getInt64()->push_back(INT64_VAL);
    EXPECT_TRUE((*allPrimitives.getInt64())[0] == INT64_VAL);

    allPrimitives.setFloat(nullptr);
    allPrimitives.getFloat()->push_back(FLOAT_VAL);
    EXPECT_TRUE((*allPrimitives.getFloat())[0] == FLOAT_VAL);

    allPrimitives.setString(nullptr);
    allPrimitives.getString()->push_back(STRING_VAL);
    EXPECT_TRUE(EA::StdC::Strcmp((*allPrimitives.getString())[0], STRING_VAL) == 0);

    allPrimitives.setOutOfClassEnum(nullptr);
    allPrimitives.getOutOfClassEnum()->push_back(EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2);
    EXPECT_TRUE((*allPrimitives.getOutOfClassEnum())[0] == EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2);
}



TEST(SetGetListPrimitives, UnionAllocInPlace)
{
    EA::TDF::Test::AllListsPrimitivesUnionAllocInPlace allPrimitives;

    allPrimitives.setBool(nullptr);
    allPrimitives.getBool()->push_back(true);
    EXPECT_TRUE((*allPrimitives.getBool())[0] == true);

    allPrimitives.setUInt8(nullptr);
    allPrimitives.getUInt8()->push_back(UINT8_VAL);
    EXPECT_TRUE((*allPrimitives.getUInt8())[0] == UINT8_VAL);

    allPrimitives.setUInt16(nullptr);
    allPrimitives.getUInt16()->push_back(UINT16_VAL);
    EXPECT_TRUE((*allPrimitives.getUInt16())[0] == UINT16_VAL);

    allPrimitives.setUInt32(nullptr);
    allPrimitives.getUInt32()->push_back(UINT32_VAL);
    EXPECT_TRUE((*allPrimitives.getUInt32())[0] == UINT32_VAL);

    allPrimitives.setUInt64(nullptr);
    allPrimitives.getUInt64()->push_back(UINT64_VAL);
    EXPECT_TRUE((*allPrimitives.getUInt64())[0] == UINT64_VAL);

    allPrimitives.setInt8(nullptr);
    allPrimitives.getInt8()->push_back(INT8_VAL);
    EXPECT_TRUE((*allPrimitives.getInt8())[0] == INT8_VAL);

    allPrimitives.setInt16(nullptr);
    allPrimitives.getInt16()->push_back(INT16_VAL);
    EXPECT_TRUE((*allPrimitives.getInt16())[0] == INT16_VAL);

    allPrimitives.setInt32(nullptr);
    allPrimitives.getInt32()->push_back(INT32_VAL);
    EXPECT_TRUE((*allPrimitives.getInt32())[0] == INT32_VAL);

    allPrimitives.setInt64(nullptr);
    allPrimitives.getInt64()->push_back(INT64_VAL);
    EXPECT_TRUE((*allPrimitives.getInt64())[0] == INT64_VAL);

    allPrimitives.setFloat(nullptr);
    allPrimitives.getFloat()->push_back(FLOAT_VAL);
    EXPECT_TRUE((*allPrimitives.getFloat())[0] == FLOAT_VAL);

    allPrimitives.setString(nullptr);
    allPrimitives.getString()->push_back(STRING_VAL);
    EXPECT_TRUE(EA::StdC::Strcmp((*allPrimitives.getString())[0], STRING_VAL) == 0);

    allPrimitives.setOutOfClassEnum(nullptr);
    allPrimitives.getOutOfClassEnum()->push_back(EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2);
    EXPECT_TRUE((*allPrimitives.getOutOfClassEnum())[0] == EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2);
}


TEST(SetGetListComplex, BaseClass)
{
    EA::TDF::Test::AllListsComplexClass allComplex;

    allComplex.getClass().pull_back()->setBool(true);
    EXPECT_TRUE(allComplex.getClass()[0]->getBool() == true);

    allComplex.getUnion().pull_back()->setBool(true);
    EXPECT_TRUE(allComplex.getUnion()[0]->getBool() == true);

    EA::TDF::Test::ABitField bits;
    bits.setBits(BITFIELD_VAL);
    allComplex.getBitField().push_back(bits);
    EXPECT_TRUE(allComplex.getBitField()[0].getBits() == BITFIELD_VAL);
    
    allComplex.getBlob().pull_back()->setData(BLOB_VAL, BLOB_VAL_SIZE);
    EXPECT_TRUE(memcmp(allComplex.getBlob()[0]->getData(), BLOB_VAL, BLOB_VAL_SIZE) == 0);

    allComplex.getObjectType().push_back(OBJECT_TYPE_VAL);
    EXPECT_TRUE(allComplex.getObjectType()[0] == OBJECT_TYPE_VAL);

    allComplex.getObjectId().push_back(OBJECT_ID_VAL);
    EXPECT_TRUE(allComplex.getObjectId()[0] == OBJECT_ID_VAL);

    allComplex.getTime().push_back(TIME_VAL);
    EXPECT_TRUE(allComplex.getTime()[0] == TIME_VAL);
   
    EA::TDF::Test::AllPrimitivesClass allPrims;
    EA::TDF::VariableTdfBase* var = allComplex.getVariable().pull_back();
    var->set(allPrims);
    EXPECT_TRUE(allComplex.getVariable()[0]->get() == &allPrims);

    allComplex.getListBool().pull_back()->push_back(true);
    EXPECT_TRUE(allComplex.getListBool().at(0)->at(0) == true);

    allComplex.getListClass().pull_back()->pull_back()->setBool(true);
    EXPECT_TRUE(allComplex.getListClass().at(0)->at(0)->getBool() == true);

    (*allComplex.getMapIntBool().pull_back())[MAP_INT_KEY] = true;
    EXPECT_TRUE((*allComplex.getMapIntBool()[0])[MAP_INT_KEY] == true);

    (*allComplex.getMapStringBool().pull_back())[MAP_STRING_KEY] = true;
    EXPECT_TRUE((*allComplex.getMapStringBool()[0])[MAP_STRING_KEY] == true);

    EA::TDF::Test::AllPrimitivesClass* c = allComplex.getMapIntClass().pull_back()->allocate_element();
    c->setBool(true);
    (*allComplex.getMapIntClass()[0])[MAP_INT_KEY] = c;
    EXPECT_TRUE((*allComplex.getMapIntClass()[0])[MAP_INT_KEY]->getBool() == true);

    c = allComplex.getMapStringClass().pull_back()->allocate_element();
    c->setBool(true);
    (*allComplex.getMapStringClass()[0])[MAP_STRING_KEY] = c;
    EXPECT_TRUE((*allComplex.getMapStringClass()[0])[MAP_STRING_KEY]->getBool() == true);
}


TEST(SetGetListComplex, Union)
{
    EA::TDF::Test::AllListsComplexUnion allComplex;

    allComplex.setClass(nullptr);
    allComplex.getClass()->pull_back()->setBool(true);
    EXPECT_TRUE((*allComplex.getClass())[0]->getBool() == true);

    allComplex.setUnion(nullptr);
    allComplex.getUnion()->pull_back()->setBool(true);
    EXPECT_TRUE((*allComplex.getUnion())[0]->getBool() == true);

    allComplex.setBitField(nullptr);
    EA::TDF::Test::ABitField bits;
    bits.setBits(BITFIELD_VAL);
    allComplex.getBitField()->push_back(bits);
    EXPECT_TRUE((*allComplex.getBitField())[0].getBits() == BITFIELD_VAL);

    allComplex.setBlob(nullptr);
    allComplex.getBlob()->pull_back()->setData(BLOB_VAL, BLOB_VAL_SIZE);
    EXPECT_TRUE(memcmp((*allComplex.getBlob())[0]->getData(), BLOB_VAL, BLOB_VAL_SIZE) == 0);

    allComplex.setObjectType(nullptr);
    allComplex.getObjectType()->push_back(OBJECT_TYPE_VAL);
    EXPECT_TRUE((*allComplex.getObjectType())[0] == OBJECT_TYPE_VAL);

    allComplex.setObjectId(nullptr);
    allComplex.getObjectId()->push_back(OBJECT_ID_VAL);
    EXPECT_TRUE((*allComplex.getObjectId())[0] == OBJECT_ID_VAL);

    allComplex.setTime(nullptr);
    allComplex.getTime()->push_back(TIME_VAL);
    EXPECT_TRUE((*allComplex.getTime())[0] == TIME_VAL);

    allComplex.setVariable(nullptr);
    EA::TDF::Test::AllPrimitivesClass allPrims;
    EA::TDF::VariableTdfBase* var = allComplex.getVariable()->pull_back();
    var->set(allPrims);
    EXPECT_TRUE((*allComplex.getVariable())[0]->get() == &allPrims);

    allComplex.setListBool(nullptr);
    allComplex.getListBool()->pull_back()->push_back(true);
    EXPECT_TRUE(allComplex.getListBool()->at(0)->at(0) == true);

    allComplex.setListClass(nullptr);
    allComplex.getListClass()->pull_back()->pull_back()->setBool(true);
    EXPECT_TRUE(allComplex.getListClass()->at(0)->at(0)->getBool() == true);

    allComplex.setMapIntBool(nullptr);
    (*allComplex.getMapIntBool()->pull_back())[MAP_INT_KEY] = true;
    EXPECT_TRUE((*allComplex.getMapIntBool()->at(0))[MAP_INT_KEY] == true);

    allComplex.setMapStringBool(nullptr);
    (*allComplex.getMapStringBool()->pull_back())[MAP_STRING_KEY] = true;
    EXPECT_TRUE((*allComplex.getMapStringBool()->at(0))[MAP_STRING_KEY] == true);

    allComplex.setMapIntClass(nullptr);
    EA::TDF::Test::AllPrimitivesClass* c = allComplex.getMapIntClass()->pull_back()->allocate_element();
    c->setBool(true);
    (*allComplex.getMapIntClass()->at(0))[MAP_INT_KEY] = c;
    EXPECT_TRUE((*allComplex.getMapIntClass()->at(0))[MAP_INT_KEY]->getBool() == true);
    
    allComplex.setMapStringClass(nullptr);
    c = allComplex.getMapStringClass()->pull_back()->allocate_element();
    c->setBool(true);
    (*allComplex.getMapStringClass()->at(0))[MAP_STRING_KEY] = c;
    EXPECT_TRUE((*allComplex.getMapStringClass()->at(0))[MAP_STRING_KEY]->getBool() == true);
}


TEST(SetGetListComplex, UnionAllocInPlace)
{
    EA::TDF::Test::AllListsComplexUnionAllocInPlace allComplex;

    allComplex.setClass(nullptr);
    allComplex.getClass()->pull_back()->setBool(true);
    EXPECT_TRUE((*allComplex.getClass())[0]->getBool() == true);

    allComplex.setUnion(nullptr);
    allComplex.getUnion()->pull_back()->setBool(true);
    EXPECT_TRUE((*allComplex.getUnion())[0]->getBool() == true);

    allComplex.setBitField(nullptr);
    EA::TDF::Test::ABitField bits;
    bits.setBits(BITFIELD_VAL);
    allComplex.getBitField()->push_back(bits);
    EXPECT_TRUE((*allComplex.getBitField())[0].getBits() == BITFIELD_VAL);

    allComplex.setBlob(nullptr);
    allComplex.getBlob()->pull_back()->setData(BLOB_VAL, BLOB_VAL_SIZE);
    EXPECT_TRUE(memcmp((*allComplex.getBlob())[0]->getData(), BLOB_VAL, BLOB_VAL_SIZE) == 0);

    allComplex.setObjectType(nullptr);
    allComplex.getObjectType()->push_back(OBJECT_TYPE_VAL);
    EXPECT_TRUE((*allComplex.getObjectType())[0] == OBJECT_TYPE_VAL);

    allComplex.setObjectId(nullptr);
    allComplex.getObjectId()->push_back(OBJECT_ID_VAL);
    EXPECT_TRUE((*allComplex.getObjectId())[0] == OBJECT_ID_VAL);

    allComplex.setTime(nullptr);
    allComplex.getTime()->push_back(TIME_VAL);
    EXPECT_TRUE((*allComplex.getTime())[0] == TIME_VAL);

    allComplex.setVariable(nullptr);
    EA::TDF::Test::AllPrimitivesClass allPrims;
    EA::TDF::VariableTdfBase* var = allComplex.getVariable()->pull_back();
    var->set(allPrims);
    EXPECT_TRUE((*allComplex.getVariable())[0]->get() == &allPrims);

    allComplex.setListBool(nullptr);
    allComplex.getListBool()->pull_back()->push_back(true);
    EXPECT_TRUE(allComplex.getListBool()->at(0)->at(0) == true);

    allComplex.setListClass(nullptr);
    allComplex.getListClass()->pull_back()->pull_back()->setBool(true);
    EXPECT_TRUE(allComplex.getListClass()->at(0)->at(0)->getBool() == true);

    allComplex.setMapIntBool(nullptr);
    (*allComplex.getMapIntBool()->pull_back())[MAP_INT_KEY] = true;
    EXPECT_TRUE((*allComplex.getMapIntBool()->at(0))[MAP_INT_KEY] == true);

    allComplex.setMapStringBool(nullptr);
    (*allComplex.getMapStringBool()->pull_back())[MAP_STRING_KEY] = true;
    EXPECT_TRUE((*allComplex.getMapStringBool()->at(0))[MAP_STRING_KEY] == true);

    allComplex.setMapIntClass(nullptr);
    EA::TDF::Test::AllPrimitivesClass* c = allComplex.getMapIntClass()->pull_back()->allocate_element();
    c->setBool(true);
    (*allComplex.getMapIntClass()->at(0))[MAP_INT_KEY] = c;
    EXPECT_TRUE((*allComplex.getMapIntClass()->at(0))[MAP_INT_KEY]->getBool() == true);

    allComplex.setMapStringClass(nullptr);
    c = allComplex.getMapStringClass()->pull_back()->allocate_element();
    c->setBool(true);
    (*allComplex.getMapStringClass()->at(0))[MAP_STRING_KEY] = c;
    EXPECT_TRUE((*allComplex.getMapStringClass()->at(0))[MAP_STRING_KEY]->getBool() == true);
}



TEST(SetGetMapPrimitives, BaseClass)
{
    EA::TDF::Test::AllMapsPrimitivesClass allPrimitives;

    allPrimitives.getIntBool()[MAP_INT_KEY] = true;
    EXPECT_TRUE(allPrimitives.getIntBool()[MAP_INT_KEY] == true);

    allPrimitives.getStringBool()[MAP_STRING_KEY] = true;
    EXPECT_TRUE(allPrimitives.getStringBool()[MAP_STRING_KEY] == true);
    
    allPrimitives.getIntUInt8()[MAP_INT_KEY] = UINT8_VAL;
    EXPECT_TRUE(allPrimitives.getIntUInt8()[MAP_INT_KEY] == UINT8_VAL);

    allPrimitives.getStringUInt8()[MAP_STRING_KEY] = UINT8_VAL;
    EXPECT_TRUE(allPrimitives.getStringUInt8()[MAP_STRING_KEY] == UINT8_VAL);
    
    allPrimitives.getIntUInt16()[MAP_INT_KEY] = UINT16_VAL;
    EXPECT_TRUE(allPrimitives.getIntUInt16()[MAP_INT_KEY] == UINT16_VAL);

    allPrimitives.getStringUInt16()[MAP_STRING_KEY] = UINT16_VAL;
    EXPECT_TRUE(allPrimitives.getStringUInt16()[MAP_STRING_KEY] == UINT16_VAL);
    
    allPrimitives.getIntUInt32()[MAP_INT_KEY] = UINT32_VAL;
    EXPECT_TRUE(allPrimitives.getIntUInt32()[MAP_INT_KEY] == UINT32_VAL);

    allPrimitives.getStringUInt32()[MAP_STRING_KEY] = UINT32_VAL;
    EXPECT_TRUE(allPrimitives.getStringUInt32()[MAP_STRING_KEY] == UINT32_VAL);
    
    allPrimitives.getIntUInt64()[MAP_INT_KEY] = UINT64_VAL;
    EXPECT_TRUE(allPrimitives.getIntUInt64()[MAP_INT_KEY] == UINT64_VAL);

    allPrimitives.getStringUInt64()[MAP_STRING_KEY] = UINT64_VAL;
    EXPECT_TRUE(allPrimitives.getStringUInt64()[MAP_STRING_KEY] == UINT64_VAL);

    allPrimitives.getIntInt8()[MAP_INT_KEY] = INT8_VAL;
    EXPECT_TRUE(allPrimitives.getIntInt8()[MAP_INT_KEY] == INT8_VAL);

    allPrimitives.getStringInt8()[MAP_STRING_KEY] = INT8_VAL;
    EXPECT_TRUE(allPrimitives.getStringInt8()[MAP_STRING_KEY] == INT8_VAL);

    allPrimitives.getIntInt16()[MAP_INT_KEY] = INT16_VAL;
    EXPECT_TRUE(allPrimitives.getIntInt16()[MAP_INT_KEY] == INT16_VAL);

    allPrimitives.getStringInt16()[MAP_STRING_KEY] = INT16_VAL;
    EXPECT_TRUE(allPrimitives.getStringInt16()[MAP_STRING_KEY] == INT16_VAL);

    allPrimitives.getIntInt32()[MAP_INT_KEY] = INT32_VAL;
    EXPECT_TRUE(allPrimitives.getIntInt32()[MAP_INT_KEY] == INT32_VAL);

    allPrimitives.getStringInt32()[MAP_STRING_KEY] = INT32_VAL;
    EXPECT_TRUE(allPrimitives.getStringInt32()[MAP_STRING_KEY] == INT32_VAL);

    allPrimitives.getIntInt64()[MAP_INT_KEY] = INT64_VAL;
    EXPECT_TRUE(allPrimitives.getIntInt64()[MAP_INT_KEY] == INT64_VAL);

    allPrimitives.getStringInt64()[MAP_STRING_KEY] = INT64_VAL;
    EXPECT_TRUE(allPrimitives.getStringInt64()[MAP_STRING_KEY] == INT64_VAL);

    allPrimitives.getIntFloat()[MAP_INT_KEY] = FLOAT_VAL;
    EXPECT_TRUE(allPrimitives.getIntFloat()[MAP_INT_KEY] == FLOAT_VAL);
    
    allPrimitives.getStringFloat()[MAP_STRING_KEY] = FLOAT_VAL;
    EXPECT_TRUE(allPrimitives.getStringFloat()[MAP_STRING_KEY] == FLOAT_VAL);

    allPrimitives.getIntString()[MAP_INT_KEY] = STRING_VAL;
    EXPECT_TRUE(EA::StdC::Strcmp(allPrimitives.getIntString()[MAP_INT_KEY], STRING_VAL) == 0);

    allPrimitives.getStringString()[MAP_STRING_KEY] = STRING_VAL;
    EXPECT_TRUE(EA::StdC::Strcmp(allPrimitives.getStringString()[MAP_STRING_KEY], STRING_VAL) == 0);

    allPrimitives.getIntOutOfClassEnum()[MAP_INT_KEY] = EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2;
    EXPECT_TRUE(allPrimitives.getIntOutOfClassEnum()[MAP_INT_KEY] == EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2);

    allPrimitives.getStringOutOfClassEnum()[MAP_STRING_KEY] = EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2;
    EXPECT_TRUE(allPrimitives.getStringOutOfClassEnum()[MAP_STRING_KEY] == EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2);

    allPrimitives.getIntInClassEnum()[MAP_INT_KEY] = EA::TDF::Test::AllMapsPrimitivesClass::IN_OF_CLASS_ENUM_VAL_2;
    EXPECT_TRUE(allPrimitives.getIntInClassEnum()[MAP_INT_KEY] == EA::TDF::Test::AllMapsPrimitivesClass::IN_OF_CLASS_ENUM_VAL_2);

    allPrimitives.getStringInClassEnum()[MAP_STRING_KEY] = EA::TDF::Test::AllMapsPrimitivesClass::IN_OF_CLASS_ENUM_VAL_2;
    EXPECT_TRUE(allPrimitives.getStringInClassEnum()[MAP_STRING_KEY] == EA::TDF::Test::AllMapsPrimitivesClass::IN_OF_CLASS_ENUM_VAL_2);
}


TEST(SetGetMapPrimitives, Union)
{
    EA::TDF::Test::AllMapsPrimitivesUnion allPrimitives;

    allPrimitives.setIntBool(nullptr);
    (*allPrimitives.getIntBool())[MAP_INT_KEY] = true;
    EXPECT_TRUE((*allPrimitives.getIntBool())[MAP_INT_KEY] == true);

    allPrimitives.setStringBool(nullptr);
    (*allPrimitives.getStringBool())[MAP_STRING_KEY] = true;
    EXPECT_TRUE((*allPrimitives.getStringBool())[MAP_STRING_KEY] == true);

    allPrimitives.setIntUInt8(nullptr);
    (*allPrimitives.getIntUInt8())[MAP_INT_KEY] = UINT8_VAL;
    EXPECT_TRUE((*allPrimitives.getIntUInt8())[MAP_INT_KEY] == UINT8_VAL);

    allPrimitives.setStringUInt8(nullptr);
    (*allPrimitives.getStringUInt8())[MAP_STRING_KEY] = UINT8_VAL;
    EXPECT_TRUE((*allPrimitives.getStringUInt8())[MAP_STRING_KEY] == UINT8_VAL);

    allPrimitives.setIntUInt16(nullptr);
    (*allPrimitives.getIntUInt16())[MAP_INT_KEY] = UINT16_VAL;
    EXPECT_TRUE((*allPrimitives.getIntUInt16())[MAP_INT_KEY] == UINT16_VAL);

    allPrimitives.setStringUInt16(nullptr);
    (*allPrimitives.getStringUInt16())[MAP_STRING_KEY] = UINT16_VAL;
    EXPECT_TRUE((*allPrimitives.getStringUInt16())[MAP_STRING_KEY] == UINT16_VAL);

    allPrimitives.setIntUInt32(nullptr);
    (*allPrimitives.getIntUInt32())[MAP_INT_KEY] = UINT32_VAL;
    EXPECT_TRUE((*allPrimitives.getIntUInt32())[MAP_INT_KEY] == UINT32_VAL);

    allPrimitives.setStringUInt32(nullptr);
    (*allPrimitives.getStringUInt32())[MAP_STRING_KEY] = UINT32_VAL;
    EXPECT_TRUE((*allPrimitives.getStringUInt32())[MAP_STRING_KEY] == UINT32_VAL);

    allPrimitives.setIntUInt64(nullptr);
    (*allPrimitives.getIntUInt64())[MAP_INT_KEY] = UINT64_VAL;
    EXPECT_TRUE((*allPrimitives.getIntUInt64())[MAP_INT_KEY] == UINT64_VAL);

    allPrimitives.setStringUInt64(nullptr);
    (*allPrimitives.getStringUInt64())[MAP_STRING_KEY] = UINT64_VAL;
    EXPECT_TRUE((*allPrimitives.getStringUInt64())[MAP_STRING_KEY] == UINT64_VAL);

    allPrimitives.setIntInt8(nullptr);
    (*allPrimitives.getIntInt8())[MAP_INT_KEY] = INT8_VAL;
    EXPECT_TRUE((*allPrimitives.getIntInt8())[MAP_INT_KEY] == INT8_VAL);

    allPrimitives.setStringInt8(nullptr);
    (*allPrimitives.getStringInt8())[MAP_STRING_KEY] = INT8_VAL;
    EXPECT_TRUE((*allPrimitives.getStringInt8())[MAP_STRING_KEY] == INT8_VAL);

    allPrimitives.setIntInt16(nullptr);
    (*allPrimitives.getIntInt16())[MAP_INT_KEY] = INT16_VAL;
    EXPECT_TRUE((*allPrimitives.getIntInt16())[MAP_INT_KEY] == INT16_VAL);

    allPrimitives.setStringInt16(nullptr);
    (*allPrimitives.getStringInt16())[MAP_STRING_KEY] = INT16_VAL;
    EXPECT_TRUE((*allPrimitives.getStringInt16())[MAP_STRING_KEY] == INT16_VAL);

    allPrimitives.setIntInt32(nullptr);
    (*allPrimitives.getIntInt32())[MAP_INT_KEY] = INT32_VAL;
    EXPECT_TRUE((*allPrimitives.getIntInt32())[MAP_INT_KEY] == INT32_VAL);

    allPrimitives.setStringInt32(nullptr);
    (*allPrimitives.getStringInt32())[MAP_STRING_KEY] = INT32_VAL;
    EXPECT_TRUE((*allPrimitives.getStringInt32())[MAP_STRING_KEY] == INT32_VAL);

    allPrimitives.setIntInt64(nullptr);
    (*allPrimitives.getIntInt64())[MAP_INT_KEY] = INT64_VAL;
    EXPECT_TRUE((*allPrimitives.getIntInt64())[MAP_INT_KEY] == INT64_VAL);

    allPrimitives.setStringInt64(nullptr);
    (*allPrimitives.getStringInt64())[MAP_STRING_KEY] = INT64_VAL;
    EXPECT_TRUE((*allPrimitives.getStringInt64())[MAP_STRING_KEY] == INT64_VAL);

    allPrimitives.setIntFloat(nullptr);
    (*allPrimitives.getIntFloat())[MAP_INT_KEY] = FLOAT_VAL;
    EXPECT_TRUE((*allPrimitives.getIntFloat())[MAP_INT_KEY] == FLOAT_VAL);

    allPrimitives.setStringFloat(nullptr);
    (*allPrimitives.getStringFloat())[MAP_STRING_KEY] = FLOAT_VAL;
    EXPECT_TRUE((*allPrimitives.getStringFloat())[MAP_STRING_KEY] == FLOAT_VAL);

    allPrimitives.setIntString(nullptr);
    (*allPrimitives.getIntString())[MAP_INT_KEY] = STRING_VAL;
    EXPECT_TRUE(EA::StdC::Strcmp((*allPrimitives.getIntString())[MAP_INT_KEY], STRING_VAL) == 0);

    allPrimitives.setStringString(nullptr);
    (*allPrimitives.getStringString())[MAP_STRING_KEY] = STRING_VAL;
    EXPECT_TRUE(EA::StdC::Strcmp((*allPrimitives.getStringString())[MAP_STRING_KEY], STRING_VAL) == 0);

    allPrimitives.setIntOutOfClassEnum(nullptr);
    (*allPrimitives.getIntOutOfClassEnum())[MAP_INT_KEY] = EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2;
    EXPECT_TRUE((*allPrimitives.getIntOutOfClassEnum())[MAP_INT_KEY] == EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2);

    allPrimitives.setStringOutOfClassEnum(nullptr);
    (*allPrimitives.getStringOutOfClassEnum())[MAP_STRING_KEY] = EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2;
    EXPECT_TRUE((*allPrimitives.getStringOutOfClassEnum())[MAP_STRING_KEY] == EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2);
}


TEST(SetGetMapPrimitives, UnionAllocInPlace)
{
    EA::TDF::Test::AllMapsPrimitivesUnionAllocInPlace allPrimitives;

    allPrimitives.setIntBool(nullptr);
    (*allPrimitives.getIntBool())[MAP_INT_KEY] = true;
    EXPECT_TRUE((*allPrimitives.getIntBool())[MAP_INT_KEY] == true);

    allPrimitives.setStringBool(nullptr);
    (*allPrimitives.getStringBool())[MAP_STRING_KEY] = true;
    EXPECT_TRUE((*allPrimitives.getStringBool())[MAP_STRING_KEY] == true);

    allPrimitives.setIntUInt8(nullptr);
    (*allPrimitives.getIntUInt8())[MAP_INT_KEY] = UINT8_VAL;
    EXPECT_TRUE((*allPrimitives.getIntUInt8())[MAP_INT_KEY] == UINT8_VAL);

    allPrimitives.setStringUInt8(nullptr);
    (*allPrimitives.getStringUInt8())[MAP_STRING_KEY] = UINT8_VAL;
    EXPECT_TRUE((*allPrimitives.getStringUInt8())[MAP_STRING_KEY] == UINT8_VAL);

    allPrimitives.setIntUInt16(nullptr);
    (*allPrimitives.getIntUInt16())[MAP_INT_KEY] = UINT16_VAL;
    EXPECT_TRUE((*allPrimitives.getIntUInt16())[MAP_INT_KEY] == UINT16_VAL);

    allPrimitives.setStringUInt16(nullptr);
    (*allPrimitives.getStringUInt16())[MAP_STRING_KEY] = UINT16_VAL;
    EXPECT_TRUE((*allPrimitives.getStringUInt16())[MAP_STRING_KEY] == UINT16_VAL);

    allPrimitives.setIntUInt32(nullptr);
    (*allPrimitives.getIntUInt32())[MAP_INT_KEY] = UINT32_VAL;
    EXPECT_TRUE((*allPrimitives.getIntUInt32())[MAP_INT_KEY] == UINT32_VAL);

    allPrimitives.setStringUInt32(nullptr);
    (*allPrimitives.getStringUInt32())[MAP_STRING_KEY] = UINT32_VAL;
    EXPECT_TRUE((*allPrimitives.getStringUInt32())[MAP_STRING_KEY] == UINT32_VAL);

    allPrimitives.setIntUInt64(nullptr);
    (*allPrimitives.getIntUInt64())[MAP_INT_KEY] = UINT64_VAL;
    EXPECT_TRUE((*allPrimitives.getIntUInt64())[MAP_INT_KEY] == UINT64_VAL);

    allPrimitives.setStringUInt64(nullptr);
    (*allPrimitives.getStringUInt64())[MAP_STRING_KEY] = UINT64_VAL;
    EXPECT_TRUE((*allPrimitives.getStringUInt64())[MAP_STRING_KEY] == UINT64_VAL);

    allPrimitives.setIntInt8(nullptr);
    (*allPrimitives.getIntInt8())[MAP_INT_KEY] = INT8_VAL;
    EXPECT_TRUE((*allPrimitives.getIntInt8())[MAP_INT_KEY] == INT8_VAL);

    allPrimitives.setStringInt8(nullptr);
    (*allPrimitives.getStringInt8())[MAP_STRING_KEY] = INT8_VAL;
    EXPECT_TRUE((*allPrimitives.getStringInt8())[MAP_STRING_KEY] == INT8_VAL);

    allPrimitives.setIntInt16(nullptr);
    (*allPrimitives.getIntInt16())[MAP_INT_KEY] = INT16_VAL;
    EXPECT_TRUE((*allPrimitives.getIntInt16())[MAP_INT_KEY] == INT16_VAL);

    allPrimitives.setStringInt16(nullptr);
    (*allPrimitives.getStringInt16())[MAP_STRING_KEY] = INT16_VAL;
    EXPECT_TRUE((*allPrimitives.getStringInt16())[MAP_STRING_KEY] == INT16_VAL);

    allPrimitives.setIntInt32(nullptr);
    (*allPrimitives.getIntInt32())[MAP_INT_KEY] = INT32_VAL;
    EXPECT_TRUE((*allPrimitives.getIntInt32())[MAP_INT_KEY] == INT32_VAL);

    allPrimitives.setStringInt32(nullptr);
    (*allPrimitives.getStringInt32())[MAP_STRING_KEY] = INT32_VAL;
    EXPECT_TRUE((*allPrimitives.getStringInt32())[MAP_STRING_KEY] == INT32_VAL);

    allPrimitives.setIntInt64(nullptr);
    (*allPrimitives.getIntInt64())[MAP_INT_KEY] = INT64_VAL;
    EXPECT_TRUE((*allPrimitives.getIntInt64())[MAP_INT_KEY] == INT64_VAL);

    allPrimitives.setStringInt64(nullptr);
    (*allPrimitives.getStringInt64())[MAP_STRING_KEY] = INT64_VAL;
    EXPECT_TRUE((*allPrimitives.getStringInt64())[MAP_STRING_KEY] == INT64_VAL);

    allPrimitives.setIntFloat(nullptr);
    (*allPrimitives.getIntFloat())[MAP_INT_KEY] = FLOAT_VAL;
    EXPECT_TRUE((*allPrimitives.getIntFloat())[MAP_INT_KEY] == FLOAT_VAL);

    allPrimitives.setStringFloat(nullptr);
    (*allPrimitives.getStringFloat())[MAP_STRING_KEY] = FLOAT_VAL;
    EXPECT_TRUE((*allPrimitives.getStringFloat())[MAP_STRING_KEY] == FLOAT_VAL);

    allPrimitives.setIntString(nullptr);
    (*allPrimitives.getIntString())[MAP_INT_KEY] = STRING_VAL;
    EXPECT_TRUE(EA::StdC::Strcmp((*allPrimitives.getIntString())[MAP_INT_KEY], STRING_VAL) == 0);

    allPrimitives.setStringString(nullptr);
    (*allPrimitives.getStringString())[MAP_STRING_KEY] = STRING_VAL;
    EXPECT_TRUE(EA::StdC::Strcmp((*allPrimitives.getStringString())[MAP_STRING_KEY], STRING_VAL) == 0);

    allPrimitives.setIntOutOfClassEnum(nullptr);
    (*allPrimitives.getIntOutOfClassEnum())[MAP_INT_KEY] = EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2;
    EXPECT_TRUE((*allPrimitives.getIntOutOfClassEnum())[MAP_INT_KEY] == EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2);

    allPrimitives.setStringOutOfClassEnum(nullptr);
    (*allPrimitives.getStringOutOfClassEnum())[MAP_STRING_KEY] = EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2;
    EXPECT_TRUE((*allPrimitives.getStringOutOfClassEnum())[MAP_STRING_KEY] == EA::TDF::Test::OUT_OF_CLASS_ENUM_VAL_2);
}


TEST(SetGetMapComplex, BaseClass)
{
    EA::TDF::Test::AllMapsComplexClass allComplex;
    
    EA::TDF::Test::AllPrimitivesClass* c = allComplex.getIntClass().allocate_element();
    c->setBool(true);
    allComplex.getIntClass()[MAP_INT_KEY] = c;
    EXPECT_TRUE(allComplex.getIntClass()[MAP_INT_KEY]->getBool() == true);

    c = allComplex.getStringClass().allocate_element();
    c->setBool(true);
    allComplex.getStringClass()[MAP_STRING_KEY] = c;
    EXPECT_TRUE(allComplex.getStringClass()[MAP_STRING_KEY]->getBool() == true);

    EA::TDF::Test::AllPrimitivesUnion* u = allComplex.getIntUnion().allocate_element();
    u->setBool(true);
    allComplex.getIntUnion()[MAP_INT_KEY] = u;
    EXPECT_TRUE(allComplex.getIntUnion()[MAP_INT_KEY]->getBool() == true);

    u = allComplex.getStringUnion().allocate_element();
    u->setBool(true);
    allComplex.getStringUnion()[MAP_STRING_KEY] = u;
    EXPECT_TRUE(allComplex.getStringUnion()[MAP_STRING_KEY]->getBool() == true);
    
    EA::TDF::Test::ABitField bits;
    bits.setBits(BITFIELD_VAL);
    allComplex.getIntBitField()[MAP_INT_KEY] = bits;
    EXPECT_TRUE(allComplex.getIntBitField()[MAP_INT_KEY].getBits() == BITFIELD_VAL);

    allComplex.getStringBitField()[MAP_STRING_KEY] = bits;
    EXPECT_TRUE(allComplex.getStringBitField()[MAP_STRING_KEY].getBits() == BITFIELD_VAL);

    EA::TDF::TdfBlob* blob = allComplex.getIntBlob().allocate_element();
    blob->setData(BLOB_VAL, BLOB_VAL_SIZE);
    allComplex.getIntBlob()[MAP_INT_KEY] = blob;
    EXPECT_TRUE(memcmp(allComplex.getIntBlob()[MAP_INT_KEY]->getData(), BLOB_VAL, BLOB_VAL_SIZE) == 0);

    blob = allComplex.getStringBlob().allocate_element();
    blob->setData(BLOB_VAL, BLOB_VAL_SIZE);
    allComplex.getStringBlob()[MAP_STRING_KEY] = blob;
    EXPECT_TRUE(memcmp(allComplex.getStringBlob()[MAP_STRING_KEY]->getData(), BLOB_VAL, BLOB_VAL_SIZE) == 0);

    allComplex.getIntObjectType()[MAP_INT_KEY] = OBJECT_TYPE_VAL;
    EXPECT_TRUE(allComplex.getIntObjectType()[MAP_INT_KEY] == OBJECT_TYPE_VAL);

    allComplex.getStringObjectType()[MAP_STRING_KEY] = OBJECT_TYPE_VAL;
    EXPECT_TRUE(allComplex.getStringObjectType()[MAP_STRING_KEY] == OBJECT_TYPE_VAL);

    allComplex.getIntObjectId()[MAP_INT_KEY] = OBJECT_ID_VAL;
    EXPECT_TRUE(allComplex.getIntObjectId()[MAP_INT_KEY] == OBJECT_ID_VAL);

    allComplex.getStringObjectId()[MAP_STRING_KEY] = OBJECT_ID_VAL;
    EXPECT_TRUE(allComplex.getStringObjectId()[MAP_STRING_KEY] == OBJECT_ID_VAL);

    allComplex.getIntTimeValue()[MAP_INT_KEY] = TIME_VAL;
    EXPECT_TRUE(allComplex.getIntTimeValue()[MAP_INT_KEY] == TIME_VAL);

    allComplex.getStringTimeValue()[MAP_STRING_KEY] = TIME_VAL;
    EXPECT_TRUE(allComplex.getStringTimeValue()[MAP_STRING_KEY] == TIME_VAL);

    EA::TDF::Test::AllPrimitivesClass allPrims;
    EA::TDF::VariableTdfBase* var = allComplex.getIntVariable().allocate_element();
    var->set(allPrims);
    allComplex.getIntVariable()[MAP_INT_KEY] = var;
    EXPECT_TRUE(allComplex.getIntVariable()[MAP_INT_KEY]->get() == &allPrims);

    var = allComplex.getStringVariable().allocate_element();
    var->set(allPrims);
    allComplex.getStringVariable()[MAP_STRING_KEY] = var;
    EXPECT_TRUE(allComplex.getStringVariable()[MAP_STRING_KEY]->get() == &allPrims);

    EA::TDF::Test::IntBoolListMap::ValType* boolList = allComplex.getIntBoolListMap().allocate_element();
    boolList->push_back(true);
    allComplex.getIntBoolListMap()[MAP_INT_KEY] = boolList;
    EXPECT_TRUE(allComplex.getIntBoolListMap()[MAP_INT_KEY]->at(0) == true);
    
    boolList = allComplex.getStringBoolListMap().allocate_element();
    boolList->push_back(true);
    allComplex.getStringBoolListMap()[MAP_STRING_KEY] = boolList;
    EXPECT_TRUE(allComplex.getStringBoolListMap()[MAP_STRING_KEY]->at(0) == true);

    EA::TDF::Test::IntClassListMap::ValType* classList = allComplex.getIntClassListMap().allocate_element();
    classList->pull_back()->setBool(true);
    allComplex.getIntClassListMap()[MAP_INT_KEY] = classList;
    EXPECT_TRUE(allComplex.getIntClassListMap()[MAP_INT_KEY]->at(0)->getBool() == true);

    classList = allComplex.getStringClassListMap().allocate_element();
    classList->pull_back()->setBool(true);
    allComplex.getStringClassListMap()[MAP_STRING_KEY] = classList;
    EXPECT_TRUE(allComplex.getStringClassListMap()[MAP_STRING_KEY]->at(0)->getBool() == true);

    EA::TDF::Test::IntIntBoolMapMap::ValType* intBoolMap = allComplex.getIntIntBoolMapMap().allocate_element();
    (*intBoolMap)[MAP_INT_KEY] = true;
    allComplex.getIntIntBoolMapMap()[MAP_INT_KEY] = intBoolMap;
    EXPECT_TRUE((*allComplex.getIntIntBoolMapMap()[MAP_INT_KEY])[MAP_INT_KEY] == true);

    intBoolMap = allComplex.getStringIntBoolMapMap().allocate_element();
    (*intBoolMap)[MAP_INT_KEY] = true;
    allComplex.getStringIntBoolMapMap()[MAP_STRING_KEY] = intBoolMap;
    EXPECT_TRUE((*allComplex.getStringIntBoolMapMap()[MAP_STRING_KEY])[MAP_INT_KEY] == true);

    EA::TDF::Test::IntIntClassMapMap::ValType* intClassMap = allComplex.getIntIntClassMapMap().allocate_element();
    c = intClassMap->allocate_element();
    c->setBool(true);
    (*intClassMap)[MAP_INT_KEY] = c;
    allComplex.getIntIntClassMapMap()[MAP_INT_KEY] = intClassMap;
    EXPECT_TRUE((*allComplex.getIntIntClassMapMap()[MAP_INT_KEY])[MAP_INT_KEY]->getBool() == true);

    intClassMap = allComplex.getStringIntClassMapMap().allocate_element();
    c = intClassMap->allocate_element();
    c->setBool(true);
    (*intClassMap)[MAP_INT_KEY] = c;
    allComplex.getStringIntClassMapMap()[MAP_STRING_KEY] = intClassMap;
    EXPECT_TRUE((*allComplex.getStringIntClassMapMap()[MAP_STRING_KEY])[MAP_INT_KEY]->getBool() == true);
}

TEST(SetGetMapComplex, Union)
{
    EA::TDF::Test::AllMapsComplexUnion allComplex;

    allComplex.setIntClass(nullptr);
    EA::TDF::Test::AllPrimitivesClass* c = allComplex.getIntClass()->allocate_element();
    c->setBool(true);
    (*allComplex.getIntClass())[MAP_INT_KEY] = c;
    EXPECT_TRUE((*allComplex.getIntClass())[MAP_INT_KEY]->getBool() == true);

    allComplex.setStringClass(nullptr);
    c = allComplex.getStringClass()->allocate_element();
    c->setBool(true);
    (*allComplex.getStringClass())[MAP_STRING_KEY] = c;
    EXPECT_TRUE((*allComplex.getStringClass())[MAP_STRING_KEY]->getBool() == true);

    allComplex.setIntUnion(nullptr);
    EA::TDF::Test::AllPrimitivesUnion* u = allComplex.getIntUnion()->allocate_element();
    u->setBool(true);
    (*allComplex.getIntUnion())[MAP_INT_KEY] = u;
    EXPECT_TRUE((*allComplex.getIntUnion())[MAP_INT_KEY]->getBool() == true);

    allComplex.setStringUnion(nullptr);
    u = allComplex.getStringUnion()->allocate_element();
    u->setBool(true);
    (*allComplex.getStringUnion())[MAP_STRING_KEY] = u;
    EXPECT_TRUE((*allComplex.getStringUnion())[MAP_STRING_KEY]->getBool() == true);

    allComplex.setIntBitField(nullptr);
    EA::TDF::Test::ABitField bits;
    bits.setBits(BITFIELD_VAL);
    (*allComplex.getIntBitField())[MAP_INT_KEY] = bits;
    EXPECT_TRUE((*allComplex.getIntBitField())[MAP_INT_KEY].getBits() == BITFIELD_VAL);

    allComplex.setStringBitField(nullptr);
    (*allComplex.getStringBitField())[MAP_STRING_KEY] = bits;
    EXPECT_TRUE((*allComplex.getStringBitField())[MAP_STRING_KEY].getBits() == BITFIELD_VAL);

    allComplex.setIntBlob(nullptr);
    EA::TDF::TdfBlob* blob = allComplex.getIntBlob()->allocate_element();
    blob->setData(BLOB_VAL, BLOB_VAL_SIZE);
    (*allComplex.getIntBlob())[MAP_INT_KEY] = blob;
    EXPECT_TRUE(memcmp((*allComplex.getIntBlob())[MAP_INT_KEY]->getData(), BLOB_VAL, BLOB_VAL_SIZE) == 0);

    allComplex.setStringBlob(nullptr);
    blob = allComplex.getStringBlob()->allocate_element();
    blob->setData(BLOB_VAL, BLOB_VAL_SIZE);
    (*allComplex.getStringBlob())[MAP_STRING_KEY] = blob;
    EXPECT_TRUE(memcmp((*allComplex.getStringBlob())[MAP_STRING_KEY]->getData(), BLOB_VAL, BLOB_VAL_SIZE) == 0);

    allComplex.setIntObjectType(nullptr);
    (*allComplex.getIntObjectType())[MAP_INT_KEY] = OBJECT_TYPE_VAL;
    EXPECT_TRUE((*allComplex.getIntObjectType())[MAP_INT_KEY] == OBJECT_TYPE_VAL);

    allComplex.setStringObjectType(nullptr);
    (*allComplex.getStringObjectType())[MAP_STRING_KEY] = OBJECT_TYPE_VAL;
    EXPECT_TRUE((*allComplex.getStringObjectType())[MAP_STRING_KEY] == OBJECT_TYPE_VAL);

    allComplex.setIntObjectId(nullptr);
    (*allComplex.getIntObjectId())[MAP_INT_KEY] = OBJECT_ID_VAL;
    EXPECT_TRUE((*allComplex.getIntObjectId())[MAP_INT_KEY] == OBJECT_ID_VAL);

    allComplex.setStringObjectId(nullptr);
    (*allComplex.getStringObjectId())[MAP_STRING_KEY] = OBJECT_ID_VAL;
    EXPECT_TRUE((*allComplex.getStringObjectId())[MAP_STRING_KEY] == OBJECT_ID_VAL);

    allComplex.setIntTimeValue(nullptr);
    (*allComplex.getIntTimeValue())[MAP_INT_KEY] = TIME_VAL;
    EXPECT_TRUE((*allComplex.getIntTimeValue())[MAP_INT_KEY] == TIME_VAL);

    allComplex.setStringTimeValue(nullptr);
    (*allComplex.getStringTimeValue())[MAP_STRING_KEY] = TIME_VAL;
    EXPECT_TRUE((*allComplex.getStringTimeValue())[MAP_STRING_KEY] == TIME_VAL);

    allComplex.setIntVariable(nullptr);
    EA::TDF::Test::AllPrimitivesClass allPrims;
    EA::TDF::VariableTdfBase* var = allComplex.getIntVariable()->allocate_element();
    var->set(allPrims);
    (*allComplex.getIntVariable())[MAP_INT_KEY] = var;
    EXPECT_TRUE((*allComplex.getIntVariable())[MAP_INT_KEY]->get() == &allPrims);

    allComplex.setStringVariable(nullptr);
    var = allComplex.getStringVariable()->allocate_element();
    var->set(allPrims);
    (*allComplex.getStringVariable())[MAP_STRING_KEY] = var;
    EXPECT_TRUE((*allComplex.getStringVariable())[MAP_STRING_KEY]->get() == &allPrims);

    allComplex.setIntBoolListMap(nullptr);
    EA::TDF::Test::IntBoolListMap::ValType* boolList = allComplex.getIntBoolListMap()->allocate_element();
    boolList->push_back(true);
    (*allComplex.getIntBoolListMap())[MAP_INT_KEY] = boolList;
    EXPECT_TRUE((*allComplex.getIntBoolListMap())[MAP_INT_KEY]->at(0) == true);

    allComplex.setStringBoolListMap(nullptr);
    boolList = allComplex.getStringBoolListMap()->allocate_element();
    boolList->push_back(true);
    (*allComplex.getStringBoolListMap())[MAP_STRING_KEY] = boolList;
    EXPECT_TRUE((*allComplex.getStringBoolListMap())[MAP_STRING_KEY]->at(0) == true);

    allComplex.setIntClassListMap(nullptr);
    EA::TDF::Test::IntClassListMap::ValType* classList = allComplex.getIntClassListMap()->allocate_element();
    classList->pull_back()->setBool(true);
    (*allComplex.getIntClassListMap())[MAP_INT_KEY] = classList;
    EXPECT_TRUE((*allComplex.getIntClassListMap())[MAP_INT_KEY]->at(0)->getBool() == true);

    allComplex.setStringClassListMap(nullptr);
    classList = allComplex.getStringClassListMap()->allocate_element();
    classList->pull_back()->setBool(true);
    (*allComplex.getStringClassListMap())[MAP_STRING_KEY] = classList;
    EXPECT_TRUE((*allComplex.getStringClassListMap())[MAP_STRING_KEY]->at(0)->getBool() == true);

    allComplex.setIntIntBoolMapMap(nullptr);
    EA::TDF::Test::IntIntBoolMapMap::ValType* intBoolMap = allComplex.getIntIntBoolMapMap()->allocate_element();
    (*intBoolMap)[MAP_INT_KEY] = true;
    (*allComplex.getIntIntBoolMapMap())[MAP_INT_KEY] = intBoolMap;
    EXPECT_TRUE((*(*allComplex.getIntIntBoolMapMap())[MAP_INT_KEY])[MAP_INT_KEY] == true);

    allComplex.setStringIntBoolMapMap(nullptr);
    intBoolMap = allComplex.getStringIntBoolMapMap()->allocate_element();
    (*intBoolMap)[MAP_INT_KEY] = true;
    (*allComplex.getStringIntBoolMapMap())[MAP_STRING_KEY] = intBoolMap;
    EXPECT_TRUE((*(*allComplex.getStringIntBoolMapMap())[MAP_STRING_KEY])[MAP_INT_KEY] == true);

    allComplex.setIntIntClassMapMap(nullptr);
    EA::TDF::Test::IntIntClassMapMap::ValType* intClassMap = allComplex.getIntIntClassMapMap()->allocate_element();
    c = intClassMap->allocate_element();
    c->setBool(true);
    (*intClassMap)[MAP_INT_KEY] = c;
    (*allComplex.getIntIntClassMapMap())[MAP_INT_KEY] = intClassMap;
    EXPECT_TRUE((*(*allComplex.getIntIntClassMapMap())[MAP_INT_KEY])[MAP_INT_KEY]->getBool() == true);

    allComplex.setStringIntClassMapMap(nullptr);
    intClassMap = allComplex.getStringIntClassMapMap()->allocate_element();
    c = intClassMap->allocate_element();
    c->setBool(true);
    (*intClassMap)[MAP_INT_KEY] = c;
    (*allComplex.getStringIntClassMapMap())[MAP_STRING_KEY] = intClassMap;
    EXPECT_TRUE((*(*allComplex.getStringIntClassMapMap())[MAP_STRING_KEY])[MAP_INT_KEY]->getBool() == true);
}


TEST(SetGetMapComplex, UnionAllocInPlace)
{
    EA::TDF::Test::AllMapsComplexUnionAllocInPlace allComplex;

    allComplex.setIntClass(nullptr);
    EA::TDF::Test::AllPrimitivesClass* c = allComplex.getIntClass()->allocate_element();
    c->setBool(true);
    (*allComplex.getIntClass())[MAP_INT_KEY] = c;
    EXPECT_TRUE((*allComplex.getIntClass())[MAP_INT_KEY]->getBool() == true);

    allComplex.setStringClass(nullptr);
    c = allComplex.getStringClass()->allocate_element();
    c->setBool(true);
    (*allComplex.getStringClass())[MAP_STRING_KEY] = c;
    EXPECT_TRUE((*allComplex.getStringClass())[MAP_STRING_KEY]->getBool() == true);

    allComplex.setIntUnion(nullptr);
    EA::TDF::Test::AllPrimitivesUnion* u = allComplex.getIntUnion()->allocate_element();
    u->setBool(true);
    (*allComplex.getIntUnion())[MAP_INT_KEY] = u;
    EXPECT_TRUE((*allComplex.getIntUnion())[MAP_INT_KEY]->getBool() == true);

    allComplex.setStringUnion(nullptr);
    u = allComplex.getStringUnion()->allocate_element();
    u->setBool(true);
    (*allComplex.getStringUnion())[MAP_STRING_KEY] = u;
    EXPECT_TRUE((*allComplex.getStringUnion())[MAP_STRING_KEY]->getBool() == true);

    allComplex.setIntBitField(nullptr);
    EA::TDF::Test::ABitField bits;
    bits.setBits(BITFIELD_VAL);
    (*allComplex.getIntBitField())[MAP_INT_KEY] = bits;
    EXPECT_TRUE((*allComplex.getIntBitField())[MAP_INT_KEY].getBits() == BITFIELD_VAL);

    allComplex.setStringBitField(nullptr);
    (*allComplex.getStringBitField())[MAP_STRING_KEY] = bits;
    EXPECT_TRUE((*allComplex.getStringBitField())[MAP_STRING_KEY].getBits() == BITFIELD_VAL);
    
    allComplex.setIntBlob(nullptr);
    EA::TDF::TdfBlob* blob = allComplex.getIntBlob()->allocate_element();
    blob->setData(BLOB_VAL, BLOB_VAL_SIZE);
    (*allComplex.getIntBlob())[MAP_INT_KEY] = blob;
    EXPECT_TRUE(memcmp((*allComplex.getIntBlob())[MAP_INT_KEY]->getData(), BLOB_VAL, BLOB_VAL_SIZE) == 0);

    allComplex.setStringBlob(nullptr);
    blob = allComplex.getStringBlob()->allocate_element();
    blob->setData(BLOB_VAL, BLOB_VAL_SIZE);
    (*allComplex.getStringBlob())[MAP_STRING_KEY] = blob;
    EXPECT_TRUE(memcmp((*allComplex.getStringBlob())[MAP_STRING_KEY]->getData(), BLOB_VAL, BLOB_VAL_SIZE) == 0);

    allComplex.setIntObjectType(nullptr);
    (*allComplex.getIntObjectType())[MAP_INT_KEY] = OBJECT_TYPE_VAL;
    EXPECT_TRUE((*allComplex.getIntObjectType())[MAP_INT_KEY] == OBJECT_TYPE_VAL);

    allComplex.setStringObjectType(nullptr);
    (*allComplex.getStringObjectType())[MAP_STRING_KEY] = OBJECT_TYPE_VAL;
    EXPECT_TRUE((*allComplex.getStringObjectType())[MAP_STRING_KEY] == OBJECT_TYPE_VAL);

    allComplex.setIntObjectId(nullptr);
    (*allComplex.getIntObjectId())[MAP_INT_KEY] = OBJECT_ID_VAL;
    EXPECT_TRUE((*allComplex.getIntObjectId())[MAP_INT_KEY] == OBJECT_ID_VAL);

    allComplex.setStringObjectId(nullptr);
    (*allComplex.getStringObjectId())[MAP_STRING_KEY] = OBJECT_ID_VAL;
    EXPECT_TRUE((*allComplex.getStringObjectId())[MAP_STRING_KEY] == OBJECT_ID_VAL);

    allComplex.setIntTimeValue(nullptr);
    (*allComplex.getIntTimeValue())[MAP_INT_KEY] = TIME_VAL;
    EXPECT_TRUE((*allComplex.getIntTimeValue())[MAP_INT_KEY] == TIME_VAL);

    allComplex.setStringTimeValue(nullptr);
    (*allComplex.getStringTimeValue())[MAP_STRING_KEY] = TIME_VAL;
    EXPECT_TRUE((*allComplex.getStringTimeValue())[MAP_STRING_KEY] == TIME_VAL);

    allComplex.setIntVariable(nullptr);
    EA::TDF::Test::AllPrimitivesClass allPrims;
    EA::TDF::VariableTdfBase* var = allComplex.getIntVariable()->allocate_element();
    var->set(allPrims);
    (*allComplex.getIntVariable())[MAP_INT_KEY] = var;
    EXPECT_TRUE((*allComplex.getIntVariable())[MAP_INT_KEY]->get() == &allPrims);

    allComplex.setStringVariable(nullptr);
    var = allComplex.getStringVariable()->allocate_element();
    var->set(allPrims);
    (*allComplex.getStringVariable())[MAP_STRING_KEY] = var;
    EXPECT_TRUE((*allComplex.getStringVariable())[MAP_STRING_KEY]->get() == &allPrims);


    allComplex.setIntBoolListMap(nullptr);
    EA::TDF::Test::IntBoolListMap::ValType* boolList = allComplex.getIntBoolListMap()->allocate_element();
    boolList->push_back(true);
    (*allComplex.getIntBoolListMap())[MAP_INT_KEY] = boolList;
    EXPECT_TRUE((*allComplex.getIntBoolListMap())[MAP_INT_KEY]->at(0) == true);

    allComplex.setStringBoolListMap(nullptr);
    boolList = allComplex.getStringBoolListMap()->allocate_element();
    boolList->push_back(true);
    (*allComplex.getStringBoolListMap())[MAP_STRING_KEY] = boolList;
    EXPECT_TRUE((*allComplex.getStringBoolListMap())[MAP_STRING_KEY]->at(0) == true);

    allComplex.setIntClassListMap(nullptr);
    EA::TDF::Test::IntClassListMap::ValType* classList = allComplex.getIntClassListMap()->allocate_element();
    classList->pull_back()->setBool(true);
    (*allComplex.getIntClassListMap())[MAP_INT_KEY] = classList;
    EXPECT_TRUE((*allComplex.getIntClassListMap())[MAP_INT_KEY]->at(0)->getBool() == true);

    allComplex.setStringClassListMap(nullptr);
    classList = allComplex.getStringClassListMap()->allocate_element();
    classList->pull_back()->setBool(true);
    (*allComplex.getStringClassListMap())[MAP_STRING_KEY] = classList;
    EXPECT_TRUE((*allComplex.getStringClassListMap())[MAP_STRING_KEY]->at(0)->getBool() == true);

    allComplex.setIntIntBoolMapMap(nullptr);
    EA::TDF::Test::IntIntBoolMapMap::ValType* intBoolMap = allComplex.getIntIntBoolMapMap()->allocate_element();
    (*intBoolMap)[MAP_INT_KEY] = true;
    (*allComplex.getIntIntBoolMapMap())[MAP_INT_KEY] = intBoolMap;
    EXPECT_TRUE((*(*allComplex.getIntIntBoolMapMap())[MAP_INT_KEY])[MAP_INT_KEY] == true);

    allComplex.setStringIntBoolMapMap(nullptr);
    intBoolMap = allComplex.getStringIntBoolMapMap()->allocate_element();
    (*intBoolMap)[MAP_INT_KEY] = true;
    (*allComplex.getStringIntBoolMapMap())[MAP_STRING_KEY] = intBoolMap;
    EXPECT_TRUE((*(*allComplex.getStringIntBoolMapMap())[MAP_STRING_KEY])[MAP_INT_KEY] == true);

    allComplex.setIntIntClassMapMap(nullptr);
    EA::TDF::Test::IntIntClassMapMap::ValType* intClassMap = allComplex.getIntIntClassMapMap()->allocate_element();
    c = intClassMap->allocate_element();
    c->setBool(true);
    (*intClassMap)[MAP_INT_KEY] = c;
    (*allComplex.getIntIntClassMapMap())[MAP_INT_KEY] = intClassMap;
    EXPECT_TRUE((*(*allComplex.getIntIntClassMapMap())[MAP_INT_KEY])[MAP_INT_KEY]->getBool() == true);

    allComplex.setStringIntClassMapMap(nullptr);
    intClassMap = allComplex.getStringIntClassMapMap()->allocate_element();
    c = intClassMap->allocate_element();
    c->setBool(true);
    (*intClassMap)[MAP_INT_KEY] = c;
    (*allComplex.getStringIntClassMapMap())[MAP_STRING_KEY] = intClassMap;
    EXPECT_TRUE((*(*allComplex.getStringIntClassMapMap())[MAP_STRING_KEY])[MAP_INT_KEY]->getBool() == true);
}
