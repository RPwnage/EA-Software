/*************************************************************************************************/
/*!
    \file   logclubnewsutil.h

    $Header: //gosdev/games/FIFA/2022/GenX/irt/blazeserver/customcomponent/osdkseasonalplay/custom/logclubnewsutil.h#1 $
    $Change: 1646537 $
    $DateTime: 2021/02/01 04:49:23 $

    \attention
        (c) Electronic Arts. All Rights Reserved
*/
/*************************************************************************************************/
#ifndef BLAZE_OSDKSEASONALPLAY_LOGCLUBNEWSUTIL
#define BLAZE_OSDKSEASONALPLAY_LOGCLUBNEWSUTIL

#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"
#include "clubs/tdf/clubs_base.h"

namespace Blaze
{
namespace OSDKSeasonalPlay
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
} //namespace OSDKSeasonalPlay
} //namespace Blaze

#endif //BLAZE_OSDKSEASONALPLAY_LOGCLUBNEWSUTIL
