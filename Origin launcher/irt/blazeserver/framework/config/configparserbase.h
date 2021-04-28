/*! ************************************************************************************************/
/*!
    \file configparserbase.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_CONFIG_PARSER_BASE_H
#define BLAZE_CONFIG_PARSER_BASE_H

#include "EASTL/string.h"

namespace Blaze
{

class ConfigMap;
class ConfigSequence;

/*! ************************************************************************************************/
/*! \brief provides a framework for parsing a config map.  Use the logConfigProblem method
        to log errors/warnings as you parse.  The class will track the number of warnings & errors.
***************************************************************************************************/
class ConfigParserBase
{
    NON_COPYABLE(ConfigParserBase);

public:

    /*! ************************************************************************************************/
    /*! \brief get information regard to config file parsing, eg: error/warning

        \param[in] the log string to append the parse overview
    ***************************************************************************************************/
    void appendParseErrorOverview(eastl::string &logMessage) const;


    /*! ************************************************************************************************/
    /*! \brief get the number of warnings encountered while parsing the config file.
        \return the warning count
    *************************************************************************************************/
    size_t getWarningCount() const { return mWarningCount; }

    /*! ************************************************************************************************/
    /*! \brief get the number of errors encountered while parsing the config file.
        \return the error count
    *************************************************************************************************/
    size_t getErrorCount() const { return mErrorCount; }

    /*! ************************************************************************************************/
    /*! \brief compare two config parser classes for equality.  This only checks that the config map
             for each of the config parser bases are equal.
        \return true if this ConfigParserBase and the rhs ConfigParserBase are equal.
    *************************************************************************************************/
    bool operator==(const ConfigParserBase& rhs) const;

    /*! ************************************************************************************************/
    /*! \brief check to see if the config map has changed on reconfigure.
        \return true if the configMap has changed.
    *************************************************************************************************/
    bool needsReconfigure(const ConfigMap& configMap) const;

    /*! ************************************************************************************************/
    /*! \brief Update the config map pointer.  This is required during a reconfigure
        if the settings haven't changed, the old ConfigMap will still get
        deleted by the framework.
    *************************************************************************************************/
    void cacheConfigMapPointer(const ConfigMap& configMap);

protected:

    /*! ************************************************************************************************/
    /*! brief create a ConfigInfoBase obj, holding some common members and behaviors shared by the 
           inherited config objects eg. GameSessionConfig & MatchmakingConfig 

        Note: Config object which is responsible to extract data information from .cfg file should 
            inherit from this class and call this constructor in it's own constructor 

        \param [in] config pointer to the global config map
        \param [in] configKey key of config file section to identify a specific config entry
    *************************************************************************************************/
    ConfigParserBase(const ConfigMap* config, const char8_t* configKey);

    //! destructor
    ~ConfigParserBase(void) {}

    /*! ************************************************************************************************/
    /*! brief logging wrapper that inserts a boilerplate 'configKey' string to the front of the msg;
            also increments the internal warning/error counts.

        \param[in]    logLevel - level to log the message at
        \param[in]    formatString - printf-style format string.
    *************************************************************************************************/
    void logConfigProblem(Logging::Level logLevel, const char8_t* formatString, ...) ATTRIBUTE_PRINTF_CHECK(3,4);

private:
    const ConfigMap* configMapFromConfigKey(const ConfigMap* config, const char* configKey) const;

protected:
    const ConfigMap* mConfig;

private:
    const char* mConfigKey;
    size_t mWarningCount;
    size_t mErrorCount;
};

} // namespace Blaze

#endif //BLAZE_CONFIG_PARSER_BASE_H
