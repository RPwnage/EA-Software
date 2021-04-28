/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ConfigFile

    The ConfigFile class parses a standard Blaze properties file and allows clients to retrieve
    named properties of various types.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include <stdio.h>
#include "framework/blaze.h"
#include "framework/config/config_file.h"
#include "framework/config/config_map.h"
#include "framework/util/shared/blazestring.h"


#if defined(EA_PLATFORM_UNIX) || defined(EA_PLATFORM_LINUX)
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#endif

#include "EAIO/EAFileBase.h"
#include "EAIO/EAFileUtil.h"

//Because of how flex does its C++ parsers, we can't include more than one lexer's header at a time.
//Therefore we have to hide the actual lexer and use a factory function
namespace Blaze { class ConfigPPLexer; }
typedef eastl::map<eastl::string, eastl::string> PredefMap;
extern Blaze::ConfigPPLexer *createConfigPPLexer(const eastl::string& rootDir, eastl::string &out, PredefMap &map);
extern void destroyConfigPPLexer(Blaze::ConfigPPLexer *);
extern int configppparse(Blaze::ConfigPPLexer *lexer, const char8_t *file);
extern const PredefMap& getPPLexerDefineMap(Blaze::ConfigPPLexer *);
extern bool validatePPLexer(Blaze::ConfigPPLexer *lexer);

namespace Blaze { class ConfigLexer; }
extern Blaze::ConfigLexer *createConfigLexer(eastl::string &buf, Blaze::ConfigMap &rootMap);
extern void destroyConfigLexer(Blaze::ConfigLexer *);
extern int configparse (Blaze::ConfigLexer *lexer);
extern void lexerDisableStringEscapes (Blaze::ConfigLexer *lexer);
extern void lexerSetAllowUnquotedStrings(Blaze::ConfigLexer *lexer);
extern bool validateLexer(Blaze::ConfigLexer *lexer);

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/
void PredefineMap::addBuiltInValues()
{
    // Build predefine map with system specific overrides
#ifdef EA_PLATFORM_WINDOWS
    //add double quote so it is evaluate to a string
    TCHAR userNameBuf[254];
    DWORD userNameSize=(DWORD)sizeof(userNameBuf);
    if (GetUserName(userNameBuf,&userNameSize))
    {
        (*this)["DEV_USER_NAME"] = userNameBuf;

        blaze_strlwr(userNameBuf);
        (*this)["DEV_USER_NAME_LOWER"] = userNameBuf;
    }

    (*this)["SERVER_PLATFORM"] = "Windows";
#endif

#if defined(EA_PLATFORM_UNIX) || defined(EA_PLATFORM_LINUX)
    const char8_t* userName = getenv("USER");
    if (userName == nullptr )
        userName = "unknown";

    (*this)["DEV_USER_NAME"] = userName;

    eastl::string userNameLower(userName);
    userNameLower.make_lower();
    (*this)["DEV_USER_NAME_LOWER"] = userNameLower;

    (*this)["SERVER_PLATFORM"] = "Linux";
#endif

#if ENABLE_CLANG_SANITIZERS
    (*this)["SANITIZER"] = "1";
#else
    (*this)["SANITIZER"] = "0";
#endif

}

void PredefineMap::addFromKeyValuePair(const eastl::string& keyValuePair)
{
    eastl::string::size_type pos = keyValuePair.find('=');
    if (pos == eastl::string::npos)
    {
        insert(keyValuePair);
    }
    else
    {
        eastl::string key = keyValuePair.substr(0, pos);
        eastl::string value = keyValuePair.substr(pos + 1, keyValuePair.size());
        (*this)[key] = value;
    }
}


ConfigFile* ConfigFile::createFrom(const ConfigFile& config)
{
    return BLAZE_NEW_CONFIG ConfigFile(config);
}


