/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ClubsCommandBase

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"

// clubs includes
#include "clubs/clubscommandbase.h"
#include "clubs/clubsslaveimpl.h"

namespace Blaze
{
namespace Clubs
{
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/
ClubsCommandBase::ClubsCommandBase(ClubsSlaveImpl* component)
    : mComponent(component)
{
    TRACE_LOG("[ClubsCommandBase].ClubsCommandBase() - constructing clubs command base");
}

ClubsCommandBase::~ClubsCommandBase()
{
    TRACE_LOG("[ClubsCommandBase].~ClubsCommandBase() - destroying clubs command base");
}

} // Clubs
} // Blaze

