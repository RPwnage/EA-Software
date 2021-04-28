/*************************************************************************************************/
/*!
    \file   maxrivalsreached.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

// clubs includes
#include "clubs/clubsslaveimpl.h"


namespace Blaze
{
namespace Clubs
{

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief onMaxRivalsReached

    Custom processing upon max rival count is reached for club.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::onMaxRivalsReached(ClubsDatabase *clubsDb,
    const ClubId clubId,
    bool &handled)
{
    // we do not handle this event
    // let default actions be executed
    // Blaze's default handle will delete the oldest rival stored in the database.
    // Ping is not doing anything special beside the default actions.
    
    handled = false;
    return Blaze::ERR_OK;
}

} // Clubs
} // Blaze
