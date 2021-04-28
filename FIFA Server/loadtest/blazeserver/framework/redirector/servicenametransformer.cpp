#include "framework/blaze.h"
#include "redirector/tdf/redirectortypes.h"
#include "redirector/tdf/redirector_server.h"
#include "framework/util/shared/blazestring.h"
#include "framework/util/locales.h"
#include "framework/util/inputfilter.h"
#include "framework/redirector/servicenametransformer.h"

namespace Blaze
{
namespace Redirector
{
//class ServiceNameTransform
//////////////////////////////////////////////////////////////////////////
bool ServiceNameTransform::initialize(int32_t entryIdx, const ServiceNameTransformData* config)
{
    const char8_t* newServiceName = config->getNewServiceName(); 
    if (newServiceName == nullptr)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[ServiceNameTransformer].initialize: missing 'newServiceName' parameter in "
            "serviceNameTransforms at index " << entryIdx);
        return false;
    }
    mOutputServiceName = blaze_strdup(newServiceName);

    const char8_t* newConnectionProfile = config->getNewConnectionProfile();

    //if connection profile is not specified 
    if (newConnectionProfile == nullptr || newConnectionProfile[0] == '\0')
        newConnectionProfile = config->getInput().getConnectionProfile();

    //if connection profile is still not specified, mOutputConnectionProfile remains to be null
    if (newConnectionProfile != nullptr && newConnectionProfile[0] != '\0')
        mOutputConnectionProfile = blaze_strdup(newConnectionProfile);

    mOutputClientType = config->getNewClientType();

    bool ignoreCase = config->getIgnoreCase();
    RegexType regexType = config->getRegexType();

    const InputData& inputMap = config->getInput();

    if (!mInputFilters[INPUT_SERVICE_NAME].initialize(
        inputMap.getServiceName(), 
        ignoreCase, regexType))
    {
        return false;
    }
    if (!mInputFilters[INPUT_ENVIRONMENT].initialize(
        inputMap.getEnvironment(),
        ignoreCase, regexType))
    {
        return false;
    }
    if (!mInputFilters[INPUT_PLATFORM].initialize(
        inputMap.getPlatform(),
        ignoreCase, regexType))
    {
        return false;
    }
    if (!mInputFilters[INPUT_CONNECTION_PROFILE].initialize(
        inputMap.getConnectionProfile(),
        ignoreCase, regexType))
    {
        return false;
    }
    if (!mInputFilters[INPUT_CLIENT_NAME].initialize(
        inputMap.getClientName(),
        ignoreCase, regexType))
    {
        return false;
    }
    if (!mInputFilters[INPUT_CLIENT_VERSION].initialize(
        inputMap.getClientVersion(),
        ignoreCase, regexType))
    {
        return false;
    }
    if (!mInputFilters[INPUT_CLIENT_SKU_ID].initialize(
        inputMap.getClientSkuId(),
        ignoreCase, regexType))
    {
        return false;
    }
    if (!mInputFilters[INPUT_CLIENT_LOCALE].initialize(
        inputMap.getClientLocale(),
        ignoreCase, regexType))
    {
        return false;
    }
    if (!mInputFilters[INPUT_BLAZESDK_VERSION].initialize(
        inputMap.getBlazeSDKVersion(),
        ignoreCase, regexType))
    {
        return false;
    }
    if (!mInputFilters[INPUT_BLAZESDK_BUILD_DATE].initialize(
        inputMap.getBlazeSDKBuildDate(),
        ignoreCase, regexType))
    {
        return false;
    }
    if (!mInputFilters[INPUT_DIRTYSDK_VERSION].initialize(
        inputMap.getDirtySDKVersion(),
        ignoreCase, regexType))
    {
        return false;
    }
    if (!mInputFilters[INPUT_CLIENT_TYPE].initialize(
        inputMap.getClientType(),
        ignoreCase, regexType))
    {
        return false;
    }

    return true;
}

