/*************************************************************************************************/
/*!
    \file   collectorutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_GAMEREPORTING_COLLECTORUTIL_H
#define BLAZE_GAMEREPORTING_COLLECTORUTIL_H

#include "gamereporting/tdf/gamereporting.h"
#include "gamereporting/tdf/gamereporting_server.h"

#include "EASTL/vector_map.h"

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
class Collector
{
    NON_COPYABLE(Collector);

public:
    Collector(CollectorData& data);
    ~Collector();

    //  report type definitions used to categorize game reports.  
    //  custom report implementations may define their own report types relative to REPORT_TYPE_FIRST_CUSTOM.
    enum
    {
        REPORT_TYPE_GAME_MIDGAME = 0,
        REPORT_TYPE_GAME_FINISHED = 1,
        REPORT_TYPE_GAME_DNF = 2,
        REPORT_TYPE_FIRST_CUSTOM = 10
    };

    //  Sets the GameReport and its category for a specified player.
    //      If a report already existed for that player, the old report is substituted with the new.
    void addReport(BlazeId playerId, const GameReport& report, uint32_t reportCategory);
    void removeReport(BlazeId playerId);
    void clearReports();

    //  Retrieves a reference of a player's game report (which the caller can modify.)
    GameReport* getGameReport(BlazeId playerId, uint32_t& reportCategory) const;

    //  Checks whether a report exists for specified player.
    bool hasReport(BlazeId playerId) const;

    //  Checks whether the collector has reports for all players specified in the finished GameInfo object.
    bool isFinished(const GameInfo& gameInfo) const;

    //  Checks whether the user submitting the report is in the game.
    bool isUserInGame(BlazeId blazeId, const GameInfo& gameInfo, bool& isDNF) const;

    //  Reject (throw out) reports that aren't in the game
    void filterReports(const GameInfo& gameInfo);

    //  Checks whether there are any reports in the collector.
    bool isEmpty() const;

    //  Retrieves entire player report map
    const CollectorReportMap& getGameReportMap() const;

    //  Retrieves all game reports of specified report category in the order they were submitted.
    typedef eastl::pair<GameManager::PlayerId, GameReport*> CategorizedGameReport;
    typedef eastl::vector<CategorizedGameReport> CategorizedGameReports;
    void getGameReportsByCategory(CategorizedGameReports& reports, uint32_t reportCategory) const;
    void clearGameReportsByCategory(uint32_t reportCategory);

private:
    CollectorData& mData;
};

}   // namespace Utilities

}   // namespace GameReporting
}   // namespace Blaze

#endif
