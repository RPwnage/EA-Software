/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef CONFIG_BOOT_UTIL_H
#define CONFIG_BOOT_UTIL_H

/*** Include files *******************************************************************************/

#include "framework/tdf/frameworkconfigtypes_server.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class ConfigFile;

class PredefineMap;

class ConfigBootUtil
{
    NON_COPYABLE(ConfigBootUtil);
public:
    enum Error
    {
        CONFIG_OK,
        CONFIG_NOT_FOUND,
        CONFIG_SYNTAX_ERROR,
        CONFIG_UNDEFINED,
        CONFIG_NO_DIRECTORY,
    };

    enum Flags
    {
        // Config is a blaze component (ie: gamemanager) as opposed to a feature (ie: logging)
        CONFIG_IS_COMPONENT = 0x1,
        // If the file is not found, create a dummy file instead 
        CONFIG_CREATE_DUMMY = 0x2,
        // Reconfigure
        CONFIG_RELOAD = 0x4,
        // Preconfig Loading (needed for generics in config files)
        CONFIG_LOAD_PRECONFIG = 0x8
    };

    ConfigBootUtil(const eastl::string& bootConfigFilename, const eastl::string& overrideFileName, 
        const PredefineMap& externalPredefines, bool configStringEscapes);
    ~ConfigBootUtil();
    bool initialize();
    ConfigFile* createFrom(const char8_t* configName, uint32_t flags = 0, Error* result = nullptr) const;
    const eastl::string& getBootConfigFilename() const { return mBootConfigFilename; }
    const BootConfig& getConfigTdf() const { return mBootConfigTdf; }
    const ConfigFile* getConfigFile() const { return mBootConfig; }
    bool getConfigStringEscapes() const { return mConfigStringEscapes; }
    
private:    
    ConfigBootUtil::Error findConfigFile(const char8_t* configName, uint32_t flags, bool isOverride, 
        eastl::string& fileName, eastl::string& rootDir) const;

    eastl::string mBootConfigFilename;
    eastl::string mOverrideFilename;
    const ConfigFile* mBootConfig;
    const PredefineMap& mExternalPredefines;
    BootConfig mBootConfigTdf;
    bool mConfigStringEscapes;
};


} // Blaze

#endif // CONFIG_BOOT_UTIL_H
