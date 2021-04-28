/*! ************************************************************************************************/
/*!
    \file   ruledefinition.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/matchmaker/minfitthresholdlist.h"
#include "gamemanager/matchmaker/matchmakingconfig.h" // for config key names (for logging)

#include "gamemanager/gamesessionsearchslave.h" // for GameSession  in updateMatchmakingCache()

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    static RuleDefinitionConfigSet* gRuleDefinitionConfigSet = nullptr;
    const RuleDefinitionConfigSet& getRuleDefinitionConfigSet()
    {
        if (!gRuleDefinitionConfigSet)
            gRuleDefinitionConfigSet = BLAZE_NEW RuleDefinitionConfigSet;
        return *gRuleDefinitionConfigSet;
    }

    void destroyRuleDefinitionConfigSet()
    {
        delete gRuleDefinitionConfigSet;
        gRuleDefinitionConfigSet = nullptr;
    }

    const char8_t* RuleDefinition::RULE_DEFINITION_CONFIG_WEIGHT_KEY =  "weight";
    const char8_t* RuleDefinition::RULE_DEFINITION_CONFIG_SALIENCE_KEY = "salience";

    const uint32_t RuleDefinition::INVALID_RULE_SALIENCE = UINT32_MAX;
    const RuleDefinitionId RuleDefinition::INVALID_DEFINITION_ID = UINT32_MAX;

    RuleDefinition::RuleDefinition()
        : mName(nullptr), mWeight(0), mSalience(INVALID_RULE_SALIENCE)
    {
        mMinFitThresholdListContainer.setRuleDefinition(this);
    }

    RuleDefinition::~RuleDefinition()
    {
        if (mName != nullptr) BLAZE_FREE((char8_t*) mName);
    }

    bool RuleDefinition::isType(const char8_t* ruleType) const
    {
        if ((ruleType == nullptr) || (ruleType[0] == '\0'))
        {
            return false;
        }

        return blaze_stricmp(ruleType, getType()) == 0;
    }

    /*! ************************************************************************************************/
    /*! \brief initialize base rule definition values.  Every rule is rquired to specify these things.
    
        \param[in] name of the rule
        \param[in] salience of the rule
        \param[in] weight of the rule
    ***************************************************************************************************/
    void RuleDefinition::initialize(const char8_t* name, uint32_t salience, uint32_t weight)
    {
        if(mName == nullptr)
        {
            // priority rules have already had their names set, they aren't read from a config file
            // this prevents those rules from having their names re-allocated
            mName = blaze_strdup(name);
        }

        mWeight = weight;

        // Default salience is the order of config parsing in the matchmaker.cfg
        mSalience = salience;

        TRACE_LOG("[RuleDefinition].initialize rule '" << name << "' has salience of '" << mSalience << "'.");

        if (salience == INVALID_RULE_SALIENCE)
        {
            // log warning that salience was missing, initialize should always pass in a valid salience
            WARN_LOG("[RuleDefinition].initialize rule '" << name << "' did not have an assigned sailence.");
        }

        // Cache off values for true and false to prevent from having to regenerate hash every access.
        if (GameReteRuleDefinition::isInitialized())
        {
            mCachedAttributeBooleanTrue = GameReteRuleDefinition::getStringTable().reteHash("true");
            mCachedAttributeBooleanFalse = GameReteRuleDefinition::getStringTable().reteHash("false");
        }
    }

    /*! ************************************************************************************************/
    /*! \brief initialize a rule with a min fit threshold decay.
    
        \param[in] name of the rule
        \param[in] salience of the rule
        \param[in] weight of the rule
        \param[in] minFitThreshold configuration map of the rule
        \return
    ***************************************************************************************************/
    bool RuleDefinition::initialize( const char8_t* name, uint32_t salience, uint32_t weight, const FitThresholdsMap& minFitThreshold )
    {
        initialize(name, salience, weight);

        // Call helper to parse the min fit threshold list for us
        return mMinFitThresholdListContainer.parseMinFitThresholdList(minFitThreshold);
    }

    void RuleDefinition::initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const
    {
        GetMatchmakingConfigResponse::PredefinedRuleConfigList& predefinedRulesList = getMatchmakingConfigResponse.getPredefinedRules();
        PredefinedRuleConfig& predefinedRuleConfig = *predefinedRulesList.pull_back();

        predefinedRuleConfig.setRuleName(getName());
        predefinedRuleConfig.setWeight(mWeight);

        mMinFitThresholdListContainer.initMinFitThresholdListInfo(predefinedRuleConfig.getThresholdNames());
    }

    void RuleDefinition::toString(eastl::string &dest, const char8_t* prefix) const
    {
        dest.append_sprintf("%s%s = {\n", prefix, getName());

        if(isDisabled())
        {
            dest.append_sprintf("######WARNING###### Rule not available or disabled\n");
        }
        else
        {
            MatchmakingUtil::writeWeightToString(dest, prefix, mWeight); // Log the weight
            writeSalienceToString(dest, prefix, mSalience);
            logConfigValues(dest, prefix); // Custom Logging by the derived class
            mMinFitThresholdListContainer.writeMinFitThresholdListsToString(dest, prefix); // Log the Threshold
        }

        dest.append_sprintf("%s}\n", prefix);
    }

    void RuleDefinition::writeSalienceToString(eastl::string &dest, const char8_t* prefix, uint32_t salience) const
    {
        dest.append_sprintf("%s  %s = %u\n", prefix, RULE_DEFINITION_CONFIG_SALIENCE_KEY, salience);
    }

    Rete::WMEAttribute RuleDefinition::getWMEAttributeName(const char8_t* attributeName) const
    {
        // RETE_TODO: this is not very efficient, lots of string searching.  Can gain efficiency by building this by hand.
        // Max attributeName length is 33 chars.
        char8_t buf[128];
        blaze_strnzcpy(buf, getName(), sizeof(buf));
        blaze_strnzcat(buf, "_", sizeof(buf));
        blaze_strnzcat(buf, attributeName, sizeof(buf));

        return GameReteRuleDefinition::getStringTable().reteHash(buf);
    }

    Blaze::GameManager::Rete::WMEAttribute RuleDefinition::getWMEAttributeValue(const char8_t* attributeValue, bool isGarbageCollectable /*= false*/) const
    {
        return GameReteRuleDefinition::getStringTable().reteHash(attributeValue, isGarbageCollectable);
    }

    Rete::WMEAttribute RuleDefinition::getWMEBooleanAttributeValue(bool booleanAttributeValue) const
    {
        return booleanAttributeValue ? mCachedAttributeBooleanTrue : mCachedAttributeBooleanFalse;
    }

    Rete::WMEAttribute RuleDefinition::getWMEUInt16AttributeValue(uint16_t numericAtributeValue) const
    {
        char8_t buf[8];
        blaze_snzprintf(buf, sizeof(buf), "%u", numericAtributeValue);
        // TODO: potentially don't hash numeric values (values only) and just let the rule assume they are unique...

        return GameReteRuleDefinition::getStringTable().reteHash(buf);
    }

    Rete::WMEAttribute RuleDefinition::getWMEInt32AttributeValue(int32_t numericAtributeValue) const
    {
        char8_t buf[8];
        blaze_snzprintf(buf, sizeof(buf), "%d", numericAtributeValue);
        // TODO: potentially don't hash numeric values (values only) and just let the rule assume they are unique...

        return GameReteRuleDefinition::getStringTable().reteHash(buf);
    }

    Rete::WMEAttribute RuleDefinition::getWMEInt64AttributeValue(int64_t numericAtributeValue) const
    {
        char8_t buf[32];
        blaze_snzprintf(buf, sizeof(buf), "%" PRId64, numericAtributeValue);
        // TODO: potentially don't hash numeric values (values only) and just let the rule assume they are unique...

        return GameReteRuleDefinition::getStringTable().reteHash(buf);
    }

    Rete::WMEAttribute RuleDefinition::getCachedWMEAttributeName(const char8_t* attributeName) const
    {
        WMEAttributeNameMap::const_iterator iter = mWMEAttributeNameMap.find(attributeName);
        if (iter != mWMEAttributeNameMap.end())
        {
            return iter->second;
        }

        return Rete::WME_ANY_HASH;
    }
} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
