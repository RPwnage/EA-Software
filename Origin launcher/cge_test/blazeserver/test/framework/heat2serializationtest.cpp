/*************************************************************************************************/
/*!
\file heat2serializationtest.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/heat2serializationtest.h"

#include "framework/blaze.h"
#include "framework/tdf/protobuftest_server.h"
#include "EATDF/tdffactory.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/protocol/shared/heat2encoder.h"

namespace Test
{
namespace heat2 
{
    // Known Results:
    // if encoder == true, then UnknownTdfIdInGeneric fails.
    // if encoder == true and decoder == false, then ComplexLists, ComplexMaps and UnknownTdfIdInGeneric all fail. 
    // if decoder == true, then all tests pass.
    bool gEncoderHeat1Compat = false;
    bool gDecoderHeat1Compat = false;

    void Heat2SerializationTest::SetUp()
    {
        EA::TDF::TdfFactory::fixupTypes();
    }

    void Heat2SerializationTest::TearDown()
    {

    }

    void fillTestData(Blaze::Protobuf::Test::AllPrimitivesClass & test)
    {
        test.setBool(true);
        test.setUInt8(40);
        test.setUInt16(41);
        test.setUInt32(42);
        test.setUInt64(43);
        test.setInt8(-40);
        test.setInt16(-41);
        test.setInt32(-42);
        test.setInt64(-43);
        test.setFloat(42.42f);
        test.setString("Hello World!");
        test.setOutOfClassEnum(Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_1);
        test.setInClassEnum(Blaze::Protobuf::Test::AllPrimitivesClass::IN_OF_CLASS_ENUM_VAL_1);
    }

    void fillTestData(Blaze::Protobuf::Test::AllComplexClass & test)
    {
        test.getClass().setUInt32(42);
        test.getUnion().setUInt8(42);
        test.getBitField().setBIT_3(true);
        test.getBitField().setBIT_1_2(2);
        uint8_t blob[5] = { '1', '2', '\0', '4', '5' };
        test.getBlob().setData(blob, 5);
        EA::TDF::ObjectType objType(42, 0);
        test.setObjectType(objType);
        EA::TDF::ObjectId objId(42, 0, 4242);
        test.setObjectId(objId);
        test.getTime().setSeconds(1000);
        static Blaze::Protobuf::Test::AllPrimitivesClass primClass;
        primClass.setUInt8(42);
        test.setVariable(primClass);
        test.getGeneric().get().set(42);
    }

    void fillTestData(Blaze::Protobuf::Test::AllListsPrimitivesClass & test)
    {
        const int kListSize = 5;
        for (int i = 0; i < kListSize; ++i)
        {
            test.getBool().push_back(true);

            test.getUInt8().push_back(40);
            test.getUInt16().push_back(41);
            test.getUInt32().push_back(42);
            test.getUInt64().push_back(43);

            test.getInt8().push_back(-40);
            test.getInt16().push_back(-41);
            test.getInt32().push_back(-42);
            test.getInt64().push_back(-43);

            test.getFloat().push_back(42.42f);

            test.getString().push_back("Hello World!");

            test.getOutOfClassEnum().push_back(Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_2);
            test.getInClassEnum().push_back(Blaze::Protobuf::Test::AllListsPrimitivesClass::IN_OF_CLASS_ENUM_VAL_1);
        }

        test.getBool()[2] = false;
    }

    void fillTestData(Blaze::Protobuf::Test::AllListsComplexClass & test)
    {
        static Blaze::Protobuf::Test::AllPrimitivesClass primClass;
        static Blaze::Protobuf::Test::AllPrimitivesClass primClass2;
        fillTestData(primClass);

        const int kListSize = 5;
        for (int i = 0; i < kListSize; ++i)
        {
            uint8_t blob[5] = { '1', '2', '\0', '4', '5' };
            test.getBlob().pull_back()->setData(blob, 5);

            primClass.copyInto(*test.getClass().pull_back());

            test.getUnion().pull_back()->setString("Hello World!");

            primClass2.setInt16(42);
            test.getUnionComplex().pull_back()->setClass(&primClass2);

            Blaze::Protobuf::Test::ABitField& bitfield = test.getBitField().push_back();
            bitfield.setBIT_3(true);
            bitfield.setBIT_1_2(2);

            EA::TDF::ObjectType& objType = test.getObjectType().push_back();
            objType.component = 42;
            objType.type = 0;

            EA::TDF::ObjectId& objId = test.getObjectId().push_back();
            objId.type.component = 42;
            objId.type.type = 0;
            objId.id = 42;

            EA::TDF::TimeValue& timeValue = test.getTime().push_back();
            timeValue.setMicroSeconds(42);

            test.getVariable().pull_back()->set(primClass);

            test.getGeneric().pull_back()->set(primClass);

            // list of list of bool
            {
                auto boolList = test.getListBool().pull_back();
                boolList->push_back(true); boolList->push_back(false); boolList->push_back(true);
            }

            // list of list of int
            {
                auto intList = test.getListInt().pull_back();
                intList->push_back(42); intList->push_back(0); intList->push_back(42);
            }

            // list of list of short int
            {
                auto shortIntList = test.getListShortInt().pull_back();
                shortIntList->push_back(42); shortIntList->push_back(0); shortIntList->push_back(42);
            }

            {
                auto enumList = test.getListEnum().pull_back();
                enumList->push_back(Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_1); enumList->push_back(Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_0); enumList->push_back(Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_2);
            }

            // list of list of string
            {
                auto stringList = test.getListString().pull_back();
                stringList->push_back("Hello World!"); stringList->push_back("Hello World!"); stringList->push_back("Hello World!");
            }

            // list of list of class/message
            {
                auto classList = test.getListClass().pull_back();
                classList->pull_back()->setFloat(42.42f);
            }

            // list of a map<int, int>
            {
                auto mapIntInt = test.getListMapIntInt().pull_back();
                (*mapIntInt)[42] = 42;
                (*mapIntInt)[0] = 0;
            }

            // list of a map<int16_t, int8_t>
            {
                auto mapShortIntShortShortInt = test.getListMapShortIntShortShortInt().pull_back();
                (*mapShortIntShortShortInt)[42] = 42;
                (*mapShortIntShortShortInt)[0] = 0;
            }

            // list of a map<int, class>
            {
                auto mapIntClass = test.getListMapIntClass().pull_back();

                auto primMapClass = mapIntClass->allocate_element();
                primMapClass->setOutOfClassEnum(Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_3);

                (*mapIntClass)[42] = primMapClass;
                (*mapIntClass)[0] = primMapClass;
            }
        }

    }

    void fillTestData(Blaze::Protobuf::Test::AllMapsPrimitivesClass & test)
    {
        // uint32_t key
        test.getIntBool()[42] = true;
        test.getIntBool()[43] = false;
        test.getIntBool()[0] = false;

        test.getIntUInt8()[42] = 42;
        test.getIntUInt8()[0] = 0;

        test.getIntUInt64()[42] = 424242;

        test.getIntInt8()[42] = -42;
        test.getIntInt8()[0] = -42;

        test.getIntFloat()[42] = 42.42f;

        test.getIntString()[42] = "Hello World!";
        test.getIntString()[0] = "Hello World!";
        test.getIntString()[43] = "";

        // string key
        test.getStringBool()["true"] = true;
        test.getStringBool()["false"] = false;

        test.getStringUInt8()["42"] = 42;
        test.getStringUInt8()["0"] = 0;

        test.getStringUInt64()["42"] = 424242;

        test.getStringInt8()["42"] = -42;
        test.getStringInt8()["0"] = -42;

        test.getStringFloat()["42"] = 42.42f;

        test.getStringString()["42"] = "Hello World!";
        test.getStringString()["0"] = "Hello World!";
        test.getStringString()["43"] = "";

        test.getStringOutOfClassEnum()["42"] = Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_1;
        test.getStringOutOfClassEnum()["0"] = Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_0;

        test.getStringInClassEnum()["42"] = Blaze::Protobuf::Test::AllMapsPrimitivesClass::IN_OF_CLASS_ENUM_VAL_3;
        test.getStringInClassEnum()["0"] = Blaze::Protobuf::Test::AllMapsPrimitivesClass::IN_OF_CLASS_ENUM_VAL_0;


        // enum key
        test.getEnumBool()[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_1] = true;
        test.getEnumBool()[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_2] = false;
        test.getEnumBool()[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_0] = false;

        test.getEnumUInt8()[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_1] = 42;
        test.getEnumUInt8()[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_0] = 0;

        test.getEnumUInt64()[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_1] = 424242;

        test.getEnumInt8()[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_1] = -42;
        test.getEnumInt8()[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_0] = -42;

        test.getEnumFloat()[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_1] = 42.42f;

        test.getEnumString()[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_1] = "Hello World!";
        test.getEnumString()[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_0] = "Hello World!";
        test.getEnumString()[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_2] = "";

    }

    void fillTestData(Blaze::Protobuf::Test::AllMapsComplexClass & test)
    {
        // Class as value
        static Blaze::Protobuf::Test::AllPrimitivesClass primClass;
        fillTestData(primClass);

#define ALLOC_MAP(map, key, value) map[key] = map.allocate_element();  value.copyInto(*map[key]);

        ALLOC_MAP(test.getIntClass(), 42, primClass);
        ALLOC_MAP(test.getStringClass(), "42", primClass);
        ALLOC_MAP(test.getEnumClass(), Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_0, primClass);

        // union as value
        static Blaze::Protobuf::Test::AllPrimitivesUnion unionClass;
        unionClass.setString("Hello World!");

        ALLOC_MAP(test.getIntUnion(), 42, unionClass);
        ALLOC_MAP(test.getStringUnion(), "42", unionClass);
        ALLOC_MAP(test.getEnumUnion(), Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_1, unionClass);

        // bitfied as value
        Blaze::Protobuf::Test::ABitField bitfield;
        bitfield.setBIT_3(true);
        bitfield.setBIT_1_2(2);

        test.getIntBitField()[42] = bitfield;
        test.getStringBitField()["42"] = bitfield;
        test.getEnumBitField()[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_2] = bitfield;

        // blob as value
        uint8_t blob[5] = { '1', '2', '\0', '4', '5' };
        static EA::TDF::TdfBlob blobClass;
        blobClass.setData(blob, 5);

        ALLOC_MAP(test.getIntBlob(), 42, blobClass);
        ALLOC_MAP(test.getStringBlob(), "42", blobClass);
        ALLOC_MAP(test.getEnumBlob(), Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_2, blobClass);

        // ObjectType as value
        EA::TDF::ObjectType objType(42, 0);
        test.getStringObjectType()["42"] = objType;

        // ObjectId as value 
        EA::TDF::ObjectId objId(42, 0, 42);
        test.getStringObjectId()["42"] = objId;

        // TimeValue as value
        EA::TDF::TimeValue timeValue(42);
        test.getIntTimeValue()[42] = timeValue;
        test.getStringTimeValue()["42"] = timeValue;
        test.getEnumTimeValue()[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_3] = timeValue;

        //variable tdf as value
        EA::TDF::VariableTdfBase variableTdf;
        variableTdf.set(primClass);

        ALLOC_MAP(test.getIntVariable(), 42, variableTdf);
        ALLOC_MAP(test.getStringVariable(), "42", variableTdf);
        ALLOC_MAP(test.getEnumVariable(), Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_0, variableTdf);

        // generic tdf as value
        EA::TDF::GenericTdfType genericTdf;
        genericTdf.set(42.42f);

        ALLOC_MAP(test.getIntGeneric(), 42, genericTdf);
        ALLOC_MAP(test.getStringGeneric(), "42", genericTdf);
        ALLOC_MAP(test.getEnumGeneric(), Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_0, genericTdf);

        // map of a <int, list<int> >
        {
            auto listOfInt = test.getMapIntListInt().allocate_element();
            listOfInt->push_back(42);
            listOfInt->push_back(4242);

            auto listOfInt2 = test.getMapIntListInt().allocate_element();
            listOfInt2->push_back(421);
            listOfInt2->push_back(42421);

            test.getMapIntListInt()[42] = listOfInt;
            test.getMapIntListInt()[0] = listOfInt2;
        }

        // map of a <int, list<enum> >
        {
            auto listOfEnum = test.getMapIntListEnum().allocate_element();
            listOfEnum->push_back(Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_1);
            listOfEnum->push_back(Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_0);

            auto listOfEnum2 = test.getMapIntListEnum().allocate_element();
            listOfEnum2->push_back(Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_2);
            listOfEnum2->push_back(Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_3);

            test.getMapIntListEnum()[42] = listOfEnum;
            test.getMapIntListEnum()[0] = listOfEnum2;
        }

        // map of a <int, list<string> >
        {
            auto listOfString = test.getMapIntListString().allocate_element();
            listOfString->push_back("Hello World!");
            listOfString->push_back("Hello World!2");

            auto listOfString2 = test.getMapIntListString().allocate_element();
            listOfString2->push_back("Hello World!3");
            listOfString2->push_back("Hello World!4");

            test.getMapIntListString()[42] = listOfString;
            test.getMapIntListString()[0] = listOfString2;
        }

        // map of a <int, list<class> >
        {
            auto listOfClass = test.getMapIntListClass().allocate_element();
            listOfClass->pull_back()->setInt32(42);
            listOfClass->pull_back()->setUInt16(42);

            auto listOfClass2 = test.getMapIntListClass().allocate_element();
            listOfClass2->pull_back()->setInt32(422);
            listOfClass2->pull_back()->setUInt16(422);

            test.getMapIntListClass()[42] = listOfClass;
            test.getMapIntListClass()[0] = listOfClass2;
        }

        // map of a <int, map<int, int> >
        {
            auto mapOfIntInt = test.getMapIntMapIntInt().allocate_element();
            (*mapOfIntInt)[42] = 42;
            (*mapOfIntInt)[0] = 42;

            auto mapOfIntInt2 = test.getMapIntMapIntInt().allocate_element();
            (*mapOfIntInt2)[42] = 422;
            (*mapOfIntInt2)[0] = 422;

            test.getMapIntMapIntInt()[42] = mapOfIntInt;
            test.getMapIntMapIntInt()[0] = mapOfIntInt2;
        }

        // map of a <int, map<string, string> >
        {
            auto mapOfStringString = test.getMapIntMapStringString().allocate_element();
            (*mapOfStringString)["42"] = "42";
            (*mapOfStringString)["0"] = "42";

            auto mapOfStringString2 = test.getMapIntMapStringString().allocate_element();
            (*mapOfStringString2)["422"] = "422";
            (*mapOfStringString2)["02"] = "422";

            test.getMapIntMapStringString()[42] = mapOfStringString;
            test.getMapIntMapStringString()[0] = mapOfStringString2;
        }

        // map of a <int, map<string, class> >
        {
            auto mapOfStringClass = test.getMapIntMapStringClass().allocate_element();
            ALLOC_MAP((*mapOfStringClass), "42", primClass);
            ALLOC_MAP((*mapOfStringClass), "0", primClass);

            auto mapOfStringClass2 = test.getMapIntMapStringClass().allocate_element();
            ALLOC_MAP((*mapOfStringClass2), "422", primClass);
            ALLOC_MAP((*mapOfStringClass2), "02", primClass);

            test.getMapIntMapStringClass()[42] = mapOfStringClass;
            test.getMapIntMapStringClass()[0] = mapOfStringClass2;
        }

        // map of a <int, map<enum, string> >
        {
            auto mapOfEnumString = test.getMapIntMapEnumString().allocate_element();
            (*mapOfEnumString)[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_1] = "42";
            (*mapOfEnumString)[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_2] = "0";

            auto mapOfEnumString2 = test.getMapIntMapEnumString().allocate_element();
            (*mapOfEnumString2)[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_2] = "422";
            (*mapOfEnumString2)[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_3] = "20";

            test.getMapIntMapEnumString()[42] = mapOfEnumString;
            test.getMapIntMapEnumString()[0] = mapOfEnumString2;
        }

#undef ALLOC_MAP
    }

    void fillTestData(Blaze::Protobuf::Test::OuterClass & test)
    {
        test.setUInt8(42);
        test.getInner().setUInt8(42);
        test.getInner().getInnerInnerClass().setUInt8(42);
    }

    void fillTestData(Blaze::Protobuf::Test::AllPrimitivesClassExtra & test)
    {
        test.setBool(true);
        test.setUInt8(40);
        test.setUInt16(41);
        test.setUInt32(42);
        test.setUInt64(43);
        test.setInt8(-40);
        test.setInt16(-41);
        test.setInt32(-42);
        test.setInt64(-43);
        test.setFloat(42.42f);
        test.setString("Hello World!");
        test.setOutOfClassEnum(Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_1);
        test.setBool_B(true);
        test.setUInt8_B(10);
        test.setUInt16_B(141);
        test.setUInt32_B(142);
        test.setUInt64_B(143);
        test.setInt8_B(-10);
        test.setInt16_B(-141);
        test.setInt32_B(-142);
        test.setInt64_B(-143);
        test.setFloat_B(142.42f);
        test.setString_B("1 Hello World!");
        test.setOutOfClassEnum_B(Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_2);
    }

    void fillTestData(Blaze::Protobuf::Test::AllComplexClassExtra & test)
    {
        test.getClass().setUInt32(42);
        test.getUnion().setUInt8(42);
        test.getBitField().setBIT_3(true);
        test.getBitField().setBIT_1_2(2);
        uint8_t blob[5] = { '1', '2', '\0', '4', '5' };
        test.getBlob().setData(blob, 5);
        EA::TDF::ObjectType objType(42, 0);
        test.setObjectType(objType);
        EA::TDF::ObjectId objId(42, 0, 4242);
        test.setObjectId(objId);
        test.getTime().setSeconds(1000);
        static Blaze::Protobuf::Test::AllPrimitivesClass primClass;
        primClass.setUInt8(42);
        test.setVariable(primClass);
        test.getGeneric().get().set(42);

        test.getClass_B().setUInt32(142);
        test.getUnion_B().setUInt8(142);
        test.getBitField_B().setBIT_3(true);
        test.getBitField_B().setBIT_1_2(1);
        uint8_t blob_B[5] = { '1', '3', '\0', '4', '7' };
        test.getBlob_B().setData(blob_B, 5);
        EA::TDF::ObjectType objType_B(12, 0);
        test.setObjectType_B(objType_B);
        EA::TDF::ObjectId objId_B(12, 0, 1212);
        test.setObjectId_B(objId_B);
        test.getTime_B().setSeconds(1320);
        static Blaze::Protobuf::Test::AllPrimitivesClass primClass_B;
        primClass_B.setUInt8(12);
        test.setVariable_B(primClass_B);
        test.getGeneric_B().get().set(12);
    }

    TEST_F(Heat2SerializationTest, BasicPrimitives)
    {
        Blaze::RawBuffer buffer(1024*16);
        Blaze::Heat2Encoder encoder;  encoder.setHeat1BackCompat(gEncoderHeat1Compat);
        Blaze::Heat2Decoder decoder;  decoder.setHeat1BackCompat(gDecoderHeat1Compat);
        Blaze::Protobuf::Test::AllPrimitivesClass test;

        fillTestData(test);

        EXPECT_TRUE(encoder.encode(buffer, test));

        Blaze::Protobuf::Test::AllPrimitivesClass test2;
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.equalsValue(test2));
    }

    TEST_F(Heat2SerializationTest, SettingNonZeroDefaultValuesToZero)
    {
        Blaze::RawBuffer buffer(1024*16);
        Blaze::Heat2Encoder encoder;  encoder.setHeat1BackCompat(gEncoderHeat1Compat);
        Blaze::Heat2Decoder decoder;  decoder.setHeat1BackCompat(gDecoderHeat1Compat);
        Blaze::Protobuf::Test::AllPrimitivesClass test;

        test.setBool(false);
        test.setInt32(0);
        test.setFloat(0.0f);
        test.setString("");

        EXPECT_TRUE(encoder.encode(buffer, test));

        Blaze::Protobuf::Test::AllPrimitivesClass test2;
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.equalsValue(test2));
    }

    TEST_F(Heat2SerializationTest, PrimitiveUnion)
    {
        Blaze::RawBuffer buffer(1024*16);
        Blaze::Heat2Encoder encoder;  encoder.setHeat1BackCompat(gEncoderHeat1Compat);
        Blaze::Heat2Decoder decoder;  decoder.setHeat1BackCompat(gDecoderHeat1Compat);
        Blaze::Protobuf::Test::AllPrimitivesUnion test2;

        // int64
        Blaze::Protobuf::Test::AllPrimitivesUnion test;
        test.setInt64(424242);

        EXPECT_TRUE(encoder.encode(buffer, test));
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.equalsValue(test2));

        // string 
        test.setString("Hello World!");

        EXPECT_TRUE(encoder.encode(buffer, test));
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);
        
        EXPECT_TRUE(test.equalsValue(test2));

        // enum
        test.setOutOfClassEnum(Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_2);

        EXPECT_TRUE(encoder.encode(buffer, test));
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.equalsValue(test2));

        // empty union
        test.switchActiveMember(Blaze::Protobuf::Test::AllPrimitivesUnion::MEMBER_UNSET);

        EXPECT_TRUE(encoder.encode(buffer, test));
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.equalsValue(test2));

    }

    TEST_F(Heat2SerializationTest, ComplexUnion)
    {
        Blaze::RawBuffer buffer(1024*16);
        Blaze::Heat2Encoder encoder;  encoder.setHeat1BackCompat(gEncoderHeat1Compat);
        Blaze::Heat2Decoder decoder;  decoder.setHeat1BackCompat(gDecoderHeat1Compat);
        Blaze::Protobuf::Test::AllComplexUnion test2;

        // message member
        Blaze::Protobuf::Test::AllComplexUnion test;
        test.switchActiveMember(Blaze::Protobuf::Test::AllComplexUnion::MEMBER_CLASS);
        // set int32 on the message
        test.getClass()->setInt32(4242);

        EXPECT_TRUE(encoder.encode(buffer, test));
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.equalsValue(test2));


        // set string on the message
        test.getClass()->setString("Hello World!");

        EXPECT_TRUE(encoder.encode(buffer, test));
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.equalsValue(test2));


        // blob member
        uint8_t blob[5] = { '1', '2', '\0', '4', '5' };
        test.switchActiveMember(Blaze::Protobuf::Test::AllComplexUnion::MEMBER_BLOB);
        test.getBlob()->setData(blob, 5);

        EXPECT_TRUE(encoder.encode(buffer, test));
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.equalsValue(test2));


        // ObjectType member
        EA::TDF::ObjectType objType(42, 0);
        test.setObjectType(objType);

        EXPECT_TRUE(encoder.encode(buffer, test));
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.equalsValue(test2));


        // ObjectId member
        EA::TDF::ObjectId objId(42, 0, 42);
        test.setObjectId(objId);

        EXPECT_TRUE(encoder.encode(buffer, test));
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.equalsValue(test2));


        // TimeValue member
        EA::TDF::TimeValue timeValue(42);
        test.setTime(timeValue);

        EXPECT_TRUE(encoder.encode(buffer, test));
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.equalsValue(test2));


        // variable member
        Blaze::Protobuf::Test::AllPrimitivesClass primClass;
        primClass.setUInt8(42);

        test.switchActiveMember(Blaze::Protobuf::Test::AllComplexUnion::MEMBER_VARIABLE);
        test.getVariable()->set(primClass);

        EXPECT_TRUE(encoder.encode(buffer, test));
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.equalsValue(test2));


        // generic member
        test.switchActiveMember(Blaze::Protobuf::Test::AllComplexUnion::MEMBER_GENERIC);
        test.getGeneric()->get().set("Hello World!");

        EXPECT_TRUE(encoder.encode(buffer, test));
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.equalsValue(test2));
    }

    TEST_F(Heat2SerializationTest, Blob)
    {
        Blaze::RawBuffer buffer(1024*16);
        Blaze::Heat2Encoder encoder;  encoder.setHeat1BackCompat(gEncoderHeat1Compat);
        Blaze::Heat2Decoder decoder;  decoder.setHeat1BackCompat(gDecoderHeat1Compat);
        Blaze::Protobuf::Test::AllComplexClass test;

        uint8_t blob[5] = { '1', '2', '\0', '4', '5' };
        test.getBlob().setData(blob, 5);

        EXPECT_TRUE(encoder.encode(buffer, test));

        Blaze::Protobuf::Test::AllComplexClass test2;
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.equalsValue(test2));
    }

    TEST_F(Heat2SerializationTest, ComplexClass)
    {
        Blaze::RawBuffer buffer(1024*16);
        Blaze::Heat2Encoder encoder;  encoder.setHeat1BackCompat(gEncoderHeat1Compat);
        Blaze::Heat2Decoder decoder;  decoder.setHeat1BackCompat(gDecoderHeat1Compat);
        Blaze::Protobuf::Test::AllComplexClass test;

        fillTestData(test);
        EXPECT_TRUE(encoder.encode(buffer, test));

        Blaze::Protobuf::Test::AllComplexClass test2;
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.equalsValue(test2));
    }

    TEST_F(Heat2SerializationTest, Generic)
    {
        Blaze::RawBuffer buffer(1024*16);
        Blaze::Heat2Encoder encoder;  encoder.setHeat1BackCompat(gEncoderHeat1Compat);
        Blaze::Heat2Decoder decoder;  decoder.setHeat1BackCompat(gDecoderHeat1Compat);
        Blaze::Protobuf::Test::AllComplexClass test;

        // bool
        {
            test.getGeneric().get().set(true);

            EXPECT_TRUE(encoder.encode(buffer, test));

            Blaze::Protobuf::Test::AllComplexClass test2;
            EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // unsigned
        {
            test.getGeneric().get().set(4242u);

            EXPECT_TRUE(encoder.encode(buffer, test));

            Blaze::Protobuf::Test::AllComplexClass test2;
            EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // int32
        {
            test.getGeneric().get().set(-42);

            EXPECT_TRUE(encoder.encode(buffer, test));

            Blaze::Protobuf::Test::AllComplexClass test2;
            EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // float
        {
            test.getGeneric().get().set(42.42f);

            EXPECT_TRUE(encoder.encode(buffer, test));

            Blaze::Protobuf::Test::AllComplexClass test2;
            EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

            EXPECT_TRUE(test.equalsValue(test2));
        }


        // int64
        {
            test.getGeneric().get().set((int64_t)(-42));

            EXPECT_TRUE(encoder.encode(buffer, test));

            Blaze::Protobuf::Test::AllComplexClass test2;
            EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // uint64
        {
            test.getGeneric().get().set((uint64_t)(42));

            EXPECT_TRUE(encoder.encode(buffer, test));

            Blaze::Protobuf::Test::AllComplexClass test2;
            EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // string
        {
            test.getGeneric().get().set("Hello World!");

            EXPECT_TRUE(encoder.encode(buffer, test));

            Blaze::Protobuf::Test::AllComplexClass test2;
            EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // blob
        {
            uint8_t blob[5] = { '1', '2', '\0', '4', '5' };
            EA::TDF::TdfBlob tdfBlob;
            tdfBlob.setData(blob, 5);

            test.getGeneric().get().set(tdfBlob);

            EXPECT_TRUE(encoder.encode(buffer, test));

            Blaze::Protobuf::Test::AllComplexClass test2;
            EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // ObjectType
        {
            EA::TDF::ObjectType objType(42, 0);

            test.getGeneric().get().set(objType);

            EXPECT_TRUE(encoder.encode(buffer, test));

            Blaze::Protobuf::Test::AllComplexClass test2;
            EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // ObjectId
        {
            EA::TDF::ObjectId objId(42, 0, 4242);

            test.getGeneric().get().set(objId);

            EXPECT_TRUE(encoder.encode(buffer, test));

            Blaze::Protobuf::Test::AllComplexClass test2;
            EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // TimeValue
        {
            EA::TDF::TimeValue timeValue(42);

            test.getGeneric().get().set(timeValue);

            EXPECT_TRUE(encoder.encode(buffer, test));

            Blaze::Protobuf::Test::AllComplexClass test2;
            EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // bitfield
        {
            Blaze::Protobuf::Test::ABitField bitfield;
            bitfield.setBIT_3(true);
            bitfield.setBIT_1_2(2);

            test.getGeneric().get().set(bitfield);

            EXPECT_TRUE(encoder.encode(buffer, test));

            Blaze::Protobuf::Test::AllComplexClass test2;
            EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // Union
        {
            Blaze::Protobuf::Test::AllPrimitivesUnion primsUnion;
            primsUnion.setInt64(424242);

            test.getGeneric().get().set(primsUnion);

            EXPECT_TRUE(encoder.encode(buffer, test));

            Blaze::Protobuf::Test::AllComplexClass test2;
            EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // Message
        {
            Blaze::Protobuf::Test::AllPrimitivesClass primsClass;
            primsClass.setInt64(424242);

            test.getGeneric().get().set(primsClass);

            EXPECT_TRUE(encoder.encode(buffer, test));

            Blaze::Protobuf::Test::AllComplexClass test2;
            EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // Lists
        {
            //float list
            {
                EA::TDF::TdfPrimitiveVector<float> floatList;
                floatList.push_back(42.f);
                floatList.push_back(42.42f);

                test.getGeneric().get().set(floatList);

                EXPECT_TRUE(encoder.encode(buffer, test));

                Blaze::Protobuf::Test::AllComplexClass test2;
                EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

                EXPECT_TRUE(test.equalsValue(test2));
            }

            //string list
            {
                EA::TDF::TdfPrimitiveVector<EA::TDF::TdfString> stringList;
                stringList.push_back("Hello World!");
                stringList.push_back("42");

                test.getGeneric().get().set(stringList);

                EXPECT_TRUE(encoder.encode(buffer, test));

                Blaze::Protobuf::Test::AllComplexClass test2;
                EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

                EXPECT_TRUE(test.equalsValue(test2));
            }

            // ObjectType list
            {
                EA::TDF::TdfPrimitiveVector<EA::TDF::ObjectType> objectTypeList;
                objectTypeList.push_back(EA::TDF::ObjectType(42, 0));
                objectTypeList.push_back(EA::TDF::ObjectType(42, 0));

                test.getGeneric().get().set(objectTypeList);

                EXPECT_TRUE(encoder.encode(buffer, test));

                Blaze::Protobuf::Test::AllComplexClass test2;
                EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

                EXPECT_TRUE(test.equalsValue(test2));
            }

            // ObjectId list
            {
                EA::TDF::TdfPrimitiveVector<EA::TDF::ObjectId> objectIdList;
                objectIdList.push_back(EA::TDF::ObjectId(42, 0, 42));
                objectIdList.push_back(EA::TDF::ObjectId(42, 0, 42));

                test.getGeneric().get().set(objectIdList);

                EXPECT_TRUE(encoder.encode(buffer, test));

                Blaze::Protobuf::Test::AllComplexClass test2;
                EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

                EXPECT_TRUE(test.equalsValue(test2));
            }

            // TimeValue list
            {
                EA::TDF::TdfPrimitiveVector<EA::TDF::TimeValue> timeValueList;
                timeValueList.push_back(EA::TDF::TimeValue(42));
                timeValueList.push_back(EA::TDF::TimeValue(4242));

                test.getGeneric().get().set(timeValueList);

                EXPECT_TRUE(encoder.encode(buffer, test));

                Blaze::Protobuf::Test::AllComplexClass test2;
                EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

                EXPECT_TRUE(test.equalsValue(test2));
            }
        }
    }

    TEST_F(Heat2SerializationTest, PrimitiveLists)
    {
        Blaze::RawBuffer buffer(1024*16);
        Blaze::Heat2Encoder encoder;  encoder.setHeat1BackCompat(gEncoderHeat1Compat);
        Blaze::Heat2Decoder decoder;  decoder.setHeat1BackCompat(gDecoderHeat1Compat);
        Blaze::Protobuf::Test::AllListsPrimitivesClass test;

        fillTestData(test);
        EXPECT_TRUE(encoder.encode(buffer, test));

        Blaze::Protobuf::Test::AllListsPrimitivesClass test2;
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.equalsValue(test2));
    }

    TEST_F(Heat2SerializationTest, ComplexLists)
    {
        Blaze::RawBuffer buffer(1024*16);
        Blaze::Heat2Encoder encoder;  encoder.setHeat1BackCompat(gEncoderHeat1Compat);
        Blaze::Heat2Decoder decoder;  decoder.setHeat1BackCompat(gDecoderHeat1Compat);
        Blaze::Protobuf::Test::AllListsComplexClass test;

        fillTestData(test);
        EXPECT_TRUE(encoder.encode(buffer, test));

        Blaze::Protobuf::Test::AllListsComplexClass test2;
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.equalsValue(test2));
    }

    TEST_F(Heat2SerializationTest, PrimitiveMaps)
    {
        Blaze::RawBuffer buffer(1024*16);
        Blaze::Heat2Encoder encoder;  encoder.setHeat1BackCompat(gEncoderHeat1Compat);
        Blaze::Heat2Decoder decoder;  decoder.setHeat1BackCompat(gDecoderHeat1Compat);
        Blaze::Protobuf::Test::AllMapsPrimitivesClass test;

        fillTestData(test);
        EXPECT_TRUE(encoder.encode(buffer, test));

        Blaze::Protobuf::Test::AllMapsPrimitivesClass test2;
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.equalsValue(test2));
    }

    TEST_F(Heat2SerializationTest, ComplexMaps)
    {
        Blaze::RawBuffer buffer(1024*16);
        Blaze::Heat2Encoder encoder;  encoder.setHeat1BackCompat(gEncoderHeat1Compat);
        Blaze::Heat2Decoder decoder;  decoder.setHeat1BackCompat(gDecoderHeat1Compat);
        Blaze::Protobuf::Test::AllMapsComplexClass test;

        fillTestData(test);
        EXPECT_TRUE(encoder.encode(buffer, test));

        Blaze::Protobuf::Test::AllMapsComplexClass test2;
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.equalsValue(test2));
    }

    TEST_F(Heat2SerializationTest, NestedClass)
    {
        Blaze::RawBuffer buffer(1024*16);
        Blaze::Heat2Encoder encoder;  encoder.setHeat1BackCompat(gEncoderHeat1Compat);
        Blaze::Heat2Decoder decoder;  decoder.setHeat1BackCompat(gDecoderHeat1Compat);
        Blaze::Protobuf::Test::OuterClass test;

        fillTestData(test);
        EXPECT_TRUE(encoder.encode(buffer, test));

        Blaze::Protobuf::Test::OuterClass test2;
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.equalsValue(test2));
    }


    TEST_F(Heat2SerializationTest, MissingPrimitives)
    {
        Blaze::RawBuffer buffer(1024 * 16);
        Blaze::Heat2Encoder encoder;  encoder.setHeat1BackCompat(gEncoderHeat1Compat);
        Blaze::Heat2Decoder decoder;  decoder.setHeat1BackCompat(gDecoderHeat1Compat);
        Blaze::Protobuf::Test::AllPrimitivesClass test;

        fillTestData(test);
        EXPECT_TRUE(encoder.encode(buffer, test));

        // Decode into a class with extra (unset) values:
        Blaze::Protobuf::Test::AllPrimitivesClassExtra test2;
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_EQ(test.getBool(), test2.getBool());
        EXPECT_EQ(test.getUInt8(), test2.getUInt8());
        EXPECT_EQ(test.getUInt16(), test2.getUInt16());
        EXPECT_EQ(test.getUInt32(), test2.getUInt32());
        EXPECT_EQ(test.getUInt64(), test2.getUInt64());
        EXPECT_EQ(test.getInt8(), test2.getInt8());
        EXPECT_EQ(test.getInt16(), test2.getInt16());
        EXPECT_EQ(test.getInt32(), test2.getInt32());
        EXPECT_EQ(test.getInt64(), test2.getInt64());
        EXPECT_EQ(test.getFloat(), test2.getFloat());
        EXPECT_EQ(test.getOutOfClassEnum(), test2.getOutOfClassEnum());
    }

    TEST_F(Heat2SerializationTest, ExtraPrimitives)
    {
        Blaze::RawBuffer buffer(1024 * 16);
        Blaze::Heat2Encoder encoder;  encoder.setHeat1BackCompat(gEncoderHeat1Compat);
        Blaze::Heat2Decoder decoder;  decoder.setHeat1BackCompat(gDecoderHeat1Compat);
        Blaze::Protobuf::Test::AllPrimitivesClassExtra test;

        fillTestData(test);
        EXPECT_TRUE(encoder.encode(buffer, test));

        // Decode into a class missing some values:
        Blaze::Protobuf::Test::AllPrimitivesClass test2;
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_EQ(test.getBool(), test2.getBool());
        EXPECT_EQ(test.getUInt8(), test2.getUInt8());
        EXPECT_EQ(test.getUInt16(), test2.getUInt16());
        EXPECT_EQ(test.getUInt32(), test2.getUInt32());
        EXPECT_EQ(test.getUInt64(), test2.getUInt64());
        EXPECT_EQ(test.getInt8(), test2.getInt8());
        EXPECT_EQ(test.getInt16(), test2.getInt16());
        EXPECT_EQ(test.getInt32(), test2.getInt32());
        EXPECT_EQ(test.getInt64(), test2.getInt64());
        EXPECT_EQ(test.getFloat(), test2.getFloat());
        EXPECT_EQ(test.getOutOfClassEnum(), test2.getOutOfClassEnum());
    }

    TEST_F(Heat2SerializationTest, TypeChangePrimitives)
    {
        Blaze::RawBuffer buffer(1024 * 16);
        Blaze::Heat2Encoder encoder;  encoder.setHeat1BackCompat(gEncoderHeat1Compat);
        Blaze::Heat2Decoder decoder;  decoder.setHeat1BackCompat(gDecoderHeat1Compat);
        Blaze::Protobuf::Test::AllPrimitivesClassExtra test;

        fillTestData(test);
        EXPECT_TRUE(encoder.encode(buffer, test));

        // Decode into a class missing some values:
        Blaze::Protobuf::Test::AllPrimitivesClassExtraTypeChange test2;
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_EQ(test.getBool(), test2.getBool());
        EXPECT_EQ(test.getUInt8(), test2.getUInt8());
        EXPECT_EQ(test.getUInt16(), test2.getUInt16());
        EXPECT_EQ(test.getUInt32(), test2.getUInt32());
        EXPECT_EQ(test.getUInt64(), test2.getUInt64());
        EXPECT_EQ(test.getInt8(), test2.getInt8());
        EXPECT_EQ(test.getInt16(), test2.getInt16());
        EXPECT_EQ(test.getInt32(), test2.getInt32());
        EXPECT_EQ(test.getInt64(), test2.getInt64());
        EXPECT_EQ(test.getFloat(), test2.getFloat());
        EXPECT_EQ(test.getOutOfClassEnum(), test2.getOutOfClassEnum());
    }

    TEST_F(Heat2SerializationTest, MissingComplex)
    {
        Blaze::RawBuffer buffer(1024 * 16);
        Blaze::Heat2Encoder encoder;  encoder.setHeat1BackCompat(gEncoderHeat1Compat);
        Blaze::Heat2Decoder decoder;  decoder.setHeat1BackCompat(gDecoderHeat1Compat);
        Blaze::Protobuf::Test::AllComplexClass test;

        fillTestData(test);
        EXPECT_TRUE(encoder.encode(buffer, test));

        Blaze::Protobuf::Test::AllComplexClassExtra test2;
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.getClass().equalsValue(test2.getClass()));
        EXPECT_TRUE(test.getUnion().equalsValue(test2.getUnion()));
        EXPECT_EQ(test.getBitField(), test2.getBitField());
        EXPECT_EQ(test.getBlob(), test2.getBlob());
        EXPECT_EQ(test.getObjectType(), test2.getObjectType());
        EXPECT_EQ(test.getObjectId(), test2.getObjectId());
        EXPECT_EQ(test.getTime(), test2.getTime());
        EXPECT_TRUE(test2.getVariable() ? test.getVariable()->equalsValue(*test2.getVariable()) : false);
        EXPECT_TRUE(test.getGeneric().equalsValue(test2.getGeneric()));
    }

    TEST_F(Heat2SerializationTest, ExtraComplex)
    {
        Blaze::RawBuffer buffer(1024 * 16);
        Blaze::Heat2Encoder encoder;  encoder.setHeat1BackCompat(gEncoderHeat1Compat);
        Blaze::Heat2Decoder decoder;  decoder.setHeat1BackCompat(gDecoderHeat1Compat);
        Blaze::Protobuf::Test::AllComplexClassExtra test;

        fillTestData(test);
        EXPECT_TRUE(encoder.encode(buffer, test));

        Blaze::Protobuf::Test::AllComplexClass test2;
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.getClass().equalsValue(test2.getClass()));
        EXPECT_TRUE(test.getUnion().equalsValue(test2.getUnion()));
        EXPECT_EQ(test.getBitField(), test2.getBitField());
        EXPECT_EQ(test.getBlob(), test2.getBlob());
        EXPECT_EQ(test.getObjectType(), test2.getObjectType());
        EXPECT_EQ(test.getObjectId(), test2.getObjectId());
        EXPECT_EQ(test.getTime(), test2.getTime());
        EXPECT_TRUE(test2.getVariable() ? test.getVariable()->equalsValue(*test2.getVariable()) : false);
        EXPECT_TRUE(test.getGeneric().equalsValue(test2.getGeneric()));
    }

    TEST_F(Heat2SerializationTest, TypeChangeComplex)
    {
        Blaze::RawBuffer buffer(1024 * 16);
        Blaze::Heat2Encoder encoder;  encoder.setHeat1BackCompat(gEncoderHeat1Compat);
        Blaze::Heat2Decoder decoder;  decoder.setHeat1BackCompat(gDecoderHeat1Compat);
        Blaze::Protobuf::Test::AllComplexClassExtra test;

        fillTestData(test);
        EXPECT_TRUE(encoder.encode(buffer, test));

        Blaze::Protobuf::Test::AllComplexClassExtraTypeChange test2;
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        EXPECT_TRUE(test.getClass().equalsValue(test2.getClass()));
        EXPECT_TRUE(test.getUnion().equalsValue(test2.getUnion()));
        EXPECT_EQ(test.getBitField(), test2.getBitField());
        EXPECT_EQ(test.getBlob(), test2.getBlob());
        EXPECT_EQ(test.getObjectType(), test2.getObjectType());
        EXPECT_EQ(test.getObjectId(), test2.getObjectId());
        EXPECT_EQ(test.getTime(), test2.getTime());
        EXPECT_TRUE(test2.getVariable() ? test.getVariable()->equalsValue(*test2.getVariable()) : false);
        EXPECT_TRUE(test.getGeneric().equalsValue(test2.getGeneric()));
    }

    TEST_F(Heat2SerializationTest, UnknownTdfIdInGeneric)
    {
        Blaze::RawBuffer buffer(1024 * 16);
        Blaze::Heat2Encoder encoder;  encoder.setHeat1BackCompat(gEncoderHeat1Compat);
        Blaze::Heat2Decoder decoder;  decoder.setHeat1BackCompat(gDecoderHeat1Compat);
        Blaze::Protobuf::Test::AllComplexClassExtra test;
        fillTestData(test);



        Blaze::Protobuf::Test::AllComplexClass test3;
        fillTestData(test3);
        
        // Generic that will be made invalid by removing the id:
        test.getGeneric().get().set(test3);

        EXPECT_TRUE(encoder.encode(buffer, test));

        // Remove type to verify that encoder still handles skipping the value when set to complex types:
        EA::TDF::TdfFactory::get().deregisterType(test.getGeneric().get().getTdfId());

        Blaze::Protobuf::Test::AllComplexClassExtra test2;
        EXPECT_EQ(decoder.decode(buffer, test2), Blaze::ERR_OK);

        test.getGeneric().clear();
        EXPECT_TRUE(test.equalsValue(test2));

        EA::TDF::TdfFactory::get().registerType(Blaze::Protobuf::Test::AllComplexClass::REGISTRATION_NODE);



        Blaze::Protobuf::Test::MapIntListString mapIntListString;
        auto listOfString = mapIntListString.allocate_element();
        listOfString->push_back("Hello World!");
        listOfString->push_back("Hello World!2");

        auto listOfString2 = mapIntListString.allocate_element();
        listOfString2->push_back("Hello World!3");
        listOfString2->push_back("Hello World!4");

        mapIntListString[42] = listOfString;
        mapIntListString[0] = listOfString2;

        test.getGeneric().get().set(mapIntListString);

        EXPECT_TRUE(encoder.encode(buffer, test));

        EA::TDF::TdfFactory::get().deregisterType(test.getGeneric().get().getTdfId());

        Blaze::Protobuf::Test::AllComplexClassExtra test4;
        EXPECT_EQ(decoder.decode(buffer, test4), Blaze::ERR_OK);

        test.getGeneric().clear();
        EXPECT_TRUE(test.equalsValue(test4));

    }
}
}