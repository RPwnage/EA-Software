/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIRECTOR_SERVICENAMETRANSFORMER_H
#define BLAZE_REDIRECTOR_SERVICENAMETRANSFORMER_H

/*** Include files *******************************************************************************/

#include "EASTL/vector.h"

#include "redirector/tdf/redirectortypes.h"
#include "framework/util/shared/blazestring.h"
#include "framework/util/locales.h"
#include "framework/util/inputfilter.h"
#include "redirector/tdf/redirector_configtypes_server.h"



/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Redirector
{
class RedirectorConfig;

class ServiceNameTransform :public ServiceNameTransformData
{
    NON_COPYABLE(ServiceNameTransform);

public:
        ServiceNameTransform(EA::Allocator::ICoreAllocator& allocator = (EA_TDF_GET_DEFAULT_ICOREALLOCATOR),
            const char8_t* debugName = "")
            : ServiceNameTransformData(allocator, debugName),
            mOutputServiceName(nullptr),
            mOutputConnectionProfile(nullptr),
            mOutputClientType(CLIENT_TYPE_INVALID)
        {
        }

        ~ServiceNameTransform() override
        {
            BLAZE_FREE(mOutputServiceName);
            BLAZE_FREE(mOutputConnectionProfile);
        }

        const char8_t* getInputServiceName() const
        {
            return mInputFilters[INPUT_SERVICE_NAME].getInput();
        }

        bool initialize(int32_t entryIdx, const ServiceNameTransformData* config);
        bool transform(const ServerInstanceRequest& request,
            char8_t* newServiceName, size_t newServiceNameLen,
            char8_t* newConnectionProfile, size_t newConnectionProfileLen, ClientType& newClientType);
           
private:
    enum
    {
        INPUT_SERVICE_NAME = 0,
        INPUT_CLIENT_NAME,
        INPUT_CLIENT_VERSION,
        INPUT_CLIENT_SKU_ID,
        INPUT_CLIENT_LOCALE,
        INPUT_ENVIRONMENT,
        INPUT_BLAZESDK_VERSION,
        INPUT_BLAZESDK_BUILD_DATE,
        INPUT_DIRTYSDK_VERSION,
        INPUT_PLATFORM,
        INPUT_CONNECTION_PROFILE,
        INPUT_CLIENT_TYPE,

        INPUT_MAX
    };

    InputFilter mInputFilters[INPUT_MAX];
    char8_t* mOutputServiceName;
    char8_t* mOutputConnectionProfile;
    ClientType mOutputClientType;
};

typedef EA::TDF::tdf_ptr<ServiceNameTransform> ServiceNameTransformPtr;

class ServerInstanceRequest;

class ServiceNameTransformManager
{
    NON_COPYABLE(ServiceNameTransformManager);

public:
    ServiceNameTransformManager() { }

    bool initialize(const  RedirectorConfig& config);
  
    bool transform(const ServerInstanceRequest& request,
            char8_t* newServiceName, size_t newServiceNameLen,
            char8_t* newConnectionProfile, size_t newConnectionProfileLen, ClientType& newClientType);
    
private:
    typedef eastl::vector<ServiceNameTransformPtr> TransformationList;
    TransformationList mTransforms;
};

} // Redirector
} // Blaze

#endif // BLAZE_REDIRECTOR_SERVICENAMETRANSFORMER_H

