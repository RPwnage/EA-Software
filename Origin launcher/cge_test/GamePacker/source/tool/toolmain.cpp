/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include <EABase/eabase.h>
#include <EAAssert/eaassert.h>
#include <EASTL/sort.h>
#include <EASTL/string.h>
#include <EASTL/array.h>
#include <EASTL/vector_map.h>
#include <EASTL/vector_set.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/bonus/adaptors.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EAScanf.h>
#include <EAStdC/EAStopwatch.h>
#include <EAStdC/EADateTime.h>
#include <EAStdC/EACType.h>
#include <EAMain/EAMain.h>
#include <EAIO/PathString.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAFileDirectory.h>
#include <eathread/eathread.h>
#include "gamepacker/internal/packerinternal.h"
#include "gamepacker/packer.h"
#include "gamepacker/handlers.h"
#include "toolmain.h"


using namespace Packer;


typedef eastl::vector<int64_t> ValueList;
typedef eastl::vector<double> MetricValueList;
typedef eastl::map<int, int> PartySizeFreqMap;
typedef eastl::map<int, double> PercentileMap;
struct SummaryMetrics : FactorMetrics
{
    MetricValueList values;
    void accumulateMetric(double value)
    {
        FactorMetrics::accumulateMetric(value);
        values.push_back(value);
    }
};

struct Tokenizer
{
    Tokenizer(char8_t* input, const char8_t* inputDelimiters);
    char8_t* getToken();
    char8_t* nextToken();

    char8_t* context;
    const char8_t* delimiters;
    char8_t* token;
};

Tokenizer::Tokenizer(char8_t* input, const char8_t* inputDelimiters) : context(nullptr), delimiters(inputDelimiters)
{
    token = EA::StdC::Strtok(input, delimiters, &context);
}

char8_t* Tokenizer::getToken()
{
    return token;
}

char8_t* Tokenizer::nextToken()
{
    token = EA::StdC::Strtok(NULL, delimiters, &context);
    return token;
}

enum class RequiredPlayerDataField
{
    GAME_ID,
    PARTY_ID,
    PLAYER_ID,
    TEAM_INDEX,
    E_COUNT
};

enum class RequiredPartyDataField
{
    GAME_ID,
    PARTY_ID,
    IMMUTABLE,
    PLAYER_COUNT,
    TEAM_INDEX,
    E_COUNT
};

struct CommandLineOption
{
    const char8_t* optionName;
    const char8_t* optionAbbreviation;
    bool matchOption(EA::EAMain::CommandLine& commandLine, int32_t& result) const;
};

static CommandLineOption cmdOptConfig               = { "-config","-cfg" };
static CommandLineOption cmdOptReport               = { "-report","-r" };
static CommandLineOption cmdOptTrace                = { "-trace","-t" };
static CommandLineOption cmdOptPlayers              = { "-players","-p" };
static CommandLineOption cmdOptTeamCapacity         = { "-teamcapacity","-tc" };
static CommandLineOption cmdOptMaxWorkingSet        = { "-maxworkingset","-mws" };
static CommandLineOption cmdOptPlayerProperties     = { "-playerproperties","-pp" };
static CommandLineOption cmdOptQualityFactors       = { "-qualityfactors","-qf" };
static CommandLineOption cmdOptQualityFactorMode    = { "-qualityfactormode","-qfm" };
static CommandLineOption cmdOptPlayerData           = { "-playerdata","-pd" };
static CommandLineOption cmdOptPartyData            = { "-partydata","-td" };
static CommandLineOption cmdOptPartySize            = { "-partysize","-ps" };

static CommandLineOption* gCommandLineOptions[] = {
    &cmdOptConfig,
    &cmdOptReport,
    &cmdOptTrace,
    &cmdOptPlayers,
    &cmdOptTeamCapacity,
    &cmdOptMaxWorkingSet,
    &cmdOptPlayerProperties,
    &cmdOptQualityFactors,
    &cmdOptQualityFactorMode,
    &cmdOptPlayerData,
    &cmdOptPartyData,
    &cmdOptPartySize
};

static const char* gReportModeNames[(int32_t)ReportMode::E_COUNT] = {
    "summary", 
    "games", 
    "parties",
    "players", 
    "workingset" 
};

static const char* gTraceModeNames[(int32_t)TraceMode::E_COUNT] = {
    "games",
    "players", 
    "algo" 
};


static const int32_t MIN_PLAYER_REPORT_COLUMNS = (int32_t)RequiredPlayerDataField::E_COUNT; // used for both output and input
static const int32_t MIN_PARTY_REPORT_COLUMNS = 5; // used for both output and input

static const PropertyValue UNSPECIFIED_PROPERTY_FIELD_VALUE = -1; // data marker used to indicate that the player does not have a value for this field (field CAN be used in packing the game)
static const PropertyValue OMITTED_PROPERTY_FIELD_VALUE = -2; // data marker used to indicate that the player does not use this field (field CANNOT be used in packing the game)

static void generateChiSquaredData(ValueList& chidata);
static void initializeMetric(SummaryMetrics& data, int64_t capacity);
static void completeMetric(SummaryMetrics& data);
static double computeSigma(const SummaryMetrics& data);
static void computePercentiles(const SummaryMetrics& data, PercentileMap& percentiles);
static int32_t usage(const char* prg);
static void errorExit(const char* fmt, ...);
static void printOrderedSubsequences(int* maxmin, int* minmax, int32_t arraysize, int32_t seqlen);
static void enumerateOrderedSubsequences();
static bool findFile(const char8_t* relativefilepath, eastl::string& absolutefilepath);
static bool getLine(FILE* f, eastl::string& line);
static void cutString(eastl::string& first, eastl::string& second, const eastl::string& from, const char8_t ch);

namespace GameSizeFactor {
    extern struct FactorTransform gTransform;
}
namespace TeamSizeFactor {
    extern struct FactorTransform gTransform;
}

struct DefaultUtilityMethods : public PackerUtilityMethods {
    GameId mGameId = 0;
    uint32_t mTraceModeBits = 0;
    eastl::vector_set<int64_t> mDebugTraceFilterMap[(uint32_t)TraceMode::E_COUNT];

    Time utilPackerMeasureTime() override
    {
        return 0;
    }

    GameId utilPackerMakeGameId(PackerSilo& packer) override
    {
        return ++mGameId;
    }

    void utilPackerUpdateMetric(PackerSilo& packer, PackerMetricType type, int64_t delta) override
    {
    }

    void utilPackerLog(Packer::PackerSilo& packer, Packer::LogLevel level, const char8_t* fmt, va_list args) override
    {
        vprintf(fmt, args);
        if (level == LogLevel::TRACE)
            printf("\n");
    }

    bool utilPackerTraceEnabled(TraceMode mode) const override
    {
        return (mTraceModeBits & (1 << (uint32_t)mode)) != 0;
    }

    bool utilPackerTraceFilterEnabled(TraceMode mode, int64_t entityId) const override
    {
        if ((mTraceModeBits & (1 << (uint32_t)mode)) == 0)
            return false;
        const auto& filtermap = mDebugTraceFilterMap[(uint32_t)mode];
        if (filtermap.empty())
            return true;
        return filtermap.find(entityId) != filtermap.end();
    }

    int32_t utilRand() override
    {
        return rand();
    }

    void enableTrace(TraceMode mode, bool enable)
    {
        if (enable)
            mTraceModeBits |= (1 << (uint32_t)mode);
        else
            mTraceModeBits &= ~(1 << (uint32_t)mode);
    }

    void enableTraceFilter(TraceMode mode, int64_t filter)
    {
        mDebugTraceFilterMap[(uint32_t)mode].insert(filter);
    }

};


