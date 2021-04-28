/*! ************************************************************************************************/
/*!
\file propertyrule.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/propertiesrules/propertyrule.h"
#include "gamemanager/matchmaker/rules/propertiesrules/propertyruledefinition.h"
#include "gamemanager/matchmakingfiltersutil.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    PropertyRule::PropertyRule(const RuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : Rule(ruleDefinition, matchmakingAsyncStatus), 
        mIsPackerSession(false)
    {}

    PropertyRule::PropertyRule(const PropertyRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : Rule(otherRule, matchmakingAsyncStatus),
        mIsPackerSession(otherRule.mIsPackerSession)
    {
        otherRule.mPropertyFilterMap.copyInto(mPropertyFilterMap);
        
    }

    PropertyRule::~PropertyRule()
    {}

    bool PropertyRule::addConditions(Rete::ConditionBlockList& conditionBlockList) const
    {
        bool useUnset = isUsingUnset();
        for (auto& filterDefItr : mPropertyFilterMap)
        {
            const MatchmakingFilterName& filterName = filterDefItr.first;
            const EA::TDF::GenericTdfType& curFilterGeneric = *filterDefItr.second;
            const EA::TDF::TdfPtr& filterCriteria = (const EA::TDF::TdfPtr&)&curFilterGeneric.get().asTdf();   // PACKER_TODO: Clean up this conversion code

            // Lookup/use the Filters as defined in the Config
            const auto& filterConfigDefItr = getDefinition().getFilters().find(filterName);
            if (filterConfigDefItr == getDefinition().getFilters().end())
            {
                BLAZE_ERR_LOG(getLogCategory(), "[PropertyRule].addConditions: filter(" << filterName << ") is missing from gamepacker.cfg.  Check your configs.");
                continue;
            }

            // Filters have already applied the properties (in Scenarios.cpp) before we get to this point. 
            const auto& filterDef = *filterConfigDefItr->second;

            auto& baseConditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);

            eastl::string gamePropertyName = filterDef.getGameProperty();
            gamePropertyName.make_lower();

            switch (filterCriteria->getTdfId())
            {
            case IntRangeFilter::TDF_ID:
            {
                IntRangeFilter& filter = (IntRangeFilter&)*filterCriteria;
                int64_t maxValue = filter.getMax();
                int64_t minValue = filter.getMin();
                int64_t center = filter.getCenter();
                
                // PACKER_TODO: This Filter is combines two different ways to define a range, but by reusing the same values it becomes non-intuitive.  
                //  The RangeFilter should explicitly say if it's going to use [min, max] or [center +/- (offsetMin,offsetMax)].  
                {
                    minValue = center - minValue;
                    maxValue = center + maxValue;
                }

                auto& orConditions = baseConditions.push_back();
                orConditions.push_back(Rete::ConditionInfo(Rete::Condition(gamePropertyName.c_str(), minValue, maxValue, mRuleDefinition, false, false, useUnset), 0, this));

                break;
            }

            case IntSetFilter::TDF_ID:
            {
                IntSetFilter& setFilter = (IntSetFilter&)*filterCriteria;
                bool negate = setFilter.getOperation() == EXCLUDE;
                EA_ASSERT(negate || setFilter.getOperation() == INCLUDE);  // PACKER_TODO:  These are literally the only enum values.  Why bother with the Assert?
                
                if (setFilter.getValues().empty())
                {
                    BLAZE_TRACE_LOG(getLogCategory(), "[PropertyRule].addConditions: empty values for setFilter(" << filterName << "), disabling the filter.");
                }
                else
                {
                    if (!negate)
                    {
                        baseConditions.push_back();
                    }
                    uint64_t index = 0;
                    for (auto& val : setFilter.getValues())
                    {
                        if (negate)
                        {
                            baseConditions.push_back();
                        }
                        auto& orConditions = baseConditions.back();

                        Rete::Condition condition(gamePropertyName.c_str(), val, val, mRuleDefinition, negate, false, useUnset);
                        auto conditionMapping = getDefinition().getConditionGenericMap().find(condition);
                        if (conditionMapping == getDefinition().getConditionGenericMap().end())
                        {
                            getDefinition().getConditionGenericMap()[condition].set(val);
                        }
                        // condition.setUserData((void*)index);            // Set a tracking value that will be used in PropertyRuleDefinition::convertProdNodeToSiloProperties to get the proper Value.
                        orConditions.push_back(Rete::ConditionInfo(condition, 0, this));
                        ++index;
                    }
                }
                break;
            }
            case StringSetFilter::TDF_ID:
            {
                StringSetFilter& setFilter = (StringSetFilter&)*filterCriteria;
                bool negate = setFilter.getOperation() == EXCLUDE;
                EA_ASSERT(negate || setFilter.getOperation() == INCLUDE);  // PACKER_TODO:  These are literally the only enum values.  Why bother with the Assert?

                if (setFilter.getValues().empty())
                {
                    BLAZE_TRACE_LOG(getLogCategory(), "[PropertyRule].addConditions: empty values for setFilter(" << filterName << "), disabling the filter.");
                }
                else
                {
                    if (!negate)
                    {
                        baseConditions.push_back();
                    }
                    uint64_t index = 0;
                    for (auto& val : setFilter.getValues())
                    {
                        if (negate)
                        {
                            baseConditions.push_back();
                        }
                        auto& orConditions = baseConditions.back();

                        Rete::Condition condition(gamePropertyName.c_str(), getDefinition().getWMEAttributeValue(val), mRuleDefinition, negate, false, useUnset);
                        auto conditionMapping = getDefinition().getConditionGenericMap().find(condition);
                        if (conditionMapping == getDefinition().getConditionGenericMap().end())
                        {
                            getDefinition().getConditionGenericMap()[condition].set(val);
                        }
                        //condition.setUserData((void*)index);            // Set a tracking value that will be used in PropertyRuleDefinition::convertProdNodeToSiloProperties to get the proper Value.
                        orConditions.push_back(Rete::ConditionInfo(Rete::Condition(gamePropertyName.c_str(), getDefinition().getWMEAttributeValue(val), mRuleDefinition, negate, false, useUnset), 0, this));
                        ++index;
                    }
                }
                break;
            }

            case IntEqualityFilter::TDF_ID:
            {
                IntEqualityFilter& eqFilter = (IntEqualityFilter&)*filterCriteria;

                const bool negate = eqFilter.getOperation() == UNEQUAL;
                EA_ASSERT(negate || eqFilter.getOperation() == EQUAL);
                auto& orConditions = baseConditions.push_back();
                int64_t val = eqFilter.getValue();
                orConditions.push_back(Rete::ConditionInfo(Rete::Condition(gamePropertyName.c_str(), val, val, mRuleDefinition, negate, false, useUnset), 0, this));

                // PACKER_TODO = There was some ruleDefinition.bitNameHelper(bfMember->mBitStart, propertyBitmaskItr->first.c_str(), gamePropertyBitName); logic here, that's gone now because it was insane. 
                break;
            }
            case StringEqualityFilter::TDF_ID:
            {
                StringEqualityFilter& eqFilter = (StringEqualityFilter&)*filterCriteria;

                const bool negate = eqFilter.getOperation() == UNEQUAL;
                EA_ASSERT(negate || eqFilter.getOperation() == EQUAL);
                auto& orConditions = baseConditions.push_back();
                orConditions.push_back(Rete::ConditionInfo(Rete::Condition(gamePropertyName.c_str(), getDefinition().getWMEAttributeValue(eqFilter.getValue()), mRuleDefinition, negate, false, useUnset), 0, this));
                break;
            }

            case UIntBitwiseFilter::TDF_ID:
            {
                UIntBitwiseFilter& bitwiseFilter = (UIntBitwiseFilter&)*filterCriteria;

                PropertyRuleDefinition::FilterBits maskBits, testBits;
                maskBits.from_uint64(bitwiseFilter.getBitMask());
                testBits.from_uint64(bitwiseFilter.getTestBits());

                if (!maskBits.any())
                {
                    BLAZE_TRACE_LOG(getLogCategory(), "[PropertyRule].addConditions: bitmask(0) for bitwise filter(" << filterName << "), force-set all bits");
                    maskBits.flip();
                }

                size_t oldMaskBitsCount = 0;
                auto operation = bitwiseFilter.getOperation();
                switch (operation)
                {
                case GameManager::ANY_SET_BITS_MATCH:
                    baseConditions.push_back();
                    // intentionally fallthrough
                default:
                    if (BLAZE_IS_LOGGING_ENABLED(getLogCategory(), Logging::TRACE))
                    {
                        oldMaskBitsCount = maskBits.count();
                    }
                    if (operation == GameManager::UNSET_BITS_MATCH)
                    {
                        maskBits &= ~testBits; // update maskBits to only include unset bits in the testBits
                    }
                    else
                    {
                        EA_ASSERT_MSG(operation == GameManager::SET_BITS_MATCH || operation == GameManager::ANY_SET_BITS_MATCH, "Unsupported operation!");
                        maskBits &= testBits; // update maskBits to only include set bits in the testBits
                    }
                    if (BLAZE_IS_LOGGING_ENABLED(getLogCategory(), Logging::TRACE))
                    {
                        auto newMaskBitCount = maskBits.count();
                        if (newMaskBitCount < oldMaskBitsCount)
                        {
                            BLAZE_TRACE_LOG(getLogCategory(), "[PropertyRule].addConditions: bitwise filter(" << filterName << ") op(" << BitTestOperationToString(operation) << ") removed(" << oldMaskBitsCount - maskBits.count() << ") bits from mask, remaining(" << newMaskBitCount << ")");
                        }
                    }
                    break;
                }

                const auto WME_ATTR_BOOLEAN_TRUE = getDefinition().getWMEBooleanAttributeValue(true);
                const auto WME_ATTR_BOOLEAN_FALSE = getDefinition().getWMEBooleanAttributeValue(false);
                eastl::string gamePropertyBitName;
                // scan the bits covered by the bitmask and insert condition for each one
                for (auto bit = maskBits.find_first(); bit < maskBits.kSize; bit = maskBits.find_next(bit))
                {
                    auto boolWmeValue = testBits.test(bit) ? WME_ATTR_BOOLEAN_TRUE : WME_ATTR_BOOLEAN_FALSE;
                    getDefinition().bitNameHelper(bit, gamePropertyName, gamePropertyBitName);
                    baseConditions.push_back().push_back(Rete::ConditionInfo(Rete::Condition(gamePropertyBitName.c_str(), boolWmeValue, mRuleDefinition, false, false, useUnset), 0, this));
                }
                break;
            }

            case PlatformFilter::TDF_ID:
            {
                PlatformFilter& platformFilter = (PlatformFilter&)*filterCriteria;
                
                // Iterate over all hosted Platforms and add the ALLOWED/not platforms.
                for (auto& curPlatform : gController->getHostedPlatforms())
                {
                    bool platformAllowed = (platformFilter.getClientPlatformListOverride().findAsSet(curPlatform) != platformFilter.getClientPlatformListOverride().end());
                    bool platformRequired = (platformFilter.getRequiredPlatformList().findAsSet(curPlatform) != platformFilter.getRequiredPlatformList().end());

                    // If the Allowed Platforms list is empty, all values are required:
                    if (!platformFilter.getClientPlatformListOverride().empty())
                    {
                        // Example:  [allow]/[require] sessions
                        // [pc, steam, xbox]/[pc, steam, xbox] session conditions pull in [pc, steam, xbox]/[pc] or [pc, steam, xbox]/[pc, xbox] sessions, not [pc, ps4]/[pc] or [pc, steam, xbox].
                        // [pc, steam, xbox]/[pc] session conditions pull in [pc, steam, xbox, ps4]/[pc] or [pc, steam, xbox]/[pc, xbox] sessions, not [pc, ps4]/[pc] or [pc, steam, xbox, ps4]/[pc, ps4].
                        // [pc]/[pc] session conditions pull in [pc, steam, xbox, ps4]/[pc] not [*]/[pc, anything].
                        //
                        // The more restrictive Sessions (ex. mustmatch pc) pull in the less restrictive Session (ex. pc full XP).
                        // The silo that gets created is == to the allow list  (so [pc, steam, xbox]/[pc] generates a [pc, steam, xbox] silo, that can create games with only those platforms....  When is it the intersection?, or fewer?)

                        // Packer Session search Conditions: 
                        if (platformAllowed)
                        {
                            auto& orConditions = baseConditions.push_back();
                            orConditions.push_back(Rete::ConditionInfo(Rete::Condition(getDefinition().getAllowedPlatformString(curPlatform), getDefinition().getWMEBooleanAttributeValue(true), mRuleDefinition, false, false, useUnset), 0, this));
                        }
                        else
                        {
                            auto& orConditions = baseConditions.push_back();
                            orConditions.push_back(Rete::ConditionInfo(Rete::Condition(getDefinition().getRequiredPlatformString(curPlatform), getDefinition().getWMEBooleanAttributeValue(false), mRuleDefinition, false, false, useUnset), 0, this));
                        }
                    }
                    else
                    {
                        // Packer Silo search Conditions:   (We still check against 'AllowedPlatformString', since that's what the Active Games set.)
                        auto& orConditions = baseConditions.push_back();
                        orConditions.push_back(Rete::ConditionInfo(Rete::Condition(getDefinition().getAllowedPlatformString(curPlatform), getDefinition().getWMEBooleanAttributeValue(platformRequired), mRuleDefinition, false, false, useUnset), 0, this));
                    }
                }
                break;
            }
            default:
                BLAZE_ASSERT_LOG(getLogCategory(), "[PropertyRule].addConditions: Invalid filter requirement for filter (" << filterName << ")");
                break;
            }
        }

        return true;
    }

    bool PropertyRule::initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err)
    {
        return initializeInternal(matchmakingSupplementalData.mFilterMap, err);
    }

    bool PropertyRule::initializeInternal(const MatchmakingFilterCriteriaMap& filterCriteriaMap, MatchmakingCriteriaError& err, bool packerSession)
    {
        mIsPackerSession = packerSession;

        // Copy the Filters to a local store:
        filterCriteriaMap.copyInto(mPropertyFilterMap);
        for (auto& curFilter : mPropertyFilterMap)
        {
            auto& curFilterName = curFilter.first;
            auto& curFilterGeneric = curFilter.second;

            // Lookup/use the Filters as defined in the Config
            const auto& filterConfigDefItr = getDefinition().getFilters().find(curFilterName);
            if (filterConfigDefItr == getDefinition().getFilters().end())
            {
                StringBuilder errorString;
                errorString << "Filter(" << curFilterName << ") is missing from gamepacker.cfg.  Check your configs.";
                err.setErrMessage(errorString.c_str());
                BLAZE_ERR_LOG(getLogCategory(), "[PropertyRule].initializeInternal: " << err.getErrMessage());
                return false;
            }

            // This should already have been caught:
            if (!curFilterGeneric->isValid() || curFilterGeneric->get().getType() != EA::TDF::TDF_ACTUAL_TYPE_TDF)
            {
                StringBuilder errorString;
                errorString << "Filter(" << curFilterName << ") was unable to parse the Requirement data correctly.  Check your configs, and values sent from the client.";
                err.setErrMessage(errorString.c_str());
                BLAZE_ERR_LOG(getLogCategory(), "[PropertyRule].initializeInternal: " << err.getErrMessage());
                return false;
            }
            
            const EA::TDF::TdfPtr& filterCriteria = (const EA::TDF::TdfPtr)&curFilterGeneric->get().asTdf();   // PACKER_TODO: Clean up this conversion code
            BLAZE_TRACE_LOG(getLogCategory(), "[PropertyRule].initialize: use filter(" << curFilterName << "), requirement(" << SbFormats::TdfObject(*filterCriteria, true) << ")");
            
            // Validate the criteria (Previously in resolvePropertyValueReferencesInFilterCriteria):
            switch (filterCriteria->getTdfId())
            {
            case IntRangeFilter::TDF_ID:
            {
                IntRangeFilter& filter = (IntRangeFilter&)*filterCriteria;
                int64_t maxValue = filter.getMax();
                int64_t minValue = filter.getMin();
                int64_t center = filter.getCenter();

                // PACKER_TODO: This Filter is combines two different ways to define a range, but by reusing the same values it becomes non-intuitive.  
                //  The RangeFilter should explicitly say if it's going to use [min, max] or [center +/- (offsetMin,offsetMax)].  
                {
                    minValue = center - minValue;
                    maxValue = center + maxValue;
                }

                if (minValue > maxValue)
                {
                    StringBuilder errorString;
                    errorString << "Range Filter(" << curFilterName << ") min(" << minValue << ") > max(" << maxValue << "). Invalid values. ";
                    err.setErrMessage(errorString.c_str());
                    BLAZE_ERR_LOG(getLogCategory(), "[PropertyRule].initializeInternal: " << err.getErrMessage());
                    return false;
                }
                break;
            }

            default:
                break;
            }

            // Packer Session logic (previously in gamemanagerslaveimpl.cpp)
            if (mIsPackerSession)
            {
                switch (filterCriteria->getTdfId())
                {
                case IntRangeFilter::TDF_ID:
                {
                    {
                        StringBuilder errMsg;
                        errMsg << "Filter (" << curFilterName << ") - IntRangeFilter unsupported with GamePacker.";
                        err.setErrMessage(errMsg.c_str());
                        BLAZE_ERR_LOG(getLogCategory(), "[PropertyRule].initializeInternal: " << err.getErrMessage());
                        return false;
                    }
                    break;
                }
                case IntSetFilter::TDF_ID:
                {
                    IntSetFilter& filter = (IntSetFilter&)*filterCriteria;
                    if (filter.getOperation() == EXCLUDE)
                    {
                        StringBuilder errMsg;
                        errMsg << "Filter(" << curFilterName << ") - IntSetFilter unsupported configuration (operation == EXCLUDE) with GamePacker.";
                        BLAZE_ERR_LOG(getLogCategory(), "[PropertyRule].initializeInternal: " << err.getErrMessage());
                        return false;
                    }
                    break;
                }
                case StringSetFilter::TDF_ID:
                {
                    StringSetFilter& filter = (StringSetFilter&)*filterCriteria;
                    if (filter.getOperation() == EXCLUDE)
                    {
                        StringBuilder errMsg;
                        errMsg << "Filter(" << curFilterName << ") - StringSetFilter unsupported configuration (operation == EXCLUDE) with GamePacker.";
                        BLAZE_ERR_LOG(getLogCategory(), "[PropertyRule].initializeInternal: " << err.getErrMessage());
                        return false;
                    }
                    break;
                }

                case IntEqualityFilter::TDF_ID:
                {
                    IntEqualityFilter& filter = (IntEqualityFilter&)*filterCriteria;
                    if (filter.getOperation() == UNEQUAL)
                    {
                        StringBuilder errMsg;
                        errMsg << "Filter(" << curFilterName << ") - IntEqualityFilter unsupported configuration (operation == UNEQUAL) with GamePacker.";
                        BLAZE_ERR_LOG(getLogCategory(), "[PropertyRule].initializeInternal: " << err.getErrMessage());
                        return false;
                    }
                    break;
                }
                case StringEqualityFilter::TDF_ID:
                {
                    StringEqualityFilter& filter = (StringEqualityFilter&)*filterCriteria;
                    if (filter.getOperation() == UNEQUAL)
                    {
                        StringBuilder errMsg;
                        errMsg << "Filter(" << curFilterName << ") - StringEqualityFilter unsupported configuration (operation == UNEQUAL) with GamePacker.";
                        BLAZE_ERR_LOG(getLogCategory(), "[PropertyRule].initializeInternal: " << err.getErrMessage());
                        return false;
                    }
                    break;
                }

                default:
                    break;
                }
            }
        }




        return true;
    }

    FitScore PropertyRule::evaluateForMMFindGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const
    {
        // NOTE: This rule is currently RETE only
        return 0;
    }

    /*! ************************************************************************************************/
    /*! \brief Clone the current rule copying the rule criteria using the copy ctor.  Used for subsessions.
    ****************************************************************************************************/
    Rule* PropertyRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW PropertyRule(*this, matchmakingAsyncStatus);
    }

    int32_t PropertyRule::getLogCategory() const
    {
        return mRuleDefinition.getRuleDefinitionCollection()->getLogCategory();
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
