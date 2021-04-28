/*************************************************************************************************/
/*!
    \file   collectorutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "collectorutil.h"

#include "gamereporting/tdf/gamereportingevents_server.h"
#include "framework/event/eventmanager.h"
#include "gamereporting/rpc/gamereportingslave.h"

namespace Blaze
{
namespace GameReporting
{
namespace Utilities
{

/**
    Collector
        Contains player reports mapped to a player ID. A convenience utility to store incoming game reports for later use.
        Ability to tag reports, allowing caller to group reports into categories (for example, IN_GAME (DNF), POST_GAME, or a custom type.
        
 */
Collector::Collector(CollectorData& data)
    : mData(data)
{
}

Collector::~Collector()
{
}

//  Sets the GameReport and its category for a specified player.
//      If a report already existed for that player, the old report is substituted with the new.
void Collector::addReport(BlazeId playerId, const GameReport& report, uint32_t reportCategory)
{
    CollectorGameReport* reportNode = mData.getReports().allocate_element();
    report.copyInto(reportNode->getGameReport());
    reportNode->setOrder(mData.getReportIndex());
    mData.setReportIndex(reportNode->getOrder() + 1);
    reportNode->setCategory(reportCategory);
    mData.getReports()[playerId] = reportNode;
}

void Collector::removeReport(BlazeId playerId)
{
    mData.getReports().erase(playerId);
}

void Collector::clearReports()
{
    mData.getReports().release();
    mData.setReportIndex(0);
}

//  Checks whether a report exists for specified player.
bool Collector::hasReport(BlazeId playerId) const
{
    return mData.getReports().find(playerId) != mData.getReports().end();
}

//  Checks whether the collector has reports for all players specified in the finished GameInfo object.
bool Collector::isFinished(const GameInfo& gameInfo) const
{
    //  is this a valid gameInfo object (i.e. when finished a game should have a reporting id.)
    if (gameInfo.getGameReportingId() == GameManager::INVALID_GAME_REPORTING_ID)
        return false;

    //  both player maps should be sorted by BlazeId, so a start-to-end comparision of the collector's player map with the gameinfo player map suffices.
    CollectorReportMap::const_iterator collectorPlayerItr = mData.getReports().begin();
    CollectorReportMap::const_iterator collectorPlayerEnd = mData.getReports().end();
    GameInfo::PlayerInfoMap::const_iterator gameInfoPlayerItr = gameInfo.getPlayerInfoMap().begin();
    GameInfo::PlayerInfoMap::const_iterator gameInfoPlayerEnd = gameInfo.getPlayerInfoMap().end();
    for (; collectorPlayerItr != collectorPlayerEnd && gameInfoPlayerItr != gameInfoPlayerEnd; ++collectorPlayerItr)
    {
        if (collectorPlayerItr->first == gameInfoPlayerItr->first)
        {
            ++gameInfoPlayerItr;
        }
        else 
        {
            const PlayerInfo *playerInfo = gameInfoPlayerItr->second;

            //  skip this player if he disconnected
            if (playerInfo->getRemovedReason() == GameManager::PLAYER_CONN_LOST || playerInfo->getRemovedReason() == GameManager::BLAZESERVER_CONN_LOST)
            {
                ++gameInfoPlayerItr;
            }
        }
    }

    //  if we've reached the end of GameInfo's player map, we're sure that at least every player in GameInfo is accounted for in the collector.
    return gameInfoPlayerItr == gameInfoPlayerEnd;
}

//  Checks whether the user submitting the report is in the game.
bool Collector::isUserInGame(BlazeId blazeId, const GameInfo& gameInfo, bool& isDNF) const
{
    //  is this a valid gameInfo object (i.e. when finished a game should have a reporting id.)
    if (gameInfo.getGameReportingId() == GameManager::INVALID_GAME_REPORTING_ID)
        return false;

    GameInfo::PlayerInfoMap::const_iterator gameInfoPlayerItr = gameInfo.getPlayerInfoMap().begin();
    GameInfo::PlayerInfoMap::const_iterator gameInfoPlayerEnd = gameInfo.getPlayerInfoMap().end();
    for (; gameInfoPlayerItr != gameInfoPlayerEnd; ++gameInfoPlayerItr)
    {
        if (blazeId == gameInfoPlayerItr->first)
        {
            isDNF = !gameInfoPlayerItr->second->getFinished();
            return true;
        }
    }

    return false;
}