int32_t main(int32_t argc, char *argv[])
{
    DefaultUtilityMethods utilityMethods;
    
    PackerSilo packer; // initialize packer
    packer.setUtilityMethods(utilityMethods);
    packer.setEvalSeedMode(GameEvalRandomSeedMode::NONE); // turn off randomization to ensure test result repeatability
    int32_t matchResult;
    eastl::unique_ptr<EA::EAMain::CommandLine> commandLine(new EA::EAMain::CommandLine(argc, argv));
    if (argc <= 1 || commandLine->HasHelpSwitch() || commandLine->FindSwitch("--help") >= 0)
    {
        exit(usage(argv[0]));
    }
    eastl::string configFilePath;
    if (cmdOptConfig.matchOption(*commandLine, matchResult))
    {
        const char8_t* arg = commandLine->Arg(matchResult + 1);
        eastl::string tryconfigpath;
        if (findFile(arg, tryconfigpath))
        {
            eastl::string configContents;
            configContents.reserve(200);
            FILE* f = fopen(tryconfigpath.c_str(), "r");
            if (f != nullptr)
            {
                size_t readcount = 0;
                size_t totalread = 0;
                while ((readcount = fread(configContents.begin() + configContents.size(), 1, configContents.capacity() - configContents.size(), f)) > 0)
                {
                    totalread += readcount;
                    configContents.force_size(configContents.size() + readcount);
                    configContents[totalread] = '\0'; // terminate the string provisionally
                    if (configContents.capacity() - configContents.size() < 100)
                        configContents.reserve(configContents.capacity() * 2);
                }
                fclose(f);
                configFilePath = tryconfigpath;
                commandLine.reset(new EA::EAMain::CommandLine(configContents.c_str())); // replace bootstrap command line object with one we formed from the input of the config file
            }
            else
                errorExit("failed to open config file: %s", tryconfigpath.c_str());
        }
    }
    else
        configFilePath = "none";

    if (cmdOptReport.matchOption(*commandLine, matchResult))
    {
        const char8_t* arg = commandLine->Arg(matchResult + 1);
        for (int32_t i = 0; i < (int32_t)ReportMode::E_COUNT; ++i)
        {
            if (strstr(arg, gReportModeNames[i]) != NULL)
                packer.setReport((ReportMode)i);
        }
    }
    else
        packer.setReport(ReportMode::SUMMARY);

    if (cmdOptTrace.matchOption(*commandLine, matchResult))
    {
        eastl::string arg = commandLine->Arg(matchResult + 1);
        eastl::vector<char8_t*> contextStack;
        const char8_t* traceModesDelimiter = ";";
        const char8_t* traceModeNameDelimiter = ":";
        const char8_t* traceModeFilterDelimiter = ",";
        contextStack.push_back(NULL);
        char8_t* token = EA::StdC::Strtok(arg.begin(), traceModesDelimiter, &contextStack.back());
        while (token != nullptr)
        {
            contextStack.push_back(NULL);
            // extract tracemode name token
            token = EA::StdC::Strtok(token, traceModeNameDelimiter, &contextStack.back());
            TraceMode traceMode = TraceMode::E_COUNT;
            eastl::string traceModeName;
            if (token != nullptr)
            {
                traceModeName = token;
                for (int32_t i = 0; i < (int32_t)TraceMode::E_COUNT; ++i)
                {
                    if (traceModeName == gTraceModeNames[i])
                    {
                        traceMode = (TraceMode)i;
                        utilityMethods.enableTrace(traceMode, true);
                        break;
                    }
                }
            }
            if (traceMode == TraceMode::E_COUNT)
            {
                errorExit("invalid tracemode: %s.\n", traceModeName.c_str());
            }
            // extract tracemode filterlist token
            token = EA::StdC::Strtok(NULL, traceModeNameDelimiter, &contextStack.back());
            contextStack.pop_back();
            if (token != nullptr)
            {
                contextStack.push_back(NULL);
                token = EA::StdC::Strtok(token, traceModeFilterDelimiter, &contextStack.back());
                while (token != nullptr)
                {
                    int32_t elementValue = EA::StdC::AtoI32(token);
                    utilityMethods.enableTraceFilter(traceMode, elementValue);
                    token = EA::StdC::Strtok(NULL, traceModeFilterDelimiter, &contextStack.back());
                }
                contextStack.pop_back();
            }
            token = EA::StdC::Strtok(NULL, traceModesDelimiter, &contextStack.back());
        }
    }

    int32_t maxPlayers = 120; // TODO: use maxGames instead and calculate from team capacity*teamcount*maxGames

    if (cmdOptPlayers.matchOption(*commandLine, matchResult))
    {
        const char8_t* arg = commandLine->Arg(matchResult + 1);
        int32_t num = EA::StdC::AtoI32(arg);
        if (num > 0)
            maxPlayers = num;
    }

    int32_t teamCount = MAX_TEAM_COUNT; // TODO: set this from command line params
    int32_t teamCapacity = 4;

    if (cmdOptTeamCapacity.matchOption(*commandLine, matchResult))
    {
        const char8_t* arg = commandLine->Arg(matchResult + 1);
        int32_t num = EA::StdC::AtoI32(arg);
        if (num > 0)
            teamCapacity = (decltype(teamCapacity))num;
    }
    int32_t gameCapacity = teamCount * teamCapacity;
    EA_ASSERT(gameCapacity <= MAX_PARTY_COUNT);

    packer.setVar(GameTemplateVar::GAME_CAPACITY, gameCapacity);
    packer.setVar(GameTemplateVar::TEAM_CAPACITY, teamCapacity);
    packer.setVar(GameTemplateVar::TEAM_COUNT, teamCount);

    if (cmdOptMaxWorkingSet.matchOption(*commandLine, matchResult))
    {
        const char8_t* arg = commandLine->Arg(matchResult + 1);
        int32_t num = EA::StdC::AtoI32(arg);
        if (num >= 0)
            packer.setMaxWorkingSet(num);
    }

    int32_t numPlayerPropertyFields = 0;
    
    struct FieldDescriptorUsage {
        const FieldDescriptor* fieldDesc = nullptr;
        // NOTE: specifies whether this particular field is actually used in the -qualityfactor option of the tool,
        // only 'used' fields have data to be loaded into packer and their summary written to the report.
        bool used = false;
    };
    eastl::list<FieldDescriptorUsage> fieldUsageList; // used to link command line to actual propeties when its time to read the data file, *needs* to be a list because PropertyFieldBinding points directly to it!

    // This stucture is meant to make it a bit easier to configure the tool
    // by enabling a quality factor to be bound to one or more fields explicitly
    // and to track that info for ease of reporting.
    struct PropertyFieldBinding {
        eastl::vector<FieldDescriptorUsage*> fieldUsageList;
        const PropertyDescriptor* propertyDesc = nullptr;
    };

    eastl::vector<PropertyFieldBinding> propertyBindingList; // this represents the groups specified on the -playerproperties option

    if (cmdOptPlayerProperties.matchOption(*commandLine, matchResult))
    {
        eastl::string arg = commandLine->Arg(matchResult + 1);
        char8_t* context = NULL;
        const char8_t* delimiter = ",";
        char8_t* token = EA::StdC::Strtok(arg.begin(), delimiter, &context);
        while (token != nullptr)
        {
            eastl::string propertyName, fieldRemainder;
            cutString(propertyName, fieldRemainder, token, '/');
            if (propertyName.empty())
                errorExit("invalid property format: %s", token);
            auto& bindInfo = propertyBindingList.push_back();
            while (!fieldRemainder.empty())
            {
                eastl::string fieldName;
                cutString(fieldName, fieldRemainder, fieldRemainder, '+');
                if (fieldName.empty())
                    errorExit("invalid property format: %s, remainder: %s", token, fieldRemainder.c_str());
                auto& fieldDescriptor = packer.addPropertyField(propertyName, fieldName);
                bindInfo.propertyDesc = fieldDescriptor.mPropertyDescriptor;
                fieldUsageList.push_back({ &fieldDescriptor, false }); // add unique field defintion
                bindInfo.fieldUsageList.push_back(&fieldUsageList.back()); // bind property to field usage
                numPlayerPropertyFields++;
            }

            token = EA::StdC::Strtok(NULL, delimiter, &context);
        }
    }
    else
        numPlayerPropertyFields = 1;

    if (cmdOptQualityFactors.matchOption(*commandLine, matchResult))
    {
        eastl::string arg = commandLine->Arg(matchResult + 1);
        char8_t* context = NULL;
        const char8_t* delimiter = ",";
        char8_t* token = EA::StdC::Strtok(arg.begin(), delimiter, &context);
        while (token != nullptr)
        {
            // Format:  ((optional)propertyName/propertyFieldIndex).transform((optional):shaperType.shaperParamName=paramValue)
            eastl::string factorSpec = token;

            eastl::string transformAndShaper;
            eastl::string propertyNameAndField;
            cutString(propertyNameAndField, transformAndShaper, factorSpec, '.');

            eastl::string propertyName;
            eastl::string fieldName;
            cutString(propertyName, fieldName, propertyNameAndField, '/');

            if (!propertyName.empty())
            {
                if (fieldName.empty())
                {
                    bool found = false;
                    for (auto& binding : propertyBindingList)
                    {
                        if (binding.propertyDesc->mPropertyName == propertyName)
                        {
                            if (binding.fieldUsageList.empty())
                            {
                                errorExit("factor(%s) referenced property(%s) must have one or more fields!", factorSpec.c_str(), propertyName.c_str());
                            }
                            else
                            {
                                for (auto& fieldUsageItr : binding.fieldUsageList)
                                {
                                    fieldUsageItr->used = true; // mark all fields referenced from the binding as used, sine the binding is used
                                }
                                found = true;
                                break;
                            }
                        }
                    }
                    if (!found)
                        errorExit("factor(%s) referenced property(%s) must be in the -playerproperties list!", factorSpec.c_str(), propertyName.c_str());
                }
                else
                {
                    bool found = false;
                    for (auto& binding : propertyBindingList)
                    {
                        if (binding.propertyDesc->mPropertyName == propertyName)
                        {
                            if (binding.fieldUsageList.size() != 1)
                            {
                                // multi-field bindings are skipped for now... since our format only supports referring to one
                                continue;
                            }
                            auto& fieldUsage = binding.fieldUsageList.front();
                            if (fieldUsage->fieldDesc->mFieldName == fieldName)
                            {
                                fieldUsage->used = true;
                                found = true;
                                break;
                            }
                        }
                    }
                    if (!found)
                        errorExit("factor(%s) referenced property(%s) and field(%s) combo must be in the -playerproperties list!", factorSpec.c_str(), propertyName.c_str(), fieldName.c_str());
                }
            }

            eastl::string transformName;
            eastl::string factorShaperSpec;
            cutString(transformName, factorShaperSpec, transformAndShaper, ':');

            eastl::string factorShaperTypeSpec;
            eastl::string factorShaperParam;
            cutString(factorShaperTypeSpec, factorShaperParam, factorShaperSpec, '=');

            PropertyAggregate shaperType = PropertyAggregate::SUM;
            float shaperParam = 0; // parameter used to control rank calculation
            if (!factorShaperTypeSpec.empty())
            {
                eastl::string factorShaperType;
                eastl::string factorShaperVariable; // currently unused
                cutString(factorShaperType, factorShaperVariable, factorShaperTypeSpec, '.');

                if (factorShaperType == "range")
                    shaperType = PropertyAggregate::MINMAXRANGE;
                else if (factorShaperType == "ratio")
                    shaperType = PropertyAggregate::MINMAXRATIO;

                shaperParam = EA::StdC::AtoF32(factorShaperParam.c_str());
                if (shaperParam < 0)
                {
                    errorExit("invalid qualityfactor shaper: %s, param must be positive.\n", factorShaperTypeSpec.c_str());
                }
            }
            else
            {
                shaperType = PropertyAggregate::MINMAXRANGE; // default shapertype to range
            }

            EA_ASSERT(shaperType < PropertyAggregate::E_COUNT);

            GameQualityFactorConfig config;
            config.mPropertyName = propertyName;
            config.mTransform = transformName;

            config.mTeamAggregate = shaperType;

            if (shaperType == PropertyAggregate::MINMAXRATIO)
            {
                config.mBestValue   = 1;
                config.mViableValue = 1;
                config.mWorstValue  = 0;
                if (config.mTransform == "avg")
                {
                    config.mBestValue = 0;
                    config.mViableValue = 1;
                    config.mWorstValue = 1;
                }
            }
            else
            {
                // Hardcoded defaults
                if (config.mTransform == "max2")
                {
                    config.mBestValue = 0;
                    config.mViableValue = 0;
                    config.mWorstValue = 10000;
                }
                else if (config.mTransform == "size.min")
                {
                    config.mBestValue = (float)packer.getVar(GameTemplateVar::TEAM_CAPACITY);
                    config.mViableValue = 4;
                    config.mWorstValue = 0;
                }
                else if (config.mTransform == "sum")
                {
                    config.mBestValue = 0;
                    config.mViableValue = 0;
                    config.mWorstValue = 50000; // PACKER_TODO: default values in the generated input files used by the GamePackerTool are in range 0..10000, with 5 players per side. Need to have this value computed and set from the GamePackerTool directly.
                }
                else if (config.mTransform == "size")
                {
                    config.mBestValue = (float)packer.getVar(GameTemplateVar::GAME_CAPACITY);
                    config.mGoodEnoughValue = config.mBestValue; // To test good enough value change this line to something other than best...
                    config.mViableValue = config.mGoodEnoughValue;
                    config.mWorstValue = 0;
                }
                else if (config.mTransform == "minmax")  // min max delta
                {
                    config.mBestValue = 0;
                    config.mViableValue = 0;
                    config.mWorstValue = 10000;
                }
                else if (config.mTransform == "avg")
                {
                    config.mBestValue = 0;
                    //config.mGoodEnoughValue = 30; // To test the goodEnoughValue uncomment this, it should significantly boost the quality of subsequent factors.
                    config.mViableValue = 100;
                    config.mWorstValue = 10000;
                }
            }

            if (!factorShaperTypeSpec.empty())
            {
                config.mGranularity = shaperParam;
            }

            if (!packer.addFactor(config))
                errorExit("Failed to add factor!");

            token = EA::StdC::Strtok(NULL, delimiter, &context);
        }

        if (packer.getFactors().empty())
        {
            errorExit("invalid qualityfactors.\n");
        }
    }
    else
    {
        errorExit("required qualityfactors.\n");
    }

    size_t numSourcePlayerFields = 0;
    for (auto& fieldUsage : fieldUsageList)
    {
        if (fieldUsage.used)
            numSourcePlayerFields++;
    }

    if (cmdOptQualityFactorMode.matchOption(*commandLine, matchResult))
    {
        const char8_t* arg = commandLine->Arg(matchResult + 1);
        int32_t num = EA::StdC::AtoI32(arg);
        if (num > 0)
            packer.setDebugFactorMode(num);
    }

    eastl::vector_map<int, int> testPartyCountBySizeMap;

    eastl::string playerDataPath;
    eastl::string partyDataPath;
    ValueDistributions distrib = DIST_MANUAL;
    eastl::vector<PropertyValue> playerPropertyMax;
    eastl::vector<PropertyValue> playerPropertyMin;

    EA::StdC::Stopwatch stopWatch(EA::StdC::Stopwatch::kUnitsSeconds);
    stopWatch.SetElapsedTime(0);

    if (cmdOptPlayerData.matchOption(*commandLine, matchResult))
    {
        // The reason for this is because we use the FieldId to track the position of the field in the input data file
        EA_ASSERT_MSG(packer.getFieldsCreatedCount() == packer.getPropertyFieldCount(), "Field removal not supported by the tool!");

        const char8_t* arg = commandLine->Arg(matchResult + 1);
        eastl::string tryInputPath;
        if (findFile(arg, tryInputPath))
        {
            eastl::string configContents;
            configContents.reserve(200);
            FILE* f = fopen(tryInputPath.c_str(), "r");
            if (f != nullptr)
            {
                eastl::vector<PropertyValue> fieldValues;
                const auto expectedMinColumns = MIN_PLAYER_REPORT_COLUMNS + numPlayerPropertyFields;
                fieldValues.reserve(expectedMinColumns);
                // read player data from standard input
                int32_t uniqueParties = 0;
                eastl::string line;
                bool more = false;
                do
                {
                    line.clear();
                    fieldValues.clear();
                    more = getLine(f, line);
                    // line format:
                    // gameid partyid playerid team Property0,Property1,...
                    if (line[0] != ' ') // skip comments and headers
                        continue;
                    char8_t* context = NULL;
                    const char8_t* delimiter = " ";
                    char8_t* token = EA::StdC::Strtok(line.begin(), delimiter, &context);
                    while (token != nullptr)
                    {
                        auto fieldValue = EA::StdC::AtoI64(token);
                        fieldValues.push_back(fieldValue);
                        token = EA::StdC::Strtok(NULL, delimiter, &context);
                    }

                    if ((int32_t)fieldValues.size() < expectedMinColumns)
                    {
                        errorExit("insufficient number(%d) of player data fields, expected(%d).\n", (int32_t)fieldValues.size(), expectedMinColumns);
                    }
                    const auto partyId = (PartyId)fieldValues[(int32_t)RequiredPlayerDataField::PARTY_ID];
                    const auto playerId = (PlayerId)fieldValues[(int32_t)RequiredPlayerDataField::PLAYER_ID];
                    Time dummyExpiryTime = (Time)partyId; // NOTE: expire in order of id
                    if (packer.addPackerParty(partyId, dummyExpiryTime, dummyExpiryTime))
                    {
                        uniqueParties++;
                    }
                    auto& party = *packer.getPackerParty(partyId);
                    auto partyPlayerIndex = packer.addPackerPlayer(playerId, party);

                    for (auto& fieldUsage : fieldUsageList)
                    {
                        if (!fieldUsage.used)
                            continue;
                        auto fieldOffset = fieldUsage.fieldDesc->mFieldId - 1; // NOTE: The field id is always incremented, so it can be used to track the order of the insertion of the fields, despite the map not being sorted, because the tool never removes any fields!
                        auto fieldValue = fieldValues[(int32_t)RequiredPlayerDataField::E_COUNT + fieldOffset];
                        if (fieldValue >= 0)
                        {
                            // if value is valid, then set it
                            party.setPlayerPropertyValue(fieldUsage.fieldDesc, partyPlayerIndex, &fieldValue);
                        }
                        else if (fieldValue == UNSPECIFIED_PROPERTY_FIELD_VALUE)
                        {
                            party.setPlayerPropertyValue(fieldUsage.fieldDesc, partyPlayerIndex, nullptr);
                        }
                    }

                    if (party.getPlayerCount() > teamCapacity)
                        errorExit("invalid player data, party: %" PRId64 " playercount: %d exceeds configured team capacity: %d", (int64_t)partyId, (int32_t)party.getPlayerCount(), (int32_t)teamCapacity);

                } while (more);

                if (uniqueParties != (int32_t)packer.getParties().size())
                    errorExit("invalid player data, party ids must be contiguous.");

                fclose(f);
                playerDataPath = tryInputPath;
            }
            else
                errorExit("failed to open input file: %s", tryInputPath.c_str());
        }
    }
    else
        playerDataPath.clear();

    if (cmdOptPartyData.matchOption(*commandLine, matchResult))
    {
        if (playerDataPath.empty())
            errorExit("-partydata option requires -playerdata\n");
        const char8_t* arg = commandLine->Arg(matchResult + 1);
        eastl::string tryInputPath;
        if (findFile(arg, tryInputPath))
        {
            eastl::string configContents;
            configContents.reserve(200);
            FILE* f = fopen(tryInputPath.c_str(), "r");
            if (f != nullptr)
            {
                eastl::vector<PropertyValue> fieldValues;
                fieldValues.reserve(MIN_PARTY_REPORT_COLUMNS + 0);
                // read player data from standard input
                eastl::string line;
                bool more = false;
                do
                {
                    line.clear();
                    fieldValues.clear();
                    more = getLine(f, line);
                    // line format:
                    // gameid partyid playerid team Property0,Property1,...
                    if (line[0] != ' ') // skip comments and headers
                        continue;
                    char8_t* context = NULL;
                    const char8_t* delimiter = " ";
                    char8_t* token = EA::StdC::Strtok(line.begin(), delimiter, &context);
                    while (token != nullptr)
                    {
                        auto fieldValue = EA::StdC::AtoI64(token);
                        fieldValues.push_back(fieldValue);
                        token = EA::StdC::Strtok(NULL, delimiter, &context);
                    }

                    if ((int32_t)fieldValues.size() < MIN_PARTY_REPORT_COLUMNS + 0)
                    {
                        errorExit("insufficient number(%d) of party data fields, expected(%d).\n", (int32_t)fieldValues.size(), MIN_PARTY_REPORT_COLUMNS + 0);
                    }
                    const auto gameId = (PartyId)fieldValues[(int32_t)RequiredPartyDataField::GAME_ID];
                    const auto partyId = (PartyId)fieldValues[(int32_t)RequiredPartyDataField::PARTY_ID];
                    const bool immutable = (PlayerId)fieldValues[(int32_t)RequiredPartyDataField::IMMUTABLE] != 0;
                    auto* party = packer.getPackerParty(partyId);
                    if (party != nullptr)
                    {
                        if (party->mImmutable != immutable)
                        {
                            party->mImmutable = immutable; // changing immutability requires updating the priority to ensure that immutable party is dequeued first
                            packer.updatePackerPartyPriority(*party);
                        }
                    }
                    else
                        errorExit("invalid party data, party id(%" PRId64 ") must match player data read previously.", partyId);

                } while (more);

                fclose(f);
                partyDataPath = tryInputPath;
            }
            else
                errorExit("failed to open input file: %s", tryInputPath.c_str());
        }
    }
    else
        partyDataPath.clear();

    distrib = packer.getPlayers().empty() ? DIST_CHISQUARED : DIST_MANUAL;

    if (distrib != DIST_MANUAL)
    {
        eastl::vector_map<int, float> playerPercentByPartySizeMap;
        if (cmdOptPartySize.matchOption(*commandLine, matchResult))
        {
            eastl::string arg = commandLine->Arg(matchResult + 1);
            char8_t* context = NULL;
            const char8_t* delimiter = ",";
            char8_t* token = EA::StdC::Strtok(arg.begin(), delimiter, &context);
            float pctRemaining = 100.0;
            while (token != nullptr)
            {
                int32_t partySize = 1;
                float playerPercent = 0;
                int32_t count = EA::StdC::Sscanf(token, "%d:%f%%", &partySize, &playerPercent);
                switch (count)
                {
                case 1:
                    playerPercent = pctRemaining;
                case 2:
                    if (partySize < 1 || partySize > teamCapacity)
                    {
                        errorExit("partysize %d must between 1 and team capacity : %d, inclusive.\n", partySize, (int32_t)teamCapacity);
                    }
                    if (pctRemaining < playerPercent)
                    {
                        errorExit("partysize: %d:%f%% exceeds remaining player percentage: %f%%.\n", partySize, playerPercent, pctRemaining);
                    }
                    pctRemaining -= playerPercent;
                    playerPercentByPartySizeMap[partySize] += playerPercent;
                    break;
                default:
                    errorExit("partysize required.\n");
                }
                token = EA::StdC::Strtok(NULL, delimiter, &context);
            }
        }
        if (playerPercentByPartySizeMap.empty())
            playerPercentByPartySizeMap[1] = 100.0f;
        else
        {
            float percentSum = 0;
            for (auto& it : playerPercentByPartySizeMap)
            {
                percentSum += it.second;
            }
            if (percentSum != 100.0f)
                errorExit("partysize must sum to 100%");
        }

        int32_t assignedPlayers = 0;
        eastl::vector_map<int, int> partyCountBySizeMap;
        for (auto& it : playerPercentByPartySizeMap)
        {
            double partyCountDouble = (maxPlayers * it.second) / (100.0f * it.first);
            int32_t partyCount = (int32_t)(partyCountDouble);
            if (partyCount > 0)
            {
                partyCountBySizeMap[it.first] = partyCount;
                assignedPlayers += it.first * partyCount;
            }
        }
        if (partyCountBySizeMap.empty())
        {
            errorExit("no parties.\n");
        }
        // shove the remaining players into the smallest party (to ensure it is the fullest)
        int32_t unassignedPlayers = maxPlayers - assignedPlayers;
        if (unassignedPlayers > 0)
        {
            int32_t remainder = unassignedPlayers % partyCountBySizeMap.front().first;
            partyCountBySizeMap.front().second += unassignedPlayers / partyCountBySizeMap.front().first;
            if (remainder > 0)
                partyCountBySizeMap.front().second++;
        }
        // party size by cumulative density probability function lookup map
        // basically, a map of value ranges that fall into 0..totalpartycount where each range maps to a party size.
        eastl::vector_map<int, int> partySizeByCdfMap;
        int32_t totalpartycount = 0;
        for (auto& it : partyCountBySizeMap)
        {
            int32_t partycount = it.second;
            totalpartycount += partycount;
            partySizeByCdfMap[totalpartycount] = it.first;
        }

        ValueList chidata;
        generateChiSquaredData(chidata);

        eastl::vector<PropertyValueList> playerPropertyArrayList;
        playerPropertyArrayList.resize(maxPlayers);

        // seed player property data
        for (size_t p = 0; p < numSourcePlayerFields; ++p)
        {
            // NOTE: we iterate  the 'inefficient' way by doing linear passes over all players one player property at a time
            // because we want rand() to generate consistent data if synthetic properties are added between tool runs
            for (int32_t i = 0; i < maxPlayers; ++i)
            {
                auto& playerProperties = playerPropertyArrayList[i];
                PropertyValue sampledValue = 0;
                if (distrib == DIST_GAUSSIAN)
                {
                    EA_ASSERT(i < EAArrayCount(gaussianDist));
                    sampledValue = gaussianDist[i];
                }
                else if (distrib == DIST_CHISQUARED)
                {
                    // seed the player skills using consequtive rand() calls for repeatability
                    sampledValue = chidata[rand() % chidata.size()]; // randomly sample array of 10K to select chi squared distribution of skill data bounded between 1-10K skillpoints
#if 0
                // scale value^1/depth of property (just for the hell of it to get some more dispersion across synthetic data)
                    for (int32_t x = 0; x < p; ++x)
                        sampledValue /= 10;
                    if (sampledValue < 1)
                        sampledValue = 1;
#endif
                }
                else
                    EA_ASSERT(false);
                playerProperties.push_back(sampledValue);
            }
        }

        srand(0); // reset random seed to generate consistent party size groupings irrespective of the number of players/properties/partysizes


        int32_t nextPartySize = 0;
        int32_t nextPackerPartyId = 0;
        // seed parties
        for (int32_t i = 0; i < maxPlayers; ++i)
        {
            Party* party = nullptr;
            if (nextPartySize == 0)
            {
                // calculate new party size using the cdf map
                int32_t partySeed = rand() % totalpartycount;
                auto it = partySizeByCdfMap.lower_bound(partySeed);
                if (it == partySizeByCdfMap.end())
                    it = partySizeByCdfMap.rbegin().base();
                nextPartySize = it->second;
                EA_ASSERT(partyCountBySizeMap.find(nextPartySize) != partyCountBySizeMap.end());

                auto newPartyId = ++nextPackerPartyId;
                auto dummyExpiryTime = (Time)newPartyId; // NOTE: expire in order of partyid
                if (!packer.addPackerParty(newPartyId, dummyExpiryTime, dummyExpiryTime))
                {
                    EA_FAIL(); // must succeed!
                }
                party = packer.getPackerParty(newPartyId);
            }
            else
            {
                party = packer.getPackerParty(nextPackerPartyId);
            }
            auto partyPlayerIndex = packer.addPackerPlayer(i + 1, *party);

            auto& propValues = playerPropertyArrayList[i];

            auto column = 0;
            for (auto& fieldUsage : fieldUsageList)
            {
                if (!fieldUsage.used)
                    continue;

                auto fieldValue = propValues[column];
                party->setPlayerPropertyValue(fieldUsage.fieldDesc, partyPlayerIndex, &fieldValue);

                column++;
            }

            if (party->getPlayerCount() >= nextPartySize)
            {
                // done building party, sort all players by skill, descending (helps when viewing reports)
                nextPartySize = 0; // reset target party size
            }
        }
    }
    else
    {
        maxPlayers = (int32_t)packer.getPlayers().size();
    }

    testPartyCountBySizeMap.clear();
    for (auto& partyItr : packer.getParties())
    {
        // done building party
        testPartyCountBySizeMap[partyItr.second.getPlayerCount()]++;
    }

    playerPropertyMax.clear();
    playerPropertyMin.clear();
    playerPropertyMax.resize(numSourcePlayerFields);
    playerPropertyMin.resize(numSourcePlayerFields);
    for (size_t p = 0; p < numSourcePlayerFields; ++p)
    {
        playerPropertyMin[p] = INT64_MAX;
        playerPropertyMax[p] = -1;
    }

    // calculate per property value ranges
    for (auto& partyItr : packer.getParties())
    {
        auto& party = partyItr.second;
        auto column = 0;
        for (auto& fieldUsage : fieldUsageList)
        {
            if (!fieldUsage.used)
                continue;
            if (!party.hasPlayerPropertyField(fieldUsage.fieldDesc))
                continue; // Not all players in a party define the field, it will always be omitted from evaluation and does not contribute to the scores.
            for (PartyPlayerIndex i = 0; i < (PartyPlayerIndex)party.mPartyPlayers.size(); ++i)
            {
                auto fieldValue = party.getPlayerPropertyValue(fieldUsage.fieldDesc, i);
                if (playerPropertyMin[column] > fieldValue)
                    playerPropertyMin[column] = fieldValue;
                if (playerPropertyMax[column] < fieldValue)
                    playerPropertyMax[column] = fieldValue;
            }
            column++;
        }
    }

#if defined(EA_DEBUG)
    int32_t totalMappedPlayerCount = 0;
    for (auto it : testPartyCountBySizeMap)
    {
        totalMappedPlayerCount += it.second*it.first;
    }

    EA_ASSERT(totalMappedPlayerCount >= maxPlayers);
#endif

    EA_ASSERT(gameCapacity > 0);

    // Pack the games and get an time for this run. 
    EA::StdC::Stopwatch packStopWatch(EA::StdC::Stopwatch::kUnitsSeconds, true);
    packer.packGames(nullptr);
    packStopWatch.Stop();
    stopWatch.SetElapsedTimeFloat(stopWatch.GetElapsedTimeFloat() + packStopWatch.GetElapsedTimeFloat());

    const int32_t maxFullGames = (maxPlayers / gameCapacity);

    packer.reportLog(ReportMode::WORKINGSET, "--%s--\n", gReportModeNames[(int32_t)ReportMode::WORKINGSET]);
    packer.reportLog(ReportMode::WORKINGSET, "workingsetsize freq\n");

    for (auto& it : packer.getDebugGameWorkingSetFreq())
    {
        packer.reportLog(ReportMode::WORKINGSET, "%*d %*d\n",
            sizeof("workingsetsize") - 1, it.first,
            sizeof("freq") - 1, it.second);
    }

    eastl::vector<SummaryMetrics> factorMetrics;
    factorMetrics.resize(packer.getFactors().size());
    for (auto& m : factorMetrics)
        initializeMetric(m, maxFullGames);

    struct GameReportColumn
    {
        const char* columnName; // major column name
        eastl::vector<const char*> subColumns; // minor subcolumn names
    };

    const int32_t GAME_REPORT_MIN_COL_WIDTH = 7;
    eastl::vector<GameReportColumn> gameReportColumns = {
        { "game", { "gameid", "evicted", "viable" } },
    };

    const eastl::vector_set<eastl::string> omitFactors = 
    {
#if 0
        "GameSize",
        "TeamSize"
#endif
    }; // factors that are always omitted from all reports

    static const char8_t* SCORE_COL_NAME = "score%";

    // append factor/specific columns
    for (auto& factor : packer.getFactors())
    {
        if (omitFactors.count(factor.mFactorName) == 0)
        {
            auto& column = gameReportColumns.push_back();
            column.columnName = factor.mFactorName.c_str();
            column.subColumns = { SCORE_COL_NAME };
        }
    }

    packer.reportLog(ReportMode::GAMES, "--%s--\n", gReportModeNames[(int32_t)ReportMode::GAMES]);

#define GAME_REPORT_COLUMN_PADDING " "

    eastl::string gameReportHeader;
    eastl::string gameReportSubheader;

    for (auto& column : gameReportColumns)
    {
        size_t start = gameReportSubheader.size();
        for (auto& subcolname : column.subColumns)
        {
            gameReportSubheader.append_sprintf("%*s ", eastl::max(GAME_REPORT_MIN_COL_WIDTH, (int32_t)strlen(subcolname)), subcolname);
        }
        int32_t headersize = (int32_t) (gameReportSubheader.size() - start);
        gameReportHeader.append_sprintf("%-*s", headersize, column.columnName);
        gameReportHeader.append(GAME_REPORT_COLUMN_PADDING); // append additional major column spacing
        gameReportSubheader.append(GAME_REPORT_COLUMN_PADDING); // align subcolumn
    }

    packer.reportLog(ReportMode::GAMES, "%s", gameReportHeader.c_str());
    packer.reportLog(ReportMode::GAMES, "\n");
    packer.reportLog(ReportMode::GAMES, "%s", gameReportSubheader.c_str());
    packer.reportLog(ReportMode::GAMES, "\n");

    int32_t numViableGames = 0;
    eastl::string lineBuf;
    for (auto& itr : packer.getGames())
    {
        auto& game = itr.second;
        packer.reportLog(ReportMode::GAMES, "%*d ", eastl::max(GAME_REPORT_MIN_COL_WIDTH, (int32_t)strlen("gameid")), game.mGameId);
        packer.reportLog(ReportMode::GAMES, "%*d ", eastl::max(GAME_REPORT_MIN_COL_WIDTH, (int32_t)strlen("evicted")), game.mEvictedCount);
        bool viableGame = true;

        lineBuf.clear();
        for (auto& factor : packer.getFactors())
        {
            EvalContext evalContext(factor, GameState::PRESENT);
            {
                auto score = factor.passiveEval(evalContext, game);
                if (score == INVALID_SCORE)
                {
                    packer.traceLog(TraceMode::ALGO, "Warning: Game(%" PRId64 ") created with invalid/non-viable score(%f), on factor(%s). This may be due to factor/ranges being configured restrictive, for such a party to easily match with. Party count(%d), member count(%d).", game.mGameId, score, factor.mFactorName.c_str(), game.getPartyCount(GameState::PRESENT), game.getPlayerCount(GameState::PRESENT));
                    continue;
                }
            }
            auto rawScore = factor.computeRawScore(evalContext.mEvalResult);
            if (viableGame && !factor.mScoreOnlyFactor)
            {
                viableGame = factor.isViableScore(rawScore);
            }
            if (omitFactors.count(factor.mFactorName) == 0)
            {
                // format the factor metric
                lineBuf.append_sprintf(GAME_REPORT_COLUMN_PADDING "%*.2f ", eastl::max(GAME_REPORT_MIN_COL_WIDTH, (int32_t)strlen(SCORE_COL_NAME)), rawScore * 100);
            }

            // accumulate metric for summary reporting
            auto& m = factorMetrics[factor.mFactorIndex];

            // The factor value range is not the score.
            // Factors like game size, team size, have a value range outside of 0-1,
            // while factors like team.ued.skill.ratio have a value within 0-1 which makes them appear the same.
            // Nonetheless, the factor's value range is different than its score range because the latter is 0..1 for ALL factors.
            auto factorValueRange = factor.mBestValue - factor.mWorstValue;
            if (factorValueRange < 0)
                factorValueRange *= -1;

            auto factorValueDistanceFromIdeal = 1.0f;
            if (!evalContext.mEvalResult.empty())
            {
                factorValueDistanceFromIdeal -= evalContext.computeRawScore();
                EA_ASSERT_MSG(factorValueDistanceFromIdeal >= 0, "Factor value distance cannot be negative!");
            }

            factorValueDistanceFromIdeal *= factorValueRange;

            // else // special case, the quality factor could not be evaluated due to lack of field value participation, use score distance is 1.0 by default
            
            // capture the delta between the ideal value that is achievable for a factor vs the value we attained for this game
            m.accumulateMetric(factorValueDistanceFromIdeal);
        }

        numViableGames += viableGame;
        packer.reportLog(ReportMode::GAMES, "%*d ", eastl::max(GAME_REPORT_MIN_COL_WIDTH, (int32_t)strlen("viable")), viableGame);
        packer.reportLog(ReportMode::GAMES, "%*s ", (int32_t)lineBuf.size(), lineBuf.c_str());

        packer.reportLog(ReportMode::GAMES, "\n");
    }

    for (auto& m : factorMetrics)
        completeMetric(m);

    eastl::vector<const char*> partyReportColumns = { "gameid", "partyid", "immutbl", "players", "team", "evicted", "imprvtm" };
    packer.reportLog(ReportMode::PARTYS, "--%s--\n", gReportModeNames[(int32_t)ReportMode::PARTYS]);
    for (auto* column : partyReportColumns)
    {
        packer.reportLog(ReportMode::PARTYS, "%s ", column);
    }
    packer.reportLog(ReportMode::PARTYS, "\n");

    int32_t numPlayers = 0;
    int32_t numParties = 0;
    PartySizeFreqMap partySizeFreqMap;
    for (auto& gameItr : packer.getGames())
    {
        auto& game = gameItr.second;
        numPlayers += game.getPlayerCount(GameState::PRESENT);
        for (int32_t i = 0, size = game.getPartyCount(GameState::PRESENT); i < size; ++i)
        {
            const GameParty& gameParty = game.mGameParties[i];
            numParties++;
            const Party& party = *packer.getPackerParty(gameParty.mPartyId);
            EA_ASSERT(party.mGameId == game.mGameId);
            // CHANGE: initialize these values by using the a specific propertyIndex()
            packer.reportLog(ReportMode::PARTYS, "%*" PRId64 " %*" PRId64 " %*d %*d %*d %*" PRId64 " %*" PRId64 "\n",
                strlen(partyReportColumns[0]), (int64_t)game.mGameId,
                strlen(partyReportColumns[1]), (int64_t)gameParty.mPartyId,
                strlen(partyReportColumns[2]), (int32_t)gameParty.mImmutable,
                strlen(partyReportColumns[3]), (int32_t)gameParty.mPlayerCount,
                strlen(partyReportColumns[4]), (int32_t)game.getTeamIndexByPartyIndex(i, GameState::PRESENT),
                strlen(partyReportColumns[5]), (int64_t)party.mEvictionCount,
                strlen(partyReportColumns[6]), (int64_t)party.mAddedToGameTimestamp
            );

            ++partySizeFreqMap[gameParty.mPlayerCount];
        }
    }

    eastl::vector<eastl::string> playerReportColumns = { "gameid", "partyid", "playerid", "team" };
    EA_ASSERT(playerReportColumns.size() == (size_t)RequiredPlayerDataField::E_COUNT);

    for (auto& fieldUsage : fieldUsageList)
    {
        if (!fieldUsage.used)
            continue;

        eastl::string& lowerCasePropertyName = playerReportColumns.push_back();
        lowerCasePropertyName = fieldUsage.fieldDesc->getFullFieldName();
        lowerCasePropertyName.make_lower();
    }

    EA_ASSERT(playerReportColumns.size() == numSourcePlayerFields + (size_t)RequiredPlayerDataField::E_COUNT);

    packer.reportLog(ReportMode::PLAYERS, "--%s--\n", gReportModeNames[(int32_t)ReportMode::PLAYERS]);
    for (auto& column : playerReportColumns)
    {
        packer.reportLog(ReportMode::PLAYERS, "%s ", column.c_str());
    }
    packer.reportLog(ReportMode::PLAYERS, "\n");

    for (auto& gameItr : packer.getGames())
    {
        auto& game = gameItr.second;
        struct PlayerInfo
        {
            PartyId partyid;
            PlayerId playerid;
            int32_t team;
            int32_t playerIndex;
            const Party* party;
        };
        PlayerInfo playerInfoByTeam[MAX_TEAM_COUNT][MAX_PLAYER_COUNT];
        int32_t playerCountByTeam[MAX_TEAM_COUNT] = {};
        for (int32_t i = 0, size = game.getPartyCount(GameState::PRESENT); i < size; ++i)
        {
            const auto& gameParty = game.mGameParties[i];
            const auto& party = *packer.getPackerParty(gameParty.mPartyId);
            EA_ASSERT(party.mGameId == game.mGameId);

            auto teamIndex = game.getTeamIndexByPartyIndex(i, GameState::PRESENT);
            if (teamIndex >= 0)
            {
                PartyPlayerIndex partyPlayerIndex = 0;
                for (auto& partyPlayer : party.mPartyPlayers)
                {
                    auto& playerInfo = playerInfoByTeam[teamIndex][playerCountByTeam[teamIndex]++];
                    memset(&playerInfo, 0, sizeof(playerInfo));
                    playerInfo.partyid = party.mPartyId;
                    playerInfo.playerid = partyPlayer.mPlayerId;
                    playerInfo.team = teamIndex;
                    playerInfo.playerIndex = partyPlayerIndex++;
                    playerInfo.party = &party;
                }
            }
            else
                EA_FAIL_MSG("Invalid team index!");
        }
        for (int32_t team = 0; team < teamCount; ++team)
        {
            // sort all players by skill: descending, helpful for viewing report
            //eastl::sort(playerInfoByTeam[team], playerInfoByTeam[team] + playerCountByTeam[team], [](const PlayerInfo& a, const PlayerInfo& b) { return a.propertyValues[0] > b.propertyValues[0]; });
            for (int32_t i = 0; i < playerCountByTeam[team]; ++i)
            {
                const auto& playerinfo = playerInfoByTeam[team][i];
                packer.reportLog(ReportMode::PLAYERS, "%*" PRId64 " %*" PRId64 " %*" PRId64 " %*d",
                    (int32_t)playerReportColumns[0].size(), (int64_t)game.mGameId,
                    (int32_t)playerReportColumns[1].size(), (int64_t)playerinfo.partyid,
                    (int32_t)playerReportColumns[2].size(), (int64_t)playerinfo.playerid,
                    (int32_t)playerReportColumns[3].size(), (int32_t)playerinfo.team
                );
                auto column = 0;
                for (auto& fieldUsage : fieldUsageList)
                {
                    if (!fieldUsage.used)
                        continue;
                    PropertyValue fieldValue = OMITTED_PROPERTY_FIELD_VALUE;
                    if (playerinfo.party->hasPlayerPropertyField(fieldUsage.fieldDesc))
                    {
                        bool hasValue = false;
                        fieldValue = playerinfo.party->getPlayerPropertyValue(fieldUsage.fieldDesc, playerinfo.playerIndex, &hasValue);
                        if (!hasValue)
                            fieldValue = UNSPECIFIED_PROPERTY_FIELD_VALUE;
                    }
                    packer.reportLog(ReportMode::PLAYERS, " %*" PRId64,
                        (int32_t)playerReportColumns[MIN_PLAYER_REPORT_COLUMNS + column].size(), fieldValue
                    );
                    column++;
                }
                packer.reportLog(ReportMode::PLAYERS, "\n");
            }
        }
    }

    eastl::string reportModesString;
    for (int32_t mode = 0; mode < (int32_t)ReportMode::E_COUNT; ++mode)
    {
        if (packer.isReportEnabled((ReportMode)mode))
        {
            reportModesString += gReportModeNames[mode];
            reportModesString += ',';
        }
    }
    if (reportModesString.empty())
        reportModesString = "none";
    else
        reportModesString.pop_back();

    eastl::string traceModesString;
    for (int32_t mode = 0; mode < (int32_t)TraceMode::E_COUNT; ++mode)
    {
        if (utilityMethods.utilPackerTraceEnabled((TraceMode)mode))
        {
            traceModesString += gTraceModeNames[mode];
            traceModesString += ',';
        }
    }
    if (traceModesString.empty())
        traceModesString = "none";
    else
        traceModesString.pop_back();

    eastl::string qualityFactorsString;
    for (auto& factor : packer.getFactors())
    {
        qualityFactorsString += factor.mScoreOnlyFactor ? '-' : '+';
        qualityFactorsString += factor.mFactorName;
        qualityFactorsString += ',';
    }
    if (!qualityFactorsString.empty())
        qualityFactorsString.pop_back();

#define SUMMARY_SECTION_PREFIX ""
#define SUMMARY_SUBSECTION_PREFIX "   *"
#define SUMMARY_FIELD_PREFIX "    "
#define SUMMARY_FIELD_KEY_FMT "%-36s"

    EA::StdC::DateTime date;

    packer.reportLog(ReportMode::SUMMARY, "--%s--\n", gReportModeNames[(int32_t)ReportMode::SUMMARY]);
    packer.reportLog(ReportMode::SUMMARY,
        SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %s\n"
        SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %s\n"
        SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %s\n"
        SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %s\n"
        SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %s\n"
        SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %s\n"
        SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %s\n"
        SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %s\n"
        SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %d/%d/%02d %02d:%02d:%02d.%03d\n"
        SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %d\n"
        SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %d\n"
        SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %d\n"
        SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %d\n"
        ,
        "qualityfactors", qualityFactorsString.c_str(),
        "reportmode", reportModesString.c_str(),
        "tracemode", traceModesString.c_str(),
        "build",
#ifdef EA_DEBUG
        "debug",
#else
        "release",
#endif
        "distribution", distributionNames[distrib],
        "config", configFilePath.c_str(),
        "playerdata", playerDataPath.empty() ? "none" : playerDataPath.c_str(),
        "partydata", partyDataPath.empty() ? "none" : partyDataPath.c_str(),
        "rundate", date.GetParameter(EA::StdC::kParameterYear), date.GetParameter(EA::StdC::kParameterMonth), date.GetParameter(EA::StdC::kParameterDayOfMonth), date.GetParameter(EA::StdC::kParameterHour), date.GetParameter(EA::StdC::kParameterMinute), date.GetParameter(EA::StdC::kParameterSecond), date.GetParameter(EA::StdC::kParameterNanosecond) / 1000000,
        "numplayers", numPlayers,
        "numparties", numParties,
        "numgames", (int32_t)packer.getGames().size(),
        "numviablegames", (int32_t)numViableGames,
        "maxgameworkingset", packer.getDebugGameWorkingSetFreq().empty() ? 0 : packer.getDebugGameWorkingSetFreq().crbegin()->first
    );

    packer.reportLog(ReportMode::SUMMARY,
        SUMMARY_SECTION_PREFIX "player:\n"
    );

    for (int32_t p = 0; p < numSourcePlayerFields; ++p)
    {
        auto& fieldName = playerReportColumns[MIN_PLAYER_REPORT_COLUMNS + p];
        packer.reportLog(ReportMode::SUMMARY,
            SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %" PRId64 " - %" PRId64 "\n"
            ,
            fieldName.c_str(), playerPropertyMin[p], playerPropertyMax[p]
        );
    }

    packer.reportLog(ReportMode::SUMMARY,
        SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %d (%.2f%%)\n"
        ,
        "evicted", packer.getEvictedPlayersCount(), 100.0*packer.getEvictedPlayersCount() / numPlayers
    );

    packer.reportLog(ReportMode::SUMMARY,
        SUMMARY_SECTION_PREFIX "party:\n"
        SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %d (%.2f%%)\n"
        ,
        "evicted", packer.getEvictedPartiesCount(), 100.0*packer.getEvictedPartiesCount() / numParties
    );

    for (auto& it : partySizeFreqMap)
    {
        // Output party size, count (% of player population)
        eastl::string key;
        key.sprintf("partysize:%2d", it.first);
        packer.reportLog(ReportMode::SUMMARY,
            SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %d (%.2f%%)\n",
            key.c_str(), it.second, 100.0 * it.second*it.first / numPlayers);
    }

    {
        packer.reportLog(ReportMode::SUMMARY,
            SUMMARY_SECTION_PREFIX "factorevals/interim:\n");
        packer.reportLog(ReportMode::SUMMARY,
            SUMMARY_SUBSECTION_PREFIX "improved*\n");
        for (auto& factor : packer.getFactors())
        {
            if (factor.mScoreOnlyFactor)
                continue;

            auto total = factor.mInterimMetrics.mImproved + factor.mInterimMetrics.mUnchanged + factor.mInterimMetrics.mDegraded + factor.mInterimMetrics.mRejected;
            auto value = factor.mInterimMetrics.mImproved;
            packer.reportLog(ReportMode::SUMMARY,
                SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %" PRIu64 " / %" PRIu64 " (%.2f%%)\n"
                , factor.mFactorName.c_str(), value, total, total > 0 ? 100.0 * value / total : 0.0);
        }
        packer.reportLog(ReportMode::SUMMARY,
            SUMMARY_SUBSECTION_PREFIX "unchanged*\n");
        for (auto& factor : packer.getFactors())
        {
            if (factor.mScoreOnlyFactor)
                continue;

            auto total = factor.mInterimMetrics.mImproved + factor.mInterimMetrics.mUnchanged + factor.mInterimMetrics.mDegraded + factor.mInterimMetrics.mRejected;
            auto value = factor.mInterimMetrics.mUnchanged;
            packer.reportLog(ReportMode::SUMMARY,
                SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %" PRIu64 " / %" PRIu64 " (%.2f%%)\n"
                , factor.mFactorName.c_str(), value, total, total > 0 ? 100.0 * value / total : 0.0);
        }
        packer.reportLog(ReportMode::SUMMARY,
            SUMMARY_SUBSECTION_PREFIX "degraded*\n");
        for (auto& factor : packer.getFactors())
        {
            if (factor.mScoreOnlyFactor)
                continue;

            auto total = factor.mInterimMetrics.mImproved + factor.mInterimMetrics.mUnchanged + factor.mInterimMetrics.mDegraded + factor.mInterimMetrics.mRejected;
            auto value = factor.mInterimMetrics.mDegraded;
            packer.reportLog(ReportMode::SUMMARY,
                SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %" PRIu64 " / %" PRIu64 " (%.2f%%)\n"
                , factor.mFactorName.c_str(), value, total, total > 0 ? 100.0 * value / total : 0.0);
        }
        packer.reportLog(ReportMode::SUMMARY,
            SUMMARY_SUBSECTION_PREFIX "rejected*\n");
        for (auto& factor : packer.getFactors())
        {
            if (factor.mScoreOnlyFactor)
                continue;

            auto total = factor.mInterimMetrics.mImproved + factor.mInterimMetrics.mUnchanged + factor.mInterimMetrics.mDegraded + factor.mInterimMetrics.mRejected;
            auto value = factor.mInterimMetrics.mRejected;
            packer.reportLog(ReportMode::SUMMARY,
                SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %" PRIu64 " / %" PRIu64 " (%.2f%%)\n"
                , factor.mFactorName.c_str(), value, total, total > 0 ? 100.0 * value / total : 0.0);
        }
    }

    {
        packer.reportLog(ReportMode::SUMMARY,
            SUMMARY_SECTION_PREFIX "factorevals/final:\n");
        packer.reportLog(ReportMode::SUMMARY,
            SUMMARY_SUBSECTION_PREFIX "improved*\n");
        for (auto& factor : packer.getFactors())
        {
            if (factor.mScoreOnlyFactor)
                continue;

            auto total = factor.mFinalMetrics.mImproved + factor.mFinalMetrics.mUnchanged + factor.mFinalMetrics.mDegraded + factor.mFinalMetrics.mRejected;
            auto value = factor.mFinalMetrics.mImproved;
            packer.reportLog(ReportMode::SUMMARY,
                SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %" PRIu64 " / %" PRIu64 " (%.2f%%)\n"
                , factor.mFactorName.c_str(), value, total, total > 0 ? 100.0 * value / total : 0.0);
        }
        packer.reportLog(ReportMode::SUMMARY,
            SUMMARY_SUBSECTION_PREFIX "unchanged*\n");
        for (auto& factor : packer.getFactors())
        {
            if (factor.mScoreOnlyFactor)
                continue;

            auto total = factor.mFinalMetrics.mImproved + factor.mFinalMetrics.mUnchanged + factor.mFinalMetrics.mDegraded + factor.mFinalMetrics.mRejected;
            auto value = factor.mFinalMetrics.mUnchanged;
            packer.reportLog(ReportMode::SUMMARY,
                SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %" PRIu64 " / %" PRIu64 " (%.2f%%)\n"
                , factor.mFactorName.c_str(), value, total, total > 0 ? 100.0 * value / total : 0.0);
        }
        packer.reportLog(ReportMode::SUMMARY,
            SUMMARY_SUBSECTION_PREFIX "degraded*\n");
        for (auto& factor : packer.getFactors())
        {
            if (factor.mScoreOnlyFactor)
                continue;

            auto total = factor.mFinalMetrics.mImproved + factor.mFinalMetrics.mUnchanged + factor.mFinalMetrics.mDegraded + factor.mFinalMetrics.mRejected;
            auto value = factor.mFinalMetrics.mDegraded;
            packer.reportLog(ReportMode::SUMMARY,
                SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %" PRIu64 " / %" PRIu64 " (%.2f%%)\n"
                , factor.mFactorName.c_str(), value, total, total > 0 ? 100.0 * value / total : 0.0);
        }
        packer.reportLog(ReportMode::SUMMARY,
            SUMMARY_SUBSECTION_PREFIX "rejected*\n");
        for (auto& factor : packer.getFactors())
        {
            if (factor.mScoreOnlyFactor)
                continue;

            auto total = factor.mFinalMetrics.mImproved + factor.mFinalMetrics.mUnchanged + factor.mFinalMetrics.mDegraded + factor.mFinalMetrics.mRejected;
            auto value = factor.mFinalMetrics.mRejected;
            packer.reportLog(ReportMode::SUMMARY,
                SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %" PRIu64 " / %" PRIu64 " (%.2f%%)\n"
                , factor.mFactorName.c_str(), value, total, total > 0 ? 100.0 * value / total : 0.0);
        }
    }

    packer.reportLog(ReportMode::SUMMARY,
        SUMMARY_SECTION_PREFIX "gamequality:\n");
    {
        // TODO: replace this hardcoded printout with automatic metrics obtained from the corresponding factors ala: for (auto&& factor : gameTemplate.getFactors())
        packer.reportLog(ReportMode::SUMMARY,
            SUMMARY_SUBSECTION_PREFIX SUMMARY_FIELD_KEY_FMT "    ", "aggregates|percentiles*");
        PercentileMap percentilemap = { { 0, 0 },{ 20, 0 },{ 50, 0 },{ 75, 0 },{ 95, 0 },{ 100, 0 } };
        static const char* PIPE_SPACER = "     |";
        // print headings
        packer.reportLog(ReportMode::SUMMARY, " %7s", "avg");
        packer.reportLog(ReportMode::SUMMARY, " %7s", "sum");
        packer.reportLog(ReportMode::SUMMARY, PIPE_SPACER);
        for (auto& it : percentilemap)
        {
            eastl::string str;
            if (it.first <= 0)
                str = "min";
            else if (it.first >= 100)
                str = "max";
            else
                str.sprintf(".%2d", it.first);
            packer.reportLog(ReportMode::SUMMARY, " %7s",
                str.c_str());
        }
        packer.reportLog(ReportMode::SUMMARY, "\n");

        for (auto& factor : packer.getFactors())
        {
            if (factor.mScoreOnlyFactor)
                continue;

            if (omitFactors.count(factor.mFactorName) == 0)
            {
                auto& m = factorMetrics[factor.mFactorIndex];
                computePercentiles(m, percentilemap);
                bool isScoreOnly = false;
                eastl::string factorName;
                factorName += isScoreOnly ? '-' : '+';
                factorName += factor.mFactorName;
                packer.reportLog(ReportMode::SUMMARY,
                    SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = [",
                    factorName.c_str());
                eastl::string output;
                output.append_sprintf(" %7.1f", m.mean); // maybe: change to (int32_t)round(m.mean)
                output.append_sprintf(" %7.1f", m.sum);
                output.append(PIPE_SPACER);
                for (auto& it : percentilemap)
                {
                    output.append_sprintf(" %7.1f", it.second);
                }
                packer.reportLog(ReportMode::SUMMARY, "%s ]\n", output.c_str());
            }
        }
    }
    packer.reportLog(ReportMode::SUMMARY,
        SUMMARY_SECTION_PREFIX "perf:\n"
        SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %d\n"
        SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %d (%.2f%%)\n"
        SUMMARY_FIELD_PREFIX SUMMARY_FIELD_KEY_FMT " = %f sec\n"
        ,
        "optmatches", packer.getDebugFoundOptimalMatchCount(),
        "optmatchesdiscarded", packer.getDebugFoundOptimalMatchDiscardCount(), packer.getDebugFoundOptimalMatchDiscardCount()*100.0 / (packer.getDebugFoundOptimalMatchCount() + packer.getDebugFoundOptimalMatchDiscardCount()),
        "cputime", stopWatch.GetElapsedTimeFloat()
    );

    fprintf(stderr, "Press any key to continue...");

    getchar();

    return 0;
}


void cutString(eastl::string& first, eastl::string& second, const eastl::string& from, const char8_t ch)
{
    // extract factortype
    eastl_size_t offset = from.find_first_of(ch);
    if (offset == eastl::string::npos)
    {
        first = from;
        second.clear();
    }
    else
    {
        first = from.substr(0, offset);
        second = from.substr(offset + 1);
    }
}

static int32_t usage(const char* prg)
{
    eastl::string buf;
    printf("Usage: %s [-config <relfilepath>] [-playerdata <relfilepath>] [-report <section>,...] [-trace <context>[,...]] [-players <N>] [-teamsize <1..32>] [-partysizes <partysize>[:<player population %%>][,...]] [-qualityfactors <factor>[,...]] [-partialbalancealgo <algo>] [-maxworkingset <0-N>]\n", prg);
    printf("    -config(-cfg)           : Specify relative file path used for reading the remaining command line options. Program searches upwards starting at CWD.\n");
    printf("        Example:\n");
    printf("            -cfg etc/gpsettings.cfg\n");
    printf("    -playerdata(-pd)        : Specify relative file path used for reading space delimited player data used by the algorithm (when omitted tool generates synthetic player data using specified -players -partysizes options). Program searches upwards starting at CWD.\n");
    printf("    -playerproperties(-pp)        : Specify named player properties used as inputs by the quality factors. Order must match properties in -playerdata file.\n");
    printf("        Example:\n");
    printf("            -pp SkillX,SkillY,PingSiteSJC,PingSiteIAD\n");
    printf("        Example:\n");
    printf("            -pd etc/players.txt\n");
    printf("    -report(-r)             : Output packing report for specified section(s)\n");
    printf("        Example:\n");
    printf("            -r ");
    buf.clear();
    for (int32_t i = 0; i < (int32_t)ReportMode::E_COUNT; ++i)
    {
        buf += gReportModeNames[i];
        buf += ',';
    }
    buf.pop_back();
    printf("%s\n", buf.c_str());
    printf("    -trace(-t)              : Output packing operation trace for specified context(s) (optionally restricted to specific ids). Format: <context>[:<id>,...];...\n");
    printf("        Example:\n");
    printf("            -t ");
    buf.clear();
    for (int32_t i = 0; i < (int32_t)TraceMode::E_COUNT; ++i)
    {
        buf += gTraceModeNames[i];
        buf += "[:<id>,...];";
    }
    buf.pop_back();
    printf("%s\n", buf.c_str());
    printf("    -players(-p)            : Number of players to simulate. Should be multiple of teamcapacity to make full games\n");
    printf("    -teamcapacity(-tc)      : Max number of players that can be packed into a single game team\n");
    printf("    -partysize(-ps)         : Sizes of parties to simulate. Must be between 1 and teamcapacity\n");
    printf("        Example:\n");
    printf("            -ps 1 # equivalent to: -ps 1:100%%\n");
    printf("            -ps 1:70%%,2 # equivalent to: -ps 1:70%%,2:30%%\n");
    printf("            -ps 1:90%%,2:5%%,3:3%%,4:2%%\n");
    printf("    -qualityfactors(-qf)    : Quality factor(s) used by the packing algorithm\n");
    printf("        Example:\n");
    printf("            -qf <qualityfactorname>[:ratio.scale=<0..N>],... # formula: (int32_t)(1-subdomain/domain)*scale\n");
    printf("            -qf <qualityfactorname>[:range.threshold=<0..N>],... # formula: (domain-subdomain)>threshold?(domain-subdomain):0\n");
    printf("            -qf <qualityfactorname>[/<propertyIndex>][+<propertyIndex]... # optionally specify which player properties the factor operates on");
    buf.clear();
    for (auto& factorSpec : getAllFactorSpecs())
    {
        buf += factorSpec.mTransformName;
        buf += ',';
    }
    buf.pop_back();
    printf("%s\n", buf.c_str());
    printf("    -maxworkingset(-mws)    : Threshold that governs how aggressively imperfect games are retired from further improvement. Lower numbers trade off quality for higher performance. 0 == unbounded\n");

    return 1;
}


void generateChiSquaredData(ValueList& chidata)
{
    int32_t numelements = 10000;
    chidata.reserve(numelements);

    for (int32_t i = 0; i < sizeof(chisquared_10K) / sizeof(chisquared_10K[0]); ++i)
    {
        const ChiSquared& chi = chisquared_10K[i];
        for (int32_t j = 0; j < chi.numsamples; ++j)
        {
            int32_t sample = chi.from + rand() % (chi.to - chi.from);
            chidata.push_back(sample);
        }
    }
}

void initializeMetric(SummaryMetrics& data, int64_t capacity)
{
    data.values.reserve(capacity);
}

void completeMetric(SummaryMetrics& data)
{
    eastl::sort(data.values.begin(), data.values.end());
}

#if 0
double computeSigma(const SummaryMetrics& data)
{
    double sigma = 0.0f;
    if (data.count >= 2)
        sigma = sqrt(data.m2 / (data.count - 1));
    return sigma;

}

/**
* Given 2 sets of identical elements sorted min to max and max to min
* this function prints all possible unique sub-sequences of elements of size seqlen.
* This algorithm is used to evaluate all possible options for evicting parties such
* that an incoming party(possibly containing multiple players) can be absorbed into
* the game thereby improving the minimum skill range between the game's max and min
* skilled players.
*/
void printOrderedSubsequences(int* maxmin, int* minmax, int32_t arraysize, int32_t seqlen)
{
    EA_ASSERT(arraysize > seqlen);
    for (int32_t i = 0; i <= seqlen; ++i)
    {
        printf("[");
        for (int32_t p = 0; p < seqlen - i; ++p)
            printf("%2d", minmax[p]);
        for (int32_t p = 0; p < i; ++p)
            printf("%2d", maxmin[p]);
        printf("]\n");
    }
}

void enumerateOrderedSubsequences()
{
    int32_t maxmin[] = { 5, 4, 3, 2, 1 };
    int32_t minmax[] = { 1, 2, 3, 4, 5 };
    printf("sequence: {");
    for (auto i : minmax)
        printf("%2d", i);
    printf("}\n");
    for (int32_t i = 1; i < EAArrayCount(maxmin); ++i)
    {
        printf("print seqlen: %d\n", i);
        printOrderedSubsequences(maxmin, minmax, EAArrayCount(maxmin), i);
    }
}
#endif

void computePercentiles(const SummaryMetrics& data, PercentileMap& percentiles)
{
    for (auto& it : percentiles)
        it.second = 0;
    if (data.values.empty())
        return;
    // http://www.dummies.com/education/math/statistics/how-to-calculate-percentiles-in-statistics/
    for (auto& it : percentiles)
    {
        if (it.first <= 0)
            it.second = *data.values.begin();
        else if (it.first >= 100)
            it.second = *data.values.rbegin();
        else
        {
            int32_t offset = ((int32_t)data.values.size() * it.first) / 100;
            it.second = *(data.values.begin() + offset);
        }
    }
}

bool findFile(const char8_t* relativefilepath, eastl::string& absolutefilepath)
{
    EA::IO::Path::PathString8 configfilepathRelative(relativefilepath);
    if (!EA::IO::Path::IsRelative(configfilepathRelative))
        errorExit("config file path must be relative.");
    EA::IO::Path::PathString8 cwdpath;
    int32_t count = EA::IO::Directory::GetCurrentWorkingDirectory(cwdpath.begin(), cwdpath.kMaxSize);
    if (count > 0)
    {
        cwdpath.force_size(count);
        EA::IO::Path::PathString8 cwdpathScratch(cwdpath);
        do
        {
            EA::IO::Path::PathString8 tryconfigpath(cwdpathScratch);
            EA::IO::Path::Join(tryconfigpath, configfilepathRelative);

            if (EA::IO::File::Exists(tryconfigpath.c_str()))
            {
                absolutefilepath = tryconfigpath.c_str();
                break;
            }
            EA::IO::Path::TruncateComponent(cwdpathScratch, -1);
        } while (!cwdpathScratch.empty());
        
        if (absolutefilepath.empty())
            errorExit("failed to find file: %s via recursive upward search starting at: %s", configfilepathRelative.c_str(), cwdpath.c_str());
    }
    else
    {
        errorExit("failed to get current working directory.\n");
    }
    return !absolutefilepath.empty();
}

bool getLine(FILE* f, eastl::string& line)
{
    for (;;)
    {
        int32_t c = fgetc(f);
        if (c == EOF)
            return false;
        if (c == '\n')
            break;
        line += (char) c;
    }
    return true;
}

void errorExit(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    printf("FATAL: ");
    vprintf(fmt, args);
    va_end(args);

    exit(1);
}


bool CommandLineOption::matchOption(EA::EAMain::CommandLine& commandLine, int32_t& result) const
{
    result = commandLine.FindSwitch(optionName);
    if (result < 0)
        result = commandLine.FindSwitch(optionAbbreviation);
    return result >= 0;
}

