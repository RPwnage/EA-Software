
#include "Ignition/Leaderboards.h"

namespace Ignition
{

Leaderboards::Leaderboards()
    : BlazeHubUiBuilder("Leaderboards")
{
    getActions().add(&getActionViewLeaderboards());
}

Leaderboards::~Leaderboards()
{
}

void Leaderboards::onAuthenticatedPrimaryUser()
{
    setIsVisibleForAll(true);
}

void Leaderboards::onDeAuthenticatedPrimaryUser()
{
    setIsVisibleForAll(false);
    getWindows().clear();
}

void Leaderboards::GetLeaderboardTreeCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::Stats::LeaderboardTree *leaderboardTree)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);

    Pyro::UiNodeTable &newTable = getWindow("Leaderboards").getTable("Leaderboard Hierarchy");
    newTable.setPosition(Pyro::ControlPosition::TOP_LEFT);

    newTable.getActions().insert(&getActionGetLeaderboardDataTopRanked());
    newTable.getActions().insert(&getActionGetLeaderboardDataCenteredAroundCurrentPlayer());

    newTable.getColumn("name").setText("Name");

    traverseLeaderboardFolderTree(leaderboardTree->getLeaderboardTreeTopFolder(), newTable.getRows());

    REPORT_BLAZE_ERROR(blazeError);
}

void Leaderboards::traverseLeaderboardFolderTree(const Blaze::Stats::LeaderboardTreeFolder *folder, Pyro::UiNodeTableRowContainer &rows)
{
    for (Blaze::Stats::LeaderboardTreeFolder::LeaderboardTreeFolderList::const_iterator i = folder->getFolderList()->begin();
        i != folder->getFolderList()->end();
        i++)
    {
        Pyro::UiNodeTableRow &row = rows.getById_va("folder%d", (*i)->getId());
        row.setIconImage(Pyro::IconImage::FOLDER_CLOSED);

        row.getField("name") = (*i)->getName();

        traverseLeaderboardFolderTree((*i), row.getChildRows());
    }

    for (Blaze::Stats::LeaderboardTreeFolder::LeaderboardTreeLeaderboardList::const_iterator i = folder->getLeaderboardList()->begin();
        i != folder->getLeaderboardList()->end();
        i++)
    {
        Pyro::UiNodeTableRow &row = rows.getById_va("lb%d", (*i)->getId());
        row.setIconImage(Pyro::IconImage::SCHEMA);

        row.getField("name") = (*i)->getName();
    }
}

void Leaderboards::GetLeaderboardDataTopRanked(const char8_t *leaderboardName)
{
    Blaze::Stats::GetLeaderboardCb cb(this, &Leaderboards::GetLeaderboardCb_GlobalView);
    vLOG(gBlazeHub->getLeaderboardAPI()->requestLeaderboard(cb, leaderboardName));
}

void Leaderboards::GetLeaderboardDataCenteredAroundCurrentPlayer(const char8_t *leaderboardName)
{
    Blaze::Stats::GetLeaderboardCb cb(this, &Leaderboards::GetLeaderboardCb_CenteredView);
    vLOG(gBlazeHub->getLeaderboardAPI()->requestLeaderboard(cb, leaderboardName));
}

void Leaderboards::initActionGetLeaderboardDataTopRanked(Pyro::UiNodeAction &action)
{
    action.setText("Get Leaderboard Data - Top Ranked");
}

void Leaderboards::actionGetLeaderboardDataTopRanked(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    GetLeaderboardDataTopRanked(parameters["name"]);
}

void Leaderboards::initActionGetLeaderboardDataCenteredAroundCurrentPlayer(Pyro::UiNodeAction &action)
{
    action.setText("Get Leaderboard Data - Centered Around Current Player");
}

void Leaderboards::actionGetLeaderboardDataCenteredAroundCurrentPlayer(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    GetLeaderboardDataCenteredAroundCurrentPlayer(parameters["name"]);
}

// This function is shows how to populate the ScopeNameValueMap's for a
// few of the leaderboard that require scope info in order to obtain data
// from them.
void Leaderboards::populateSampleScopeNameValueMap(const char8_t *leaderboardName, Blaze::Stats::ScopeNameValueMap *map)
{
    if (blaze_strcmp(leaderboardName, "UserAllTime") == 0)
    {
        map->insert(eastl::make_pair("map", 1));
    }
    else if (blaze_strcmp(leaderboardName, "WeaponsLB") == 0)
    {
        map->insert(eastl::make_pair("weapon", 2));
        map->insert(eastl::make_pair("level", 1));
    }
    else if (blaze_strcmp(leaderboardName, "SampleGameLB") == 0)
    {
        map->insert(eastl::make_pair("accountcountry", 21843));
    }
    else if (blaze_strcmp(leaderboardName, "ClubRegionalLB") == 0)
    {
        map->insert(eastl::make_pair("clubregion", 1));
    }
    else if (blaze_strcmp(leaderboardName, "ClubSeasonalLB") == 0)
    {
        map->insert(eastl::make_pair("seasonlevel", 1));
    }
    else if (blaze_strcmp(leaderboardName, "ClubMemberLB") == 0)
    {
        map->insert(eastl::make_pair("club", 1));
    }
    else if (blaze_strcmp(leaderboardName, "RivalsClubMemberPuplicLB") == 0)
    {
        map->insert(eastl::make_pair("club", 1));
    }
    else if (blaze_strcmp(leaderboardName, "MultiCatLBTest1") == 0)
    {
        map->insert(eastl::make_pair("level", 1));
    }
    else if (blaze_strcmp(leaderboardName, "MultiCatLBTest2") == 0)
    {
        map->insert(eastl::make_pair("level", 1));
    }
}

