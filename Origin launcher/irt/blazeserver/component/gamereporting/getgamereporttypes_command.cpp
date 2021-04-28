/*************************************************************************************************/
/*!
    \file   getgamereporttypes_command.cpp


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
#include "gamereporting/rpc/gamereportingslave/getgamereporttypes_stub.h"
#include "gamereportingconfig.h"
#include "gamehistoryconfig.h"


namespace Blaze
{
namespace GameReporting
{

class GetGameReportTypesCommand : public GetGameReportTypesCommandStub
{
public:
    GetGameReportTypesCommand(Message* message, GameReportingSlaveImpl* componentImpl);
    ~GetGameReportTypesCommand() override {} 

private:
    GameReportingSlaveImpl *mComponent;
    GetGameReportTypesCommandStub::Errors execute() override;

};
DEFINE_GETGAMEREPORTTYPES_CREATE()

GetGameReportTypesCommand::GetGameReportTypesCommand(Message* message,GameReportingSlaveImpl* componentImpl)
    : GetGameReportTypesCommandStub(message)
{
    mComponent = componentImpl;
}

GetGameReportTypesCommandStub::Errors GetGameReportTypesCommand::execute()
{
    //Check if we trust this user
    if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_GET_GAMEREPORT_INFORMATION))
    {
        WARN_LOG("[GetGameReportTypesCommandStub].execute: User [" << gCurrentUserSession->getBlazeId() << "] attempted to get reports types, no permission!");
        return ERR_AUTHORIZATION_REQUIRED;
    }

    const GameTypeCollection& gameTypeCollection = mComponent->getGameTypeCollection();
    mResponse.getGameReportTypes().reserve(gameTypeCollection.getGameTypeMap().size());

    GameTypeMap::const_iterator iter, end;
    iter = gameTypeCollection.getGameTypeMap().begin();
    end = gameTypeCollection.getGameTypeMap().end();

    for (; iter != end; ++iter)
    {
        const GameType *gameType = iter->second;
        GameReportType* gameReportType = mResponse.getGameReportTypes().pull_back();
        gameReportType->setGameReportName(gameType->getGameReportName());

        const HistoryTableDefinitions& historyTableDefinitions = gameType->getHistoryTableDefinitions();
        gameReportType->getHistoryTables().reserve(historyTableDefinitions.size());

        HistoryTableDefinitions::const_iterator tableIter, tableEnd;
        tableIter = historyTableDefinitions.begin();
        tableEnd = historyTableDefinitions.end();

        for (; tableIter != tableEnd; ++tableIter)
        {
            TableData* data = gameReportType->getHistoryTables().pull_back();
            data->setTable(tableIter->first);

            HistoryTableDefinition *historyTableDefinition = tableIter->second;
            data->getPrimaryKey().reserve(historyTableDefinition->getPrimaryKeyList().size());
            data->getColumns().reserve(historyTableDefinition->getColumnList().size());

            HistoryTableDefinition::PrimaryKeyList::const_iterator pkIter, pkEnd;
            pkIter = historyTableDefinition->getPrimaryKeyList().begin();
            pkEnd = historyTableDefinition->getPrimaryKeyList().end();

            for (; pkIter != pkEnd; ++pkIter)
            {
                data->getPrimaryKey().push_back(pkIter->c_str());
            }

            HistoryTableDefinition::ColumnList::const_iterator columnIter, columnEnd;
            columnIter = historyTableDefinition->getColumnList().begin();
            columnEnd = historyTableDefinition->getColumnList().end();

            for (; columnIter != columnEnd; ++columnIter)
            {
                data->getColumns().push_back(columnIter->first.c_str());
            }
        }
    }

    return GetGameReportTypesCommandStub::ERR_OK;
} 

} // GameReporting
} // Blaze

