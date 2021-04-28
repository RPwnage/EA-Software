
#include "Ignition/MatchmakingManagement.h"

namespace Ignition
{
    MatchmakingManagement::MatchmakingManagement(const char8_t *id)
        : Pyro::UiNodeWindow(id)
    {

    }

    MatchmakingManagement::~MatchmakingManagement() {}


    void MatchmakingManagement::showDebugInfo(const Blaze::GameManager::DebugTopResult *result, const Blaze::GameManager::DebugFindGameResults& mmFindGameResults)
    {
        result->copyInto(mFindGameResult);

        Pyro::UiNodeValueContainer &valueGroupNode = getValuePanel("Match Details").getValues();
        valueGroupNode.clear();

        valueGroupNode["GameId"].setDescription("The id of this game");
        valueGroupNode["GameId"] = result->getGameId();

        valueGroupNode["GameName"].setDescription("The name of this game");
        valueGroupNode["GameName"] = result->getGameName();

        valueGroupNode["FitScore"].setDescription("The overall fit score evaluated for the game.");
        valueGroupNode["FitScore"] = result->getOverallFitScore();

        valueGroupNode["MaxFitScore"].setDescription("The maximum fit score possible for the game.");
        valueGroupNode["MaxFitScore"] = result->getMaxFitScore();

        valueGroupNode["OveralResult"].setDescription("The match result.");
        valueGroupNode["OveralResult"] = Blaze::GameManager::DebugEvaluationResultToString(result->getOverallResult());

        if (mmFindGameResults.getFoundGame().getGame().getGameId() != Blaze::GameManager::INVALID_GAME_ID)
        {
            valueGroupNode["FoundGame"].setDescription("Was this the matched game.");
            valueGroupNode["FoundGame"] = true;
        }

        // Debug info table
        Pyro::UiNodeTable *debugTable = getTables().findById("FGDebugInfo");

        if (debugTable == nullptr)
        {
            debugTable = new Pyro::UiNodeTable("FGDebugInfo");
            debugTable->setPosition(Pyro::ControlPosition::TOP_MIDDLE);

            debugTable->RowSelected += Pyro::UiNodeTable::RowSelectedEventHandler(this, &MatchmakingManagement::fgDebugTable_RowSelectedChanged);
            debugTable->RowUnselected += Pyro::UiNodeTable::RowUnselectedEventHandler(this, &MatchmakingManagement::fgDebugTable_RowSelectedChanged);

            debugTable->getColumn("rule").setText("Rule");
            debugTable->getColumn("result").setText("Result");
            debugTable->getColumn("fitScore").setText("FitScore");
            debugTable->getColumn("maxFitScore").setText("Max FitScore");

            getTables().add(debugTable);
        }
        
        Blaze::GameManager::DebugRuleResultMap::const_iterator iter = result->getRuleResults().begin();
        Blaze::GameManager::DebugRuleResultMap::const_iterator end = result->getRuleResults().end();
        for (; iter != end; ++iter)
        {
            const Blaze::GameManager::DebugRuleResult *ruleResult = iter->second;

            Pyro::UiNodeTableRow &row = debugTable->getRow_va("%s", iter->first.c_str());
            row.setIconImage(Pyro::IconImage::BLANK);

            Pyro::UiNodeTableRowFieldContainer &fields = row.getFields();
            fields["rule"] = iter->first.c_str();
            fields["result"] = Blaze::GameManager::DebugEvaluationResultToString(ruleResult->getResult());
            fields["fitScore"] = ruleResult->getFitScore();
            fields["maxFitScore"] = ruleResult->getMaxFitScore();
        }
    }

