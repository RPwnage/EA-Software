/////////////////////////////////////////////////////////////////////////////
// EATrace/EALogConfig.cpp
//
// Copyright (c) 2005 Electronic Arts Inc.
// Written by Jon Parise.
// Maintained by Paul Pedriana.
/////////////////////////////////////////////////////////////////////////////


#include <EATrace/internal/Config.h>
#if defined(_MSC_VER)
    #pragma warning(push)          // This disabling is needed only for EATextUtil.h for older EAStdC versions building on 64 bit platforms. Remove this at January 2013.
    #pragma warning(disable: 4267) // 'argument' : conversion from 'size_t' to ...
#endif
#include <EAStdC/EATextUtil.h>
#include <EAStdC/EAString.h>
#include <EAIO/EAIniFile.h>
#include <EAIO/EAFileUtil.h>
#include <EATrace/EALogConfig.h>
#include <EATrace/internal/RefCount.h>
#include <EATrace/Allocator.h>
#include <EATrace/EALog_imp.h>
#include <EATrace/EATrace.h>
#include <coreallocator/icoreallocator_interface.h>


namespace EA
{
    namespace Trace
    {
        namespace
        {
            struct ConfigurationParams
            {
                ConfigurationParams(IO::IniFile &config, Allocator::ICoreAllocator *pAllocator) :
                    mConfig(config),
                    mpAllocator(pAllocator)
                { }

                IO::IniFile &mConfig;
                Allocator::ICoreAllocator *mpAllocator;

            private:
                ConfigurationParams(const ConfigurationParams &);
                ConfigurationParams & operator=(const ConfigurationParams &);
            };

            tLevel GetLevelFromName(EA::IO::StringW& sName)
            {
                if(sName.comparei(EA_WCHAR("all")) == 0)
                    return kLevelAll;
                else if (sName.comparei(EA_WCHAR("none")) == 0)
                    return kLevelNone;
                else if (sName.comparei(EA_WCHAR("min")) == 0)
                    return kLevelMin;
                else if (sName.comparei(EA_WCHAR("debug")) == 0)
                    return kLevelDebug;
                else if (sName.comparei(EA_WCHAR("info")) == 0)
                    return kLevelInfo;
                else if (sName.comparei(EA_WCHAR("warn")) == 0)
                    return kLevelWarn;
                else if (sName.comparei(EA_WCHAR("error")) == 0)
                    return kLevelError;
                else if (sName.comparei(EA_WCHAR("fatal")) == 0)
                    return kLevelFatal;
                else if (sName.comparei(EA_WCHAR("max")) == 0)
                    return kLevelMax;
                else
                    EA_FAIL_F(("Unknown filter level: %S", sName.c_str()));

                return kLevelUndefined;
            }

