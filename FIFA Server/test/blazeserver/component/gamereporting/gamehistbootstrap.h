/*************************************************************************************************/
/*!
    \file   gamehistbootstrap.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_GAME_HIST_BOOTSTRAP_H
#define BLAZE_GAME_HIST_BOOTSTRAP_H

/*** Include files *******************************************************************************/

#include "EASTL/string.h"
#include "EASTL/hash_set.h"
#include "EASTL/set.h"

#include "gamereporting/gamereportingslaveimpl.h"
#include "gamereporting/gametype.h"
#include "gamereporting/gamehistoryconfig.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
// Forward declarations

namespace GameReporting
{

class HistoryTableDefinition;

typedef eastl::set<ReportAttributeName, EA::TDF::TdfStringCompareIgnoreCase > ColumnNameSet;
typedef eastl::list<eastl::string> Index;
typedef eastl::hash_map<eastl::string, Index> IndexesByName;
typedef eastl::hash_map<eastl::string, IndexesByName> IndexesByTableName;
typedef eastl::hash_map<eastl::string, IndexesByTableName> IndexesByGameType;

static const char8_t RESERVED_GAMETABLE_NAME[] = "gamekey";
static const char8_t RESERVED_PARTICIPANTTABLE_NAME[] = "hist_participant";
static const char8_t RESERVED_SUBMITTER_TABLE_NAME[] = "hist_submitter";
static const char8_t GAMEID_COLUMN_PREFIX[] = "game_id_";

class GameHistoryBootstrap 
{
public:

    GameHistoryBootstrap(const GameReportingSlaveImpl* component,
                         const GameTypeMap* mGameTypeMap, 
                         const GameReportQueryConfigMap* gameReportQueryConfigMap,
                         const GameReportViewConfigMap* gameReportViewConfigMap,
                         bool allowDestruct);
    virtual ~GameHistoryBootstrap();

    bool run();

private:

    BlazeRpcError doDbBootStrap();

    bool retrieveExistingTables();

    bool initializeGameHistoryTables() const;

    bool initializeGameTypeTables(const GameType& gameType) const;

    bool initializeHistoryTable(const GameType& gameType, const char8_t* tableName,
        const HistoryTableDefinition* historyTableDefinition) const;

    bool createBuiltInGameTable(const GameType& gameType) const;

    bool createBuiltInParticipantTable(const GameType& gameType) const;

    bool createBuiltInSubmitterTable(const GameType& gameType) const;

    bool createHistoryTable(const GameType& gameType, const char8_t* tableName,
        const HistoryTableDefinition* historyTableDefinition) const;

    bool createGameRefTable(const GameType& gameType, const char8_t* tableName, 
        const HistoryTableDefinition* historyTableDefinition) const;

    bool verifyHistoryTableColumns(const GameType& gameType, const char8_t* tableName,
        const HistoryTableDefinition* historyTableDefinition) const;

    bool verifyGameRefTableColumns(const GameType& gameType, const char8_t* tableName,
        const HistoryTableDefinition* historyTableDefinition) const;

    void buildIndexesInMemory(const GameType& gameType, const char8_t* filterName,
        const char8_t* returnType, const GameReportFilterList& filterList);

    bool removeExtraIndexFromMap(IndexesByName& indexes, const Index& newIndex, bool* indexUsed = nullptr);

    BlazeRpcError createIndexesFromMemory();

    bool verifyGameKeyTableColumns(const GameType &gameType, const char8_t* tableName) const;

private:
    const GameReportingSlaveImpl* mComponent;
    const GameTypeMap* mGameTypeMap;
    const GameReportQueryConfigMap* mGameReportQueryConfigMap;
    const GameReportViewConfigMap* mGameReportViewConfigMap;

    bool mAllowDestructiveActions;

    StringHashSet mExistingTableNames;
    typedef eastl::map<const eastl::string, const char8_t*> GamekeyTableReservedColumnsMap;
    GamekeyTableReservedColumnsMap mGamekeyTableReservedColumnsMap;

    IndexesByGameType mIndexesInMemory;
};

} // GameReporting
} // Blaze
#endif // BLAZE_GAME_HIST_BOOTSTRAP_H
