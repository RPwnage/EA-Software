/*************************************************************************************************/
/*!
    \file   membermetadataupdated.cpp


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
    \brief onMemberMetadataUpdated

    Custom processing upon member metadata updated.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::onMemberMetadataUpdated(ClubsDatabase * clubsDb, 
        const ClubId clubId, const ClubMember* member)
{
    return Blaze::ERR_OK;
}
} // Clubs
} // Blaze
