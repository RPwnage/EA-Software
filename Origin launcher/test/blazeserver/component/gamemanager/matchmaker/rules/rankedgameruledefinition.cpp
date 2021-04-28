/*! ************************************************************************************************/
/*!
    \file   rankedgameruledefinition.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/random.h"

#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rules/rankedgameruledefinition.h"
#include "gamemanager/matchmaker/rules/rankedgamerule.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(RankedGameRuleDefinition, "Predefined_RankedGameRule", RULE_DEFINITION_TYPE_SINGLE);
    const char8_t* RankedGameRuleDefinition::RANKEDRULE_RETE_KEY = "rankedGame";

    /*! ************************************************************************************************/
    /*!
        \brief Construct an uninitialized RankedGameRuleDefinition.  NOTE: do not use until initialized and
        at least 1 minFitThresholdList has been added (see addMinFitThresholdList and initialize).
    *************************************************************************************************/
    RankedGameRuleDefinition::RankedGameRuleDefinition()
        : mMatchingFitPercent(1.0f),
          mMismatchFitPercent(0.5f)
    {
        mVotingSystem.setVotingMethod(OWNER_WINS);
        // side (sprint3.03), no need to explicitly set mMinFitThresholdListContainer.setThresholdAllowedValueType(ALLOW_NUMERIC|ALLOW_EXACT_MATCH);
        // as these auto initialized by default (see ThresholdValueInfo constructor)
    }
    
    //////////////////////////////////////////////////////////////////////////
    // from GameReteRuleDefinition grandparent
    //////////////////////////////////////////////////////////////////////////

    /*! ************************************************************************************************/
    /*! \brief insertWorkingMemoryElements insert/update working memory elements in the WME Manager.
        \param[in] gameSessionSlave - used for WME name and value for this rule
    *************************************************************************************************/
    void RankedGameRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        // Find the setting for this rule to insert
        const RankedGameRulePrefs::RankedGameDesiredValue ranked = gameSessionSlave.getGameSettings().getRanked()?
            RankedGameRulePrefs::RANKED : RankedGameRulePrefs::UNRANKED;
        
        // insert
        const char8_t* value = RankedGameRulePrefs::RankedGameDesiredValueToString(ranked);
        wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(),
            RANKEDRULE_RETE_KEY, RuleDefinition::getWMEAttributeValue(value), *this);
    }

    /*! ************************************************************************************************/
    /*! \brief upsertWorkingMemoryElements is called from upsertWorkingMemoryElements if
        shouldUpdateWorkingMemoryElements returns true.  This method should insert/update any
        WME's from the DataProvider that have changed.

        \param[in/out] wmeManager - The WME manager used to insert/update the WMEs.
        \param[in/out] dataProvider - The Data provider used to create the WMEs.
        \param[in/out] hint - A hint used to determine what has changed.
        \return 
    *************************************************************************************************/
    void RankedGameRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        // Find the setting for this rule to insert
        const RankedGameRulePrefs::RankedGameDesiredValue ranked = gameSessionSlave.getGameSettings().getRanked()?
            RankedGameRulePrefs::RANKED : RankedGameRulePrefs::UNRANKED;

        // upsert
        const char8_t* value = RankedGameRulePrefs::RankedGameDesiredValueToString(ranked);
        wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(),
            RANKEDRULE_RETE_KEY, RuleDefinition::getWMEAttributeValue(value), *this);
    }

    //////////////////////////////////////////////////////////////////////////
    // from RuleDefinition
    //////////////////////////////////////////////////////////////////////////

    /*! ************************************************************************************************/
    /*!
        \brief Initialize a RankedGameRuleDefinition obj; returns false if a supplied value failed validation
            (and the rule's value doesn't match what was supplied).

        Note: the rankedGameRule definition clamps the range of the two fitPercents to be between 0 and 1.0
        (inclusive).

        \param[in] configMap - The section of the entire configuration map that pertains to our rule
        \return true on success, false otherwise
    *************************************************************************************************/
    bool RankedGameRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        const RankedGameRuleConfig& rankedGameRuleConfig = matchmakingServerConfig.getRules().getPredefined_RankedGameRule();

        if(!rankedGameRuleConfig.isSet())
        {
            WARN_LOG("[RankedGameRuleDefinition] Rule " << getConfigKey() << " disabled, not present in configuration.");
            return false;
        }

        // parses common weight, salience, minFitThresholdList (blaze3.03)
        if (!RuleDefinition::initialize(name, salience, rankedGameRuleConfig.getWeight(), rankedGameRuleConfig.getMinFitThresholdLists()))
        {
            return false;
        }

        // parse voting method
        if (!mVotingSystem.configure(rankedGameRuleConfig.getVotingMethod()))
        {
            ERR_LOG("[RankedGameRuleDefinition].initialize failed to configure the voting system for rule '" << name << "'");
            return false;
        }

        // verify fitPercent ranges (each is clamped between 1.0 and 0)
        bool valueClamped = false;
        mMatchingFitPercent = clampFitPercent(rankedGameRuleConfig.getMatchingFitPercent(), valueClamped);
        mMismatchFitPercent = clampFitPercent(rankedGameRuleConfig.getMismatchFitPercent(), valueClamped);
        return (!valueClamped);
    }


    /*! ************************************************************************************************/
    /*!
        \brief Write the rule defn into an eastl string for logging.  NOTE: inefficient.

        The ruleDefn is written into the string using the config file's syntax.

        \param[in/out]  str - the ruleDefn is written into this string, blowing away any previous value.
        \param[in]  prefix - optional whitespace string that's prepended to each line of the ruleDefn.
                            Used to 'indent' a rule for more readable logs.
    *************************************************************************************************/
    void RankedGameRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        Voting::writeVotingMethodToString(dest, prefix, mVotingSystem.getVotingMethod());

        dest.append_sprintf("%s  %s = %1.2f\n", prefix, MatchmakingConfig::MATCHING_FIT_PERCENT_KEY, mMatchingFitPercent);
        dest.append_sprintf("%s  %s = %1.2f\n", prefix, MatchmakingConfig::MISMATCH_FIT_PERCENT_KEY, mMismatchFitPercent);
    }

    //////////////////////////////////////////////////////////////////////////
    // MM rule specific functions
    //////////////////////////////////////////////////////////////////////////

    /*! ************************************************************************************************/
    /*!
        \brief return the supplied fitPercent value, clamped to the range (0..1).  If the value was clamped,
        'valueClamped' is set to true.
    
        \param[in] fitPercent an unclamped fitPercent value
        \param[out] valueClamped set to true if the fitPercent was clamped (otherwise not altered)
        \return the supplied fitPercent, clamped between 0 and 1.0.
    ***************************************************************************************************/
    float RankedGameRuleDefinition::clampFitPercent(float fitPercent, bool &valueClamped)
    {
        // check upper bound (1.0)
        if (fitPercent > 1.0f)
        {
            valueClamped = true;
            return 1.0f;
        }

        // check lower bound (0)
        if (fitPercent < 0)
        {
            valueClamped = true;
            return 0;
        }
        
        return fitPercent; // value not clamped
    }


} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
