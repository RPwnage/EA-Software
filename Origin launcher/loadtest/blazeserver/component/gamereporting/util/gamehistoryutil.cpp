/*************************************************************************************************/
/*!
    \file   gamehistoryutil.cpp
    \attention
        (c) Electronic Arts. All Rights Reserved
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "gamehistoryutil.h"
#include "gamereportingslaveimpl.h"

namespace Blaze
{
namespace GameReporting
{

/*************************************************************************************************/
/*!
    \brief getGameHistory
        Retrieve game history for a given entity

    \param[in] entityId            - entityId to retrieve game history for, (i.e. player ID, club ID)
    \param[in/out] gameReportsList - game reports found for this user
    \return true if no errors encountered recovering game history
*/
/*************************************************************************************************/
bool GameHistoryUtil::getGameHistory(const char8_t* queryName, int32_t maxGamesChecked,
                                     QueryVarValuesList& queryVarValues, GameReportsList &gameReportsList)
{
    if (queryName == nullptr)
    {
        ERR_LOG("[GameHistoryUtil].getGameHistory(): Game report query not set.");
        return false;
    }

    GetGameReports gameReportRequest;
    gameReportRequest.setMaxGameReport(maxGamesChecked);
    gameReportRequest.setQueryName(queryName);
    queryVarValues.copyInto(gameReportRequest.getQueryVarValues());

    BlazeRpcError err = mComponent.getGameReports(gameReportRequest, gameReportsList);

    if (err != ERR_OK)
    {
        ERR_LOG("[GameHistoryUtil].getGameHistory(): Error retrieving game history query '" << queryName << "' (err=" 
                << (ErrorHelp::getErrorName(err)) << ").");
        return false;
    }

    return true;
}

/*************************************************************************************************/
/*!
    \brief countGameMatchingValues
        Count a specific game attribute's given value versus a single entity opponent

    \param[in] opponentId - entity id to find wins against
    \param[in] reportTypeName - report type name for the given opponentId
    \param[in] lossKey - the player report attribute key for a game loss
    \param[in] lossValue - the value set for the loss key representing a loss
    \param[in] gameReportsList - game reports to search for losses by this player
    \return count of game attribute's value against provided opponent
*/
/*************************************************************************************************/
void GameHistoryUtil::getGameHistoryMatchingValues(const char8_t *table, const Collections::AttributeMap &map,
                                                   const GameReportsList &gameReportsList,
                                                   GameReportsList &matchedGameReportsList) const
{
    const GameReportsList::GameHistoryReportList &reportList = gameReportsList.getGameReportList();

    GameReportsList::GameHistoryReportList::const_iterator grIter = reportList.begin();
    GameReportsList::GameHistoryReportList::const_iterator grEnd = reportList.end();

    for (; grIter != grEnd; ++grIter)
    {
        GameHistoryReport::TableRowMap::const_iterator trIter = (*grIter)->getTableRowMap().find(table);

        if (trIter == (*grIter)->getTableRowMap().end())
        {
            TRACE_LOG("[GameHistoryUtil].getGameHistoryMatchingValues() : Table '" << table << "' is not found in game reports list");
            break;
        }

        const GameHistoryReport::TableRows *tableRows = trIter->second;

        GameHistoryReport::TableRowList::const_iterator rowIter, rowEnd;
        rowIter = tableRows->getTableRowList().begin();
        rowEnd = tableRows->getTableRowList().end();

        for (; rowIter != rowEnd; ++rowIter)
        {
            const Collections::AttributeMap &attribMap = (*rowIter)->getAttributeMap();

            if (compareAttributeMap(map, attribMap))
            {
                matchedGameReportsList.getGameReportList().push_back((*grIter)->clone());
                TRACE_LOG("[GameHistoryUtil].getGameHistoryMatchingValues() : Game history ID " << (*grIter)->getGameHistoryId() << " matches the search criteria");
                break;
            }
        }
    }
}

/*************************************************************************************************/
/*!
    \brief getConsecutiveResults
        Count consecutive results starting from most recent game

    \param[in] entityId        - entityId to find the consecutive results
    \param[in] column          - the column name
    \param[in] value           - the value to match
    \param[in] gameReportsList - game reports to search for the provided attribute key
    \return count of consecutive recent games that match the provided key and value
*/
/*************************************************************************************************/
void GameHistoryUtil::getGameHistoryConsecutiveMatchingValues(const char8_t *table, const Collections::AttributeMap &map,
                                                              const GameReportsList &gameReportsList,
                                                              GameReportsList &matchedGameReportsList) const
{
    const GameReportsList::GameHistoryReportList &reportList = gameReportsList.getGameReportList();

    GameReportsList::GameHistoryReportList::const_iterator grIter = reportList.begin();
    GameReportsList::GameHistoryReportList::const_iterator grEnd = reportList.end();

    for (; grIter != grEnd; ++grIter)
    {
        GameHistoryReport::TableRowMap::const_iterator trIter = (*grIter)->getTableRowMap().find(table);

        if (trIter == (*grIter)->getTableRowMap().end())
        {
            break;
        }

        const GameHistoryReport::TableRows *tableRows = trIter->second;

        GameHistoryReport::TableRowList::const_iterator rowIter, rowEnd;
        rowIter = tableRows->getTableRowList().begin();
        rowEnd = tableRows->getTableRowList().end();

        bool isMatched = false;

        for (; rowIter != rowEnd; ++rowIter)
        {
            const Collections::AttributeMap &attribMap = (*rowIter)->getAttributeMap();

            if (compareAttributeMap(map, attribMap))
            {
                matchedGameReportsList.getGameReportList().push_back((*grIter)->clone());
                isMatched = true;
                break;
            }
        }

        if (!isMatched)
        {
            return;
        }
    }
}

/*************************************************************************************************/
/*!
    \brief compareAttributeMap
        Compare two attribute maps to find out if one map is subset of the other

    \param[in] mapA        - attribute map
    \param[in] mapB        - attribute map
    \return true if the smaller attribute map is a subset of the other attribute map
*/
/*************************************************************************************************/
bool GameHistoryUtil::compareAttributeMap(const Collections::AttributeMap &mapA, const Collections::AttributeMap &mapB) const
{
    Collections::AttributeMap smallerMap, largerMap;

    if (mapA.size() > mapB.size())
    {
        smallerMap.insert(mapB.begin(), mapB.end());
        largerMap.insert(mapA.begin(), mapA.end());
    }
    else
    {
        smallerMap.insert(mapA.begin(), mapA.end());
        largerMap.insert(mapB.begin(), mapB.end());
    }

    Collections::AttributeMap::const_iterator attribIter, attribEnd;
    attribIter = smallerMap.begin();
    attribEnd = smallerMap.end();

    for (; attribIter != attribEnd; ++attribIter)
    {
        Collections::AttributeMap::const_iterator largerAttribIter = largerMap.find(attribIter->first);
        if (largerAttribIter != largerMap.end())
        {
            if (blaze_strcmp(largerAttribIter->second.c_str(), attribIter->second.c_str()) == 0)
            {
                continue;
            }
        }
        return false;
    }

    return true;

}

} //namespace GameReporting
} //namespace Blaze
