/*************************************************************************************************/
/*!
    \file   getgamereports_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetGameReports

    Retrieves game reports allowing the client to explicitly specify query name and variables.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"

// gamereporting includes
#include "gamereporting/tdf/gamereporting.h"
#include "gamereporting/tdf/gamehistory.h"
#include "gamereportingslaveimpl.h"
#include "gamereporting/rpc/gamereportingslave/getgamereports_stub.h"
#include "gamereportingconfig.h"
#include "gamehistoryconfig.h"
#include "gamereporting/gamehistbootstrap.h"

#include "EASTL/set.h"

namespace Blaze
{
namespace GameReporting
{

typedef eastl::set<GameHistoryId> GameIdSet;

class GetGameReportsCommand : public GetGameReportsCommandStub
{
public:
    GetGameReportsCommand(Message* message, GetGameReports* request, GameReportingSlaveImpl* componentImpl);
    ~GetGameReportsCommand() override;

private:
    // Not owned memory.
    GameReportingSlaveImpl* mComponent;

    const GameReportQueryConfig* mQueryConfig;
    const GameReportQuery* mQuery;
    const GameType *mGameType;

    bool mOwnQueryConfig;

    GameIdSet mGameIdSet;

    // States
    GetGameReportsCommandStub::Errors execute() override;

    void updateMapFromDbRow(const HistoryTableDefinition::ColumnList &columnList, const char8_t *table,
        Collections::AttributeMap &map, const DbRow *row, const StringHashSet* columnSet) const;
    bool retrieveRowsFromDbByGameId(DbConnPtr& conn, HistoryTableDefinitions::const_iterator tableDefIter);

};
DEFINE_GETGAMEREPORTS_CREATE()

GetGameReportsCommand::GetGameReportsCommand(Message* message, GetGameReports* request, GameReportingSlaveImpl* componentImpl)
    : GetGameReportsCommandStub(message, request),
    mComponent(componentImpl), mQueryConfig(nullptr), mQuery(nullptr), mGameType(nullptr), mOwnQueryConfig(false)
{
}

GetGameReportsCommand::~GetGameReportsCommand()
{
    if (mOwnQueryConfig)
        delete mQueryConfig;
}
/* Private methods *******************************************************************************/  
void GetGameReportsCommand::updateMapFromDbRow(const HistoryTableDefinition::ColumnList &columnList, const char8_t *table,
                                               Collections::AttributeMap &map, const DbRow *row, const StringHashSet* columnSet) const
{
    HistoryTableDefinition::ColumnList::const_iterator colIter, colEnd;
    colIter = columnList.begin();
    colEnd = columnList.end();

    for (; colIter != colEnd; ++colIter)
    {
        if (columnSet != nullptr && columnSet->find(colIter->first.c_str()) == columnSet->end())
        {
            continue;
        }

        char8_t asStr[Collections::MAX_ATTRIBUTEVALUE_LEN];

        switch (colIter->second)
        {
        case HistoryTableDefinition::INT:
            blaze_snzprintf(asStr, sizeof(asStr), "%d", row->getInt(colIter->first.c_str()));
            break;
        case HistoryTableDefinition::UINT:
            blaze_snzprintf(asStr, sizeof(asStr), "%u", row->getUInt(colIter->first.c_str()));
            break;
        case HistoryTableDefinition::DEFAULT_INT:
            blaze_snzprintf(asStr, sizeof(asStr), "%" PRId64, row->getInt64(colIter->first.c_str()));
            break;
        case HistoryTableDefinition::ENTITY_ID:
            blaze_snzprintf(asStr, sizeof(asStr), "%" PRId64, (int64_t)row->getUInt64(colIter->first.c_str()));
            break;
        case HistoryTableDefinition::FLOAT:
            blaze_snzprintf(asStr, sizeof(asStr), "%.2f", (double_t) row->getFloat(colIter->first.c_str()));
            break;
        case HistoryTableDefinition::STRING:
            blaze_snzprintf(asStr, sizeof(asStr), "%s", row->getString(colIter->first.c_str()));
            break;
        default:
            continue;
        }

        map[colIter->first.c_str()] = asStr;
    }
}

