/*! ************************************************************************************************/
/*!
    \file   inputsanitizer.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/inputsanitizer.h"
#include "gamemanager/templateattributes.h"
#include "gamemanager/gamemanagerslaveimpl.h"



namespace Blaze
{

// Move this to Framework if Templates are used by other systems:
namespace GameManager
{

    bool inputSanitizerConfigValidation(const SanitizerDefinitionMap& sanitizersConfig, ConfigureValidationErrors& validationErrors)
    {
        // Load the config files and figure out what to do. 
        AttributeToTypeDescMap attrToTypeDesc;

        TdfIdHashSet acceptableTdfs = { IntSanitizer::TDF_ID,
                                        UIntSanitizer::TDF_ID,
                                        FloatSanitizer::TDF_ID,
                                        StringSanitizer::TDF_ID,
                                        ListIntSanitizer::TDF_ID,
                                        ListUIntSanitizer::TDF_ID,
                                        ListFloatSanitizer::TDF_ID,
                                        ListStringSanitizer::TDF_ID,
                                        MapIntIntSanitizer::TDF_ID,
                                        MapIntUIntSanitizer::TDF_ID,
                                        MapIntFloatSanitizer::TDF_ID,
                                        MapIntStringSanitizer::TDF_ID,
                                        MapUIntIntSanitizer::TDF_ID,
                                        MapUIntUIntSanitizer::TDF_ID,
                                        MapUIntFloatSanitizer::TDF_ID,
                                        MapUIntStringSanitizer::TDF_ID,
                                        MapStringIntSanitizer::TDF_ID,
                                        MapStringUIntSanitizer::TDF_ID,
                                        MapStringFloatSanitizer::TDF_ID,
                                        MapStringStringSanitizer::TDF_ID,
                                        PingSiteSanitizer::TDF_ID };

        for (const auto& sanitizer : sanitizersConfig)
        {
            // Verify that all of the data types actually exist.
            // This is required because the config decoder now has logic to load generics into default data types (string, list<string>).
            const SanitizerName& sanitizerName = sanitizer.first;

            // For each mapped element, we do the same thing that the old code did:
            StringBuilder errorPrefix;
            errorPrefix << "[GameManagerSlaveImpl].inputSanitizerConfigValidation: Sanitizer: " << sanitizerName.c_str() << " ";
            validateTemplateTdfAttributeMapping(sanitizer.second->getSanitizer(), attrToTypeDesc, validationErrors, errorPrefix.get(), acceptableTdfs);
        }



        return (validationErrors.getErrorMessages().size() == 0);
    }

    template <typename T, typename T2>
    SanitizerFailureOperation runSanitizer(const T& sanitizer, const T2& input, T2& output)
    {
        if (input < sanitizer.getMin() || input > sanitizer.getMax())
        {
            // failed test - now do fail op:
            switch (sanitizer.getFailureOp())
            {
            case CLAMP_VALUE:    output = (input < sanitizer.getMin()) ? sanitizer.getMin() : sanitizer.getMax();  break;
            case FAIL_INPUT:     break;
            case USE_DEFAULT:    output = sanitizer.getDefault();   break;  // Problem - At this point, the default is not accessible.  It was part of the Config, but we're in the structure instead. 
            case CHOOSE_RANDOM:  break; // Not Applicable.  Could get a random number I guess?
            case REMOVE_ENTRY:   break; // Applicable to the Map itself, but not here.  Need to indicate that a removal should occur, rather than a 100% failure. 
            case NO_FAILURE:     output = input;  break;
            }

            return sanitizer.getFailureOp();
        }

        output = input;
        return NO_FAILURE;
    }

    template <typename T, typename T2>
    bool sanitizerChooseRandom(const T& sanitizer, T2& output)
    {
        return false; 
    }
    bool sanitizerChooseRandom(const StringSanitizer& sanitizer, EA::TDF::TdfString& output)
    {
        if (sanitizer.getAllowList().empty())
            return false;
        output = sanitizer.getAllowList()[Random::getRandomNumber(sanitizer.getAllowList().size())];
        return true;
    }


    SanitizerFailureOperation runSanitizer(const StringSanitizer& sanitizer, const EA::TDF::TdfString& input, EA::TDF::TdfString& output)
    {
        // It's a list, so we have 2 options: 
        bool blockFailure = false;
        if (!sanitizer.getAllowList().empty())
        {
            blockFailure = blockFailure || (sanitizer.getAllowList().findAsSet(input) == sanitizer.getAllowList().end());
        }
        if (!sanitizer.getBlockList().empty())
        {
            blockFailure = blockFailure || (sanitizer.getBlockList().findAsSet(input) != sanitizer.getBlockList().end());
        }

        bool lengthFailure = false;
        if (sanitizer.getMaxLength() != 0 && input.length() > sanitizer.getMaxLength())
            lengthFailure = true;

        if (blockFailure || lengthFailure)
        {
            // failed test - now do fail op:
            switch (sanitizer.getFailureOp())
            {
            case FAIL_INPUT:     break;
            case CLAMP_VALUE:
                if (lengthFailure && !blockFailure)
                    output.set(input, (EA::TDF::TdfStringLength)sanitizer.getMaxLength());
                break;
            case USE_DEFAULT:    output = sanitizer.getDefault();   break;
            case CHOOSE_RANDOM:
                if (!sanitizerChooseRandom(sanitizer, output))
                    return FAIL_INPUT;
                break;
            case REMOVE_ENTRY:   break; // Applicable to the Map itself, but not here.  Need to indicate that a removal should occur, rather than a 100% failure. 
            case NO_FAILURE:     output = sanitizer.getInput();  break;
            }

            return sanitizer.getFailureOp();
        }

        output = input;
        return NO_FAILURE;
    }
    SanitizerFailureOperation runSanitizer(const StringSanitizer& sanitizer, const char8_t* input, EA::TDF::TdfString& output)
    {
        EA::TDF::TdfString temp(input);
        return runSanitizer(sanitizer, temp, output);
    }


    template <typename T, typename T2>
    void updateAttrProp(const T& sanitizer, const T2& output, TemplateAttributes& attributes, PropertyNameMap& properties)
    {
        // We ALWAYS send the value to the output property/attribute, because that's where it has to go.
        if (!sanitizer.mSanitizerConfig->getOutAttrAsTdfString().empty())
        {
            auto curAttr = attributes.find(sanitizer.mSanitizerConfig->getOutAttr());
            if (curAttr == attributes.end())
            {
                attributes[sanitizer.mSanitizerConfig->getOutAttr()] = attributes.allocate_element();
                curAttr = attributes.find(sanitizer.mSanitizerConfig->getOutAttr());
            }
            curAttr->second->set(output);
        }
        if (!sanitizer.mSanitizerConfig->getOutPropertyAsTdfString().empty())
        {
            auto curProp = properties.find(sanitizer.mSanitizerConfig->getOutProperty());
            if (curProp == properties.end())
            {
                properties[sanitizer.mSanitizerConfig->getOutProperty()] = properties.allocate_element();
                curProp = properties.find(sanitizer.mSanitizerConfig->getOutProperty());
            }
            curProp->second->set(output);
        }
    }


    struct InputSanitizerData
    {
        eastl::string mSanitizerName;

        // Pointer to the config definition for this sanitizer:
        SanitizerDefinitionPtr mSanitizerConfig;

        // Dynamic data built from Config:
        EA::TDF::TdfPtr mSanitizerData;
    };


    template <typename TdfClass, typename valueType>
    bool caseSimple(const InputSanitizerData& sanitizerData, TemplateAttributes& attributes, PropertyNameMap& properties)
    {
        TdfClass& sanitizer = (TdfClass&)*sanitizerData.mSanitizerData;
        valueType output = 0; 
        if (runSanitizer(sanitizer, sanitizer.getInput(), output) == FAIL_INPUT)
            return false;
            
        updateAttrProp(sanitizerData, output, attributes, properties);
        return true;
    }

    template <typename TdfClass, typename listType, typename valueType>
    bool caseList(const InputSanitizerData& sanitizerData, TemplateAttributes& attributes, PropertyNameMap& properties)
    {
        TdfClass& sanitizer = (TdfClass&)*sanitizerData.mSanitizerData;
        listType listOutput; 
        for (const auto& input : sanitizer.getInput())
        {
            valueType output = 0; 
            SanitizerFailureOperation failOp = runSanitizer(sanitizer.getValue(), input, output); 
            if (failOp == FAIL_INPUT)
                return false;
            if (failOp != REMOVE_ENTRY)
                listOutput.push_back(output); 
        }
        if (listOutput.empty())
        {
            valueType output = 0; 
            SanitizerFailureOperation failOp = sanitizer.getEmptyFailureOp(); 
            if (failOp == FAIL_INPUT)
                return false;
            if (failOp == NO_FAILURE)
                return true;
            if (failOp == USE_DEFAULT)
                listOutput.push_back(sanitizer.getValue().getDefault());
            if (failOp == CHOOSE_RANDOM)
            {
                sanitizerChooseRandom(sanitizer.getValue(), output);
                listOutput.push_back(output); 
            }
        }
        updateAttrProp(sanitizerData, listOutput, attributes, properties);
        return true;
    }

    template <typename TdfClass, typename mapType, typename keyType, typename valueType>
    bool caseMap(const InputSanitizerData& sanitizerData, TemplateAttributes& attributes, PropertyNameMap& properties)
    {
        TdfClass& sanitizer = (TdfClass&)*sanitizerData.mSanitizerData;
        mapType mapOutput; 
        for (const auto& input : sanitizer.getInput())
        {
            keyType keyOutput = 0; 
            SanitizerFailureOperation failOp = runSanitizer(sanitizer.getKey(), input.first, keyOutput); 
            if (failOp == FAIL_INPUT)
                return false;
            if (failOp != REMOVE_ENTRY)
            {
                valueType valueOutput = 0; 
                failOp = runSanitizer(sanitizer.getValue(), input.second, valueOutput); 
                if (failOp == FAIL_INPUT)
                    return false;
                if (failOp != REMOVE_ENTRY)
                    mapOutput[keyOutput] = valueOutput; 
            }
        }

        updateAttrProp(sanitizerData, mapOutput, attributes, properties);
        return true;
    }


    bool casePingsiteSanitizer(const InputSanitizerData& sanitizerData, TemplateAttributes& attributes, PropertyNameMap& properties)
    {
        PingSiteSanitizer& psSanitizer = (PingSiteSanitizer&)*sanitizerData.mSanitizerData;

        // Combine the various pingsite lists: 
        ListString acceptablePingsites;
        if (!psSanitizer.getHostedPingSites().empty())
        {
            psSanitizer.getHostedPingSites().copyInto(acceptablePingsites);
        }

        if (!psSanitizer.getRequestedPingSites().empty())
        {
            if (acceptablePingsites.empty())
                psSanitizer.getRequestedPingSites().copyInto(acceptablePingsites);
            else
            {
                // Intersect the two lists:  
                for (auto curSiteItr = acceptablePingsites.begin(); curSiteItr != acceptablePingsites.end(); )
                {
                    if (psSanitizer.getRequestedPingSites().findAsSet(*curSiteItr) == psSanitizer.getRequestedPingSites().end())
                        curSiteItr = acceptablePingsites.erase(curSiteItr);
                    else
                        ++curSiteItr;
                }

                if (acceptablePingsites.empty())
                {
                    ERR_LOG("applySanitizers: Requested pingsite list and hosted pingsite lists are non-intersecting.  Failing request.");
                    return false;
                }
            }
        }

        // This value could come from a Property if we want to (in the future):
        if (gUserSessionManager)
        {
            auto& qosPingsiteAliasMap = gUserSessionManager->getQosConfig().getPingSiteInfoByAliasMap();
            if (!qosPingsiteAliasMap.empty())
            {
                if (acceptablePingsites.empty())
                {
                    for (auto& pingSiteIter : gUserSessionManager->getQosConfig().getPingSiteInfoByAliasMap())
                    {
                        acceptablePingsites.push_back(pingSiteIter.first);
                    }
                }
                else
                {
                    // Intersect the two lists:  
                    for (auto curSiteItr = acceptablePingsites.begin(); curSiteItr != acceptablePingsites.end(); )
                    {
                        if (qosPingsiteAliasMap.find(*curSiteItr) == qosPingsiteAliasMap.end())
                            curSiteItr = acceptablePingsites.erase(curSiteItr);
                        else
                            ++curSiteItr;
                    }

                    // If the desired list is not in QoS - Use the full QoS list?  (This undermines the idea of an allowlist in the first place)
                    if (acceptablePingsites.empty())
                    {
                        ERR_LOG("applySanitizers: Requested pingsite lists and qos pingsite lists are non-intersecting.  Failing request.");
                        return false;
                    }
                }
            }
        }

        // After gathering the valid pingsites, we check the results of the maxLatencyCheck and merge that in. 
        ListString latencyCheckedPingsites;
        for (auto& curPingsiteLatency : psSanitizer.getMergedPingsiteLatencyMap())
        {
            if (curPingsiteLatency.second <= psSanitizer.getMaximumLatency())
                latencyCheckedPingsites.push_back(curPingsiteLatency.first);
        }

        if (!latencyCheckedPingsites.empty() && !acceptablePingsites.empty())
        {
            // Intersect the two lists:  
            for (auto curSiteItr = latencyCheckedPingsites.begin(); curSiteItr != latencyCheckedPingsites.end(); )
            {
                if (acceptablePingsites.findAsSet(*curSiteItr) == acceptablePingsites.end())
                    curSiteItr = latencyCheckedPingsites.erase(curSiteItr);
                else
                    ++curSiteItr;
            }

            if (latencyCheckedPingsites.empty())
            {
                ERR_LOG("applySanitizers: Pingsites that match latency are not in the acceptable list, using the full acceptable list instead.");
                acceptablePingsites.copyInto(latencyCheckedPingsites);
            }
        }
        else
        {
            if (acceptablePingsites.empty())
            {
                ERR_LOG("applySanitizers: Qos information missing from client and server.  Unable to handle.");
                return false;
            }

            TRACE_LOG("applySanitizers: All pingsites exceeded max latency, using the full acceptable list instead.");
            acceptablePingsites.copyInto(latencyCheckedPingsites);
        }

        updateAttrProp(sanitizerData, latencyCheckedPingsites, attributes, properties);
        return true;
    }


    BlazeRpcError applySanitizer(const InputSanitizerData& sanitizer, TemplateAttributes& attributes, PropertyNameMap& properties)
    {
        bool success = false;

        // We need to check what the TDF is pointing to, since that determines what packer logic to run:
        switch (sanitizer.mSanitizerData->getTdfId())
        {
        case IntSanitizer::TDF_ID:        success = caseSimple<IntSanitizer, int64_t>(sanitizer, attributes, properties);                break;
        case UIntSanitizer::TDF_ID:       success = caseSimple<UIntSanitizer, uint64_t>(sanitizer, attributes, properties);              break;
        case FloatSanitizer::TDF_ID:      success = caseSimple<FloatSanitizer, float>(sanitizer, attributes, properties);                break;
        case StringSanitizer::TDF_ID:     success = caseSimple<StringSanitizer, EA::TDF::TdfString>(sanitizer, attributes, properties);  break;

        case ListIntSanitizer::TDF_ID:        success = caseList<ListIntSanitizer, ListInt, int64_t>(sanitizer, attributes, properties);                  break;
        case ListUIntSanitizer::TDF_ID:       success = caseList<ListUIntSanitizer, ListUInt, uint64_t>(sanitizer, attributes, properties);               break;
        case ListFloatSanitizer::TDF_ID:      success = caseList<ListFloatSanitizer, ListFloat, float>(sanitizer, attributes, properties);                break;
        case ListStringSanitizer::TDF_ID:     success = caseList<ListStringSanitizer, ListString, EA::TDF::TdfString>(sanitizer, attributes, properties); break;

        case MapIntIntSanitizer::TDF_ID:    success = caseMap<MapIntIntSanitizer, MapIntInt, int64_t, int64_t>(sanitizer, attributes, properties);                    break;
        case MapIntUIntSanitizer::TDF_ID:   success = caseMap<MapIntUIntSanitizer, MapIntUInt, int64_t, uint64_t>(sanitizer, attributes, properties);                 break;
        case MapIntFloatSanitizer::TDF_ID:  success = caseMap<MapIntFloatSanitizer, MapIntFloat, int64_t, float>(sanitizer, attributes, properties);                  break;
        case MapIntStringSanitizer::TDF_ID: success = caseMap<MapIntStringSanitizer, MapIntString, int64_t, EA::TDF::TdfString>(sanitizer, attributes, properties);   break;
        case MapUIntIntSanitizer::TDF_ID:     success = caseMap<MapUIntIntSanitizer, MapUIntInt, uint64_t, int64_t>(sanitizer, attributes, properties);                   break;
        case MapUIntUIntSanitizer::TDF_ID:    success = caseMap<MapUIntUIntSanitizer, MapUIntUInt, uint64_t, uint64_t>(sanitizer, attributes, properties);                break;
        case MapUIntFloatSanitizer::TDF_ID:   success = caseMap<MapUIntFloatSanitizer, MapUIntFloat, uint64_t, float>(sanitizer, attributes, properties);                 break;
        case MapUIntStringSanitizer::TDF_ID:  success = caseMap<MapUIntStringSanitizer, MapUIntString, uint64_t, EA::TDF::TdfString>(sanitizer, attributes, properties);  break;
        case MapStringIntSanitizer::TDF_ID:     success = caseMap<MapStringIntSanitizer, MapStringInt, EA::TDF::TdfString, int64_t>(sanitizer, attributes, properties);                  break;
        case MapStringUIntSanitizer::TDF_ID:    success = caseMap<MapStringUIntSanitizer, MapStringUInt, EA::TDF::TdfString, uint64_t>(sanitizer, attributes, properties);               break;
        case MapStringFloatSanitizer::TDF_ID:   success = caseMap<MapStringFloatSanitizer, MapStringFloat, EA::TDF::TdfString, float>(sanitizer, attributes, properties);                break;
        case MapStringStringSanitizer::TDF_ID:  success = caseMap<MapStringStringSanitizer, MapStringString, EA::TDF::TdfString, EA::TDF::TdfString>(sanitizer, attributes, properties); break;

        case PingSiteSanitizer::TDF_ID:   success = casePingsiteSanitizer(sanitizer, attributes, properties); break;

        default:
            // Unknown tdf type used.  Should have failed config first.
            break;
        }

        if (!success)
        {
            ERR_LOG("applySanitizers: Failed sanitizer " << sanitizer.mSanitizerName << ".  Please check the values sent from the client.");
            return GAMEMANAGER_ERR_INPUT_SANITIZER_FAILURE;
        }

        return ERR_OK;
    }

    BlazeRpcError applySanitizers(const SanitizerDefinitionMap& sanitizersConfig, const SanitizerNameList& sanitizersToApply, TemplateAttributes& attributes, PropertyNameMap& properties, const char8_t*& outFailingSanitizer)
    {
        for (auto& curSanitizerName : sanitizersToApply)
        {
            InputSanitizerData curData;
            curData.mSanitizerName = curSanitizerName;
            outFailingSanitizer = curSanitizerName.c_str();

            const auto sanitizerConfig = sanitizersConfig.find(curSanitizerName);
            if (sanitizerConfig == sanitizersConfig.end())
            {
                ERR_LOG("applySanitizers: Input Sanitizer " << curSanitizerName << " no longer exists.");
                return ERR_SYSTEM;
            }
            curData.mSanitizerConfig = sanitizerConfig->second;

            const char8_t* failingAttr = nullptr;
            BlazeRpcError blazeErr = applyTemplateTdfAttributes(curData.mSanitizerData, curData.mSanitizerConfig->getSanitizer(), attributes, properties, failingAttr, "[InputSanitizer].applySanitizers: ");
            if (blazeErr != Blaze::ERR_OK)
            {
                ERR_LOG("applySanitizers: Input Sanitizer " << curSanitizerName << ", unable to apply value from config for the attribute " << failingAttr << ".");
                return blazeErr;
            }
            
            blazeErr = applySanitizer(curData, attributes, properties);
            if (blazeErr != Blaze::ERR_OK)
            {
                ERR_LOG("applySanitizers: Input Sanitizer " << curSanitizerName  << " failed.");
                return blazeErr;
            }
        }

        outFailingSanitizer = "";
        return ERR_OK;
    }

    BlazeRpcError getSanitizersPropertiesUsed(const SanitizerDefinitionMap& sanitizersConfig, const SanitizerNameList& sanitizersToApply, PropertyNameList& propertiesUsed)
    {
        for (auto& curSanitizerName : sanitizersToApply)
        {
            const auto sanitizerConfig = sanitizersConfig.find(curSanitizerName);
            if (sanitizerConfig == sanitizersConfig.end())
            {
                ERR_LOG("applySanitizers: Input Sanitizer " << curSanitizerName << " no longer exists.");
                return ERR_SYSTEM;
            }

            for (auto& tdfMapping : sanitizerConfig->second->getSanitizer())
            {
                for (auto& attrMapping : *tdfMapping.second)
                {
                    if (!attrMapping.second->getPropertyNameAsTdfString().empty())
                        propertiesUsed.insertAsSet(attrMapping.second->getPropertyName());
                }
            }
        }

        return ERR_OK;
    }

}
}
