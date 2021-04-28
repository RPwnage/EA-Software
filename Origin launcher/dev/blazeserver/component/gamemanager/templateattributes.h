/*! ************************************************************************************************/
/*!
    \file   templateattributes.h

      This is a file that holds the helper functions for attributes. Used by Scenarios and Templates. 

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_TEMPLATE_ATTRIBUTES
#define BLAZE_TEMPLATE_ATTRIBUTES


#include "gamemanager/gamemanagerslaveimpl.h"

namespace Blaze
{

// Move this to Framework if Templates are used by other systems:
namespace GameManager
{
    typedef eastl::hash_set<EA::TDF::TdfId> TdfIdHashSet;
    typedef eastl::map<eastl::string, const EA::TDF::TypeDescription*> AttributeToTypeDescMap;
    class PropertyManager;

    // Validate the attribute mappings provided from the config:
    // This checks the that the types are valid, that they don't conflict with any existing types, and that they have a default and/or a name set.
    bool validateTemplateAttributeMapping(const TemplateAttributeMapping& attributes, AttributeToTypeDescMap& attrToTypeDesc, const EA::TDF::TdfGenericReference& requestRef,
                                          ConfigureValidationErrors& validationErrors, const char8_t* errorStringPrefix, const char8_t* ruleName = nullptr, bool nameIncludesTdf = true);

    bool validateTemplateTdfAttributeMapping(const TemplateAttributeTdfMapping& templateAttributeMapping, AttributeToTypeDescMap& attrToTypeDesc,
                                          ConfigureValidationErrors& validationErrors, const char8_t* errorStringPrefix, const TdfIdHashSet& acceptableTdfs);


    // Apply the template attributes provided to the referenced type:
    BlazeRpcError applyTemplateAttributeDefinition(EA::TDF::TdfGenericReference criteriaAttr, const TemplateAttributeDefinition& attrConfig, const TemplateAttributes& clientAttributes,
        const PropertyNameMap& properties, const char8_t*& outFailingAttribute, const char8_t* componentDescription, const EA::TDF::TypeDescriptionBitfieldMember* bfMember = nullptr);

    // Returns an error if the type wasn't setup correctly, 
    BlazeRpcError applyTemplateAttributes(EA::TDF::TdfGenericReference tdfToApplyAttributesOntoRef, const TemplateAttributeMapping& templateAttributeMapping, const TemplateAttributes& clientAttributes, 
                                          const PropertyNameMap& clientProperties, const char8_t*& outFailingAttribute, const char8_t* componentDescription, bool nameIncludesTdf = true, const PropertyManager* propertyManager = nullptr);

    // Apply the template attributes provided to the referenced type:
    // Returns an error if the type wasn't setup correctly, 
    BlazeRpcError applyTemplateTdfAttributes(EA::TDF::TdfPtr& outTdfPtr, const TemplateAttributeTdfMapping& templateAttributeMapping, const TemplateAttributes& clientAttributes,
        const PropertyNameMap& clientProperties, const char8_t*& outFailingAttribute, const char8_t* componentDescription);


    // Get a list of TypeDescriptions associated with a given attribute in a given attribute mapping. 
    // This can be used to determine if an attribute is mapped, or to tell what type should be used for a given attribute.
    void getTypeDescsForAttribute(const TemplateAttributeName& attribute, eastl::list<const TemplateAttributeMapping*> templateAttributeMappings, eastl::list<const EA::TDF::TypeDescription*>& outTypeDescs);

    // Attempt to apply a merge operation to the given source data, and place it into the GenericTdfType value:
    bool attemptMergeOp(const EA::TDF::TdfGenericConst& sourceValue, MergeOp curMergeOp, EA::TDF::GenericTdfType& mergedGeneric);
}
}

#endif  // BLAZE_TEMPLATE_ATTRIBUTES