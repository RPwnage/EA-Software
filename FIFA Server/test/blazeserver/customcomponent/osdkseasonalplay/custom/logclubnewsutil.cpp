/*************************************************************************************************/
/*!
    \file   logclubnewsutil.cpp

    $Header: //gosdev/games/FIFA/2022/GenX/test/blazeserver/customcomponent/osdkseasonalplay/custom/logclubnewsutil.cpp#1 $
    $Change: 1636281 $
    $DateTime: 2020/12/03 11:59:08 $

    \attention
        (c) Electronic Arts 2010. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"

#include "osdkseasonalplay/custom/logclubnewsutil.h"

// clubs includes
#include "component/clubs/rpc/clubsslave.h"
#include "component/clubs/clubsdbconnector.h"


namespace Blaze
{
namespace OSDKSeasonalPlay
{

/*************************************************************************************************/
/*!
    \brief  logNews

    Logs clubs news for the passed in member. The member must be a club.

*/
/*************************************************************************************************/
BlazeRpcError LogClubNewsUtil::logNews(MemberId memberId, const char8_t* message, const char8_t* param1)
{
    BlazeRpcError error = ERR_OK;

    // 
    // get the clubs slave
    //
    Clubs::ClubsSlave* clubsComponent
        = static_cast<Clubs::ClubsSlave*>(
        gController->getComponent(Clubs::ClubsSlave::COMPONENT_ID,false));
    if (NULL == clubsComponent)
    {
        ERR_LOG("[LogClubNewsUtil].logNews. Unable to get the clubs slave component. Unable to log club news");
        return Blaze::ERR_SYSTEM;
    }

    Clubs::PostNewsRequest req;
    Clubs::ClubId cid = static_cast<Clubs::ClubId>(memberId);
    req.setClubId(cid); 
    req.getClubNews().setType(Clubs::CLUBS_SERVER_GENERATED_NEWS);
    req.getClubNews().setStringId(message);
    req.getClubNews().setParamList(param1);

    // to invoke this RPC will require authentication
    UserSession::pushSuperUserPrivilege();
    error = clubsComponent->postNews(req);
    UserSession::popSuperUserPrivilege();

    if (Blaze::ERR_OK != error)
    {
        ERR_LOG("[LogClubNewsUtil].logNews. Clubs logEvent error. Unable to log club news. Error = "
                << Blaze::ErrorHelp::getErrorName(error));
    }

    return error;
}


}
}
