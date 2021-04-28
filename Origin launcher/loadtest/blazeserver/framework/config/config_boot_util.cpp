/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ConfigBootUtil

    This class decodes the Blaze boot file into a BootConfig TDF, and provides method(s) to load
    other config files based on the options specified in the boot file.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/config/configdecoder.h"
#include "framework/config/config_file.h"
#include "framework/config/config_boot_util.h"

#include "EAIO/EAFileUtil.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/
    ConfigBootUtil::ConfigBootUtil(const eastl::string& bootConfigFilename, const eastl::string& overrideFileName, const PredefineMap& externalPredefines, bool configStringEscapes)
    : mBootConfigFilename(bootConfigFilename),
    mOverrideFilename(overrideFileName),
    mBootConfig(nullptr),
    mExternalPredefines(externalPredefines),
    mConfigStringEscapes(configStringEscapes)
{
}

ConfigBootUtil::~ConfigBootUtil()
{    
    delete mBootConfig;
}

bool ConfigBootUtil::initialize()
{
    PredefineMap::const_iterator itEnd= mExternalPredefines.end();
    for (PredefineMap::const_iterator it=mExternalPredefines.begin(); it != itEnd; ++it)
    {
        BLAZE_INFO_LOG(Log::SYSTEM, "[ConfigBootUtil].initialize: config predefines(" << it->first << "=" << it->second << ")");    
    }

    bool allowUnquotedStrings = false;
  #ifdef BLAZE_ENABLE_DEPRECATED_UNQUOTED_STRING_IN_CONFIG
    allowUnquotedStrings = true;
  #endif
    ConfigFile::Error result = ConfigFile::ERROR_NONE;

    if (!mOverrideFilename.empty())
    {
        BLAZE_INFO_LOG(Log::SYSTEM, "[ConfigBootUtil].initialize: using boot override file '" << mOverrideFilename << "'.");
    }

    mBootConfig = ConfigFile::createFromFile("./", mBootConfigFilename, "./", mOverrideFilename, allowUnquotedStrings, mExternalPredefines, &result);
    if (mBootConfig == nullptr)
    {
        // Tailor the message to whether the file can't be read (i.e. doesn't exist) or had
        // a syntax error.
        if (result == ConfigFile::ERROR_FILE)
        {
            BLAZE_FATAL_LOG(Log::SYSTEM, "[ConfigBootUtil].initialize: server boot configuration file '" << mBootConfigFilename 
                << "' (or a file included by it) not found or cannot be opened");
        }
        else
        {
            BLAZE_FATAL_LOG(Log::SYSTEM, "[ConfigBootUtil].initialize: errors detected in server boot configuration file '" << mBootConfigFilename << "' (or a file included by it)");
        }
        return false;
    }

    //Now decode the boot file.
    ConfigDecoder decoder(*mBootConfig, true);
    if (!decoder.decode(&mBootConfigTdf))
    {
        BLAZE_FATAL_LOG(Log::SYSTEM, "[ConfigBootUtil].initialize: failed TDF decode of boot configuration file '" << mBootConfigFilename << "'");
        return false;
    }

    //Now decode the server instance details from the boot config tdf
    eastl::set<eastl::string> instanceNames;
    for (ServerInstanceConfigList::iterator
        it = mBootConfigTdf.getServerInstances().begin(),
        end = mBootConfigTdf.getServerInstances().end();
        it != end; ++it)
    {
        ServerInstanceConfig& config = **it;
        if (config.getName()[0] == '\0')
        {
            BLAZE_FATAL_LOG(Log::SYSTEM, "[ConfigBootUtil].initialize: Missing instance name for server instance of type '" << config.getType() << "'");
            return false;
        }
        else if (!instanceNames.insert(config.getName()).second)
        {
            BLAZE_FATAL_LOG(Log::SYSTEM, "[ConfigBootUtil].initialize: Multiple server instances are using instance name '" << config.getName() << "'");
            return false;
        }
    }

    return true;
}

