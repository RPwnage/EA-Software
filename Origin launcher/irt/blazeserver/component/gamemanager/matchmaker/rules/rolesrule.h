/*! ************************************************************************************************/
/*!
    \file   rolesrule.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_ROLES_RULE_H
#define BLAZE_MATCHMAKING_ROLES_RULE_H

#include "gamemanager/matchmaker/rules/rolesruledefinition.h"
#include "gamemanager/matchmaker/rules/rule.h"
#include "gamemanager/rolescommoninfo.h"

namespace eastl
{
    template <>
    struct hash<Blaze::GameManager::PlayerRoleList>
    {
        size_t operator()(const Blaze::GameManager::PlayerRoleList& roles) const
        {
            size_t ret = 0;
            for (const auto& role : roles)
                ret ^= eastl::hash<const char8_t*>()(role);

            return ret;
        }
    };

    template<>
    struct equal_to<Blaze::GameManager::PlayerRoleList>
    {
        bool operator()(const Blaze::GameManager::PlayerRoleList& a, const Blaze::GameManager::PlayerRoleList& b) const
        {
            if (a.size() != b.size())
                return false;

            auto aIt = a.begin();
            auto bIt = b.begin();
            for (; aIt != a.end() && bIt != b.end(); ++aIt, ++bIt)
            {
                if (*aIt != *bIt)
                    return false;
            }

            return true;
        }
    };
}

namespace Blaze
{
namespace GameManager
{
    class GameSession;

namespace Matchmaker
{
    class Matchmaker;
    class RolesRuleDefinition;
    struct MatchmakingSupplementalData;

    /*! ************************************************************************************************/
    /*!
        \brief Blaze Matchmaking predefined rule.  The Roles Rule ensures that required roles are joinable for
        the desired team in a given rule. In FG-mode, does a pre-eval in RETE for the allowability & joinability of a role.
        A post-RETE step ensures that space required on the target team is actually available on matched games.
    *************************************************************************************************/
    class RolesRule : public Rule
    {
        NON_COPYABLE(RolesRule);
    public:
        RolesRule(const RolesRuleDefinition &definition, MatchmakingAsyncStatus* status);
        RolesRule(const RolesRule &other, MatchmakingAsyncStatus* status);
        ~RolesRule() override {}

        // returns true if any member of the matchmaking session has more than one entry in their roles list.
        bool hasMultiRoleMembers() const { return mHasMultiRoleMembers; }

        // Rule interface
        bool initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err) override;
        FitScore evaluateForMMFindGame(const Search::GameSessionSearchSlave& gameSession, ReadableRuleConditions& debugRuleConditions) const override;
        void evaluateForMMCreateGame(SessionsEvalInfo &sessionsEvalInfo, const MatchmakingSession &otherSession, SessionsMatchInfo &sessionsMatchInfo) const override;
        Rule* clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const override;

        bool isDisabled() const override { return mRuleDefinition.isDisabled(); }

        bool isReteCapable() const override { return mRuleDefinition.isReteCapable(); }
        bool isMatchAny() const override { return false; }

        UpdateThresholdResult updateCachedThresholds(uint32_t elapsedSeconds, bool forceUpdate) override;

        // ReteRule interface
        bool addConditions(Rete::ConditionBlockList& conditionBlockList) const override;

        FitScore evaluateGameAgainstAny(Search::GameSessionSearchSlave& gameSession, Rete::ConditionBlockId blockId, GameManager::Matchmaker::ReadableRuleConditions& debugRuleConditions) const override { return 0; }

        // No async status for roles rule
        void updateAsyncStatus() override {}

        const Blaze::GameManager::TeamIdRoleSizeMap& getTeamIdRoleSpaceRequirements() const { return mTeamIdRoleSpaceRequirements; }
        //const Blaze::GameManager::RoleSizeMap& getRoleSizeMap() const { return mMaxRoleSpaceRequirements; }

    private:
        void getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const override;
        uint64_t getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const override;
        void getDiagnosticDisplayString(eastl::string& desiredValuesTag, const RolesSizeMap& desiredRoles) const;

    private:

        TeamIdRoleSizeMap mTeamIdRoleSpaceRequirements;
        TeamIdPlayerRoleNameMap mTeamIdPlayerRoleRequirements;
        RolesSizeMap mDesiredRoleSpaceRequirements;      // Count of each unique set of desired roles
        uint16_t mTeamCount;
        bool mHasMultiRoleMembers; // true if any mm session member has more than one entry in their desired roles
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_ROLES_RULE_H
