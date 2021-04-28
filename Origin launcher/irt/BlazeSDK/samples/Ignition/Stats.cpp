
#include "Ignition/Stats.h"

namespace Ignition
{

Stats::Stats()
    : BlazeHubUiBuilder("Stats"),
      mSelectedStatsGroup(nullptr),
      mCurrentStatsView(nullptr)
{
    getActions().add(&getActionRequestStatsGroupList());
}

Stats::~Stats()
{
}

void Stats::onAuthenticatedPrimaryUser()
{
    runRequestKeyScopes();

    setIsVisibleForAll(true);
}

void Stats::onDeAuthenticatedPrimaryUser()
{
    setIsVisibleForAll(false);

    mCurrentStatsView = nullptr;

    getWindows().clear();
}

void Stats::refreshStatsGroup()
{
    if (mSelectedStatsGroup != nullptr)
    {
        Pyro::UiNodeTable &tableNode = getWindow("Stats Groups").getTable("Stats Group Columns and Data");
        tableNode.setPosition(Pyro::ControlPosition::BOTTOM_MIDDLE);

        tableNode.getActions().insert(&getActionCreateStatsView());

        tableNode.getColumns().clear();
        tableNode.getRows().clear();

        int32_t columnIndex = 0;
        tableNode.getColumn_va("col%d", columnIndex++).setText("Name");

        for (Blaze::Stats::StatGroupResponse::StatDescSummaryList::const_iterator i = mSelectedStatsGroup->getStatKeyColumnsList()->begin();
            i != mSelectedStatsGroup->getStatKeyColumnsList()->end();
            i++)
        {
            tableNode.getColumn_va("col%d", columnIndex++).setText((*i)->getLongDesc());
        }

        if (mCurrentStatsView != nullptr)
        {
            const Blaze::Stats::StatValues::EntityStatsList *entityStatsList;

            entityStatsList = nullptr;

            // check if group has no scope first...
            if (mCurrentStatsView->getStatValues() != nullptr)
            {
                entityStatsList = &mCurrentStatsView->getStatValues()->getEntityStatsList();
            }
            // otherwise...
            else if (mCurrentStatsView->getKeyScopeStatsValueMap()->empty() == false)
            {
                Blaze::Stats::KeyScopeStatsValueMap::const_iterator it;
                /// @todo more dynamic lookup
                it = mCurrentStatsView->getKeyScopeStatsValueMap()->find("ISmap=0;ISmode=0");
                if (it != mCurrentStatsView->getKeyScopeStatsValueMap()->end())
                {
                    entityStatsList = &it->second->getEntityStatsList();
                }
            }

            if (entityStatsList != nullptr)
            {
                for (Blaze::Stats::StatValues::EntityStatsList::const_iterator entityItr = entityStatsList->begin();
                    entityItr != entityStatsList->end();
                    entityItr++)
                {
                    const Blaze::UserManager::User *user;
                    columnIndex = 0;

                    Pyro::UiNodeTableRow &row = tableNode.getRow_va("%d", (*entityItr)->getEntityId());

                    user = gBlazeHub->getUserManager()->getUserById((*entityItr)->getEntityId());
                    if (user != nullptr)
                    {
                        row.getField_va("col%d", columnIndex++) = user->getName();
                    }
                    else
                    {
                        row.getField_va("col%d", columnIndex++).setAsString_va("EntityId=%d", (*entityItr)->getEntityId());
                    }

                    for (Blaze::Stats::EntityStats::StringStatValueList::const_iterator valueItr = (*entityItr)->getStatValues().begin();
                        valueItr != (*entityItr)->getStatValues().end();
                        valueItr++)
                    {
                        row.getField_va("col%d", columnIndex++) = (*valueItr).c_str();
                    }
                }
            }
            else
            {
                tableNode.getRow("noStatsFound").getField("col0") = "No stats found";
            }
        }
        else
        {
            tableNode.getRow("createStatsViewToSeeStats").getField("col0") = "Create Stats View to see stats";
        }
    }
}

void Stats::runRequestKeyScopes()
{
    Blaze::Stats::GetKeyScopeCb cb(this, &Stats::GetKeyScopeCb);
    gBlazeHub->getStatsAPI()->requestKeyScopes(cb);
}

void Stats::GetKeyScopeCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, const Blaze::Stats::KeyScopes *keyScropes)
{
    //REPORT_FUNCTOR_CALLBACK(nullptr);

    //REPORT_BLAZE_ERROR(blazeError);
}

void Stats::initActionRequestStatsGroupList(Pyro::UiNodeAction &action)
{
    action.setText("Request Stats Group List");
    action.setDescription("Retrieves a list of all configured stats groups");
}

void Stats::actionRequestStatsGroupList(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Stats::GetStatsGroupListCb cb(this, &Stats::GetStatsGroupListCb);
    vLOG(gBlazeHub->getStatsAPI()->requestStatsGroupList(cb));
}

