/*! ************************************************************************************************/
/*!
\file gamesettingsruledefinition.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/rules/gamesettingsruledefinition.h"
#include "gamemanager/matchmaker/rules/gamesettingsrule.h"

#include "gamemanager/rete/wmemanager.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/tdf/gamemanager_server.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    DEFINE_RULE_DEF_CPP(GameSettingsRuleDefinition, "Predefined_GameSettingsRule", RULE_DEFINITION_TYPE_SINGLE);
    const char8_t* GameSettingsRuleDefinition::GAMESETTINGSRULE_GAMEBROWSER_RETE_KEY = "OpenToBrowsing";
    const char8_t* GameSettingsRuleDefinition::GAMESETTINGSRULE_MATCHMAKING_RETE_KEY = "OpenToMatchmaking";


    //const uint32_t GameSettingsRuleDefinition::GAME_SETTING_LIST_SIZE = GameSettingsRuleDefinition::GAME_SETTING_SIZE;
    GameSettingsRuleDefinition::GameSetting const GameSettingsRuleDefinition::mGameSettingList[GameSettingsRuleDefinition::GAME_SETTING_SIZE] =
    {
        GAME_SETTING_OPEN_TO_BROWSING,
        GAME_SETTING_OPEN_TO_MATCHMAKING
    };

    /*! ************************************************************************************************/
    /*! \brief ctor
    ***************************************************************************************************/
    GameSettingsRuleDefinition::GameSettingsRuleDefinition()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    GameSettingsRuleDefinition::~GameSettingsRuleDefinition()
    {
    }

    void GameSettingsRuleDefinition::insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        for (uint32_t i = 0; i < GAME_SETTING_SIZE; ++i)
        {
            GameSetting gameSetting = mGameSettingList[i];
            const char8_t* gameSettingStr = GameSettingNameToString(gameSetting);
            bool settingValue = getSettingValueForGame(gameSetting, gameSessionSlave);

            TRACE_LOG("[GameSettingsRuleDefinition].insertWorkingMemoryElements game " << gameSessionSlave.getGameId() 
                      << " in setting " << gameSettingStr << "(" << (settingValue ? "true" : "false") << ")");

            wmeManager.insertWorkingMemoryElement(gameSessionSlave.getGameId(), GameSettingNameToString(gameSetting), getWMEBooleanAttributeValue(settingValue), *this);
        }
    }

    void GameSettingsRuleDefinition::upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        for (uint32_t i = 0; i < GAME_SETTING_SIZE; ++i)
        {
            GameSetting gameSetting = mGameSettingList[i];
            const char8_t* gameSettingStr = GameSettingNameToString(gameSetting);
            bool settingValue = getSettingValueForGame(gameSetting, gameSessionSlave);

            TRACE_LOG("[GameSettingsRuleDefinition].upsertWorkingMemoryElements game " << gameSessionSlave.getGameId() 
                      << " in setting " << gameSettingStr << "(" << (settingValue ? "true" : "false") << ")");

            wmeManager.upsertWorkingMemoryElement(gameSessionSlave.getGameId(), GameSettingNameToString(gameSetting), getWMEBooleanAttributeValue(settingValue), *this);
        }
    }

    bool GameSettingsRuleDefinition::initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig )
    {
        // parses common configuration.  0 weight, this rule does not effect the final result.
        RuleDefinition::initialize(name, salience, 0);

        return true;
    }

    void GameSettingsRuleDefinition::logConfigValues(eastl::string &dest, const char8_t* prefix) const
    {

    }

    const char8_t* GameSettingsRuleDefinition::GameSettingNameToString(GameSetting gameSetting) const
    {
        switch (gameSetting)
        {
        case GAME_SETTING_OPEN_TO_BROWSING:
            return GAMESETTINGSRULE_GAMEBROWSER_RETE_KEY;
        case GAME_SETTING_OPEN_TO_MATCHMAKING:
            return GAMESETTINGSRULE_MATCHMAKING_RETE_KEY;
        default:
            return "";
        }
    }

    bool GameSettingsRuleDefinition::getSettingValueForGame(GameSetting gameSetting, const Search::GameSessionSearchSlave& gameSessionSlave) const
    {
        const bool isLockedForPreferredJoins = gameSessionSlave.isLockedForPreferredJoins();
        if (isLockedForPreferredJoins)
            TRACE_LOG("[GameSettingsRuleDefinition] Overriding any open to public joins settings on game, to use value false, due to game(" << gameSessionSlave.getGameId() << ") being locked for preferred joins.");

        switch (gameSetting)
        {
        case GAME_SETTING_OPEN_TO_BROWSING:
            return (!isLockedForPreferredJoins && gameSessionSlave.getGameSettings().getOpenToBrowsing());
        case GAME_SETTING_OPEN_TO_MATCHMAKING:
            return (!isLockedForPreferredJoins && gameSessionSlave.getGameSettings().getOpenToMatchmaking());
        default:
            return false;
        }
    }


} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