void Leaderboards::GetLeaderboardCb_GlobalView(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::Stats::Leaderboard *leaderboard)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);

    if (blazeError == Blaze::ERR_OK)
    {
        Blaze::Stats::ScopeNameValueMap scopeNameValueMap;

        populateSampleScopeNameValueMap(leaderboard->getName(), &scopeNameValueMap);

        Blaze::Stats::Leaderboard::CreateGlobalViewCb cb(this, &Leaderboards::CreateGlobalViewCb);
        vLOG(leaderboard->createGlobalLeaderboardView(cb, 0, 99, &scopeNameValueMap));
    }

    REPORT_BLAZE_ERROR(blazeError);
}

void Leaderboards::GetLeaderboardCb_CenteredView(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::Stats::Leaderboard *leaderboard)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);

    if (blazeError == Blaze::ERR_OK)
    {
        Blaze::Stats::ScopeNameValueMap scopeNameValueMap;

        populateSampleScopeNameValueMap(leaderboard->getName(), &scopeNameValueMap);

        Blaze::Stats::Leaderboard::CreateGlobalViewCb cb(this, &Leaderboards::CreateGlobalViewCb);
        vLOG(leaderboard->createGlobalLeaderboardViewCentered(cb, gBlazeHub->getLoginManager(0)->getBlazeId(), 100, &scopeNameValueMap));
    }
 
    REPORT_BLAZE_ERROR(blazeError);
}

void Leaderboards::CreateGlobalViewCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::Stats::GlobalLeaderboardView *globalLeaderboardView)
{
    REPORT_FUNCTOR_CALLBACK(nullptr);

    if (blazeError == Blaze::ERR_OK)
    {
        Pyro::UiNodeTable &tableNode = getWindow("Leaderboards").getTable("Leaderboard View");
        tableNode.setPosition(Pyro::ControlPosition::TOP_RIGHT);

        if (blazeError == Blaze::ERR_OK)
        {
            tableNode.getColumns().clear();
            tableNode.getRows().clear();

            int32_t columnIndex = 0;
            tableNode.getColumn_va("col%d", columnIndex++).setText("Entity Name");

            for (Blaze::Stats::StatDescSummaryList::const_iterator i = globalLeaderboardView->getLeaderboard()->getStatKeyColumnsList()->begin();
                i != globalLeaderboardView->getLeaderboard()->getStatKeyColumnsList()->end();
                i++)
            {
                Pyro::UiNodeTableColumn &column = tableNode.getColumn_va("col%d", columnIndex++);

                if (strcmp((*i)->getName(), globalLeaderboardView->getLeaderboard()->getRankingStat()) == 0)
                {
                    column.setText_va("%s (%s)", (*i)->getName(), globalLeaderboardView->getLeaderboard()->isAscending() ? "asc" : "desc");
                }
                else
                {
                    column.setText((*i)->getName());
                }
            }

            for (Blaze::Stats::LeaderboardStatValues::RowList::const_iterator i = globalLeaderboardView->getStatValues()->getRows().begin();
                i != globalLeaderboardView->getStatValues()->getRows().end();
                i++)
            {
                Pyro::UiNodeTableRow &rowNode = tableNode.getRow_va("%d", (*i)->getUser().getBlazeId());
                if (gBlazeHub->getLoginManager(0)->getBlazeId() == (*i)->getUser().getBlazeId())
                {
                    rowNode.setIsSelected(true);
                }

                columnIndex = 0;

                rowNode.getField_va("col%d", columnIndex++) = (*i)->getUser().getName();

                for (Blaze::Stats::LeaderboardStatValuesRow::StringStatValueList::const_iterator stats = (*i)->getOtherStats().begin();
                    stats != (*i)->getOtherStats().end();
                    stats++)
                {
                    rowNode.getField_va("col%d", columnIndex++) = (*stats).c_str();
                }
            }
        }
    }

    REPORT_BLAZE_ERROR(blazeError);
}

void Leaderboards::initActionViewLeaderboards(Pyro::UiNodeAction &action)
{
    action.setText("View Leaderboards");
    action.setDescription("View Leaderboards");
}

void Leaderboards::actionViewLeaderboards(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Stats::GetLeaderboardTreeCb cb(this, &Leaderboards::GetLeaderboardTreeCb);
    vLOG(gBlazeHub->getLeaderboardAPI()->requestLeaderboardTree(cb, "TopFolder"));
}

}
