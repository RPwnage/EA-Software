#include "framework/blaze.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "modrule.h"
#include "modruledefinition.h"
#include <EAStdC/EAScanf.h>

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    ModRule::ModRule(const ModRuleDefinition& ruleDefinition, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(ruleDefinition, matchmakingAsyncStatus)
        , mDesiredClientMods(0)
        , mIsDisabled(true)
    {
    }

    ModRule::ModRule(const ModRule& otherRule, MatchmakingAsyncStatus* matchmakingAsyncStatus)
        : SimpleRule(otherRule, matchmakingAsyncStatus)
        , mDesiredClientMods(otherRule.mDesiredClientMods)
        , mIsDisabled(otherRule.mIsDisabled)
    {
    }

    ModRule::~ModRule() {}

    bool ModRule::initialize(const MatchmakingCriteriaData& criteriaData, const MatchmakingSupplementalData& matchmakingSupplementalData, MatchmakingCriteriaError& err)
    {
        if (matchmakingSupplementalData.mMatchmakingContext.canOnlySearchResettableGames())
        {
            // Rule not needed when doing resettable games only
            return true;
        }

        const ModRuleCriteria& modCriteria = criteriaData.getModRuleCriteria();
        mDesiredClientMods = modCriteria.getDesiredModRegister();
        mIsDisabled = !modCriteria.getIsEnabled();

        return true;
    }

    void ModRule::calcFitPercent(const Search::GameSessionSearchSlave& gameSession, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        uint32_t serverMods = gameSession.getGameModRegister();
        

        if ((serverMods & mDesiredClientMods) == serverMods)
        {
            fitPercent = 0.0; // no effect on fit score
            isExactMatch = true;

            TRACE_LOG("[ModRule] Mod match (server modmask: " << serverMods << ", client modmask: "
                << mDesiredClientMods << ")" );

            debugRuleConditions.writeRuleCondition("Game mod %u matches client mod %u", serverMods, mDesiredClientMods);
        }
        else
        {
            debugRuleConditions.writeRuleCondition("Game mod %u no match client mod %u", serverMods, mDesiredClientMods);
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;
        }
        
    }


    void ModRule::calcFitPercent(const Rule& otherRule, float& fitPercent, bool& isExactMatch, ReadableRuleConditions& debugRuleConditions) const
    {
        

        if ((mDesiredClientMods & static_cast<const ModRule&>(otherRule).mDesiredClientMods) == mDesiredClientMods)
        {
            fitPercent = 0.0; // no effect on fit score
            isExactMatch = true;
            TRACE_LOG("[ModRule] Mod match (this modmask " << mDesiredClientMods
                << ", other modmask: " <<  static_cast<const ModRule&>(otherRule).mDesiredClientMods << ")");

            debugRuleConditions.writeRuleCondition("Other mod %u matches client mod %u", static_cast<const ModRule&>(otherRule).mDesiredClientMods, mDesiredClientMods);
        }
        else
        {
            fitPercent = FIT_PERCENT_NO_MATCH;
            isExactMatch = false;

            debugRuleConditions.writeRuleCondition("Other mod %u no match client mod %u", static_cast<const ModRule&>(otherRule).mDesiredClientMods, mDesiredClientMods);
        }

        
    }


    void ModRule::updateAsyncStatus()
    {
    }

    Rule* ModRule::clone(MatchmakingAsyncStatus* matchmakingAsyncStatus) const
    {
        return BLAZE_NEW ModRule(*this, matchmakingAsyncStatus);
    }

    bool ModRule::addConditions(Rete::ConditionBlockList& conditionBlockList) const
    {
        const ModRuleDefinition &ruleDef = getDefinition();

        Rete::ConditionBlock& baseConditions = conditionBlockList.at(Rete::CONDITION_BLOCK_BASE_SEARCH);

        const int MOD_COUNT = sizeof(GameModRegister) * 8;

        const FitScore EXACT_MATCH_FIT_SCORE = calcWeightedFitScore(true /*isExactMatch*/, 1.0f /*fitPercent*/);
        const Rete::WMEAttribute WME_ATTR_BOOLEAN_FALSE = ruleDef.getWMEBooleanAttributeValue(false);

        for (int bit = 0; bit < MOD_COUNT; bit++)
        {
            if (((mDesiredClientMods >> bit) & 0x1) == 0)
            {
                Rete::OrConditionList& baseOrConditions = baseConditions.push_back();

                baseOrConditions.push_back(Rete::ConditionInfo(
                    Rete::Condition(ruleDef.getWMEAttributeNameForBit(bit).c_str(), WME_ATTR_BOOLEAN_FALSE, mRuleDefinition),
                    EXACT_MATCH_FIT_SCORE, this
                    ));
            }
        }

       return true;
    }

    /*! ************************************************************************************************/
    /*! \brief Implemented to enable per desired value break down metrics
    ****************************************************************************************************/
    void ModRule::getRuleValueDiagnosticSetupInfos(RuleDiagnosticSetupInfoList& setupInfos) const
    {
        setupInfos.push_back(RuleDiagnosticSetupInfo("desiredMods",
            eastl::string(eastl::string::CtorSprintf(), "%u", mDesiredClientMods).c_str()));
    }

    /*! ************************************************************************************************/
    /*! \brief ModRule only preliminary filters with RETE and relies on post rete, and can't simply calculate diagnostics from
            conditions. Override to provide searching by using a special game counts cache
    ***************************************************************************************************/
    uint64_t ModRule::getDiagnosticGamesVisible(const RuleDiagnosticSetupInfo& diagnosticInfo, const Rete::ConditionBlockList& sessionConditions, const DiagnosticsSearchSlaveHelper& helper) const
    {
        if (isMatchAny())
        {
            return helper.getOpenToMatchmakingCount();
        }
        return helper.getModsGameCount(mDesiredClientMods);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

