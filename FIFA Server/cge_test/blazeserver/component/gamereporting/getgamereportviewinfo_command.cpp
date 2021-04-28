/*************************************************************************************************/
/*!
    \file   getgamereportviewinfo_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetGameReportViewInfo

    Retrieves game report view info allowing the client to explicitly specify view name.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/util/localization.h"

// gamereporting includes
#include "gamereporting/tdf/gamehistory.h"
#include "gamereportingslaveimpl.h"
#include "gamereporting/rpc/gamereportingslave/getgamereportviewinfo_stub.h"
#include "gamehistoryconfig.h"

namespace Blaze
{
namespace GameReporting
{

class GetGameReportViewInfoCommand : public GetGameReportViewInfoCommandStub
{
public:
    GetGameReportViewInfoCommand(Message* message, GetGameReportViewInfo* request, GameReportingSlaveImpl* componentImpl);
    ~GetGameReportViewInfoCommand() override {}

private:
    GameReportingSlaveImpl* mComponent;
    GetGameReportViewInfoCommandStub::Errors execute() override;

};
DEFINE_GETGAMEREPORTVIEWINFO_CREATE()

GetGameReportViewInfoCommand::GetGameReportViewInfoCommand(Message* message, GetGameReportViewInfo* request, GameReportingSlaveImpl* componentImpl)
    : GetGameReportViewInfoCommandStub(message, request)
{
    mComponent = componentImpl;
}


GetGameReportViewInfoCommandStub::Errors GetGameReportViewInfoCommand::execute()
{
    //Check if we trust this user
    if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_GET_GAMEREPORT_INFORMATION))
    {
        WARN_LOG("[GetGameReportViewInfoCommandStub].execute: User [" << gCurrentUserSession->getBlazeId() << "] attempted to get reports info ["<< mRequest.getName() << "], no permission!");
        return ERR_AUTHORIZATION_REQUIRED;
    }

    const GameReportViewConfig* viewConfig = mComponent->getGameReportViewConfig(mRequest.getName());

    if (viewConfig == nullptr)
    {
        return GAMEHISTORY_ERR_UNKNOWN_VIEW;
    }

    viewConfig->getGameReportView()->getViewInfo().copyInto(mResponse);
    mResponse.setDesc(gLocalization->localize(mResponse.getDesc(), gCurrentUserSession->getSessionLocale()));

    return ERR_OK;
}

} // GameReporting
} // Blaze