ConfigFile* ConfigFile::createFromFile(const eastl::string& rootDir, const eastl::string& filePath, const eastl::string& overrideRootDir, const eastl::string& overrideFilename, const bool allowUnquotedStrings, const PredefineMap& predefines, Error* errResult, const bool stringEscapes)
{
    ConfigFile* result = createFromFile(rootDir, filePath, allowUnquotedStrings, predefines, errResult, stringEscapes);

    if (result != nullptr && !overrideFilename.empty())
    {
        ConfigFile* overrideFile = createFromFile(overrideRootDir, overrideFilename, allowUnquotedStrings, predefines, errResult, stringEscapes);
        if (overrideFile == nullptr || !result->applyOverride(*overrideFile))
        {
            //Override file failed to parse, bail
            BLAZE_ERR_LOG(Log::CONFIG, "[ConfigFile].createFromFile: Failed to apply override configuration file: " << overrideFilename << " to: " << filePath);
            delete result;
            result = nullptr;
        }
        else
        {
            BLAZE_TRACE_LOG(Log::CONFIG, "[ConfigFile].createFromFile: Applied override configuration file: " << overrideFilename << " to: " << filePath);
        }
    }

    return result;
}

ConfigFile* ConfigFile::createFromString(const char8_t* data, const char8_t* overrideData, 
                                         const bool allowUnquotedStrings, Error* errResult, const bool stringEscapes)
{
    ConfigFile* result = createFromString(data, allowUnquotedStrings, errResult, stringEscapes);

    if (result != nullptr && overrideData != nullptr && *overrideData != '\0')
    {
        ConfigFile* overrideFile = createFromString(overrideData, allowUnquotedStrings, errResult, stringEscapes);

        if (overrideFile == nullptr || !result->applyOverride(*overrideFile))
        {
            //Override file failed to parse, bail
            delete result;
            result = nullptr;
        }
    }

    return result;
}

/*************************************************************************************************/
/*!
    \brief create

    Static creator method.

    \param[in]  fileName - The filename to build a config file object from
    \param[in]  predefines - The map of defines to to build a config file object from
*/
/*************************************************************************************************/
ConfigFile* ConfigFile::createFromFile(const eastl::string& rootDir, const eastl::string& filePath, const bool allowUnquotedStrings, const PredefineMap& predefines, Error* result, const bool stringEscapes)
{
    ConfigFile *configFile = BLAZE_NEW_CONFIG ConfigFile(rootDir, filePath, predefines);
    if (!configFile->parse(allowUnquotedStrings, stringEscapes, result))
    {
        delete configFile;
        return nullptr;
    }

    return configFile;
}

/*! ************************************************************************************************/
/*! \brief Runs the preprocessor only on a file.

    \param[in] filePath - the path to the file
    \param[in] externalPredefines - list of external predefines (from cmd line)
    \param[in/out] result - the result status enumeration of parsing the file.

    \return the config file if parsed, otherwise null.
***************************************************************************************************/
ConfigFile* ConfigFile::preprocessFromFile(const eastl::string& rootDir, const eastl::string& filePath, const PredefineMap& predefineMap, Error* result)
{
    ConfigFile *configFile = BLAZE_NEW_CONFIG ConfigFile(rootDir, filePath, predefineMap);
    if (!configFile->preprocess(result))
    {
        delete configFile;
        return nullptr;
    }

    return configFile;
}

/*************************************************************************************************/
/*!
    \brief create

    Static creator method.

    \param[in]  data - data to create the file from.
*/
/*************************************************************************************************/
ConfigFile* ConfigFile::createFromString(const char8_t* data, const bool allowUnquotedStrings, Error* result, const bool stringEscapes)
{
    ConfigFile *configFile = BLAZE_NEW_CONFIG ConfigFile(data);
    if (!configFile->parse(allowUnquotedStrings, stringEscapes, result))
    {
        delete configFile;
        configFile = nullptr;
    }
   
    return configFile;
}

const ConfigMap& ConfigFile::getFeatureConfigMap(const char8_t* featureName) const
{
    const ConfigMap* featureMap = getMap(featureName);
    if (featureMap == nullptr)
    {
        //For some unknown reason some components have configs like "config.<componentName>"  This is 
        //dumb but not something worth eliminating at this point in time.  If just grabbing the component
        //name didn't work, try this alternate naming scheme.
        eastl::string nameString("component.");
        nameString.append(featureName);
        featureMap = getMap(nameString.c_str());

        if (featureMap == nullptr)
        {
            //Feature was not internal to the map, just return the top level map.
            featureMap = this;
        }
    }    

    return *featureMap;
}

/*** Private Methods ******************************************************************************/

ConfigFile::ConfigFile(const eastl::string& rootDir, const eastl::string& filePath, const PredefineMap &predefineMap)
    : mRootDir(rootDir), mFilePath(filePath), mOverrideFile(nullptr), mDefineMap(predefineMap)
{
}