bool GetGameReportsCommand::retrieveRowsFromDbByGameId(DbConnPtr& conn, HistoryTableDefinitions::const_iterator tableDefIter)
{
    if (mQueryConfig == nullptr)
        return false;

    QueryPtr tableQuery = DB_NEW_QUERY_PTR(conn);

    tableQuery->append("SELECT `game_id`");

    const StringHashSet* columnSet = mQueryConfig->getColumnSetByTable(tableDefIter->first);
    if (columnSet != nullptr)
    {
        StringHashSet::const_iterator cIter, cEnd;
        cIter = columnSet->begin();
        cEnd = columnSet->end();

        for (; cIter != cEnd; ++cIter)
        {
            tableQuery->append(", `$s`", cIter->c_str());
        }
    }
    else
    {
        HistoryTableDefinition::ColumnList::const_iterator columnIter, columnEnd;
        columnIter = tableDefIter->second->getColumnList().begin();
        columnEnd = tableDefIter->second->getColumnList().end();

        for (; columnIter != columnEnd; ++columnIter)
        {
            tableQuery->append(", `$s`", columnIter->first.c_str());
        }
    }

    char8_t tableName[GameTypeReportConfig::GameHistory::MAX_TABLENAME_LEN];
    if (mGameType == nullptr)
        return false;
    if ( blaze_stricmp(tableDefIter->first.c_str(), RESERVED_GAMETABLE_NAME) != 0)
    {
        blaze_snzprintf(tableName, sizeof(tableName), "%s_hist_%s",
            mGameType->getGameReportName().c_str(), tableDefIter->first.c_str());
    }
    else
    {
        blaze_snzprintf(tableName, sizeof(tableName), "%s_%s",
            mGameType->getGameReportName().c_str(), tableDefIter->first.c_str());
    }
    blaze_strlwr(tableName);

    tableQuery->append(" FROM `$s` WHERE `game_id` IN (", tableName);

    GameIdSet::const_iterator gameIter, gameFirst;
    gameIter = gameFirst = mGameIdSet.begin();

    for (; gameIter != mGameIdSet.end(); ++gameIter)
    {
        if (gameIter != gameFirst)
        {
            tableQuery->append(", ");
        }
        tableQuery->append("$U", *gameIter);
    }

    tableQuery->append(")");

    DbResultPtr tableResult;
    BlazeRpcError dberr = conn->executeQuery(tableQuery, tableResult);

    if (dberr != DB_ERR_OK)
    {
        TRACE_LOG("[GetGameReportsCommand].execute - Error executing query: " << getDbErrorString(dberr) << ".");
        return false;
    }

    if (tableResult != nullptr && !tableResult->empty())
    {
        DbResult::const_iterator rowIter = tableResult->begin();

        // insert data to attribute map
        for (; rowIter != tableResult->end(); ++rowIter)
        {
            const DbRow *row = *rowIter;
            GameHistoryId gameId = row->getUInt64("game_id");

            GameReportsList::GameHistoryReportList::iterator iter, end;
            iter = (mResponse.getGameReportList()).begin();
            end = (mResponse.getGameReportList()).end();
            for (; iter != end && (*iter)->getGameHistoryId() != gameId; ++iter)
            {
            }

            GameHistoryReport *gameReport = *iter;

            GameHistoryReport::TableRows *tableRows;
            GameHistoryReport::TableRowMap::iterator tableIter = 
                gameReport->getTableRowMap().find(tableDefIter->first.c_str());

            if (tableIter != gameReport->getTableRowMap().end())
            {
                tableRows = tableIter->second;
            }
            else
            {
                //tableRows = gameReport->getTableRowMap().allocate_element();
                //tableRows should have been allocated
                continue;
            }

            GameHistoryReport::TableRow *tableRow = tableRows->getTableRowList().pull_back();

            if (columnSet != nullptr)
                tableRow->getAttributeMap().reserve(columnSet->size());
            else
                tableRow->getAttributeMap().reserve(tableDefIter->second->getColumnList().size());

            updateMapFromDbRow(tableDefIter->second->getColumnList(), tableDefIter->first,
                tableRow->getAttributeMap(), row, columnSet);
        }
    }

    return true;
}

