/*! ************************************************************************************************/
/*!
\file   matchmakingfiltersutil.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#include "framework/blaze.h"
#include "framework/usersessions/usersessionmanager.h"
#include "gamemanager/rpc/gamemanagerslave.h"
#include "gamemanager/templateattributes.h"
#include "matchmakingfiltersutil.h"
#include "gamemanager/gamepacker/gamepackerslaveimpl.h"
#include "gamemanager/searchcomponent/searchslaveimpl.h"

namespace Blaze
{
namespace GameManager
{
    const char8_t* PropertyManager::PROPERTY_PLAYER_PARTICIPANT_IDS = "players.participantIds";

    const size_t GAME_PROPERTY_PREFIX_LEN = strlen(GAME_PROPERTY_PREFIX);
    const size_t CALLER_PROPERTY_PREFIX_LEN = strlen(CALLER_PROPERTY_PREFIX);

    bool isGamePropertyName(const char8_t* gameProperty, const char8_t* propertyName)
    {
        return blaze_stricmp(gameProperty + GameManager::GAME_PROPERTY_PREFIX_LEN, propertyName) == 0;
    }

    bool isPlayersProperty(const char8_t* playerProperty)
    {
        return blaze_strnicmp(playerProperty, "players", strlen("players")) == 0;
    }

    PropertyManager::PropertyManager()
    {
        // PACKER_TODO - Update when real derived Properties are added.
        initDerivedPropertyExpressions();
    }

    PropertyManager::~PropertyManager()
    {
    }

    bool PropertyManager::validateConfig(const PropertyConfig& config, ConfigureValidationErrors& validationErrors) const
    {
        eastl::string errorStringPrefix = "PropertyManager::validateConfig";
        EA::TDF::TdfFactory& tdfFactory = EA::TDF::TdfFactory::get();
        eastl::hash_set<EA::TDF::TdfId> acceptableTdfs = {  ReplicatedGameDataServer::TDF_ID,
                                                            GameCreationData::TDF_ID,
                                                            StartMatchmakingInternalRequest::TDF_ID,
                                                            ReplicatedGamePlayerServer::TDF_ID,
                                                            PerPlayerJoinData::TDF_ID,
                                                            UserSessionInfo::TDF_ID };

        eastl::hash_set<eastl::string> mappedProperties;

        for (const auto& curTdfMapping : config.getTdfDataSources())
        {
            const EA::TDF::TypeDescription* typeDesc = tdfFactory.getTypeDesc(curTdfMapping.first);
            if (typeDesc == nullptr)
            {
                StringBuilder strBuilder;
                strBuilder << errorStringPrefix << ".  Attribute mapping uses unknown type name: " << curTdfMapping.first << ".  Make sure that the type is a TDF.";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
                continue;
            }

            if (!acceptableTdfs.empty() && acceptableTdfs.find(typeDesc->getTdfId()) == acceptableTdfs.end())
            {
                StringBuilder strBuilder;
                strBuilder << errorStringPrefix << ".  Attribute mapping uses unacceptable type name: " << typeDesc->getShortName() << ".  If desired, add this to the acceptableTdfs set.";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
                continue;
            }

            // Allocate a temp version of the TDF to use in the lookups:
            EA::TDF::TdfPtr requestTdfPtr = tdfFactory.create(typeDesc->getTdfId());
            EA::TDF::TdfGenericReference requestRef(requestTdfPtr);
            
            for (const auto& curPropMapping : *curTdfMapping.second)
            {
                auto& tdfMemberName = curPropMapping.second;

                // Check that the value can be parsed by the Tdf system: 
                EA::TDF::TdfGenericReference criteriaAttr;
                if (!tdfFactory.getTdfMemberReference(requestRef, tdfMemberName, criteriaAttr, nullptr, false))
                {
                    StringBuilder strBuilder;
                    strBuilder << errorStringPrefix <<
                        "Unable to get TdfMemberReference for attribute. Member location: '" << tdfMemberName <<
                        "'.  Check if this value has changed its name or location. ";
                    validationErrors.getErrorMessages().push_back(strBuilder.get());
                }

                mappedProperties.insert(curPropMapping.first.c_str());
            }
        }

        for (const auto& curPropMapping : config.getDerivedProperties())
        {
            const auto& curPropName = curPropMapping.first;
            const auto& curDerivedFunc = curPropMapping.second;

            // Check that the properties used are valid:
            auto funcIter = mDerivedFunctionMap.find(curDerivedFunc.c_str());
            if (funcIter == mDerivedFunctionMap.end())
            {
                StringBuilder strBuilder;
                strBuilder << errorStringPrefix << "Property "<< curPropName.c_str() << " uses unknown derived property function: '" << curDerivedFunc.c_str() << "'";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
            }

            // Add to the mapped properties
            mappedProperties.insert(curPropMapping.first.c_str());
        }


        for (const auto& curPropName : config.getRequiredPackerInputProperties())
        {
            if (mappedProperties.find(curPropName.c_str()) == mappedProperties.end())
            {
                StringBuilder strBuilder;
                strBuilder << errorStringPrefix << "Unknown property: '" << curPropName << "' used in RequiredPackerInputProperties.";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
            }
        }
        for (const auto& curPropName : config.getGameBrowserPackerProperties())
        {
            if (mappedProperties.find(curPropName.c_str()) == mappedProperties.end())
            {
                StringBuilder strBuilder;
                strBuilder << errorStringPrefix << "Unknown property: '" << curPropName << "' used in gameBrowserPackerProperties.";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
            }
        }
        for (const auto& curPropName : config.getPackerOutputProperties())
        {
            if (mappedProperties.find(curPropName.c_str()) == mappedProperties.end())
            {
                StringBuilder strBuilder;
                strBuilder << errorStringPrefix << "Unknown property: '" << curPropName << "' used in packerOutputProperties.";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
            }

            auto gameCreationDataMapping = config.getTdfDataSources().find("GameCreationData");
            if (gameCreationDataMapping != config.getTdfDataSources().end())
            {
                if (gameCreationDataMapping->second->find(curPropName.c_str()) == gameCreationDataMapping->second->end())
    {
                    StringBuilder strBuilder;
                    strBuilder << errorStringPrefix << "Property: '" << curPropName << "' used in packerOutputProperties needs to be mapped in the GameCreationData.";
                    validationErrors.getErrorMessages().push_back(strBuilder.get());
                }
            }
        }

        return (validationErrors.getErrorMessages().size() == 0);
    }

    bool PropertyManager::onConfigure(const PropertyConfig& config)
    {
        // Build the values 
        for (const auto& curPropName : config.getPackerOutputProperties())
        {
            // The source of these values should be mapped to the GameCreationData value.
            auto gameCreationDataMapping = config.getTdfDataSources().find("GameCreationData");
            if (gameCreationDataMapping != config.getTdfDataSources().end())
            {
                auto tdfMemberName = gameCreationDataMapping->second->find(curPropName.c_str());
                if (tdfMemberName != gameCreationDataMapping->second->end())
                {
                    eastl::string createGameReqValue = "CreateGameRequest.gameCreationData.";
                    createGameReqValue += tdfMemberName->second.c_str();
                    mCgRequestPropertyNameMapping[createGameReqValue] = curPropName.c_str();

                }
            }
        }

        config.copyInto(mPropertyConfig);

        return true;
    }

    bool PropertyManager::validateFilters(const PropertyConfig& config, const MatchmakingFilterMap& filters, ConfigureValidationErrors& validationErrors) const
    {
        AttributeToTypeDescMap attrToTypeDesc;

        eastl::set<EA::TDF::TdfId> acceptableTdfs = {   IntRangeFilter::TDF_ID,
                                                        IntSetFilter::TDF_ID,
                                                        StringSetFilter::TDF_ID,
                                                        IntEqualityFilter::TDF_ID,
                                                        StringEqualityFilter::TDF_ID,
                                                        UIntBitwiseFilter::TDF_ID,
                                                        PlatformFilter::TDF_ID };

        for (auto& filterItr : filters)
        {
            auto& filterName = filterItr.first;
            auto& matchmakingFilterDefinition = *filterItr.second;
            // 1. Validate Game Property
            StringBuilder sb;
            
            if (!matchmakingFilterDefinition.getIsPackerSiloFilter())
            {
                PropertyNameList propertiesUsed;
                propertiesUsed.push_back(matchmakingFilterDefinition.getGameProperty());        // We should also add in all the properties that are used in the values themselves. (But we can't if those props come form Input sanitizers. Sigh.)
                if (!validatePropertyList(config, propertiesUsed, nullptr))
                {
                    sb << "[PropertyManager].validateFilter: failed to parse property(" << matchmakingFilterDefinition.getGameProperty() << ") from for scenarios' filter(" << filterName << ").";
                    validationErrors.getErrorMessages().push_back(sb.get());
                    return false;
                }
            }

            // 2. Validate Requirement
            StringBuilder errorPrefix;
            errorPrefix << "[PropertyManager].filterRequirementMapConfigValidation: Filter: " << filterName.c_str() << " ";
            bool result = validateTemplateTdfAttributeMapping(matchmakingFilterDefinition.getRequirement(), attrToTypeDesc, validationErrors, errorPrefix.get(), acceptableTdfs);
            if (!result)
            {
                ERR_LOG("[PropertyManager].validateFilter: validation failed for filter(" << filterName << ").");
                return false;
            }
        }
        return true;
    }


    // NOTE: This code splits the property name by '.' delimiter token assuming each property has the structure <object>.<propname>... = <value>
    bool PropertyDataSources::validateAndFetchProperty(const char8_t* propertyName, EA::TDF::TdfGenericReference& outRef, const EA::TDF::TypeDescriptionBitfieldMember** outBfMember, bool addUknownProperties) const
    {
        EA::TDF::TdfFactory& tdfFactory = EA::TDF::TdfFactory::get();
        const char8_t* tdfMemberName = strchr(propertyName, '.');
        EA_ASSERT_MSG(tdfMemberName != nullptr, "property name must contain .");
        EA::TDF::EATDFEastlString qualifiedName = mPropertyMap.getTypeDescription().shortName;
        qualifiedName += tdfMemberName;
        EA::TDF::TdfGenericReference propertiesMapRef(mPropertyMap);
        
        // We specifically avoid adding new map/list/generic values with this function, since it's supposed to be used for lookup only.
        if (tdfFactory.getTdfMemberReference(propertiesMapRef, qualifiedName.c_str(), outRef, outBfMember, true, addUknownProperties))
        {
            TRACE_LOG("[PropertyManager].validateAndFetchProperty: extracted property(" << propertyName << ") as tdf member(" << qualifiedName.c_str() << ")");
            return true;
        }
        else
        {
            // PACKER_TODO:  This log is expected when the lookup fails because the member doesn't exist (yet).  We don't have a way to indicate that there was a parsing error, though.
            //   The function is used in two places, in one it just wants to validate the type/data lookup is valid, in other places it's used to actually gather the missing values. 
            //   Therefore there should be two functions. 
            WARN_LOG("[PropertyManager].validateAndFetchProperty: misconfigured property or internal error: failed to extract tdf member(" << qualifiedName.c_str() << ") ref for property(" << propertyName << ")");
            return false;
        }
    }

    EA::TDF::TdfGenericValue& PropertyDataSources::insertAndReturn(const char8_t* propertyName)
    {
        auto itr = mPropertyMap.find(propertyName);
        if (itr == mPropertyMap.end())
        {
            auto& prop = mPropertyMap[propertyName];
            prop = mPropertyMap.allocate_element();
            return prop->getValue();
        }
        else
        {
            return itr->second->getValue();
        }
    }

    /*! ************************************************************************************************/
    /*! \brief get the value for port and IP, for game/caller properties, from NetworkAddress
    *************************************************************************************************/
    bool PropertyManager::getPortAndIpValue(uint16_t& port, uint32_t& ipAddress, const NetworkAddress& networkAddress)
    {
        port = 0;
        ipAddress = 0;
        switch (networkAddress.getActiveMember())
        {
        case NetworkAddress::MEMBER_HOSTNAMEADDRESS:
            port = networkAddress.getHostNameAddress()->getPort();
            // no IP
            break;
        case NetworkAddress::MEMBER_IPADDRESS:
            port = networkAddress.getIpAddress()->getPort();
            ipAddress = networkAddress.getIpAddress()->getIp();
            break;
        case NetworkAddress::MEMBER_IPPAIRADDRESS:
            port = networkAddress.getIpPairAddress()->getExternalAddress().getPort();
            ipAddress = networkAddress.getIpPairAddress()->getExternalAddress().getIp();
            break;
        default:
            // got invalid, unset or xbox client address
            TRACE_LOG("[PropertyManager].getPortAndIpValue: game session hostname unavailable, using NetworkAddress member index(" << networkAddress.getActiveMemberIndex() << "). Using default port and ip.");
            return false;
            break;
        }
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief translate the Roster UED property formula to old configurations' group value formula
    ****************************************************************************************************/
    bool PropertyManager::propertyUedFormulaToGroupValueFormula(GroupValueFormula& valueFormula, const EA::TDF::EATDFEastlString& propertyString)
    {
        if (blaze_stricmp(propertyString.c_str(), PROPERTY_NAME_UED_FORMULA_MAX) == 0)//"max"
            valueFormula = GROUP_VALUE_FORMULA_MAX;
        else if (blaze_stricmp(propertyString.c_str(), PROPERTY_NAME_UED_FORMULA_MIN) == 0)//"min"
            valueFormula = GROUP_VALUE_FORMULA_MIN;
        else if (blaze_stricmp(propertyString.c_str(), PROPERTY_NAME_UED_FORMULA_AVG) == 0)//"avg"
            valueFormula = GROUP_VALUE_FORMULA_AVERAGE;
        else if (blaze_stricmp(propertyString.c_str(), PROPERTY_NAME_UED_FORMULA_SUM) == 0)//"sum"
            valueFormula = GROUP_VALUE_FORMULA_SUM;
        else
        {
            WARN_LOG("[PropertyManager].propertyUedFormulaToGroupValueFormula: unhandled property value formula(" << propertyString.c_str() << ")");
            return false;
        }
        return true;
    }

    bool PropertyManager::parsePropertyName(const PropertyName& fullPropertyName, PropertyName& propertyName, PropertyName& propertyFieldName) const
    {
        // All properties are of the form (type).(value)[field-optional]
        // Therefore, we skip any values that are prior to the (type). stuff:
        const char8_t* propertyStart = fullPropertyName.c_str();
        const char8_t* prevSection = propertyStart;
        const char8_t* nextSection = blaze_strstr(prevSection, ".");
        while (nextSection != nullptr)
        {
            propertyStart = prevSection;
            prevSection = nextSection + 1;
            nextSection = blaze_strstr(prevSection, ".");
        }

        

        const char8_t* arrayLocation = blaze_strstr(propertyStart, "[");
        if (arrayLocation != nullptr)
        {
            propertyName.set(propertyStart, (EA::TDF::TdfStringLength)(arrayLocation - propertyStart));
            
            // Skip the '['
            ++arrayLocation;
            const char8_t* arrayEndLocation = blaze_strstr(propertyStart, "]");
            if (arrayLocation < arrayEndLocation)
            {
                propertyFieldName.set(arrayLocation, (EA::TDF::TdfStringLength)(arrayEndLocation - arrayLocation));
            }
            else
            {
                return false;  // Just using "game.value[]" is not supported currently.  
            }
        }
        else
        {
            propertyName.set(propertyStart, fullPropertyName.length() - (EA::TDF::TdfStringLength)(propertyStart - fullPropertyName.c_str()));
        }

        return true;
    }

    bool PropertyManager::lookupTdfDataSourceProperty(const EA::TDF::Tdf* dataSource, const PropertyName& fullPropertyName, EA::TDF::GenericTdfType& outProp, bool isListValue) const
    {
        if (dataSource == nullptr)
            return false;

        // Grab the config association: 
        auto tdfIter = mPropertyConfig.getTdfDataSources().find(dataSource->getClassName());
        if (tdfIter == mPropertyConfig.getTdfDataSources().end())
        {
            ERR_LOG("[PropertyManager].lookupTdfDataSourceProperty: Unknown/unmapped data source (" << dataSource->getClassName() << ").  Check config for missing values.");
            return false;
        }

        // Take the [ATTR_NAME] part out of the fullPropertyName, and search for that specifically. 
        PropertyName propertyName, propertyFieldName;
        if (!parsePropertyName(fullPropertyName, propertyName, propertyFieldName))
        {
            ERR_LOG("[PropertyManager].lookupTdfDataSourceProperty: PropertyName (" << fullPropertyName.c_str() << ") could not be parsed.");
            return false;
        }

        // See if the property is mapped to a TDF Member:
        auto tdfMemberIter = tdfIter->second->find(propertyName.c_str());
        if (tdfMemberIter == tdfIter->second->end())
        {
            return false;
        }

        bool isUedValue = (blaze_strstr(propertyName.c_str(), GameManager::PROPERTY_NAME_UED_VALUES) != nullptr);
        eastl::string memberName = tdfMemberIter->second.c_str();
        if (!propertyFieldName.empty())
        {
            // Special case for UED which wants to map based on a value that's unknown to TDFs:
            if (isUedValue)
            {
                UserExtendedDataKey key;
                if (!gUserSessionManager->getUserExtendedDataKey(propertyFieldName.c_str(), key))
                {
                    ERR_LOG("[PropertyManager].convertProperties: Unknown uedValue field for (" << fullPropertyName << ").");
                    return false;
                }

                memberName.append_sprintf("[%d]", key);
            }
            else
            {
                memberName.append_sprintf("[%s]", propertyFieldName.c_str());
            }
        }


        EA::TDF::TdfGenericReference criteriaRef(*dataSource);

        // TDF data lookup:
        EA::TDF::TdfGenericReference tdfRef;
        const EA::TDF::TypeDescriptionBitfieldMember* outBfMember = nullptr;
        EA::TDF::TdfFactory& tdfFactory = EA::TDF::TdfFactory::get();
        if (tdfFactory.getTdfMemberReference(criteriaRef, memberName.c_str(), tdfRef, &outBfMember, false))
        {
            uint32_t tempInt = 0;
            if (outBfMember != nullptr)
            {
                // If the property was pointing to a bitfield member, we don't have any way to track that, so rather than referencing the bf member, we just convert it to an int and pass that around.
                tdfRef.asBitfield().getValueByDesc(outBfMember, tempInt);

                EA::TDF::TdfGenericReference tdfRefInt(tempInt);
                tdfRef = tdfRefInt;
            }

            if (!isListValue)
            {
                // Non-list case (ex. "game.foo")
                outProp.set(tdfRef);
                TRACE_LOG("[PropertyManager].lookupTdfDataSourceProperty:  Data source (" << dataSource->getClassName() << ") lookup of member " << memberName.c_str() << " success.");
                return true;
            }
            else 
            {
                if (!outProp.isValid())
                {
                    const EA::TDF::TypeDescription* listTypeDesc = tdfFactory.getListTypeDescFromValue(tdfRef.getTdfId());
                    if (listTypeDesc == nullptr)
                    {
                        ERR_LOG("[PropertyManager].lookupTdfDataSourceProperty: Data source (" << dataSource->getClassName() << ") uses value type ("<< tdfRef.getFullName() <<") that has no corresponding list.  Make sure that a TDF list type is defined and used in code.");
                        return false;
                    }

                    // Special case for "players.uedValues" list of maps:  (Uses map<string - uedName, value> instead of map<int64, value)
                    if (isUedValue && propertyFieldName.empty())
                        listTypeDesc = &EA::TDF::TypeDescriptionSelector<PropertyValueMapList>::get();

                    outProp.setType(*listTypeDesc);
                }

                if (outProp.get().getType() != EA::TDF::TDF_ACTUAL_TYPE_LIST)
                {
                    ERR_LOG("[PropertyManager].lookupTdfDataSourceProperty: Data source (" << dataSource->getClassName() << ") found, but using a non-list type.  Make sure that there are not incompatible sources of the value.");
                    return false;
                }

                // Pull back from the list, and add the new element:
                EA::TDF::TdfGenericReference listValueRef;
                outProp.get().asList().pullBackRef(listValueRef);

                if (isUedValue && propertyFieldName.empty() && 
                    &tdfRef.getTypeDescription() == &EA::TDF::TypeDescriptionSelector<UserExtendedDataMap>::get() && 
                    &listValueRef.getTypeDescription() == &EA::TDF::TypeDescriptionSelector<PropertyValueMap>::get())
                {
                    // PACKER_TODO - Figure out a way to avoid this UED conversion hack (which converts an efficient map<uint64, uint64> to a map<string, value>)
                    const UserExtendedDataMap* uedMap = (UserExtendedDataMap*)&tdfRef.asMap();
                    PropertyValueMap* propMap = (PropertyValueMap*)&listValueRef.asMap();
                    for (auto& curUED : *uedMap)
                    {
                        const char8_t* uedName = gUserSessionManager->getUserExtendedDataName(curUED.first);
                        if (uedName != nullptr)
                        {
                            auto& pair = propMap->asMap().push_back_unsorted();
                            pair.first = uedName;
                            pair.second = curUED.second;
                        }
                    }
                }
                else
                {
                    tdfRef.convertToResult(listValueRef);
                }



                TRACE_LOG("[PropertyManager].lookupTdfDataSourceProperty:  Data source (" << dataSource->getClassName() << ") lookup of member " << memberName.c_str() << " success. (As List)");
                return true;
            }
        }
        
        WARN_LOG("[PropertyManager].lookupTdfDataSourceProperty:  Data source (" << dataSource->getClassName() << ") lookup of member " << memberName.c_str() << " failed.");
        return false;
    }

    bool PropertyManager::lookupDerivedProperty(const PropertyDataSources& dataSources, const PropertyName& fullPropertyName, EA::TDF::GenericTdfType& outProp) const
    {
        PropertyName propertyName, propertyFieldName;
        if (!parsePropertyName(fullPropertyName, propertyName, propertyFieldName))
        {
            ERR_LOG("[PropertyManager].lookupTdfDataSourceProperty: PropertyName (" << fullPropertyName.c_str() << ") could not be parsed.");
            return false;
        }

        // Grab the config association: 
        auto expressionIter = mPropertyConfig.getDerivedProperties().find(propertyName);
        if (expressionIter == mPropertyConfig.getDerivedProperties().end())
        {
            // ERR_LOG("[PropertyManager].lookupDerivedProperty: Unknown/unmapped property (" << fullPropertyName << ").  Check config for missing values.");
            return false;
        }

        // Lookup the expression in the (temp) derived function mapping:
        auto funcIter = mDerivedFunctionMap.find(expressionIter->second.c_str());
        if (funcIter == mDerivedFunctionMap.end())
        {
            // ERR_LOG("[PropertyManager].lookupDerivedProperty: Unknown/unmapped property (" << fullPropertyName << ").  Check config for missing values.");
            return false;
        }

        // Run the function:
        if ((*funcIter->second)(dataSources, outProp))
        {
            TRACE_LOG("[PropertyManager].lookupDerivedProperty: extracted property(" << fullPropertyName << ") from derived function (" << expressionIter->second.c_str() << ")");
            return true;
        }
        else
        {
            // PACKER_TODO:  This log is expected when the lookup fails because the member doesn't exist (yet).  We don't have a way to indicate that there was a parsing error, though.
            //   The function is used in two places, in one it just wants to validate the type/data lookup is valid, in other places it's used to actually gather the missing values. 
            //   Therefore there should be two functions. 
            WARN_LOG("[PropertyManager].lookupDerivedProperty: misconfigured property or internal error: failed derived function (" << expressionIter->second.c_str() << ") for property(" << fullPropertyName << ")");
            return false;
        }
    }

// TEMP DERIVED PROPERTIES:
    bool getRecordVersion(const PropertyDataSources& dataSources, EA::TDF::GenericTdfType& outProp)
    {
        if (dataSources.mGameSession == nullptr)
            return false;

        // Alternative is to get this value from dataSources.mServerGameData  (via GameId gameId = dataSources.mServerGameData->getReplicatedGameData().getGameId();)
        outProp.set(dataSources.mGameSession->getGameRecordVersion());
        return true;
    }

    bool getTeamCount(const PropertyDataSources& dataSources, EA::TDF::GenericTdfType& outProp)
    {
        if (dataSources.mGameSession != nullptr)
        {
            // This data is also accessible via the mServerGameData.  Using mGameSession for simpler functions. 
            outProp.set(dataSources.mGameSession->getTeamCount());
            return true;
        }
        else if (dataSources.mGameCreationData != nullptr)
        {
            outProp.set((uint16_t)(dataSources.mGameCreationData->getTeamIds().empty() ? 1 : dataSources.mGameCreationData->getTeamIds().size()));
            return true;
        }

        return false;
    }

    bool getTeamCapacity(const PropertyDataSources& dataSources, EA::TDF::GenericTdfType& outProp)
    {
        if (dataSources.mGameSession != nullptr)
        {
            // This data is also accessible via the mServerGameData.  Using mGameSession for simpler functions. 
            outProp.set(dataSources.mGameSession->getTeamCapacity());
            return true;
        }
        else if (dataSources.mGameCreationData != nullptr)
        {
            uint16_t teamCount = (dataSources.mGameCreationData->getTeamIds().empty() ? 1 : dataSources.mGameCreationData->getTeamIds().size());
            uint16_t participantCapacity = GameManager::getSlotCapacity(dataSources.mGameCreationData->getSlotCapacitiesMap());
            outProp.set((uint16_t)(participantCapacity / teamCount));
            return true;
        }

        return false;
    }

    bool getParticipantCapacity(const PropertyDataSources& dataSources, EA::TDF::GenericTdfType& outProp)
    {
        if (dataSources.mGameSession != nullptr)
        {
            // This data is also accessible via the mServerGameData.  Using mGameSession for simpler functions. 
            outProp.set(dataSources.mGameSession->getTotalParticipantCapacity());
            return true;
        }
        else if (dataSources.mGameCreationData != nullptr)
        {
            outProp.set(GameManager::getSlotCapacity(dataSources.mGameCreationData->getSlotCapacitiesMap()));
            return true;
        }

        return false;
    }

    bool getGamePercentFull(const PropertyDataSources& dataSources, EA::TDF::GenericTdfType& outProp)
    {
        if (dataSources.mGameSession != nullptr && dataSources.mPlayerRoster != nullptr)
        {
            // Data from multiple sources - derived property required
            outProp.set((100 * dataSources.mPlayerRoster->getParticipantCount()) / dataSources.mGameSession->getTotalParticipantCapacity());
            return true;
        }

        return false;
    }

    bool getQualityFactorHash(const PropertyDataSources& dataSources, EA::TDF::GenericTdfType& outProp)
    {
        // The actual Hash Logic is in GamePackerMasterImpl::initializeAggregateProperties, and requires access to the currently used Properties, Attributes, and GameTemplate.
        // Implementing this generically would have a chicken-egg problem, due to that dependency, but may still be possible if required. 

        int32_t invalidHash = 0;
        outProp.set(invalidHash);
        return true;
    }

    bool getGameStateOpenToMatchmaking(const PropertyDataSources& dataSources, EA::TDF::GenericTdfType& outProp)
    {
        if (dataSources.mGameSession != nullptr)
        {
            bool stateOpenToMatchmaking = false;
            bool isGameFull = dataSources.mGameSession->getPlayerRoster()->getParticipantCount() == dataSources.mGameSession->getTotalParticipantCapacity(); // full games are *never* joinable via MM, irrespective of their other settings

            if (!isGameFull && dataSources.mGameSession->getGameSettings().getOpenToMatchmaking())
            {
                if (dataSources.mGameSession->getGameState() == PRE_GAME)
                    stateOpenToMatchmaking = true;
                else if (dataSources.mGameSession->getGameState() == IN_GAME && dataSources.mGameSession->getGameSettings().getJoinInProgressSupported())
                    stateOpenToMatchmaking = true;
            }
            outProp.set(stateOpenToMatchmaking);
            return true;
        }
        else if (dataSources.mGameCreationData != nullptr)
        {
            // If we're creating a game, we assume it's open to MM.
            bool stateOpenToMatchmaking = true;
            outProp.set(stateOpenToMatchmaking);
            return true;
        }

        return false;
    }

    bool getGameModeAttribute(const PropertyDataSources& dataSources, EA::TDF::GenericTdfType& outProp)
    {
        // Have to get the attribute name from the config somewhere.
        if (dataSources.mGameSession != nullptr)
        {
            outProp.set(dataSources.mGameSession->getGameMode());
            return true;
        }
        else if (dataSources.mGameCreationData != nullptr)
        {
            // getConfig().getGameSession().getGameModeAttributeName()
            // dataSources.mGameCreationData->getGameAttribs()->find();
            // return true;
        }

        return false;
    }

    bool getGameIpAddress(const PropertyDataSources& dataSources, EA::TDF::GenericTdfType& outProp)
    {
        if (dataSources.mGameSession != nullptr)
        {
            uint16_t port = 0;
            uint32_t ipAddress = 0;
            if (dataSources.mGameSession->getTopologyHostNetworkAddressList().begin() != dataSources.mGameSession->getTopologyHostNetworkAddressList().end())
                PropertyManager::getPortAndIpValue(port, ipAddress, *dataSources.mGameSession->getTopologyHostNetworkAddressList().front());

            outProp.set(ipAddress);
            return true;
        }

        return false;
    }
    bool getGameIpPort(const PropertyDataSources& dataSources, EA::TDF::GenericTdfType& outProp)
    {
        // getTopologyHostNetworkAddressList is accessible from the TDF, but we still need custom logic to convert the address based on the union values used:
        if (dataSources.mGameSession != nullptr)
        {
            uint16_t port = 0;
            uint32_t ipAddress = 0;
            if (dataSources.mGameSession->getTopologyHostNetworkAddressList().begin() != dataSources.mGameSession->getTopologyHostNetworkAddressList().end())
                PropertyManager::getPortAndIpValue(port, ipAddress, *dataSources.mGameSession->getTopologyHostNetworkAddressList().front());

            outProp.set(port);
            return true;
        }

        return false;
    }

    bool getCallerIpAddress(const PropertyDataSources& dataSources, EA::TDF::GenericTdfType& outProp)
    {
        if (dataSources.mCallerInfo != nullptr)
        {
            uint16_t port = 0;
            uint32_t ipAddress = 0;
            PropertyManager::getPortAndIpValue(port, ipAddress, dataSources.mCallerInfo->getNetworkAddress());
            outProp.set(ipAddress);
            return true;
        }

        return false;
    }
    bool getCallerIpPort(const PropertyDataSources& dataSources, EA::TDF::GenericTdfType& outProp)
    {
        // getTopologyHostNetworkAddressList is accessible from the TDF, but we still need custom logic to convert the address based on the union values used:
        if (dataSources.mCallerInfo != nullptr)
        {
            uint16_t port = 0;
            uint32_t ipAddress = 0;
            PropertyManager::getPortAndIpValue(port, ipAddress, dataSources.mCallerInfo->getNetworkAddress());
            outProp.set(port);
            return true;
        }

        return false;
    }

    bool getLeaderBestPingSite(const PropertyDataSources& dataSources, EA::TDF::GenericTdfType& outProp)
    {
        if (dataSources.mCallerInfo != nullptr)
        {
            // The pingsite, and geoloc derived properties can be removed once a way to indicate 'players.' vs. 'caller.' UserSessionInfo data.
            outProp.set(dataSources.mCallerInfo->getBestPingSiteAlias());
            return true;
        }
        return false;
    }
    bool getCallerLatitude(const PropertyDataSources& dataSources, EA::TDF::GenericTdfType& outProp)
    {
        if (dataSources.mCallerInfo != nullptr)
        {
            // The pingsite, and geoloc derived properties can be removed once a way to indicate 'players.' vs. 'caller.' UserSessionInfo data.
            outProp.set((int32_t)dataSources.mCallerInfo->getLatitude());
            return true;
        }
        return false;
    }
    bool getCallerLongitude(const PropertyDataSources& dataSources, EA::TDF::GenericTdfType& outProp)
    {
        if (dataSources.mCallerInfo != nullptr)
        {
            // The pingsite, and geoloc derived properties can be removed once a way to indicate 'players.' vs. 'caller.' UserSessionInfo data.
            outProp.set((int32_t)dataSources.mCallerInfo->getLongitude());
            return true;
        }
        return false;
    }

    bool getParticipantCount(const PropertyDataSources& dataSources, EA::TDF::GenericTdfType& outProp)
    {
        uint32_t count = dataSources.mPlayersInfoList.size();
        outProp.set(count);
        return true;
    }

    bool PropertyManager::initDerivedPropertyExpressions()
    {
        // This system does not use the Blaze Expression system because the expression system is designed to convert values into known types.
        // For Derived Properties, we do not know what the type will be until we do the lookup, and we may need to store it as that type. 
        //   (Generally, the value will be a int64/float/uint64/string or list<>)
        // To make this code easier to work with, we just map functions directly based on their names.  
        // This'll work until we get something in that does more complicated logic. 
#define ADD_DERIVED_FUNCTION(func) mDerivedFunctionMap[#func "()"] = func;
        ADD_DERIVED_FUNCTION(getRecordVersion);
        ADD_DERIVED_FUNCTION(getTeamCount);
        ADD_DERIVED_FUNCTION(getTeamCapacity);
        ADD_DERIVED_FUNCTION(getParticipantCapacity);
        ADD_DERIVED_FUNCTION(getGamePercentFull);
        ADD_DERIVED_FUNCTION(getQualityFactorHash);
        ADD_DERIVED_FUNCTION(getGameStateOpenToMatchmaking);
        ADD_DERIVED_FUNCTION(getGameModeAttribute);
        ADD_DERIVED_FUNCTION(getGameIpAddress);
        ADD_DERIVED_FUNCTION(getGameIpPort);
        ADD_DERIVED_FUNCTION(getCallerIpAddress);
        ADD_DERIVED_FUNCTION(getCallerIpPort);
        ADD_DERIVED_FUNCTION(getLeaderBestPingSite);
        ADD_DERIVED_FUNCTION(getCallerLatitude);
        ADD_DERIVED_FUNCTION(getCallerLongitude);
        ADD_DERIVED_FUNCTION(getParticipantCount);
#undef ADD_DERIVED_FUNCTION

        return true;
    }
    
    bool PropertyManager::validatePropertyList(const PropertyConfig& config, const PropertyNameList& propertiesUsed, const char8_t** outMissingProperty) const
    {
        for (const auto& fullPropertyName : propertiesUsed)
        {
            if (outMissingProperty != nullptr)
                *outMissingProperty = fullPropertyName.c_str();

            PropertyName propertyName, propertyFieldName;
            if (!parsePropertyName(fullPropertyName, propertyName, propertyFieldName))
            {
                // Unable to parse the property name:
                return false;
            }

            // Check Data Sources:
            bool foundInDataSources = false;
            for (const auto& dataSource : config.getTdfDataSources())
            {
                if (dataSource.second->find(propertyName) != dataSource.second->end())
                {
                    // Data source exists
                    foundInDataSources = true;
                    continue;
                }
            }
            if (foundInDataSources)
                continue;

            // Check Derived Properties:
            if (config.getDerivedProperties().find(propertyName) != config.getDerivedProperties().end())
            {
                // Found in derived Properties  (hard to check for map/list, so we don't)
                continue;
            }
            
            // Need to do something for the other ones that aren't from a direct data source -
            // Ex.  CreateGameTemplateName is set per-subsession, so it's a value that we just set directly in the Scenario system. 
            // There are ways to handle that, maybe just add it as an explicit data source, so that we don't have to have these weird one-offs?
            // (i.e.  Make a dumb TDF that holds all the random values that we want to pass around that aren't part of the regular data types?)
            return false;
        }

        return true;
    }
    

    bool PropertyManager::convertProperty(const PropertyDataSources& dataSources, const PropertyName& fullPropertyName, EA::TDF::GenericTdfType &outProp) const
    {
        // 2.  Check if the property exists already in the sanitized list:
        auto itr = dataSources.mSanitizedProperties.find(fullPropertyName);
        if (itr != dataSources.mSanitizedProperties.end() && itr->second->isValid())
        {
            outProp.set(itr->second->get());
            return true;
        }


        // 3. Attempt to find the value in one of the TDFs that we have tracked:
        if (lookupTdfDataSourceProperty(dataSources.mMatchmakingRequest, fullPropertyName, outProp, false))
            return true;

        if (lookupTdfDataSourceProperty(dataSources.mGameCreationData, fullPropertyName, outProp, false))
            return true;

        if (lookupTdfDataSourceProperty(dataSources.mServerGameData, fullPropertyName, outProp, false))
            return true;

        // "players" properties are lists, so they need to be constructed as such
        bool foundList = false;
        for (const auto& userSessionInfo : dataSources.mPlayersInfoList)  // UserSessionInfo
        {
            if (lookupTdfDataSourceProperty(userSessionInfo, fullPropertyName, outProp, true))
                foundList = true;
        }
        if (foundList)
            return true;

        if (dataSources.mPlayerRoster)  // ReplicatedGamePlayerServer
        {
            PlayerRoster::PlayerInfoList playerList;
            dataSources.mPlayerRoster->getPlayers(playerList, PlayerRoster::ROSTER_PLAYERS);
            for (const auto& playerInfo : playerList)
            {
                if (lookupTdfDataSourceProperty(playerInfo->getPlayerData(), fullPropertyName, outProp, true))
                    foundList = true;
            }
            if (foundList)
                return true;
        }

        //for (const auto& playerJoinData : dataSources.mPlayerJoinDataList)  // PerPlayerJoinData
        //{
        //    if (lookupTdfDataSourceProperty(playerJoinData, fullPropertyName, outProp, true))
        //        foundList = true;
        //}
        //if (foundList)
        //    return true;


        // 4. Attempt to find via derived property:
        if (lookupDerivedProperty(dataSources, fullPropertyName, outProp))
        {
            return true;   // Success!
        }


        // 5.  Older lookup methods:  (PACKER_TODO - Remove)

        // Lookup the value in the original data format: 
        EA::TDF::TdfGenericReference tdfRef;
        const EA::TDF::TypeDescriptionBitfieldMember* outBfMember = nullptr;
        if (dataSources.validateAndFetchProperty(fullPropertyName, tdfRef, &outBfMember, false))
        {
            uint32_t tempInt = 0;
            if (outBfMember != nullptr)
            {
                // If the input was a bitfield member, we don't have any way to track that, so rather than referencing the bf member, we just convert it to an int and pass that around.
                tdfRef.asBitfield().getValueByDesc(outBfMember, tempInt);

                EA::TDF::TdfGenericReference tdfRefInt(tempInt);
                tdfRef = tdfRefInt;
            }
            // Convert the value to the simpler format (based on what is used):  
            outProp.set(tdfRef);
            return true;
        }


        return false;
    }

    bool PropertyManager::convertProperties(const PropertyDataSources& dataSources, const PropertyNameList& propertiesUsed, PropertyNameMap& outProperties, const char8_t** outMissingProperty) const
    {
        bool foundAllValues = true;
        // for each property that we're using, use the old lookup function, and then convert it to a Generic. 
        for (auto& fullPropertyName : propertiesUsed)
        {
            if (fullPropertyName.empty())
                continue;

            // 1.  Allocate space for the property that we're outputting:
            bool allocatedGeneric = false;
            auto& prop = outProperties[fullPropertyName];
            if (!prop)
            {
                allocatedGeneric = true;
                prop = outProperties.allocate_element();
            }

            // Attempt to convert (gather) the property from the data sources:
            if (!convertProperty(dataSources, fullPropertyName, *prop))
            {
                // 6.  If we get here, the property was missing from the data sources, so we have to remove it from the lists:
                if (allocatedGeneric)
                    outProperties.erase(fullPropertyName);
                foundAllValues = false;
                if (outMissingProperty != nullptr)
                    *outMissingProperty = fullPropertyName.c_str();
            }
        }

        return foundAllValues;
    }




    // --- Common fixed property accessors
    PropertyDataSources::PropertyDataSources()
    {
        clear();
    }

    void PropertyDataSources::clear()
    {
        // PACKER_TODO: Make sure this clears all values that are stored.
        mPropertyMap.clear();
        mCallerInfo = nullptr;
        mPlayersInfoList.clear();
        mServerGameData = nullptr;
        mGameSession = nullptr;
        mPlayerRoster = nullptr;
        mSanitizedProperties.clear();
        mGameCreationData = nullptr;
        mMatchmakingRequest = nullptr;

        TeamProperties emptyTeamProps;
        emptyTeamProps.copyInto(mTeamProperties);
    }

    void PropertyDataSources::addPlayerRosterSourceData(const PlayerRoster* playerRoster)
    {
        mPlayerRoster = playerRoster;

        // member list & player count
        mPlayersInfoList.clear();
        if (mPlayerRoster != nullptr)
        {
            PlayerRoster::PlayerInfoList playerList;
            mPlayerRoster->getPlayers(playerList, PlayerRoster::ROSTER_PARTICIPANTS);
            for (auto& playerInfo : playerList)
            {
                playerInfo->getUserInfo().copyInto(*mPlayersInfoList.pull_back());
            }
        }
    }

    // --- Caller property accessors
    CallerProperties::CallerProperties(bool setDefaults /*= false*/) 
    {
        if (setDefaults)
        {
            dedicatedServerOverrideGameId();
        }
    }

    Blaze::GameManager::GameId& CallerProperties::dedicatedServerOverrideGameId()
    {
        auto& propValue = insertAndReturn(PROPERTY_NAME_DEDICATED_SERVER_OVERRIDE_GAME_ID);
        if (!propValue.isValid())
            propValue.set(GameId(0));
        return *reinterpret_cast<GameId*>(propValue.asAny());
    }

    void PropertyDataSources::addPlayersSourceData(const UserJoinInfoList& userJoinInfo, PlayerJoinData* pjd)
    {
        // Fill in the caller/players data sources:
        for (auto& userInfo : userJoinInfo)
        {
            userInfo->getUser().copyInto(*mPlayersInfoList.pull_back());
            if (userInfo->getIsHost())
            {
                mCallerInfo = mPlayersInfoList.back();      // Use the value we just added/copied.
            }
        }
    }

    bool PropertyManager::translateCgrAttributeToPropertyName(const char8_t* attributeString, eastl::string& propertyString) const
    {
        propertyString = attributeString;
        for (auto itr = mCgRequestPropertyNameMapping.begin(); itr != mCgRequestPropertyNameMapping.end(); ++itr)
            {
            auto pos = propertyString.find(itr->first.c_str());
            if (pos != eastl::string::npos) {
                propertyString.replace(pos, itr->first.size(), itr->second.c_str());
                return true;
            }
        }
        return false;
    }

    void PropertyManager::configGamePropertiesPropertyNameList(const PackerConfig& packerConfig, PropertyNameList& propertyList)
    {
        eastl::string propertyDescriptorName;
        for (const auto& qualityFactorCfg : packerConfig.getQualityFactors())
        {
            if (configGamePropertyNameToFactorInputPropertyName(qualityFactorCfg->getGameProperty(), propertyDescriptorName))
            {
                // Copy of logic from  configGamePropertyNameToFactorInputPropertyName
                eastl::string expectedPropertyName = GameManager::PLAYERS_PROPERTY_PREFIX;
                expectedPropertyName += propertyDescriptorName;
                propertyList.insertAsSet(expectedPropertyName.c_str());
            }

            // Add in any properties required for the other fields:
            if (!qualityFactorCfg->getKeys().getPropertyNameAsTdfString().empty())
                propertyList.insertAsSet(qualityFactorCfg->getKeys().getPropertyName());

            if (!qualityFactorCfg->getScoringMap().getPropertyNameAsTdfString().empty())
                propertyList.insertAsSet(qualityFactorCfg->getScoringMap().getPropertyName());

            if (!qualityFactorCfg->getScoreShaper().empty())
            {
                for (auto& curMapping : *qualityFactorCfg->getScoreShaper().begin()->second)
                {
                    if (!curMapping.second->getPropertyNameAsTdfString().empty())
                        propertyList.insertAsSet(curMapping.second->getPropertyName());
                }
            }
        }
    }

    bool PropertyManager::configGamePropertyNameToFactorInputPropertyName(const char8_t* gamePropertyName, eastl::string& propertyDescriptorName)
    {
        eastl::string output;
        auto* start = gamePropertyName;
        auto* found = strrchr(gamePropertyName, '.');
        if (found != nullptr)
            start = found + 1;

        // PACKER_TODO: Currently only 3 player input property types supported!

        if (blaze_strnicmp(start, GameManager::PROPERTY_NAME_PING_SITE_LATENCIES, strlen(GameManager::PROPERTY_NAME_PING_SITE_LATENCIES)) == 0 ||
            blaze_strnicmp(start, GameManager::PROPERTY_NAME_UED_VALUES, strlen(GameManager::PROPERTY_NAME_UED_VALUES)) == 0 ||
            blaze_strnicmp(start, GameManager::PROPERTY_NAME_GAME_ATTRIBUTES, strlen(GameManager::PROPERTY_NAME_GAME_ATTRIBUTES)) == 0)
        {
            // eg: game.players.pingSiteLatencies[]
            // eg: game.teams.players.uedValues[]
            // eg: game.players.uedValues[]
            // eg: game.attributes[]
            propertyDescriptorName = start;
            return true;
        }

        return false;
    }

} // namespace GameManager
} // namespace Blaze
