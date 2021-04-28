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
#include "gamereporting/rpc/gamereportingslave/getgamereportcolumninfo_stub.h"
#include "gamehistoryconfig.h"


namespace Blaze
{
namespace GameReporting
{

class GetGameReportColumnInfoCommand : public GetGameReportColumnInfoCommandStub
{
public:
    GetGameReportColumnInfoCommand(Message* message, GetGameReportColumnInfo* request, GameReportingSlaveImpl* componentImpl);
    ~GetGameReportColumnInfoCommand() override {}

private:
    GameReportingSlaveImpl* mComponent;
    GetGameReportColumnInfoCommandStub::Errors execute() override;

};
DEFINE_GETGAMEREPORTCOLUMNINFO_CREATE()

GetGameReportColumnInfoCommand::GetGameReportColumnInfoCommand(Message* message, GetGameReportColumnInfo* request, GameReportingSlaveImpl* componentImpl)
    : GetGameReportColumnInfoCommandStub(message, request),
    mComponent(componentImpl)
{
}

GetGameReportColumnInfoCommandStub::Errors GetGameReportColumnInfoCommand::execute()
{
    //Check if we trust this user
    if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_GET_GAMEREPORT_INFORMATION))
    {
        WARN_LOG("[GetGameReportColumnInfoCommandStub].execute: User [" << gCurrentUserSession->getBlazeId() << "] attempted to get report info [" << mRequest.getName() << "], no permission!");
        return ERR_AUTHORIZATION_REQUIRED;
    }

    const GameReportViewConfig* viewConfig = mComponent->getGameReportViewConfig(mRequest.getName());

    if (viewConfig == nullptr)
    {
        return GAMEHISTORY_ERR_UNKNOWN_VIEW;
    }

    const GameReportView::GameReportColumnList& columnList = viewConfig->getGameReportView()->getColumns();
    GameReportView::GameReportColumnList::const_iterator iter = columnList.begin();
    GameReportView::GameReportColumnList::const_iterator end = columnList.end();

    for ( ; iter != end; ++iter )
    {
        GameReportColumnInfo* column = mResponse.getColumnInfoList().pull_back();
        const GameReportColumn *configColumn = (*iter);

        // We should consider consolidating classes with GameReportColumn to enable us to use copyInto here.
        configColumn->getKey().copyInto(column->getKey());
        column->setEntityType(configColumn->getEntityType());
        column->setShortDesc(mComponent->getLocalizedString(configColumn->getShortDesc()).c_str());
        column->setDesc(mComponent->getLocalizedString(configColumn->getDesc()).c_str());
        column->setFormat(configColumn->getFormat());
        column->setMetadata(configColumn->getMetadata());
        column->setType(configColumn->getType());
        column->setKind(configColumn->getKind());
        column->setUnknownValue(configColumn->getUnknownValue());
    }

    return ERR_OK;
}

} // GameReporting
} // Blaze