GetGameReportsCommandStub::Errors GetGameReportsCommand::execute()
{
    //Check if we trust this user
    if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_GET_GAMEREPORT_INFORMATION))
    {
        WARN_LOG("[GetGameReportsCommandStub].execute: User [" << UserSession::getCurrentUserBlazeId() << "] attempted to get reports info ["<< mRequest.getQueryName() << "], no permission!");
        return ERR_AUTHORIZATION_REQUIRED;
    }

    if ((mRequest.getQueryName())[0] != '\0')
    {
        mQueryConfig = mComponent->getGameReportQueryConfig(mRequest.getQueryName());
        if (mQueryConfig == nullptr)
        {
            return GAMEHISTORY_ERR_UNKNOWN_QUERY;
        }
        mQuery = mQueryConfig->getGameReportQuery();
        mGameType = mComponent->getGameTypeCollection().getGameType(mQuery->getTypeName());
    }
    else
    {
        mGameType = mComponent->getGameTypeCollection().getGameType(mRequest.getGameReportQuery().getTypeName());
        if (mGameType == nullptr)
        {
            ERR_LOG("[GetGameReportsCommand].execute(): Game type '" << mRequest.getGameReportQuery().getTypeName() << "' does not exist.");
            return GAMEHISTORY_ERR_INVALID_GAMETYPE;
        }

        if (mGameType->getHistoryTableDefinitions().empty())
        {
            ERR_LOG("[GetGameReportsCommand].execute(): No game history table defined for game type '" << mRequest.getGameReportQuery().getTypeName() << "'.");
            return ERR_SYSTEM;
        }

        GameReportQuery::GameReportColumnKeyList::const_iterator keyIter, keyEnd;
        keyIter = mRequest.getGameReportQuery().getColumnKeyList().begin();
        keyEnd = mRequest.getGameReportQuery().getColumnKeyList().end();

        for (; keyIter != keyEnd; ++keyIter)
        {
            if (!validateGameReportColumnKey(**keyIter, mGameType))
            {
                ERR_LOG("[GetGameReportsCommand].execute(): Failed to validate the game report column (attribute type: '" 
                    << mRequest.getGameReportQuery().getTypeName() << "', table: '" << (*keyIter)->getTable() << "', attribute name: '" 
                    << (*keyIter)->getAttributeName() << ", index: '" << (*keyIter)->getIndex() << "'.");
                return GAMEHISTORY_ERR_INVALID_COLUMNKEY;
            }
        }

        GameReportFilterList::iterator filtIter, filtEnd;
        filtIter = mRequest.getGameReportQuery().getFilterList().begin();
        filtEnd = mRequest.getGameReportQuery().getFilterList().end();

        for (; filtIter != filtEnd; ++filtIter)
        {
            if (!validateGameReportFilter(**filtIter, mGameType, false))
            {
                ERR_LOG("[GetGameReportsCommand].execute(): Failed to validate the filter "
                    "(attribute type: '" << mRequest.getGameReportQuery().getTypeName() << "', table: '" << (*filtIter)->getTable() 
                    << "', attribute name: '" << (*filtIter)->getAttributeName() << ", index: '" << (*filtIter)->getIndex() << "'.");
                return GAMEHISTORY_ERR_INVALID_FILTER;
            }
        }

        mQueryConfig = BLAZE_NEW GameReportQueryConfig(mRequest.getGameReportQuery(), mGameType);
        mQuery = mQueryConfig->getGameReportQuery();
        mOwnQueryConfig = true;
    }

    if (mRequest.getQueryVarValues().size() < mQueryConfig->getNumOfVars())
    {
        ERR_LOG("[GetGameReportsCommand].execute(): Number of query variables provided is not enough.");
        return GAMEHISTORY_ERR_MISSING_QVARS;
    }

    // start the query by getting a connection.
    DbConnPtr conn = mComponent->getDbReadConnPtr(mGameType->getGameReportName());
    if (conn == nullptr)
    {
        ERR_LOG("[GetGameReportsCommand].execute(): Failed to obtain connection.");
        return ERR_SYSTEM;
    }

    QueryPtr query = DB_NEW_QUERY_PTR(conn);

    query->append(mQueryConfig->getQueryStringPrependFilter());

    QueryVarValuesList::const_iterator qIter, qEnd;
    qIter = mRequest.getQueryVarValues().begin();
    qEnd = mRequest.getQueryVarValues().end();

    GameReportFilterList::const_iterator fIter, fEnd;
    fIter = mQuery->getFilterList().begin();
    fEnd = mQuery->getFilterList().end();

    bool invalidValueFound = false;
    bool needAndOperator = false;

    for (; fIter != fEnd && qIter != qEnd; ++fIter)
    {
        if ((*fIter)->getHasVariable() && (*fIter)->getIndex() == 0)
        {
            if (!validateQueryVarValue((*qIter).c_str()))
            {
                ERR_LOG("[GetGameReportViewCommand].execute(): Invalid value in one of the provided query variable.");
                invalidValueFound = true;
                break;
            }

            (needAndOperator) && (query->append(" AND "));

            if ((*fIter)->getEntityType() == EA::TDF::OBJECT_TYPE_INVALID)
            {
                query->append((*fIter)->getExpression(), qIter->c_str());
            }
            else
            {
                invalidValueFound = true;
                EntityId id;
                if (blaze_str2int(qIter->c_str(), &id) != qIter->c_str())
                {
                    EA::TDF::ObjectId bobjId((*fIter)->getEntityType(), id);
                    const eastl::string expandedList = expandBlazeObjectId(bobjId).c_str();
                    if (!expandedList.empty())
                    {
                        invalidValueFound = false;
                        query->append((*fIter)->getExpression(), expandedList.c_str());
                    }
                }

                if (invalidValueFound)
                {
                    ERR_LOG("[GetGameReportsCommand].execute(): Failed to convert '" << qIter->c_str() << "' to blaze object id of type " 
                        << (*fIter)->getEntityType().toString().c_str() << ".");
                    break;
                }
            }

            needAndOperator = true;
            ++qIter;
        }
    }

    if (invalidValueFound)
    {
        return GAMEHISTORY_ERR_INVALID_QVARS;
    }

    // set the max number of rows to return
    uint32_t maxRowsToReturn = mQuery->getMaxGameReport();
    if ((mRequest.getMaxGameReport() > 0) && (maxRowsToReturn > mRequest.getMaxGameReport()))
        maxRowsToReturn = mRequest.getMaxGameReport();

    query->append(mQueryConfig->getQueryStringAppendFilter(), maxRowsToReturn);

    DbResultPtr result;
    BlazeRpcError dberr = conn->executeQuery(query, result);

    if (dberr != DB_ERR_OK)
    {
        ERR_LOG("[GetGameReportsCommand].execute - Error executing query: " << getDbErrorString(dberr) << ".");
        return GAMEHISTORY_ERR_INVALID_QUERY;
    }

    if (result != nullptr && !result->empty())
    {
        DbResult::const_iterator rowIter = result->begin();

        if (mGameType == nullptr)
            return ERR_SYSTEM;
        const HistoryTableDefinitions& historyTableDefinitions = mGameType->getHistoryTableDefinitions();
        const HistoryTableDefinitions& builtInTableDefinitions = mGameType->getBuiltInTableDefinitions();
        const StringHashSet* tableSet = mQueryConfig->getRequestedTables();

        (mResponse.getGameReportList()).reserve(result->size());

        StringHashSet::const_iterator tableIter, tableBegin, tableEnd;
        tableBegin = tableSet->begin();
        tableEnd = tableSet->end();

        // insert data of game attributes to game attribute map
        for (; rowIter != result->end(); ++rowIter)
        {
            DbRow *row = *rowIter;

            GameHistoryReport *gameReport = (mResponse.getGameReportList()).pull_back();
            gameReport->setGameHistoryId(row->getUInt64("game_id"));
            gameReport->setGameReportingId(row->getUInt64("game_id"));
            gameReport->setFlags(row->getUInt64("flags"));
            gameReport->setFlagReason(row->getString("flag_reason"));
            gameReport->setOnline(row->getUInt64("online") == 1 ? true : false);
            gameReport->setTimestamp(row->getInt64("timestamp"));

            gameReport->setGameReportName(mQuery->getTypeName());
            gameReport->getTableRowMap().reserve(tableSet->size());

            for (tableIter = tableBegin; tableIter != tableEnd; ++tableIter)
            {
                GameHistoryReport::TableRows *tableRows = gameReport->getTableRowMap().allocate_element();
                gameReport->getTableRowMap()[tableIter->c_str()] = tableRows;
            }

            mGameIdSet.insert(row->getUInt64("game_id"));
        }

        HistoryTableDefinitions::const_iterator tableDefIter;

        for (tableIter = tableBegin; tableIter != tableEnd; ++tableIter)
        {
            tableDefIter = historyTableDefinitions.find(tableIter->c_str());
            if (tableDefIter == historyTableDefinitions.end())
            {
                tableDefIter = builtInTableDefinitions.find(tableIter->c_str());
                if (tableDefIter == builtInTableDefinitions.end())
                {
                    continue;
                }
            }

            if (!retrieveRowsFromDbByGameId(conn, tableDefIter))
            {                      
                return GAMEHISTORY_ERR_INVALID_QUERY;
            }
        }
    }

    return ERR_OK;
}

} // GameReporting
} // Blaze

