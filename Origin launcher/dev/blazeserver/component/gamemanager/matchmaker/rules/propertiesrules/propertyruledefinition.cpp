/*! ************************************************************************************************/
/*!
\file propertyruledefinition.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/propertiesrules/propertyruledefinition.h"

// so we can lookup preconfig
#include "gamemanager/searchcomponent/searchslaveimpl.h"
#include "gamemanager/tdf/search_config_server.h"
#include "gamemanager/matchmakercomponent/matchmakerslaveimpl.h"
#include "gamemanager/tdf/matchmaker_component_config_server.h"
#include "gamemanager/tdf/matchmaking_properties_config_server.h"
#include "gamemanager/gamesessionsearchslave.h"

#include "gamemanager/matchmaker/rules/propertiesrules/propertyrule.h"
#include "gamemanager/matchmaker/matchmakinggameinfocache.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(PropertyRuleDefinition, "propertyRule", RULE_DEFINITION_TYPE_PRIORITY);

    PropertyRuleDefinition::PropertyRuleDefinition()
    {
    }

    PropertyRuleDefinition::~PropertyRuleDefinition()
    {
    }

    bool PropertyRuleDefinition::initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig)
    {
        // WARNING - Do not add additional functionality to this command, it is not called on the PackerSlave.  (Add functionality to the next initialize function.)

        Blaze::Search::SearchSlaveImpl* searchSlave = static_cast<Blaze::Search::SearchSlaveImpl*>(gController->getComponent(Blaze::Search::SearchSlave::COMPONENT_ID, true, false));
        // PACKER_TODO: Note, we are ignoring the salience right now, but it definitely needs to be taken into account, that said property rule is special because it needs to determine its salience based on the property it is filtering on.
        // Likely the solution may be to lookup the property name from the filterSalience list.
        if (searchSlave != nullptr)
        {
            return initialize(searchSlave->getConfig().getFilters(), searchSlave->getConfig().getScenariosConfig().getGlobalFilters(), searchSlave->getConfig().getProperties());
        }
        else
        {
            Blaze::Matchmaker::MatchmakerSlaveImpl* mmSlave = static_cast<Blaze::Matchmaker::MatchmakerSlaveImpl*>(gController->getComponent(Blaze::Matchmaker::MatchmakerSlave::COMPONENT_ID, true, false));
            if (mmSlave != nullptr)
            {
                return initialize(mmSlave->getConfig().getFilters(), mmSlave->getConfig().getScenariosConfig().getGlobalFilters(), mmSlave->getConfig().getProperties());
            }
            else
            {
                EA_FAIL_MSG("This initializer only handles Matchmaker and Search slaves"); // PACKER_TODO: clean this up
            }
        }
        return false;
    }

    bool PropertyRuleDefinition::initialize(const MatchmakingFilterMap& filters, const MatchmakingFilterList& globalFilters, const PropertyConfig& propertyConfig)
    {
        for (auto& curPlatform : gController->getHostedPlatforms())
        {
            mAllowedStringMap[curPlatform].sprintf("ALLOWED_%s", ClientPlatformTypeToString(curPlatform));
            mRequiredStringMap[curPlatform].sprintf("REQUIRED_%s", ClientPlatformTypeToString(curPlatform));
        }

        mGamePropertyNameList.clear();
        filters.copyInto(mPropertyFiltersConfig);

        mPropertyManager.onConfigure(propertyConfig);


        uint32_t dummySalience = 0; // PACKER_TODO: salience needs to be specified independently for each property rule definition, we need to make a collection of these rules where each definition has a single property that it tracks
        RuleDefinition::initialize(mRuleDefConfigKey, dummySalience, 0);

        // Validate that all global filters are known:
        for (auto& filter : globalFilters)
        {
            if (filters.find(filter) == filters.end())
            {
                ERR_LOG("[PropertyRuleDefinition].initialize: Global filter(" << filter << "), does not exist in filter map.  Check the config files and ensure all global filters match valid filter names..");
                return false;
            }
        }

        // Load the filters:
        for (auto& filter : filters)
        {
            if (filter.second->getIsPackerSiloFilter())
                continue;

            const char8_t* filterType = (globalFilters.findAsSet(filter.first) != globalFilters.end()) ? "global" : "scenario";
            const char8_t* gamePropertyName = filter.second->getGameProperty();

            mGamePropertyNameList.insertAsSet(gamePropertyName);
            cacheWMEAttributeName(gamePropertyName);                // We cache the value here so it will speed up WME lookups, and can be accessed from the JoinNode. 

            TRACE_LOG("[PropertyRuleDefinition].initialize: add property(" << gamePropertyName << ") for " << filterType << " filter(" << filter.first.c_str() << ")");
        }


        const char8_t* outMissingProperty = nullptr;
        if (!mPropertyManager.validatePropertyList(propertyConfig, mGamePropertyNameList, &outMissingProperty))
        {
            ERR_LOG("[PropertyRuleDefinition].initialize: failed to add filter for invalid game property(" << (outMissingProperty ? outMissingProperty : "UNKNOWN") << ").");
            return false;
        }
        
        return true;
    }

    void PropertyRuleDefinition::initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const
    {
      
    }

    void PropertyRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        upsertWorkingMemoryElementsInternal(wmeManager, gameSessionSlave, false);
    }

    // This is the function called by Packer Session to insert WMEs (for Silo selection/creation).
    void PropertyRuleDefinition::addPackerSessionWMEs(GameManager::Rete::WMEId packerSessionId, const MatchmakingFilterCriteriaMap& filterMap, const GameManager::PropertyNameList& mmPropertyList, GameManager::Rete::WMEManager& wmeManager) const
    {
        typedef eastl::hash_set<EA::TDF::TdfString, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> IndexedPropertySet;
        IndexedPropertySet indexedPropertySet;

        // Add all the Filter's WMEs:
        for (auto& filterDefItr : filterMap)
        {
            const GameManager::MatchmakingFilterName& filterName = filterDefItr.first;
            EA::TDF::GenericTdfType& filterGenericCriteria = *filterDefItr.second;
            const EA::TDF::TdfPtr filterCriteria = (const EA::TDF::TdfPtr)&filterGenericCriteria.get().asTdf();   // TODO: Clean up this conversion code

            // Lookup/use the Filters as defined in the Config
            const auto& filterConfigDefItr = getFilters().find(filterName);
            if (filterConfigDefItr == getFilters().end())
            {
                ERR_LOG("[PropertyRuleDefinition].addPackerSessionWMEs: filter(" << filterName << ") is missing from gamepacker.cfg.  Check your configs.");
                continue;
            }

            // Filters have already applied the properties (in Scenarios.cpp) before we get to this point. 
            const auto& filterDef = *filterConfigDefItr->second;
            const EA::TDF::TdfString& gamePropertyName = filterDef.getGamePropertyAsTdfString();
            eastl::string wmeName = filterDef.getGameProperty();
            wmeName.make_lower();

            // Add the WMEs:
            bool propertyIndexed = insertPackerSessionWME(filterCriteria, wmeManager, packerSessionId, wmeName);

            if (!propertyIndexed)
            {
                WARN_LOG("[PropertyRuleDefinition].addPackerSessionWMEs: no values were added to property " << gamePropertyName.c_str());
            }

            if (propertyIndexed)
                indexedPropertySet.insert(gamePropertyName);
        }

        // Add indexes for unreferenced properties (Properties used by Scenarios/Filters that reference this CG Template, but not set by the Filters)
        // These values came from the CreateGameTemplate config values, and were set via populateTemplateToPropertyListMap in Scenarios code (before being set to the getMatchmakingPropertyList).
        for (auto& propertyName : mmPropertyList)
        {
            if (indexedPropertySet.find(propertyName.c_str()) == indexedPropertySet.end())
            {
                TRACE_LOG("[PropertyRuleDefinition].addPackerSessionWMEs: Inserting WME_UNSET_HASH value for PackerSession(" << packerSessionId << "), gameProperty(" << propertyName << ")");
                wmeManager.insertWorkingMemoryElement(packerSessionId, propertyName.c_str(), GameManager::Rete::WME_UNSET_HASH, *this, false);
            }
        }
    }

    bool PropertyRuleDefinition::insertPackerSessionWME(const EA::TDF::TdfPtr filterCriteria, GameManager::Rete::WMEManager& wmeManager, GameManager::Rete::WMEId wmeId, const eastl::string& wmeName) const
    {
        BLAZE_TRACE_LOG(wmeManager.getLogCategory(), "[PropertyRuleDefinition].insertWorkingMemoryElement: Insert property(" << wmeId << ":" << wmeName << ") for PackerSessionWME.");


        switch (filterCriteria->getTdfId())
        {
        case GameManager::IntRangeFilter::TDF_ID:
            return false; // PACKER_TODO: implement as a set of individual values (this is very inefficient but we have no choice for now, due to the nature of RETE impl) Would only work for small integer ranges < 20 elements.

        case GameManager::IntSetFilter::TDF_ID:
        {
            GameManager::IntSetFilter& filter = (GameManager::IntSetFilter&)*filterCriteria;
            if (filter.getValues().empty())
            {
                WARN_LOG("[GamePackerSlaveImpl].addToReteIndex: IntSetFilter filter for property(" << wmeName << ") value list is empty, skipping."); // PACKER_TODO: This should be validated at configuration time.
                return false;
            }
            for (int64_t intValue : filter.getValues())
            {
                wmeManager.insertWorkingMemoryElement(wmeId, wmeName.c_str(), (Blaze::GameManager::Rete::WMEAttribute)intValue, *this);
            }
            break;
        }
        case GameManager::StringSetFilter::TDF_ID:
        {
            GameManager::StringSetFilter& filter = (GameManager::StringSetFilter&)*filterCriteria;
            if (filter.getValues().empty())
            {
                WARN_LOG("[GamePackerSlaveImpl].addToReteIndex: StringSetFilter filter for property(" << wmeName << ") value list is empty, skipping."); // PACKER_TODO: This should be validated at configuration time.
                return false;
            }
            for (EA::TDF::TdfString& stringValue : filter.getValues())
            {
                wmeManager.insertWorkingMemoryElement(wmeId, wmeName.c_str(), getWMEAttributeValue(stringValue.c_str()), *this);
            }
            break;
        }

        case GameManager::UIntBitwiseFilter::TDF_ID:
        {
            GameManager::UIntBitwiseFilter& filter = (GameManager::UIntBitwiseFilter&)*filterCriteria;
            wmeManager.insertWorkingMemoryElement(wmeId, wmeName.c_str(), (Blaze::GameManager::Rete::WMEAttribute)filter.getTestBits(), *this);
            break;
        }

        case GameManager::IntEqualityFilter::TDF_ID:
        {
            GameManager::IntEqualityFilter& filter = (GameManager::IntEqualityFilter&)*filterCriteria;
            wmeManager.insertWorkingMemoryElement(wmeId, wmeName.c_str(), (Blaze::GameManager::Rete::WMEAttribute)filter.getValue(), *this);
            break;
        }
        case GameManager::StringEqualityFilter::TDF_ID:
        {
            GameManager::StringEqualityFilter& filter = (GameManager::StringEqualityFilter&)*filterCriteria;
            wmeManager.insertWorkingMemoryElement(wmeId, wmeName.c_str(), getWMEAttributeValue(filter.getValue()), *this);
            break;
        }

        case GameManager::PlatformFilter::TDF_ID:
        {
            GameManager::PlatformFilter& filter = (GameManager::PlatformFilter&)*filterCriteria;

            // Iterate over all hosted Platforms and add the ALLOWED/not platforms.
            for (auto& curPlatform : gController->getHostedPlatforms())
            {
                // The Override list determines what's 'allowed'
                bool platformAllowed = (filter.getClientPlatformListOverride().findAsSet(curPlatform) != filter.getClientPlatformListOverride().end());
                wmeManager.insertWorkingMemoryElement(wmeId, getAllowedPlatformString(curPlatform), getWMEBooleanAttributeValue(platformAllowed), *this);

                bool platformRequired = (filter.getRequiredPlatformList().findAsSet(curPlatform) != filter.getRequiredPlatformList().end());
                wmeManager.insertWorkingMemoryElement(wmeId, getRequiredPlatformString(curPlatform), getWMEBooleanAttributeValue(platformRequired), *this);
            }
            break;
        }

        default:
            ERR_LOG("Unknown filter type!");
            return false;

        } // for filters in filterMap

        return true;
    }

    void PropertyRuleDefinition::convertProdNodeToSiloProperties(const MatchmakingFilterCriteriaMap& sessionFilters, Blaze::GameManager::Rete::ProductionNode* prodNode, GameManager::PropertyNameMap& outSiloProperties) const
    {
        Blaze::GameManager::Rete::JoinNode& node = prodNode->getParent();
        // NOTE:  The following logic is dependent on the PropertyRule code.  It may make sense to move this logic there (possibly via a static function).
        for (auto& filterDefItr : sessionFilters)
        {
            const GameManager::MatchmakingFilterName& filterName = filterDefItr.first;
            EA::TDF::GenericTdfType& filterGenericCriteria = *filterDefItr.second;
            const EA::TDF::TdfPtr filterCriteria = (const EA::TDF::TdfPtr)&filterGenericCriteria.get().asTdf();   // TODO: Clean up this conversion code

             // Lookup/use the Filters as defined in the Config
            const auto& filterConfigDefItr = getFilters().find(filterName);
            if (filterConfigDefItr == getFilters().end())
            {
                ERR_LOG("[PropertyRuleDefinition].convertProdNodeToSiloProperties: filter(" << filterName << ") is missing from gamepacker.cfg.  Check your configs.");
                continue;
            }

            const auto& filterDef = *filterConfigDefItr->second;
            const EA::TDF::TdfString& gamePropertyName = filterDef.getGamePropertyAsTdfString();

            // Add the new element: 
            if (outSiloProperties.find(gamePropertyName) != outSiloProperties.end())
            {
                ERR_LOG("[PropertyRuleDefinition].convertProdNodeToSiloProperties: filter(" << filterName << ") is using game property (" << gamePropertyName << ") which has already been set on the Silo!  Skipping.");
                continue;
            }

            // Find the node that has the condition we're checking for: 
            bool foundNode = false;
            void* nodeUserData = nullptr;
            EA::TDF::GenericTdfType genericValue;
            {
                eastl::string wmeName = filterDef.getGameProperty();
                wmeName.make_lower();
                GameManager::Rete::WMEAttribute wmeAttributeName = getWMEAttributeName(wmeName.c_str());

                auto curNode = &node;
                while (!curNode->isHeadNode())
                {
                    if (curNode->getProductionTest().getName() == wmeAttributeName)
                    {
                        foundNode = true;
                        break;
                    }
                    curNode = curNode->getParent();
                }

                if (foundNode)
                {
                    nodeUserData = curNode->getProductionTest().getUserData();
                    auto conditionMapping = mConditionGenericMap.find(curNode->getProductionTest());
                    if (conditionMapping != mConditionGenericMap.end())
                    {
                        genericValue = conditionMapping->second;
                    }
                }
            }

            // Create a Generic to store for this node, depending on the Filter Type: 
            switch (filterCriteria->getTdfId())
            {
            case GameManager::IntEqualityFilter::TDF_ID:
            {
                GameManager::IntEqualityFilter& filter = (GameManager::IntEqualityFilter&)*filterCriteria;
                genericValue.set(filter.getValue());
                break;
            }
            case GameManager::StringEqualityFilter::TDF_ID:
            {
                GameManager::StringEqualityFilter& filter = (GameManager::StringEqualityFilter&)*filterCriteria;
                genericValue.set(filter.getValue());
                break;
            }

            case GameManager::IntRangeFilter::TDF_ID:
                break;

            case GameManager::IntSetFilter::TDF_ID:
            {
// Nothing more to do, since the Generic is already loaded above:

                //GameManager::IntSetFilter& filter = (GameManager::IntSetFilter&)*filterCriteria;
                //uint64_t index = (uint64_t)nodeUserData;
                //if (!foundNode || index >= filter.getValues().size())
                //{
                //    ERR_LOG("[PropertyRuleDefinition].convertProdNodeToSiloProperties: IntSetFilter filter(" << filterName << ") was either unable (" << foundNode << ") to find a node, or had a value outside the value range. (" << index << "/" << filter.getValues().size() << ")  Skipping.");
                //    continue;
                //}
                //genericValue.set(filter.getValues()[index]);
                break;
            }
            case GameManager::StringSetFilter::TDF_ID:
            {
// Nothing more to do, since the Generic is already loaded above:

                //GameManager::StringSetFilter& filter = (GameManager::StringSetFilter&)*filterCriteria;
                //uint64_t index = (uint64_t)nodeUserData;
                //if (!foundNode || index >= filter.getValues().size())
                //{
                //    ERR_LOG("[PropertyRuleDefinition].convertProdNodeToSiloProperties: StringSetFilter filter(" << filterName << ") was either unable (" << foundNode << ") to find a node, or had a value outside the value range. (" << index << "/" << filter.getValues().size() << ")  Skipping.");
                //    continue;
                //}
                //genericValue.set(filter.getValues()[index]);
                break;
            }

            case GameManager::UIntBitwiseFilter::TDF_ID:
            {
                GameManager::UIntBitwiseFilter& filter = (GameManager::UIntBitwiseFilter&)*filterCriteria;
                // PACKER_TODO:  Check if Bitwise filters are even supported.
                genericValue.set(filter.getTestBits());
                break;
            }

            case GameManager::PlatformFilter::TDF_ID:
            {
                GameManager::PlatformFilter& filter = (GameManager::PlatformFilter&)*filterCriteria;
                genericValue.set(filter.getClientPlatformListOverride());
                break;
            }

            default:
                ERR_LOG("Unknown filter type!");
                continue;

            } // for filters in filterMap

            // Set the Property:
            if (genericValue.isValid())
            {
                outSiloProperties[gamePropertyName] = outSiloProperties.allocate_element();
                genericValue.copyInto(*outSiloProperties[gamePropertyName]);
            }
        }
    }

    void PropertyRuleDefinition::convertPackerSessionFiltersToSiloFilters(const MatchmakingFilterCriteriaMap& sessionFilters, const GameManager::PropertyNameMap& siloProperties, MatchmakingFilterCriteriaMap& outRequestFilters) const
    {
        // Rewrite the Session's filter requirements by substituting the property values from the current silo
        for (auto& filterItr : sessionFilters)
        {
            const auto& filterName = filterItr.first;
            const EA::TDF::GenericTdfType& filterGenericCriteria = *filterItr.second;
            const auto& filterConfigDefItr = getFilters().find(filterName);
            if (filterConfigDefItr == nullptr)
            {
                ERR_LOG("[PropertyRuleDefinition].convertPackerSessionFiltersToSiloFilters: Missing/Unknown filter used in request: (" << filterName << ").");
                continue;
            }

            // If no Game Property was used by the Filter, we don't add it here:  (Used for the QualityFactorHash filter)
            if (filterConfigDefItr->second->getIsPackerSiloFilter())
            {
                TRACE_LOG("[PropertyRuleDefinition].convertPackerSessionFiltersToSiloFilters: Skipping filter: (" << filterName << ") since it's only a packer silo filter. ");
                continue;
            }

            auto siloPropertiesItr = siloProperties.find(filterConfigDefItr->second->getGameProperty());
            if (siloPropertiesItr == siloProperties.end() || !siloPropertiesItr->second->isValid())
            {
                if (filterGenericCriteria.getTdfId() != GameManager::PlatformFilter::TDF_ID)   // PlatformFilter isn't expected to set any silo properties
                {
                    TRACE_LOG("[PropertyRuleDefinition].convertPackerSessionFiltersToSiloFilters: Filter: (" << filterName << ") uses property that is not valid or not included. ");
                    continue;
                }
            }

            // Add the value to the request:
            auto reqFilterElement = outRequestFilters.allocate_element();
            filterGenericCriteria.copyInto(*reqFilterElement);
            outRequestFilters[filterName] = reqFilterElement;


            const EA::TDF::TdfPtr reqFilterCriteria = (const EA::TDF::TdfPtr)&reqFilterElement->get().asTdf();   // PACKER_TODO: Clean up this conversion code
            switch (reqFilterCriteria->getTdfId())
            {
            case GameManager::IntSetFilter::TDF_ID:
            {
                int64_t intValue = 0;
                siloPropertiesItr->second->convertToResult(intValue);
                GameManager::IntSetFilter& filter = (GameManager::IntSetFilter&)*reqFilterCriteria;
                filter.getValues().asVector().clear();
                filter.getValues().asVector().push_back(intValue);
                break;
            }
            case GameManager::StringSetFilter::TDF_ID:
            {
                EA::TDF::TdfString stringValue;
                siloPropertiesItr->second->convertToResult(stringValue);
                GameManager::StringSetFilter& filter = (GameManager::StringSetFilter&)*reqFilterCriteria;
                filter.getValues().asVector().clear();
                filter.getValues().asVector().push_back(stringValue);
                break;
            }

            case GameManager::PlatformFilter::TDF_ID:
            {
                // For the new Silo we use the requirements of the Session, but set all the Allowed/Optional Platforms as 'REQUIRED', so that we find only Exact Match games. 
                GameManager::PlatformFilter& filter = (GameManager::PlatformFilter&)*reqFilterCriteria;
                filter.getRequiredPlatformList().clear();
                filter.getClientPlatformListOverride().copyInto(filter.getRequiredPlatformList());
                filter.getClientPlatformListOverride().clear();
                break;
            }

            case GameManager::UIntBitwiseFilter::TDF_ID:
            case GameManager::IntEqualityFilter::TDF_ID:
            case GameManager::StringEqualityFilter::TDF_ID:
                // PACKER_TODO Range filter is currently not supported
            case GameManager::IntRangeFilter::TDF_ID:
            default:
                break;
            }
        }
    }

    void PropertyRuleDefinition::updatePlatformList(const MatchmakingFilterCriteriaMap& sessionFilters, const GameManager::PropertyNameMap& siloProperties, ClientPlatformTypeList& outPlatformList) const
    {
        for (auto& filterItr : sessionFilters)
        {
            const auto& filterName = filterItr.first;
            const EA::TDF::GenericTdfType& filterGenericCriteria = *filterItr.second;
            const auto& filterConfigDefItr = getFilters().find(filterName);
            if (filterConfigDefItr == nullptr)
            {
                ERR_LOG("[PropertyRuleDefinition].updatePlatformList: Missing/Unknown filter used in request: (" << filterName << ").");
                continue;
            }

            // We're just looking for the PlatformFilter:  (We assume there is only one.  Prior checks in GamePackerMasterImpl::initializeAggregateProperties should ensure this.)
            const EA::TDF::TdfPtr filterCriteria = (const EA::TDF::TdfPtr)&filterGenericCriteria.get().asTdf();   // PACKER_TODO: Clean up this conversion code
            if (filterCriteria->getTdfId() != GameManager::PlatformFilter::TDF_ID)
                continue;

            const GameManager::PlatformFilter& filter = (const GameManager::PlatformFilter&)*filterCriteria;
            if (outPlatformList.empty() || filter.getCrossplayMustMatch())
            {
                // First user and 'must match' platforms all just set the platform list directly: 
                filter.getClientPlatformListOverride().copyInto(outPlatformList);
            }
            else
            {
                // After that, we build the list via an intersection of the allowed list:
                for (auto outCurPlatIter = outPlatformList.begin(); outCurPlatIter != outPlatformList.end(); )
                {
                    if (filter.getClientPlatformListOverride().findAsSet(*outCurPlatIter) == filter.getClientPlatformListOverride().end())
                        outCurPlatIter = outPlatformList.erase(outCurPlatIter);
                    else
                        ++outCurPlatIter;
                }
            }

            // Only should be one PlatformFilter, so we can end immediately:
            return;
        }
    }

    const char8_t* PropertyRuleDefinition::getAllowedPlatformString(ClientPlatformType platform) const
    {
        auto iter = mAllowedStringMap.find(platform);
        if (iter != mAllowedStringMap.end())
            return iter->second.c_str();

        return "ALLOWED_INVALID";
    }

    const char8_t* PropertyRuleDefinition::getRequiredPlatformString(ClientPlatformType platform) const
    {
        auto iter = mRequiredStringMap.find(platform);
        if (iter != mRequiredStringMap.end())
            return iter->second.c_str();

        return "REQUIRED_INVALID";
    }

    void PropertyRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        upsertWorkingMemoryElementsInternal(wmeManager, gameSessionSlave, true);
    }

    EA::TDF::TdfId getFilterTdfId(const char8_t* filterTdfName)
    {
        // PACKER_TODO:  Just cache these all beforehand to avoid pointless string look ups
        return EA::TDF::TdfFactory::get().getTdfIdFromName(filterTdfName);
    }

    // This is the function called by Active Games to insert WMEs.
    void PropertyRuleDefinition::upsertWorkingMemoryElementsInternal(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave, bool upsert) const
    {
        GameId gameId = gameSessionSlave.getGameId();

        PropertyNameMap curPropertyMap;
        PropertyNameMap& prevPropertyMap = gameSessionSlave.getPrevProperties();

        const PropertyDataSources& gameProperties = gameSessionSlave.getGameProperties();
        mPropertyManager.convertProperties(gameProperties, mGamePropertyNameList, curPropertyMap);

        for (auto& filterPropItr : mPropertyFiltersConfig)
        {
            const auto& filterName = filterPropItr.first;
            const auto& gamePropertyName = filterPropItr.second->getGameProperty();
            if (filterPropItr.second->getIsPackerSiloFilter())
                continue;

            PropertyNameMap::iterator curProp = curPropertyMap.find(gamePropertyName);
            if (curProp == curPropertyMap.end())
            {
                BLAZE_WARN_LOG(wmeManager.getLogCategory(), "[PropertyRuleDefinition].upsertWorkingMemoryElementsInternal: game property(" << gamePropertyName << ") used by filter(" << filterName << ") missing from current game(" << gameId << ") properties");
                continue;
            }
            if (filterPropItr.second->getRequirement().empty())
            {
                BLAZE_WARN_LOG(wmeManager.getLogCategory(), "[PropertyRuleDefinition].upsertWorkingMemoryElementsInternal: requirement used by filter(" << filterName << ") missing from current game(" << gameId << ") properties");
                continue;
            }
            const auto& requirementTypeName = filterPropItr.second->getRequirement().begin()->first;

            const EA::TDF::GenericTdfType& curValue = *curProp->second;
            const EA::TDF::GenericTdfType* prevValue = nullptr;

            if (upsert)
            {
                PropertyNameMap::iterator prevProp = prevPropertyMap.find(gamePropertyName);
                if (prevProp == prevPropertyMap.end())
                {
                    BLAZE_WARN_LOG(wmeManager.getLogCategory(), "[PropertyRuleDefinition].upsertWorkingMemoryElementsInternal: game property(" << gamePropertyName << ") used by filter(" << filterName << ") missing from previous game(" << gameId << ") properties");
                }
                prevValue = prevProp->second;
            }

            bool success = updateActiveGameWMEs(getFilterTdfId(requirementTypeName.c_str()), (Blaze::GameManager::Rete::WMEId)gameId, wmeManager, gamePropertyName, curValue, prevValue);
            if (!success)
            {
                BLAZE_TRACE_LOG(wmeManager.getLogCategory(), "[PropertyRuleDefinition].upsertWorkingMemoryElementsInternal: skip unchanged property(" << gamePropertyName << "), value(" << curValue << ") for game(" << gameId << ")");
            }

        }

        // Copy over the old cached values: 
        // PACKER_TODO - This doesn't really seem like it should be needed.  We should be able to just use the RETE data, and do upserts every time. 
        curPropertyMap.copyInto(prevPropertyMap);
    }

    void PropertyRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
       

    }

    void PropertyRuleDefinition::updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const
    {
        // PACKER_TODO:  We're not caching the values currently.  If we need to do that (for speed) do the calc in GameSessionSearchSlave::updateGameProperties instead. 
    }

    bool PropertyRuleDefinition::updateActiveGameWMEs(EA::TDF::TdfId filterTdfId, Blaze::GameManager::Rete::WMEId wmeId, GameManager::Rete::WMEManager& wmeManager, const PropertyName& gamePropertyName, const EA::TDF::GenericTdfType& curValue, const EA::TDF::GenericTdfType* prevValue) const
    {
        eastl::string wmeName = gamePropertyName.c_str();
        wmeName.make_lower();

        BLAZE_TRACE_LOG(wmeManager.getLogCategory(), "[PropertyRuleDefinition].insertWorkingMemoryElement: Insert property(" << wmeId << ":" << gamePropertyName << "), value(" << curValue << ").");

        // Switch by the PropertyType: 
        switch (filterTdfId)
        {
        // int64_t types:
        case IntRangeFilter::TDF_ID: 
        case IntSetFilter::TDF_ID:    
        case IntEqualityFilter::TDF_ID:
        {
            if (prevValue)
            {
                if (prevValue->equalsValue(curValue))
                    return false;

                int64_t intValue = 0;
                prevValue->convertToResult(intValue);
                wmeManager.removeWorkingMemoryElement(wmeId, wmeName.c_str(), (Blaze::GameManager::Rete::WMEAttribute)intValue, *this);
            }

            // Convert to type: 
            int64_t intValue = 0;
            curValue.convertToResult(intValue);

            // Add/remove type from WMEs
            wmeManager.insertWorkingMemoryElement(wmeId, wmeName.c_str(), (Blaze::GameManager::Rete::WMEAttribute)intValue, *this);
            break;
        }

        case PlatformFilter::TDF_ID:
        {
            // Active Games only set the ALLOWED platform fields, since that's what the silos will be looking for:
            if (prevValue)
            {
                if (prevValue->equalsValue(curValue))
                    return false;
                
                ClientPlatformTypeList platformList;
                prevValue->convertToResult(platformList);
                for (auto& curPlatform : gController->getHostedPlatforms())
                {
                    // The Override list determines what's 'allowed'
                    bool platformAllowed = (platformList.findAsSet(curPlatform) != platformList.end());
                    wmeManager.removeWorkingMemoryElement(wmeId, getAllowedPlatformString(curPlatform), getWMEBooleanAttributeValue(platformAllowed), *this);
                }
            }

            // Convert to type: 
            ClientPlatformTypeList platformList;
            curValue.convertToResult(platformList);
            for (auto& curPlatform : gController->getHostedPlatforms())
            {
                // The Override list determines what's 'allowed'
                bool platformAllowed = (platformList.findAsSet(curPlatform) != platformList.end());
                wmeManager.insertWorkingMemoryElement(wmeId, getAllowedPlatformString(curPlatform), getWMEBooleanAttributeValue(platformAllowed), *this);
            }
            break;
        }

        // String types:
        case StringSetFilter::TDF_ID:       
        case StringEqualityFilter::TDF_ID:  
        {
            if (prevValue)
            {
                if (prevValue->equalsValue(curValue))
                    return false;

                EA::TDF::TdfString stringValue;
                prevValue->convertToResult(stringValue);
                wmeManager.removeWorkingMemoryElement(wmeId, wmeName.c_str(), getWMEAttributeValue(stringValue.c_str()), *this);
            }

            // Convert to type: 
            EA::TDF::TdfString stringValue;
            curValue.convertToResult(stringValue);

            // Add/remove type from WMEs
            wmeManager.insertWorkingMemoryElement(wmeId, wmeName.c_str(), getWMEAttributeValue(stringValue.c_str()), *this);
            break;
        }

        // Bitwise special case
        case UIntBitwiseFilter::TDF_ID:
        {
            // Convert to type: 
            uint64_t curUintValue = 0;
            curValue.convertToResult(curUintValue);

            uint64_t prevUintValue = 0;
            if (prevValue)
            {
                if (prevValue->equalsValue(curValue))
                    return false;

                prevValue->convertToResult(prevUintValue);
            }

            // We have to put in all 64 values, since the mask may include any of them. 
            // PACKER_TODO - We could switch this to use the negate value, and only set values for TRUE.  
            //    That would reduce the number of entries in the base case, rather than always using 64.
            const auto WME_BOOLEAN_TRUE = getWMEBooleanAttributeValue(true);
            const auto WME_BOOLEAN_FALSE = getWMEBooleanAttributeValue(false);

            eastl::string gamePropertyBitName;
            for (uint32_t bit = 0; bit < 64; ++bit)
            {
                bitNameHelper(bit, wmeName, gamePropertyBitName); // NOTE: Always use bitmask name to consistently handle bitfield and non-bitfield properties because it matches the game.propertyName, rather than the more specific game.propertyName[bitfieldMemberName]

                bool curBit = curUintValue & (uint64_t(1) << bit);
                if (prevValue)
                {
                    bool prevBit = prevUintValue & (uint64_t(1) << bit);
                    if (curBit == prevBit)
                        continue;

                    wmeManager.removeWorkingMemoryElement(wmeId, gamePropertyBitName.c_str(), prevBit ? WME_BOOLEAN_TRUE : WME_BOOLEAN_FALSE, *this, false);
                }

                wmeManager.insertWorkingMemoryElement(wmeId, gamePropertyBitName.c_str(), curBit ? WME_BOOLEAN_TRUE : WME_BOOLEAN_FALSE, *this, false);
            }

            break;
        }
        default:
            BLAZE_WARN_LOG(wmeManager.getLogCategory(), "[PropertyRuleDefinition].insertWorkingMemoryElements: Filter for property(" << gamePropertyName << ") somehow is missing a tdf id.  This should've been caught at config time.");
            return false;
        }

        return true;
    }


} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
