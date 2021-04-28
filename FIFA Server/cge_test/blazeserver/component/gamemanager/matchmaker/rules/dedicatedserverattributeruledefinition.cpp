/*! ************************************************************************************************/
/*!
\file dedicatedserverattributeruledefinition.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "dedicatedserverattributeruledefinition.h"

#include "gamemanager/matchmaker/rules/dedicatedserverattributerule.h"
#include "gamemanager/matchmaker/matchmakinggameinfocache.h"

#include "gamemanager/matchmaker/rules/dedicatedserverattributeevaluator.h"

namespace Blaze
{
    namespace GameManager
    {
        namespace Matchmaker
        {
            DEFINE_RULE_DEF_CPP(DedicatedServerAttributeRuleDefinition, "dedicatedServerAttributeRules", RULE_DEFINITION_TYPE_MULTI);

            const char8_t* DedicatedServerAttributeRuleDefinition::ATTRIBUTE_RULE_DEFINITION_INVALID_LITERAL_VALUE = "";
            const int32_t DedicatedServerAttributeRuleDefinition::INVALID_POSSIBLE_VALUE_INDEX = -1;

            DedicatedServerAttributeRuleDefinition::DedicatedServerAttributeRuleDefinition()
            {
            }

            DedicatedServerAttributeRuleDefinition::~DedicatedServerAttributeRuleDefinition()
            {
            }

            //////////////////////////////////////////////////////////////////////////
            // GameReteRuleDefinition Functions
            //////////////////////////////////////////////////////////////////////////

            bool DedicatedServerAttributeRuleDefinition::initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig)
            {
                // Only parse when explicit value.
                PossibleValueContainer possibleValues;
                FitTableContainer fitTable;

                DedicatedServerAttributeRuleServerConfigMap::const_iterator iter = matchmakingServerConfig.getRules().getDedicatedServerAttributeRules().find(name);
                if (iter == matchmakingServerConfig.getRules().getDedicatedServerAttributeRules().end())
                {
                    ERR_LOG("[DedicatedServerAttributeRuleDefinition].initialize failed because could not find generic rule by name " << name);
                    return false;
                }
                const DedicatedServerAttributeRuleServerConfig& ruleConfig = *(iter->second);

                RuleDefinition::initialize(name, salience, ruleConfig.getWeight());

                if (!initAttributeName(ruleConfig.getAttributeName()))
                {
                    ERR_LOG("[DedicatedServerAttributeRuleDefinition].initialize failed to initialize the attribute name for rule '" << name << "'");
                    return false;
                }

                mDefaultValue = ruleConfig.getDefaultValue();

                mMinFitThreshold = ruleConfig.getMinFitThresholdValue();
                if ((mMinFitThreshold < 0.0f) || (mMinFitThreshold > 1.0f))
                {
                    return false;
                }

                mAttributeRuleType = ruleConfig.getRuleType();
                if (mAttributeRuleType == EXPLICIT_TYPE)
                {
                    // possible values & fitTable
                    if (!parsePossibleValues(ruleConfig.getPossibleValues(), possibleValues))
                    {
                        ERR_LOG("[DedicatedServerAttributeRuleDefinition].initialize failed to parse possible values for rule '" << name << "'");
                        return false;
                    }

                    if (!initPossibleValueList(possibleValues))
                    {
                        ERR_LOG("[DedicatedServerAttributeRuleDefinition].initialize failed to initialize possible values for rule '" << name << "'");
                        return false;
                    }

                    bool foundDefault = false;
                    for (const auto& value : mPossibleValues)
                    {
                        if (value == mDefaultValue)
                        {
                            foundDefault = true;
                            break;
                        } 
                    }
                    if (!foundDefault)
                    {
                        WARN_LOG("[DedicatedServerAttributeRuleDefinition].initialize: default value ' "<< mDefaultValue << "'does noex exist in possible values for rule '" << name << "'");
                    }

                    if (!mFitTable.initialize(ruleConfig.getFitTable(), ruleConfig.getSparseFitTable(), ruleConfig.getPossibleValues()))
                    {
                        ERR_LOG("[DedicatedServerAttributeRuleDefinition].initialize failed to initialize fit table for rule '" << name << "'");
                        return false;
                    }
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
            bool DedicatedServerAttributeRuleDefinition::initAttributeName(const char8_t* attribName)
            {
                // non-nullptr, non-empty
                if ((attribName == nullptr) || (attribName[0] == '\0'))
                {
                    ERR_LOG(LOG_PREFIX << "rule \"" << getName() << "\" : does not define an attribName.  (skipping invalid rule)");
                    return false;
                }

                mAttributeName.set(attribName);
                cacheWMEAttributeName(mAttributeName);

                return true;
            }

            void DedicatedServerAttributeRuleDefinition::initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const
            {
                GetMatchmakingConfigResponse::GenericRuleConfigList &genericRulesList = getMatchmakingConfigResponse.getGenericRules();

                GenericRuleConfig& genericRuleConfig = *genericRulesList.pull_back();
                initGenericRuleConfig(genericRuleConfig);
            }

            void DedicatedServerAttributeRuleDefinition::initGenericRuleConfig(GenericRuleConfig &ruleConfig) const
            {
                ruleConfig.setRuleName(getName());

                //store weight
                ruleConfig.setWeight(mWeight);

                ruleConfig.setAttributeName(mAttributeName.c_str());

                //store off possible values
                size_t numPossibleValues = mPossibleValues.size();
                if (numPossibleValues > 0)
                {
                    size_t lastIndex = numPossibleValues - 1;
                    for (size_t i = 0; i <= lastIndex; ++i)
                    {
                        ruleConfig.getPossibleValues().push_back(mPossibleValues[i].c_str());
                    }
                } // if
            }
            bool DedicatedServerAttributeRuleDefinition::initPossibleValueList(const PossibleValueContainer& possibleValues)
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
                for (size_t i = 0; i<numPossibleValues; ++i)
                {
                    if (!addPossibleValue(possibleValues[i]))
                    {
                        return false; // note: error already logged
                    }
                }

                return true;
            }
            

            bool DedicatedServerAttributeRuleDefinition::parsePossibleValues(const PossibleValuesList& possibleValuesList, PossibleValueContainer &possibleValues) const
            {
                EA_ASSERT(possibleValues.empty());

                // init the vector with the config sequence strings
                possibleValues.reserve(possibleValuesList.size());

                PossibleValuesList::const_iterator iter = possibleValuesList.begin();
                PossibleValuesList::const_iterator end = possibleValuesList.end();

                for (; iter != end; ++iter)
                {
                    const char8_t* possibleValueStr = *iter;
                    Collections::AttributeValue possibleValue(possibleValueStr);
                    possibleValues.push_back(possibleValue);
                }

                return true;
            }

            void DedicatedServerAttributeRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
            {
                DedicatedServerAttributeEvaluator::insertWorkingMemoryElements(*this, wmeManager, gameSessionSlave);
            }

            void DedicatedServerAttributeRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
            {
                DedicatedServerAttributeEvaluator::upsertWorkingMemoryElements(*this, wmeManager, gameSessionSlave);
            }

            void DedicatedServerAttributeRuleDefinition::updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const
            {
                if (matchmakingCache.isDedicatedServerAttributeDirty())
                {
                    if (mAttributeRuleType == EXPLICIT_TYPE)
                    {
                        // update this rule's cached game attrib value
                        matchmakingCache.cacheDedicatedServerAttributeIndex(gameSession, *this);
                    }
                }
            }


            const char8_t* DedicatedServerAttributeRuleDefinition::getDedicatedServerAttributeRuleType(const char8_t* ruleName, const DedicatedServerAttributeRuleServerConfig& dedicatedServerAttributeRule)
            {
                return DedicatedServerAttributeRuleDefinition::getConfigKey();
            }

            const char8_t* DedicatedServerAttributeRuleDefinition::getDedicatedServerAttributeValue(const GameSession& gameSession) const
            {
                return gameSession.getDedicatedServerAttrib(getAttributeName());
            }

            int DedicatedServerAttributeRuleDefinition::getPossibleValueIndex(const Collections::AttributeValue& value) const
            {
                if (value.length() == 0)
                    return INVALID_POSSIBLE_VALUE_INDEX;

                int index = 0;
                PossibleValueContainer::const_iterator iter = mPossibleValues.begin();
                PossibleValueContainer::const_iterator end = mPossibleValues.end();
                for (; iter != end; ++iter, ++index)
                {
                    const Collections::AttributeValue &possibleValue = *iter;
                    if (blaze_stricmp(possibleValue.c_str(), value.c_str()) == 0)
                        return index;
                }

                return INVALID_POSSIBLE_VALUE_INDEX;
            }


            bool DedicatedServerAttributeRuleDefinition::addPossibleValue(const Collections::AttributeValue& possibleValue)
            {
                if (possibleValue.c_str()[0] == '\0')
                {
                    // error: empty string as possible value
                    ERR_LOG("[DedicatedServerAttributeRuleDefinition].addPossibleValue '" << getName() << "' : contains an invalid (empty) possibleValue string. (skipping invalid rule)");
                    return false;
                }

                // ensure that the possibleValue is unique
                if (getPossibleValueIndex(possibleValue) >= 0)
                {
                    ERR_LOG("[DedicatedServerAttributeRuleDefinition].addPossibleValue '" << getName() << "' : already defines '"
                        << possibleValue.c_str() << "' as a possibleValue (they must be unique within a rule - skipping invalid rule)");
                    return false;
                }

                mPossibleValues.push_back(possibleValue);
                return true;
            }

            float DedicatedServerAttributeRuleDefinition::getFitPercent(size_t myValueIndex, size_t otherEntityValueIndex) const
            {
                size_t numPossibleValues = mPossibleValues.size();
                if ((myValueIndex >= numPossibleValues) || (otherEntityValueIndex >= numPossibleValues))
                {
                    EA_ASSERT((myValueIndex < numPossibleValues) && (otherEntityValueIndex < numPossibleValues));
                    return (float)INVALID_POSSIBLE_VALUE_INDEX;
                }
                return mFitTable.getFitPercent(myValueIndex, otherEntityValueIndex);
            }
            
            void DedicatedServerAttributeRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
            {
                dest.append_sprintf("%s  %s = %s\n", prefix, "ruleType", AttributeRuleTypeToString(getAttributeRuleType()));
                dest.append_sprintf("%s  %s = %s\n", prefix, "attributeName", mAttributeName.c_str());

                // possible values
                size_t numPossibleValues = mPossibleValues.size();
                if (numPossibleValues > 0)
                {
                    dest.append_sprintf("%s  %s = [ ", prefix, "possibleValues");
                    size_t lastIndex = numPossibleValues - 1;
                    for (size_t i = 0; i <= lastIndex; ++i)
                    {
                        if (i != lastIndex)
                            dest.append_sprintf("%s, ", mPossibleValues[i].c_str());
                        else
                            dest.append_sprintf("%s ]\n", mPossibleValues[i].c_str());
                    }

                    // fitTable
                    dest.append_sprintf("%s  %s =       [ ", prefix, FitTable::GENERICRULE_FIT_TABLE_KEY);
                    for (size_t row = 0; row<numPossibleValues; ++row)
                    {
                        if (row != 0)
                        {
                            // note: extra whitespace to line up with possibleValues sequence
                            dest.append_sprintf("%s                     ", prefix);
                        }
                        for (size_t col = 0; col<numPossibleValues; ++col)
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

