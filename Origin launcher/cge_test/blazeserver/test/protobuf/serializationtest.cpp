/*************************************************************************************************/
/*!
\file serializationtest.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "protobuf/serializationtest.h"

#include "framework/blaze.h"
#include "framework/tdf/protobuftest_server.h"
#include "EATDF/tdffactory.h"

namespace Test
{
namespace protobuf 
{
    void SerializationTest::SetUp()
    {
        EA::TDF::TdfFactory::fixupTypes();
    }

    void SerializationTest::TearDown()
    {

    }

    TEST_F(SerializationTest, BasicPrimitives)
    {
        Blaze::Protobuf::Test::AllPrimitivesClass test;
        
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

        std::string serialized = test.SerializeAsString();

        Blaze::Protobuf::Test::AllPrimitivesClass test2;
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test.equalsValue(test2));
    }

    TEST_F(SerializationTest, SettingNonZeroDefaultValuesToZero)
    {
        Blaze::Protobuf::Test::AllPrimitivesClass test;

        test.setBool(false);
        test.setInt32(0);
        test.setFloat(0.0f);
        test.setString("");

        std::string serialized = test.SerializeAsString();

        Blaze::Protobuf::Test::AllPrimitivesClass test2;
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test.equalsValue(test2));
    }

    TEST_F(SerializationTest, Bitfield)
    {
        Blaze::Protobuf::Test::ABitField test;

        test.setBIT_3(true);
        test.setBIT_1_2(2);

        std::string serialized = test.SerializeAsString();

        Blaze::Protobuf::Test::ABitField test2;
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test == test2);
    }

    TEST_F(SerializationTest, PrimitiveUnion)
    {
        std::string serialized;
        Blaze::Protobuf::Test::AllPrimitivesUnion test2;

        // int64
        Blaze::Protobuf::Test::AllPrimitivesUnion test;
        test.setInt64(424242);

        serialized = test.SerializeAsString();
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test.equalsValue(test2));

        // string 
        test.setString("Hello World!");

        serialized = test.SerializeAsString();
        test2.ParseFromString(serialized);
        
        EXPECT_TRUE(test.equalsValue(test2));

        // enum
        test.setOutOfClassEnum(Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_2);

        serialized = test.SerializeAsString();
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test.equalsValue(test2));

        // empty union
        test.switchActiveMember(Blaze::Protobuf::Test::AllPrimitivesUnion::MEMBER_UNSET);

        serialized = test.SerializeAsString();
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test.equalsValue(test2));

    }

    TEST_F(SerializationTest, ComplexUnion)
    {
        std::string serialized;
        Blaze::Protobuf::Test::AllComplexUnion test2;

        // message member
        Blaze::Protobuf::Test::AllComplexUnion test;
        test.switchActiveMember(Blaze::Protobuf::Test::AllComplexUnion::MEMBER_CLASS);
        // set int32 on the message
        test.getClass()->setInt32(4242);

        serialized = test.SerializeAsString();
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test.equalsValue(test2));


        // set string on the message
        test.getClass()->setString("Hello World!");

        serialized = test.SerializeAsString();
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test.equalsValue(test2));


        // blob member
        uint8_t blob[5] = { '1', '2', '\0', '4', '5' };
        test.switchActiveMember(Blaze::Protobuf::Test::AllComplexUnion::MEMBER_BLOB);
        test.getBlob()->setData(blob, 5);

        serialized = test.SerializeAsString();
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test.equalsValue(test2));


        // ObjectType member
        EA::TDF::ObjectType objType(42, 0);
        test.setObjectType(objType);

        serialized = test.SerializeAsString();
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test.equalsValue(test2));


        // ObjectId member
        EA::TDF::ObjectId objId(42, 0, 42);
        test.setObjectId(objId);

        serialized = test.SerializeAsString();
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test.equalsValue(test2));


        // TimeValue member
        EA::TDF::TimeValue timeValue(42);
        test.setTime(timeValue);

        serialized = test.SerializeAsString();
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test.equalsValue(test2));


        // variable member
        Blaze::Protobuf::Test::AllPrimitivesClass primClass;
        primClass.setUInt8(42);

        test.switchActiveMember(Blaze::Protobuf::Test::AllComplexUnion::MEMBER_VARIABLE);
        test.getVariable()->set(primClass);

        serialized = test.SerializeAsString();
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test.equalsValue(test2));


        // generic member
        test.switchActiveMember(Blaze::Protobuf::Test::AllComplexUnion::MEMBER_GENERIC);
        test.getGeneric()->get().set("Hello World!");

        serialized = test.SerializeAsString();
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test.equalsValue(test2));
    }

    TEST_F(SerializationTest, Blob)
    {
        Blaze::Protobuf::Test::AllComplexClass test;

        uint8_t blob[5] = { '1', '2', '\0', '4', '5' };
        test.getBlob().setData(blob, 5);

        std::string serialized = test.SerializeAsString();

        Blaze::Protobuf::Test::AllComplexClass test2;
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test.equalsValue(test2));
    }

    TEST_F(SerializationTest, ComplexClass)
    {
        Blaze::Protobuf::Test::AllComplexClass test;

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

        Blaze::Protobuf::Test::AllPrimitivesClass primClass;
        primClass.setUInt8(42);

        test.setVariable(primClass);

        test.getGeneric().get().set(42);

        std::string serialized = test.SerializeAsString();

        Blaze::Protobuf::Test::AllComplexClass test2;
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test.equalsValue(test2));
    }

    TEST_F(SerializationTest, Generic)
    {
        Blaze::Protobuf::Test::AllComplexClass test;

        // bool
        {
            test.getGeneric().get().set(true);

            std::string serialized = test.SerializeAsString();

            Blaze::Protobuf::Test::AllComplexClass test2;
            test2.ParseFromString(serialized);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // unsigned
        {
            test.getGeneric().get().set(4242u);

            std::string serialized = test.SerializeAsString();

            Blaze::Protobuf::Test::AllComplexClass test2;
            test2.ParseFromString(serialized);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // int32
        {
            test.getGeneric().get().set(-42);

            std::string serialized = test.SerializeAsString();

            Blaze::Protobuf::Test::AllComplexClass test2;
            test2.ParseFromString(serialized);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // float
        {
            test.getGeneric().get().set(42.42f);

            std::string serialized = test.SerializeAsString();

            Blaze::Protobuf::Test::AllComplexClass test2;
            test2.ParseFromString(serialized);

            EXPECT_TRUE(test.equalsValue(test2));
        }


        // int64
        {
            test.getGeneric().get().set((int64_t)(-42));

            std::string serialized = test.SerializeAsString();

            Blaze::Protobuf::Test::AllComplexClass test2;
            test2.ParseFromString(serialized);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // uint64
        {
            test.getGeneric().get().set((uint64_t)(42));

            std::string serialized = test.SerializeAsString();

            Blaze::Protobuf::Test::AllComplexClass test2;
            test2.ParseFromString(serialized);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // string
        {
            test.getGeneric().get().set("Hello World!");

            std::string serialized = test.SerializeAsString();

            Blaze::Protobuf::Test::AllComplexClass test2;
            test2.ParseFromString(serialized);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // blob
        {
            uint8_t blob[5] = { '1', '2', '\0', '4', '5' };
            EA::TDF::TdfBlob tdfBlob;
            tdfBlob.setData(blob, 5);

            test.getGeneric().get().set(tdfBlob);

            std::string serialized = test.SerializeAsString();

            Blaze::Protobuf::Test::AllComplexClass test2;
            test2.ParseFromString(serialized);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // ObjectType
        {
            EA::TDF::ObjectType objType(42, 0);

            test.getGeneric().get().set(objType);

            std::string serialized = test.SerializeAsString();

            Blaze::Protobuf::Test::AllComplexClass test2;
            test2.ParseFromString(serialized);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // ObjectId
        {
            EA::TDF::ObjectId objId(42, 0, 4242);

            test.getGeneric().get().set(objId);

            std::string serialized = test.SerializeAsString();

            Blaze::Protobuf::Test::AllComplexClass test2;
            test2.ParseFromString(serialized);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // TimeValue
        {
            EA::TDF::TimeValue timeValue(42);

            test.getGeneric().get().set(timeValue);

            std::string serialized = test.SerializeAsString();

            Blaze::Protobuf::Test::AllComplexClass test2;
            test2.ParseFromString(serialized);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // bitfield
        {
            Blaze::Protobuf::Test::ABitField bitfield;
            bitfield.setBIT_3(true);
            bitfield.setBIT_1_2(2);

            test.getGeneric().get().set(bitfield);

            std::string serialized = test.SerializeAsString();

            Blaze::Protobuf::Test::AllComplexClass test2;
            test2.ParseFromString(serialized);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // Union
        {
            Blaze::Protobuf::Test::AllPrimitivesUnion primsUnion;
            primsUnion.setInt64(424242);

            test.getGeneric().get().set(primsUnion);

            std::string serialized = test.SerializeAsString();

            Blaze::Protobuf::Test::AllComplexClass test2;
            test2.ParseFromString(serialized);

            EXPECT_TRUE(test.equalsValue(test2));
        }

        // Message
        {
            Blaze::Protobuf::Test::AllPrimitivesClass primsClass;
            primsClass.setInt64(424242);

            test.getGeneric().get().set(primsClass);

            std::string serialized = test.SerializeAsString();

            Blaze::Protobuf::Test::AllComplexClass test2;
            test2.ParseFromString(serialized);

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

                std::string serialized = test.SerializeAsString();

                Blaze::Protobuf::Test::AllComplexClass test2;
                test2.ParseFromString(serialized);

                EXPECT_TRUE(test.equalsValue(test2));
            }

            //string list
            {
                EA::TDF::TdfPrimitiveVector<EA::TDF::TdfString> stringList;
                stringList.push_back("Hello World!");
                stringList.push_back("42");

                test.getGeneric().get().set(stringList);

                std::string serialized = test.SerializeAsString();

                Blaze::Protobuf::Test::AllComplexClass test2;
                test2.ParseFromString(serialized);

                EXPECT_TRUE(test.equalsValue(test2));
            }

            // ObjectType list
            {
                EA::TDF::TdfPrimitiveVector<EA::TDF::ObjectType> objectTypeList;
                objectTypeList.push_back(EA::TDF::ObjectType(42, 0));
                objectTypeList.push_back(EA::TDF::ObjectType(42, 0));

                test.getGeneric().get().set(objectTypeList);

                std::string serialized = test.SerializeAsString();

                Blaze::Protobuf::Test::AllComplexClass test2;
                test2.ParseFromString(serialized);

                EXPECT_TRUE(test.equalsValue(test2));
            }

            // ObjectId list
            {
                EA::TDF::TdfPrimitiveVector<EA::TDF::ObjectId> objectIdList;
                objectIdList.push_back(EA::TDF::ObjectId(42, 0, 42));
                objectIdList.push_back(EA::TDF::ObjectId(42, 0, 42));

                test.getGeneric().get().set(objectIdList);

                std::string serialized = test.SerializeAsString();

                Blaze::Protobuf::Test::AllComplexClass test2;
                test2.ParseFromString(serialized);

                EXPECT_TRUE(test.equalsValue(test2));
            }

            // TimeValue list
            {
                EA::TDF::TdfPrimitiveVector<EA::TDF::TimeValue> timeValueList;
                timeValueList.push_back(EA::TDF::TimeValue(42));
                timeValueList.push_back(EA::TDF::TimeValue(4242));

                test.getGeneric().get().set(timeValueList);

                std::string serialized = test.SerializeAsString();

                Blaze::Protobuf::Test::AllComplexClass test2;
                test2.ParseFromString(serialized);

                EXPECT_TRUE(test.equalsValue(test2));
            }
        }
    }

    TEST_F(SerializationTest, PrimitiveLists)
    {
        Blaze::Protobuf::Test::AllListsPrimitivesClass test;
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

        std::string serialized = test.SerializeAsString();

        Blaze::Protobuf::Test::AllListsPrimitivesClass test2;
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test.equalsValue(test2));
    }

    TEST_F(SerializationTest, ComplexLists)
    {
        Blaze::Protobuf::Test::AllListsComplexClass test;
        const int kListSize = 5;
        for (int i = 0; i < kListSize; ++i)
        {
            uint8_t blob[5] = { '1', '2', '\0', '4', '5' };
            test.getBlob().pull_back()->setData(blob, 5);

            Blaze::Protobuf::Test::AllPrimitivesClass& primClass = *(test.getClass().pull_back());
            primClass.setBool(true);
            primClass.setUInt8(40);
            primClass.setUInt16(41);
            primClass.setUInt32(42);
            primClass.setUInt64(43);
            primClass.setInt8(-40);
            primClass.setInt16(-41);
            primClass.setInt32(-42);
            primClass.setInt64(-43);
            primClass.setFloat(42.42f);
            primClass.setOutOfClassEnum(Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_1);
            primClass.setString("Hello World!");

            test.getUnion().pull_back()->setString("Hello World!");

            Blaze::Protobuf::Test::AllPrimitivesClass primClass2;
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


        std::string serialized = test.SerializeAsString();

        Blaze::Protobuf::Test::AllListsComplexClass test2;
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test.equalsValue(test2));
    }

    TEST_F(SerializationTest, PrimitiveMaps)
    {
        Blaze::Protobuf::Test::AllMapsPrimitivesClass test;

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


        std::string serialized = test.SerializeAsString();

        Blaze::Protobuf::Test::AllMapsPrimitivesClass test2;
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test.equalsValue(test2));
    }

    TEST_F(SerializationTest, ComplexMaps)
    {
        Blaze::Protobuf::Test::AllMapsComplexClass test;

        // Class as value
        auto primClass = test.getIntClass().allocate_element();
        primClass->setUInt16(42);
        primClass->setString("Hello World!");

        test.getIntClass()[42] = primClass;
        test.getStringClass()["42"] = primClass;
        test.getEnumClass()[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_0] = primClass;

        // union as value
        auto unionClass = test.getIntUnion().allocate_element();
        unionClass->setString("Hello World!");

        test.getIntUnion()[42] = unionClass;
        test.getStringUnion()["42"] = unionClass;
        test.getEnumUnion()[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_1] = unionClass;


        // bitfied as value
        Blaze::Protobuf::Test::ABitField bitfield;
        bitfield.setBIT_3(true);
        bitfield.setBIT_1_2(2);

        test.getIntBitField()[42] = bitfield;
        test.getStringBitField()["42"] = bitfield;
        test.getEnumBitField()[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_0] = bitfield;

        // blob as value
        uint8_t blob[5] = { '1', '2', '\0', '4', '5' };
        auto blobClass = test.getIntBlob().allocate_element();
        blobClass->setData(blob, 5);

        test.getIntBlob()[42] = blobClass;
        test.getStringBlob()["42"] = blobClass;
        test.getEnumBlob()[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_2] = blobClass;

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

        test.getIntVariable()[42] = &variableTdf;
        test.getStringVariable()["42"] = &variableTdf;
        test.getEnumVariable()[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_0] = &variableTdf;

        // generic tdf as value
        auto genericTdf = test.getIntGeneric().allocate_element();
        genericTdf->get().set(42.42f);

        test.getIntGeneric()[42] = genericTdf;
        test.getStringGeneric()["42"] = genericTdf;
        test.getEnumGeneric()[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_0] = genericTdf;

        // map of a <int, list<int> >
        {
            auto listOfInt = test.getMapIntListInt().allocate_element();
            listOfInt->push_back(42);
            listOfInt->push_back(4242);

            test.getMapIntListInt()[42] = listOfInt;
            test.getMapIntListInt()[0] = listOfInt;
        }

        // map of a <int, list<enum> >
        {
            auto listOfEnum = test.getMapIntListEnum().allocate_element();
            listOfEnum->push_back(Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_1);
            listOfEnum->push_back(Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_0);

            test.getMapIntListEnum()[42] = listOfEnum;
            test.getMapIntListEnum()[0] = listOfEnum;
        }

        // map of a <int, list<string> >
        {
            auto listOfString = test.getMapIntListString().allocate_element();
            listOfString->push_back("Hello World!");
            listOfString->push_back("Hello World!");

            test.getMapIntListString()[42] = listOfString;
            test.getMapIntListString()[0] = listOfString;
        }

        // map of a <int, list<class> >
        {
            auto listOfClass = test.getMapIntListClass().allocate_element();
            listOfClass->pull_back()->setInt32(42);
            listOfClass->pull_back()->setUInt16(42);

            test.getMapIntListClass()[42] = listOfClass;
            test.getMapIntListClass()[0] = listOfClass;
        }

        // map of a <int, map<int, int> >
        {
            auto mapOfIntInt = test.getMapIntMapIntInt().allocate_element();
            (*mapOfIntInt)[42] = 42;
            (*mapOfIntInt)[0] = 42;

            test.getMapIntMapIntInt()[42] = mapOfIntInt;
            test.getMapIntMapIntInt()[0] = mapOfIntInt;
        }

        // map of a <int, map<string, string> >
        {
            auto mapOfStringString = test.getMapIntMapStringString().allocate_element();
            (*mapOfStringString)["42"] = "42";
            (*mapOfStringString)["0"] = "42";

            test.getMapIntMapStringString()[42] = mapOfStringString;
            test.getMapIntMapStringString()[0] = mapOfStringString;
        }

        // map of a <int, map<string, class> >
        {
            auto mapOfStringClass = test.getMapIntMapStringClass().allocate_element();
            auto primClass2 = mapOfStringClass->allocate_element();
            primClass2->setString("Hello World!");

            (*mapOfStringClass)["42"] = primClass2;
            (*mapOfStringClass)["0"] = primClass2;

            test.getMapIntMapStringClass()[42] = mapOfStringClass;
            test.getMapIntMapStringClass()[0] = mapOfStringClass;
        }

        // map of a <int, map<enum, string> >
        {
            auto mapOfEnumString = test.getMapIntMapEnumString().allocate_element();
            (*mapOfEnumString)[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_1] = "42";
            (*mapOfEnumString)[Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_1] = "0";

            test.getMapIntMapEnumString()[42] = mapOfEnumString;
            test.getMapIntMapEnumString()[0] = mapOfEnumString;
        }


        std::string serialized = test.SerializeAsString();

        Blaze::Protobuf::Test::AllMapsComplexClass test2;
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test.equalsValue(test2));
    }

    TEST_F(SerializationTest, NestedClass)
    {
        Blaze::Protobuf::Test::OuterClass test;

        test.setUInt8(42);
        test.getInner().setUInt8(42);
        test.getInner().getInnerInnerClass().setUInt8(42);

        std::string serialized = test.SerializeAsString();

        Blaze::Protobuf::Test::OuterClass test2;
        test2.ParseFromString(serialized);

        EXPECT_TRUE(test.equalsValue(test2));
    }
}
}