/*************************************************************************************************/
/*!
\file retetest.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVER_TEST_RETE_TEST_H
#define BLAZE_SERVER_TEST_RETE_TEST_H


#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test/mock/mocklogger.h"

#include "framework/blazedefines.h"
#include "framework/metrics/metrics.h"
#include "gamemanager/rete/retenetwork.h"
#include "gamemanager/rete/productionlistener.h"
#include "gamemanager/rete/reteruledefinition.h"
#include "gamemanager/rete/productionmanager.h"
#include "gamemanager/rete/stringtable.h"


namespace BlazeServerTest
{
    extern MockLogger gMockLogger;

namespace Matchmaker
{
    using namespace Blaze::GameManager::Rete;
    using namespace Blaze::GameManager::Matchmaker;

    // Cant decide if we want to move these to the mock specific directory since they are very much RETE specific...
    class MockProductionListener : public LegacyProductionListener
    {
    public:
        MockProductionListener() {}

        MOCK_CONST_METHOD0(getProductionId, ProductionId());
        MOCK_CONST_METHOD0(getReteRules, const ReteRuleContainer&());
        MOCK_METHOD2(onTokenAdded, bool(const ProductionToken& token, ProductionInfo& info));
        MOCK_METHOD2(onTokenRemoved, void(const ProductionToken& token, ProductionInfo& info));
        MOCK_METHOD2(onTokenUpdated, void(const ProductionToken& token, ProductionInfo& info));

        MOCK_METHOD2(onProductionNodeHasTokens, void(ProductionNode& pNode, ProductionInfo& pInfo));
        MOCK_METHOD1(onProductionNodeDepletedTokens, void(ProductionNode& pNode));
        MOCK_METHOD0(initialSearchComplete, void());

        MOCK_METHOD2(pushNode, void(const ConditionInfo& conditionInfo, ProductionInfo& productionInfo));
        MOCK_METHOD2(popNode, void(const ConditionInfo& conditionInfo, ProductionInfo& productionInfo));
    };

    class MockConditionProvider : public ConditionProvider
    {
    public:
        MOCK_CONST_METHOD0(getProviderName, const char8_t*());
        MOCK_CONST_METHOD0(getProviderSalience, const uint32_t());
        MOCK_CONST_METHOD0(getProviderMaxFitScore, const FitScore());
    };


    class MockReteRuleDefinition : public ReteRuleDefinition
    {
    public:
        MOCK_CONST_METHOD0(getID, uint32_t());
        MOCK_CONST_METHOD0(getName, const char8_t*());

        MOCK_CONST_METHOD1(getCachedWMEAttributeName, WMEAttribute(const char8_t* attributeName));
    };


    class ReteTest : public testing::Test
    {

    protected:
        ReteTest();

        ~ReteTest() override;

        // gTest functions

        // Code here will be called immediately after the constructor (right
        // before each test).
        void SetUp() override;

        // Code here will be called immediately after each test (right
        // before the destructor).
        void TearDown() override {}

        // Helper functions
        void registerRuleA(MockReteRuleDefinition& providerDef, MockConditionProvider& provider) const;
        void registerRuleB(MockReteRuleDefinition& providerDef, MockConditionProvider& provider) const;

    protected:

        Blaze::Metrics::MetricsCollection mMetricsCollection;
        class RuleDefinitionCollection* mRuleDefinitionCollection = nullptr;
        ReteNetwork mReteNetwork;
        // Might make sense to look at making our configurations have virtual methods
        // so they are easy to mock....
        Blaze::GameManager::ReteNetworkConfig mReteConfig;

        StringTable mStringTable;
        
        // normally this is the rule name
        const char8_t* PROVIDER_NAME_A = "providerA";
        // normally, this is the rule id.
        const Blaze::GameManager::Matchmaker::RuleDefinitionId PROVIDER_ID_A = 1234;
        // normally,  this is the attribute name of the property we're adding
        const char8_t* RETE_KEY_A = "conditionA";

        // normally this is the rule name
        const char8_t* PROVIDER_NAME_B = "providerB";
        // normally, this is the rule id.
        const Blaze::GameManager::Matchmaker::RuleDefinitionId PROVIDER_ID_B = 2345;
        // normally,  this is the attribute name of the property we're adding
        const char8_t* RETE_KEY_B = "conditionB";

        // normally, this is the mm session id
        const ProductionId PRODUCTION_ID_1 = 1111;


    };

} // namespace Matchmaker
} // namespace BlazeServerTest

#endif
