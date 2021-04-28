/*************************************************************************************************/
/*!
    \file


    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/version.h"
#include "framework/logger.h"
#include "framework/util/shared/blazestring.h"
#include "framework/util/compression/basezlibcompression.h"
#include "framework/connection/socketutil.h"
#include "framework/config/config_file.h"
#include "framework/config/config_boot_util.h"
#include <stdlib.h>
#ifdef EA_PLATFORM_LINUX
#include <signal.h>
#endif
#include "framework/controller/processcontroller.h"
#include "framework/system/consolesignalhandler.h"
#include "framework/system/threadlocal.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/component/blazerpc.h"
#include "framework/grpc/grpcutils.h"

#if defined(TARGET_gamereporting)
#include "gamereporting/gamereportingcustom.h"
#endif

#include "EATDF/tdffactory.h"
#include "EAStdC/EAString.h"

#include <google/protobuf/stubs/common.h>

const char8_t* cVersionInfo = "-v";
const char8_t* cVerboseVersion = "-vv";
const char8_t* cLabel = "-l";
const char8_t* cPidFile = "-p";
const char8_t* cInstanceId = "--id";
const char8_t* cInstanceType = "--type";
const char8_t* cInstanceName = "--name";
const char8_t* cInstancePort = "--port";
const char8_t* cInternalPorts = "--internalports";
const char8_t* cExternalPorts = "--externalports";
const char8_t* cOutOfService = "--outOfService";
const char8_t* cDbdestructive = "--dbdestructive";
const char8_t* cListCategories = "--list-log-categories";
const char8_t* cLogDir = "--logdir";
const char8_t* cLogName = "--logname";
const char8_t* cLogOutput = "--logoutput";
const char8_t* cArgFile = "--argfile";
const char8_t* cOverrideFile = "--bootFileOverride";
const char8_t* cConFile ="configFile";
const char8_t* cDisableConfigStringEscapes = "--disable-config-string-escapes";
const char8_t* cMonitorPid = "--monitor-pid";
const char8_t* cTool = "--tool";
const char8_t* cDisableFirewallCheck = "--disable-firewall-check";

//defined in allocation.cpp since it does the processing there.
extern const char8_t* ALLOC_TYPE_CMD_NAME;
extern const char8_t* ALLOC_TRACK_CMD_NAME;

#ifdef EA_PLATFORM_LINUX
extern const char _BlazeBuildTime[];
extern const char _BlazeBuildLocation[];
extern const char _BlazeP4DepotLocation[];
extern const char _BlazeChangelist[];
extern const char _BlazeBuildTarget[];
extern const char* _BlazeEditedFiles[];
#else
const char _BlazeBuildTime[] = "unknown";
const char _BlazeBuildLocation[] = "unknown";
const char _BlazeP4DepotLocation[] = "unknown";
const char _BlazeChangelist[] = "unknown";
const char _BlazeBuildTarget[] = "unknown";
extern const char* _BlazeEditedFiles[] = { nullptr };
#endif

namespace cfgtest { int cfgtest_main(int argc, char** argv); }
namespace email2hash { int email2hash_main(int argc, char** argv); }
namespace heat2logreader { int heat2logreader_main(int argc, char** argv); }
namespace memtest { int memtest_main(int argc, char** argv); }
namespace obfuscate { int obfuscate_main(int argc, char** argv); }
namespace rdirtransform { int rdirtransform_main(int argc, char** argv); }
namespace rpcinfo { int rpcinfo_main(int argc, char** argv); }
namespace profantest { int profantest_main(int argc, char** argv); }
namespace profancomp { int profancomp_main(int argc, char** argv); }
namespace codecperformance { int codecperformance_main(int argc, char** argv); }
#ifdef EA_PLATFORM_LINUX
namespace firedrill { int firedrill_main(int argc, char** argv); }
namespace monitor { int monitor_main(int argc, char** argv); }
namespace redisinspector { int redisinspector_main(int argc, char** argv); } // for now linux only, redisConnect fails on Win
#endif
namespace metricsinjector { int metricsinjector_main(int argc, char** argv); }
namespace protobuftest { int protobuftest_main(int argc, char** argv); }


using namespace Blaze;

static void shutdownHandler(void* context)
{
    if (gProcessController != nullptr)
    {
        gProcessController->shutdown(ProcessController::EXIT_NORMAL);
    }
}

static void reloadHandler(void* context)
{
    if (gProcessController != nullptr)
        gProcessController->reconfigure();
}

// local signal version of GOSCC's "drain" & "accept" buttons for a server instance
static void setServiceStateHandler(bool inService)
{
    BLAZE_INFO_LOG(Log::SYSTEM, "[BlazeServer] setServiceStateHandler: " << (inService ? "in-service" : "out-of-service"));
    if (gProcessController != nullptr)
        gProcessController->setServiceState(inService);
}

static void printMessage(const char8_t* msg)
{
    printf("%s", msg);
#if defined EA_PLATFORM_WINDOWS
    OutputDebugString(msg);
#endif
}

static void printErrorMessage(const char8_t* msg)
{
    EA::StdC::Fprintf(stderr, msg);
#if defined EA_PLATFORM_WINDOWS
    OutputDebugString(msg);
#endif
}

static int32_t usage(const char8_t* prg)
{
    eastl::string usage;
    usage.append_sprintf("Usage: Runs either the main Blaze Server or a tool\n");
    usage.append_sprintf("       %s -v|-vv|--list-log-categories\n", prg);
    usage.append_sprintf("       %s [blazeServerOptions]... <bootConfigFile>\n", prg);
    usage.append_sprintf("       <toolName> [toolSpecificOptions]...\n");
    usage.append_sprintf("       %s --tool <toolName> [toolSpecificOptions]...\n", prg);
    usage.append_sprintf("\n");
    usage.append_sprintf("Print requested information and exit:\n");
    usage.append_sprintf("    -v                    : Print version information\n");
    usage.append_sprintf("    -vv                   : Print verbose version information\n");
    usage.append_sprintf("    --list-log-categories : Return a list of configurable log categories\n");
    usage.append_sprintf("\n");
    usage.append_sprintf("Options for running a Blaze Server\n");
    usage.append_sprintf("    -l <label>            : Add label on command line to help with searching for the process\n");
    usage.append_sprintf("    -p <file>             : Specify the name of the .pid file\n");
    usage.append_sprintf("    -Dname[=var]          : Define a config file preprocessor entry\n");
    usage.append_sprintf("    --type <type>         : Specifies server instance type (see serverConfigs section of blaze.boot for types) (requires --id & --port)\n");
    usage.append_sprintf("    --port <port>         : Specifies server instance base port to bind to (requires --id & --type)\n");
    usage.append_sprintf("    --name <name>         : Specifies server instance name (REQUIRED)\n");
    usage.append_sprintf("    --dbdestructive       : Allow destructive actions on database\n");
    usage.append_sprintf("    --logdir <dir>        : Define the directory where logging should be output to\n");
    usage.append_sprintf("    --logname <file>      : Define the log file prefix for all log files.  If this is omitted, not logs will be written to disk\n");
    usage.append_sprintf("    --logoutput <kind>    : Provides an initial value for the output mode of the main server log.  This value is always overriden by the output setting logging.cfg\n");
    usage.append_sprintf("    --argfile <file>      : Define the argument file\n");
    usage.append_sprintf("    --bootFileOverride    : Specifies a series of values to override after parsing the boot file\n");
    usage.append_sprintf("    --monitor-pid <num>   : The PID of the monitor process to be signaled when an exception is hit\n");
    usage.append_sprintf("    --disable-config-string-escapes : Prevents the config parser from processing \\n into a newline in strings\n");
    usage.append_sprintf("    %s NORMAL|PASSTHROUGH : Selects the underlying allocator to use for all allocations\n", ALLOC_TYPE_CMD_NAME);
    usage.append_sprintf("    %s ALL(default)|NONE  : Toggles per-memory group allocation tracking (N/A: PASSTHROUGH)\n", ALLOC_TRACK_CMD_NAME);
    usage.append_sprintf("\n");
    usage.append_sprintf("Available tools (each have their own tool-specific arguments) - run by either:");
    usage.append_sprintf(" passing the tool name as the argument to the --tool option");
    usage.append_sprintf(" or invoking a soft link to this executable who's link name is the desired tool name\n");
    // NOTE We explicitly do not include obfuscate in the usage documentation to make it more difficult
    // for someone who compromises a blaze server box to decrypt database passwords
    usage.append_sprintf("    cfgtest                 Print the contents of a Blaze config file\n");
    usage.append_sprintf("    email2hash              Print the hash for an email address\n");
    usage.append_sprintf("    heat2logreader          Convert binary log file to human readable format\n");
    usage.append_sprintf("    memtest                 Test the memory tracker\n");
    usage.append_sprintf("    rdirtransform           Run a service name transform based on a config and specified inputs\n");
    usage.append_sprintf("    rpcinfo                 List all Blaze components and their ids\n");
    usage.append_sprintf("    profantest              Test profanity\n");
    usage.append_sprintf("    profancomp              Generates profanity binary file using the supplied pattern file input\n");
    usage.append_sprintf("    codecperformance        Measures TDF codec performance\n");


#ifdef EA_PLATFORM_LINUX
    usage.append_sprintf("    firedrill               Analyze fire/heat2 traffic from a packet capture\n");
    usage.append_sprintf("    monitor                 Run the application that manages and monitors a Blaze instance (typically only invoked by the server script)\n");
    usage.append_sprintf("    metricsinjector         Export blaze metrics for external collection.\n");
#endif
    usage.append_sprintf("    protobuftest            Simple program to test protobuf <-> EATDF serialization/deserialization.\n");

    printErrorMessage(usage.c_str());
    return ProcessController::EXIT_FAIL;
}

static void printVersion(const ServerVersion& version, bool verbose)
{
    eastl::string buf;
    buf.append_sprintf("%s\n", version.getVersion());
    buf.append("(c) Electronic Arts. All Rights Reserved.\n");
    buf.append_sprintf("Build time: %s\n", version.getBuildTime());
    buf.append_sprintf("Build location: %s\n", version.getBuildLocation());
    buf.append_sprintf("Build target: %s\n", version.getBuildTarget());
    buf.append_sprintf("Depot location: %s\n", version.getP4DepotLocation());
    if (_BlazeEditedFiles[0] != nullptr)
    {
        buf.append("Modified source files:\n");
        for(int32_t i = 0; _BlazeEditedFiles[i] != nullptr; ++i)
        {
            buf.append_sprintf("    %s\n", _BlazeEditedFiles[i]);
        }
    }

    printMessage(buf.c_str());
}

static void buildVersion(ServerVersion& version)
{
    char8_t versBuf[256];
    blaze_snzprintf(versBuf, sizeof(versBuf), "Blaze %s (CL# %s)",
            _BlazeVersion, _BlazeChangelist);
    version.setVersion(versBuf);
    version.setBuildTime(_BlazeBuildTime);
    version.setBuildLocation(_BlazeBuildLocation);
    version.setBuildTarget(_BlazeBuildTarget);
    version.setP4DepotLocation(_BlazeP4DepotLocation);
    uint32_t modifiedSrc = 0;
    for(modifiedSrc = 0; _BlazeEditedFiles[modifiedSrc] != nullptr; ++modifiedSrc)
        ;
    version.setModifiedSourceFiles(modifiedSrc);
}

static uint32_t getProcessId()
{
#if defined EA_PLATFORM_LINUX
    return getpid();
#else   // defined EA_PLATFORM_WINDOWS
    return GetCurrentProcessId();
#endif
}

#if defined(EA_PLATFORM_LINUX)
extern char _binary_vers_tar_gz_start[];
extern char _binary_vers_tar_gz_end[];
extern unsigned long long _binary_vers_tar_gz_size[];
#endif

static bool buildArgMap(const char8_t* arg, CommandLineArgs& argMap, bool& needsValue, int32_t& predefinesCount)
{
    if (arg == nullptr)
        return false;

    needsValue = false;

    if (strcmp(arg, cVersionInfo) == 0 || strcmp(arg, cVerboseVersion) == 0)
    {
        return false;
    }

    else if (strcmp(arg, cLabel) == 0 
        || strcmp(arg, cPidFile) == 0 
        || strcmp(arg, cLogDir) == 0 
        || strcmp(arg, cLogName) == 0
        || strcmp(arg, cLogOutput) == 0
        || strcmp(arg, cArgFile) == 0
        || strcmp(arg, cOverrideFile) == 0
        || strcmp(arg, cMonitorPid) == 0
        || strcmp(arg, cInstanceId) == 0
        || strcmp(arg, cInstanceType) == 0
        || strcmp(arg, cInstanceName) == 0
        || strcmp(arg, cInstancePort) == 0
        || strcmp(arg, cInternalPorts) == 0
        || strcmp(arg, cExternalPorts) == 0)
    {
        needsValue = true;
    }
    else if (strcmp(arg, cDbdestructive) == 0)
    {            
        argMap[cDbdestructive] = "";
    }
    else if (strcmp(arg, cDisableFirewallCheck) == 0)
    {
        argMap[cDisableFirewallCheck] = "";
    }
    else if (strcmp(arg, cOutOfService) == 0)
    {
        argMap[cOutOfService] = "";
    }
    else if (strcmp(arg, cDisableConfigStringEscapes) == 0)
    {
        argMap[cDisableConfigStringEscapes] = "";
    }
    else if (strcmp(arg, cListCategories) == 0)
    {
        argMap[cListCategories] = "";
    }     
    else if ((arg[0] == '-') && (arg[1] == 'D'))
    {
        argMap[arg] = "";
        ++predefinesCount;
    }
    else if (strcmp(arg, ALLOC_TYPE_CMD_NAME) == 0)
    {
        needsValue = true; 
    }
    else if (strcmp(arg, ALLOC_TRACK_CMD_NAME) == 0)
    {
        needsValue = true; 
    }
    else if (arg[0] == '-')
    {
        return false;
    }
    else
    {
        argMap[cConFile] = arg;
    }

    return true;
}

static bool printVersionInfo(const char8_t* arg, const ServerVersion& version)
{
    if (arg == nullptr)
        return false;

    if (strcmp(arg, cVersionInfo) == 0)
    {
        printVersion(version, false);
        return true;
    }
    else if (strcmp(arg, cVerboseVersion) == 0)
    {
        printVersion(version, true);
        return true;
    }
    
    return false;
}

static bool readArgFile(CommandLineArgs& argMap, eastl::string& sArg, int32_t& predefinesCount)
{
    if (!sArg.empty())
        sArg.clear();

    CommandLineArgs::iterator aIt;

    if ((aIt = argMap.find(cArgFile)) != argMap.end())
    {
        FILE* aFile = fopen(aIt->second.c_str(), "r");
        if (aFile == nullptr)
        {
            eastl::string errorLog;
            errorLog.sprintf("Specified argument file doesn't exist: %s \n", aIt->second.c_str());
            printErrorMessage(errorLog.c_str());
            return false;
        }
        else
        {
            
            char8_t ch;
            ch  = EOF;
            eastl::string sValue;

            bool needsValue = false;

            while ((ch  = (char8_t)fgetc(aFile)) != EOF || !sArg.empty() || !sValue.empty())
            {
                if (ch != EOF && !isspace(ch))
                {
                    if (!needsValue)
                    {
                        sArg.append_sprintf("%c", ch);
                    }
                    else
                    {
                        sValue.append_sprintf("%c", ch);
                    }
                    continue;
                }

                if (!needsValue)
                {
                    if (!sArg.empty())
                    {
                        if(!buildArgMap(sArg.c_str(), argMap, needsValue, predefinesCount))
                        {
                            fclose(aFile);
                            return false;
                        }

                        if (strcmp(sArg.c_str(), cArgFile) == 0)
                        {
                            eastl::string errorLog;
                            errorLog.sprintf("Argument file cannot include the parameter --argfile \n");
                            printErrorMessage(errorLog.c_str());
                            fclose(aFile);
                            return false;
                        }

                        if (!needsValue)
                            sArg.clear(); 
                    }
                }
                else
                {
                    if (!sValue.empty())
                    {
                        argMap[sArg.c_str()] = sValue.c_str();
                        sArg.clear();
                        sValue.clear();
                        needsValue = false;
                    }
                    else
                    {
                        fclose(aFile);
                        return false;
                    }
                }
            }

            fclose(aFile);
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int blazeserver_main(int32_t argc, char8_t** argv)
{
    eastl::string configFileName;
    eastl::string overrideFileName;
    const char8_t* pidFileName = nullptr;
    bool listLogCategories = false;
    bool configStringEscapes = true;
    const char8_t* logDir = nullptr;
    const char8_t* logName = nullptr;
    const char8_t* logOutput = nullptr;
    bool needsValue = false;
    int32_t predefinesCount = 0;

    CommandLineArgs argMap;

    EA::TDF::TdfFactory::fixupTypes();

    ServerVersion version;
    buildVersion(version);

    PredefineMap predefineMap;
    predefineMap.addBuiltInValues();


    for(int32_t arg = 1; arg < argc; ++arg)
    {
        
        if(!buildArgMap(argv[arg], argMap, needsValue, predefinesCount))
        {
            if (printVersionInfo(argv[arg], version))
                return ProcessController::EXIT_NORMAL;
            else
                return usage(argv[0]);
        }
            

        if (needsValue)
        {
            if (arg + 1 == argc)
                return usage(argv[0]);

            argMap[argv[arg]] = argv[arg + 1];
            ++arg;
        }
    }
   
    eastl::string sArg;

    if (!readArgFile(argMap, sArg, predefinesCount))
    {
        if (!sArg.empty() && printVersionInfo(sArg.c_str(), version))
            return ProcessController::EXIT_NORMAL;

        else
            return usage(argv[0]);
    }

    if (argMap.find(cInstanceId) != argMap.end())
    {
        eastl::string warnLog;
        warnLog.sprintf("WARNING: Parameter %s is deprecated \n", cInstanceId);
        printMessage(warnLog.c_str());
    }

    const bool disableFirewallCheck = argMap.find(cDisableFirewallCheck) != argMap.end();
    const bool instanceTypeSet = argMap.find(cInstanceType) != argMap.end();
    const bool instanceNameSet = argMap.find(cInstanceName) != argMap.end();
    const bool instancePortSet = argMap.find(cInstancePort) != argMap.end();
    const bool portListSet = argMap.find(cInternalPorts) != argMap.end() || argMap.find(cExternalPorts) != argMap.end();
    const bool bootSingleInstance = instanceTypeSet || instanceNameSet || instancePortSet;

    if (instancePortSet && portListSet)
    {
        eastl::string errorLog = "Argument --port is mutually exclusive with --internalports and --externalports.\n";
        printErrorMessage(errorLog.c_str());
        return usage(argv[0]);
    }

    // Single instances can be booted without specifying port (either via --port or --internalports/--externalports),
    // but this will change if/when PAS support is removed
    if (bootSingleInstance && !(instanceNameSet && instanceTypeSet))
    {
        eastl::string errorLog = "Missing argument(s) {--type|--name} while booting single instance.\n";
        printErrorMessage(errorLog.c_str());
        return usage(argv[0]); // if one of name/type is set the others are required
    }

    ProcessController::CmdParams params;
    params.processId = getProcessId();

    CommandLineArgs::iterator aIt;
    CommandLineArgs::iterator aEnd = argMap.end();

    if ((aIt = argMap.find(cLogName)) != aEnd)
    {
        logName = aIt->second.c_str(); 
    }

    if ((aIt = argMap.find(cLogDir)) != aEnd)
    {
        logDir = aIt->second.c_str(); 
    }

    if ((aIt = argMap.find(cLogOutput)) != aEnd)
    {
        logOutput = aIt->second.c_str();
    }

    ProcessController::ExitCode exitCode = ProcessController::EXIT_NORMAL;

    // Scope for fiberStorage
    {
        // Must initialize fiber storage before Logger
        Fiber::FiberStorageInitializer fiberStorage;

        LoggingConfig::Output logOutputVal = LoggingConfig::BOTH;
        if (logOutput && !LoggingConfig::ParseOutput(logOutput, logOutputVal))
        {
            eastl::string errorLog;
            errorLog.sprintf("Invalid value '%s' specified for --logoutput argument. Valid values are \"file\", \"stdout\", or \"both\". Default is \"both\".", logOutput);
            printErrorMessage(errorLog.c_str());
            return ProcessController::EXIT_FAIL;
        }

        BlazeRpcError err = Logger::initialize(Logging::INFO, logDir, logName, logOutputVal);
        if (err != Blaze::ERR_OK)
        {
            eastl::string errorLog;
            errorLog.sprintf("Failed to initialize logger(logDir=%s, logName=%s, logOutput=%s) \n", logDir, logName, logOutput);
            printErrorMessage(errorLog.c_str());
            return ProcessController::EXIT_FAIL;
        }
#if defined(TARGET_gamereporting) 
        GameReporting::registerCustomReportTdfs();
#endif
        BlazeRpcComponentDb::initialize();
        BlazeRpcComponentDb::dumpIndexInfoToLog();

        if ((aIt = argMap.find(cConFile)) != aEnd)
        {
            configFileName = aIt->second.c_str(); 
        }

        if ((aIt = argMap.find(cOverrideFile)) != aEnd)
        {
            overrideFileName = aIt->second.c_str(); 
        }

        if ((aIt = argMap.find(cInstancePort)) != aEnd)
        {
            params.instanceBasePort = (uint16_t)EA::StdC::AtoU32(aIt->second.c_str());
        }
        if ((aIt = argMap.find(cInternalPorts)) != aEnd)
        {
            params.internalPorts = aIt->second;
        }
        if ((aIt = argMap.find(cExternalPorts)) != aEnd)
        {
            params.externalPorts = aIt->second;
        }
    
        if ((aIt = argMap.find(cInstanceType)) != aEnd)
        {
            blaze_strnzcpy(params.instanceType, aIt->second.c_str(), sizeof(params.instanceType));
        }
    
        if ((aIt = argMap.find(cInstanceName)) != aEnd)
        {
            blaze_strnzcpy(params.instanceName, aIt->second.c_str(), sizeof(params.instanceName));
        }

        if (argMap.find(cDbdestructive) != aEnd)
        {
            params.allowDestruct = true;
        }
    
        if (argMap.find(cOutOfService) != aEnd)
        {
            params.startInService = false;
        }

        if (argMap.find(cDisableConfigStringEscapes) != aEnd)
        {
            // enabled by default, we can disable with the flag.
            configStringEscapes = false;
        }

        if (argMap.find(cListCategories) != aEnd)
        {
            listLogCategories = true;
        }

        if ((aIt = argMap.find(cPidFile)) != aEnd)
        {
            pidFileName = aIt->second.c_str(); 
        }

        if ((aIt = argMap.find(cMonitorPid)) != aEnd)
        {
            params.monitorPid = EA::StdC::AtoU32(aIt->second.c_str());
        }

        aIt = argMap.begin();
        for (; aIt != aEnd; ++aIt)
        {
            if ((aIt->first.c_str()[0] == '-') && (aIt->first.c_str()[1] == 'D'))
            {
                predefineMap.addFromKeyValuePair(&aIt->first.c_str()[2]);
            }
        }

        BLAZE_INFO_LOG(Log::SYSTEM, "Version        : " << version.getVersion());
        BLAZE_INFO_LOG(Log::SYSTEM, "Build location : " << version.getBuildLocation());
#if defined(EA_PLATFORM_UNIX) || defined(EA_PLATFORM_LINUX)
        char currentworkpath[1024];
        BLAZE_INFO_LOG(Log::SYSTEM, "Current path   : " << getcwd(currentworkpath,sizeof(currentworkpath)));
#endif
        for(int32_t arg = 0; arg < argc; ++arg)
        {
            BLAZE_INFO_LOG(Log::SYSTEM, "argv[" << arg << "]        : " << argv[arg]);
        }

        if (listLogCategories)
        {
            printMessage("Available log category names:\n");
            Logger::CategoryNameList* categories = Logger::getRegisteredCategoryNames();
            for(Logger::CategoryNameList::iterator i = categories->begin(); i != categories->end(); ++i)
            {
                char buffer[128];
                blaze_snzprintf(buffer, sizeof(buffer), "    %s\n", *i); // blaze_snzprintf clears the buffer in case of an error so no init step is necessary
                printMessage(buffer);
            }
            Logger::destroy();
            return ProcessController::EXIT_NORMAL;
        }

        if (configFileName.empty())
        {
            Logger::destroy();
            return usage(argv[0]);
        }

        const char8_t* zlibVersionString = BaseZlibCompression::getZlibVersion();
        if (blaze_strcmp(zlibVersionString, BaseZlibCompression::getZlibInterfaceVersion()) != 0)
        {
            BLAZE_FATAL_LOG(Log::SYSTEM, "[BlazeServer] Zlib version mismatch! Linked: " << zlibVersionString 
                << ", Required: " << BaseZlibCompression::getZlibInterfaceVersion() << ", check build script for accidental usage of -lz option pulling in the system zlib.");
            Logger::destroy();
            return (int)ProcessController::EXIT_FATAL;
        }

#if defined EA_PLATFORM_LINUX  || defined EA_PLATFORM_WINDOWS
        char8_t pidFile[256];
        if (pidFileName == nullptr)
        {
            // Create PID file based on executable name if no pid file given on command line
            char8_t* s = strrchr(argv[0], '/');
            if (s == nullptr)
                s = argv[0];
            else
                s++;
            blaze_snzprintf(pidFile, sizeof(pidFile), "%s.pid", s);
            pidFileName = pidFile;
        }
        FILE* pidFp = fopen(pidFileName, "w");
        if (pidFp == nullptr)
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "Failed to write PID file (" << pidFileName << "): " << strerror(errno));
        }
        else
        {
            EA::StdC::Fprintf(pidFp, "%u\n", getProcessId());
            BLAZE_INFO_LOG(Log::SYSTEM, "PID = " << getProcessId());
            fclose(pidFp);
        }
#endif

        char8_t currGmtTime[64];
        BLAZE_INFO_LOG(Log::SYSTEM, "Current GMT time = " << TimeValue::getTimeOfDay().toString(currGmtTime, sizeof(currGmtTime)));

#ifdef EA_PLATFORM_LINUX
        // ignore SIGPIPE which gets generated on socket activity.  If not ignored, a SIGPIPE will
        // terminate the server.
        signal(SIGPIPE, SIG_IGN);  
#endif

        if (!InstallConsoleSignalHandler(shutdownHandler, nullptr, reloadHandler, nullptr, setServiceStateHandler))
        {
            BLAZE_FATAL_LOG(Log::SYSTEM, "[BlazeServer] fail to install console signal handler. Please try to start the server again.");
            Logger::destroy();
            return (int)ProcessController::EXIT_FATAL;
        }

        if (params.allowDestruct)
        {
            BLAZE_INFO_LOG(Log::SYSTEM, "[BlazeServer] Server set to allow destructive database actions.");
        }
#if ENABLE_CLANG_SANITIZERS
        BLAZE_INFO_LOG(Log::SYSTEM, "[BlazeServer] Server is built with Address Sanitizer(ASan) or Leak Sanitizer(LSan) support.");
#endif

        if (!disableFirewallCheck) // If the --disable-check-firewall command line option is *not* set
        {
            // Configure the Windows Firewall
            if (!SocketUtil::configureInboundWindowsFirewallRule())
            {
                BLAZE_WARN_LOG(Log::CONNECTION, "[BlazeServer] Failed to configure the Windows Firewall");
            }
        }

        SocketUtil::initializeNetworking();

        Blaze::Grpc::grpcCoreSetupAllocationCallbacks(); // This needs to happen before the grpc_init call as that allocates memory.  
        grpc_init();

        //Open a boot util and try to initialize it
        BLAZE_INFO_LOG(Log::SYSTEM, "[BlazeServer] Parsing boot file.");
        ConfigBootUtil bootUtil(configFileName, overrideFileName, predefineMap, configStringEscapes);
        if (bootUtil.initialize())
        {
            //Start and run the server - the process controller will only live inside this if statement
            ProcessController processController(bootUtil, version, params, argMap);
            if (processController.initialize())
            {
                processController.start();
            }

            ProcessController::logShutdownCause();

            exitCode = processController.getExitCode();
        }
        else
        {
            //Boot failed to parse.  It will print a message internally, just bail.
            exitCode = ProcessController::EXIT_FAIL;
        }

        grpc_shutdown();
        google::protobuf::ShutdownProtobufLibrary();

        SocketUtil::shutdownNetworking();

        ResetDefaultConsoleSignalHandler();


        BLAZE_INFO_LOG(Log::SYSTEM, "[BlazeServer] Shutdown complete.");

        Logger::destroy();
#if defined(TARGET_gamereporting)
        GameReporting::deregisterCustomReportTdfs();
#endif
    }
    
    EA::TDF::TdfFactory::cleanupTypeAllocations();

    freeThreadLocalInfo();

#ifdef EA_PLATFORM_LINUX
    // This should always be the last thing we do on process exit to ensure that the pid file
    // is always available to the server startup/shutdown scripts so the server can be forcible
    // killed if it doesn't want to shutdown cleanly.
    remove(pidFileName);
#endif

    return (int) exitCode;
}

int program_main(int argc, char** argv)
{
    gIsMainThread = true; 

    // The Blaze Server executable includes several tools which are linked into the main executable.
    // Tools may be invoked either by specifying --tool and the tool name as the first arguments
    // on the command line, or by invoking the executable via the tool name
    // (this would be accomplished either by renaming the exe but more typically via a soft link).
    //
    // The tool variable indicates whether we've found a --tool option on the command line.
    // If so, it is a failure if we do not find the requested tool and we exit.
    bool tool = false;
    char* toolName = argv[0];
    eastl::string exe = toolName;

    if (argc >= 2 && strcmp(argv[1], cTool) == 0)
    {
        // If --tool is specified, then we must have a tool name as well
        if (argc < 3)
            return (usage(argv[0]));

        // Tweak the command line args so that each tool sees 'exeName --tool <toolName>'
        // as a single argument
        tool = true;
        toolName = argv[2];
        exe.sprintf("%s %s %s", argv[0], argv[1], argv[2]);
        argc -= 2;
        argv[2] = const_cast<char*>(exe.c_str());
        argv = &argv[2];
    }
    
    // Tools are intended to be used via soft links, we choose the appropriate 'main' based on command line exe name
#ifndef BLAZE_LITE
    if (strstr(toolName, "cfgtest"))
        return cfgtest::cfgtest_main(argc, argv);
    if (strstr(toolName, "email2hash"))
        return email2hash::email2hash_main(argc, argv);
    if (strstr(toolName, "heat2logreader"))
        return heat2logreader::heat2logreader_main(argc, argv);
    if (strstr(toolName, "memtest"))
        return memtest::memtest_main(argc, argv);
    if (strstr(toolName, "obfuscate"))
        return obfuscate::obfuscate_main(argc, argv);
    if (strstr(toolName, "rdirtransform"))
        return rdirtransform::rdirtransform_main(argc, argv);
    if (strstr(toolName, "rpcinfo"))
        return rpcinfo::rpcinfo_main(argc, argv);
    if (strstr(toolName, "profantest"))
        return profantest::profantest_main(argc, argv);
    if (strstr(toolName, "profancomp"))
        return profancomp::profancomp_main(argc, argv);
    if (strstr(toolName, "codecperformance"))
        return codecperformance::codecperformance_main(argc, argv);
#ifdef EA_PLATFORM_LINUX 
    if (strstr(toolName, "firedrill"))
        return firedrill::firedrill_main(argc, argv);
    if (strstr(toolName, "monitor"))
        return monitor::monitor_main(argc, argv);
    if (strstr(toolName, "redisinspector"))
        return redisinspector::redisinspector_main(argc, argv);
#endif
    if (strstr(toolName, "metricsinjector"))
        return metricsinjector::metricsinjector_main(argc, argv);
    if (strstr(toolName, "protobuftest"))
        return protobuftest::protobuftest_main(argc, argv);
#endif // !BLAZE_LITE

#ifdef EA_PLATFORM_WINDOWS
    if (strstr(toolName, "addfwrules"))
    {
        return (SocketUtil::configureInboundWindowsFirewallRule() ? 0 : 1);
    }
#endif

    if (tool)
        return(usage(argv[0]));

    return blazeserver_main(argc, argv);
}