ConfigFile* ConfigBootUtil::createFrom(const char8_t* configName, uint32_t flags, Error* result) const
{
    if (result)
        *result = CONFIG_OK;

    if (mBootConfig == nullptr)
    {
        //not initialized, should never happen, but bail
        EA_ASSERT_MESSAGE(false, "ConfigBootUtil::createFrom: Tried to use the config before it was initialized.");
        if (result)
            *result = CONFIG_UNDEFINED;
        return nullptr;
    }

    bool allowUnquotedStrings = false;
  #ifdef BLAZE_ENABLE_DEPRECATED_UNQUOTED_STRING_IN_CONFIG
    allowUnquotedStrings = true;
  #endif

    //Find our filename    
    eastl::string fileName, rootDir;
    Error findFileErr = findConfigFile(configName, flags, false, fileName, rootDir);
    if (findFileErr != CONFIG_OK)
    {
        //No file.  If it wasn't some configuration error, see if we can do a dummy, otherwise bail.
        if (findFileErr == CONFIG_NOT_FOUND && ((flags & CONFIG_CREATE_DUMMY) != 0))
        {
            //Return a dummy
            BLAZE_INFO_LOG(Log::CONFIG, "[ConfigBootUtil].createFrom: Config file not found,  using dummy file instead");
            eastl::string dummyConfig;
            dummyConfig.append_sprintf("%s={}\n", configName);
            return ConfigFile::createFromString(dummyConfig.c_str(), allowUnquotedStrings);
        }
        else
        {
            //We're done
            if (result != nullptr)
                *result = findFileErr;
            return nullptr;
        }
    }

    //We've got a file, lets parse it.

    // By default, use the already read in boot config.
    const ConfigFile* predefineConfig = mBootConfig;
    if (flags & CONFIG_RELOAD)
    {
        // During reconfigure, we always rerun the preprocessor on the boot file.
        ConfigFile::Error preprocessErr = ConfigFile::ERROR_NONE;
        ConfigFile* tempPreProcessFile = ConfigFile::preprocessFromFile(mBootConfig->getRootDir(), mBootConfig->getFilePath(), mExternalPredefines, &preprocessErr);
        if (tempPreProcessFile != nullptr)
        {
            BLAZE_INFO_LOG(Log::CONFIG, "[ConfigBootUtil].createFrom: Opened boot config file: '" << mBootConfig->getFilePath() << "'.");
            predefineConfig = tempPreProcessFile;
        }
        else
        {
            // We failed to parse our predefines in the boot config.  Warn, and default back to our old one.
            predefineConfig = mBootConfig;
            BLAZE_WARN_LOG(Log::CONFIG, "[ConfigBootUtil].createFrom: Failed to parse boot config file during reconfigure: '" 
                << mBootConfig->getFilePath() << "' due to syntax error, defaulting to already loaded values.");
        }
    }
       
    //find any override file
    eastl::string overrideFileName, overrideRootDir;
    findConfigFile(configName, flags, true, overrideFileName, overrideRootDir);

    // use the define map from the boot file to parse the the new config file
    ConfigFile::Error parseErr = ConfigFile::ERROR_NONE;
    ConfigFile* file = ConfigFile::createFromFile(rootDir, fileName, overrideRootDir, overrideFileName, allowUnquotedStrings, predefineConfig->getDefineMap(), &parseErr, mConfigStringEscapes);

    if (file != nullptr)
    {
        // success
        if (overrideFileName.empty())
        {
            BLAZE_INFO_LOG(Log::CONFIG, "[ConfigBootUtil].createFrom: Opened config file: '" << fileName << "'.");     
        }
        else
        {
            BLAZE_INFO_LOG(Log::CONFIG, "[ConfigBootUtil].createFrom: Opened config file: '" << fileName << "' with override file '" << overrideFileName << "'.");     
        }
    }
    else
    {        
        BLAZE_WARN_LOG(Log::CONFIG, "[ConfigBootUtil].createFrom: Failed to parse config file: '" << fileName << "' due to syntax error.");
        if (result != nullptr)
            *result = CONFIG_SYNTAX_ERROR;
    }

    // Cleanup the memory on our temp predefine file.
    if (predefineConfig != mBootConfig)
    {
        delete predefineConfig;
    }

    return file;
}

