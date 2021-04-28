/*! ************************************************************************************************/
/*!
\file   matchmakingfiltersutil.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_FILTER_UTIL
#define BLAZE_MATCHMAKING_FILTER_UTIL

#include "gamemanager/tdf/matchmaker_config_server.h"
#include "gamemanager/tdf/matchmaking_properties_config_server.h"
#include "gamemanager/playerroster.h"
#include "gamepacker/property.h"

namespace Blaze
{
namespace Search
{
    class GameSessionSearchSlave;
}

namespace GameManager
{
    class PackerConfig;
    class PropertyDataSources;

    class PropertyManager
    {
    public:
        PropertyManager();
        ~PropertyManager();

        bool validateConfig(const PropertyConfig& config, ConfigureValidationErrors& validationErrors) const;
        bool onConfigure(const PropertyConfig& config);

        const PropertyNameList& getRequiredPackerGameProperties() const { return mPropertyConfig.getRequiredPackerInputProperties(); }
        const PropertyNameList& getGameBrowserPackerProperties() const { return mPropertyConfig.getGameBrowserPackerProperties(); }

        //typedef eastl::fixed_substring<char8_t> FixedSubString;
        bool parsePropertyName(const PropertyName& propertyNameFull, PropertyName& propertyName, PropertyName& propertyFieldName) const;
        bool validatePropertyList(const PropertyConfig& config, const PropertyNameList& propertiesUsed, const char8_t** outMissingProperty) const;

        // Convert the existing Format (which requires validateAndFetchProperty calls), into the the simple TdfMapping format (where each value is the value)
        // If a sanitized value already exists, that is used instead of refetching. 
        bool convertProperty(const PropertyDataSources& dataSources, const PropertyName& fullPropertyName, EA::TDF::GenericTdfType &outProp) const;
        bool convertProperties(const PropertyDataSources& dataSources, const PropertyNameList& propertiesUsed, PropertyNameMap& outProperties, const char8_t** outMissingProperty = nullptr) const;
        bool lookupTdfDataSourceProperty(const EA::TDF::Tdf* dataSource, const PropertyName& fullPropertyName, EA::TDF::GenericTdfType &outProp, bool isListValue) const;

        // PACKER_TODO:  Implement derived properties
        bool initDerivedPropertyExpressions();
        bool lookupDerivedProperty(const PropertyDataSources& dataSources, const PropertyName& fullPropertyName, EA::TDF::GenericTdfType& outProp) const;

        bool translateCgrAttributeToPropertyName(const char8_t* attributeString, eastl::string& propertyString) const;
        


// PACKR_TODO - Below are old functions/design:

        //filter names are defined in matchmaking_global_filters.cfg
        static const char8_t* PROPERTY_PLAYER_PARTICIPANT_IDS; // currently hardcoded property

        static constexpr const char8_t* FILTER_NAME_QF_HASH = "qfHashFilter";
        static constexpr const char8_t* FILTER_NAME_DSO = "dsoFilter";
        static constexpr const char8_t* FILTER_NAME_GPVS = "gpvsFilter";

        bool validateFilters(const PropertyConfig& config, const MatchmakingFilterMap& filters, ConfigureValidationErrors& validationErrors) const;

        //defaultIfEmpty sets the outValue to 0/false if the inValue is empty for convertChar2Int64/convertChar2Bool helper functions
        static bool getPortAndIpValue(uint16_t& port, uint32_t& ipAddress, const NetworkAddress& networkAddress);

        // Currently source properties only map to Player properties.
        static void configGamePropertiesPropertyNameList(const PackerConfig& packerConfig, PropertyNameList& propertyList);
        static bool configGamePropertyNameToFactorInputPropertyName(const char8_t* gamePropertyName, eastl::string& propertyDescriptorName);

    private:
        static bool propertyUedFormulaToGroupValueFormula(GroupValueFormula& valueFormula, const EA::TDF::EATDFEastlString& propertyString);

        eastl::hash_map<eastl::string, eastl::string> mCgRequestPropertyNameMapping;

        // Properties required for Packer to work.  Duplicates GameTemplate::sConfigVarNames.
        PropertyConfig mPropertyConfig;

        typedef bool(*DerivedPropertyFunction)(const PropertyDataSources& dataSources, EA::TDF::GenericTdfType& outProp);
        eastl::hash_map<eastl::string, DerivedPropertyFunction> mDerivedFunctionMap;
    };



    /*
     The PropertyDataSources class holds multiple locations of SourceData that can be used by the PropertyManager to convert into Properties.
     The expectation is that the PropertyDataSources are only held onto for a short time, and that the Properties are mostly passed around as named Generics.
     
     The clear() function resets these stored data sources.
    */

    class PropertyDataSources
    {
    public:
        PropertyNameMap mPropertyMap;                // Original "Source Data", to be replaced with something that holds UserSessionInfo*, MM Request, ReplicatedGameData*, etc.

        // Source Data:                              // (PACKER_TODO:  Expand this section to cover all the different types of source data, and remove the mPropertyMap entirely)
        UserSessionInfoPtr mCallerInfo;              // Holds 'caller.*' properties - The host of a matchmaking call.
        UserSessionInfoList mPlayersInfoList;        // Holds 'player.*' properties (UED, etc.)
        ReplicatedGameDataServerPtr mServerGameData; // Holds 'game.*' properties 
        Search::GameSessionSearchSlave* mGameSession = nullptr;         // Holds 'game.*' properties - simplifies some lookup functions, rather than duplicating logic
        const PlayerRoster* mPlayerRoster = nullptr; // Holds 'player.*' properties not managed by UserSessionInfo.
        PropertyNameMap mSanitizedProperties;        // Values that have passed through the input sanitizer.  Maps directly to an output value (int, list, etc.), with no further processing expected. (no '.avg')

        const GameCreationData* mGameCreationData = nullptr;        // Holds values that come from MM request (that are translate to Properties)
        StartMatchmakingInternalRequestPtr mMatchmakingRequest;     // Holds MM values that are outside the GameCreationData

        TeamProperties mTeamProperties;

        PropertyDataSources();
        void clear();

        void addPlayerRosterSourceData(const PlayerRoster* playerRoster);
        void addPlayersSourceData(const UserJoinInfoList& userJoinInfo, PlayerJoinData* pjd = nullptr);  // PACKER_TODO:  Hook the PJD up so that PlayerAttributes/Team/Role choice are accessible.

        // Convert the existing Format (which requires validateAndFetchProperty calls), into the the simple TdfMapping format (where each value is the value)
        // If a sanitized value already exists, that is used instead of refetching. 
        bool validateAndFetchProperty(const char8_t* propertyName, EA::TDF::TdfGenericReference& outRef, const EA::TDF::TypeDescriptionBitfieldMember** outBfMember = nullptr, bool addUnknownProperties = false) const;

    protected:
        EA::TDF::TdfGenericValue& insertAndReturn(const char8_t* propertyName);
    };


    // Brief: Helper used to get / set supported caller properties in a type - safe way
    class CallerProperties : public PropertyDataSources
    {
    public:
        CallerProperties(bool setDefaults = false);

        GameId& dedicatedServerOverrideGameId();

    };

    bool isGameProperty(const char8_t* gameProperty);
    bool isGamePropertyName(const char8_t* gameProperty, const char8_t* propertyName);

} // namespace GameManager
} // namespace Blaze
#endif //BLAZE_MATCHMAKING_FILTER_UTIL