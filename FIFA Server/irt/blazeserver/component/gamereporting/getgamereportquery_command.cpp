/*************************************************************************************************/
/*!
    \file   getgamereportquery_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetGameReportQuery

    Retrieves a game report query defined in gamereporting config by query name.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/util/localization.h"

// gamereporting includes
#include "gamereporting/tdf/gamehistory.h"
#include "gamereportingslaveimpl.h"
#include "gamereporting/rpc/gamereportingslave/getgamereportquery_stub.h"
#include "gamehistoryconfig.h"


namespace Blaze
{
namespace GameReporting
{

class GetGameReportQueryCommand : public GetGameReportQueryCommandStub
{
public:
    GetGameReportQueryCommand(Message* message, GetGameReportQuery* request, GameReportingSlaveImpl* componentImpl);
    ~GetGameReportQueryCommand() override {}

private:
    GameReportingSlaveImpl* mComponent;
    GetGameReportQueryCommandStub::Errors execute() override;

};
DEFINE_GETGAMEREPORTQUERY_CREATE()

GetGameReportQueryCommand::GetGameReportQueryCommand(Message* message, GetGameReportQuery* request, GameReportingSlaveImpl* componentImpl)
    : GetGameReportQueryCommandStub(message, request)
{
    mComponent = componentImpl;
}


GetGameReportQueryCommandStub::Errors GetGameReportQueryCommand::execute()
{
    //Check if we trust this user
    if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_GET_GAMEREPORT_INFORMATION))
    {
        WARN_LOG("[GetGameReportQueryCommandStub].execute: User [" << gCurrentUserSession->getBlazeId() << "] attempted to get report info ["<< mRequest.getName() << "], no permission!");
        return ERR_AUTHORIZATION_REQUIRED;
    }

    const GameReportQueryConfig* queryConfig = mComponent->getGameReportQueryConfig(mRequest.getName());

    if (queryConfig == nullptr)
    {
        return GAMEHISTORY_ERR_UNKNOWN_QUERY;
    }

    queryConfig->getGameReportQuery()->copyInto(mResponse);

    return ERR_OK;
}

} // GameReporting
} // Blaze

