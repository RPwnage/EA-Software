/*************************************************************************************************/
/*!
    \file   logclubnewsutil.cpp

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/fifacups/custom/logclubnewsutil.cpp#1 $
    $Change: 286819 $
    $DateTime: 2011/02/24 16:14:33 $

    \attention
        (c) Electronic Arts 2010. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"

#include "fifacups/custom/logclubnewsutil.h"

// clubs includes
#include "component/clubs/clubsslaveimpl.h"
#include "component/clubs/clubsdbconnector.h"


namespace Blaze
{
namespace FifaCups
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
    Clubs::ClubsSlaveImpl* clubsComponent
        = static_cast<Clubs::ClubsSlaveImpl*>(
        gController->getComponent(Clubs::ClubsSlaveImpl::COMPONENT_ID));
    if (NULL == clubsComponent)
    {
        ERR_LOG("[LogClubNewsUtil].logNews. Unable to get the clubs slave component. Unable to log club news");
        return Blaze::ERR_SYSTEM;
    }

    // get the clubs db connection
    Clubs::ClubsDbConnector dbConn(clubsComponent);
    if (dbConn.acquireDbConnection(false) == NULL)
    {
        ERR_LOG("[LogClubNewsUtil].logNews. Unable to get the clubs database connection. Unable to log club news");
        return Blaze::ERR_SYSTEM;
    }

    Clubs::ClubId clubId = static_cast<Clubs::ClubId>(memberId);
    if (NULL == param1)
    {
        TRACE_LOG("[LogClubNewsUtil].logNews. Logging message for Club "<<clubId <<": "<<message <<" ");
        error = clubsComponent->logEvent(&dbConn.getDb(), clubId, message);
    }
    else
    {
        TRACE_LOG("[LogClubNewsUtil].logNews. Logging message for Club "<<clubId <<": "<<message <<", param="<<param1 <<" ");
        error = clubsComponent->logEvent(&dbConn.getDb(), clubId, message, Clubs::NEWS_PARAM_STRING, param1);
    }

    if (Blaze::ERR_OK != error)
    {
        ERR_LOG("[LogClubNewsUtil].logNews. Clubs logEvent error. Unable to log club news. Error = "<<Blaze::ErrorHelp::getErrorName(error) <<"");
    }

    return error;
}


}
}
