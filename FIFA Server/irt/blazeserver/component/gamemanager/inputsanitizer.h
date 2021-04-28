/*! ************************************************************************************************/
/*!
    \file   inputsanitizer.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/


#ifndef BLAZE_INPUT_SANITIZER
#define BLAZE_INPUT_SANITIZER

#include "gamemanager/gamemanagerslaveimpl.h"

namespace Blaze
{

// Move this to Framework if Templates are used by other systems:
namespace GameManager
{
    bool inputSanitizerConfigValidation(const SanitizerDefinitionMap& sanitizersConfig, ConfigureValidationErrors& validationErrors);

    BlazeRpcError applySanitizers(const SanitizerDefinitionMap& sanitizersConfig, const SanitizerNameList& sanitizersToApply, TemplateAttributes& attributes, PropertyNameMap& properties, const char8_t*& outFailingSanitizer);

    BlazeRpcError getSanitizersPropertiesUsed(const SanitizerDefinitionMap& sanitizersConfig, const SanitizerNameList& sanitizersToApply, PropertyNameList& propertiesUsed);
}
}

#endif // BLAZE_INPUT_SANITIZER
