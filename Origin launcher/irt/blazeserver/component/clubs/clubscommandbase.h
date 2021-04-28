/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CLUBS_CLUBSCOMMANDBASE_H
#define BLAZE_CLUBS_CLUBSCOMMANDBASE_H

/*** Include files *******************************************************************************/
#include "clubs/clubsslaveimpl.h"
#include "clubs/clubsdbconnector.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Clubs
{

class ClubsSlaveImpl;

class ClubsCommandBase
{
public:
    ClubsCommandBase(ClubsSlaveImpl* component);
    virtual ~ClubsCommandBase();

protected:
    ClubsSlaveImpl* mComponent;
};

} // Clubs
} // Blaze

#endif // BLAZE_CLUBS_CLUBSCOMMANDBASE_H

