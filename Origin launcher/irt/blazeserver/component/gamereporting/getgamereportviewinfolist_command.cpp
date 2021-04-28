/*************************************************************************************************/
/*!
    \file   getgamereportviewinfolist_command.cpp


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

#include "framework/util/localization.h"

// gamereporting includes
#include "gamereporting/tdf/gamereporting.h"
#include "gamereporting/tdf/gamehistory.h"
#include "gamereportingslaveimpl.h"
#include "gamereporting/rpc/gamereportingslave/getgamereportviewinfolist_stub.h"
#include "gamereportingconfig.h"
#include "gamehistoryconfig.h"


namespace Blaze
{
namespace GameReporting
{

class GetGameReportViewInfoListCommand : public GetGameReportViewInfoListCommandStub
{
public:
    GetGameReportViewInfoListCommand(Message* message, GameReportingSlaveImpl* componentImpl);
    ~GetGameReportViewInfoListCommand() override {}

private:
    GameReportingSlaveImpl *mComponent;

    GetGameReportViewInfoListCommandStub::Errors execute() override;
};

DEFINE_GETGAMEREPORTVIEWINFOLIST_CREATE()

GetGameReportViewInfoListCommand::GetGameReportViewInfoListCommand(Message* message, GameReportingSlaveImpl* componentImpl)
    : GetGameReportViewInfoListCommandStub(message)
{
    mComponent = componentImpl;
}


GetGameReportViewInfoListCommandStub::Errors GetGameReportViewInfoListCommand::execute()
{
    //Check if we trust this user
    if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_GET_GAMEREPORT_INFORMATION))
    {
        WARN_LOG("[GetGameReportViewInfoListCommandStub].execute: User [" << gCurrentUserSession->getBlazeId() << "] attempted to get report view info, no permission!");
        return ERR_AUTHORIZATION_REQUIRED;
    }

    GameReportViewInfosList::GameReportViewInfoList::const_iterator iter, end;
    iter = (mComponent->getGameReportViewsCollection().getGameReportViewInfoList())->begin();
    end = (mComponent->getGameReportViewsCollection().getGameReportViewInfoList())->end();

    mResponse.getViewInfo().reserve(mComponent->getGameReportViewsCollection().getGameReportViewInfoList()->size());

    for (; iter != end; ++iter)
    {
        GameReportViewInfo* viewInfo = mResponse.getViewInfo().pull_back();
        (*iter)->copyInto(*viewInfo);
        viewInfo->setDesc(gLocalization->localize(viewInfo->getDesc(), gCurrentUserSession->getSessionLocale()));
    }

    return ERR_OK;
}

} // GameReporting
} // Blaze