    void MatchmakingManagement::showDebugInfo(const Blaze::GameManager::DebugSessionResult *result, const Blaze::GameManager::DebugCreateGameResults& mmCreateGameResults, Blaze::GameManager::FitScore maxFitScore)
    {
        result->copyInto(mCreateGameResult);

        Pyro::UiNodeValueContainer &valueGroupNode = getValuePanel("Match Details").getValues();
        valueGroupNode.clear();

        valueGroupNode["MMSessionId"].setDescription("The id of the matching matchmaking session.");
        valueGroupNode["MMSessionId"] = result->getMatchedSessionId();

        valueGroupNode["PlayerName"].setDescription("The name of the player who made the matching matchmaking session.");
        valueGroupNode["PlayerName"] = result->getOwnerPlayerName();

        valueGroupNode["BlazeId"].setDescription("The blaze id of the player who made the matching matchmaking session.");
        valueGroupNode["BlazeId"] = result->getOwnerBlazeId();

        valueGroupNode["FitScore"].setDescription("The overall fit score evaluated for the game.");
        valueGroupNode["FitScore"] = result->getOverallFitScore();

        valueGroupNode["MaxFitScore"].setDescription("The maximum fit score possible for the game.");
        valueGroupNode["MaxFitScore"] = maxFitScore;

        if (mmCreateGameResults.getCreatorId() == result->getOwnerBlazeId())
        {
            valueGroupNode["GameCreator"].setDescription("Was the player issuing the create request, generally the host.");
            valueGroupNode["GameCreator"] = true;
        }

        Pyro::UiNodeTable *debugTable = getTables().findById("CGDebugInfo");

        if (debugTable == nullptr)
        {
            debugTable = new Pyro::UiNodeTable("CGDebugInfo");
            debugTable->setPosition(Pyro::ControlPosition::TOP_MIDDLE);

            debugTable->RowSelected += Pyro::UiNodeTable::RowSelectedEventHandler(this, &MatchmakingManagement::cgDebugTable_RowSelectedChanged);
            debugTable->RowUnselected += Pyro::UiNodeTable::RowUnselectedEventHandler(this, &MatchmakingManagement::cgDebugTable_RowSelectedChanged);

            debugTable->getColumn("rule").setText("Rule");
            debugTable->getColumn("result").setText("Result");
            debugTable->getColumn("fitScore").setText("FitScore");
            debugTable->getColumn("maxFitScore").setText("Max FitScore");

            getTables().add(debugTable);
        }

        Blaze::GameManager::DebugRuleResultMap::const_iterator iter = result->getRuleResults().begin();
        Blaze::GameManager::DebugRuleResultMap::const_iterator end = result->getRuleResults().end();
        for (; iter != end; ++iter)
        {
            const Blaze::GameManager::DebugRuleResult *ruleResult = iter->second;

            Pyro::UiNodeTableRow &row = debugTable->getRow_va("%s", iter->first.c_str());
            row.setIconImage(Pyro::IconImage::BLANK);

            Pyro::UiNodeTableRowFieldContainer &fields = row.getFields();
            fields["rule"] = iter->first.c_str();
            fields["result"] = Blaze::GameManager::DebugEvaluationResultToString(ruleResult->getResult());
            fields["fitScore"] = ruleResult->getFitScore();
            fields["maxFitScore"] = ruleResult->getMaxFitScore();
        }
    }


    void MatchmakingManagement::fgDebugTable_RowSelectedChanged(Pyro::UiNodeTable *sender, Pyro::UiNodeTableRow *row)
    {
        if (row != nullptr)
        {
            if (row->getIsSelected())
            {
                row->setIconImage(Pyro::IconImage::DETAILS);
                Blaze::GameManager::DebugRuleResultMap::const_iterator iter = mFindGameResult.getRuleResults().find(row->getField("rule").getAsString());
                if (iter != mFindGameResult.getRuleResults().end())
                {
                    updateConditionTable(iter->second->getConditions());
                }
            }
            else
            {
                row->setIconImage(Pyro::IconImage::BLANK);
            }
        }
    }

    void MatchmakingManagement::cgDebugTable_RowSelectedChanged(Pyro::UiNodeTable *sender, Pyro::UiNodeTableRow *row)
    {
        if (row != nullptr)
        {
            if (row->getIsSelected())
            {
                row->setIconImage(Pyro::IconImage::DETAILS);
                Blaze::GameManager::DebugRuleResultMap::const_iterator iter = mCreateGameResult.getRuleResults().find(row->getField("rule").getAsString());
                if (iter != mCreateGameResult.getRuleResults().end())
                {
                    updateConditionTable(iter->second->getConditions());
                }
            }
            else
            {
                row->setIconImage(Pyro::IconImage::BLANK);
            }
        }
    }

    void MatchmakingManagement::updateConditionTable(const Blaze::GameManager::ReadableRuleConditionList& conditions)
    {
        Pyro::UiNodeTable *conditionTable = getTables().findById("ConditionInfo");

        if (conditionTable == nullptr)
        {
            conditionTable = new Pyro::UiNodeTable("ConditionInfo");
            conditionTable->setPosition(Pyro::ControlPosition::TOP_RIGHT);

            conditionTable->getColumn("condition").setText("condition");

                getTables().add(conditionTable);
        }

        conditionTable->getRows().clear();

        Blaze::GameManager::ReadableRuleConditionList::const_iterator conditionIter = conditions.begin();
        Blaze::GameManager::ReadableRuleConditionList::const_iterator conditionEnd = conditions.end();
        uint32_t i = 1;
        for (; conditionIter != conditionEnd; ++conditionIter)
        {
            Pyro::UiNodeTableRow &row = conditionTable->getRow_va("%s", conditionIter->c_str());
            row.setIconImage(Pyro::IconImage::NONE);

            Pyro::UiNodeTableRowFieldContainer &fields = row.getFields();

            fields["condition"] = conditionIter->c_str();
            ++i;
        }

    }
}
