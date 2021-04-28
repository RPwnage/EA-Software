/*************************************************************************************************/
/*!
    \file   gamehistoryutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved
*/
/*************************************************************************************************/
#ifndef BLAZE_GAMEREPORTING_GAMEHISTORYUTIL
#define BLAZE_GAMEREPORTING_GAMEHISTORYUTIL

#include "gamereporting/tdf/gamehistory.h"

namespace Blaze
{
namespace GameReporting
{

class GameReportingSlaveImpl;

class GameHistoryUtil
{

public:
    GameHistoryUtil(GameReportingSlaveImpl& component)
        : mComponent(component)
    {
    }

    bool getGameHistory(const char8_t* queryName, int32_t maxGamesChecked,
        QueryVarValuesList& queryVarValues, GameReportsList &gameReportsList);

    void getGameHistoryMatchingValues(const char8_t *table, const Collections::AttributeMap &map,
        const GameReportsList &gameReportsList, GameReportsList &matchedGameReportsList) const;

    void getGameHistoryConsecutiveMatchingValues(const char8_t *table, const Collections::AttributeMap &map,
        const GameReportsList &gameReportsList, GameReportsList &matchedGameReportsList) const;

    GameHistoryUtil& operator=(const GameHistoryUtil&);

private:
    bool compareAttributeMap(const Collections::AttributeMap &mapA, const Collections::AttributeMap &mapB) const;

private:
    GameReportingSlaveImpl& mComponent;

};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_GAMEREPORTING_GAMEHISTORYUTIL