            bool ConfigureReporter(const wchar_t* pSection, const wchar_t*, void* pContext)
            {
                const bool                 kContinueEnumeration(true);
                EA::IO::String8            sReporter;
                IServer* const             pServer = GetServer();
                AutoRefCount<ILogReporter> pReporter;
                const char8_t*             pLineEnd = "\n";

                // Extract our configurtion parameters.
                ConfigurationParams* const       pParams = reinterpret_cast<ConfigurationParams*>(pContext);
                IO::IniFile&                     config = pParams->mConfig;
                Allocator::ICoreAllocator* const pAllocator = pParams->mpAllocator;

                EA::StdC::Strlcpy(sReporter, pSection);

                // Start by attempting to get an existing reporter with this name.
                if (!pServer->GetLogReporter(sReporter.c_str(), pReporter.AsPPTypeParam()))
                {
                    EA::IO::StringW sType;
                    if (config.ReadEntry(pSection, EA_WCHAR("Type"), sType) <= 0)
                        sType = EA_WCHAR("debugger");

                    // Create a new reporter with the specified parameters.
                    if (sType.comparei(EA_WCHAR("debugger")) == 0)
                    {
                        pReporter = new(pAllocator, EATRACE_ALLOC_PREFIX "LogReporterDebugger") LogReporterDebugger(sReporter.c_str());
                    }
                    else if (sType.comparei(EA_WCHAR("file")) == 0)
                    {
                        EA::IO::StringW sFilename;
                        if (config.ReadEntry(pSection, EA_WCHAR("Filename"), sFilename) <= 0)
                        {
                            sFilename = pSection;
                            sFilename.append(EA_WCHAR(".log"));
                        }
                        pLineEnd = EA_LINEEND_WINDOWS_8;
                        EA::IO::File::Remove(sFilename.c_str());
                        pReporter = new(pAllocator, EATRACE_ALLOC_PREFIX "LogReporterFile") LogReporterFile(sReporter.c_str(), sFilename.c_str());
                    }
                    else if (sType.comparei(EA_WCHAR("stdio")) == 0)
                    {
                        pReporter = new(pAllocator, EATRACE_ALLOC_PREFIX "LogReporterStdio") LogReporterStdio();
                    }
                    else if (sType.comparei(EA_WCHAR("dialog")) == 0)
                    {
                        pReporter = new(pAllocator, EATRACE_ALLOC_PREFIX "LogReporterDialog") LogReporterDialog(sReporter.c_str(), EA::Trace::kOutputTypeText);
                    }
                    else if (sType.comparei(EA_WCHAR("console")) == 0)
                    {
                        // Defer creation of the console reporter to the user interface.
                        return kContinueEnumeration;
                    }
                    else
                    {
                        EA_FAIL_F(("Unknown log reporter type '%ls' for reporter %hs", sType.c_str(), sReporter.c_str()));
                        return kContinueEnumeration;
                    }

                    EA_VERIFY(pServer->AddLogReporter(pReporter));
                }
                EA_ASSERT(pReporter);

                EA::IO::StringW sGlobalLevel;
                if (config.ReadEntry(pSection, EA_WCHAR("Level"), sGlobalLevel) <= 0)
                    sGlobalLevel = EA_WCHAR("none");

                EA::IO::StringW sFilter;
                if (config.ReadEntry(pSection, EA_WCHAR("Filter"), sFilter) <= 0)
                    sFilter.clear();

                EA::Trace::AutoRefCount<EA::Trace::LogFilterGroupLevels> pFilter = new(pAllocator, EATRACE_ALLOC_PREFIX "FilterGroupLevels") EA::Trace::LogFilterGroupLevels;
                if (pFilter)
                {
                    pFilter->AddGroupLevel(NULL, GetLevelFromName(sGlobalLevel));

                    // Now apply any specific per-group level filters.
                    if (!sFilter.empty())
                    {
                        EA::IO::StringW sToken, sGroup, sLevel;

                        while (EA::StdC::SplitTokenDelimited(sFilter, EA_WCHAR(','), &sToken))
                        {
                            if (sToken.empty()) 
                                continue;

                            EA::StdC::SplitTokenSeparated(sToken, EA_WCHAR(':'), &sGroup);
                            EA::StdC::SplitTokenSeparated(sToken, EA_WCHAR(':'), &sLevel);

                            if (sGroup.empty() || sLevel.empty())
                                continue;

                            EA::IO::String8 sGroup8;
                            EA::StdC::Strlcpy(sGroup8, sGroup);
                            pFilter->AddGroupLevel(sGroup8.c_str(), GetLevelFromName(sLevel));
                        }
                    }

                    pReporter->SetFilter(pFilter);
                }

                EA::IO::StringW sFormatter;
                if (config.ReadEntry(pSection, EA_WCHAR("Formatter"), sFormatter) <= 0)
                    sFormatter = EA_WCHAR("fancy");

                if (sFormatter.comparei(EA_WCHAR("prefixed")) == 0)
                    pReporter->SetFormatter(new(pAllocator, EATRACE_ALLOC_PREFIX "PrefixedFormatter") EA::Trace::LogFormatterPrefixed("prefixed", pLineEnd));
                else if (sFormatter.comparei(EA_WCHAR("simple")) == 0)
                    pReporter->SetFormatter(new(pAllocator, EATRACE_ALLOC_PREFIX "SimpleFormatter") EA::Trace::LogFormatterSimple("simple"));
                else
                {
                    EA::Trace::LogFormatterFancy* pFormatter = new(pAllocator, EATRACE_ALLOC_PREFIX "FancyFormatter") EA::Trace::LogFormatterFancy("fancy", pAllocator);
                    if (pFormatter)
                        pFormatter->SetFlags(EA::Trace::LogFormatterFancy::kFlagGroup | EA::Trace::LogFormatterFancy::kFlagLevel);
                    pReporter->SetFormatter(pFormatter);
                }

                return kContinueEnumeration;
            }
        }

        bool Configure(IO::IniFile& config, const wchar_t* pReporter, Allocator::ICoreAllocator* pAllocator)
        {
            // Make sure the configuration file exists.
            if (!IO::File::Exists(config.GetPath()))
                return false;

            // We definitely need a log server, too.
            IServer* const pServer = GetServer();
            if (!pServer)
                return false;

            // Create a context structure to parameterize the configuration.
            ConfigurationParams params(config, (pAllocator) ? pAllocator : EA::Trace::GetAllocator());

            if (pReporter)
            {
                // If we don't have a section in the configuration file for this
                // reporter, return failure.
                if (!config.SectionExists(pReporter))
                    return false;

                // Configure just the specificly-named reporter.
                ConfigureReporter(pReporter, NULL, &params);
            }
            else
            {
                // Walk each of the sections in the configuration file.
                config.EnumSections(ConfigureReporter, &params);
            }

            // Update the trace helper tables with the (potentially) updated filters.
            pServer->UpdateLogFilters();

            return true;
        }

    } // namespace Trace
} // namespace EA


#if defined(_MSC_VER)
    #pragma warning(pop)
#endif


