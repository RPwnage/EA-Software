/*************************************************************************************************/
/*!
    \file   awardupdated.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

// clubs includes
#include "clubsslaveimpl.h"


namespace Blaze
{
namespace Clubs
{

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief onAwardUpdated

    Custom processing upon member added.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::onAwardUpdated(ClubsDatabase * clubsDb, 
        const ClubId clubId, const ClubAwardId awardId)
{
    // FIFA SPECIFIC CODE START
    // DevTrack: 29695 - Production no longer wants "The club has been awarded a new accomplishment message posted" so this function does nothing.
    // The code below is commented out, and only returns ok
    /*
    if (awardId < 90)
    {
        const char8_t *message = LOG_EVENT_AWARD_TROPHY;
        logEvent(clubsDb, clubId, getEventString(message), NEWS_PARAM_NONE, nullptr);
    }
    */
    // FIFA SPECIFIC CODE END
    return Blaze::ERR_OK;
}
} // Clubs
} // Blaze
