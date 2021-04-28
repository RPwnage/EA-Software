/*************************************************************************************************/
/*!
\file gamereporting.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "gamereporting/gamereportingtest.h"

#include "framework/blaze.h"
#include "EATDF/tdffactory.h"
#include "framework/tdf/protobuftest_server.h"
#include "gamereporting/gamereportcompare.h"
#include "gamereporting/gametype.h"
#include "gamereporting/util/collatorutil.h"

namespace Test
{
namespace protobuf 
{
    void GameReportingTest::SetUp()
    {
        EA::TDF::TdfFactory::fixupTypes();
    }

    void GameReportingTest::TearDown()
    {

    }

    TEST_F(GameReportingTest, BasicPrimitives)
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


        Blaze::Protobuf::Test::AllPrimitivesClass test2;
        test.copyInto(test2);


        Blaze::GameReporting::GameTypeConfig gameTypeConfig;
        gameTypeConfig.setCustomDataTdf("Blaze::Protobuf::Test::AllPrimitivesClass");
        gameTypeConfig.getReport().setReportTdf("Blaze::Protobuf::Test::AllPrimitivesClass");

        Blaze::GameManager::GameReportName gameReportName = "UnitTest";

        Blaze::GameReporting::GameType gameType;
        EXPECT_TRUE(gameType.init(gameTypeConfig, gameReportName));

        Blaze::GameReporting::GameReportCompare reportCompare;
        EXPECT_TRUE(reportCompare.compare(gameType, test, test2));


        test.setInt8(-41);
        EXPECT_FALSE(reportCompare.compare(gameType, test, test2));
    }

    TEST_F(GameReportingTest, ComplexClass)
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

        Blaze::Protobuf::Test::AllComplexClass test2;
        test.copyInto(test2);


        Blaze::GameReporting::GameTypeConfig gameTypeConfig;
        gameTypeConfig.setCustomDataTdf("Blaze::Protobuf::Test::AllComplexClass");
        gameTypeConfig.getReport().setReportTdf("Blaze::Protobuf::Test::AllComplexClass");

        Blaze::GameManager::GameReportName gameReportName = "UnitTest";

        Blaze::GameReporting::GameType gameType;
        EXPECT_TRUE(gameType.init(gameTypeConfig, gameReportName));

        Blaze::GameReporting::GameReportCompare reportCompare;
        EXPECT_TRUE(reportCompare.compare(gameType, test, test2));


        test.getBitField().setBIT_3(false);
        EXPECT_FALSE(reportCompare.compare(gameType, test, test2));
    }


    TEST_F(GameReportingTest, PrimitiveLists)
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

        Blaze::Protobuf::Test::AllListsPrimitivesClass test2;
        test.copyInto(test2);


        Blaze::GameReporting::GameTypeConfig gameTypeConfig;
        gameTypeConfig.setCustomDataTdf("Blaze::Protobuf::Test::AllListsPrimitivesClass");
        gameTypeConfig.getReport().setReportTdf("Blaze::Protobuf::Test::AllListsPrimitivesClass");

        Blaze::GameManager::GameReportName gameReportName = "UnitTest";

        Blaze::GameReporting::GameType gameType;
        EXPECT_TRUE(gameType.init(gameTypeConfig, gameReportName));

        Blaze::GameReporting::GameReportCompare reportCompare;
        EXPECT_TRUE(reportCompare.compare(gameType, test, test2));


        test.getBool()[4] = false;
        EXPECT_FALSE(reportCompare.compare(gameType, test, test2));
    }
    /*
    TEST_F(GameReportingTest, ComplexLists)
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


        Blaze::Protobuf::Test::AllListsComplexClass test2;
        test.copyInto(test2);


        Blaze::GameReporting::GameTypeConfig gameTypeConfig;
        gameTypeConfig.setCustomDataTdf("Blaze::Protobuf::Test::AllListsComplexClass");
        gameTypeConfig.getReport().setReportTdf("Blaze::Protobuf::Test::AllListsComplexClass");

        Blaze::GameManager::GameReportName gameReportName = "UnitTest";

        Blaze::GameReporting::GameType gameType;
        EXPECT_TRUE(gameType.init(gameTypeConfig, gameReportName));

        Blaze::GameReporting::GameReportCompare reportCompare;
        EXPECT_TRUE(reportCompare.compare(gameType, test, test2));

        test.getClass()[4]->setBool(false);
        EXPECT_FALSE(reportCompare.compare(gameType, test, test2));
    }*/

    TEST_F(GameReportingTest, SubreportChecks)
    {
        Blaze::Protobuf::Test::AllListsComplexClass test;
        Blaze::Protobuf::Test::AllPrimitivesClass& primClass = *(test.getClass().pull_back());
        primClass.setBool(true);
        primClass.setUInt8(40);
        primClass.setUInt16(41);
        primClass.setUInt32(42);
        primClass.setUInt64(43);
        primClass.setString("Hello World!");
        test.getVariable().pull_back()->set(primClass);


        Blaze::Protobuf::Test::AllListsComplexClass test2;
        test.copyInto(test2);



        Blaze::GameReporting::GameTypeConfig gameTypeConfig;
        gameTypeConfig.setCustomDataTdf("Blaze::Protobuf::Test::AllListsComplexClass");
        gameTypeConfig.getReport().setReportTdf("Blaze::Protobuf::Test::AllListsComplexClass");
        gameTypeConfig.getReport().getSubreports()["variable"] = gameTypeConfig.getReport().getSubreports().allocate_element();
        gameTypeConfig.getReport().getSubreports()["variable"]->setReportTdf("Blaze::Protobuf::Test::AllPrimitivesClass");
        gameTypeConfig.getReport().getSubreports()["class"] = gameTypeConfig.getReport().getSubreports().allocate_element();
        gameTypeConfig.getReport().getSubreports()["class"]->setReportTdf("Blaze::Protobuf::Test::AllPrimitivesClass");

        Blaze::GameManager::GameReportName gameReportName = "UnitTest";

        Blaze::GameReporting::GameType gameType;
        EXPECT_TRUE(gameType.init(gameTypeConfig, gameReportName));

        Blaze::GameReporting::GameReportCompare reportCompare;
        EXPECT_TRUE(reportCompare.compare(gameType, test, test2));

        ((Blaze::Protobuf::Test::AllPrimitivesClass*)test.getVariable()[0]->get())->setUInt16(123);
        Blaze::GameReporting::GameReportCompare reportCompare2;
        EXPECT_FALSE(reportCompare2.compare(gameType, test, test2));

        gameTypeConfig.getReport().getSubreports()["class"]->setSkipComparison(true);
        Blaze::GameReporting::GameType gameType2;
        EXPECT_TRUE(gameType2.init(gameTypeConfig, gameReportName));
        
        Blaze::GameReporting::GameReportCompare reportCompare3;
        EXPECT_TRUE(reportCompare3.compare(gameType2, test, test2));

    }

    TEST_F(GameReportingTest, CollateChecks)
    {
        // Verify that merge modes work correctly:
        Blaze::Protobuf::Test::AllMapsComplexClass test, mergedValues, baseValues;

        // Class as value
        auto primClass = test.getIntClass().allocate_element();
        primClass->setUInt16(42);
        primClass->setString("Hello World!");

        test.getIntClass()[42] = primClass;

        // map of a <int, list<string> >
        {
            auto listOfString = test.getMapIntListString().allocate_element();
            listOfString->push_back("Hello World!");
            listOfString->push_back("Hello World!");

            test.getMapIntListString()[42] = listOfString;
        }

        // map of a <int, list<class> >
        {
            auto listOfClass = test.getMapIntListClass().allocate_element();
            listOfClass->pull_back()->setInt32(42);
            listOfClass->pull_back()->setUInt16(42);

            test.getMapIntListClass()[42] = listOfClass;
        }

        Blaze::GameReporting::Utilities::Collator collator;
        collator.setMergeMode(Blaze::GameReporting::Utilities::Collator::MERGE_COPY);
        collator.merge(mergedValues, test);

        EXPECT_TRUE(test.equalsValue(mergedValues));
        test.copyInto(baseValues);


        // Clear Values, for new entries:
        test.getIntClass().erase(42);
        test.getMapIntListString().erase(42);

        // New map entry
        primClass = test.getIntClass().allocate_element();
        primClass->setUInt16(42);
        primClass->setString("CHANGE!");

        test.getIntClass()[44] = primClass;

        // Change Value:
        {
            auto listOfString = test.getMapIntListString().allocate_element();
            listOfString->push_back("CHANGE!");
            listOfString->push_back("CHANGE!");

            test.getMapIntListString()[42] = listOfString;
        }

        // New entry (keeping old one too)
        {
            auto listOfClass = test.getMapIntListClass().allocate_element();
            listOfClass->pull_back()->setInt32(42);
            listOfClass->pull_back()->setUInt16(42);

            test.getMapIntListClass()[44] = listOfClass;
        }

        collator.setMergeMode(Blaze::GameReporting::Utilities::Collator::MERGE_COPY);
        collator.merge(mergedValues, test);
        EXPECT_TRUE(test.equalsValue(mergedValues));

        baseValues.copyInto(mergedValues);
        collator.setMergeMode(Blaze::GameReporting::Utilities::Collator::MERGE_MAPS);       // New elements are added, old elements stay the same
        collator.merge(mergedValues, test);
        EXPECT_FALSE(test.equalsValue(mergedValues));

        EXPECT_TRUE(mergedValues.getIntClass().find(42) != mergedValues.getIntClass().end());
        EXPECT_TRUE(mergedValues.getIntClass().find(44) != mergedValues.getIntClass().end());
        EXPECT_TRUE(mergedValues.getMapIntListString().find(42) != mergedValues.getMapIntListString().end());
        EXPECT_TRUE(mergedValues.getMapIntListClass().find(42) != mergedValues.getMapIntListClass().end());
        EXPECT_TRUE(mergedValues.getMapIntListClass().find(44) != mergedValues.getMapIntListClass().end());

        EXPECT_TRUE((*mergedValues.getMapIntListString()[42])[0] == "Hello World!");
        EXPECT_TRUE((mergedValues.getIntClass()[44]->getStringAsTdfString()) == "CHANGE!");
        EXPECT_TRUE((mergedValues.getIntClass()[42]->getStringAsTdfString()) == "Hello World!");

        baseValues.copyInto(mergedValues);
        collator.setMergeMode(Blaze::GameReporting::Utilities::Collator::MERGE_MAPS_COPY);   // New elements are added, overwrite existing values, old elements stay
        collator.merge(mergedValues, test);
        EXPECT_FALSE(test.equalsValue(mergedValues));

        EXPECT_TRUE(mergedValues.getIntClass().find(42) != mergedValues.getIntClass().end());
        EXPECT_TRUE(mergedValues.getIntClass().find(44) != mergedValues.getIntClass().end());
        EXPECT_TRUE(mergedValues.getMapIntListString().find(42) != mergedValues.getMapIntListString().end());
        EXPECT_TRUE(mergedValues.getMapIntListClass().find(42) != mergedValues.getMapIntListClass().end());
        EXPECT_TRUE(mergedValues.getMapIntListClass().find(44) != mergedValues.getMapIntListClass().end());

        EXPECT_TRUE((*mergedValues.getMapIntListString()[42])[0] == "CHANGE!");
        EXPECT_TRUE((mergedValues.getIntClass()[44]->getStringAsTdfString()) == "CHANGE!");
        EXPECT_TRUE((mergedValues.getIntClass()[42]->getStringAsTdfString()) == "Hello World!");
    }
}
}