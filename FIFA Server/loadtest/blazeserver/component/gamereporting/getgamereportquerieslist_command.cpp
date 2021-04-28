/*************************************************************************************************/
/*!
    \file   getgamereportquerieslist_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetGameReportQueriesList

    Retrieves game reports allowing the client to explicitly specify query name and variables.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

// gamereporting includes
#include "gamereporting/tdf/gamehistory.h"
#include "gamereportingslaveimpl.h"
#include "gamereporting/rpc/gamereportingslave/getgamereportquerieslist_stub.h"
#include "gamehistoryconfig.h"


namespace Blaze
{
namespace GameReporting
{

class GetGameReportQueriesListCommand : public GetGameReportQueriesListCommandStub
{
public:
    GetGameReportQueriesListCommand(Message* message, GameReportingSlaveImpl* componentImpl);
    ~GetGameReportQueriesListCommand() override {}

private:
    GameReportingSlaveImpl *mComponent;

    GetGameReportQueriesListCommandStub::Errors execute() override;

};
DEFINE_GETGAMEREPORTQUERIESLIST_CREATE()

GetGameReportQueriesListCommand::GetGameReportQueriesListCommand(Message* message, GameReportingSlaveImpl* componentImpl)
: GetGameReportQueriesListCommandStub(message)
{
    mComponent = componentImpl;
}

GetGameReportQueriesListCommandStub::Errors GetGameReportQueriesListCommand::execute()
{
    //Check if we trust this user
    if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_GET_GAMEREPORT_INFORMATION))
    {
        WARN_LOG("[GetGameReportQueriesListCommandStub].execute: User [" << gCurrentUserSession->getBlazeId() << "] attempted to get report info, no permission!");
        return ERR_AUTHORIZATION_REQUIRED;
    }

    mResponse.getQueries().reserve(mComponent->getGameReportViewsCollection().getGameReportViewConfigMap()->size());

    GameReportQueryConfigMap::const_iterator iter, end;
    iter = (mComponent->getGameReportQueriesCollection().getGameReportQueryConfigMap())->begin();
    end = (mComponent->getGameReportQueriesCollection().getGameReportQueryConfigMap())->end();

    for (; iter != end; ++iter)
    {
        GameReportQuery* query = mResponse.getQueries().pull_back();
        (iter->second->getGameReportQuery())->copyInto(*query);
    }

    return ERR_OK;
}

} // GameReporting
} // Blaze

