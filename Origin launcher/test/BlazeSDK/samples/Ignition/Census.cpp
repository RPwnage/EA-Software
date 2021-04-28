
#include "Ignition/Census.h"

namespace Ignition
{

Census::Census()
    : BlazeHubUiBuilder("Census")
{
    getActions().add(&getActionStartListeningToCensus());
    getActions().add(&getActionStopListeningToCensus());
    getActions().add(&getActionGetLatestCensusData());
}

Census::~Census()
{
}


void Census::onAuthenticatedPrimaryUser()
{
    setIsVisibleForAll(false);
    getActionStartListeningToCensus().setIsVisible(true);
    getActionGetLatestCensusData().setIsVisible(true);
}

void Census::onDeAuthenticatedPrimaryUser()
{
    setIsVisibleForAll(false);
    getWindows().clear();
}

void Census::onNotifyCensusData()
{
    REPORT_LISTENER_CALLBACK(("This is called periodically so you can update your UI with fresh Census data"));

    const Blaze::GameManager::GameManagerCensusData* gmCensusData 
        = gBlazeHub->getGameManagerAPI()->getCensusData();
    const Blaze::Clubs::ClubsCensusData* clubsCensusData = nullptr;
    const Blaze::UserManagerCensusData* userSessionCensusData = gBlazeHub->getUserManager()->getCensusData();
    updateCensusWindow(gmCensusData, clubsCensusData, userSessionCensusData);
}

void Census::updateCensusWindow(const Blaze::GameManager::GameManagerCensusData* gmCensusData,
                                const Blaze::Clubs::ClubsCensusData* clubsCensusData,
                                const Blaze::UserManagerCensusData* userSessionCensusData)
{
    Pyro::UiNodeWindow &window = getWindow("Census Data");
    window.setDockingState(Pyro::DockingState::DOCK_TOP);

    Pyro::UiNodeTable &table = window.getTable("Census Values");
    table.getColumn("name").setText("Name");
    table.getColumn("count").setText("Count");
    table.getColumn("time").setText("Time (seconds)");
    table.getColumn("rate").setText("Player Matchmaking Rate");

    Pyro::UiNodeTableRowContainer& rows = table.getRows();
    if (gmCensusData != nullptr)
    {
        Pyro::UiNodeTableRow *row = &rows.getById("activeGames");
        row->getField("name") = "Number of Active Games";
        row->getField("count") = gmCensusData->getNumOfActiveGame();
        //values["Number of Active Games"] = gmCensusData->getNumOfActiveGame(); //, "The number of games currently being players on the Blaze server",
        for (Blaze::GameManager::GameManagerCensusData::CountsPerPingSite::const_iterator it = gmCensusData->getNumOfActiveGamePerPingSite().begin(),
             end = gmCensusData->getNumOfActiveGamePerPingSite().end();
             it != end;
             ++it)
        {
            eastl::string buf;
            buf.append_sprintf("Number of Active Games (%s)", it->first.c_str());
            Pyro::UiNodeTableRowContainer &childRows = row->getChildRows();
            Pyro::UiNodeTableRow &childRow = childRows.getById_va("activeGames_%s", it->first.c_str());
            childRow.getField("name") = buf.c_str();
            childRow.getField("count") = it->second;
        }

        row = &rows.getById("joinedPlayers");
        row->getField("name") = "Number of Joined Players";
        row->getField("count") = gmCensusData->getNumOfJoinedPlayer();
        for (Blaze::GameManager::GameManagerCensusData::CountsPerPingSite::const_iterator it = gmCensusData->getNumOfJoinedPlayerPerPingSite().begin(),
            end = gmCensusData->getNumOfJoinedPlayerPerPingSite().end();
            it != end;
            ++it)
        {
            eastl::string buf;
            buf.append_sprintf("Number of Joined Players (%s)", it->first.c_str());
            Pyro::UiNodeTableRowContainer &childRows = row->getChildRows();
            Pyro::UiNodeTableRow &childRow = childRows.getById_va("joinedPlayers_%s", it->first.c_str());
            childRow.getField("name") = buf.c_str();
            childRow.getField("count") = it->second;
        }

        row = &rows.getById("gameGroups");
        row->getField("name") = "Number of Game Groups";
        row->getField("count") = gmCensusData->getNumOfGameGroup();

        row = &rows.getById("numPlayersGameGroups");
        row->getField("name") = "Number of Joined Players in Game Groups";
        row->getField("count") = gmCensusData->getNumOfPlayersInGameGroup();

        row = &rows.getById("sessions");
        row->getField("name") = "Number of Logged Sessions";
        row->getField("count") = gmCensusData->getNumOfLoggedSession();

        row = &rows.getById("mmSessions");
        row->getField("name") = "Number of Matchmaking Sessions";
        row->getField("count") = gmCensusData->getNumOfMatchmakingSession();

        // expand out by scenario
        row = &rows.getById("scenarioMMSessions");
        row->getField("name") = "Matchmaking Sessions (per-Scenario)";
        // expand out by pingsite
        for (auto& it : gmCensusData->getMatchmakingCensusData().getScenarioMatchmakingData())
        {
            eastl::string buf;
            buf.append_sprintf("Matchmaking Sessions for '%s' (global)", it.first.c_str()); 
            Pyro::UiNodeTableRowContainer &childRows = row->getChildRows();
            Pyro::UiNodeTableRow &childRow = childRows.getById_va("numSessions_%s", it.first.c_str());
            childRow.getField("name") = buf.c_str();
            childRow.getField("count") .setAsString_va("%u", it.second->getNumOfMatchmakingSession());

            // expand scenarios by ping site
            for (auto& pingSiteIter : it.second->getMatchmakingSessionPerPingSite())
            {
                eastl::string pingSiteBuf;
                pingSiteBuf.append_sprintf("Matchmaking Sessions for ping site '%s'", pingSiteIter.first.c_str());
                Pyro::UiNodeTableRowContainer &pingSiteChildRows = childRow.getChildRows();
                Pyro::UiNodeTableRow &pingSiteChildRow = pingSiteChildRows.getById_va("numSessions_%s", pingSiteIter.first.c_str());
                pingSiteChildRow.getField("name") = pingSiteBuf.c_str();
                pingSiteChildRow.getField("count").setAsString_va("%u", pingSiteIter.second);
            }
        }

        row = &rows.getById("estimatedTTM");
        row->getField("name") = "Estimated Time To Match (all matchmaking)";
        row->getField("count") = "";
        // expand out by pingsite
        for (auto ttmPs : gmCensusData->getMatchmakingCensusData().getEstimatedTimeToMatchPerPingsiteGroup())
        {
            for (auto ttm : *ttmPs.second)
            {
                eastl::string buf;
                buf.append_sprintf("Estimated Time To Match for ping site '%s' group '%s'", ttmPs.first.c_str(), ttm.first.c_str());
                Pyro::UiNodeTableRowContainer &childRows = row->getChildRows();
                Pyro::UiNodeTableRow &childRow = childRows.getById_va("ettm_%s_%s", ttmPs.first.c_str(), ttm.first.c_str());
                childRow.getField("name") = buf.c_str();
                childRow.getField("count") = "";
                childRow.getField("time").setAsString_va("%f", (float)ttm.second.getMicroSeconds() / 1000000.0f);
            }
        }

        // expand out by scenario
        row = &rows.getById("estimatedScenarioTTM");
        row->getField("name") = "Estimated Time To Match (per-Scenario)";
        // expand out by pingsite
        for (Blaze::GameManager::ScenarioMatchmakingCensusDataMap::const_iterator it = gmCensusData->getMatchmakingCensusData().getScenarioMatchmakingData().begin(),
            end = gmCensusData->getMatchmakingCensusData().getScenarioMatchmakingData().end();
            it != end;  ++it)
        {
            eastl::string buf;
            buf.append_sprintf("Estimated Time To Match for '%s' (global)", it->first.c_str());
            Pyro::UiNodeTableRowContainer &childRows = row->getChildRows();
            Pyro::UiNodeTableRow &childRow = childRows.getById_va("ettm_%s", it->first.c_str());
            childRow.getField("name") = buf.c_str();
            childRow.getField("count") = "";
            childRow.getField("time").setAsString_va("%f", (float)it->second->getGlobalEstimatedTimeToMatch().getMicroSeconds() / 1000000.0f);
            // expand scenarios by ping site
            for (auto ttmPs : it->second->getEstimatedTimeToMatchPerPingsiteGroup())
            {
                for (auto ttm : *ttmPs.second)
                {
                    eastl::string pingSiteBuf;
                    pingSiteBuf.append_sprintf("Estimated Time To Match for ping site '%s' group '%s'", ttmPs.first.c_str(), ttm.first.c_str());
                    Pyro::UiNodeTableRowContainer &pingSiteChildRows = childRow.getChildRows();
                    Pyro::UiNodeTableRow &pingSiteChildRow = pingSiteChildRows.getById_va("ettm_%s_%s", ttmPs.first.c_str(), ttm.first.c_str());
                    pingSiteChildRow.getField("name") = pingSiteBuf.c_str();
                    pingSiteChildRow.getField("count") = "";
                    pingSiteChildRow.getField("time").setAsString_va("%f", (float)ttm.second.getMicroSeconds() / 1000000.0f);
                }
            }
        }

        // expand scenarios by ping site
        row = &rows.getById("PMR");
        row->getField("name") = "Player Matchmaking Rate (all matchmaking)";
        row->getField("count") = "";
        // expand out by pingsite
        for (auto pmrPs : gmCensusData->getMatchmakingCensusData().getPlayerMatchmakingRatePerPingsiteGroup())
        {
            for (auto pmr : *pmrPs.second)
            {
                eastl::string buf;
                buf.append_sprintf("Player Matchmaking Rate for ping site '%s' group '%s'", pmrPs.first.c_str(), pmr.first.c_str());
                Pyro::UiNodeTableRowContainer &childRows = row->getChildRows();
                Pyro::UiNodeTableRow &childRow = childRows.getById_va("pmr_%s_%s", pmrPs.first.c_str(), pmr.first.c_str());
                childRow.getField("name") = buf.c_str();
                childRow.getField("count") = "";
                childRow.getField("time") = "";
                childRow.getField("rate").setAsString_va("%f", pmr.second);
            }
        }

        // expand out by scenario
        row = &rows.getById("ScenarioPMR");
        row->getField("name") = "Player Matchmaking Rate (per-Scenario)";
        // expand out by pingsite
        for (auto& it : gmCensusData->getMatchmakingCensusData().getScenarioMatchmakingData())
        {
            eastl::string buf;
            buf.append_sprintf("Player Matchmaking Rate for '%s' (global)", it.first.c_str());
            Pyro::UiNodeTableRowContainer &childRows = row->getChildRows();
            Pyro::UiNodeTableRow &childRow = childRows.getById_va("pmr_%s", it.first.c_str());
            childRow.getField("name") = buf.c_str();
            childRow.getField("count") = "";
            childRow.getField("time") = "";
            childRow.getField("rate").setAsString_va("%f", it.second->getGlobalPlayerMatchmakingRate());
            // expand scenarios by ping site
            for (auto pmrPs : it.second->getPlayerMatchmakingRatePerPingsiteGroup())
            {
                for (auto pmr : *pmrPs.second)
                {
                    eastl::string pingSiteBuf;
                    pingSiteBuf.append_sprintf("Player Matchmaking Rate for ping site '%s' group '%s'", pmrPs.first.c_str(), pmr.first.c_str());
                    Pyro::UiNodeTableRowContainer &pingSiteChildRows = childRow.getChildRows();
                    Pyro::UiNodeTableRow &pingSiteChildRow = pingSiteChildRows.getById_va("pmr_%s_%s", pmrPs.first.c_str(), pmr.first.c_str());
                    pingSiteChildRow.getField("name") = pingSiteBuf.c_str();
                    pingSiteChildRow.getField("count") = "";
                    pingSiteChildRow.getField("time") = "";
                    pingSiteChildRow.getField("rate").setAsString_va("%f", pmr.second);
                }
            }
        }

    }

    if (clubsCensusData != nullptr)
    {
        Pyro::UiNodeTableRow *row = &rows.getById("clubs");
        row->getField("name") = "Number of Clubs";
        row->getField("count") = clubsCensusData->getNumOfClubs();

        row = &rows.getById("clubs");
        row->getField("name") = "Number of Club Members in Clubs";
        row->getField("count") = clubsCensusData->getNumOfClubMembers();
    }

    if (userSessionCensusData != nullptr)
    {
        Pyro::UiNodeTableRow *row = &rows.getById("userscountclient");
        row->getField("name") = "Number of Users by Client Type";
        uint32_t totalUsers = 0;

        for (Blaze::ConnectedCountsMap::const_iterator it = userSessionCensusData->getConnectedPlayerCounts().begin(), end = userSessionCensusData->getConnectedPlayerCounts().end();
             it != end;
             ++it)
        {
            eastl::string buf;
            buf.append_sprintf("Number of Users (%s)", Blaze::ClientTypeToString(it->first));
            Pyro::UiNodeTableRowContainer &childRows = row->getChildRows();
            Pyro::UiNodeTableRow &childRow = childRows.getById_va("userscountclient_%s", Blaze::ClientTypeToString(it->first));
            childRow.getField("name") = buf.c_str();
            childRow.getField("count") = it->second;
            totalUsers += it->second;
        }
        row->getField("count") = totalUsers;
        totalUsers = 0;

        row = &rows.getById("userscountpingsite");
        row->getField("name") = "Number of Users by Ping Site";

        for (Blaze::ConnectedCountsByPingSiteMap::const_iterator it = userSessionCensusData->getConnectedPlayerCountByPingSite().begin(),
            end = userSessionCensusData->getConnectedPlayerCountByPingSite().end();
            it != end;
            ++it)
        {
            eastl::string buf;
            buf.append_sprintf("Number of Users (%s)", it->first.c_str());
            Pyro::UiNodeTableRowContainer &childRows = row->getChildRows();
            Pyro::UiNodeTableRow &childRow = childRows.getById_va("userscountpingsite_%s", it->first.c_str());
            childRow.getField("name") = buf.c_str();
            childRow.getField("count") = it->second;
            totalUsers += it->second;
        }
        row->getField("count") = totalUsers;
    }
}

void Census::actionStartListeningToCensus(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    vLOG(gBlazeHub->getCensusDataAPI()->addListener(this));

    vLOG(gBlazeHub->getCensusDataAPI()->subscribeToCensusData(
        Blaze::CensusData::CensusDataAPI::SubscribeToCensusDataCb(this, &Census::SubscribeToCensusDataCb)));

    getActionStartListeningToCensus().setIsVisible(false);
    getActionStopListeningToCensus().setIsVisible(true);
}

void Census::SubscribeToCensusDataCb(Blaze::BlazeError blazeError)
{
    REPORT_FUNCTOR_CALLBACK("This is called when the call to subscribeToCensusData completes");
    REPORT_BLAZE_ERROR(blazeError);
}

void Census::actionStopListeningToCensus(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    vLOG(gBlazeHub->getCensusDataAPI()->unsubscribeFromCensusData(
        Blaze::CensusData::CensusDataAPI::UnsubscribeFromCensusDataCb(this, &Census::UnsubscribeToCensusDataCb)));

    vLOG(gBlazeHub->getCensusDataAPI()->removeListener(this));

    getActionStartListeningToCensus().setIsVisible(true);
    getActionStopListeningToCensus().setIsVisible(false);
}

void Census::UnsubscribeToCensusDataCb(Blaze::BlazeError blazeError)
{
    REPORT_FUNCTOR_CALLBACK("This is called when the call to unsubscribeToCensusData completes");
    REPORT_BLAZE_ERROR(blazeError);
}

void Census::actionGetLatestCensusData(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    vLOG(gBlazeHub->getCensusDataAPI()->getLatestCensusData(
        Blaze::CensusData::CensusDataComponent::GetLatestCensusDataCb(this, &Census::GetLatestCensusDataCb)));
}

void Census::GetLatestCensusDataCb(const Blaze::CensusData::NotifyServerCensusData *resultValue, Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_FUNCTOR_CALLBACK("This is called when the call to GetLatestCensusDataCb completes");
    if (blazeError == Blaze::ERR_OK)
    {
        const Blaze::GameManager::GameManagerCensusData* gmCensusData = nullptr;
        const Blaze::Clubs::ClubsCensusData* clubsCensusData = nullptr;
        const Blaze::UserManagerCensusData* userSessionCensusData = nullptr;
        Blaze::CensusData::NotifyServerCensusDataList::const_iterator it= resultValue->getCensusDataList().begin(),
            endit = resultValue->getCensusDataList().end();
        for (; it != endit; ++it)
        {
            const EA::TDF::Tdf* tdf = (*it)->getTdf();
            if (tdf != nullptr)
            {
                switch (tdf->getTdfId())
                {
                case Blaze::GameManager::GameManagerCensusData::TDF_ID:
                    gmCensusData = static_cast<const Blaze::GameManager::GameManagerCensusData*>(tdf);
                    break;
                case Blaze::Clubs::ClubsCensusData::TDF_ID:
                    clubsCensusData = static_cast<const Blaze::Clubs::ClubsCensusData*>(tdf);
                    break;
                case Blaze::UserManagerCensusData::TDF_ID:
                    userSessionCensusData = static_cast<const Blaze::UserManagerCensusData*>(tdf);
                    break;
                default:
                    break;
                }
            }
        }
        updateCensusWindow(gmCensusData, clubsCensusData, userSessionCensusData);
    }
    REPORT_BLAZE_ERROR(blazeError);
}

void Census::initActionStartListeningToCensus(Pyro::UiNodeAction &action)
{
    action.setText("Start Listening to Census");
    action.setDescription("Report live census data from the Blaze server");
}

void Census::initActionStopListeningToCensus(Pyro::UiNodeAction &action)
{
    action.setText("Stop Listening to Census");
    action.setDescription("Stop reporting live census data");
}

void Census::initActionGetLatestCensusData(Pyro::UiNodeAction &action)
{
    action.setText("Get latest census data");
    action.setDescription("Get latest census data.");
}


}
