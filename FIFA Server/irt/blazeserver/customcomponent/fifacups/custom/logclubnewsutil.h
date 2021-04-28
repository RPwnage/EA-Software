/*************************************************************************************************/
/*!
    \file   logclubnewsutil.h

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/fifacups/custom/logclubnewsutil.h#1 $
    $Change: 286819 $
    $DateTime: 2011/02/24 16:14:33 $

    \attention
        (c) Electronic Arts. All Rights Reserved
*/
/*************************************************************************************************/
#ifndef BLAZE_FIFACUPS_LOGCLUBNEWSUTIL
#define BLAZE_FIFACUPS_LOGCLUBNEWSUTIL

#include "fifacups/tdf/fifacupstypes.h"
#include "clubs/tdf/clubs_base.h"

namespace Blaze
{
namespace FifaCups
{

class LogClubNewsUtil
{
public:

    /*************************************************************************************************/
    /*!
        \brief logNews

        util function to log club news. The memberId must be the ClubId of a club.
    */
    /*************************************************************************************************/
    BlazeRpcError logNews(MemberId memberId, const char8_t* message, const char8_t* param1 = NULL);

    LogClubNewsUtil() {}
    ~LogClubNewsUtil() {}

};
} //namespace FifaCups
} //namespace Blaze

#endif //BLAZE_FIFACUPS_LOGCLUBNEWSUTIL
