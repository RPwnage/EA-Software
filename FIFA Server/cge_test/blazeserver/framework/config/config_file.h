/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

/*** Include files *******************************************************************************/
#include "config_map.h"
#include "EASTL/map.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class PredefineMap : public eastl::map<eastl::string, eastl::string>
{
public:
    void addBuiltInValues();
    void addFromKeyValuePair(const eastl::string& keyValuePair);
};

class ConfigFile : public ConfigMap
{
public:
    enum Error
    {
        ERROR_NONE,     // No error
        ERROR_SYNTAX,   // Syntax error
        ERROR_FILE   // File not found or cannot be opened
    };
    
    static ConfigFile* createFrom(const ConfigFile& config);

    static ConfigFile* createFromFile(const eastl::string& filePath, const bool allowUnquotedStrings = false) { return createFromFile("./", filePath, allowUnquotedStrings, PredefineMap()); }
    static ConfigFile* createFromFile(const eastl::string& rootDir, const eastl::string& filePath, const eastl::string& overrideRootDir, const eastl::string& mOverridePath, const bool allowUnquotedStrings, const PredefineMap& predefines, Error* result = nullptr, const bool stringEscapes = true);
    static ConfigFile* createFromString(const char8_t* data, const char8_t* mOverrideData, const bool allowUnquotedStrings, Error* result = nullptr, const bool stringEscapes = true);

    static ConfigFile* createFromFile(const eastl::string& rootDir, const eastl::string& filePath, const bool allowUnquotedStrings, const PredefineMap& predefines, Error* result = nullptr, const bool stringEscapes = true);
    static ConfigFile* createFromString(const char8_t* data, const bool allowUnquotedStrings, Error* result = nullptr, const bool stringEscapes = true);

    static ConfigFile* preprocessFromFile(const eastl::string& rootDir, const eastl::string& filePath, const PredefineMap& externalPredefines, Error* result = nullptr);
    
    ~ConfigFile() override;

    const eastl::string &getRawConfigData() const { return mRawConfigData; }
    const ConfigFile* getOverrideFile() const { return mOverrideFile; }
    const PredefineMap &getDefineMap() const { return mDefineMap; }

    const eastl::string& getRootDir() const { return mRootDir; }
    const eastl::string& getFilePath() const { return mFilePath; }

    bool compare(const ConfigFile* rhs) const;
    bool applyOverride(const ConfigFile& other);

    const ConfigMap& getFeatureConfigMap(const char8_t* featureName) const;

private:
    ConfigFile(const char8_t* rawData) : mRawConfigData(rawData), mOverrideFile(nullptr) {}
    ConfigFile(const eastl::string& rootDir, const eastl::string& filePath, const PredefineMap &predefineMap);
    ConfigFile(const ConfigFile &config);
    ConfigFile& operator=(const ConfigFile& otherObj);
    
    bool preprocess(ConfigFile::Error* result);
    bool parse(bool allowUnquotedStrings, bool allowStringEscapes, ConfigFile::Error* err);

    eastl::string mRootDir;
    eastl::string mFilePath;
    eastl::string mRawConfigData;
    const ConfigFile* mOverrideFile;
    PredefineMap mDefineMap;
};

} // Blaze

#endif // CONFIG_FILE_H
