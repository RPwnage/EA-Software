/*! ************************************************************************************************/
/*!
\file   ruledefinition.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_RULE_DEFINITION
#define BLAZE_RULE_DEFINITION

#include "framework/util/shared/blazestring.h" // for eastl string comparison structs
#include "EASTL/map.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/minfitthresholdlistcontainer.h"
#include "gamemanager/tdf/matchmaker.h"

#include "gamemanager/matchmaker/rules/gamereteruledefinition.h"
#include "gamemanager/rete/retetypes.h"

namespace Blaze
{
namespace Search
{
    class GameSessionSearchSlave;
}
namespace Matchmaker
{
    class MatchmakerConfig;
}

namespace GameManager
{
    class GameSession; // for use in updateMatchmakingCache
namespace Matchmaker
{
    class DiagnosticsSearchSlaveHelper;

    class MinFitThresholdList;
    class Rule;
    class RuleDefinitionCollection;

    class MatchmakingGameInfoCache;

    enum RuleDefinitionType
    {
        RULE_DEFINITION_TYPE_MULTI, // Rule can have multiple instances, separated by a configured name.
        RULE_DEFINITION_TYPE_SINGLE, // Rule has single predefined name, single instance. Loaded from config. 
        RULE_DEFINITION_TYPE_PRIORITY // Always-enabled Rule. Rule has single predefined name, single instance. May optionally have a config. Must set mName in constructor.
    };

    // Map of rule definitions' static data, by config key. Usable before RuleDefinitionCollection is created.
    //   (Currently only contains rule def type, change to use a struct if more data is added).
    typedef eastl::map<eastl::string, RuleDefinitionType> RuleDefinitionConfigSet;
    const RuleDefinitionConfigSet& getRuleDefinitionConfigSet();
    void destroyRuleDefinitionConfigSet();

    // Helper Macros required to setup rule definitions:
#define DEFINE_RULE_DEF_H(ruleDef, rule) \
    private: \
        friend struct ruleDef##Helper; \
        static EA_THREAD_LOCAL RuleDefinitionId mRuleDefinitionId; /* Not valid with Multi-type rules */ \
        static EA_THREAD_LOCAL const char* mRuleDefConfigKey; \
        static EA_THREAD_LOCAL RuleDefinitionType mRuleDefType; \
    public: \
        typedef class rule mRuleType; \
        static RuleDefinitionId getRuleDefinitionId() {return mRuleDefinitionId;} /* Not valid with Multi-type rules */ \
        static const char* getConfigKey() {return mRuleDefConfigKey;} \
        static RuleDefinitionType getRuleDefType() {return mRuleDefType; } \
        virtual const char8_t* getType() const override { return getConfigKey(); } \
        virtual const char8_t* getName() const override { return (getRuleDefType() != RULE_DEFINITION_TYPE_MULTI) ? getConfigKey() : mName; } \
        virtual void setID(RuleDefinitionId id) override \
        { \
            RuleDefinition::setID(id); \
            if (getRuleDefType() != RULE_DEFINITION_TYPE_MULTI) mRuleDefinitionId = id; \
        } \
        virtual Rule* createRule(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

    // Helper to assign the static values (in the CPP file)
#define DEFINE_RULE_DEF_CPP(ruleDef, configKey, ruleDefType) \
    EA_THREAD_LOCAL RuleDefinitionId ruleDef::mRuleDefinitionId = UINT32_MAX; /*RuleDefinition::INVALID_DEFINITION_ID - Not usable by THREAD_LOCAL*/ \
    EA_THREAD_LOCAL const char* ruleDef::mRuleDefConfigKey = configKey; \
    EA_THREAD_LOCAL RuleDefinitionType ruleDef::mRuleDefType = ruleDefType; \
    struct ruleDef##Helper { /*This struct is used to do static init (via its ctor call) of RuleDefinitionConfigSet*/ \
      ruleDef##Helper() { const_cast<RuleDefinitionConfigSet&>(getRuleDefinitionConfigSet())[ruleDef::getConfigKey()] = ruleDef::getRuleDefType(); } \
      ~ruleDef##Helper() { destroyRuleDefinitionConfigSet(); } \
    } g_##ruleDef##Helper; \
    Rule* ruleDef::createRule(MatchmakingAsyncStatus* matchmakingAsyncStatus) const \
    { \
        return BLAZE_NEW mRuleType(*this, matchmakingAsyncStatus); \
    }


    /*! ************************************************************************************************/
    /*! \brief Base class (interface) for all matchmaking & gamebrowser rule definitions.

        Implements basic initialization & logging for weights and minfitthresholdlists.

        See the Matchmaking TDD for details: http://easites.ea.com/gos/FY09%20Requirements/Blaze/TDD%20Approved/Matchmaker%20TDD.doc
    *************************************************************************************************/
    class RuleDefinition : public GameReteRuleDefinition
    {
        friend class RuleDefinitionCollection;  // Friendship for setID for quick lookup in the vector of definitions

        NON_COPYABLE(RuleDefinition);
    public:
        static const char8_t* RULE_DEFINITION_CONFIG_WEIGHT_KEY;
        static const char8_t* RULE_DEFINITION_CONFIG_SALIENCE_KEY;
        static const uint32_t INVALID_RULE_SALIENCE;
        static const RuleDefinitionId INVALID_DEFINITION_ID;

        //! \brief all rules must be initialized before use
        RuleDefinition();

        //! \brief destructor
        ~RuleDefinition() override;

        /*! ************************************************************************************************/
        /*! \brief initialize the definition with any values needed from the configuration.
                This initialize function should be implemented by the derived class and should also
                call the base class implementation to gain the benefits of parsing the weight and
                minfitthresholdlists.

            \param[in] name The Rule Definition Name. An unique string identifier for this rule. (Only used by RULE_DEFINITION_TYPE_MULTI)
            \param[in] configMap The ConfigMap containing the rule config to initialize the rule definition.

            \returns false if there were any errors parsing the configuration
        *************************************************************************************************/
        virtual bool initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig) = 0;

        // Rules that use range lists do not have min fit thresholds.
        void initialize(const char8_t* name, uint32_t salience, uint32_t weight);
        bool initialize(const char8_t* name, uint32_t salience, uint32_t weight, const FitThresholdsMap& minFitThreshold);
        

        /*! \brief return the name of this rule. For MULTI type definitions, each rule has a unique name. All other types just use the Config Key (getType)*/
        const char8_t* getName() const override = 0;

        /*! \brief return the type (Config Key) of this rule. */
        virtual const char8_t* getType() const = 0;

        /*! Returns true if this rule is of the specified rule type (Config Key). */
        bool isType(const char8_t* ruleType) const;

        /*! \brief return the salience order for this rule. */
        uint32_t getSalience() const { return mSalience; }
        
        /*! \brief Create a new non-initialized instance of the rule for this definition. */
        virtual Rule* createRule(MatchmakingAsyncStatus* matchmakingAsyncStatus) const = 0;

        /*! ************************************************************************************************/
        /*! \brief return true if this rule is disabled (ie: if client set the rule's minFitThresholdList name to "").

            Disabled rules are not evaluated (provides a slight performance boost if the title
            doesn't use a particular rule).  If MM is run in a mixed mode where some rules are disabled in
            some sessions and not in others some rules will still evaluate against those disabled sessions
            to make sure that the rule requirements are met.

            \return true if this rule is disabled
        ***************************************************************************************************/
        virtual bool isDisabled() const { return mMinFitThresholdListContainer.empty(); }

        /*! \brief Returns true if this rule type is rete enabled. */
        virtual bool isReteCapable() const = 0;

        /*! \brief Returns true if this rule will call evaluateForMMFindGame after rete. (Default enabled for non-Rete rules) */
        virtual bool callEvaluateForMMFindGameAfterRete() const {return !isReteCapable();}

        /*! \brief Returns true if this rule needs to evaluate a fit score post-rete.  Currently used for range rules as they can't put fit score in RETE tree. */
        virtual bool calculateFitScoreAfterRete() const { return false; }

        /*! ************************************************************************************************/
        /*! \brief Write the rule defn into an eastl string for logging.  NOTE: inefficient.

            The ruleDefn is written into the string using the config file's syntax.

            \param[in/out]  str - the ruleDefn is written into this string, blowing away any previous value.
            \param[in]  prefix - optional whitespace string that's prepended to each line of the ruleDefn.
            Used to 'indent' a rule for more readable logs.
        *************************************************************************************************/
        virtual void toString(eastl::string &dest, const char8_t* prefix = "") const;
 
        /*! \brief return this rule's weight. */
        RuleWeight getWeight() const { return mWeight; }
        
        // MM_TODO: When the work to use the TDF parser for the MM Config is complete
        // use the resulting TDF instead of each rule doing their own parsing.
        /*! ************************************************************************************************/
        /*! \brief write the configuration information for this rule into a MatchmakingRuleInfo TDF object
            \param[in\out] ruleConfig - rule configuration object to pack with data for this rule
        *************************************************************************************************/
        virtual void initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const;
        
        /*! ************************************************************************************************/
        /*!
            \brief return the max possible fitScore for this rule (weight * maxFitPercent).
            \return the max possible fit score for this rule.
        *************************************************************************************************/
        virtual FitScore getMaxFitScore() const { return mWeight; }

        /*! ************************************************************************************************/
        /*! \brief lookup and return one of this rule's minFitThresholdLists.  Returns nullptr if no list exists
            with name.

            \param[in] name The name of the thresholdList to get.  Case-insensitive.
            \return The MinFitThresholdList, or nullptr if the name wasn't found.
        *************************************************************************************************/
        const MinFitThresholdList* getMinFitThresholdList(const char8_t* name) const
        {
            return mMinFitThresholdListContainer.getMinFitThresholdList(name);
        }
        
        //////////////////////////////////////////////////////////////////////////
        // TODO: move insertLiteralValueType logic to minfitthresholdlistcontainer and
        //  move the call init to the base rule definition classes (maybe the ctor?).
        //  eventually remove this function or make it protected.
        //////////////////////////////////////////////////////////////////////////

        /*! ************************************************************************************************/
        /*! \brief retrieve fitscore based on passed in threshold literal
 
            \param[in] literalValue - literal value of the threshold
            \param[in] fitscore  - fitscore for the literal value
            \return true if the rule supports the literal value otherwise false
        ***************************************************************************************************/
        bool insertLiteralValueType(const char8_t* literalValue, const float fitscore)
        {
            return mMinFitThresholdListContainer.insertLiteralValueType(literalValue, fitscore);
        }

        /*! Get the ID for this rule definition */
        RuleDefinitionId getID() const override { return mID; }

        virtual GameManager::Rete::WMEAttribute getWMEAttributeName(const char8_t* attributeName) const;
        virtual GameManager::Rete::WMEAttribute getWMEAttributeValue(const char8_t* attributeValue, bool isGarbageCollectable = false) const;
        virtual GameManager::Rete::WMEAttribute getWMEUInt16AttributeValue(uint16_t numericAtributeValue) const;
        virtual GameManager::Rete::WMEAttribute getWMEInt32AttributeValue(int32_t numericAtributeValue) const;
        virtual GameManager::Rete::WMEAttribute getWMEInt64AttributeValue(int64_t numericAtributeValue) const;
        virtual GameManager::Rete::WMEAttribute getWMEBooleanAttributeValue(bool booleanAttributeValue) const;
        
        Rete::WMEAttribute getCachedWMEAttributeName(const char8_t* attributeName) const override;

        /*! ************************************************************************************************/
        /*! \brief Function that gets called when updating the matchmaking cache.  
                not all rule definitions need to update the matchmaking cache so default impl is NOOP.
                If the rule updates the cache it should implement this method.
        *************************************************************************************************/
        virtual void updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const {}

        /*! ************************************************************************************************/
        /*! \brief Function that's called when updating diagnostics' cached counts of games.
                Rule-specific. Not all rule definitions need to update this, so default impl is NOOP.
        *************************************************************************************************/
        virtual void updateDiagnosticsGameCountsCache(DiagnosticsSearchSlaveHelper& cache, const Search::GameSessionSearchSlave& gameSessionSlave, bool increment) const {}
        
        /*! ************************************************************************************************/
        /*! \brief return true if this rule definition does not create its own rules.
        ***************************************************************************************************/
        virtual bool isDisabledForRuleCreation() const { return false; }

    protected:
        /*! ************************** MM FRAMEWORK ONLY, DO NOT USE IN MM RULES ***************************/
        /*! \brief Set the ID for this rule definition; used in ruledefinitioncollection.cpp on init
        ****************************************************************************************************/
        virtual void setID(RuleDefinitionId id) { mID = id; }

        /*! ************************************************************************************************/
        /*! \brief Hook into toString, allows derived classes to write their custom rule defn info into
                an eastl string for logging.  NOTE: inefficient.

            The ruleDefn is written into the string using the config file's syntax.
            Helper subroutine to be provided by the specific implementation.

            \param[in/out]  str - the ruleDefn is written into this string, blowing away any previous value.
            \param[in]  prefix - optional whitespace string that's prepended to each line of the ruleDefn.
            Used to 'indent' a rule for more readable logs.
        *************************************************************************************************/
        virtual void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const = 0;       
 
        /*! ************************************************************************************************/
        /*! \brief Static logging helper: write the supplied salience into dest using the matchmaking config format.
     
            Ex: "<somePrefix>  weight = <someWeight>\n"
        
            \param[out] dest - a destination eastl string to print into
            \param[in] prefix - a string that's appended to the salience's key (to help with indenting)
            \param[in] salience - the salience value to print
        ***************************************************************************************************/
        void writeSalienceToString(eastl::string &dest, const char8_t* prefix, uint32_t salience) const;
 
        void cacheWMEAttributeName(const char8_t* attributeName) 
        { 
            Rete::WMEAttribute wmeName = GameManager::Rete::Condition::getWmeName(attributeName, *this); 
            mWMEAttributeNameMap[attributeName] = wmeName;
        }

     protected:
        MinFitThresholdListContainer mMinFitThresholdListContainer;
 
        int32_t mID;
        const char8_t* mName;
        RuleWeight mWeight;
        uint32_t mSalience;

        Rete::WMEAttribute mCachedAttributeBooleanFalse;
        Rete::WMEAttribute mCachedAttributeBooleanTrue;
        typedef eastl::hash_map<eastl::string, Rete::WMEAttribute, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo > WMEAttributeNameMap;
        WMEAttributeNameMap mWMEAttributeNameMap;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_RULE_DEFINITION
