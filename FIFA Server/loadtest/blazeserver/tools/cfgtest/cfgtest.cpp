
// prints out preprocessed config file

#include "framework/blaze.h"
#include "framework/config/config_file.h"
#include "framework/config/config_sequence.h"
#include "framework/config/config_boot_util.h"
#include "framework/config/configdecoder.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/logger.h"
#include "framework/component/blazerpc.h"
#include "framework/util/shared/rawbufferistream.h"

#include "EASTL/map.h"
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "EATDF/codec/tdfjsonencoder.h"

namespace cfgtest
{
    using namespace Blaze;

    static const int32_t gLogCategory = (Log::COMPONENT_END - 1); // This should be fine until we get 99 components

    static int usage(const char8_t* prg)
    {
        printf("Usage: %s [-Dname[=value] ...] [-V] [-override file] [-output-file file] configfile [key ...]\n", prg);
        printf("   -Dname[=value] creates predefined constant\n");
        printf("   -V                 verbose logging\n");
        printf("   -Q                 allow unquoted strings in config file\n");
        printf("   -server-type       specify server type to get the component level values (can be used more than once to provide multiple server types & overrides server instances in boot file)\n");
        printf("   -no-tdf-decode     do not decode tdfs\n");
        printf("   -print-tdf         print the decoded tdfs instead of the config files (ignored if -no-tdf-decode specified)\n");
        printf("   -print-json        print the decoded tdfs in JSON format (supercedes -print-tdf, but ignored if -no-tdf-decode specified)\n");
        printf("   -no-strict-decode  allow lax tdf decoding (ignored if -no-tdf-decode specified)\n");
        printf("   -override          additional file to use for override\n");
        printf("   -output-file       file to output values to\n");
        printf("   configfile         config file to parse\n");
        printf("   key                key of config file section to print (only works for printing in raw config file format -- not for printing in TDF or JSON format)\n");
        return 1;
    }

    static bool endsWith(const char8_t* filename, const char8_t* extension)
    {
        const char8_t* found = strstr(filename, extension);
        return (found != nullptr && found[strlen(extension)] == '\0');
    }

    int printFile(int argc, char *argv[], int keyIndex, const ConfigFile* config, FILE* outputFile)
    {
        int result = 0;
        BLAZE_INFO_LOG(gLogCategory, "file: " << config->getFilePath().c_str() << "\n");

        if (keyIndex == -1)
        {
            eastl::string val;
            config->toString(val);
            if (outputFile == nullptr)
                printf("%s\n", val.c_str());
            else
                fprintf(outputFile, "%s\n", val.c_str());
        }
        else
        {
            int32_t numFound = 0;
            for (int32_t idx = keyIndex; idx < argc; ++idx)
            {

                const ConfigItem* item = config->getItem(argv[idx]);
                if (item != nullptr)
                {
                    eastl::string itemValue;
                    switch (item->getItemType())
                    {
                    case ConfigItem::CONFIG_ITEM_MAP:
                        item->getMap()->toString(itemValue);
                        break;
                    case ConfigItem::CONFIG_ITEM_SEQUENCE:
                        item->getSequence()->toString(itemValue);
                        break;
                    case ConfigItem::CONFIG_ITEM_STRING:
                    {
                        bool isDefault = false;
                        itemValue = item->getString("", isDefault);
                        break;
                    }
                    default:
                        break;
                    }
                    if (outputFile == nullptr)
                        printf("%s\n\n", itemValue.c_str());
                    else
                        fprintf(outputFile, "%s\n\n", itemValue.c_str());
                    ++numFound;
                }
            }
            result = numFound;
        }
        return result;
    }

    void printTdf(const char8_t* name, const EA::TDF::Tdf& tdf, FILE* outputFile)
    {
        StringBuilder s;
        s << tdf;
        if (outputFile == nullptr)
        {
            printf("%s = %s\n\n", name, s.get());
        }
        else
        {
            fprintf(outputFile, "%s = %s\n\n", name, s.get());
        }
    }