bool ServiceNameTransform::transform(const ServerInstanceRequest& request,
            char8_t* newServiceName, size_t newServiceNameLen,
            char8_t* newConnectionProfile, size_t newConnectionProfileLen,
            ClientType& newClientType)
{
    char locale[8];
    LocaleTokenCreateLocalityString(locale, request.getClientLocale());

    bool match = mInputFilters[INPUT_SERVICE_NAME].match(request.getName())
        && mInputFilters[INPUT_ENVIRONMENT].match(request.getEnvironment())
        && mInputFilters[INPUT_PLATFORM].match(request.getPlatform())
        && mInputFilters[INPUT_CONNECTION_PROFILE].match(request.getConnectionProfile())
        && mInputFilters[INPUT_CLIENT_NAME].match(request.getClientName())
        && mInputFilters[INPUT_CLIENT_VERSION].match(request.getClientVersion())
        && mInputFilters[INPUT_CLIENT_SKU_ID].match(request.getClientSkuId())
        && mInputFilters[INPUT_CLIENT_LOCALE].match(locale)
        && mInputFilters[INPUT_BLAZESDK_VERSION].match(request.getBlazeSDKVersion())
        && mInputFilters[INPUT_BLAZESDK_BUILD_DATE].match(request.getBlazeSDKBuildDate())
        && mInputFilters[INPUT_DIRTYSDK_VERSION].match(request.getDirtySDKVersion())
        && mInputFilters[INPUT_CLIENT_TYPE].match(ClientTypeToString(request.getClientType()));

    if (match)
    {
        // A match was found so replace the input service name and connection profile name
        // with the overrides if they exist.

        if (mOutputServiceName != nullptr)
            blaze_strnzcpy(newServiceName, mOutputServiceName, newServiceNameLen);
        else
            blaze_strnzcpy(newServiceName, request.getName(), newServiceNameLen);

        if (mOutputConnectionProfile != nullptr)
        {
            blaze_strnzcpy(newConnectionProfile, mOutputConnectionProfile,
                newConnectionProfileLen);
        }
        else
        {
            // if mOutputConnectionProfile is not set, use the profile in the request
            blaze_strnzcpy(newConnectionProfile, request.getConnectionProfile(),
                newConnectionProfileLen);
        }
        if (mOutputClientType != CLIENT_TYPE_INVALID)
            newClientType = mOutputClientType;
        else
            newClientType = request.getClientType();
    }
    return match;
}

//class  ServiceNameTransformManager
//////////////////////////////////////////////////////////////////////////

bool ServiceNameTransformManager::initialize(const  RedirectorConfig& config)
{
    const ServiceNameTransformsList* seq = &config.getServiceNameTransforms(); 
    if (seq == nullptr)
        return true;

    ServiceNameTransformsList::const_iterator itr = seq->begin();
    ServiceNameTransformsList::const_iterator end = seq->end();
    int32_t idx = 0;
         
    for(; itr != end; ++itr)
    {
        if(!*itr)
             continue;

        ServiceNameTransformPtr transform = BLAZE_NEW ServiceNameTransform;
        if (!transform->initialize(idx, *itr))
        {
            return false;
        }

        mTransforms.push_back(transform);             
        idx++;
     }

    return true;
}

bool ServiceNameTransformManager::transform(const ServerInstanceRequest& request,
        char8_t* newServiceName, size_t newServiceNameLen,
        char8_t* newConnectionProfile, size_t newConnectionProfileLen, ClientType& newClientType)
{
    TransformationList::iterator itr = mTransforms.begin();
    TransformationList::iterator end = mTransforms.end();
    for(; itr != end; ++itr)
    {
        if ((*itr)->transform(request, newServiceName, newServiceNameLen,
                    newConnectionProfile, newConnectionProfileLen, newClientType))
        {
            return true;
        }
    }

    // No transformation happened so default to using the input parameters
    blaze_strnzcpy(newServiceName, request.getName(), newServiceNameLen);
    blaze_strnzcpy(newConnectionProfile, request.getConnectionProfile(), newConnectionProfileLen);

    return false;
}

} // namespace Redirector
} // namespace Blaze

