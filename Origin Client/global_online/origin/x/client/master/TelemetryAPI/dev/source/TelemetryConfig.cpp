///////////////////////////////////////////////////////////////////////////////
// TelemetryConfig.cpp
//
// Keeps the TelemetryAPI config values
//// Copyright (c) Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "TelemetryConfig.h"
#include "EbisuMetrics.h"
#include "TelemetryLogger.h"


#include"Utilities.h"

#include "EAStdC/EADateTime.h"
#include "EAStdC/EAString.h"
#include "EAIO/EAIniFile.h"
#include "EAAssert/eaassert.h"

#ifdef ORIGIN_MAC
#include "EAIO/EAFileUtil.h"
#endif

namespace OriginTelemetry
{
    //CONSTANTS
    eastl::string8 PROD_TELEMETRY_ENDPOINT         = eastl::string8("https://river.data.ea.com");
    eastl::string8 TEST_TELEMETRY_ENDPOINT          = eastl::string8("https://river-test.data.ea.com");

    // Initializing static members of TelemetryConfig
#if defined(ORIGIN_PC)
    const eastl::string8 TELEMETRY_DOMAIN = "ORIGIN/origin-2014-pc";
    const std::wstring TELEMETRY_LOGFILE_PATH = L"Origin\\Telemetry\\";
    const std::wstring ORIGIN_LOGFILE_PATH = L"Origin\\Logs\\";
#elif defined(ORIGIN_MAC)
    const eastl::string8 TELEMETRY_DOMAIN = "ORIGIN/origin-2014-mac";
    const eastl::wstring TELEMETRY_LOGFILE_PATH = L"Application Support/Origin/Telemetry/";
    const eastl::wstring ORIGIN_LOGFILE_PATH = L"Application Support/Origin/Logs/";
#else
#error Need to define telemetry domain name and data path for platform
#endif

    const wchar_t* const TELMETRY_XML_ERROR_FILENAME = L"dserrors.data";
    eastl::wstring TelemetryConfig::sTelemetryErrorFilename = TELMETRY_XML_ERROR_FILENAME;
    eastl::wstring TelemetryConfig::sTelemetryLogFilename = L"telemetryFile.xml";

    const wchar_t* const EACORE_INI_FILENAME = L"EACore.ini";
    const wchar_t* const EACORE_INI_TELEMETRY_SECTION = L"Telemetry";
    const wchar_t* const EACORE_INI_TELEMETRY_BREAKPOINT_SECTION = L"TelemetryBreakpoints";

    // static memeber variables.
    uint32_t TelemetryConfig::sSpamLevel = 0;

    eastl::string8 TelemetryConfig::sTelemeryServer = OriginTelemetry::PROD_TELEMETRY_ENDPOINT;
    eastl::string8 TelemetryConfig::sHooksFilter = "";
    //The default for filtering telemetry by country should go here.  Currently legal said that we don't need to filter by country so it's empty.
    eastl::string8 TelemetryConfig::sCountryFilter = "";
    int TelemetryConfig::sTelemetryPort = TelemetryConfig::HTTPS;
    bool TelemetryConfig::sIsTelemetryServerOverriden = false;
    bool TelemetryConfig::sHooksBlacklistOverriden = false;
    bool TelemetryConfig::sCountriesBlacklistOverriden = false;

    bool TelemetryConfig::sIsTelemetryXMLEnabled = false;

    eastl::string8 TelemetryConfig::mUniqueMachineHash;

    eastl::map<int32_t, eastl::wstring> TelemetryConfig::sTelemetryBreakpointMap;
    bool TelemetryConfig::sTelemetryBreakpointsEnabled = false;
    eastl::string8 TelemetryConfig::sLocaleOverride("");
    bool TelemetryConfig::sIsBOI = false;


    /// Static helper function for getting the path to the eacore.ini file.
    static EA::IO::StringW GetEACoreIniFilePath()
    {
        EA::IO::StringW ebisuIniName;
        EA::IO::StringW eacoreIniName;

#if defined(ORIGIN_PC)
        // on windows, eacore.ini file is located in the same directory as the origin executable
        char16_t processName[4096]={0};
        NonQTOriginServices::Utilities::getExecutablePath(*processName, sizeof(processName)/sizeof(processName[0]));
        ebisuIniName.assign(processName);
        EA::IO::String16::size_type pos = ebisuIniName.find_last_of(L"\\");
        if ( pos == EA::IO::String16::npos || pos+1 >= ebisuIniName.length() )
        {
            return eacoreIniName; // empty string
        }
        ebisuIniName.replace(  pos + 1, ebisuIniName.length() - (pos + 1), (L"\0") );
#elif defined(ORIGIN_MAC)
        // on mac, eacore.ini is in ~/Library/Application Support/Origin/
        const size_t kPathCapacity = 512;
        wchar_t iniDirectory[kPathCapacity];
        EA::IO::GetSpecialDirectory(EA::IO::kSpecialDirectoryUserApplicationData, iniDirectory, true, kPathCapacity);
        ebisuIniName = iniDirectory; //
        ebisuIniName.append(L"Application Support/Origin/");
#else
#error "Require platform implementation."
#endif    

        eacoreIniName = ebisuIniName; 
        eacoreIniName += EACORE_INI_FILENAME;

        return eacoreIniName;
    }