    void printOverallJson(const EA::TDF::Tdf& tdf, FILE* outputFile)
    {
        EA::TDF::JsonEncoder encoder;
        EA::TDF::MemberVisitOptions visitOpt;
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        visitOpt.onlyIfSet = true;
#endif

        RawBuffer s(2048);
        RawBufferIStream istream(s);
        encoder.setIncludeTDFMetaData(false);
        encoder.encode(istream, tdf, nullptr, visitOpt);

        if (outputFile == nullptr)
        {
            printf("%s", reinterpret_cast<char8_t*>(s.data()));
        }
        else
        {
            fprintf(outputFile, "%s", reinterpret_cast<char8_t*>(s.data()));
        }
    }

    struct ConfigDef
    {
        bool isComponent;
        EA::TDF::TdfPtr configTdf;
        EA::TDF::TdfPtr preconfigTdf;

        ConfigDef() : isComponent(false), configTdf(nullptr), preconfigTdf(nullptr) {}
        ~ConfigDef()
        {
        }
    };

    int process(int argc, char *argv[])
    {
        int32_t keyIndex = -1;

        const char8_t* configFile = nullptr;
        const char8_t* overrideFile = nullptr;
        const char8_t* outputFilename = nullptr;
        FILE* outputFile = nullptr;

        PredefineMap predefineMap;
        predefineMap.addBuiltInValues();
        EA::TDF::TdfFactory::fixupTypes();

        bool allowUnquotedStrings = false;
        bool decodeTdfs = true;
        bool printTdfs = false;
        bool printJson = false;
        DecodedConfigs decodedCfgs;
        bool strictTdfDecode = true;
        eastl::vector<eastl::string> serverTypeArgList;

        for (int arg = 1; arg < argc; ++arg)
        {
            if ((argv[arg][0] == '-') && (argv[arg][1] == 'D'))
            {
                predefineMap.addFromKeyValuePair(&argv[arg][2]);
            }
            else if ((argv[arg][0] == '-') && (argv[arg][1] == 'V'))
            {
                Blaze::Logger::initializeCategoryLevel("cfgtest", Blaze::Logging::INFO);
            }
            else if ((argv[arg][0] == '-') && (argv[arg][1] == 'Q'))
            {
                allowUnquotedStrings = true;
            }
            else if (blaze_strncmp(argv[arg], "-server-type", sizeof("-server-type")) == 0)
            {
                serverTypeArgList.push_back(argv[++arg]);
            }
            else if (blaze_strncmp(argv[arg], "-no-tdf-decode", sizeof("-no-tdf-decode")) == 0)
            {
                decodeTdfs = false;
            }
            else if (blaze_strncmp(argv[arg], "-print-tdf", sizeof("-print-tdf")) == 0)
            {
                printTdfs = true;
            }
            else if (blaze_strncmp(argv[arg], "-print-json", sizeof("-print-json")) == 0)
            {
                printJson = true;
            }
            else if (blaze_strncmp(argv[arg], "-no-strict-decode", sizeof("-no-strict-decode")) == 0)
            {
                strictTdfDecode = false;
            }
            else if (blaze_strncmp(argv[arg], "-override", sizeof("-override")) == 0)
            {
                overrideFile = argv[++arg];
            }
            else if (blaze_strncmp(argv[arg], "-output-file", sizeof("-output-file")) == 0)
            {
                outputFilename = argv[++arg];
            }
            else if (argv[arg][0] == '-')
            {
                return usage(argv[0]);
            }
            else if (configFile == nullptr)
            {
                configFile = argv[arg];
            }
            else
            {
                keyIndex = arg;
                break;
            }
        }

        //can't print tdfs if we can't decode them
        if (!decodeTdfs)
        {
            printTdfs = false;
            printJson = false;
        }

        if (configFile == nullptr)
        {
            usage(argv[0]);
            return 0;
        }

        if (overrideFile == nullptr)
        {
            overrideFile = "";
        }

        if (outputFilename != nullptr)
        {
            outputFile = fopen(outputFilename, "w");
            if (outputFile == nullptr)
            {
                printf("Unable to open output file %s for write. Error %d. Outputting to stdout instead.", outputFilename, errno);
            }
        }

        int result = 0;

        const bool isBootFile = endsWith(configFile, ".boot");
        if (isBootFile)
        {
            ConfigBootUtil bootUtil(configFile, overrideFile, predefineMap, true);
            if (!bootUtil.initialize())
            {
                if (outputFile != nullptr)
                {
                    fclose(outputFile);
                    outputFile = nullptr;
                }

                BLAZE_ERR_LOG(gLogCategory, "Unable to decode config file to TDF.\n");
                return 0;
            }

            //Now we can print out the boot file
            if (printJson)
            {
                // not printing JSON yet, just collecting...
                EA::TDF::VariableTdfBase* resultTdf = BLAZE_NEW EA::TDF::VariableTdfBase();
                resultTdf->set(bootUtil.getConfigTdf().clone());
                decodedCfgs.getConfigTdfs()["boot"] = resultTdf;
            }
            else if (printTdfs)
            {
                printTdf("boot", bootUtil.getConfigTdf(), outputFile);
            }
            else
            {
                result += printFile(argc, argv, keyIndex, bootUtil.getConfigFile(), outputFile);
            }

            typedef eastl::map<eastl::string, ConfigDef> ConfigNameMap;
            const BootConfig& bootConfig = bootUtil.getConfigTdf();
            ConfigNameMap configNames;

            // false because not components
            ConfigDef& frameDef = configNames["framework"];
            frameDef.isComponent = false;
            frameDef.configTdf = BLAZE_NEW Blaze::FrameworkConfig;

            ConfigDef& loggingDef = configNames["logging"];
            loggingDef.isComponent = false;
            loggingDef.configTdf = BLAZE_NEW Blaze::LoggingConfig;

            if (serverTypeArgList.empty())
            {
                const ServerInstanceConfigList& instanceList = bootUtil.getConfigTdf().getServerInstances();

                for (ServerInstanceConfigList::const_iterator iitr = instanceList.begin(), iend = instanceList.end(); iitr != iend; ++iitr)
                {
                    serverTypeArgList.push_back((**iitr).getType());
                }
            }

            for (const eastl::string& serverTypeArg : serverTypeArgList)
            {
                //Go through the instances and find the components in each one
                BootConfig::ServerConfigsMap::const_iterator citr = bootConfig.getServerConfigs().find(serverTypeArg.c_str());
                if (citr != bootConfig.getServerConfigs().end())
                {
                    //Go through the list of component groups and add their components
                    const ServerConfig::StringComponentsList& compList = citr->second->getComponents();
                    for (ServerConfig::StringComponentsList::const_iterator compItr = compList.begin(), compEnd = compList.end();
                        compItr != compEnd; ++compItr)
                    {
                        BootConfig::ComponentGroupsMap::const_iterator g = bootConfig.getComponentGroups().find(compItr->c_str());
                        if (g != bootConfig.getComponentGroups().end())
                        {
                            const BootConfig::ComponentGroup::StringComponentsList& cnames = g->second->getComponents();
                            for (BootConfig::ComponentGroup::StringComponentsList::const_iterator
                                cname = cnames.begin(), ecname = cnames.end(); cname != ecname; ++cname)
                            {
                                // true because components
                                ConfigDef& compDef = configNames[cname->c_str()];
                                compDef.isComponent = true;

                                if (decodeTdfs)
                                {
                                    ComponentId compId = BlazeRpcComponentDb::getComponentIdByName(cname->c_str());
                                    if (compId != 0)
                                    {
                                        compDef.configTdf = BlazeRpcComponentDb::createConfigTdf(compId);
                                        compDef.preconfigTdf = BlazeRpcComponentDb::createPreconfigTdf(compId);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            for (ConfigNameMap::iterator i = configNames.begin(), e = configNames.end(); i != e; ++i)
            {
                uint32_t flags = ConfigBootUtil::CONFIG_CREATE_DUMMY;
                if (i->second.isComponent)
                    flags |= ConfigBootUtil::CONFIG_IS_COMPONENT;
                ConfigFile* subConfig = bootUtil.createFrom(i->first.c_str(), flags);

                if (subConfig != nullptr)
                {
                    EA::TDF::Tdf* configTdf = i->second.configTdf;

                    if (decodeTdfs)
                    {
                        const ConfigMap& configMap = subConfig->getFeatureConfigMap(i->first.c_str());
                        if (configTdf != nullptr)
                        {
                            ConfigDecoder decoder(configMap, strictTdfDecode);
                            if (!decoder.decode(configTdf))
                            {
                                BLAZE_ERR_LOG(gLogCategory, "Failed to decode TDF for component " << i->first.c_str() << "\n");
                                break;
                            }
                        }
                        else
                        {
                            BLAZE_WARN_LOG(gLogCategory, "No TDF to decode into for component " << i->first.c_str() << "\n");
                        }
                    }

                    if (printJson)
                    {
                        // add next element/object -- or ignore if there isn't a config TDF, because we don't want to mix the raw config file with the JSON we're building here
                        if (configTdf != nullptr)
                        {
                            EA::TDF::VariableTdfBase* resultTdf = BLAZE_NEW EA::TDF::VariableTdfBase();
                            resultTdf->set(configTdf);
                            decodedCfgs.getConfigTdfs()[i->first.c_str()] = resultTdf;
                        }
                    }
                    else if (printTdfs && configTdf != nullptr)
                    {
                        printTdf(i->first.c_str(), *configTdf, outputFile);
                    }
                    else
                    {
                        result += printFile(argc, argv, keyIndex, subConfig, outputFile);
                    }

                    delete subConfig;
                }
                else
                {
                    BLAZE_ERR_LOG(gLogCategory, "Failed to open sub-config file " << i->first.c_str() << "\n");
                    break;
                }
            }

            if (printJson)
            {
                printOverallJson(decodedCfgs, outputFile);
            }
        }
        else
        {
            //If this is not a boot file we don't have context to print it as tdf
            // load/process the file
            ConfigFile* config = ConfigFile::createFromFile("./", configFile, "./", overrideFile, allowUnquotedStrings, predefineMap);
            if (config == nullptr)
            {
                if (outputFile != nullptr)
                {
                    fclose(outputFile);
                    outputFile = nullptr;
                }

                BLAZE_ERR_LOG(gLogCategory, "Unable to read/parse config file.\n");
                return 0;
            }

            result = printFile(argc, argv, keyIndex, config, outputFile);

            delete config;
        }

        if (outputFile != nullptr)
        {
            fclose(outputFile);
            outputFile = nullptr;
        }

        EA::TDF::TdfFactory::cleanupTypeAllocations();


        return result;
    }


    int cfgtest_main(int argc, char** argv)
    {
        // Must initialize fiber storage before Logger
        Fiber::FiberStorageInitializer fiberStorage;

        BlazeRpcError err = Logger::initialize();
        if (err != Blaze::ERR_OK)
        {
            printf("Unable to initialize logger.\n");
            return 1;
        }

        BlazeRpcComponentDb::initialize();

        Blaze::Logger::registerCategory(gLogCategory, "cfgtest");
        Blaze::Logger::setLevel(Blaze::Logging::WARN);
        Blaze::Logger::initializeCategoryDefaults();

        int result = process(argc, argv);

        Logger::destroy();

        //process exit status: zero is success, non-zero is failure.
        return (result == 0) ? 1 : 0;
    }
}

