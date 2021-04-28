/*! ************************************************************************************************/
/*!
\file gameattributeruledefinition.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gameattributeruledefinition.h"

#include "gamemanager/matchmaker/rules/gameattributerule.h"
#include "gamemanager/matchmaker/matchmakinggameinfocache.h"

#include "gamemanager/matchmaker/rules/gameattributeevaluator.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(GameAttributeRuleDefinition, "gameAttributeRules", RULE_DEFINITION_TYPE_MULTI);

    const char8_t* GameAttributeRuleDefinition::ATTRIBUTE_RULE_DEFINITION_INVALID_LITERAL_VALUE = "";
    const int32_t GameAttributeRuleDefinition::INVALID_POSSIBLE_VALUE_INDEX = -1;

    GameAttributeRuleDefinition::GameAttributeRuleDefinition()
    {
    }

    GameAttributeRuleDefinition::~GameAttributeRuleDefinition()
    {
    }

    //////////////////////////////////////////////////////////////////////////
    // GameReteRuleDefinition Functions
    //////////////////////////////////////////////////////////////////////////

    bool GameAttributeRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        // Only parse when explicit value.
        PossibleValueContainer possibleValues;
        FitTableContainer fitTable;

        GameAttributeRuleServerConfigMap::const_iterator iter = matchmakingServerConfig.getRules().getGameAttributeRules().find(name);
        if (iter == matchmakingServerConfig.getRules().getGameAttributeRules().end())
        {
            ERR("[GameAttributeRuleDefinition].initialize failed because could not find generic rule by name %s", name);
            return false;
        }
        const GameAttributeRuleServerConfig& ruleConfig = *(iter->second);

        if (!RuleDefinition::initialize(name, salience, ruleConfig.getWeight(), ruleConfig.getMinFitThresholdLists()))
        {
            ERR_LOG("[GameAttributeRuleDefinition].initialize failed to initialize the base class for rule '" << name << "'");
            return false;
        }

        if (!initAttributeName(ruleConfig.getAttributeName()))
        {
            ERR_LOG("[GameAttributeRuleDefinition].initialize failed to initialize the attribute name for rule '" << name << "'");
            return false;
        }

        mAttributeRuleType = ruleConfig.getRuleType();
        if (mAttributeRuleType == EXPLICIT_TYPE)
        {
            // votingMethod
            if (!mVotingSystem.configure(ruleConfig.getVotingMethod()))
            {
                ERR_LOG("[GameAttributeRuleDefinition].initialize failed to configure the voting system for rule '" << name << "'");
                return false;
            }

            // possible values & fitTable
            if (!parsePossibleValues(ruleConfig.getPossibleValues(), possibleValues))
            {
                ERR_LOG("[GameAttributeRuleDefinition].initialize failed to parse possible values for rule '" << name << "'");
                return false;
            }

            if (!initPossibleValueList(possibleValues))
            {
                ERR_LOG("[GameAttributeRuleDefinition].initialize failed to initialize possible values for rule '" << name << "'"); 
                return false;
            }

            // initialize the default attribute value, relies upon the possible value list already being initialized.
            Collections::AttributeValue defaultAbstainValue = ruleConfig.getDefaultAbstainValue();
            if (!initDefaultAbstainValue(defaultAbstainValue))
            {
                ERR_LOG("[GameAttributeRuleDefinition].initialize failed to initialize default abstain value for rule '" << name << "'");
                return false;
            }

            if (!mFitTable.initialize(ruleConfig.getFitTable(), ruleConfig.getSparseFitTable(), ruleConfig.getPossibleValues()))
            {
                ERR_LOG("[GameAttributeRuleDefinition].initialize failed to initialize fit table for rule '" << name << "'");
                return false;
            }
        }
        else
        {
            mMatchingFitPercent = ruleConfig.getMatchingFitPercent();
            mMismatchFitPercent = ruleConfig.getMismatchFitPercent();
        }


        return true;
    }

    /*! ************************************************************************************************/
    /*!
        \brief Helper: initialize & validate the rule's attributeName.  Returns false on error.

        \param[in]  attribName - the rule attribute's name.  Cannot be nullptr/empty; should be <=15 characters,
                            not including the null character.
        \return true on success, false otherwise
    *************************************************************************************************/
    bool GameAttributeRuleDefinition::initAttributeName(const char8_t* attribName)
    {
        // non-nullptr, non-empty
        if ( (attribName == nullptr) || (attribName[0] == '\0') )
        {
            ERR_LOG(LOG_PREFIX << "rule \"" << getName() << "\" : does not define an attribName.  (skipping invalid rule)");
            return false;
        }

        mAttributeName.set(attribName);
        cacheWMEAttributeName(mAttributeName);

        return true;
    }

    void GameAttributeRuleDefinition::initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const
    {
        GetMatchmakingConfigResponse::GenericRuleConfigList &genericRulesList = getMatchmakingConfigResponse.getGenericRules();

        GenericRuleConfig& genericRuleConfig = *genericRulesList.pull_back();
        initGenericRuleConfig(genericRuleConfig);
    }

    void GameAttributeRuleDefinition::initGenericRuleConfig( GenericRuleConfig &ruleConfig ) const
    {
        ruleConfig.setRuleName(getName());

        //store weight
        ruleConfig.setWeight(mWeight);

        //store off the fit thresholds
        mMinFitThresholdListContainer.initMinFitThresholdListInfo(ruleConfig.getThresholdNames());

        ruleConfig.setAttributeName(mAttributeName.c_str());

        //store off possible values
        size_t numPossibleValues = mPossibleValues.size();
        if (numPossibleValues > 0)
        {
            size_t lastIndex = numPossibleValues - 1;
            for (size_t i=0; i<=lastIndex; ++i)
            {
                ruleConfig.getPossibleValues().push_back(mPossibleValues[i].c_str());
            }
        } // if
    }
    bool GameAttributeRuleDefinition::initPossibleValueList(const PossibleValueContainer& possibleValues)
    {
        if (possibleValues.empty())
        {
            // error: list must define possible values
            ERR_LOG(LOG_PREFIX << "rule \"" << getName() << "\" : doesn't define a possibleValues list.");
            return false;
        }

        size_t numPossibleValues = possibleValues.size();

        // ensure that this rule's possible values can fit
        if (numPossibleValues > MAX_EXPLICIT_ATTRIBUTE_RULE_POSSIBLE_VALUE_COUNT)
        {
            // error: too many possible values (either shrink rule, or enlarge the MAX_EXPLICIT_ATTRIBUTE_RULE_POSSIBLE_VALUE_COUNT
            //   (in matchmaker tdf file)
            ERR_LOG(LOG_PREFIX << "rule \"" << getName() << "\" : possibleValues list is too long.  Either shrink the rule, or recompile after enlarging MAX_EXPLICIT_ATTRIBUTE_RULE_POSSIBLE_VALUE_COUNT (in file matchmaker.tdf).");
            return false;
        }

        mPossibleValues.reserve(numPossibleValues);

        // copy over the possible values, validating each as we go.
        //   note: since the possible values dictate the fitTable, we can't skip/ignore
        //   invalid possible values.
        for(size_t i=0; i<numPossibleValues; ++i)
        {
            if (!addPossibleValue(possibleValues[i]))
            {
                return false; // note: error already logged
            }
        }

        // init mPossibleActualValues - a copy of the mPossibleValues vector, with ABSTAIN/RANDOM removed.
        //   this is used when we're finalizing a 'random' value.
        mPossibleActualValues.reserve(numPossibleValues);
        for(size_t i=0; i<mPossibleValues.size(); ++i)
        {
            const char8_t *value = mPossibleValues[i];
            if ( !(isValueRandom(value) || isValueAbstain(value)) )
            {
                // Note: value string mem still owned by mPossibleValues, we just copy the string pointer.
                mPossibleActualValues.push_back(value);
            }
        }

        // warn if there's only 1 actual value (not including abstain/random)
        //  it's weird & suspect, but not technically wrong.
        if (mPossibleActualValues.size() == 1)
        {
            WARN_LOG(LOG_PREFIX << "rule \"" << getName() << "\" : contains only 1 possibleValue (not including RANDOM/ABSTAIN)...");
        }

        return true;
    }
    bool GameAttributeRuleDefinition::initDefaultAbstainValue(const Collections::AttributeValue& defaultAbstainValue)
    {
        bool isAbstainAPossibleValue = false;

        if (defaultAbstainValue.c_str()[0] == '\0')
        {
            ERR_LOG(LOG_PREFIX << "rule \"" << getName() << "\" : default abstain value is nullptr or empty.");
            return false;
        }

        PossibleValueContainer::const_iterator citr = mPossibleValues.begin();
        PossibleValueContainer::const_iterator citr_end = mPossibleValues.end();
        for (int ndx = 0; citr != citr_end; ++citr, ++ndx)
        {
            const Collections::AttributeValue& possibleValue = *citr;
            isAbstainAPossibleValue = (isAbstainAPossibleValue) || (blaze_stricmp(FitTable::ABSTAIN_LITERAL_CONFIG_VALUE, possibleValue.c_str()) == 0);
            if (blaze_stricmp(defaultAbstainValue, possibleValue.c_str()) == 0)
            {
                mDefaultAbstainValueIndex = ndx;
                return true;
            }
        }

        if (isAbstainAPossibleValue)
        {
            ERR_LOG(LOG_PREFIX << "rule \"" << getName() << "\" : default abstain value \"" << defaultAbstainValue.c_str() << "\" is not part of the possible values.");
            return false;
        }

        return true;
    }

    bool GameAttributeRuleDefinition::parsePossibleValues(const PossibleValuesList& possibleValuesList, PossibleValueContainer &possibleValues) const
    {
        EA_ASSERT(possibleValues.empty());

        // init the vector with the config sequence strings
        possibleValues.reserve(possibleValuesList.size());

        PossibleValuesList::const_iterator iter = possibleValuesList.begin();
        PossibleValuesList::const_iterator end = possibleValuesList.end();

        for(; iter != end; ++iter)
        {
            const char8_t* possibleValueStr = *iter;
            Collections::AttributeValue possibleValue(possibleValueStr);
            possibleValues.push_back( possibleValue );
        }

        return true;
    }





    void GameAttributeRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        GameAttributeEvaluator::insertWorkingMemoryElements(*this, wmeManager, gameSessionSlave);
    }

    void GameAttributeRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        GameAttributeEvaluator::upsertWorkingMemoryElements(*this, wmeManager, gameSessionSlave);
    }

    void GameAttributeRuleDefinition::updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const
    {
        if (matchmakingCache.isGameAttributeDirty())
        {
            if (mAttributeRuleType == EXPLICIT_TYPE)
            {
                // update this rule's cached game attrib value
                matchmakingCache.cacheGameAttributeIndex(gameSession, *this);
            }
            else if (mAttributeRuleType == ARBITRARY_TYPE)
            {
                // update this rule's cached game attrib value
                matchmakingCache.cacheArbitraryGameAttributeIndex(gameSession, *this);
            }
        }
    }


    const char8_t* GameAttributeRuleDefinition::getGameAttributeRuleType(const char8_t* ruleName, const GameAttributeRuleServerConfig& gameAttributeRule)
    {
        return GameAttributeRuleDefinition::getConfigKey();
    }

    const char8_t* GameAttributeRuleDefinition::getGameAttributeValue(const GameSession& gameSession) const
    {
        return gameSession.getGameAttrib(getAttributeName());
    }



    // return a random value from the mPossibleActualValues array
    // ie: a random value not equal to RANDOM_LITERAL or ABSTAIN_LITERAL
    const Collections::AttributeValue& GameAttributeRuleDefinition::getRandomPossibleValue() const
    {
        size_t winningIndex = Random::getRandomNumber( (int) mPossibleActualValues.size());
        return mPossibleActualValues[winningIndex];
    }

    const Collections::AttributeValue& GameAttributeRuleDefinition::getDefaultAbstainValue() const
    {
        EA_ASSERT(mDefaultAbstainValueIndex != INVALID_POSSIBLE_VALUE_INDEX);
        return mPossibleValues[mDefaultAbstainValueIndex];
    }

    int GameAttributeRuleDefinition::getPossibleValueIndex(const Collections::AttributeValue& value) const
    {
        if (value.length() == 0)
            return INVALID_POSSIBLE_VALUE_INDEX;

        int index = 0;
        PossibleValueContainer::const_iterator iter = mPossibleValues.begin();
        PossibleValueContainer::const_iterator end = mPossibleValues.end();
        for( ; iter != end; ++iter, ++index)
        {
            const Collections::AttributeValue &possibleValue = *iter;
            if (blaze_stricmp(possibleValue.c_str(), value.c_str()) == 0)
                return index;
        }

        return INVALID_POSSIBLE_VALUE_INDEX;
    }

    int GameAttributeRuleDefinition::getPossibleActualValueIndex(const Collections::AttributeValue& value) const
    {
        if (value.c_str()[0] == '\0')
            return INVALID_POSSIBLE_VALUE_INDEX;

        int index = 0;
        PossibleValueContainer::const_iterator iter = mPossibleActualValues.begin();
        PossibleValueContainer::const_iterator end = mPossibleActualValues.end();
        for( ; iter != end; ++iter, ++index)
        {
            const Collections::AttributeValue &possibleValue = *iter;
            if (blaze_stricmp(possibleValue.c_str(), value.c_str()) == 0)
                return index;
        }

        return INVALID_POSSIBLE_VALUE_INDEX;
    }

    bool GameAttributeRuleDefinition::addPossibleValue(const Collections::AttributeValue& possibleValue)
    {
        if (possibleValue.c_str()[0] == '\0')
        {
            // error: empty string as possible value
            ERR_LOG("[GameAttributeRuleDefinition].addPossibleValue '" << getName() << "' : contains an invalid (empty) possibleValue string. (skipping invalid rule)");
            return false;
        }

        // ensure that the possibleValue is unique
        if (getPossibleValueIndex(possibleValue) >= 0)
        {
            ERR_LOG("[GameAttributeRuleDefinition].addPossibleValue '" << getName() << "' : already defines '" 
                    << possibleValue.c_str() << "' as a possibleValue (they must be unique within a rule - skipping invalid rule)");
            return false;
        }

        mPossibleValues.push_back( possibleValue );
        return true;
    }

    float GameAttributeRuleDefinition::getFitPercent(size_t myValueIndex, size_t otherEntityValueIndex) const
    {
        size_t numPossibleValues = mPossibleValues.size();
        if ( (myValueIndex >= numPossibleValues) || (otherEntityValueIndex >= numPossibleValues) )
        {
            EA_ASSERT( (myValueIndex < numPossibleValues) && (otherEntityValueIndex < numPossibleValues) );
            return (float)INVALID_POSSIBLE_VALUE_INDEX;
        }
        return mFitTable.getFitPercent(myValueIndex, otherEntityValueIndex);
    }

    void GameAttributeRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        dest.append_sprintf("%s  %s = %s\n", prefix, "ruleType", AttributeRuleTypeToString(getAttributeRuleType()));
        dest.append_sprintf("%s  %s = %s\n", prefix, "attributeName", mAttributeName.c_str());
        Voting::writeVotingMethodToString(dest, prefix, mVotingSystem.getVotingMethod());

        // possible values
        size_t numPossibleValues = mPossibleValues.size();
        if (numPossibleValues > 0)
        {
            dest.append_sprintf("%s  %s = [ ", prefix, "possibleValues");
            size_t lastIndex = numPossibleValues - 1;
            for (size_t i=0; i<=lastIndex; ++i)
            {
                if (i != lastIndex)
                    dest.append_sprintf("%s, ", mPossibleValues[i].c_str());
                else
                    dest.append_sprintf("%s ]\n", mPossibleValues[i].c_str());
            }

            // fitTable
            dest.append_sprintf("%s  %s =       [ ", prefix, FitTable::GENERICRULE_FIT_TABLE_KEY);
            for (size_t row=0; row<numPossibleValues; ++row)
            {
                if (row != 0)
                {
                    // note: extra whitespace to line up with possibleValues sequence
                    dest.append_sprintf("%s                     ", prefix);
                }
                for (size_t col=0; col<numPossibleValues; ++col)
                {
                    float fitPercent = getFitPercent(row, col);
                    if (col != lastIndex)
                    {
                        dest.append_sprintf("%0.2f,  ", fitPercent);
                    }
                    else
                    {
                        if (row != lastIndex)
                            dest.append_sprintf("%0.2f,\n", fitPercent);
                        else
                            dest.append_sprintf("%0.2f ]\n", fitPercent);
                    }
                } // for
            } // for
        } // if

    }














} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
