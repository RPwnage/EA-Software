/*************************************************************************************************/
/*!
    \file   gamehistoryconfig.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GameHistoryConfig

    Encapsulates all of the game history configuration data.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"

// global includes
#include "framework/controller/controller.h"
#include "framework/userset/userset.h"

// gamereporting includes
#include "gamereporting/tdf/gamehistory.h"

#include "gamehistoryconfig.h"
#include "gamereporting/gamehistbootstrap.h"

// stat includes
#include "stats/statscommontypes.h"

namespace Blaze
{
namespace GameReporting
{

GameReportQueryConfig::GameReportQueryConfig(const GameReportQuery& query, const GameType* gameType)
{
    query.copyInto(mQuery);

    mNumOfVars = 0;

    StringHashSet tableReferences;
    eastl::string filterList;
    eastl::string otherTableRefs; // other tables than <gametype>_gamekey (comma-separated)
    eastl::string joinCondition;

    const HistoryTableDefinitions& historyTableDefinitions = gameType->getHistoryTableDefinitions();
    const HistoryTableDefinitions& builtInTableDefinitions = gameType->getBuiltInTableDefinitions();

    // construct column list
    GameReportQuery::GameReportColumnKeyList::const_iterator columnKeyIter, columnKeyBegin, columnKeyEnd;
    columnKeyIter = columnKeyBegin = mQuery.getColumnKeyList().begin();
    columnKeyEnd = mQuery.getColumnKeyList().end();

    for (; columnKeyIter != columnKeyEnd; ++columnKeyIter)
    {
        StringHashSet *columnSet;

        if (mRequestedTables.find((*columnKeyIter)->getTable()) == mRequestedTables.end())
        {
            mRequestedTables.insert((*columnKeyIter)->getTable());
            columnSet = BLAZE_NEW StringHashSet();
            mColumnSetByTable[(*columnKeyIter)->getTable()] = columnSet;
        }
        else
        {
            columnSet = mColumnSetByTable.find((*columnKeyIter)->getTable())->second;
        }
        columnSet->insert((*columnKeyIter)->getAttributeName());
    }

    if (mRequestedTables.empty())
    {
        HistoryTableDefinitions::const_iterator tableDefIter, tableDefEnd;
        tableDefIter = historyTableDefinitions.begin();
        tableDefEnd = historyTableDefinitions.end();

        for (; tableDefIter != tableDefEnd; ++tableDefIter)
        {
            mRequestedTables.insert(tableDefIter->first.c_str());
        }

        tableDefIter = builtInTableDefinitions.begin();
        tableDefEnd = builtInTableDefinitions.end();

        for (; tableDefIter != tableDefEnd; ++tableDefIter)
        {
            mRequestedTables.insert(tableDefIter->first.c_str());
        }
    }

    // find out what tables need to be included in the first query
    GameReportFilterList::const_iterator filterIter, filterBegin, filterEnd;
    filterIter = filterBegin = mQuery.getFilterList().begin();
    filterEnd = mQuery.getFilterList().end();

    // the filter string will be added to the WHERE clause
    bool useWhereExists = false; // using "where exists/sub-select" instead of "select distinct" may be optimal for certain expressions
    for (; filterIter != filterEnd; ++filterIter)
    {
        if (tableReferences.find((*filterIter)->getTable()) == tableReferences.end())
            tableReferences.insert((*filterIter)->getTable());

        // the *hint* for using "where exists/sub-select" or "select distinct" is checking for "not in" expression (as per use-case in GOSOPS-183820)
        if (blaze_stristr((*filterIter)->getExpression(), "not in"))
            useWhereExists = true;

        // construct where clause
        if (!(*filterIter)->getHasVariable())
        {
            // append "AND" only when there are filters written to the filter list before
            if (!filterList.empty())
                filterList.append(" AND ");
            filterList.append((*filterIter)->getExpression());
        }
        else
            mNumOfVars++;
    }

    if (tableReferences.empty())
    {
        // select all columns in all tables
        // pick the first table from the list
        if (historyTableDefinitions.size() > 0)
            tableReferences.insert(historyTableDefinitions.begin()->first.c_str());
    }
    
    StringHashSet::const_iterator tableIter, tableBegin, tableEnd;
    tableIter = tableBegin = tableReferences.begin();
    tableEnd = tableReferences.end();

    char8_t fullTableReference[GameTypeReportConfig::GameHistory::MAX_TABLENAME_LEN * 2];
    blaze_snzprintf(fullTableReference, sizeof(fullTableReference), "%s_gamekey gamekey0", gameType->getGameReportName().c_str());
    blaze_strlwr(fullTableReference);

    if (useWhereExists)
    {
        mQueryStringPrependFilter.sprintf("SELECT gamekey0.game_id, gamekey0.online, gamekey0.flags, gamekey0.flag_reason, UNIX_TIMESTAMP(gamekey0.timestamp) AS timestamp FROM ");
    }
    else
    {
        mQueryStringPrependFilter.sprintf("SELECT DISTINCT %s0.game_id, gamekey0.online, gamekey0.flags, gamekey0.flag_reason, UNIX_TIMESTAMP(gamekey0.timestamp) AS timestamp FROM ", tableBegin->c_str());
    }
    mQueryStringPrependFilter.append(fullTableReference);

    if (mQuery.getFilterList().empty())
    {
        joinCondition.sprintf("gamekey0.timestamp >= IFNULL((SELECT timestamp FROM %s_gamekey ORDER BY timestamp DESC LIMIT 1 OFFSET %u), TIME(0))", 
            gameType->getGameReportName().c_str(), mQuery.getMaxGameReport());
    }

    // construct table references and join conditions
    for (; tableIter != tableEnd; ++tableIter)
    {
        if (blaze_stricmp(tableIter->c_str(), RESERVED_GAMETABLE_NAME) != 0)
        {
            if (!joinCondition.empty())
                joinCondition.append(" AND ");
            joinCondition.append_sprintf("gamekey0.game_id = %s0.game_id", tableIter->c_str());

            if (tableIter != tableBegin)
            {
                if (!joinCondition.empty())
                    joinCondition.append(" AND ");
                joinCondition.append_sprintf("%s0.game_id = %s0.game_id", tableBegin->c_str(), tableIter->c_str());
            }

            blaze_snzprintf(fullTableReference, sizeof(fullTableReference), "%s_hist_%s %s0",
                gameType->getGameReportName().c_str(), tableIter->c_str(), tableIter->c_str());
            blaze_strlwr(fullTableReference);

            if (!otherTableRefs.empty())
                otherTableRefs.append(", ");
            otherTableRefs.append(fullTableReference);
        }
    }

    if (!otherTableRefs.empty())
    {
        if (useWhereExists)
        {
            mQueryStringPrependFilter.append(" WHERE EXISTS (SELECT * FROM ");
        }
        else
        {
            mQueryStringPrependFilter.append(", ");
        }
        mQueryStringPrependFilter.append(otherTableRefs);
    }

    if (!joinCondition.empty() || !mQuery.getFilterList().empty())
    {
        mQueryStringPrependFilter.append(" WHERE ");
        if (!joinCondition.empty())
        {
            mQueryStringPrependFilter.append(joinCondition);
            if (!filterList.empty())
            {
                mQueryStringPrependFilter.append(" AND ");
            }
        }
        mQueryStringPrependFilter.append(filterList);
        if ((!joinCondition.empty() || !filterList.empty()) && mNumOfVars > 0)
        {
            mQueryStringPrependFilter.append(" AND ");
        }
    }

    if (useWhereExists)
    {
        mQueryStringAppendFilter.sprintf("%s ORDER BY gamekey0.timestamp DESC, gamekey0.game_id DESC LIMIT $d;", otherTableRefs.empty() ? "" : ")");
    }
    else
    {
        mQueryStringAppendFilter.sprintf(" ORDER BY %s0.timestamp DESC, %s0.game_id DESC LIMIT $d;", tableBegin->c_str(), tableBegin->c_str());
    }
}

GameReportQueryConfig::~GameReportQueryConfig()
{
    StringHashSetMap::const_iterator iter, end;
    iter = mColumnSetByTable.begin();
    end = mColumnSetByTable.end();
    for (; iter != end; ++iter)
    {
        delete iter->second;
    }
}

const StringHashSet* GameReportQueryConfig::getColumnSetByTable(const char8_t* table) const
{
    StringHashSetMap::const_iterator iter = mColumnSetByTable.find(table);
    if (iter != mColumnSetByTable.end())
    {
        return iter->second;
    }
    return nullptr;
}

bool GameReportQueriesCollection::init(const GameReportingConfig& config, const GameTypeMap& gameTypeMap)
{
    const GameHistoryReportingQueryList& gameHistoryReportingQueryList = config.getGameHistoryReporting().getQueries();

    for(GameHistoryReportingQueryList::const_iterator it = gameHistoryReportingQueryList.begin(); it != gameHistoryReportingQueryList.end(); it++)
    {
        const char8_t* queryName = (*it)->getName();
        TRACE_LOG("[GameReportQueriesCollection].init: Parsing a game report query: " << queryName);

        GameManager::GameReportName typeName((*it)->getTypeName());
        GameTypeMap::const_iterator found = gameTypeMap.find(typeName);

        const GameType* gameType = found->second;
  
        GameReportQuery gameReportQuery;
        gameReportQuery.setName(queryName);
        gameReportQuery.setTypeName(typeName);
        gameReportQuery.setMaxGameReport((*it)->getMaxGamesToReturn());

        const GameHistoryReportingFilterList& gameHistoryReportingFilterList = (*it)->getFilters();
        if (!parseGameReportFilters(gameHistoryReportingFilterList, gameReportQuery.getFilterList(), gameType, true))
        {
            ERR_LOG("[GameReportQueriesCollection].init: Failed parsing a filter in query " << queryName);
            return false;
        }

        const GameHistoryReportingQueryColumnList& gameHistoryReportingQueryColumnList = (*it)->getColumns();
        if (!parseGameReportColumns(gameHistoryReportingQueryColumnList, gameReportQuery.getColumnKeyList(), gameType))
        {
            ERR_LOG("[GameReportQueriesCollection].init: Failed parsing a column in query " << queryName);
            return false;
        }

        GameReportQueryConfig* queryConfig = BLAZE_NEW GameReportQueryConfig(gameReportQuery, gameType);
        mGameReportQueryConfigMap[queryName] = queryConfig;
    }

    return true;
}

bool GameReportQueriesCollection::parseGameReportColumns(const GameHistoryReportingQueryColumnList& gameHistoryReportingQueryColumnList,
                                                         GameReportQuery::GameReportColumnKeyList& columnKeyList,
                                                         const GameType *gameType) const
{
    columnKeyList.reserve(gameHistoryReportingQueryColumnList.size());

    for (GameHistoryReportingQueryColumnList::const_iterator it = gameHistoryReportingQueryColumnList.begin(); it != gameHistoryReportingQueryColumnList.end(); it++)
    {
        GameReportColumnKey* columnKey = columnKeyList.pull_back();
        columnKey->setAttributeName((*it)->getName());
        columnKey->setTable((*it)->getTable());
        columnKey->setIndex((*it)->getIndex());

        if (!validateGameReportColumnKey(*columnKey, gameType))
        {
            columnKeyList.release();
            return false;
        }
    }

    return true;
}

void GameReportQueriesCollection::clearGameReportQueryConfigMap()
{
    GameReportQueryConfigMap::iterator iter, end;
    iter = mGameReportQueryConfigMap.begin();
    end = mGameReportQueryConfigMap.end();
    for (; iter != end; ++iter)
    {
        delete iter->second;
    }
    mGameReportQueryConfigMap.clear();
}

const GameReportQueryConfig *GameReportQueriesCollection::getGameReportQueryConfig(const char8_t* name) const
{
    GameReportQueryConfigMap::const_iterator iter = mGameReportQueryConfigMap.find(name);
    if (iter == mGameReportQueryConfigMap.end())
    {
        return nullptr;
    }
    return iter->second;
}

GameReportViewConfig::GameReportViewConfig(const GameReportView& view, const GameType* gameType)
    : mMaxVar(0), mMaxIndex(0)
{
    view.copyInto(mView);

    const char8_t* gameReportName = view.getViewInfo().getTypeName();
    const char8_t* tmpTableAlias = "tmp";

    uint32_t index0MaxVar = 0, index1MaxVar = 0;
    ColumnSet attribTypeSet[2];

    eastl::string selectedColList;
    eastl::string index0ColList;
    eastl::string tableReferences;
    eastl::string joinCondition;
    eastl::string whereClause;
    eastl::string secondTableReferences;
    eastl::string secondWhereClause;
    eastl::string customFilter;
    eastl::string secondCustomFilter;
    eastl::string keyColumn;

    // construct a string with a list of selected columns
    if (!view.getColumns().empty())
    {
        GameReportView::GameReportColumnList::const_iterator colIter, colFirst, colEnd;
        colIter = colFirst = view.getColumns().begin();
        colEnd = view.getColumns().end();

        for (; colIter != colEnd; ++colIter)
        {
            char8_t colAlias[MAX_NAME_LENGTH];
            uint32_t index = (*colIter)->getKey().getIndex();
            const char8_t* table = (*colIter)->getKey().getTable();
            const char8_t* attributeName = (*colIter)->getKey().getAttributeName();

            // special case to deal with returning unix timestamp 
            if (blaze_strcmp(attributeName, "timestamp") == 0)
            {
                blaze_snzprintf(colAlias, sizeof(colAlias), "UNIX_TIMESTAMP(`%s%d`.`%s`)", table,
                    index, attributeName);
            }
            else
            {
                blaze_snzprintf(colAlias, sizeof(colAlias), "`%s%d`.`%s`", table,
                    index, attributeName);
            }

            if ((*colIter)->getKey().getIndex() == 0)
            {
                index0ColList.append(", ");
                index0ColList.append(colAlias);
                if (blaze_strcmp(attributeName, "timestamp") == 0)
                {
                    blaze_snzprintf(colAlias, sizeof(colAlias), "UNIX_TIMESTAMP(`%s`.`%s`)", tmpTableAlias,
                        attributeName);
                }
                else
                {
                    blaze_snzprintf(colAlias, sizeof(colAlias), "`%s`.`%s`", tmpTableAlias, attributeName);
                }
            }

            selectedColList.append(colAlias);
            selectedColList.append(", ");

            // keep track of the max index used in any columns
            if (index > mMaxIndex)
            {
                mMaxIndex = index;
            }

            attribTypeSet[index].insert(table);
        }
        selectedColList.erase(selectedColList.end()-2);
    }
    else
    {
        HistoryTableDefinitions::const_iterator tableDefIter, tableEnd;
        tableDefIter = gameType->getHistoryTableDefinitions().begin();
        tableEnd = gameType->getHistoryTableDefinitions().end();

        selectedColList.append_sprintf("UNIX_TIMESTAMP(%s.timestamp)", tmpTableAlias);

        for (; tableDefIter != tableEnd; ++tableDefIter)
        {
            const char8_t* table = tableDefIter->first;

            HistoryTableDefinition::ColumnList::const_iterator colIter, colEnd;
            colIter = tableDefIter->second->getColumnList().begin();
            colEnd = tableDefIter->second->getColumnList().end();

            for (; colIter != colEnd; ++colIter)
            {
                char8_t colAlias[MAX_NAME_LENGTH];

                selectedColList.append(", ");

                // special case to deal with returning unix timestamp 
                if (blaze_strcmp(colIter->first.c_str(), "timestamp") != 0)
                {
                    blaze_snzprintf(colAlias, sizeof(colAlias), "`%s0`.`%s`", table, colIter->first.c_str());
                }

                index0ColList.append(", ");
                index0ColList.append(colAlias);
                if (blaze_strcmp(colIter->first.c_str(), "timestamp") != 0)
                {
                    blaze_snzprintf(colAlias, sizeof(colAlias), "`%s`.`%s`", tmpTableAlias, colIter->first.c_str());
                }

                selectedColList.append(colAlias);

                attribTypeSet[0].insert(table);
            }
        }
    }

    // construct the strings from the defined filters for attributes with index 0 and index 1
    GameReportFilterList::const_iterator fIter, fEnd;
    fIter = view.getFilterList().begin();
    fEnd = view.getFilterList().end();

    for (; fIter != fEnd; ++fIter)
    {
        attribTypeSet[(*fIter)->getIndex()].insert((*fIter)->getTable());

        if (!(*fIter)->getHasVariable())
        {
            if ((*fIter)->getIndex() == 0)
            {
                if (!customFilter.empty())
                {
                    customFilter.append(" AND ");
                }
                customFilter.append((*fIter)->getExpression());
            }
            else
            {
                if (!secondCustomFilter.empty())
                {
                    secondCustomFilter.append(" AND ");
                }
                secondCustomFilter.append((*fIter)->getExpression());
            }
        }
        else
        {
            ((*fIter)->getIndex() == 0) ? ++index0MaxVar : ++index1MaxVar;
        }

        // keep track of the max index used in any filters
        if ((*fIter)->getIndex() > mMaxIndex)
        {
            mMaxIndex = (*fIter)->getIndex();
        }
    }

    mMaxVar = index0MaxVar + index1MaxVar;
    joinCondition.append(" USING(game_id, timestamp)");
    tableReferences[0] = '\0';
    ColumnSet::const_iterator atIter, atBegin, atEnd;
    atIter = atBegin = attribTypeSet[0].begin();
    atEnd = attribTypeSet[0].end();
    for (; atIter != atEnd; ++atIter)
    {
        char8_t tableNameWithAlias[MAX_NAME_LENGTH*2];
        if (blaze_stricmp(*atIter, RESERVED_GAMETABLE_NAME) != 0)
        {
            blaze_snzprintf(tableNameWithAlias, sizeof(tableNameWithAlias), "`%s_hist_%s` `%s0`", gameReportName, *atIter, *atIter);
        }
        else
        {
            blaze_snzprintf(tableNameWithAlias, sizeof(tableNameWithAlias), "`%s_%s` `%s0`", gameReportName, *atIter, *atIter);
        }
        blaze_strlwr(tableNameWithAlias);

        if (atIter != atBegin)
            tableReferences.append(" JOIN ");
        tableReferences.append(tableNameWithAlias);
        if (atIter != atBegin)
            tableReferences.append(joinCondition);
    }

    atIter = attribTypeSet[1].begin();
    atEnd = attribTypeSet[1].end();

    for (; atIter != atEnd; ++atIter)
    {
        char8_t tableName[MAX_NAME_LENGTH*2];
        eastl::string diffCondition;

        blaze_snzprintf(tableName, sizeof(tableName), "`%s_hist_%s` `%s1`", gameReportName, *atIter, *atIter);
        blaze_strlwr(tableName);

        secondTableReferences.append("JOIN ");
        secondTableReferences.append(tableName);
        secondTableReferences.append(joinCondition);

        if (attribTypeSet[0].find(*atIter) != attribTypeSet[0].end())
        {
            bool found = true;

            HistoryTableDefinitions::const_iterator tableDefIter = gameType->getHistoryTableDefinitions().find(*atIter);
            if (tableDefIter == gameType->getHistoryTableDefinitions().end())
            {
                // check if this is a built-in table name
                tableDefIter = gameType->getBuiltInTableDefinitions().find(*atIter);
                if (tableDefIter == gameType->getHistoryTableDefinitions().end())
                {
                    found = false;
                }
            }

            if (found)
            {
                HistoryTableDefinition::PrimaryKeyList::const_iterator keyIter, keyBegin, keyEnd;
                keyIter = keyBegin = tableDefIter->second->getPrimaryKeyList().begin();
                keyEnd = tableDefIter->second->getPrimaryKeyList().end();
                for (; keyIter != keyEnd; ++keyIter)
                {
                    if (keyIter != keyBegin)
                        diffCondition.append(" AND ");
                    diffCondition.append_sprintf("%s.%s<>%s1.%s",
                        tmpTableAlias, keyIter->c_str(), *atIter, keyIter->c_str());
                }
            }

            if (!diffCondition.empty())
            {
                if (secondWhereClause.empty())
                {
                    secondWhereClause.append("WHERE ");
                }
                else
                {
                    secondWhereClause.append(" AND ");
                }

                secondWhereClause.append(diffCondition);
            }
        }
    }

    // include WHERE if there are filters defined for attributes with index 0
    if (index0MaxVar > 0 || customFilter[0] != '\0')
    {
        whereClause.append("WHERE ");

        if (customFilter[0] != '\0')
        {
            whereClause.append(customFilter);
            if (index0MaxVar > 0)
            {
                whereClause.append(" AND ");
            }
        }
    }

    // include WHERE if there are filters defined for attributes with index 1
    if (!secondCustomFilter.empty())
    {
        secondWhereClause.append(" AND ");
        secondWhereClause.append(secondCustomFilter);
    }

    // extract the first primary key string to be used in GROUP BY
    if (blaze_strcmp(view.getRowTypeName(), "game") == 0)
    {
         keyColumn.append("game_id");
    }
    else
    {
        HistoryTableDefinitions::const_iterator tableDefIter = gameType->getHistoryTableDefinitions().find(view.getRowTypeName());
        if (!tableDefIter->second->getPrimaryKeyList().empty())
        {
            HistoryTableDefinition::PrimaryKeyList::const_iterator keyIter;
            keyIter = tableDefIter->second->getPrimaryKeyList().begin();
            keyColumn.append(keyIter->c_str());
        }
    }

    if (mMaxIndex == 0)
    {
        mQueryStringPrependIndex0Filter.sprintf("SELECT %s0.%s%s FROM %s %s", 
            (blaze_strcmp(view.getRowTypeName(), "game") == 0) ? *atBegin : view.getRowTypeName(), 
            keyColumn.c_str(), index0ColList.c_str(), tableReferences.c_str(), whereClause.c_str());

        mQueryStringAppendFilter.sprintf(
            " GROUP BY %s ORDER BY %s0.timestamp DESC, %s0.game_id DESC LIMIT $d;", 
            keyColumn.c_str(), *atBegin, *atBegin);
    }
    else
    {
        mQueryStringPrependIndex0Filter.sprintf(
            "SELECT %s.%s, %s FROM (SELECT * FROM %s %s",
            tmpTableAlias, keyColumn.c_str(), selectedColList.c_str(), tableReferences.c_str(), whereClause.c_str());
        mQueryStringPrependIndex1Filter.sprintf(
            " ORDER BY %s0.timestamp DESC, %s0.game_id DESC LIMIT $d) AS %s %s %s ", *atBegin, *atBegin,
            tmpTableAlias, secondTableReferences.c_str(), secondWhereClause.c_str());

		if (view.getSkipGrouping())
		{
			mQueryStringAppendFilter.sprintf(
				" ORDER BY %s.timestamp DESC, %s.game_id DESC LIMIT $d;", tmpTableAlias, tmpTableAlias);
		}
		else
		{
			mQueryStringAppendFilter.sprintf(
				" GROUP BY %s ORDER BY %s.timestamp DESC, %s.game_id DESC LIMIT $d;", keyColumn.c_str(),
				tmpTableAlias, tmpTableAlias);
		}
    }
}

GameReportViewConfig::~GameReportViewConfig()
{
}

bool GameReportViewsCollection::init(const GameReportingConfig& config, const GameTypeMap& gameTypeMap)
{
    const GameHistoryReportingViewList& gameHistoryReportingViewList = config.getGameHistoryReporting().getViews();
    
    for(GameHistoryReportingViewList::const_iterator it = gameHistoryReportingViewList.begin(); it != gameHistoryReportingViewList.end(); it++)
    {
        const char8_t* viewName = (*it)->getName();
        TRACE_LOG("[GameReportViewsCollection].init: Parsing a game history view: " << viewName);

        const char8_t* desc = (*it)->getDesc();
        const char8_t* metadata = (*it)->getMetadata();

        GameManager::GameReportName typeName((*it)->getTypeName());
        GameTypeMap::const_iterator found = gameTypeMap.find(typeName);

        const GameType* gameType = found->second;

        uint32_t maxRows = (*it)->getMaxRowsToReturn();

        const char8_t* rowAttributeType = (*it)->getRowAttributeType();
		bool skipGrouping = (*it)->getSkipGrouping();

        GameReportView view;
        view.getViewInfo().setName(viewName);
        view.getViewInfo().setTypeName(typeName);
        view.getViewInfo().setMetadata(metadata);
        view.getViewInfo().setDesc(desc);
        view.setRowTypeName(rowAttributeType);
        view.setMaxGames(maxRows);
		view.setSkipGrouping(skipGrouping);

        const GameHistoryReportingFilterList& gameHistoryReportingFilterList = (*it)->getFilters();
        if (!parseGameReportFilters(gameHistoryReportingFilterList, view.getFilterList(), gameType, false))
        {
            ERR_LOG("[GameReportViewsCollection].init - Error: invalid filter for game report view " << viewName);
            return false;
        }

        const GameHistoryReportingViewColumnList& gameHistoryReportingViewColumnList = (*it)->getColumns();
        if (!parseGameReportColumns(gameHistoryReportingViewColumnList, view.getColumns(), gameType))
        {
            return false;
        }

        GameReportViewConfig* viewConfig = BLAZE_NEW GameReportViewConfig(view, gameType);
        mGameReportViewConfigMap[viewName] = viewConfig;
        GameReportViewInfo* viewInfo = mGameReportViewInfoList.pull_back();
        view.getViewInfo().copyInto(*viewInfo);
    }

    return true;
}

bool GameReportViewsCollection::parseGameReportColumns(const GameHistoryReportingViewColumnList& gameHistoryReportingViewColumnList, GameReportView::GameReportColumnList &columnList, const GameType* gameType) const
{
    columnList.reserve(gameHistoryReportingViewColumnList.size());

    for (GameHistoryReportingViewColumnList::const_iterator it = gameHistoryReportingViewColumnList.begin(); it != gameHistoryReportingViewColumnList.end(); it++)
    {
        GameReportColumn* column = columnList.pull_back();
        column->getKey().setAttributeName((*it)->getName());
        column->getKey().setTable((*it)->getTable());
        column->getKey().setIndex((*it)->getIndex());

        if (!validateGameReportColumnKey(column->getKey(), gameType))
        {
            columnList.release();
            return false;
        }

        EA::TDF::ObjectType entityType;
        const char8_t* entityTypeName = (*it)->getEntityType();
        if (entityTypeName != nullptr)
        {
            entityType = BlazeRpcComponentDb::getBlazeObjectTypeByName(entityTypeName);
        }
        else
        {
            entityType = EA::TDF::OBJECT_TYPE_INVALID;
        }

        const char8_t* shortDesc = (*it)->getShortDesc();
        const char8_t* longDesc = (*it)->getLongDesc();
        const char8_t* type = (*it)->getType();
        const char8_t* kind = (*it)->getKind();
        const char8_t* metadata = (*it)->getMetadata();
        const char8_t* unknownValue = (*it)->getUnknownValue();
        const char8_t* userCoreIdent = (*it)->getUserCoreIdentName();

        static const char8_t* TYPE_INT = "int";
        static const char8_t* TYPE_FLOAT = "float";
        static const char8_t* TYPE_STRING = "string";
        Stats::StatType typeEnum = Stats::STAT_TYPE_INT;
        if (blaze_strncmp(type, TYPE_INT, sizeof(TYPE_INT)) == 0)
        {
            typeEnum = Stats::STAT_TYPE_INT;
        }
        else if (blaze_strncmp(type, TYPE_FLOAT, sizeof(TYPE_FLOAT)) == 0)
        {
            typeEnum = Stats::STAT_TYPE_FLOAT;
        }
        else if (blaze_strncmp(type, TYPE_STRING, sizeof(TYPE_STRING)) == 0)
        {
            typeEnum = Stats::STAT_TYPE_STRING;
        }
        else
        {
            typeEnum = Stats::STAT_TYPE_INT;
        }

        const char8_t* format = (*it)->getFormat();
        const int32_t typeSize = 4; //support int32
        char8_t recreateFormat[Blaze::Stats::STATS_FORMAT_LENGTH];
        if (!Stats::validateFormat(typeEnum, format, typeSize, recreateFormat, sizeof(recreateFormat)))
        {
            if (typeEnum == Blaze::Stats::STAT_TYPE_INT)
            {
                format = "%d";
            }
            else if (typeEnum == Blaze::Stats::STAT_TYPE_FLOAT)
            {
                format = "%.2f";
            }
            else if (typeEnum == Blaze::Stats::STAT_TYPE_STRING)
            {
                format = "%s";
            }
        }
        else
        {
            //point to recreated format
            format = recreateFormat;
        }

        column->setEntityType(entityType);
        column->setDesc(longDesc);
        column->setFormat(format);
        column->setKind(kind);
        column->setMetadata(metadata);
        column->setShortDesc(shortDesc);
        column->setType(typeEnum);
        column->setUnknownValue(unknownValue);
        column->setUserCoreIdentName(userCoreIdent);
    }

    return true;
}

void GameReportViewsCollection::clearGameReportViewConfigMap()
{
    GameReportViewConfigMap::iterator iter, end;
    iter = mGameReportViewConfigMap.begin();
    end = mGameReportViewConfigMap.end();
    for (;iter != end; ++iter)
    {
        delete iter->second;
    }
    mGameReportViewConfigMap.clear();
}

const GameReportViewConfig *GameReportViewsCollection::getGameReportViewConfig(const char8_t* name) const
{
    GameReportViewConfigMap::const_iterator iter = mGameReportViewConfigMap.find(name);
    if (iter == mGameReportViewConfigMap.end())
    {
        return nullptr;
    }
    return iter->second;
}


bool findGameHistoryTable(const GameTypeReportConfig& report, const char8_t* tableName, const GameTypeReportConfig::GameHistory* &gameHistory, const char8_t* columnName)
{
    for (GameTypeReportConfig::GameHistoryList::const_iterator it = report.getGameHistory().begin(); it != report.getGameHistory().end(); ++it)
    {
        if (tableName == nullptr)
            return true;
        else
        {
            if (blaze_strcmp(tableName, (*it)->getTable()) == 0)
            {
                gameHistory = (*it);
                if (columnName == nullptr)
                {
                    return true;
                }
                GameTypeReportConfig::ReportValueByAttributeMap::const_iterator itr = gameHistory->getColumns().find(columnName);
                if (itr != gameHistory->getColumns().end() ||
                    blaze_stricmp(columnName, "game_id") == 0 || blaze_stricmp(columnName, "timestamp") == 0)
                {
                    return true;
                }
            }
        }
    }

    for (GameTypeReportConfig::SubreportsMap::const_iterator it = report.getSubreports().begin(); it != report.getSubreports().end(); ++it)
    {
        if (findGameHistoryTable(*(it->second), tableName, gameHistory, columnName))
            return true;
    }
    return false;
}

void validateAttributeConfig(const char8_t* table, const char8_t* name, const char8_t* gameReportName, const GameTypeConfig& gameType, ConfigureValidationErrors& validationErrors)
{
    const GameTypeReportConfig::GameHistory* gameHistory = nullptr;
    bool foundTableColumn = findGameHistoryTable(gameType.getReport(), table, gameHistory, name);

    if (gameHistory == nullptr)
    {
        if (blaze_stricmp(table, "participant") != 0 && blaze_stricmp(table, "gamekey") != 0)
        {
            eastl::string msg;
            msg.sprintf("[GameHistoryConfig].validateAttributeConfig: Invalid table '%s' in game type '%s'",
                table, gameReportName);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
        else if (blaze_stricmp(table, "participant") == 0 && blaze_stricmp(name, "game_id") != 0 && 
                 blaze_stricmp(name, "timestamp") != 0 && blaze_stricmp(name, "entity_id") != 0 )
        {
            eastl::string msg;
            msg.sprintf("[GameHistoryConfig].validateAttributeConfig: '%s' is not a column of table 'participant' in game type '%s'",
                name, gameReportName);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
    }
    else
    {
        if (!foundTableColumn)
        {
            eastl::string msg;
            msg.sprintf("[GameHistoryConfig].validateAttributeConfig: '%s' is not a column of table '%s' in game type '%s'", 
                name, table, gameReportName);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
    }
}

bool validateAttribute(const char8_t* table, const char8_t* name, const GameType* gameType)
{
    HistoryTableDefinitions::const_iterator found = 
        gameType->getHistoryTableDefinitions().find(table);

    if (found == gameType->getHistoryTableDefinitions().end())
    {
        found = gameType->getBuiltInTableDefinitions().find(table);

        if (found == gameType->getBuiltInTableDefinitions().end())
        {
            found = gameType->getGamekeyTableDefinitions().find(table);

            if (found == gameType->getGamekeyTableDefinitions().end())
            {
                ERR_LOG("[GameHistoryConfig].validateAttribute: Invalid column name '" << name << "' in game type '" << gameType->getGameReportName().c_str() << "'");
                return false;
            }
        }
    }

    const HistoryTableDefinition *tableDefinition = found->second;

    if (tableDefinition->getColumnList().find(name) == tableDefinition->getColumnList().end() &&
        blaze_strcmp(name, "game_id") != 0 && blaze_strcmp(name, "timestamp") != 0)
    {
        ERR_LOG("Error: '" << name << "' is not an column of table '" << table << "' in game type '" << gameType->getGameReportName().c_str() << "'");
        return false;
    }

    return true;
}

bool validateExpression(const char8_t* expression)
{
    // make sure that the expression includes a '?' character
    uint16_t i, colRef;
    size_t expLen = strlen(expression);
    for (colRef = 0, i = 0; i < expLen; i++)
    {
        if (expression[i] == '?')
        {
            colRef++;
        }
        if (colRef > 1)
        {
            return false;
        }
    }
    return (colRef == 1);
}

void validateGameHistoryReportingColumn(const char8_t* colName, const char8_t* colTable, uint16_t colIndex, const char8_t* gameReportName, const GameTypeConfig& gameType, ConfigureValidationErrors& validationErrors)
{
    if ((blaze_strcmp(colName, "") == 0) || (*colName == '_'))
    {
        eastl::string msg;
        msg.sprintf("[GameHistoryConfig].validateGameHistoryReportingQueryColumn: Attribute name must be present and cannot start with \'_\'");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }

    validateAttributeConfig(colTable, colName, gameReportName, gameType, validationErrors);
}

bool validateGameReportColumnKey(const GameReportColumnKey& key, const GameType* gameType)
{
    if ((blaze_strcmp(key.getAttributeName(), "") == 0) || (*key.getAttributeName() == '_'))
    {
        ERR_LOG("[GameHistoryConfig].validateGameReportColumnKey: Column name must be present and cannot start with \'_\'");
        return false;
    }

    if (!validateAttribute(key.getTable(), key.getAttributeName(), gameType))
    {
        return false;
    }

    return true;
}

void validateGameHistoryReporingColumnUserCoreIdentName(const char8_t* entityType, const char8_t* userCoreIdentName, ConfigureValidationErrors& validationErrors)
{
    if (blaze_stricmp(userCoreIdentName, "") != 0)
    {
        if (blaze_stricmp(entityType, "usersessions.user") == 0)
        {
            if (blaze_stricmp(userCoreIdentName, "externalId") != 0 && blaze_stricmp(userCoreIdentName, "personaNamespace") != 0) 
            {
                eastl::string msg;
                msg.sprintf("[GameHistoryConfig].validateGameHistoryReporingColumnUserCoreIdentName: userCoreIdentName must match 'externalId' or 'personaNamespace'.");
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }
        }
        else
        {
            eastl::string msg;
            msg.sprintf("[GameHistoryConfig].validateGameHistoryReporingColumnUserCoreIdentName: EntityType must be present as 'usersessions.user'.");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
    }    
}

void validateGameHistoryReportingFilter(const GameHistoryReportingFilter& filter, const char8_t* gameReportName, const GameTypeConfig& gameType, bool onlyIndexZero, ConfigureValidationErrors& validationErrors)
{
    if (blaze_strcmp(filter.getTable(), "") == 0)
    {
        eastl::string msg;
        msg.sprintf("[GameHistoryConfig].validateGameHistoryReportingFilter: Attribute type not defined");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }

    if ((blaze_strcmp(filter.getName(), "") == 0) || (*filter.getName() == '_'))
    {
        eastl::string msg;
        msg.sprintf("[GameHistoryConfig].validateGameHistoryReportingFilter: Attribute name must be present and cannot start with \'_\'.");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }

    validateAttributeConfig(filter.getTable(), filter.getName(), gameReportName, gameType, validationErrors);

    if (onlyIndexZero && filter.getIndex() > 0)
    {
        eastl::string msg;
        msg.sprintf("[GameHistoryConfig].validateGameHistoryReportingFilter: Index in one of the filters in query %s must be 0.", 
            filter.getName());
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str()); 
    }

    if (filter.getIndex() > 1)
    {
        eastl::string msg;
        msg.sprintf("[GameHistoryConfig].validateGameHistoryReportingFilter: Index in one of the filters in query %s must be either 0 or 1.", 
            filter.getName());
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());  
    }

    if (!validateExpression(filter.getExpression()))
    {
        eastl::string msg;
        msg.sprintf("[GameHistoryConfig].validateGameHistoryReportingFilter: Missing '?' in one of the filters in query %s.", filter.getName());
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str()); 
    }
}

bool validateGameReportFilter(GameReportFilter& filter, const GameType* gameType, bool onlyIndexZero)
{
    if (blaze_strcmp(filter.getTable(), "") == 0)
    {
        ERR_LOG("[GameHistoryConfig].validateGameReportFilter: Attribute type not defined");
        return false;
    }

    if ((blaze_strcmp(filter.getAttributeName(), "") == 0) || (*filter.getAttributeName() == '_'))
    {
        ERR_LOG("[GameHistoryConfig].validateGameReportFilter: Attribute name must be present and cannot start with \'_\'.");
        return false;
    }

    if (!validateAttribute(filter.getTable(), filter.getAttributeName(), gameType))
    {
        return false;
    }

    if (onlyIndexZero && filter.getIndex() > 0)
    {
        ERR_LOG("[GameHistoryConfig].validateGameReportFilter: Index in one of the filters in query " << filter.getAttributeName() << " must be 0.");
        return false;
    }

    if (filter.getIndex() > 1)
    {
        ERR_LOG("[GameHistoryConfig].validateGameReportFilter: Index in one of the filters in query " << filter.getAttributeName() << " must be either 0 or 1.");
        return false;
    }

    if (!validateExpression(filter.getExpression()))
    {
        ERR_LOG("[GameHistoryConfig].validateGameReportFilter: Missing '?' in one of the filters in query " << filter.getAttributeName() << ".");
        return false;
    }

    if ((filter.getExpression())[0] != '\0')
    {
        char8_t columnName[MAX_NAME_LENGTH];
        eastl::string str = filter.getExpression();

        if (blaze_stricmp(filter.getTable(), RESERVED_GAMETABLE_NAME) != 0)
        {
            blaze_snzprintf(columnName, sizeof(columnName), "`%s%d`.`%s`", filter.getTable(),
                filter.getIndex(), filter.getAttributeName());
        }
        else
        {
            blaze_snzprintf(columnName, sizeof(columnName), "gamekey0.`%s`", filter.getAttributeName());
        }

        // replace '?' in expression with the actual attribute name
        eastl::string::size_type n = str.find('?');
        if (n != eastl::string::npos)
        {
            str.replace(n, 1, columnName);
        }

        // if $S or $s exists in expression string, then the client needs to provide the value in the request
        // to fill in the variable to complete constructing the query
        //bool hasVerbatimStringVariable = (str.find("$S", 0) != eastl::string::npos);
        //bool hasVariable = (hasVerbatimStringVariable || str.find("$s", 0) != eastl::string::npos);
		
		// In the Urraca, the format $S is changed to $s 
		// https://developer.ea.com/pages/viewpage.action?spaceKey=blaze&title=Urraca+Database+Query+Verbatim+Strings
		bool hasVerbatimStringVariable = (str.find("$s", 0) != eastl::string::npos);
		bool hasVariable = hasVerbatimStringVariable;

        if (hasVerbatimStringVariable)
        {
			bool allowVerbatimStringFormatSpecifier = false;
#ifdef BLAZE_ENABLE_DEPRECATED_VERBATIM_STRING_FORMAT_SPECIFIER_IN_DATABASE_QUERY // uncommented it in \dev\blazeserver\scripts\deprecations.xml
            allowVerbatimStringFormatSpecifier = true;
#endif

            if (!allowVerbatimStringFormatSpecifier)
            {
                ERR_LOG("[GameHistoryConfig].validateGameReportFilter: Unallowed usage of deprecated verbatim string format specifier in query " << filter.getAttributeName() << ".");
                return false;
            }
        }

        filter.setHasVariable(hasVariable);
        filter.setExpression(str.c_str());
    }

    return true;
}

bool parseGameReportFilters(const GameHistoryReportingFilterList& gameHistoryReportingFilterList, GameReportFilterList& filterList,
                            const GameType *gameType, bool onlyIndexZero)
{
    filterList.reserve(gameHistoryReportingFilterList.size());
 
    for(GameHistoryReportingFilterList::const_iterator it = gameHistoryReportingFilterList.begin(); it != gameHistoryReportingFilterList.end(); it++)
    {
        GameReportFilter* filter = filterList.pull_back();
        filter->setAttributeName((*it)->getName());
        filter->setTable((*it)->getTable());
        filter->setIndex((*it)->getIndex());
        filter->setExpression((*it)->getExpression());
        filter->setEntityType(BlazeRpcComponentDb::getBlazeObjectTypeByName((*it)->getEntityType()));

        if (!validateGameReportFilter(*filter, gameType, onlyIndexZero))
        {
            filterList.release();
            return false;
        }
    }

    return true;
}

bool validateQueryVarValue(const char8_t *queryVarValue)
{
    // fail validation if the value of a query variable is an empty string
    eastl::string value(queryVarValue);
    value.ltrim();
    return (!value.empty());
}

const eastl::string expandBlazeObjectId(const EA::TDF::ObjectId& userSetId)
{
    BlazeRpcError error = Blaze::ERR_OK;
    uint32_t size = 0;
    eastl::string expandedList = "";
 
    BlazeIdList ids;
    error = gUserSetManager->getUserBlazeIds(userSetId, ids);

    if (error == Blaze::ERR_OK)
    {
        for (BlazeIdList::const_iterator idItr = ids.begin(); idItr != ids.end() && size < MAX_LIMIT; ++idItr)
        {
            BlazeId id = *idItr;
            expandedList.append_sprintf(", %" PRId64, id);
            size++;
        }
        expandedList.erase(0, 2);
    }
 
    return expandedList;
}

///////////////////////////////////////////////////////////////////////////////

bool GameHistoryConfigParser::init(const GameReportingConfig& config, const GameTypeCollection& gameTypes)
{  
    updateConfigReferences(config);
    return onConfigureGameHistory(config, gameTypes);
}

void GameHistoryConfigParser::updateConfigReferences(const GameReportingConfig& config)
{
    mConfig = &config.getGameHistory();
}

void GameHistoryConfigParser::clear()
{
    mGameReportQueriesCollection.clearGameReportQueryConfigMap();
    mGameReportViewsCollection.clearGameReportViewConfigMap();
}

bool GameHistoryConfigParser::onConfigureGameHistory(const GameReportingConfig& config, const GameTypeCollection& gameTypes)
{
    if (mGameReportQueriesCollection.init(config, gameTypes.getGameTypeMap()) == false)
    {
        ERR_LOG("[GameHistoryConfigParser].configure: game history queries configuration invalid.");
        return false;
    }

    if (mGameReportViewsCollection.init(config, gameTypes.getGameTypeMap()) == false)
    {
        ERR_LOG("[GameHistoryConfigParser].configure: game history views configuration invalid.");
        return false;
    }

    return true;
}

} // GameReporting
} // Blaze

