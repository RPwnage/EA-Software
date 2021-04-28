
// prints out preprocessed config file

#include "framework/blaze.h"
#include "framework/config/config_file.h"
#include "framework/config/config_map.h"
#include "framework/logger.h"
#include "framework/util/shared/blazestring.h"
#include "framework/util/locales.h"
#include "framework/redirector/servicenametransformer.h"
#include "redirector/tdf/redirectortypes.h"
#include "framework/config/configdecoder.h"
#include "redirector/tdf/redirector_server.h"

namespace rdirtransform
{

using namespace Blaze;
using namespace Blaze::Redirector;

static int usage(const char8_t* prg)
{
    printf("Usage: %s configFile [key=value ...]\n", prg);
    printf("\n");
    printf("Valid keys:\n");
    printf("    serviceName\n");
    printf("    environment\n");
    printf("    platform\n");
    printf("    connectionProfile\n");
    printf("    clientName\n");
    printf("    clientVersion\n");
    printf("    clientSkuId\n");
    printf("    clientLocale\n");
    printf("    blazeSDKVersion\n");
    printf("    blazeSDKBuildDate\n");
    printf("    dirtySDKVersion\n");
    return 1;
}

static char8_t* stripQuotes(char8_t* val)
{
    if ((val == nullptr) || (val[0] == '\0'))
        return val;

    if (val[0] == '"')
    {
        val++;
        size_t len = strlen(val);
        if (val[len-1] == '"')
            val[len-1] = '\0';
    }
    return val;
}

int rdirtransform_main(int argc, char** argv)
{
    BlazeRpcError err = Logger::initialize();
    if (err != Blaze::ERR_OK)
    {
        printf("Unable to initialize logger.\n");
        exit(1);
    }

    if (argc < 3)
        return usage(argv[0]);

    ConfigFile* config = ConfigFile::createFromFile(argv[1], true);
    if (config == nullptr)
    {
        printf("Unable to read/parse config file.\n");
        exit(1);
    }

    // Parse out the input paramters
    ServerInstanceRequest request;
    for(int32_t idx = 2; idx < argc; ++idx)
    {
        char8_t* key = argv[idx];
        char8_t* val = strchr(argv[idx], '=');
        if (val == nullptr)
        {
            printf("Parameter '%s' missing a value.\n", argv[idx]);
        }
        *val++ = '\0';

        if (blaze_stricmp(key, "serviceName") == 0)
        {
            request.setName(stripQuotes(val));
        }
        else if (blaze_stricmp(key, "environment") == 0)
        {
            request.setEnvironment(stripQuotes(val));
        }
        else if (blaze_stricmp(key, "platform") == 0)
        {
            request.setPlatform(stripQuotes(val));
        }
        else if (blaze_stricmp(key, "connectionProfile") == 0)
        {
            request.setConnectionProfile(stripQuotes(val));
        }
        else if (blaze_stricmp(key, "clientName") == 0)
        {
            request.setClientName(stripQuotes(val));
        }
        else if (blaze_stricmp(key, "clientVersion") == 0)
        {
            request.setClientVersion(stripQuotes(val));
        }
        else if (blaze_stricmp(key, "clientSkuId") == 0)
        {
            request.setClientSkuId(stripQuotes(val));
        }
        else if (blaze_stricmp(key, "clientLocale") == 0)
        {
            char8_t* loc = stripQuotes(val);
            request.setClientLocale(LocaleTokenCreateFromString(loc));
        }
        else if (blaze_stricmp(key, "blazeSDKVersion") == 0)
        {
            request.setBlazeSDKVersion(stripQuotes(val));
        }
        else if (blaze_stricmp(key, "blazeSDKBuildDate") == 0)
        {
            request.setBlazeSDKBuildDate(stripQuotes(val));
        }
        else if (blaze_stricmp(key, "dirtySDKVersion") == 0)
        {
            request.setDirtySDKVersion(stripQuotes(val));
        }
        else
        {
            printf("Unrecognized input key '%s'\n", key);
            return usage(argv[0]);
        }
    }

    const ConfigMap* rdirConfig = config->getMap("component.redirector");
    if (rdirConfig == nullptr)
    {
        printf("No redirector configuration in provided config file.\n");
        exit(1);
    }

    ConfigDecoder configDecoder(*rdirConfig);
    RedirectorConfig Configuration;
    configDecoder.decode(&Configuration);

    ServiceNameTransformManager transform;
    if (!transform.initialize(Configuration))
    {
        printf("Unable to initialize service name transformer.\n");
        exit(1);
    }

    char8_t newServiceName[256];
    blaze_strnzcpy(newServiceName, request.getName(), sizeof(newServiceName));
    char8_t newConnectionProfile[256];
    ClientType newClientType;
    blaze_strnzcpy(newConnectionProfile, request.getConnectionProfile(), sizeof(newConnectionProfile));
    bool rc = transform.transform(request, newServiceName, sizeof(newServiceName),
            newConnectionProfile, sizeof(newConnectionProfile), newClientType);
    if (!rc)
    {
        printf("No transformation will be done.\n");
    }
    else
    {
        printf("newServiceName='%s'\n", newServiceName);
        printf("newConnectionProfile='%s'\n", newConnectionProfile);
        printf("newClientType='%i'\n", newClientType);
    }

    return 0;
}


}

