/*! ************************************************************************************************/
/*!
\file playerattributeruledefinition.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/playerattributeruledefinition.h"
#include "gamemanager/matchmaker/rules/playerattributerule.h"
#include "gamemanager/matchmaker/matchmakinggameinfocache.h"
#include "gamemanager/matchmaker/rules/playerattributeevaluator.h"
#include "gamemanager/playerroster.h"
#include "gamemanager/gamesessionsearchslave.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(PlayerAttributeRuleDefinition, "playerAttributeRules", RULE_DEFINITION_TYPE_MULTI);


    const char8_t* PlayerAttributeRuleDefinition::ATTRIBUTE_RULE_DEFINITION_INVALID_LITERAL_VALUE = "";
    const int32_t PlayerAttributeRuleDefinition::INVALID_POSSIBLE_VALUE_INDEX = -1;


    /*! ************************************************************************************************/
    /*! \brief ctor
    ***************************************************************************************************/
    PlayerAttributeRuleDefinition::PlayerAttributeRuleDefinition()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    PlayerAttributeRuleDefinition::~PlayerAttributeRuleDefinition()
    {
    }

    bool PlayerAttributeRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        // Only parse when explicit value.
        PossibleValueContainer possibleValues;
        FitTableContainer fitTable;

        PlayerAttributeRuleServerConfigMap::const_iterator iter = matchmakingServerConfig.getRules().getPlayerAttributeRules().find(name);
        if (iter == matchmakingServerConfig.getRules().getPlayerAttributeRules().end())
        {
            ERR("[PlayerAttributeRuleDefinition].initialize failed because could not find generic rule by name %s", name);
            return false;
        }

        const PlayerAttributeRuleServerConfig& ruleConfig = *(iter->second);
        if (!RuleDefinition::initialize(name, salience, ruleConfig.getWeight(), ruleConfig.getMinFitThresholdLists()))
        {
            ERR_LOG("[PlayerAttributeRuleDefinition].initialize failed to initialize the base class for rule '" << name << "'");
            return false;
        }

        if (!initAttributeName(ruleConfig.getAttributeName()))
        {
            ERR_LOG("[PlayerAttributeRuleDefinition].initialize failed to initialize the attribute name for rule '" << name << "'");
            return false;
        }

        mAttributeRuleType = ruleConfig.getRuleType();
        if (mAttributeRuleType == EXPLICIT_TYPE)
        {
            // possible values & fitTable
            if (!parsePossibleValues(ruleConfig.getPossibleValues(), possibleValues))
            {
                ERR_LOG("[PlayerAttributeRuleDefinition].initialize failed to parse possible values for rule '" << name << "'");
                return false;
            }

            if (!initPossibleValueList(possibleValues))
            {
                ERR_LOG("[PlayerAttributeRuleDefinition].initialize failed to initialize possible values for rule '" << name << "'");
                return false;
            }

            if (!mFitTable.initialize(ruleConfig.getFitTable(), ruleConfig.getSparseFitTable(), ruleConfig.getPossibleValues()))
            {
                ERR_LOG("[PlayerAttributeRuleDefinition].initialize failed to initialize fit table for rule '" << name << "'");
                return false;
            }
        }
        else
        {
            mMatchingFitPercent = ruleConfig.getMatchingFitPercent();
            mMismatchFitPercent = ruleConfig.getMismatchFitPercent();
        }
        mValueFormula = ruleConfig.getGroupValueFormula();
        if (!isSupportedGroupValueFormula(mValueFormula))
        {
            ERR_LOG("[PlayerAttributeRuleDefinition].initialize. Invalid GroupValueFormula (" << GroupValueFormulaToString(mValueFormula) << ") specified for rule '" << name << "'. Only AVERAGE, MIN, and MAX are supported.");
            return false;
        }

        return true;
    }

    void PlayerAttributeRuleDefinition::updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const
    {
        if (matchmakingCache.isPlayerAttributeDirty())
        {
            // update this rule's cached player attrib values
            matchmakingCache.cachePlayerAttributeValues(gameSession, *this, memberSessions);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // GameReteRuleDefinition Functions
    //////////////////////////////////////////////////////////////////////////
    void PlayerAttributeRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        PlayerAttributeEvaluator::insertWorkingMemoryElements(*this, wmeManager, gameSessionSlave);
    }

    void PlayerAttributeRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        PlayerAttributeEvaluator::upsertWorkingMemoryElements(*this, wmeManager, gameSessionSlave);
    }


    void PlayerAttributeRuleDefinition::updateAsyncStatus()
    {
        // no-op, desired async values don't change
        //  MM_TODO: in theory, we should have a way to indicate if we match ThisValue or match !ThisValue
    }


    
    /*! ************************************************************************************************/
    /*!
        \brief Helper: initialize & validate the rule's attributeName.  Returns false on error.

        \param[in]  attribName - the rule attribute's name.  Cannot be nullptr/empty; should be <=15 characters,
                            not including the null character.
        \return true on success, false otherwise
    *************************************************************************************************/
    bool PlayerAttributeRuleDefinition::initAttributeName(const char8_t* attribName)
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

    void PlayerAttributeRuleDefinition::initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const
    {
        GetMatchmakingConfigResponse::GenericRuleConfigList &genericRulesList = getMatchmakingConfigResponse.getGenericRules();

        GenericRuleConfig& genericRuleConfig = *genericRulesList.pull_back();
        initGenericRuleConfig(genericRuleConfig);
    }

    /*! ************************************************************************************************/
    /*! \brief For player attributes, we can have multiple players in the same game with different
        values for each attribute.  To represent this in the RETE tree, we 
        key off the attribute name & value combined, and use a default value of true.
        \param[in] attributeName - the name of the attribute
        \param[in] attributeValue - the value associated with the attribute
        \return a  wme attriburte representing our key for the RETE tree.
    ***************************************************************************************************/
    Rete::WMEAttribute PlayerAttributeRuleDefinition::getWMEPlayerAttributeName(const char8_t* attributeName, const char8_t* attributeValue) const
    {
        // RETE_TODO: this is not very efficient, lots of string searching.  Can gain efficiency by building this by hand.
        char8_t buf[512];
        blaze_strnzcpy(buf, getName(), sizeof(buf));
        blaze_strnzcat(buf, "_", sizeof(buf));
        blaze_strnzcat(buf, attributeName, sizeof(buf));
        blaze_strnzcat(buf, "_", sizeof(buf));
        blaze_strnzcat(buf, attributeValue, sizeof(buf));

        return GameReteRuleDefinition::getStringTable().reteHash(buf);
    }

    const char8_t* PlayerAttributeRuleDefinition::getPlayerAttributeRuleType(const char8_t* ruleName, const PlayerAttributeRuleServerConfig& playerAttributeRule)
    {
        return PlayerAttributeRuleDefinition::getConfigKey();
    }

    
    bool PlayerAttributeRuleDefinition::getPlayerAttributeValues(const Search::GameSessionSearchSlave& gameSession, PlayerAttributeRuleDefinition::AttributeValueCount& outAttrMap) const
    {
        const PlayerRoster::PlayerInfoList& playerList = gameSession.getPlayerRoster()->getPlayers(PlayerRoster::ROSTER_PARTICIPANTS);
        PlayerRoster::PlayerInfoList::const_iterator playerIter = playerList.begin();
        PlayerRoster::PlayerInfoList::const_iterator end = playerList.end();
        for ( ; playerIter != end; ++playerIter )
        {
            PlayerInfo *playerInfo = *playerIter;

            const char8_t* attribValue = playerInfo->getPlayerAttrib(getAttributeName());
            AttributeValueCount::insert_return_type returnType = outAttrMap.insert(eastl::make_pair(attribValue, 0));
            ++returnType.first->second;
        }

        return outAttrMap.empty();
    }



    bool PlayerAttributeRuleDefinition::parsePossibleValues(const PossibleValuesList& possibleValuesList, PossibleValueContainer &possibleValues) const
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

    bool PlayerAttributeRuleDefinition::initPossibleValueList(const PossibleValueContainer& possibleValues)
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


    int PlayerAttributeRuleDefinition::getPossibleValueIndex(const Collections::AttributeValue& value) const
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

    int PlayerAttributeRuleDefinition::getPossibleActualValueIndex(const Collections::AttributeValue& value) const
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

    bool PlayerAttributeRuleDefinition::addPossibleValue(const Collections::AttributeValue& possibleValue)
    {
        if (possibleValue.c_str()[0] == '\0')
        {
            // error: empty string as possible value
            ERR_LOG("[PlayerAttributeRuleDefinition].addPossibleValue '" << getName() << "' : contains an invalid (empty) possibleValue string. (skipping invalid rule)");
            return false;
        }

        // ensure that the possibleValue is unique
        if (getPossibleValueIndex(possibleValue) >= 0)
        {
            ERR_LOG("[PlayerAttributeRuleDefinition].addPossibleValue '" << getName() << "' : already defines '" 
                    << possibleValue.c_str() << "' as a possibleValue (they must be unique within a rule - skipping invalid rule)");
            return false;
        }

        mPossibleValues.push_back( possibleValue );
        return true;
    }

    float PlayerAttributeRuleDefinition::getFitPercent(uint64_t myValue, uint64_t otherEntityValue) const
    {
        if (mAttributeRuleType == EXPLICIT_TYPE)
        {
            uint64_t numPossibleValues = (uint64_t)mPossibleValues.size();
            if ( (myValue >= numPossibleValues) || (otherEntityValue >= numPossibleValues) )
            {
                EA_ASSERT( (myValue < numPossibleValues) && (otherEntityValue < numPossibleValues) );
                return FIT_PERCENT_NO_MATCH;
            }
            return mFitTable.getFitPercent(myValue, otherEntityValue);
        }
        else if (mAttributeRuleType == ARBITRARY_TYPE)
        {
            return (myValue == otherEntityValue) ? mMatchingFitPercent: mMismatchFitPercent;
        }             
        return FIT_PERCENT_NO_MATCH;
    }

    void PlayerAttributeRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
        dest.append_sprintf("%s  %s = %s\n", prefix, "ruleType", AttributeRuleTypeToString(getAttributeRuleType()));
        dest.append_sprintf("%s  %s = %s\n", prefix, "attributeName", mAttributeName.c_str());

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

    void PlayerAttributeRuleDefinition::initGenericRuleConfig( GenericRuleConfig &ruleConfig ) const
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

    const Collections::AttributeValue& PlayerAttributeRuleDefinition::getPossibleValue(size_t possibleValueIndex) const
    {
        if (possibleValueIndex >= mPossibleValues.size())
        {
            ASSERT_LOG("[PlayerAttributeRuleDefinition].getPossibleValue: internal error: possible player attribute index(" << possibleValueIndex << ") is out of bounds for cached possible values(size=" << mPossibleValues.size() << "), for rule(" << (getName() != nullptr ? getName() : "<nullptr>") << ")");
            static EA::TDF::TdfString emptyStr(ATTRIBUTE_RULE_DEFINITION_INVALID_LITERAL_VALUE);
            return emptyStr;
        }
        return mPossibleValues[possibleValueIndex];
    }

    /*! ************************************************************************************************/
    /*! \brief Function that gets called when updating matchmaking diagnostics cached values
    *************************************************************************************************/
    void PlayerAttributeRuleDefinition::updateDiagnosticsGameCountsCache(DiagnosticsSearchSlaveHelper& cache, const Search::GameSessionSearchSlave& gameSessionSlave, bool increment) const
    {
        cache.updatePlayerAttributesGameCounts(gameSessionSlave, increment, *this);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
