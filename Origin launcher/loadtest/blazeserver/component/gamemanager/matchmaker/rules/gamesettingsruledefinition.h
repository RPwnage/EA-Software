/*! ************************************************************************************************/
/*!
\file gamesettingsruledefinition.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAMESETTINGSRULEDEFINITION_H
#define BLAZE_GAMEMANAGER_GAMESETTINGSRULEDEFINITION_H

#include "gamemanager/matchmaker/rules/ruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class GameSettingsRuleDefinition : public RuleDefinition
    {
        NON_COPYABLE(GameSettingsRuleDefinition);
        DEFINE_RULE_DEF_H(GameSettingsRuleDefinition, GameSettingsRule);
    public:

        static const char8_t* GAMESETTINGSRULE_GAMEBROWSER_RETE_KEY;
        static const char8_t* GAMESETTINGSRULE_MATCHMAKING_RETE_KEY;

        /*! ************************************************************************************************/
        /*! \brief Game Settings from GameSettings which are used by MM for searching.
        *************************************************************************************************/
        enum GameSetting
        {
            GAME_SETTING_OPEN_TO_BROWSING,
            GAME_SETTING_OPEN_TO_MATCHMAKING,
            GAME_SETTING_SIZE
        };

    public:
        GameSettingsRuleDefinition();
        ~GameSettingsRuleDefinition() override;

        void insertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;
        void upsertWorkingMemoryElements(GameManager::Rete::WMEManager& wmeManager, const Search::GameSessionSearchSlave& gameSessionSlave) const override;

        //////////////////////////////////////////////////////////////////////////
        // from RuleDefinition
        //////////////////////////////////////////////////////////////////////////
        bool initialize( const char8_t* name, uint32_t salience, const MatchmakingServerConfig& matchmakingServerConfig ) override;
        void logConfigValues(eastl::string &dest, const char8_t* prefix = "") const override;
        bool isReteCapable() const override { return true; }

        bool isDisabled() const override { return false; }

        const char8_t* GameSettingNameToString(GameSetting gameSetting) const;

    // Members
    private:
        static GameSetting const mGameSettingList[GAME_SETTING_SIZE];
    private:
        bool getSettingValueForGame(GameSetting gameSetting, const Search::GameSessionSearchSlave& gameSessionSlave) const;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_GAMESETTINGSRULEDEFINITION_H
