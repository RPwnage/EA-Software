/*! ************************************************************************************************/
/*!
\file gamesettingsrule.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAMESETTINGSRULE_H
#define BLAZE_GAMEMANAGER_GAMESETTINGSRULE_H

#include "gamemanager/matchmaker/rules/simplerule.h"
#include "gamemanager/matchmaker/rules/gamesettingsruledefinition.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class GameSettingsRule : public SimpleRule
    {
        NON_COPYABLE(GameSettingsRule);
    public:
        GameSettingsRule(const GameSettingsRuleDefinition& definition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        GameSettingsRule(const GameSettingsRule& other, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ~GameSettingsRule() override {};

        bool addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const override;

        bool isMatchAny() const override { return false; }
        bool isDedicatedServerEnabled() const override { return true; }
        FitScore getMaxFitScore() const override { return 0; }

        //////////////////////////////////////////////////////////////////////////
        // from Rule
        //////////////////////////////////////////////////////////////////////////
        /*! ************************************************************************************************/
        /*!
            \brief initialize a virtualGameRule instance for use in a matchmaking session. Return true on success.
        
            \param[in]  criteriaData - Criteria data used to initialize the rule.
            \param[in]  matchmakingSupplementalData - used to lookup the rule definition
            \param[out] err - errMessage is filled out if initialization fails.
            \return true on success, false on error (see the errMessage field of err)
        ***************************************************************************************************/
        bool initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err) override;

        /*! returns true if this rule is disabled. */
        bool isDisabled() const override { return (mGameSetting == GameSettingsRuleDefinition::GAME_SETTING_SIZE); }

        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        void updateAsyncStatus() override {}

        GameSettingsRuleDefinition::GameSetting getGameSetting() const { return mGameSetting; }

    protected:
        const GameSettingsRuleDefinition& getDefinition() const { return static_cast<const GameSettingsRuleDefinition&>(mRuleDefinition); }

    // Members
    private:
        GameSettingsRuleDefinition::GameSetting mGameSetting;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_GAMESETTINGSRULE_H
