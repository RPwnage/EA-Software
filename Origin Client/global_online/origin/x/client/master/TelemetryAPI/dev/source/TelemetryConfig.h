///////////////////////////////////////////////////////////////////////////////
// TelemetryConfig.h
//
// Keeps the TelemetryAPI config values
//// Copyright (c) 2010-2013 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef TelemetryConfig_h__
#define TelemetryConfig_h__

#include "EASTL/string.h"
#include "EASTL/map.h"

namespace OriginTelemetry
{
    //TODO This and everything todo with the telemetry file obfuscation should be moved to it's own class.
    const static char8_t ESCAPE_CHAR = 'D';
    const static char8_t END_OF_STRING = '\0';
    const static char8_t END_OF_STRING_ESC = 'N';
    const static char8_t END_OF_FILE = EOF;
    const static char8_t END_OF_FILE_ESC = 'E';
    const static char8_t END_OF_LINE = '\n';
    const static char8_t END_OF_LINE_ESC = 'L';
    const eastl::string8 RANDOM_GENERATOR_RESET_CHARACTER = "R";
    const static char8_t RESET_GENERATOR_ESC = 'R';
    const static int32_t INITIAL_SEED = 0x3C4D;

    // wrapper class that can't be constructed and only contains statics.
    class TelemetryConfig
    {

    public:
        /// \brief Telemetry ports
        enum TelemetryPorts
        {
            HTTP = 80
            , HTTPS = 443
        };

        /// \brief DTOR
        ~TelemetryConfig();

        /// Process the telemetry related settings from teh eacore.ini file.
        static void processEacoreiniSettings();

        /// returns spam level, either the default one or the read from EACore.ini
        static uint32_t spamLevel();
        /// sets telemetryAPI spam level (ProtoHTTP) from EACore.ini
        /// \param level of spam, max = 3
        static void setSpamLevel(uint32_t val);

        /// \brief returns hashed mac string
        static eastl::string8 uniqueMachineHash();

        /// \brief sets hashed mac string
        static void setUniqueMachineHash(uint64_t val);

        /// \brief sets telemetry server. Should be called when we get the value from the dynamic config.
        static void setTelemetryServer(const eastl::string8& myServer);

          /// \brief set the telemetry hooks blacklist
        /// \param string that contains the blacklisted hooks
        static void setHooksBlacklist(const eastl::string8& blacklist);

        /// \brief getter function for the filter string for blacklisted telemetry hooks
        /// \return the list of blacklisted telemetry hooks
        static const char* const getHooksFilter();

        /// \brief set the telemetry countries blacklist
        /// \param string that contains the blacklisted countries
        static void setCountryBlacklist(const eastl::string8& blacklist);

        /// \brief getter function for the filter string for blacklisted countries
        /// \return the list of blacklisted countries
        static const char* const GetCountryFilter();

        /// \brief returns telemetry port
        static int telemeteryPort()
        {
            return sTelemetryPort;
        }
        /// \brief returns telemetry end point
        static const char* const telemetryServer();
        /// \brief returns telemetry domain. Platform-based
        static const char* const telemetryDomain();
        /// \brief returns telemetry data path. Platform-based
        static const wchar_t* const telemetryDataPath(); 
        /// \brief returns origin log directory path. Platform-based
        static const wchar_t* const originLogPath();
        /// Return the name of the telemetry log file without the directory path
        static const wchar_t* const telemetryLogFileName();
        /// Return the name of the telemetry error file without the directory path
        static const wchar_t* const telemetryErrorFileName();
        static const wchar_t* const telemetryErrorDefaultFileName();

        /// Returns weather or not the TelemetryXML setting was enabled in eacore.ini or not.
        static bool isTelemetryXMLEnabled();

        ///Get the telemetry breakpoints that were in the eacore.ini file.
        static eastl::map<int32_t, eastl::wstring>& telemetryBreakpointMap(){return sTelemetryBreakpointMap;} 
        ///Get if the telemetry breakpoints are enabled.
        static bool isTelemetryBreakpointsEnabled(){return sTelemetryBreakpointsEnabled;}
        ///Disable or enable telemetry breakpoints
        static void disableTelemetryBreakpoints(){sTelemetryBreakpointsEnabled = false;}

        ///Return the localeOverride
        static eastl::string8& localeOverride(){ return sLocaleOverride;}
        
        static void setIsBOI( bool isBOI ){ sIsBOI = isBOI; }
        static bool isBOI(){ return sIsBOI;}

        static void preemptErrorFileName(const eastl::string8 &pfx);

    private:
        /// disabling class construction.  This is really more of a namespace for a bunch of statics.
        /// \brief Default CTOR
        TelemetryConfig();
        TelemetryConfig(const TelemetryConfig&);
        TelemetryConfig& operator=(const TelemetryConfig&);
        TelemetryConfig& operator&(const TelemetryConfig&);

        /// Connection values.
        /// These values will not change often, it is safe to make them static.
        /// Also they will be accessed during Origin single thread mode,
        /// i.e. during the construction of the OriginApplication instance.
        static eastl::string8 sTelemeryServer;
        static eastl::string8 sHooksFilter;
        static eastl::string8 sCountryFilter;
        static int sTelemetryPort;
        static bool sIsTelemetryServerOverriden;
        static bool sCountriesBlacklistOverriden;
        static bool sHooksBlacklistOverriden;
        static eastl::string8 mUniqueMachineHash;
        static uint32_t sSpamLevel;
        static bool sIsTelemetryXMLEnabled;
        // Telemetry Breakpoints
        static eastl::map<int32_t, eastl::wstring> sTelemetryBreakpointMap; // all the breakpoints read from EACore.ini
        static bool sTelemetryBreakpointsEnabled;
        static eastl::string8 sLocaleOverride;
        static bool sIsBOI;
        static eastl::wstring sTelemetryErrorFilename;
        static eastl::wstring sTelemetryLogFilename;
    };
}

#endif // TelemetryConfig_h__