    //Helper function to get the MGS for a hook as a string.
    static eastl::string8 GetTelemetryModuleGroupStringId(const char8_t** MetricListAccessor)
    {
        eastl::string8 mgs = "";

        char8_t** metricItem = (char8_t**) MetricListAccessor;
        mgs += *metricItem;
        mgs += ":";
        if((*metricItem)!=NULL)
            metricItem++;
        mgs += *metricItem;
        mgs += ":";
        if((*metricItem)!=NULL)
            metricItem++;
        mgs += *metricItem;

        return mgs;
    }

    void TelemetryConfig::preemptErrorFileName(const eastl::string8 &pfx)
    {
        eastl::wstring widePfx = EA::StdC::ConvertString<eastl::string8, eastl::wstring>(pfx);
        sTelemetryErrorFilename = widePfx + sTelemetryErrorFilename;
        sTelemetryLogFilename = widePfx + sTelemetryLogFilename;
    }

    void TelemetryConfig::setUniqueMachineHash(uint64_t val)
    {
        mUniqueMachineHash = NonQTOriginServices::Utilities::printUint64AsHex(val);
    }

    eastl::string8 TelemetryConfig::uniqueMachineHash()
    {
        if (mUniqueMachineHash.empty())
            EA_FAIL_MSG("HashedMacAddress was not set!");
        return mUniqueMachineHash;
    }

    void TelemetryConfig::processEacoreiniSettings()
    {
        EA::IO::StringW eacoreIniName = GetEACoreIniFilePath();
#if defined(WIN32)
        if( eacoreIniName.empty() )
            return;
#endif
        // build telemetry breakpoint map from EACore.ini file
        EA::IO::IniFile eacoreIniFile(eacoreIniName.c_str());
        EA::IO::StringW value;

        int32_t isReadXML = eacoreIniFile.ReadEntry( EACORE_INI_TELEMETRY_SECTION, L"TelemetryXML", value );
        if ( isReadXML != -1)
        {
            sIsTelemetryXMLEnabled = (value.comparei(L"true") == 0);
        }

        value.clear();
        EA::IO::StringW environment = L"production";
        int32_t ret = eacoreIniFile.ReadEntry( L"Connection", L"EnvironmentName", value );
        if ( ret != -1 )
        {
            environment = value;
        }

        //Initialize the telemetry server setting base on the environment.  Then we'll check for overrides.
        if(environment == L"production")
        {
            setTelemetryServer(PROD_TELEMETRY_ENDPOINT);
        }
        else
        {
            setTelemetryServer(TEST_TELEMETRY_ENDPOINT);
        }

        // Read EACore.ini overrides if exist
        value.clear();
        ret = eacoreIniFile.ReadEntry( EACORE_INI_TELEMETRY_SECTION, L"TelemetryServer", value );
        {
            if ( ret != -1 && !value.empty() )
            {
                std::string svr(std::string(value.begin(), value.end()));
                if (svr.size() > 0)
                {
                    TelemetryConfig::setTelemetryServer(eastl::string8(svr.c_str()));
                    sIsTelemetryServerOverriden = true;
                }
            } 
        }

        value.clear();
        ret = eacoreIniFile.ReadEntry( EACORE_INI_TELEMETRY_SECTION, L"TelemetryHooksBlackList", value );
        {
            if ( ret != -1 && !value.empty() )
            {
                eastl::string8 hooks(EA::StdC::ConvertString<EA::IO::StringW, EA::IO::String8>(value).c_str());

                if (hooks.size() > 0)
                {
                    TelemetryConfig::setHooksBlacklist(eastl::string8(hooks.c_str()));
                    TelemetryConfig::sHooksBlacklistOverriden = true;
                }
            } 
        }

        value.clear();
        ret = eacoreIniFile.ReadEntry( EACORE_INI_TELEMETRY_SECTION, L"TelemetryCountryBlackList", value );
        {
            if ( ret != -1 && !value.empty() )
            {
                eastl::string8 countries(EA::StdC::ConvertString<EA::IO::StringW, EA::IO::String8>(value).c_str());

                if (countries.size() > 0)
                {
                    TelemetryConfig::setCountryBlacklist(eastl::string8(countries.c_str()));
                    TelemetryConfig::sCountriesBlacklistOverriden = true;
                }
            } 
        }

        value.clear();
        ret = eacoreIniFile.ReadEntry( EACORE_INI_TELEMETRY_SECTION, L"TelemetryLocale", value );
        if ( ret != -1 )
        {
            sLocaleOverride.assign(EA::StdC::ConvertString<EA::IO::StringW, EA::IO::String8>(value).c_str());
        }

        value.clear();
        // Get spam level if it exists in the EACore.ini
        ret = eacoreIniFile.ReadEntry( EACORE_INI_TELEMETRY_SECTION, L"TelemetrySpamLevel", value );
        if ( ret != -1 )
        {
            TelemetryConfig::setSpamLevel(EA::StdC::AtoI32(value.c_str()));
        }

        eastl::map<eastl::wstring, int> telemetryIdToIndexMap;  // map of all available telemetry events

        // enumerate all available telemetry events
        int count = 0;
        while( METRIC_LIST[count] != 0 )
        {
            eastl::string8 id = GetTelemetryModuleGroupStringId(METRIC_LIST[count]);
            eastl::wstring id16 = EA::StdC::Strlcpy<eastl::wstring, eastl::string8>(id);
            telemetryIdToIndexMap[id16] = count;
            count++;
        }

        // Check for each metric in the eacore.ini
        eastl::map<eastl::wstring, int>::iterator it = telemetryIdToIndexMap.begin();
        for( ; it != telemetryIdToIndexMap.end(); ++it )
        {
            value.clear();
            eastl::wstring id = (*it).first;
            int32_t ret = eacoreIniFile.ReadEntry(OriginTelemetry::EACORE_INI_TELEMETRY_BREAKPOINT_SECTION, (wchar_t*)(id.c_str()), value );
            if ( ret != -1 )
            {
                bool enabled = (value.compare(L"0") != 0); // non-zero
                if( enabled )
                {
                    sTelemetryBreakpointMap[(*it).second] = id;
                    sTelemetryBreakpointsEnabled = true;
                }
            }
        }
    }

