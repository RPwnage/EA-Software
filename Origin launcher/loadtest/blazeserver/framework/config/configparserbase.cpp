/*! ************************************************************************************************/
/*!
    \file configparserbase.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/config/configparserbase.h"
#include "framework/config/config_map.h"
#include "framework/util/shared/blazestring.h"
#include "framework/logger.h"

namespace Blaze
{

/*! ************************************************************************************************/
/*! brief create a ConfigInfoBase obj, holding some common members and behaviors shared by the 
    inherited config objects eg. GameSessionConfig & MatchmakingConfig 

    Note: Config object which is responsible to extract data information from .cfg file should 
    inherit from this class and call this constructor in it's own constructor 

    \param [in] config pointer to the global config map
    \param [in] configKey key of config file section to identify a specific config entry
*************************************************************************************************/
ConfigParserBase::ConfigParserBase(const ConfigMap* config, const char8_t* configKey)
    : mConfig(nullptr),
    mConfigKey(configKey),
    mWarningCount(0),
    mErrorCount(0) 
{
    mConfig = configMapFromConfigKey(config, configKey);
    if (mConfig == nullptr)
    {
        logConfigProblem(Logging::ERR,  "No configuration found; using default values...");
    }
}

/*! ************************************************************************************************/
/*!
    \brief Helper function to get the config map based on the config key.
    \return The config map if found or nullptr if not found.
*************************************************************************************************/
const ConfigMap* ConfigParserBase::configMapFromConfigKey(const ConfigMap* config, const char* configKey) const
{
    if (config == nullptr)
    {
        // nullptr passed in; most likely a problem loading the gameManager config 
        return nullptr;
    }

    // get the real config from the supplied config based on the key
    if (configKey == nullptr || blaze_stricmp(configKey, "") == 0)
    {
        return config;
    }
    else
    {
        return config->getMap(configKey);
    }
}

/*! ************************************************************************************************/
/*! \brief get information regard to config file parsing, eg: error/warning

    \param[in] the log string to append the parse overview
***************************************************************************************************/
void ConfigParserBase::appendParseErrorOverview(eastl::string &logMessage) const
{
    // setup header
    logMessage.append_sprintf("Logging %s config parsing result:\n", mConfigKey);

    if ( (mWarningCount > 0) || (mErrorCount > 0) )
    {
        logMessage.append_sprintf("ATTENTION: encountered %d warnings and %d errors while parsing the %s config map.\n"
            "\tthe settings below are what the %s is using internally, and probably disagree with the config.\n\n",
            mWarningCount, mErrorCount, mConfigKey, mConfigKey);
    }
    else
        logMessage.append_sprintf("success\n\n");
}


/*! ************************************************************************************************/
/*! brief logging wrapper that inserts a boilerplate 'configKey' string to the front of the msg;
    also increments the internal warning/error counts.

    \param[in]    logLevel - level to log the message at
    \param[in]    formatString - printf-style format string.
*************************************************************************************************/
void ConfigParserBase::logConfigProblem(Logging::Level logLevel, const char8_t* formatString, ...)
{
    // bump the warning/error counts
    if (logLevel == Logging::WARN)
    {
        mWarningCount++;
    }
    else
    {
        if (logLevel == Logging::ERR)
        {
            mErrorCount++;
        }
    }

    if (BLAZE_IS_LOGGING_ENABLED(Log::CONFIG, logLevel))
    {
        // build a new format string w/ the matchmaker prefix at the front
        char8_t actualFormatString[256];
        blaze_snzprintf(actualFormatString, sizeof(actualFormatString), 
            "%s : Config file problem: %s", mConfigKey, formatString);

        va_list args;
        va_start(args, formatString);
        // Note: file & line will be bogus, but they're not logged currently anyways
        Logger::_log(Log::CONFIG, logLevel, __FILE__, __LINE__, actualFormatString, args);
        va_end(args);
    }
}

/*! ************************************************************************************************/
/*! \brief compare two config parser classes for equality.  This only checks that the config map
    for each of the config parser bases are equal.
    \return true if this ConfigParserBase and the rhs ConfigParserBase are equal.
*************************************************************************************************/
bool ConfigParserBase::operator==(const ConfigParserBase& rhs) const
{
    return mConfig == rhs.mConfig ||
        (mConfig != nullptr && rhs.mConfig != nullptr && *mConfig == *rhs.mConfig);
}

/*! ************************************************************************************************/
/*! \brief check to see if the config map has changed on reconfigure.
    \return true if the configMap has changed.
*************************************************************************************************/
bool ConfigParserBase::needsReconfigure(const ConfigMap& configMap) const
{
    const ConfigMap* configMapByKey = configMapFromConfigKey(&configMap, mConfigKey);

    return !((mConfig == configMapByKey) ||
        ((mConfig != nullptr) && (configMapByKey != nullptr) && (*mConfig == *configMapByKey)));
}

/*! ************************************************************************************************/
/*! \brief Update the config map pointer.  This is required during a reconfigure
    if the settings haven't changed, the old ConfigMap will still get
    deleted by the framework.
*************************************************************************************************/
void ConfigParserBase::cacheConfigMapPointer(const ConfigMap& configMap)
{
    mConfig = configMapFromConfigKey(&configMap, mConfigKey);
}

} // namespace Blaze
