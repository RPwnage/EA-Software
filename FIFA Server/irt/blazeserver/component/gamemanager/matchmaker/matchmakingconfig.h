/*! ************************************************************************************************/
/*!
    \file   matchmakingconfig.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_CONFIG
#define BLAZE_MATCHMAKING_CONFIG

#include "gamemanager/tdf/gamemanager.h" // for matchmaking rule size limits
#include "gamemanager/matchmaker/minfitthresholdlist.h"
#include "gamemanager/tdf/matchmaker_config_server.h"

#include "EASTL/vector.h"
#include "EASTL/map.h"

namespace Blaze
{
namespace Matchmaker { class MatchmakerConfig; }
namespace GameManager
{
namespace Matchmaker
{
    class RuleDefinitionCollection;

    /*! ************************************************************************************************/
    /*!
        \class MatchmakingConfig
        \brief Helper class to parse the matchmaking config map.  Accessors return parsed, validated
            matchmaker config settings (or default values).

        Warnings and errors are logged.  Minor errors are corrected, but fatal errors typically cause
        a matchmaking rule to be skipped.  For example, a rule name will be truncated if it's too long
        (a warning); however, if two rules have the same name, the latter is skipped (an error).
    *************************************************************************************************/
    class MatchmakingConfig
    {
    public:

        /*! ************************************************************************************************/
        /*!
            \brief Create a matchmaking config obj, allowing the config to be parsed easily.

            \note
                MatchmakerConfig is a reference that refer to the config data.
            if blaze reloads its config files).
        *************************************************************************************************/
        MatchmakingConfig();
        //! destructor
        ~MatchmakingConfig();

        // setup the mapped values needed by the generics loading, ex. GameAttributeRule.GameMapMatchRule
        static bool registerMultiInputValues(const MatchmakingServerConfig& serverConfig, const ScenarioRuleAttributeMap& ruleAttrMap);

        // Initial rule configurations no long rely on this copied version of the TDF.  We
        // should move away from copying the configuration as the tdf isSet bits are no longer correct.
        void setConfig(const MatchmakingServerConfig& config) { mMatchmakingConfigTdf = &config; }

        static const char8_t* EXACT_MATCH_REQUIRED_TOKEN; //!< "EXACT_MATCH_REQUIRED"; indicates that a
                                                          //!< minFitThreshold value requires an exact match.

        static const char8_t* MATCHING_FIT_PERCENT_KEY; //!< config map key for arbitrary generic and rankedGameRule defn's matchingFitPercent ("matchingFitPercent")
        static const char8_t* MISMATCH_FIT_PERCENT_KEY; //!< config map key for arbitrary generic and rankedGameRule defn's mismatchFitPercent ("mismatchFitPercent")


        static const char8_t* MATCHMAKER_SETTINGS_KEY;

        bool initializeRuleDefinitionCollection(RuleDefinitionCollection& ruleDefinitionCollection, const MatchmakingServerConfig& serverConfig);
        static const char8_t* CUSTOM_RULES_KEY; /* name for the custom block of configurations */
        static const char8_t* MATCHMAKER_RULES_KEY;
        static const char8_t* RULE_SALIENCE_KEY;

        uint32_t getRuleSalience(const char8_t* ruleName, uint32_t priorityRuleCount);

        const MatchmakingServerConfig& getServerConfig() const { return *mMatchmakingConfigTdf; }

        static bool isUsingLegacyRules(const Blaze::Matchmaker::MatchmakerConfig& config);

    private:

        /*! ************************************************************************************************/
        /*!
            \brief initializes the rule definition collection with the MatchmakingServerConfig portion of the config map.
            
            \param[in\out] ruleDefinitionCollection the definitions to initialize
            \param[in] serverConfig - the server config map to find rules in.
        *************************************************************************************************/
        bool initializeRuleDefinitionsFromConfig(RuleDefinitionCollection& ruleDefinitionCollection, const MatchmakingServerConfig& serverConfig);
        static const uint32_t PRIORITY_RULE_COUNT;

        void initializePriorityRules(RuleDefinitionCollection& ruleDefinitionCollection, const MatchmakingServerConfig& serverConfig);

    private:
        const char8_t* mCurrentRuleName;// we stash off the 'current' rule name to avoid passing it into each
                                        // method that logs an error
        const char8_t* mCurrentThresholdListName; // similar logging helper

        uint32_t mNextSalience;

        const MatchmakingServerConfig* mMatchmakingConfigTdf;

    };

    template <typename T>
    bool registerMultiInputValuesHelper(const MatchmakingServerConfig& serverConfig, const ScenarioRuleAttributeMap& ruleAttrMap, 
                                        const T& ruleMap, const char8_t* ruleTypeName)
    {
        // For each element, setup the config: 
        EA::TDF::TdfFactory& tdfFactory = EA::TDF::TdfFactory::get();
        ScenarioRuleAttributeMap::const_iterator tdfMemberMapping = ruleAttrMap.find(ruleTypeName);

        if (tdfMemberMapping == ruleAttrMap.end())
        {
            ERR_LOG("[GameAttributeRuleDefinition].registerMultiInputValues(): Missing rule attributes for rule type: " << ruleTypeName );
            return false;
        }

        typename T::const_iterator ruleIter = ruleMap.begin();
        typename T::const_iterator ruleEnd = ruleMap.end();
        for (; ruleIter != ruleEnd; ++ruleIter)
        {
            const char8_t* ruleName = ruleIter->first;
            eastl::string fullRuleName;
            fullRuleName.sprintf("%s.%s", ruleTypeName, ruleName);

            ScenarioRuleAttributeTdfMemberMapping::const_iterator curMember = tdfMemberMapping->second->begin();
            ScenarioRuleAttributeTdfMemberMapping::const_iterator endMember = tdfMemberMapping->second->end();
            for (; curMember != endMember; ++curMember)
            {
                eastl::string memberLocation;
                memberLocation.sprintf(curMember->second->getName(), ruleName);

                if (memberLocation == curMember->second->getName())
                {
                    TRACE_LOG("[GameAttributeRuleDefinition].registerMultiInputValues(): Type name " << memberLocation.c_str() << " not specialized for rule: " << fullRuleName.c_str() << ". Was [%s] missing?" );
                    return false;
                }

                if (tdfFactory.registerTypeName(fullRuleName.c_str(), curMember->first, memberLocation.c_str()) == false)
                {
                    TRACE_LOG("[GameAttributeRuleDefinition].registerMultiInputValues(): An error occurred when attempting to register Type name " << memberLocation.c_str() << " for rule: " << fullRuleName.c_str() << "." );
                    return false;
                }
            }

            TRACE_LOG("[GameAttributeRuleDefinition].registerMultiInputValues(): Registered type name for rule: " << fullRuleName.c_str() );
        }
        return true;
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_CONFIG
