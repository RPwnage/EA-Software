/*! ************************************************************************************************/
/*!
\file gametyperule.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAMETYPERULE_H
#define BLAZE_GAMEMANAGER_GAMETYPERULE_H

#include "gamemanager/matchmaker/rules/gametyperuledefinition.h"
#include "gamemanager/matchmaker/rules/simplerule.h"
#include "gamemanager/tdf/gamemanagerconfig_server.h"

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class GameTypeRule : public SimpleRule
    {
        NON_COPYABLE(GameTypeRule);
    public:
        GameTypeRule(const GameTypeRuleDefinition& definition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        GameTypeRule(const GameTypeRule& other, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ~GameTypeRule() override {};

        bool addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const override;

        bool isMatchAny() const override { return false; }
        bool isDedicatedServerEnabled() const override { return true; }
        FitScore getMaxFitScore() const override { return 0; }

        //////////////////////////////////////////////////////////////////////////
        // from Rule
        //////////////////////////////////////////////////////////////////////////
        /*! ************************************************************************************************/
        /*!
            \brief initialize a GameTypeRule instance for use in a matchmaking session. Return true on success.
        
            \param[in]  criteriaData - Criteria data used to initialize the rule.
            \param[in]  matchmakingSupplementalData - used to lookup the rule definition
            \param[out] err - errMessage is filled out if initialization fails.
            \return true on success, false on error (see the errMessage field of err)
        ***************************************************************************************************/
        bool initialize(const MatchmakingCriteriaData &criteriaData, const MatchmakingSupplementalData &matchmakingSupplementalData, MatchmakingCriteriaError &err) override;

        /*! returns true - rule cannot be disabled */
        bool isDisabled() const override { return mGameTypeList.empty(); }


        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        void updateAsyncStatus() override {}

    protected:
        const GameTypeRuleDefinition& getDefinition() const { return static_cast<const GameTypeRuleDefinition&>(mRuleDefinition); }
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;
    private:
        GameTypeList mGameTypeList;
        bool mAcceptsGameGroup;
        bool mAcceptsGameSession;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_GAMETYPERULE_H