ConfigFile::ConfigFile(const ConfigFile &config)
: ConfigMap(config), mRootDir(config.mRootDir), mFilePath(config.mFilePath), mRawConfigData(config.mRawConfigData), 
  mOverrideFile(config.mOverrideFile != nullptr ? (BLAZE_NEW_CONFIG ConfigFile(*config.mOverrideFile)) : nullptr), 
  mDefineMap(config.mDefineMap)
{
}

ConfigFile::~ConfigFile()
{
    delete mOverrideFile;
}

bool ConfigFile::preprocess(ConfigFile::Error* err)
{
    if (err)
        *err = ERROR_NONE;

    // check if file exists
    if (!EA::IO::File::Exists(mFilePath.c_str()))
    {
        if (err != nullptr)
            *err = ERROR_FILE;
        return false;
    }

    bool result = true;
    ConfigPPLexer *pplexer = createConfigPPLexer(mRootDir, mRawConfigData, mDefineMap);
    int retVal = configppparse(pplexer, mFilePath.c_str());
    if ( (retVal != 0) || !validatePPLexer(pplexer) )
    {
        BLAZE_WARN_LOG(Log::CONFIG, "Failed to preprocess configuration file: " << mFilePath);
        result = false;

        if (err)
            *err = ERROR_SYNTAX;
    }

    destroyConfigPPLexer(pplexer);
    
    return result;
}

bool ConfigFile::parse(bool allowUnquotedStrings, bool allowStringEscapes, ConfigFile::Error* err)
{
    if (err)
        *err = ERROR_NONE;

    if (mRawConfigData.empty() && !preprocess(err))
    {
        return false;
    }

    ConfigLexer *lexer = createConfigLexer(mRawConfigData, *this);
    if (!allowStringEscapes)
    {
        lexerDisableStringEscapes(lexer);
    }

    if (allowUnquotedStrings)
    {
        lexerSetAllowUnquotedStrings(lexer);
    }

    bool result = true;
    if (configparse(lexer) != 0 || !validateLexer(lexer))
    {
        BLAZE_WARN_LOG(Log::CONFIG, "Failed to parse configuration file: " << mFilePath);
        
        if (err)
            *err = ERROR_SYNTAX;
        
        result = false;

    }
    
    destroyConfigLexer(lexer);

    return result;
}

bool ConfigFile::compare(const ConfigFile* rhs) const
{
    if (this == rhs)
        return true;
        
    if (mRawConfigData.compare(rhs->getRawConfigData()) != 0)
        return false;
    
    //Compare mDefineMap
    if (mDefineMap.size() != rhs->getDefineMap().size())
        return false;

    // compare the map
    PredefineMap::const_iterator thisPreMapItr = mDefineMap.begin();
    PredefineMap::const_iterator thisPreMapEnd = mDefineMap.end();    

    // Check that both maps have the same number of arguments and that
    // the keys are equal and the values are equal.  As soon as we hit
    // something that is not equal, return false
    for (; thisPreMapItr != thisPreMapEnd; ++thisPreMapItr)
    {
        PredefineMap::const_iterator rhsPreMapItr = rhs->mDefineMap.find(thisPreMapItr->first);
        if (rhsPreMapItr == rhs->mDefineMap.end()||(thisPreMapItr->second.compare(rhsPreMapItr->second) != 0))
            return false;
    }    
    
    return (*this == *rhs);
}

bool ConfigFile::applyOverride(const ConfigFile& other)
{
    mOverrideFile = &other;

    //Merge in the predefines
    for (PredefineMap::const_iterator pitr = other.mDefineMap.begin(), pend = other.mDefineMap.end(); pitr != pend; ++pitr)
    {
        mDefineMap[pitr->first] = pitr->second;
    }

    //Lookup each key in the map in sequence and set it to the override.  This is not recursive - we only
    //examine the top level map for keynames.
    other.resetIter();
    while (other.hasNext())
    {
        const char8_t* key = other.nextKey();
        const ConfigItem* otherItem = other.getItem(key);
        if (otherItem == nullptr)
        {
            //shouldn't happen
            continue;
        }

        //Now see if there's an outstanding item of the same name in this file
        if (!upsertItem(key, *otherItem))
        {
            BLAZE_ERR_LOG(Log::CONFIG, "[ConfigFile::applyOverride]: Failure attempting to override key " << key << " - item could not be inserted into map.");
            return false;
        }
    }

    return true;
}

} // Blaze