void Stats::GetStatsGroupListCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, const Blaze::Stats::StatGroupList *statGroupList)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);

    Pyro::UiNodeTable &tableNode = getWindow("Stats Groups").getTable("Stats Group List");

    tableNode.getActions().insert(&getActionGetStatsGroup());

    tableNode.getColumn("name").setText("Name");

    for (Blaze::Stats::StatGroupList::StatGroupSummaryList::const_iterator i = statGroupList->getGroups().begin();
        i != statGroupList->getGroups().end();
        i++)
    {
        Pyro::UiNodeTableRowFieldContainer &fields = tableNode.getRow((*i)->getName()).getFields();
        fields["name"] = (*i)->getName();
    }

    REPORT_BLAZE_ERROR(blazeError);
}

void Stats::initActionGetStatsGroup(Pyro::UiNodeAction &action)
{
    action.setText("Get Stats Group");
}

void Stats::actionGetStatsGroup(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Stats::GetStatsGroupCb cb(this, &Stats::GetStatsGroupCb);
    vLOG(gBlazeHub->getStatsAPI()->requestStatsGroup(cb, parameters["name"]));
}

void Stats::GetStatsGroupCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::Stats::StatsGroup *statsGroup)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);

    mSelectedStatsGroup = statsGroup;

    if (mCurrentStatsView != nullptr)
    {
        mCurrentStatsView->release();
        mCurrentStatsView = nullptr;
    }

    refreshStatsGroup();
    setupCreateStatsViewParameters(mActionCreateStatsView, mSelectedStatsGroup);

    REPORT_BLAZE_ERROR(blazeError);
}

void Stats::setupCreateStatsViewParameters(Pyro::UiNodeActionPtr actionCreateStatsView, Blaze::Stats::StatsGroup *statsGroup)
{
    Pyro::UiNodeParameterStruct& parameters = actionCreateStatsView->getParameters();
    parameters.clear();
    parameters.addEnum("periodType", Blaze::Stats::STAT_PERIOD_ALL_TIME, "periodType", "");
    for (const auto& scopeNameValueEntry : *statsGroup->getScopeNameValueMap())
    {
        const Blaze::Stats::ScopeName& scopeName = scopeNameValueEntry.first;
        const Blaze::Stats::ScopeValue& scopeValue = scopeNameValueEntry.second;
        //if scope value is a question mark defined for the group, then user input is required
        if (scopeValue == Blaze::Stats::KEY_SCOPE_VALUE_USER)
        {
            parameters.addString(scopeName.c_str(), eastl::to_string(scopeValue).c_str(), scopeName.c_str(), "*,total", "Scope for the Stat View.");
        }
    }
}

void Stats::initActionCreateStatsView(Pyro::UiNodeAction &action)
{
    action.setText("Create Stats View");
    action.setDescription("Creates a view of the selected stats group");
}

void Stats::actionCreateStatsView(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Stats::StatsGroup::CreateViewCb cb(this, &Stats::CreateViewCb);
    Blaze::EntityId entityId;

    entityId = gBlazeHub->getUserManager()->getPrimaryLocalUser()->getId();

    Blaze::Stats::ScopeNameValueMap scopeNameValueMap;
    for (const auto& scopeNameValueEntry : *mSelectedStatsGroup->getScopeNameValueMap())
    {
        const Blaze::Stats::ScopeName& scopeName = scopeNameValueEntry.first;
        const Blaze::Stats::ScopeValue& cfgScopeValue = scopeNameValueEntry.second;
        //user input is required only if the scope value is a question mark defined in cfg for the group
        if (cfgScopeValue == Blaze::Stats::KEY_SCOPE_VALUE_USER)
        {
            Blaze::Stats::ScopeValue inputScopeValue = 0;
            if (blaze_strcmp(parameters[scopeName], "*") == 0)
            {
                inputScopeValue = Blaze::Stats::KEY_SCOPE_VALUE_ALL;
            }
            else if (blaze_strcmp(parameters[scopeName], "total") == 0)
            {
                inputScopeValue = Blaze::Stats::KEY_SCOPE_VALUE_AGGREGATE;
            }
            else
            {
                inputScopeValue = parameters[scopeName];
            }
            scopeNameValueMap[scopeName] = inputScopeValue;
        }
    }

    mSelectedStatsGroup->createStatsView(
        cb,
        &entityId,
        1,
        parameters["periodType"],
        0,
        &scopeNameValueMap);
}

void Stats::CreateViewCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::Stats::StatsView *statsView)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);

    if (mCurrentStatsView != nullptr)
    {
        mCurrentStatsView->release();
        mCurrentStatsView = nullptr;
    }

    mCurrentStatsView = statsView;

    refreshStatsGroup();

    REPORT_BLAZE_ERROR(blazeError);
}

}
