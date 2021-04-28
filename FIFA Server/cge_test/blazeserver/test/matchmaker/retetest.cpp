/*************************************************************************************************/
/*!
\file retetest.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "test/matchmaker/retetest.h"
#include "gamemanager/matchmaker/ruledefinitioncollection.h"


namespace BlazeServerTest
{
namespace Matchmaker
{

    using ::testing::AtLeast;
    using ::testing::Return;
    using ::testing::NiceMock;
    using ::testing::_;

    ReteTest::ReteTest()
        : mReteNetwork(mMetricsCollection, 0)
    {
        mRuleDefinitionCollection = BLAZE_NEW RuleDefinitionCollection(mMetricsCollection, mStringTable, 0, false);
    }

    void ReteTest::SetUp()
    {
        mReteNetwork.configure(mReteConfig);
    }

    ReteTest::~ReteTest()
    {
        delete mRuleDefinitionCollection;
    }

    void ReteTest::registerRuleA(MockReteRuleDefinition& providerDef, MockConditionProvider& provider) const
    {
        EXPECT_CALL(providerDef, getName())
            .WillRepeatedly(Return(RETE_KEY_A));
        // making the cache return match any hash, just forces it to create the name string.
        EXPECT_CALL(providerDef, getCachedWMEAttributeName(RETE_KEY_A))
            .WillRepeatedly(Return(WME_ANY_HASH));
        EXPECT_CALL(providerDef, getID())
            .WillRepeatedly(Return(PROVIDER_ID_A));

        EXPECT_CALL(provider, getProviderName())
            .WillRepeatedly(Return(PROVIDER_NAME_A));
    }

    void ReteTest::registerRuleB(MockReteRuleDefinition& providerDef, MockConditionProvider& provider) const
    {
        EXPECT_CALL(providerDef, getName())
            .WillRepeatedly(Return(RETE_KEY_B));
        // making the cache return match any hash, just forces it to create the name string.
        EXPECT_CALL(providerDef, getCachedWMEAttributeName(RETE_KEY_B))
            .WillRepeatedly(Return(WME_ANY_HASH));
        EXPECT_CALL(providerDef, getID())
            .WillRepeatedly(Return(PROVIDER_ID_B));

        EXPECT_CALL(provider, getProviderName())
            .WillRepeatedly(Return(PROVIDER_NAME_B));
    }

    TEST_F(ReteTest, testRangeFilterMatch)
    {
        // Arrange - add data to filter against added conditions

        // random value that we are matching against
        const WMEAttribute attValue = 123;
        // normally, this is the game id.
        const WMEId attId = 1;

        MockReteRuleDefinition providerDef;
        providerDef.setRuleDefinitionCollection(*mRuleDefinitionCollection);
        MockConditionProvider provider;

        registerRuleA(providerDef, provider);

        mReteNetwork.getWMEManager().registerMatchAnyWME(attId);
        mReteNetwork.getWMEManager().insertWorkingMemoryElement(attId, RETE_KEY_A, attValue, providerDef);

        // gTest quirk: use NiceTest here to surpress warnings about no EXPECT_CALLS on all methods of MockProductionListener.
        NiceMock<MockProductionListener> production;
        EXPECT_CALL(production, getProductionId())
            .WillRepeatedly(Return(PRODUCTION_ID_1));

        // Act - add the production listener with a range filter to RETE to start filtering

        production.getConditions().resize(CONDITION_BLOCK_MAX_SIZE);
        ConditionBlock& conditions = production.getConditions().at(CONDITION_BLOCK_BASE_SEARCH);
        OrConditionList& orCondition = conditions.push_back();
        orCondition.push_back(ConditionInfo(
            Condition(RETE_KEY_A, 100, 200, providerDef),
            0, &provider
        ));
        

        // Assert - validate the filtering matches
        EXPECT_CALL(production, onTokenAdded(_, _))
            .Times(1)
            .WillRepeatedly(Return(1));

        mReteNetwork.getProductionManager().addProduction(production);

        mReteNetwork.getProductionManager().removeProduction(production);
    }


    TEST_F(ReteTest, testRangeFilterNoMatch)
    {
        // Arrange - add data to filter against added conditions

        // random value that we are matching against
        const WMEAttribute attValue = 23;
        // normally, this is the game id.
        const WMEId attId = 1;

        MockReteRuleDefinition providerDef;
        providerDef.setRuleDefinitionCollection(*mRuleDefinitionCollection);
        MockConditionProvider provider;
        
        registerRuleA(providerDef, provider);

        mReteNetwork.getWMEManager().registerMatchAnyWME(attId);
        mReteNetwork.getWMEManager().insertWorkingMemoryElement(attId, RETE_KEY_A, attValue, providerDef);

        // gTest quirk: use NiceTest here to surpress warnings about no EXPECT_CALLS on all methods of MockProductionListener.
        NiceMock<MockProductionListener> production;
        EXPECT_CALL(production, getProductionId())
            .WillRepeatedly(Return(PRODUCTION_ID_1));

        // Act - add the production listener with a range filter to RETE to start filtering

        production.getConditions().resize(CONDITION_BLOCK_MAX_SIZE);
        ConditionBlock& conditions = production.getConditions().at(CONDITION_BLOCK_BASE_SEARCH);
        OrConditionList& orCondition = conditions.push_back();
        orCondition.push_back(ConditionInfo(
            Condition(RETE_KEY_A, 100, 200, providerDef),
            0, &provider
        ));


        // Assert - validate the filtering never matches
        EXPECT_CALL(production, onTokenAdded(_, _))
            .Times(0);

        mReteNetwork.getProductionManager().addProduction(production);

        mReteNetwork.getProductionManager().removeProduction(production);
    }


    TEST_F(ReteTest, testRangeFilter2RulesMatch)
    {
        // Arrange - add data to filter against added conditions

        // random value that we are matching against
        const WMEAttribute aValue = 123;
        const WMEAttribute bValue = 999;
        // normally, this is the game id.
        const WMEId attId = 1;

        MockReteRuleDefinition providerDefA;
        providerDefA.setRuleDefinitionCollection(*mRuleDefinitionCollection);
        MockConditionProvider providerA;

        MockReteRuleDefinition providerDefB;
        providerDefB.setRuleDefinitionCollection(*mRuleDefinitionCollection);
        MockConditionProvider providerB;

        registerRuleA(providerDefA, providerA);
        registerRuleB(providerDefB, providerB);

        //testing the same game having multiple attribute values
        mReteNetwork.getWMEManager().registerMatchAnyWME(attId);
        mReteNetwork.getWMEManager().insertWorkingMemoryElement(attId, RETE_KEY_A, aValue, providerDefA);
        mReteNetwork.getWMEManager().insertWorkingMemoryElement(attId, RETE_KEY_B, bValue, providerDefB);

        // gTest quirk: use NiceTest here to surpress warnings about no EXPECT_CALLS on all methods of MockProductionListener.
        NiceMock<MockProductionListener> production;
        EXPECT_CALL(production, getProductionId())
            .WillRepeatedly(Return(PRODUCTION_ID_1));

        // Act - add the production listener with a range filter to RETE to start filtering

        production.getConditions().resize(CONDITION_BLOCK_MAX_SIZE);
        ConditionBlock& conditions = production.getConditions().at(CONDITION_BLOCK_BASE_SEARCH);
        OrConditionList& orConditionA = conditions.push_back();
        orConditionA.push_back(ConditionInfo(
            Condition(RETE_KEY_A, 100, 200, providerDefA),
            0, &providerA
        ));
        OrConditionList& orConditionB = conditions.push_back();
        orConditionB.push_back(ConditionInfo(
            Condition(RETE_KEY_B, 900, 1000, providerDefB),
            0, &providerB
        ));


        // Assert - validate the filtering matches
        EXPECT_CALL(production, onTokenAdded(_, _))
            .Times(1)
            .WillRepeatedly(Return(1));

        mReteNetwork.getProductionManager().addProduction(production);

        mReteNetwork.getProductionManager().removeProduction(production);
    }


    TEST_F(ReteTest, testRangeFilter2RulesNoMatch)
    {
        // Arrange - add data to filter against added conditions

        // random value that we are matching against
        const WMEAttribute aValue = 123;
        const WMEAttribute bValue = 999;
        // normally, this is the game id.
        const WMEId attId = 1;

        MockReteRuleDefinition providerDefA;
        providerDefA.setRuleDefinitionCollection(*mRuleDefinitionCollection);
        MockConditionProvider providerA;

        MockReteRuleDefinition providerDefB;
        providerDefB.setRuleDefinitionCollection(*mRuleDefinitionCollection);
        MockConditionProvider providerB;

        registerRuleA(providerDefA, providerA);
        registerRuleB(providerDefB, providerB);

        //testing the same game having multiple attribute values
        mReteNetwork.getWMEManager().registerMatchAnyWME(attId);
        mReteNetwork.getWMEManager().insertWorkingMemoryElement(attId, RETE_KEY_A, aValue, providerDefA);
        mReteNetwork.getWMEManager().insertWorkingMemoryElement(attId, RETE_KEY_B, bValue, providerDefB);

        // gTest quirk: use NiceTest here to surpress warnings about no EXPECT_CALLS on all methods of MockProductionListener.
        NiceMock<MockProductionListener> production;
        EXPECT_CALL(production, getProductionId())
            .WillRepeatedly(Return(PRODUCTION_ID_1));

        // Act - add the production listener with a range filter to RETE to start filtering

        production.getConditions().resize(CONDITION_BLOCK_MAX_SIZE);
        ConditionBlock& conditions = production.getConditions().at(CONDITION_BLOCK_BASE_SEARCH);
        OrConditionList& orConditionA = conditions.push_back();
        orConditionA.push_back(ConditionInfo(
            Condition(RETE_KEY_A, 100, 200, providerDefA),
            0, &providerA
        ));
        OrConditionList& orConditionB = conditions.push_back();
        orConditionB.push_back(ConditionInfo(
            Condition(RETE_KEY_B, 900, 950, providerDefB),
            0, &providerB
        ));


        // Assert - validate the filtering matches
        EXPECT_CALL(production, onTokenAdded(_, _))
            .Times(0);

        mReteNetwork.getProductionManager().addProduction(production);

        mReteNetwork.getProductionManager().removeProduction(production);
    }


    TEST_F(ReteTest, testEqualFilterMatch)
    {
        // Arrange - add data to filter against added conditions

        // random value that we are matching against
        const WMEAttribute attValue = 123;
        // normally, this is the game id.
        const WMEId attId = 1;

        MockReteRuleDefinition providerDef;
        providerDef.setRuleDefinitionCollection(*mRuleDefinitionCollection);
        MockConditionProvider provider;
       
        registerRuleA(providerDef, provider);

        mReteNetwork.getWMEManager().registerMatchAnyWME(attId);
        mReteNetwork.getWMEManager().insertWorkingMemoryElement(attId, RETE_KEY_A, attValue, providerDef);

        // gTest quirk: use NiceTest here to surpress warnings about no EXPECT_CALLS on all methods of MockProductionListener.
        NiceMock<MockProductionListener> production;
        EXPECT_CALL(production, getProductionId())
            .WillRepeatedly(Return(PRODUCTION_ID_1));

        // Act - add the production listener with a equal filter to RETE to start filtering

        production.getConditions().resize(CONDITION_BLOCK_MAX_SIZE);
        ConditionBlock& conditions = production.getConditions().at(CONDITION_BLOCK_BASE_SEARCH);
        OrConditionList& orCondition = conditions.push_back();
        orCondition.push_back(ConditionInfo(
            Condition(RETE_KEY_A, attValue, providerDef),
            0, &provider
        ));


        // Assert - validate the filtering matches
        EXPECT_CALL(production, onTokenAdded(_, _))
            .Times(1)
            .WillRepeatedly(Return(1));

        mReteNetwork.getProductionManager().addProduction(production);

        mReteNetwork.getProductionManager().removeProduction(production);
    }


    TEST_F(ReteTest, testEqualFilterNoMatch)
    {
        // Arrange - add data to filter against added conditions

        // random value that we are matching against
        const uint64_t attValue = 123;
        // normally, this is the game id.
        const uint64_t attId = 1;

        MockReteRuleDefinition providerDef;
        providerDef.setRuleDefinitionCollection(*mRuleDefinitionCollection);
        MockConditionProvider provider;

        registerRuleA(providerDef, provider);

        mReteNetwork.getWMEManager().registerMatchAnyWME(attId);
        mReteNetwork.getWMEManager().insertWorkingMemoryElement(attId, RETE_KEY_A, attValue, providerDef);

        // gTest quirk: use NiceTest here to surpress warnings about no EXPECT_CALLS on all methods of MockProductionListener.
        NiceMock<MockProductionListener> production;
        EXPECT_CALL(production, getProductionId())
            .WillRepeatedly(Return(PRODUCTION_ID_1));

        // Act - add the production listener with a equal filter to RETE to start filtering

        production.getConditions().resize(CONDITION_BLOCK_MAX_SIZE);
        ConditionBlock& conditions = production.getConditions().at(CONDITION_BLOCK_BASE_SEARCH);
        OrConditionList& orCondition = conditions.push_back();
        orCondition.push_back(ConditionInfo(
            Condition(RETE_KEY_A, 120, providerDef),
            0, &provider
        ));


        // Assert - validate the filtering matches
        EXPECT_CALL(production, onTokenAdded(_, _))
            .Times(0);

        mReteNetwork.getProductionManager().addProduction(production);

        mReteNetwork.getProductionManager().removeProduction(production);
    }

} // namespace Matchmaker
} // namespace BlazeServerTest