ConfigBootUtil::Error ConfigBootUtil::findConfigFile(const char8_t* configName, uint32_t flags, bool isOverride, 
                                    eastl::string& filePath, eastl::string& rootDir) const
{
    const char8_t* feature = configName;
    const char8_t* componentNameToken = "$component";
    const uint32_t componentNameTokenLength = (uint32_t) strlen(componentNameToken);
    const bool isComponent = (flags & CONFIG_IS_COMPONENT);
    const char8_t* key = isComponent ? "component" : feature;

    //Setup a copy of the root configs - for override we strip the first directory and go with the second
    //otherwise we just take the first (or "." if empty)
    typedef eastl::vector<eastl::string> StringVector;
    StringVector roots;
    const BootConfig::StringRootConfigDirectoriesList& rootDirs = mBootConfigTdf.getRootConfigDirectories();

    if (isOverride)
    {
        if (rootDirs.empty())
        {
            //bail if its empty
            return CONFIG_NOT_FOUND;     
        }
        else
        {
            BootConfig::StringRootConfigDirectoriesList::const_iterator itr = rootDirs.begin(), 
                itrEnd = rootDirs.end();
            for (++itr; itr != itrEnd; ++itr)        
            {
                roots.push_back(itr->c_str());
            }
        }
    }
    else
    {
        roots.push_back(rootDirs.empty() ? "." : rootDirs.begin()->c_str());
    }

   
    for (StringVector::const_iterator rootItr = roots.begin(), rootEnd = roots.end(); rootItr != rootEnd; ++rootItr)
    {
        rootDir = *rootItr;
        if(rootDir.back() != '/')
            rootDir += '/';

        //First check to see if the configName is an absolute file path.  If so, return that.
        filePath = rootDir + configName;
        if (EA::IO::File::Exists(filePath.c_str()) && !EA::IO::Directory::Exists(filePath.c_str()))
        {
            return CONFIG_OK;
        }

        //Now find the section of our boot paths we'll be searching for.  
        BootConfig::ConfigPathsMap::const_iterator path = mBootConfigTdf.getConfigPaths().find(key);
        if (path == mBootConfigTdf.getConfigPaths().end())
        {
            BLAZE_WARN_LOG(Log::CONFIG, "[ConfigBootUtil].findFile: configPaths." << key << " does not exist.");
            rootDir = filePath = "";
            return CONFIG_UNDEFINED;
        }

        //Go through the list of directory search paths and build a search path using string substitution 
        //for the feature name
        StringVector dirPaths;

        const ConfigPath::StringDirsList& dirList = path->second->getDirs();
        for (ConfigPath::StringDirsList::const_iterator
            directory = dirList.begin(), dirEnd = dirList.end(); directory != dirEnd; ++directory)
        {
            const char8_t* dir = *directory;
            if (dir[0] != '\0')
            {
                eastl::string& str = dirPaths.push_back();
                str = rootDir;
                const char8_t* start = dir;
                if (isComponent)
                {
                    const char8_t* end;
                    // replaces $component token with componentName (if needed)
                    while ((end = strstr(start, componentNameToken)) != nullptr)
                    {
                        str.append(start, (eastl::string::size_type)(end - start));
                        str.append(feature);
                        start = end + componentNameTokenLength;
                    }
                }
                str.append(start);
                if(str.back() != '/')
                    str += '/';
            }
        }

        //Do we have anywhere to search?
        if (dirPaths.empty())
        {
            if (!isOverride)
            {
                BLAZE_WARN_LOG(Log::CONFIG, "[ConfigBootUtil].findFile: configPaths." << key << ".dir is empty.");
            }

            rootDir = filePath = "";
            return CONFIG_NO_DIRECTORY;
        }

        //Now build the file name using string substitution.   
        eastl::string fileName;
        const char8_t* start = ((flags & CONFIG_LOAD_PRECONFIG) == 0) ? path->second->getFile() : path->second->getPreconfigFile();
        if (isComponent)
        {
            const char8_t* end;
            // replaces $component token with componentName (if needed)
            while ((end = strstr(start, componentNameToken)) != nullptr)
            {
                fileName.append(start, (eastl::string::size_type)(end - start));
                fileName.append(feature);
                start = end + componentNameTokenLength;
            }
        }
        fileName.append(start);

        //Finally, search the list of directories for the filename we've created.
        for (StringVector::const_iterator i = dirPaths.begin(), e = dirPaths.end(); i != e; ++i)
        {
            filePath = *i;
            filePath += fileName;

            if (EA::IO::File::Exists(filePath.c_str()) && !EA::IO::Directory::Exists(filePath.c_str()))
            {
                //Success, return and go
                return CONFIG_OK;
            }
        }

        //We didn't find anything.  Print an error and return empty handed

        // concatenate all search directories into a single string for logging
        eastl::string concatenatedDirPath;
        for (StringVector::const_iterator i = dirPaths.begin(), e = dirPaths.end(); i != e; ++i)
        {
            concatenatedDirPath += *i;
            concatenatedDirPath += ':';
        }

        //Print a warning if we're not going to just use a dummy file, or this is override and we don't care
        if (!isOverride && (flags & CONFIG_CREATE_DUMMY) == 0)
        {
            BLAZE_WARN_LOG(Log::CONFIG, "[ConfigBootUtil].findFile: Config file '" << fileName << "' not found @ dir: "
                << concatenatedDirPath << ".");
        }
    }

    rootDir = filePath = "";                                   
    return CONFIG_NOT_FOUND;
}

} // Blaze
