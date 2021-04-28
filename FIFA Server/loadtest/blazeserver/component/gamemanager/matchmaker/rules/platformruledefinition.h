/*! ************************************************************************************************/   
/*!
    \file   Platformruledefinition.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_PLATFORM_RULE_DEFINITION
#define BLAZE_MATCHMAKING_PLATFORM_RULE_DEFINITION

#include "gamemanager/matchmaker/rules/ruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    /*! ************************************************************************************************/
    /*!
        \brief The rule definition for finding a game that matches the desired platform.
        
    ***************************************************************************************************/
    class PlatformRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(PlatformRuleDefinition);
        DEFINE_RULE_DEF_H(PlatformRuleDefinition, PlatformRule);
    public:
        static const char8_t* PLATFORMS_ALLOWED_RETE_KEY;
        static const char8_t* PLATFORMS_OVERRIDE_RETE_KEY;

        PlatformRuleDefinition();

        //! \brief destructor
        ~PlatformRuleDefinition() override;

        bool isDisabled() const override { return false; }

        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;
        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;

        //
        // From GameReteRuleDefinition
        //

        bool isReteCapable() const override { return true; }
        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

        const char8_t* getPlatformAllowedAttrName(ClientPlatformType platform) const 
        { 
            auto iter = mWMEAttributeNameForPlatformAllowed.find(platform);
            return (iter != mWMEAttributeNameForPlatformAllowed.end()) ? iter->second.c_str() : "";
        }
        const char8_t* getPlatformOverrideAttrName(ClientPlatformType platform) const
        {
            auto iter = mWMEAttributeNameForPlatformOverride.find(platform);
            return (iter != mWMEAttributeNameForPlatformOverride.end()) ? iter->second.c_str() : "";
        }

        // End from GameReteRuleDefintion
    private:
        eastl::map<ClientPlatformType, eastl::string> mWMEAttributeNameForPlatformAllowed;
        eastl::map<ClientPlatformType, eastl::string> mWMEAttributeNameForPlatformOverride;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_PLATFORM_RULE_DEFINITION
