/*************************************************************************************************/
/*!
    \file   logevents.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \brief logEvents

    Log events to the database.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"

//club includes
#include "clubsslaveimpl.h"

namespace Blaze
{
namespace Clubs
{

// utility impl
BlazeRpcError ClubsSlaveImpl::logEventAsNews(ClubsDatabase* clubsDb, const ClubsEvent *clubEvent)
{
    TRACE_LOG("[ClubsSlaveImpl].logEventAsNews() - start");

    // ASSUMING conn != nullptr !!!

    ClubNews clubNews;
    clubNews.setAssociateClubId(clubEvent->clubId);

    clubNews.setStringId(clubEvent->message);
    clubNews.setType(CLUBS_SERVER_GENERATED_NEWS);
    eastl::string strBuff;
    
	Blaze::BlazeId contentCreator = 0;
    for(int32_t idx = 0; idx < clubEvent->numArgs; idx++)
    {
        const char8_t* formatStr = "";
        if (idx > 0)
        {
            switch(clubEvent->args[idx].type)
            {
            case NEWS_PARAM_BLAZE_ID: // string version of the BlazeId printed as an int64
                formatStr = "%s,@%s"; // special character to denote blaze id
				contentCreator = EA::StdC::AtoI64(clubEvent->args[idx].value);
                break;
            case NEWS_PARAM_STRING:
                formatStr = "%s,%s";
                break;
            case NEWS_PARAM_INT:
                formatStr = "%s,%d";
                break;
            default:
                break;
            }
            strBuff.sprintf(formatStr, clubNews.getParamList(), clubEvent->args[idx].value);
         }
         else
         {
            switch(clubEvent->args[idx].type)
            {
            case NEWS_PARAM_BLAZE_ID: // string version of the BlazeId printed as an int64
                formatStr = "@%s"; // special character to denote blaze id
				contentCreator = EA::StdC::AtoI64(clubEvent->args[idx].value);
                break;
            case NEWS_PARAM_STRING:
                formatStr = "%s";
                break;
            case NEWS_PARAM_INT:
                formatStr = "%d";
                break;
            default:
                break;
            }
            strBuff.sprintf(formatStr, clubEvent->args[idx].value);
         }
         clubNews.setParamList(strBuff.c_str());
    }
    clubNews.setContentCreator(contentCreator);
    BlazeRpcError error = insertNewsAndDeleteOldest(*clubsDb, clubEvent->clubId, clubNews.getContentCreator(), clubNews);
    if (error != Blaze::ERR_OK)
    {
        ERR_LOG("[ClubsSlaveImpl].logEventAsNews() error (" << ErrorHelp::getErrorName(error) << ") inserting server generated news");
        return(error);
    }
    return(error);
}

} // League
} // Blaze
