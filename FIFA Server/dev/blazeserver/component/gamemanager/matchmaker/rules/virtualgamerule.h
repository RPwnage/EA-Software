/*! ************************************************************************************************/
/*!
    \file   virtualgamerule.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_VIRTUAL_GAME_RULE
#define BLAZE_MATCHMAKING_VIRTUAL_GAME_RULE

#include "gamemanager/matchmaker/rules/virtualgameruledefinition.h"
#include "gamemanager/matchmaker/minfitthresholdlist.h"
#include "gamemanager/matchmaker/rules/simplerule.h"

namespace Blaze
{
namespace GameManager
{
    class MatchmakingCriteriaError;
namespace Matchmaker
{

    class VirtualGameRule : public SimpleRule
    {
        NON_COPYABLE(VirtualGameRule);
    public:
        VirtualGameRule(const VirtualGameRuleDefinition& definition, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        VirtualGameRule(const VirtualGameRule& other, MatchmakingAsyncStatus* matchmakingAsyncStatus);
        ~VirtualGameRule() override {};

        //////////////////////////////////////////////////////////////////////////
        // from ReteRule grandparent
        //////////////////////////////////////////////////////////////////////////
        bool addConditions(GameManager::Rete::ConditionBlockList& conditionBlockList) const override;
        FitScore evaluateGameAgainstAny(Search::GameSessionSearchSlave& gameSession, GameManager::Rete::ConditionBlockId blockId, GameManager::Matchmaker::ReadableRuleConditions& debugRuleConditions) const override { return 0; }
        bool isMatchAny() const override { return (((getDesiredValue() & VirtualGameRulePrefs::ABSTAIN) != 0) || Rule::isMatchAny()); }

        bool isDedicatedServerEnabled() const override { return true; }

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

        /*! returns true if this rule is disabled.  This rule is disabled if ABSTAIN is passed in as the desired value*/
        bool isDisabled() const override;


        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        /*! ************************************************************************************************/
        /*! \brief update matchmaking status for the rule to current status
        ****************************************************************************************************/
        void updateAsyncStatus() override;


    protected:
        // Create game evaluations from SimpleRule
        void calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        // Find game evaluations from SimpleRule
        void calcFitPercent(const Search::GameSessionSearchSlave &gameSession, float &fitPercent, bool &isExactMatch, ReadableRuleConditions& debugRuleConditions) const override;

        VirtualGameRulePrefs::VirtualGameDesiredValue getDesiredValue() const { return mDesiredValue; }

    private:

        void calcFitPercentInternal(VirtualGameRulePrefs::VirtualGameDesiredValue otherValue, float &fitPercent, bool &isExactMatch) const;
        const VirtualGameRuleDefinition& getDefinition() const { return static_cast<const VirtualGameRuleDefinition&>(mRuleDefinition); }

    private:

        VirtualGameRulePrefs::VirtualGameDesiredValue mDesiredValue;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_VIRTUAL_GAME_RULE
