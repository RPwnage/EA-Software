/*************************************************************************************************/
/*!
    \file   rivals.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_COMPONENT_CLUB_RIVALS_H
#define BLAZE_COMPONENT_CLUB_RIVALS_H

namespace Blaze
{
namespace Clubs
{

enum ClubRivalStatus
{
    CLUBS_RIVAL_NOT_ELIGIBLE = 0,
    CLUBS_RIVAL_PENDING_ASSIGNMENT,
    CLUBS_RIVAL_ASSIGNED
};

// FIFA SPECIFIC CODE START
enum ClubRivalChallengeType
{
    CLUBS_RIVAL_TYPE_WIN = 0,
    CLUBS_RIVAL_TYPE_SHUTOUT,
    CLUBS_RIVAL_TYPE_WINBY7,
    CLUBS_RIVAL_TYPE_WINBY5,
    CLUBS_RIVAL_TYPES_COUNT     // The number of Rival Challenge Type to randomly assign to rival pairs
};
// FIFA SPECIFIC CODE END

const uint32_t minMemberCountReqForRivals = 2;

} // Clubs
} // Blaze

#endif
