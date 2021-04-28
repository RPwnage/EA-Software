/*! ************************************************************************************************/
/*!
    \file   PlatformRuleDefinition.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/matchmaker/rules/platformruledefinition.h"
#include "gamemanager/matchmaker/rules/platformrule.h"
#include "gamemanager/matchmaker/matchmakingconfig.h" // for config key names (for logging)
#include "gamemanager/rete/wmemanager.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(PlatformRuleDefinition, "Predefined_PlatformRule", RULE_DEFINITION_TYPE_PRIORITY);

    const char8_t* PlatformRuleDefinition::PLATFORMS_ALLOWED_RETE_KEY = "PLAT_ALLOW_%s";
    const char8_t* PlatformRuleDefinition::PLATFORMS_OVERRIDE_RETE_KEY = "PLAT_OVER_%s";

    /*! ************************************************************************************************/
    /*!
        \brief Construct an uninitialized PlatformRuleDefinition.  NOTE: do not use until initialized.
    *************************************************************************************************/
    PlatformRuleDefinition::PlatformRuleDefinition()
    {
    }

    PlatformRuleDefinition::~PlatformRuleDefinition() 
    {
    }

    bool PlatformRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        // ignore return value, will be false because this rule has no fit thresholds
        RuleDefinition::initialize(name, salience, 0);

        // RETE names for the platforms:  (ex. PLATFORM_xone)
        for (auto& platformValue : GetClientPlatformTypeEnumMap().getNamesByValue())
        {
            const EA::TDF::TdfEnumInfo& info = (const EA::TDF::TdfEnumInfo&)(platformValue);
            mWMEAttributeNameForPlatformAllowed[(ClientPlatformType)info.mValue].append_sprintf(PLATFORMS_ALLOWED_RETE_KEY, info.mName);
            mWMEAttributeNameForPlatformOverride[(ClientPlatformType)info.mValue].append_sprintf(PLATFORMS_OVERRIDE_RETE_KEY, info.mName);
        }

        return  true;
    }

    void PlatformRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {
    }

    void PlatformRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        // Track all the platforms that can be accepted into the Game:
        const ClientPlatformTypeList& acceptedPlatformSet = gameSessionSlave.getCurrentlyAcceptedPlatformList();
        const ClientPlatformTypeList& basePlatformSet = gameSessionSlave.getBasePlatformList();

        // Track which platforms are enabled currently:
        for (auto& curPlatform : gController->getHostedPlatforms())
        {
            // Dynamic == currently allowed/acceptable:
            bool inAcceptedSet = acceptedPlatformSet.findAsSet(curPlatform) != acceptedPlatformSet.end();
            wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(),
                getPlatformAllowedAttrName(curPlatform), getWMEBooleanAttributeValue(inAcceptedSet), *this);

            bool inBaseSet = basePlatformSet.findAsSet(curPlatform) != basePlatformSet.end();
            wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(),
                getPlatformOverrideAttrName(curPlatform), getWMEBooleanAttributeValue(inBaseSet), *this);
        }

    }

    void PlatformRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        insertWorkingMemoryElements(wmeManager, gameSessionSlave);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
