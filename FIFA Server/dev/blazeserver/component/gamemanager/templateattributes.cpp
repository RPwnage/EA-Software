/*! ************************************************************************************************/
/*!
    \file   templateattributes.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/templateattributes.h"
#include "gamemanager/gamemanagerslaveimpl.h"



namespace Blaze
{

// Move this to Framework if Templates are used by other systems:
namespace GameManager
{
    void checkForAttrNameOverlap(eastl::map<eastl::string, const EA::TDF::TypeDescription*>& attrToTypeDesc, const char* attrName, const EA::TDF::TypeDescription* typeDesc, ConfigureValidationErrors& validationErrors) 
    {
        if (attrName == nullptr || attrName[0] == '\0')
        {
            return;
        }

        eastl::map<eastl::string, const EA::TDF::TypeDescription*>::const_iterator iter = attrToTypeDesc.find(attrName);
        if (iter == attrToTypeDesc.end())
        {
            attrToTypeDesc[attrName] = typeDesc;
        }
        else
        {
            if (!EA::TDF::TdfGenericConst::canTypesConvert(*iter->second, *typeDesc))
            {
                StringBuilder strBuilder;
                strBuilder << "checkForAttrNameOverlap: Mismatch found for attribute: " << attrName << 
                        ", (" << typeDesc->getFullName()  << " vs. " << iter->second->getFullName() << ".  Types cannot be converted between each other.";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
            }
        }
    }


    bool validateTemplateAttributeMapping(const TemplateAttributeMapping& attributes, AttributeToTypeDescMap& attrToTypeDesc, const EA::TDF::TdfGenericReference& requestRef, 
                                          ConfigureValidationErrors& validationErrors, const char8_t* errorStringPrefix, const char8_t* ruleName, bool nameIncludesTdf)
    {
        EA::TDF::TdfFactory& tdfFactory = EA::TDF::TdfFactory::get();

        // Verify all create game attributes:
        TemplateAttributeMapping::const_iterator curAttr = attributes.begin();
        TemplateAttributeMapping::const_iterator endAttr = attributes.end();
        for (; curAttr != endAttr; ++curAttr)
        {
            const char8_t* tdfMemberName =  "";

            if (ruleName == nullptr)
            {
                tdfMemberName = curAttr->first.c_str();
            }
            else
            {
                if (!tdfFactory.lookupTypeName(ruleName, curAttr->first.c_str(), tdfMemberName, true))
                {
                    // Unable to lookup mapped value:
                    StringBuilder strBuilder;
                    strBuilder << errorStringPrefix << 
                            ", rule: " << ruleName << ", attribute: " << curAttr->first.c_str() << ". Check if this rule/attribute combination is valid (see rule_attributes.cfg)." << 
                            " If this is a custom rule, make sure it was initialized with registerCustomMultiInputValues and registerMultiInputValuesHelper.";
                    validationErrors.getErrorMessages().push_back(strBuilder.get());
                }
            }

            if (!curAttr->second->getDefault().isValid() && 
                curAttr->second->getAttrNameAsTdfString().empty() &&
                curAttr->second->getPropertyNameAsTdfString().empty())
            {
                StringBuilder strBuilder;
                strBuilder << errorStringPrefix << 
                        "Attribute with member location: '"<< tdfMemberName <<
                        "' has not specified a default, attribute name, or property name. At least one of these values must be set.";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
            }

            // Check that the value can be parsed by the Tdf system: 
            EA::TDF::TdfGenericReference criteriaAttr;
            if (!tdfFactory.getTdfMemberReference(requestRef, tdfMemberName, criteriaAttr, nullptr, nameIncludesTdf))
            {
                StringBuilder strBuilder;
                strBuilder << errorStringPrefix << 
                        "Unable to get TdfMemberReference for attribute. Member location: '"<< tdfMemberName <<
                        "'.  Check if this value has changed its name or location. ";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
            }

            // Verify that the generic default is valid to map here: (This should already be valid, otherwise we couldn't parse the config)
            if (curAttr->second->getDefault().isValid() && 
                !(curAttr->second->getDefault().getValue().convertToResult(criteriaAttr)))
            {
                StringBuilder strBuilder;
                strBuilder << errorStringPrefix << 
                        "Unable to map default value for attribute: " << curAttr->second->getAttrName() << 
                        ", Member location: '"<< tdfMemberName << "'.  Check that the default value matches the internal type used.";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
            }

            // It may be valid to overlap, but you still have to have the same types:
            checkForAttrNameOverlap(attrToTypeDesc, curAttr->second->getAttrName(), &criteriaAttr.getTypeDescription(), validationErrors);
        }

        return (validationErrors.getErrorMessages().size() == 0);
    }

    bool validateTemplateTdfAttributeMapping(const TemplateAttributeTdfMapping& templateAttributeMapping, AttributeToTypeDescMap& attrToTypeDesc,
        ConfigureValidationErrors& validationErrors, const char8_t* errorStringPrefix, const TdfIdHashSet& acceptableTdfs)
    {
        EA::TDF::TdfFactory& tdfFactory = EA::TDF::TdfFactory::get();

        // First, generate the TDF that would be used, if it's not already set: 
        if (templateAttributeMapping.size() != 1)
        {
            StringBuilder strBuilder;
            strBuilder << errorStringPrefix << ".  Missing attribute type mapping, or multiple types defined for a single input (not currently supported).";
            validationErrors.getErrorMessages().push_back(strBuilder.get());
            return (validationErrors.getErrorMessages().size() == 0);
        }

        const EA::TDF::TypeDescription* typeDesc = tdfFactory.getTypeDesc(templateAttributeMapping.begin()->first);
        if (typeDesc == nullptr)
        {
            StringBuilder strBuilder;
            strBuilder << errorStringPrefix << ".  Attribute mapping uses unknown type name: " << templateAttributeMapping.begin()->first << ".  Make sure that the type is a TDF.";
            validationErrors.getErrorMessages().push_back(strBuilder.get());
            return (validationErrors.getErrorMessages().size() == 0);
        }

        if (!acceptableTdfs.empty() && acceptableTdfs.find(typeDesc->getTdfId()) == acceptableTdfs.end())
        {
            StringBuilder strBuilder;
            strBuilder << errorStringPrefix << ".  Attribute mapping uses unacceptable type name: " << typeDesc->getShortName() << ".  If desired, add this to the acceptableTdfs set.";
            validationErrors.getErrorMessages().push_back(strBuilder.get());
            return (validationErrors.getErrorMessages().size() == 0);
        }

        // Allocate a temp version of the TDF to use in the lookup:
        EA::TDF::TdfPtr requestTdfPtr = tdfFactory.create(typeDesc->getTdfId());
        EA::TDF::TdfGenericReference requestRef(requestTdfPtr);

        return validateTemplateAttributeMapping(*templateAttributeMapping.begin()->second, attrToTypeDesc, requestRef, validationErrors, errorStringPrefix, nullptr, false);
    }


    template< typename vecType, typename valueType >
    void applyMergeOp(const vecType& tempVector, MergeOp curMergeOp, valueType& outValue)
    {
        if (tempVector.vectorSize() == 0)
            return;

        switch (curMergeOp)
        {

        case MERGE_MIN:
        {
            outValue = tempVector[0];
            for (auto& value : tempVector)
                if (value < outValue)
                    outValue = value;
            break;
        }

        case MERGE_MAX:
        {
            outValue = tempVector[0];
            for (auto& value : tempVector)
                if (value > outValue)
                    outValue = value;
            break;
        }

        case MERGE_SUM:
        {
            outValue = 0;
            for (auto& value : tempVector)
                outValue += value;
            break;
        }

        case MERGE_SIZE:
        {
            outValue = (valueType)tempVector.size();
            break;
        }

        case MERGE_AVERAGE:
        {
            outValue = 0;
            for (auto& value : tempVector)
                outValue += value;
            outValue /= tempVector.vectorSize();
            break;
        }

        case MERGE_STDDEV:
        {
            valueType avgValue = 0;
            for (auto& value : tempVector)
                avgValue += value;
            avgValue /= tempVector.vectorSize();

            outValue = 0;
            for (auto& value : tempVector)
                outValue += (avgValue > value) ? (avgValue - value) : (value - avgValue);
            outValue /= tempVector.vectorSize();
            break;
        }

        case MERGE_MIN_MAX_RANGE:
        {
            valueType maxValue = tempVector[0];
            valueType minValue = tempVector[0];
            for (auto& value : tempVector)
            {
                if (value < minValue)
                    minValue = value;
                if (value > maxValue)
                    maxValue = value;
            }
            outValue = maxValue - minValue;
            break;
        }

        case MERGE_MIN_MAX_RATIO:
        {
            valueType maxValue = tempVector[0];
            valueType minValue = tempVector[0];
            for (auto& value : tempVector)
            {
                if (value < minValue)
                    minValue = value;
                if (value > maxValue)
                    maxValue = value;
            }
            outValue = (maxValue != 0) ? (minValue / maxValue) : 0;
            break;
        }

        default:
            // Unsupported merge op:
            break;
        }
    }

    template< typename vecType, typename valueType >
    void applyMergeOpMap(const vecType& tempVector, MergeOp curMergeOp, valueType& outValueMap)
    {
        if (tempVector.vectorSize() == 0)
            return;

        switch (curMergeOp)
        {

        case MERGE_MIN:
        {
            for (auto& mapValue : tempVector)
            {
                for (auto& value : *mapValue)
                {
                    auto outIter = outValueMap.find(value.first);
                    if (outIter == outValueMap.end())
                        outValueMap[value.first] = value.second;
                    else if (value.second < outIter->second)
                        outIter->second = value.second;
                }
            }
            break;
        }

        case MERGE_MAX:
        {
            for (auto& mapValue : tempVector)
            {
                for (auto& value : *mapValue)
                {
                    auto outIter = outValueMap.find(value.first);
                    if (outIter == outValueMap.end())
                        outValueMap[value.first] = value.second;
                    else if (value.second > outIter->second)
                        outIter->second = value.second;
                }
            }
            break;
        }

        case MERGE_SUM:
        {
            for (auto& mapValue : tempVector)
            {
                for (auto& value : *mapValue)
                {
                    auto outIter = outValueMap.find(value.first);
                    if (outIter == outValueMap.end())
                        outValueMap[value.first] = value.second;
                    else
                        outIter->second += value.second;
                }
            }
            break;
        }

        case MERGE_SIZE:
        {
            for (auto& mapValue : tempVector)
            {
                for (auto& value : *mapValue)
                {
                    auto outIter = outValueMap.find(value.first);
                    if (outIter == outValueMap.end())
                        outValueMap[value.first] = 1;
                    else
                        ++outIter->second;
                }
            }
            break;
        }

        case MERGE_AVERAGE:
        {
            valueType vectorSize;
            for (auto& mapValue : tempVector)
            {
                for (auto& value : *mapValue)
                {
                    auto outIter = outValueMap.find(value.first);
                    if (outIter == outValueMap.end())
                    {
                        outValueMap[value.first] = value.second;
                        vectorSize[value.first] = 1;
                    }
                    else
                    {
                        outIter->second += value.second;
                        ++vectorSize[value.first];
                    }
                }
            }
            for (auto& outValue : outValueMap)
                outValue.second /= vectorSize[outValue.first];
            break;
        }

        case MERGE_STDDEV:
        {
            valueType avgValueMap;
            valueType vectorSize;
            for (auto& mapValue : tempVector)
            {
                for (auto& value : *mapValue)
                {
                    auto avgIter = avgValueMap.find(value.first);
                    if (avgIter == avgValueMap.end())
                    {
                        avgValueMap[value.first] = value.second;
                        vectorSize[value.first] = 1;
                    }
                    else
                    {
                        avgIter->second += value.second;
                        ++vectorSize[value.first];
                    }

                }
            }
            for (auto& avgValue : avgValueMap)
                avgValue.second /= vectorSize[avgValue.first];

            for (auto& mapValue : tempVector)
            {
                for (auto& value : *mapValue)
                {
                    auto stddevValue = (avgValueMap[value.first] > value.second) ? (avgValueMap[value.first] - value.second) : (value.second - avgValueMap[value.first]);
                    auto outIter = outValueMap.find(value.first);
                    if (outIter == outValueMap.end())
                        outValueMap[value.first] = stddevValue;
                    else
                        outIter->second += stddevValue;
                }
            }
            for (auto& outValue : outValueMap)
                outValue.second /= vectorSize[outValue.first];
            break;
        }

        case MERGE_MIN_MAX_RANGE:
        {
            valueType minValueMap;
            valueType maxValueMap;
            for (auto& mapValue : tempVector)
            {
                for (auto& value : *mapValue)
                {
                    auto minIter = minValueMap.find(value.first);
                    auto maxIter = maxValueMap.find(value.first);
                    if (maxIter == maxValueMap.end())
                    {
                        minValueMap[value.first] = value.second;
                        maxValueMap[value.first] = value.second;
                    }
                    else
                    {
                        if (value.second < minIter->second)
                            minIter->second = value.second;
                        if (value.second > maxIter->second)
                            maxIter->second = value.second;
                    }
                }
            }
            for (auto minIter = minValueMap.begin(), maxIter = maxValueMap.begin(); 
                 minIter != minValueMap.end() && maxIter != maxValueMap.end(); 
                 ++minIter, ++maxIter)
            {
                outValueMap[minIter->first] = maxIter->second - minIter->second;
            }
            break;
        }

        case MERGE_MIN_MAX_RATIO:
        {
            valueType minValueMap;
            valueType maxValueMap;
            for (auto& mapValue : tempVector)
            {
                for (auto& value : *mapValue)
                {
                    auto minIter = minValueMap.find(value.first);
                    auto maxIter = maxValueMap.find(value.first);
                    if (maxIter == maxValueMap.end())
                    {
                        minValueMap[value.first] = value.second;
                        maxValueMap[value.first] = value.second;
                    }
                    else
                    {
                        if (value.second < minIter->second)
                            minIter->second = value.second;
                        if (value.second > maxIter->second)
                            maxIter->second = value.second;
                    }
                }
            }
            for (auto minIter = minValueMap.begin(), maxIter = maxValueMap.begin();
                minIter != minValueMap.end() && maxIter != maxValueMap.end();
                ++minIter, ++maxIter)
            {
                outValueMap[minIter->first] = (maxIter->second != 0) ? (minIter->second / maxIter->second) : 0;
            }
            break;
        }

        default:
            // Unsupported merge op:
            break;
        }
    }

    bool attemptMergeOp(const EA::TDF::TdfGenericConst& sourceValue, MergeOp curMergeOp, EA::TDF::GenericTdfType& mergedGeneric)
    {
        if (curMergeOp == MERGE_NONE)
            return false;

        // Basic List -> Value merges:
        if (sourceValue.getType() == EA::TDF::TDF_ACTUAL_TYPE_LIST)
        {
            const EA::TDF::TdfVectorBase& baseVec = sourceValue.asList(); // List for merges is expected to either be:  list<integral>, list<float>, etc. 

            if (baseVec.vectorSize() == 0)
            {
                TRACE_LOG("Source list provided was empty.  Skipping merge op attempt.");
                return false;
            }

            if (baseVec.getValueTypeDesc().isFloat())
            {
                auto tempVector = *(EA::TDF::TdfPrimitiveVector<float>*) &baseVec;
                applyMergeOp(tempVector, curMergeOp, mergedGeneric.setType<float>());
            }
            else if (baseVec.getValueTypeDesc().isIntegral())
            {
                if (baseVec.getValueTypeDesc().getTdfType() == EA::TDF::TDF_ACTUAL_TYPE_UINT64)
                {
                    auto tempVector = *(EA::TDF::TdfPrimitiveVector<uint64_t>*) &baseVec;
                    applyMergeOp(tempVector, curMergeOp, mergedGeneric.setType<uint64_t>());
                }
                else
                {
                    // Convert to an int64_t vector, and use that for the math. 
                    EA::TDF::TdfPrimitiveVector<int64_t> tempVec;
                    EA::TDF::TdfGenericReference tempRef(tempVec);
                    sourceValue.convertBetweenLists(tempRef);
                    applyMergeOp(tempVec, curMergeOp, mergedGeneric.setType<int64_t>());
                }
            }
            else if (baseVec.getValueTypeDesc().getTdfType() == EA::TDF::TDF_ACTUAL_TYPE_MAP)
            {
                // If we know this is a map, we need to get it and the values in the correct format, right?
                // Yes, but we actually only want to support the 12 combinations:  map<string/int64/uint64/float, int64/uint64/float>.
                // What's the 
                auto mapDesc = baseVec.getValueTypeDesc().asMapDescription();
                if (mapDesc->keyType.isFloat())
                {
                    if (mapDesc->valueType.isFloat())
                    {
                        auto tempVector = *(EA::TDF::TdfStructVector<EA::TDF::TdfPrimitiveMap<float, float> >*) &baseVec;
                        applyMergeOpMap(tempVector, curMergeOp, mergedGeneric.setType<EA::TDF::TdfPrimitiveMap<float, float> >());
                    }
                    else if (mapDesc->valueType.isIntegral())
                    {
                        if (mapDesc->valueType.getTdfType() == EA::TDF::TDF_ACTUAL_TYPE_UINT64)
                        {
                            auto tempVector = *(EA::TDF::TdfStructVector<EA::TDF::TdfPrimitiveMap<float, uint64_t> >*) &baseVec;
                            applyMergeOpMap(tempVector, curMergeOp, mergedGeneric.setType<EA::TDF::TdfPrimitiveMap<float, uint64_t> >());
                        }
                        else
                        {
                            EA::TDF::TdfStructVector<EA::TDF::TdfPrimitiveMap<float, int64_t> > tempVec;
                            EA::TDF::TdfGenericReference tempRef(tempVec);
                            sourceValue.convertBetweenLists(tempRef);
                            applyMergeOpMap(tempVec, curMergeOp, mergedGeneric.setType<EA::TDF::TdfPrimitiveMap<float, int64_t> >());
                        }
                    }
                    else
                    {
                        TRACE_LOG("Source list<map value> type provided (" << mapDesc->valueType.getFullName() << ") as input data cannot be merged.  Only integral, and float map values are supported currently.");
                        return false;
                    }
                }
                else if (mapDesc->keyType.isString())
                {
                    if (mapDesc->valueType.isFloat())
                    {
                        auto tempVector = *(EA::TDF::TdfStructVector<EA::TDF::TdfPrimitiveMap<EA::TDF::TdfString, float> >*) &baseVec;
                        applyMergeOpMap(tempVector, curMergeOp, mergedGeneric.setType<EA::TDF::TdfPrimitiveMap<EA::TDF::TdfString, float> >());
                    }
                    else if (mapDesc->valueType.isIntegral())
                    {
                        if (mapDesc->valueType.getTdfType() == EA::TDF::TDF_ACTUAL_TYPE_UINT64)
                        {
                            auto tempVector = *(EA::TDF::TdfStructVector<EA::TDF::TdfPrimitiveMap<EA::TDF::TdfString, uint64_t> >*) &baseVec;
                            applyMergeOpMap(tempVector, curMergeOp, mergedGeneric.setType<EA::TDF::TdfPrimitiveMap<EA::TDF::TdfString, uint64_t> >());
                        }
                        else
                        {
                            EA::TDF::TdfStructVector<EA::TDF::TdfPrimitiveMap<EA::TDF::TdfString, int64_t> > tempVec;
                            EA::TDF::TdfGenericReference tempRef(tempVec);
                            sourceValue.convertBetweenLists(tempRef);
                            applyMergeOpMap(tempVec, curMergeOp, mergedGeneric.setType<EA::TDF::TdfPrimitiveMap<EA::TDF::TdfString, int64_t> >());
                        }
                    }
                    else
                    {
                        TRACE_LOG("Source list<map value> type provided (" << mapDesc->valueType.getFullName() << ") as input data cannot be merged.  Only integral, and float map values are supported currently.");
                        return false;
                    }
                }
                else if (mapDesc->keyType.isIntegral())
                {
                    if (mapDesc->keyType.getTdfType() == EA::TDF::TDF_ACTUAL_TYPE_UINT64)
                    {
                        if (mapDesc->valueType.isFloat())
                        {
                            auto tempVector = *(EA::TDF::TdfStructVector<EA::TDF::TdfPrimitiveMap<uint64_t, float> >*) &baseVec;
                            applyMergeOpMap(tempVector, curMergeOp, mergedGeneric.setType<EA::TDF::TdfPrimitiveMap<uint64_t, float> >());
                        }
                        else if (mapDesc->valueType.isIntegral())
                        {
                            if (mapDesc->valueType.getTdfType() == EA::TDF::TDF_ACTUAL_TYPE_UINT64)
                            {
                                auto tempVector = *(EA::TDF::TdfStructVector<EA::TDF::TdfPrimitiveMap<uint64_t, uint64_t> >*) &baseVec;
                                applyMergeOpMap(tempVector, curMergeOp, mergedGeneric.setType<EA::TDF::TdfPrimitiveMap<uint64_t, uint64_t> >());
                            }
                            else
                            {
                                EA::TDF::TdfStructVector<EA::TDF::TdfPrimitiveMap<uint64_t, int64_t> > tempVec;
                                EA::TDF::TdfGenericReference tempRef(tempVec);
                                sourceValue.convertBetweenLists(tempRef);
                                applyMergeOpMap(tempVec, curMergeOp, mergedGeneric.setType<EA::TDF::TdfPrimitiveMap<uint64_t, int64_t> >());
                            }
                        }
                        else
                        {
                            TRACE_LOG("Source list<map value> type provided (" << mapDesc->valueType.getFullName() << ") as input data cannot be merged.  Only integral, and float map values are supported currently.");
                            return false;
                        }
                    }
                    else
                    {
                        if (mapDesc->valueType.isFloat())
                        {
                            EA::TDF::TdfStructVector<EA::TDF::TdfPrimitiveMap<int64_t, float> > tempVec;
                            EA::TDF::TdfGenericReference tempRef(tempVec);
                            sourceValue.convertBetweenLists(tempRef);
                            applyMergeOpMap(tempVec, curMergeOp, mergedGeneric.setType<EA::TDF::TdfPrimitiveMap<uint64_t, float> >());
                        }
                        else if (mapDesc->valueType.isIntegral())
                        {
                            if (mapDesc->valueType.getTdfType() == EA::TDF::TDF_ACTUAL_TYPE_UINT64)
                            {
                                EA::TDF::TdfStructVector<EA::TDF::TdfPrimitiveMap<int64_t, uint64_t> > tempVec;
                                EA::TDF::TdfGenericReference tempRef(tempVec);
                                sourceValue.convertBetweenLists(tempRef);
                                applyMergeOpMap(tempVec, curMergeOp, mergedGeneric.setType<EA::TDF::TdfPrimitiveMap<int64_t, uint64_t> >());
                            }
                            else
                            {
                                EA::TDF::TdfStructVector<EA::TDF::TdfPrimitiveMap<int64_t, int64_t> > tempVec;
                                EA::TDF::TdfGenericReference tempRef(tempVec);
                                sourceValue.convertBetweenLists(tempRef);
                                applyMergeOpMap(tempVec, curMergeOp, mergedGeneric.setType<EA::TDF::TdfPrimitiveMap<int64_t, int64_t> >());
                            }
                        }
                        else
                        {
                            TRACE_LOG("Source list<map value> type provided (" << mapDesc->valueType.getFullName() << ") as input data cannot be merged.  Only integral, and float map values are supported currently.");
                            return false;
                        }
                    }
                }
                else
                {
                TRACE_LOG("Source list<map key> type provided (" << mapDesc->keyType.getFullName() << ") as input data cannot be merged.  Only string, integral, and float map keys are supported currently.");
                    return false;
                }
            }
            else
            {
                TRACE_LOG("Source list value type provided (" << baseVec.getValueTypeDesc().getFullName() << ") as input data cannot be merged.  Only integral, float, and map lists are supported currently.");
                return false;
            }
        }
        else
        {
            // Unsupported type for merge operation.
            // Treat it like MERGE_NONE, and let the other type conversion try to handle it.  (For cases where a non-list gets sent up from the client I guess.)
            TRACE_LOG("Source type provided ("<< sourceValue.getFullName() << ") as input data cannot be merged.  Only lists are supported currently.");
            return false;
        }

        return true;
    }

    BlazeRpcError applyTemplateAttributes(EA::TDF::TdfGenericReference criteriaRef, const TemplateAttributeMapping& templateAttributeMapping, const TemplateAttributes& clientAttributes,
        const PropertyNameMap& properties, const char8_t*& outFailingAttribute, const char8_t* componentDescription, bool nameIncludesTdf, const PropertyManager* propertyManager)
    {
        BlazeRpcError err = ERR_OK;        

        TemplateAttributeDefinition tempDef;    // Temporary definition used by translateCgrAttributeToPropertyName

        EA::TDF::TdfFactory& tdfFactory = EA::TDF::TdfFactory::get();
        TemplateAttributeMapping::const_iterator curAttr = templateAttributeMapping.begin();
        TemplateAttributeMapping::const_iterator endAttr = templateAttributeMapping.end();
        for (; curAttr != endAttr; ++curAttr)
        {
            // Check for client required? Is that allowed?
            const EA::TDF::TypeDescriptionBitfieldMember* outBfMember = nullptr;
            EA::TDF::TdfGenericReference criteriaAttr;
            if (!tdfFactory.getTdfMemberReference(criteriaRef, curAttr->first, criteriaAttr, &outBfMember, nameIncludesTdf))
            {
                // We should've already checked for this.
                outFailingAttribute = curAttr->first.c_str();
                return GAMEMANAGER_ERR_INVALID_TEMPLATE_ATTRIBUTE;
            }

            // Apply property
            const TemplateAttributeDefinition* curConfig = curAttr->second;
            auto* attrName = curAttr->first.c_str();
            auto* propName = curConfig->getPropertyName();
            if (propertyManager && propName[0] == '\0')
            {
                // If the Attribute on the Config exists (and maps to a Property), we switch to using the PropertyName here:
                eastl::string propertyName;
                if (propertyManager->translateCgrAttributeToPropertyName(attrName, propertyName))
                {
                    curAttr->second->copyInto(tempDef);
                    tempDef.setPropertyName(propertyName.c_str());
                    curConfig = &tempDef;
                }
            }

            err = applyTemplateAttributeDefinition(criteriaAttr, *curConfig, clientAttributes, properties, outFailingAttribute, componentDescription, outBfMember);
            if (err != ERR_OK)
                break;
        }

        return err;
    }

    BlazeRpcError applyTemplateAttributeDefinition(EA::TDF::TdfGenericReference criteriaAttr, const TemplateAttributeDefinition& attrConfig, const TemplateAttributes& clientAttributes,
        const PropertyNameMap& properties, const char8_t*& outFailingAttribute, const char8_t* componentDescription, const EA::TDF::TypeDescriptionBitfieldMember* bfMember)
    {
        bool conversionFailedError = false;

        // Check if a property name is in use, and use that if possible:
        auto clientProp = properties.find(attrConfig.getPropertyName());
        if (clientProp != properties.end())
        {
            const EA::TDF::TdfGenericConst& clientPropValue = clientProp->second->getValue();
            if (attrConfig.getDebugOnly() && !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_START_DEBUG_MATCHMAKING_SCENARIO))
            {
                WARN_LOG(componentDescription << "Client attempted to set a debug only attribute (" << attrConfig.getPropertyName() <<
                    "). This is currently only allowed for super users. Using default value instead.");
            }
            else
            {
                EA::TDF::GenericTdfType mergedGeneric;
                if (attemptMergeOp(clientPropValue, attrConfig.getMergeOp(), mergedGeneric))
                {
                    if (mergedGeneric.getValue().convertToResult(criteriaAttr, bfMember))
                    {
                        return ERR_OK;
                    }
                }
                else if (clientPropValue.convertToResult(criteriaAttr, bfMember))
                {
                    // Value as set in the properties was assigned correctly 
                    return ERR_OK;
                }

                // Incorrect data type sent from the client, output warning and fall back on default.
                ERR_LOG(componentDescription << "Mismatching value type for property(" << attrConfig.getPropertyName() << ") sent by client: "
                    "Expected type " << criteriaAttr.getFullName() << ". Client sent type " << clientPropValue.getFullName() << ".");
                conversionFailedError = true;
            }
        }

        // Apply Attribute

        // Iterate over every mTemplateAttributes, if it exists in attributes use that value, otherwise use the default.
        TemplateAttributes::const_iterator clientAttr = clientAttributes.find(attrConfig.getAttrName());
        if (clientAttr != clientAttributes.end())
        {
            const EA::TDF::TdfGenericConst& clientAttrValue = clientAttr->second->getValue();
            if (attrConfig.getDebugOnly() && !UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_START_DEBUG_MATCHMAKING_SCENARIO))
            {
                WARN_LOG(componentDescription << "Client attempted to set a debug only attribute (" << attrConfig.getAttrName() <<
                            "). This is currently only allowed for super users. Using default value instead.");
            }
            else
            {

                EA::TDF::GenericTdfType mergedGeneric;
                if (attemptMergeOp(clientAttrValue, attrConfig.getMergeOp(), mergedGeneric))
                {
                    if (mergedGeneric.getValue().convertToResult(criteriaAttr, bfMember))
                    {
                        return ERR_OK;
                    }
                }
                else if (clientAttrValue.convertToResult(criteriaAttr, bfMember))
                {
                    // Value sent from client was assigned correctly 
                    return ERR_OK;
                }

                // Incorrect data type sent from the client, output warning and fall back on default.
                ERR_LOG(componentDescription << "Mismatching value type for attribute(" << attrConfig.getAttrName() << ") sent by client: "
                    "Expected type " << criteriaAttr.getFullName() << ". Client sent type " << clientAttrValue.getFullName() << ".");
                conversionFailedError = true;
            }
        }
            
        //Apply Default

        // If we set a default value, use it: 
        if (attrConfig.getDefault().isValid())
        {
            // No Merge op is performed here.  (We assume that the default is already merged)
            if (!attrConfig.getDefault().getValue().convertToResult(criteriaAttr, bfMember))
            {
                // We should've already checked for this in the validateConfig() call
                outFailingAttribute = attrConfig.getAttrNameAsTdfString().empty() ? attrConfig.getPropertyName() : attrConfig.getAttrName();
                return GAMEMANAGER_ERR_INVALID_TEMPLATE_ATTRIBUTE;
            }
        }
        else if (!attrConfig.getIsOptional() && !(attrConfig.getAttrNameAsTdfString().empty() && attrConfig.getPropertyNameAsTdfString().empty()))
        {
            // No default was provided, and the client didn't send one, that's an error.
            outFailingAttribute = attrConfig.getAttrNameAsTdfString().empty() ? attrConfig.getPropertyName() : attrConfig.getAttrName();
            if (conversionFailedError)
                return GAMEMANAGER_ERR_INVALID_TEMPLATE_ATTRIBUTE;
            else
                return GAMEMANAGER_ERR_MISSING_TEMPLATE_ATTRIBUTE;
        }

        return ERR_OK;
    }

    BlazeRpcError applyTemplateTdfAttributes(EA::TDF::TdfPtr& outTdfPtr, const TemplateAttributeTdfMapping& templateAttributeMapping, const TemplateAttributes& clientAttributes,
        const PropertyNameMap& properties, const char8_t*& outFailingAttribute, const char8_t* componentDescription)
    {
        EA::TDF::TdfFactory& tdfFactory = EA::TDF::TdfFactory::get();

        // First, generate the TDF that would be used, if it's not already set: 
        if (templateAttributeMapping.empty())
        {
            // Incorrect data type sent from the client, output warning and fall back on default.
            ERR_LOG(componentDescription << "Missing TDF type attribute for template mapping.  This should not happen (should already be validated by config).");
            return ERR_SYSTEM;
        }

        const EA::TDF::TypeDescription* typeDesc = tdfFactory.getTypeDesc(templateAttributeMapping.begin()->first);
        if (typeDesc == nullptr)
        {
            ERR_LOG(componentDescription << "Unable to construct TDF type from template mapping.  This should not happen (should already be validated by config).");
            return ERR_SYSTEM;
        }


        if (outTdfPtr != nullptr)
        {
            if (outTdfPtr->getTdfId() != typeDesc->getTdfId())
            {
                ERR_LOG(componentDescription << "TDF type used in template mapping differs from the value passed in the mapping request. "
                        << "("<< typeDesc->getShortName() << " vs. (Output) "<< outTdfPtr->getTypeDescription().getShortName()  <<").");
                return ERR_SYSTEM;
            }
        }
        else
        {
            // If the TDF is not set, allocate one: 
            outTdfPtr = tdfFactory.create(typeDesc->getTdfId());
        }

        EA::TDF::TdfGenericReference criteriaRef(outTdfPtr);
        return applyTemplateAttributes(criteriaRef, *templateAttributeMapping.begin()->second, clientAttributes, properties, outFailingAttribute, componentDescription, false);
    }

    void getTypeDescsForAttribute(const TemplateAttributeName& attribute, eastl::list<const TemplateAttributeMapping*> templateAttributeMappings, eastl::list<const EA::TDF::TypeDescription*>& outTypeDescs)
    {
        EA::TDF::TdfFactory& tdfFactory = EA::TDF::TdfFactory::get();
        for (auto& templateAttributeMapping : templateAttributeMappings)
        {
            for (auto& curAttr : *templateAttributeMapping)
            {
                if (attribute != curAttr.second->getAttrName())
                {
                    continue;
                }

                const EA::TDF::TypeDescription* typeDesc = tdfFactory.getTypeDesc(curAttr.first);
                if (typeDesc == nullptr)
                {
                    continue;
                }

                bool alreadyFoundType = false;
                for (auto& outTypeDesc : outTypeDescs)
                {
                    if (outTypeDesc == typeDesc)
                    {
                        alreadyFoundType = true;
                        break;
                    }
                }
                if (!alreadyFoundType)
                { 
                    outTypeDescs.push_back(typeDesc);
                }
            }
        }
    }

}
}
