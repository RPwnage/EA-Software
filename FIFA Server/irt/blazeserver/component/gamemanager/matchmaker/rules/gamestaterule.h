/*! ************************************************************************************************/
/*!
\file gamestaterule.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAMESTATERULE_H
#define BLAZE_GAMEMANAGER_GAMESTATERULE_H

#include "gamemanager/matchmaker/rules/gamestateruledefinition.h"
#include "gamemanager/matchmaker/rules/simplerule.h"
#include "gamemanager/tdf/gamemanagerconfig_server.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class GameStateRule : public SimpleRule
    {
        NON_COPYABLE(GameStateRule);
    public:
        GameStateRule(const GameStateRuleDefinition& definition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        GameStateRule(const GameStateRule& other, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ~GameStateRule() override {};

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

        /*! returns true - rule cannot be disabled */
        bool isDisabled() const override { return mGameStateWhitelist.empty(); }


        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        void updateAsyncStatus() override {}

    protected:
        const GameStateRuleDefinition& getDefinition() const { return static_cast<const GameStateRuleDefinition&>(mRuleDefinition); }

    // Members
    private:
        GameStateList mGameStateWhitelist;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_GAMESTATERULE_H