    const char* const TelemetryConfig::getHooksFilter()
    {
        return sHooksFilter.c_str();
    }

    const char* const TelemetryConfig::GetCountryFilter()
    {
        return sCountryFilter.c_str();
    }

    void TelemetryConfig::setTelemetryServer(const eastl::string8& myServer)
    {
        if (myServer == sTelemeryServer || sIsTelemetryServerOverriden)
        {
            return;
        }

        // Testing for invalid URI
        auto pos = myServer.find("://");
        if (eastl::string8::npos != pos)
        {
            auto mySchema = myServer.substr(0, pos + 3);
            if (mySchema == "http://")
            {
                 sTelemetryPort = HTTP;
                 sTelemeryServer = myServer;
                 return;  
            }
            if(mySchema == "https://")
            {
                sTelemetryPort = HTTPS;
                sTelemeryServer = myServer;
                return;
            }
        }
        
        eastl::string8 errMsg;
        errMsg.append_sprintf("TelemetryConfig::setTelemetryServer() Parsing error. Server string %s is invalid", myServer.c_str());
        TelemetryLogger::GetTelemetryLogger().logTelemetryError(errMsg);
        EA_FAIL_MSG(errMsg.c_str());
    }

    void TelemetryConfig::setHooksBlacklist(const eastl::string8& blacklist)
    {
        if(!sHooksBlacklistOverriden)
        {
            sHooksFilter = blacklist;
        }
    }

    void TelemetryConfig::setCountryBlacklist(const eastl::string8& blacklist)
    {
        if(!sCountriesBlacklistOverriden)
        {
            sCountryFilter = blacklist;
        }
    }

    const char* const TelemetryConfig::telemetryServer()
    {
        return sTelemeryServer.c_str();
    }

    const char* const TelemetryConfig::telemetryDomain()
    {
        return TELEMETRY_DOMAIN.c_str();
    }

    const wchar_t* const TelemetryConfig::telemetryDataPath()
    {
        return TELEMETRY_LOGFILE_PATH.c_str();
    }

    const wchar_t* const TelemetryConfig::originLogPath()
    {
        return ORIGIN_LOGFILE_PATH.c_str();
    }


    const wchar_t* const TelemetryConfig::telemetryLogFileName()
    {
        return sTelemetryLogFilename.c_str();
    }


    const wchar_t* const TelemetryConfig::telemetryErrorFileName()
    {
        return sTelemetryErrorFilename.c_str();
    }

    const wchar_t* const TelemetryConfig::telemetryErrorDefaultFileName()
    {
        return TELMETRY_XML_ERROR_FILENAME;
    }

    uint32_t TelemetryConfig::spamLevel()
    {
        return sSpamLevel;
    }

    void TelemetryConfig::setSpamLevel( uint32_t val )
    {
#ifdef DEBUG
        // DirtySDK new max spam level is 5
        static const size_t MAX_SPAM = 5;
        sSpamLevel = val <= MAX_SPAM ? val : MAX_SPAM;
#endif    
    }

    bool TelemetryConfig::isTelemetryXMLEnabled()
    {
        return sIsTelemetryXMLEnabled;
    }

}