//  Reject (throw out) reports that aren't in the game, 
void Collector::filterReports(const GameInfo& gameInfo)
{
    CollectorReportMap::iterator collectorPlayerItr = mData.getReports().begin();
    while (collectorPlayerItr != mData.getReports().end())
    {
        BlazeId blazeId = collectorPlayerItr->first;
        CollectorGameReport* report = collectorPlayerItr->second;
        //  find player in game's player list - if not in it, reject report.
        GameInfo::PlayerInfoMap::const_iterator gamePlayerIt = gameInfo.getPlayerInfoMap().find(blazeId);
        if (gamePlayerIt == gameInfo.getPlayerInfoMap().end())
        {
            INFO_LOG("[Collector].filterReports: throwing out report from user " << blazeId << " submitted for report id " << gameInfo.getGameReportingId());
            GameReportRejected reportRejected;
            reportRejected.setPlayerId(blazeId);
            report->getGameReport().copyInto(reportRejected.getGameReport());
            gEventManager->submitEvent((uint32_t)GameReportingSlave::EVENT_GAMEREPORTREJECTEDEVENT, reportRejected);
            collectorPlayerItr = mData.getReports().erase(collectorPlayerItr);
        }
        else
        {
            //  place mid-game reports of unfinished players into the DNF bin.
            if (report->getCategory() == REPORT_TYPE_GAME_MIDGAME)
            {
                const PlayerInfo& playerInfo = *gamePlayerIt->second;
                if (!playerInfo.getFinished())
                {
                    report->setCategory(REPORT_TYPE_GAME_DNF);
                }
            }
            ++collectorPlayerItr;
        }
    }
}

//  Checks whether there are any reports in the collector.
bool Collector::isEmpty() const
{
    return getGameReportMap().empty();
}

//  Retrieves a pointer to a player's game report (which the caller can modify) or nullptr if report doesn't exist.
GameReport* Collector::getGameReport(BlazeId playerId, uint32_t& reportCategory) const
{
    GameReport *report = nullptr;
    CollectorReportMap::iterator itr = mData.getReports().find(playerId);
    if (itr != mData.getReports().end())
    {
        report = &(itr->second->getGameReport());
        reportCategory = itr->second->getCategory();
    }
    return report;
}

//  Retrieves entire player report map
const CollectorReportMap& Collector::getGameReportMap() const
{
    return mData.getReports();
}

//  Retrieves all game reports of specified report category.
void Collector::getGameReportsByCategory(CategorizedGameReports& reports, uint32_t reportCategory) const
{
    reports.clear();
    //  assumption is that the number of reports in the map is small enough to just traverse the map's contents in search of reports
    //  instead of defining another multimap container with key = reportCategory.
    CollectorReportMap::iterator reportsItr = mData.getReports().begin();
    CollectorReportMap::iterator reportsEnd = mData.getReports().end();
    for (; reportsItr != reportsEnd; ++reportsItr)
    {
        CollectorGameReport* report = reportsItr->second;
        if (report->getCategory() == reportCategory)
        {
            CategorizedGameReport categorizedGameReport(reportsItr->first, &report->getGameReport());

            //  a bit inefficient but these are likely small vectors.  simple sort by order as we maintain order of the list
            //  during iteration.
            CategorizedGameReports::iterator catItr = reports.begin();
            CategorizedGameReports::iterator catEnd = reports.end();
            for (; catItr != catEnd; ++catItr)
            {
                //  determine if this report's order should take precedence over current item in the categorized list.
                GameManager::PlayerId referencePlayerId = catItr->first;
                CollectorReportMap::const_iterator refReportsItr = mData.getReports().find(referencePlayerId);
                if (refReportsItr->second->getOrder() > report->getOrder())
                    break;
            }
            reports.insert(catItr, categorizedGameReport);
        }
    }
}

//  Clear all game reports of specified report category.
void Collector::clearGameReportsByCategory(uint32_t reportCategory)
{
    //  assumption is that the number of reports in the map is small enough to just traverse the map's contents in search of reports
    //  instead of defining another multimap container with key = reportCategory.
    CollectorReportMap::iterator collectorPlayerItr = mData.getReports().begin();
    CollectorReportMap::iterator collectorPlayerEnd = mData.getReports().end();
    while (collectorPlayerItr != collectorPlayerEnd)
    {
        CollectorGameReport* report = collectorPlayerItr->second;
        if (report->getCategory() == reportCategory)
        {
            collectorPlayerItr = mData.getReports().erase(collectorPlayerItr);
        }
        else
        {
            ++collectorPlayerItr;
        }
    }
}

}   // namespace Utilities

}   // namespace GameReporting
}   // namespace Blaze
