/*! ************************************************************************************************/
/*!
\file propertyruledefinition.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_PROPERTYRULEDEFINITION_H
#define BLAZE_GAMEMANAGER_PROPERTYRULEDEFINITION_H

#include "gamemanager/matchmaker/rules/ruledefinition.h"
#include "gamemanager/matchmaker/rules/rule.h"
#include "gamemanager/matchmakingfiltersutil.h"
#include "gamemanager/tdf/matchmaker_server.h"
#include "EASTL/bitset.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class PropertyRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(PropertyRuleDefinition);
        DEFINE_RULE_DEF_H(PropertyRuleDefinition, PropertyRule);
    public:
        typedef eastl::bitset<64> FilterBits;  // Enough for uint64_t

        PropertyRuleDefinition();
        ~PropertyRuleDefinition() override;

        // Init Logic:
        bool initialize(const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig) override;
        bool initialize(const MatchmakingFilterMap& filters, const MatchmakingFilterList& globalFilters, const PropertyConfig& propertyConfig);
        void initRuleConfig(GetMatchmakingConfigResponse& getMatchmakingConfigResponse) const override;
   
        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;
        bool isDisabled() const override { return false; }

        // Active Game/Packer Silo Logic:
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void updateMatchmakingCache(MatchmakingGameInfoCache& matchmakingCache, const Search::GameSessionSearchSlave& gameSession, const MatchmakingSessionList* memberSessions) const override;

        // Packer Session Logic: 
        void addPackerSessionWMEs(GameManager::Rete::WMEId packerSessionId, const MatchmakingFilterCriteriaMap& filterCriteria, const GameManager::PropertyNameList& mmPropertyList, GameManager::Rete::WMEManager& wmeManager) const;
        bool insertPackerSessionWME(const EA::TDF::TdfPtr filterCriteria, GameManager::Rete::WMEManager& wmeManager, GameManager::Rete::WMEId wmeId, const eastl::string& wmeName) const;
        void convertProdNodeToSiloProperties(const MatchmakingFilterCriteriaMap& sessionFilters, Blaze::GameManager::Rete::ProductionNode* node, GameManager::PropertyNameMap& outSiloProperties) const;
        void convertPackerSessionFiltersToSiloFilters(const MatchmakingFilterCriteriaMap& sessionFilters, const GameManager::PropertyNameMap& siloProperties, MatchmakingFilterCriteriaMap& outRequestFilters) const;
        void updatePlatformList(const MatchmakingFilterCriteriaMap& sessionFilters, const GameManager::PropertyNameMap& siloProperties, ClientPlatformTypeList& outPlatformList) const;

        bool isReteCapable() const override { return true; }

        void bitNameHelper(uint32_t bit, const eastl::string& wmeIn, eastl::string& wmeOut) const { wmeOut.sprintf("%s_bit(%i)", wmeIn.c_str(), bit); }

        const MatchmakingFilterMap& getFilters() const { return mPropertyFiltersConfig; }

        const PropertyManager& getPropertyManager() const { return mPropertyManager; }

        const char8_t* getAllowedPlatformString(ClientPlatformType platform) const;
        const char8_t* getRequiredPlatformString(ClientPlatformType platform) const;

        typedef eastl::map<GameManager::Rete::Condition, EA::TDF::GenericTdfType> ConditionGenericMap;
        ConditionGenericMap& getConditionGenericMap() const { return mConditionGenericMap; };

    protected:
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElementsInternal(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave, bool upsert) const;

        bool updateActiveGameWMEs(EA::TDF::TdfId filterTdfId, Blaze::GameManager::Rete::WMEId wmeId, GameManager::Rete::WMEManager& wmeManager, const PropertyName& gamePropertyName, const EA::TDF::GenericTdfType& curValue, const EA::TDF::GenericTdfType* prevValue = nullptr) const;

    // Members
    private:
        friend class PropertyRule;

        PropertyNameList mGamePropertyNameList; // The set of indexed properties is different depending on context. On the search slave, the games need to index all properties that are referenced by either GB scenario specific filters or (MM global/scenario specific filters that reference the create game template of the game). On the packer only the MM filtering is necessary.

        // Copy of the config, since it's non-trivial to access the Component/Config from inside a Rule:
        MatchmakingFilterMap mPropertyFiltersConfig;
        PropertyManager mPropertyManager;

        // Values are added when the Conditions are added (fine) and are removed when the ProductionListener says so? 
        mutable ConditionGenericMap mConditionGenericMap;

        typedef eastl::vector_map<ClientPlatformType, eastl::string> PlatformWMEAttributeNameMap;
        PlatformWMEAttributeNameMap mAllowedStringMap;
        PlatformWMEAttributeNameMap mRequiredStringMap;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_PROPERTYRULEDEFINITION_H
