/*************************************************************************************************/
/*!
\file mergeoptest.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "matchmaker/mergeoptest.h"

#include "framework/blaze.h"
#include "test/mock/mocklogger.h"
#include "gamemanager/templateattributes.h"
#include "EATDF/tdffactory.h"

namespace Test
{
namespace mergeop 
{

    void MergeOpTest::SetUp()
    {
        EA::TDF::TdfFactory::fixupTypes();
    }

    void MergeOpTest::TearDown()
    {

    }

    TEST_F(MergeOpTest, MergeAverageTest)
    {
        EA::TDF::GenericTdfType mergedGeneric;
        Blaze::GameManager::ListUInt listUInt;
        listUInt.push_back(10);
        listUInt.push_back(20);
        listUInt.push_back(30);
        listUInt.push_back(40);
        listUInt.push_back(50);

        EA::TDF::TdfGenericReferenceConst sourceValue(listUInt);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue, Blaze::GameManager::MERGE_AVERAGE, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_UINT64);
        EXPECT_EQ(mergedGeneric.getValue().asUInt64(), 30);


        Blaze::GameManager::ListInt listInt;
        listInt.push_back(10);
        listInt.push_back(-20);
        listInt.push_back(30);
        listInt.push_back(-40);
        listInt.push_back(50);

        EA::TDF::TdfGenericReferenceConst sourceValue2(listInt);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue2, Blaze::GameManager::MERGE_AVERAGE, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_INT64);
        EXPECT_EQ(mergedGeneric.getValue().asInt64(), 6);


        Blaze::GameManager::ListFloat listFloat;
        listFloat.push_back( 10.5f);
        listFloat.push_back(-20.5f);
        listFloat.push_back( 30.5f);
        listFloat.push_back(-40.5f);
        listFloat.push_back( 50.0f);

        EA::TDF::TdfGenericReferenceConst sourceValue3(listFloat);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue3, Blaze::GameManager::MERGE_AVERAGE, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_FLOAT);
        EXPECT_EQ(mergedGeneric.getValue().asFloat (), 6.0f);

    }

    TEST_F(MergeOpTest, MergeMinTest)
    {
        EA::TDF::GenericTdfType mergedGeneric;
        Blaze::GameManager::ListUInt listUInt;
        listUInt.push_back(10);
        listUInt.push_back(20);
        listUInt.push_back(30);
        listUInt.push_back(40);
        listUInt.push_back(50);

        EA::TDF::TdfGenericReferenceConst sourceValue(listUInt);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue, Blaze::GameManager::MERGE_MIN, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_UINT64);
        EXPECT_EQ(mergedGeneric.getValue().asUInt64(), 10);


        Blaze::GameManager::ListInt listInt;
        listInt.push_back(10);
        listInt.push_back(-20);
        listInt.push_back(30);
        listInt.push_back(-40);
        listInt.push_back(50);

        EA::TDF::TdfGenericReferenceConst sourceValue2(listInt);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue2, Blaze::GameManager::MERGE_MIN, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_INT64);
        EXPECT_EQ(mergedGeneric.getValue().asInt64(), -40);


        Blaze::GameManager::ListFloat listFloat;
        listFloat.push_back(10.5f);
        listFloat.push_back(-20.5f);
        listFloat.push_back(30.5f);
        listFloat.push_back(-40.5f);
        listFloat.push_back(50.0f);

        EA::TDF::TdfGenericReferenceConst sourceValue3(listFloat);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue3, Blaze::GameManager::MERGE_MIN, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_FLOAT);
        EXPECT_EQ(mergedGeneric.getValue().asFloat(), -40.5f);

    }
    TEST_F(MergeOpTest, MergeMaxTest)
    {
        EA::TDF::GenericTdfType mergedGeneric;
        Blaze::GameManager::ListUInt listUInt;
        listUInt.push_back(10);
        listUInt.push_back(20);
        listUInt.push_back(30);
        listUInt.push_back(40);
        listUInt.push_back(50);

        EA::TDF::TdfGenericReferenceConst sourceValue(listUInt);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue, Blaze::GameManager::MERGE_MAX, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_UINT64);
        EXPECT_EQ(mergedGeneric.getValue().asUInt64(), 50);


        Blaze::GameManager::ListInt listInt;
        listInt.push_back(-10);
        listInt.push_back(-20);
        listInt.push_back(-30);
        listInt.push_back(-40);
        listInt.push_back(-50);

        EA::TDF::TdfGenericReferenceConst sourceValue2(listInt);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue2, Blaze::GameManager::MERGE_MAX, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_INT64);
        EXPECT_EQ(mergedGeneric.getValue().asInt64(), -10);


        Blaze::GameManager::ListFloat listFloat;
        listFloat.push_back(10.5f);
        listFloat.push_back(-20.5f);
        listFloat.push_back(30.5f);
        listFloat.push_back(-40.5f);
        listFloat.push_back(50.0f);

        EA::TDF::TdfGenericReferenceConst sourceValue3(listFloat);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue3, Blaze::GameManager::MERGE_MAX, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_FLOAT);
        EXPECT_EQ(mergedGeneric.getValue().asFloat(), 50.0f);

    }
    TEST_F(MergeOpTest, MergeSumTest)
    {
        EA::TDF::GenericTdfType mergedGeneric;
        Blaze::GameManager::ListUInt listUInt;
        listUInt.push_back(10);
        listUInt.push_back(20);
        listUInt.push_back(30);
        listUInt.push_back(40);
        listUInt.push_back(50);

        EA::TDF::TdfGenericReferenceConst sourceValue(listUInt);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue, Blaze::GameManager::MERGE_SUM, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_UINT64);
        EXPECT_EQ(mergedGeneric.getValue().asUInt64(), 150);


        Blaze::GameManager::ListInt listInt;
        listInt.push_back(10);
        listInt.push_back(-20);
        listInt.push_back(30);
        listInt.push_back(-40);
        listInt.push_back(50);

        EA::TDF::TdfGenericReferenceConst sourceValue2(listInt);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue2, Blaze::GameManager::MERGE_SUM, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_INT64);
        EXPECT_EQ(mergedGeneric.getValue().asInt64(), 30);


        Blaze::GameManager::ListFloat listFloat;
        listFloat.push_back(10.5f);
        listFloat.push_back(-20.5f);
        listFloat.push_back(30.5f);
        listFloat.push_back(-40.5f);
        listFloat.push_back(50.0f);

        EA::TDF::TdfGenericReferenceConst sourceValue3(listFloat);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue3, Blaze::GameManager::MERGE_SUM, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_FLOAT);
        EXPECT_EQ(mergedGeneric.getValue().asFloat(), 30.0f);

    }
    TEST_F(MergeOpTest, MergeStdDevTest)
    {
        EA::TDF::GenericTdfType mergedGeneric;
        Blaze::GameManager::ListUInt listUInt;
        listUInt.push_back(10);  // 20
        listUInt.push_back(20);  // 10
        listUInt.push_back(30);  // 0
        listUInt.push_back(40);  // 10
        listUInt.push_back(50);  // 20

        EA::TDF::TdfGenericReferenceConst sourceValue(listUInt);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue, Blaze::GameManager::MERGE_STDDEV, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_UINT64);
        EXPECT_EQ(mergedGeneric.getValue().asUInt64(), 12);


        Blaze::GameManager::ListInt listInt;
        listInt.push_back(10);    // 4
        listInt.push_back(-20);   // 26
        listInt.push_back(30);    // 24 
        listInt.push_back(-40);   // 46
        listInt.push_back(50);    // 44 

        EA::TDF::TdfGenericReferenceConst sourceValue2(listInt);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue2, Blaze::GameManager::MERGE_STDDEV, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_INT64);
        EXPECT_EQ(mergedGeneric.getValue().asInt64(), 28 );  //144/5


        Blaze::GameManager::ListFloat listFloat;
        listFloat.push_back(10.5f);   // 4.5
        listFloat.push_back(-20.5f);  // 26.5
        listFloat.push_back(30.5f);   // 24.5
        listFloat.push_back(-40.5f);  // 46.5
        listFloat.push_back(50.0f);   // 44

        EA::TDF::TdfGenericReferenceConst sourceValue3(listFloat);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue3, Blaze::GameManager::MERGE_STDDEV, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_FLOAT);
        EXPECT_EQ(mergedGeneric.getValue().asFloat(), 29.2f);  //146 / 5

    }
    TEST_F(MergeOpTest, MergeMinMaxDeltaTest)
    {
        EA::TDF::GenericTdfType mergedGeneric;
        Blaze::GameManager::ListUInt listUInt;
        listUInt.push_back(10);
        listUInt.push_back(20);
        listUInt.push_back(30);
        listUInt.push_back(40);
        listUInt.push_back(50);

        EA::TDF::TdfGenericReferenceConst sourceValue(listUInt);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue, Blaze::GameManager::MERGE_MIN_MAX_RANGE, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_UINT64);
        EXPECT_EQ(mergedGeneric.getValue().asUInt64(), 40);


        Blaze::GameManager::ListInt listInt;
        listInt.push_back(10);
        listInt.push_back(-20);
        listInt.push_back(30);
        listInt.push_back(-40);
        listInt.push_back(50);

        EA::TDF::TdfGenericReferenceConst sourceValue2(listInt);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue2, Blaze::GameManager::MERGE_MIN_MAX_RANGE, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_INT64);
        EXPECT_EQ(mergedGeneric.getValue().asInt64(), 90);


        Blaze::GameManager::ListFloat listFloat;
        listFloat.push_back(10.5f);
        listFloat.push_back(-20.5f);
        listFloat.push_back(30.5f);
        listFloat.push_back(-40.5f);
        listFloat.push_back(50.0f);

        EA::TDF::TdfGenericReferenceConst sourceValue3(listFloat);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue3, Blaze::GameManager::MERGE_MIN_MAX_RANGE, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_FLOAT);
        EXPECT_EQ(mergedGeneric.getValue().asFloat(), 90.5f);

    }

    TEST_F(MergeOpTest, MergeAverageMapTest)
    {
        EA::TDF::GenericTdfType mergedGeneric;

        Blaze::GameManager::PropertyValueMapList pingsiteLatency;  // Latency for the 'players'
        auto curLatencyMap = pingsiteLatency.allocate_element();
        //(*curLatencyMap)["A"] = 50;
        (*curLatencyMap)["B"] = 50;
        (*curLatencyMap)["C"] = 50;
        (*curLatencyMap)["D"] = 50;
        (*curLatencyMap)["E"] = 50;
        (*curLatencyMap)["F"] = 50;
        pingsiteLatency.push_back(curLatencyMap);

        curLatencyMap = pingsiteLatency.allocate_element();
        (*curLatencyMap)["A"] = 250;
        //(*curLatencyMap)["B"] = 50;
        (*curLatencyMap)["C"] = 230;
        (*curLatencyMap)["D"] = 250;
        (*curLatencyMap)["E"] = -50;
        (*curLatencyMap)["F"] = -50;
        pingsiteLatency.push_back(curLatencyMap);

        curLatencyMap = pingsiteLatency.allocate_element();
        //(*curLatencyMap)["A"] = 50;
        (*curLatencyMap)["B"] = 50;
        (*curLatencyMap)["C"] = 50;
        //(*curLatencyMap)["D"] = 250;
        (*curLatencyMap)["E"] = 60;
        (*curLatencyMap)["F"] = 0;
        pingsiteLatency.push_back(curLatencyMap);


        EA::TDF::TdfGenericReferenceConst sourceValue(pingsiteLatency);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue, Blaze::GameManager::MERGE_AVERAGE, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_MAP);
        EXPECT_EQ(mergedGeneric.getValue().asMap().getKeyType(), EA::TDF::TDF_ACTUAL_TYPE_STRING);
        EXPECT_EQ(mergedGeneric.getValue().asMap().getValueType(), EA::TDF::TDF_ACTUAL_TYPE_INT64);

        auto result = (Blaze::GameManager::MapStringInt*) &mergedGeneric.getValue().asMap();
        EXPECT_EQ(result->mapSize(), 6);
        EXPECT_EQ((*result)["A"], 250);
        EXPECT_EQ((*result)["B"], 50);
        EXPECT_EQ((*result)["C"], 110);
        EXPECT_EQ((*result)["D"], 150);
        EXPECT_EQ((*result)["E"], 20);
        EXPECT_EQ((*result)["F"], 0);

    }
    TEST_F(MergeOpTest, MergeStdDevMapTest)
    {
        EA::TDF::GenericTdfType mergedGeneric;

        Blaze::GameManager::PropertyValueMapList pingsiteLatency;  // Latency for the 'players'
        auto curLatencyMap = pingsiteLatency.allocate_element();
        //(*curLatencyMap)["A"] = 50;
        (*curLatencyMap)["B"] = 50;
        (*curLatencyMap)["C"] = 50;
        (*curLatencyMap)["D"] = 50;
        (*curLatencyMap)["E"] = 50;
        (*curLatencyMap)["F"] = 50;
        pingsiteLatency.push_back(curLatencyMap);

        curLatencyMap = pingsiteLatency.allocate_element();
        (*curLatencyMap)["A"] = 250;
        //(*curLatencyMap)["B"] = 50;
        (*curLatencyMap)["C"] = 230;
        (*curLatencyMap)["D"] = 250;
        (*curLatencyMap)["E"] = -50;
        (*curLatencyMap)["F"] = -50;
        pingsiteLatency.push_back(curLatencyMap);

        curLatencyMap = pingsiteLatency.allocate_element();
        //(*curLatencyMap)["A"] = 50;
        (*curLatencyMap)["B"] = 50;
        (*curLatencyMap)["C"] = 50;
        //(*curLatencyMap)["D"] = 250;
        (*curLatencyMap)["E"] = 60;
        (*curLatencyMap)["F"] = 0;
        pingsiteLatency.push_back(curLatencyMap);


        EA::TDF::TdfGenericReferenceConst sourceValue(pingsiteLatency);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue, Blaze::GameManager::MERGE_STDDEV, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_MAP);
        EXPECT_EQ(mergedGeneric.getValue().asMap().getKeyType(), EA::TDF::TDF_ACTUAL_TYPE_STRING);
        EXPECT_EQ(mergedGeneric.getValue().asMap().getValueType(), EA::TDF::TDF_ACTUAL_TYPE_INT64);

        auto result = (Blaze::GameManager::MapStringInt*) &mergedGeneric.getValue().asMap();
        EXPECT_EQ(result->mapSize(), 6);
        EXPECT_EQ((*result)["A"], 0);
        EXPECT_EQ((*result)["B"], 0);
        EXPECT_EQ((*result)["C"], 80);   //60+120+60 / 3
        EXPECT_EQ((*result)["D"], 100);  //100+100/2
        EXPECT_EQ((*result)["E"], 46);   //30+70+40/3 
        EXPECT_EQ((*result)["F"], 33);    //50+50+0/3

    }
    TEST_F(MergeOpTest, MergeMinMaxDeltaMapTest)
    {
        EA::TDF::GenericTdfType mergedGeneric;

        Blaze::GameManager::PropertyValueMapList pingsiteLatency;  // Latency for the 'players'
        auto curLatencyMap = pingsiteLatency.allocate_element();
        //(*curLatencyMap)["A"] = 50;
        (*curLatencyMap)["B"] = 55;
        (*curLatencyMap)["C"] = 50;
        (*curLatencyMap)["D"] = 10;
        (*curLatencyMap)["E"] = 50;
        (*curLatencyMap)["F"] = 30;
        pingsiteLatency.push_back(curLatencyMap);

        curLatencyMap = pingsiteLatency.allocate_element();
        (*curLatencyMap)["A"] = 250;
        //(*curLatencyMap)["B"] = 50;
        (*curLatencyMap)["C"] = 230;
        (*curLatencyMap)["D"] = 250;
        (*curLatencyMap)["E"] = -50;
        (*curLatencyMap)["F"] = -50;
        pingsiteLatency.push_back(curLatencyMap);

        curLatencyMap = pingsiteLatency.allocate_element();
        //(*curLatencyMap)["A"] = 50;
        (*curLatencyMap)["B"] = 50;
        (*curLatencyMap)["C"] = 10;
        //(*curLatencyMap)["D"] = 250;
        (*curLatencyMap)["E"] = 60;
        (*curLatencyMap)["F"] = 0;
        pingsiteLatency.push_back(curLatencyMap);


        EA::TDF::TdfGenericReferenceConst sourceValue(pingsiteLatency);
        EXPECT_TRUE(Blaze::GameManager::attemptMergeOp(sourceValue, Blaze::GameManager::MERGE_MIN_MAX_RANGE, mergedGeneric));

        EXPECT_EQ(mergedGeneric.getValue().getType(), EA::TDF::TDF_ACTUAL_TYPE_MAP);
        EXPECT_EQ(mergedGeneric.getValue().asMap().getKeyType(), EA::TDF::TDF_ACTUAL_TYPE_STRING);
        EXPECT_EQ(mergedGeneric.getValue().asMap().getValueType(), EA::TDF::TDF_ACTUAL_TYPE_INT64);

        auto result = (Blaze::GameManager::MapStringInt*) &mergedGeneric.getValue().asMap();
        EXPECT_EQ(result->mapSize(), 6);
        EXPECT_EQ((*result)["A"], 0);
        EXPECT_EQ((*result)["B"], 5);
        EXPECT_EQ((*result)["C"], 220);
        EXPECT_EQ((*result)["D"], 240);
        EXPECT_EQ((*result)["E"], 110);
        EXPECT_EQ((*result)["F"], 80);

    }
}
}