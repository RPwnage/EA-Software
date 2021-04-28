/*************************************************************************************************/
/*!
    \file   getclubscomponentinfo_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetClubsComponentInfo

    Gets Clubs component information such as total number of clubs

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"

// clubs includes
#include "clubs/rpc/clubsslave/getclubscomponentinfo_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

class GetClubsComponentInfoCommand : public GetClubsComponentInfoCommandStub, private ClubsCommandBase
{
public:
    GetClubsComponentInfoCommand(Message* message, ClubsSlaveImpl* componentImpl)
        : GetClubsComponentInfoCommandStub(message),
        ClubsCommandBase(componentImpl)
    {
    }

    ~GetClubsComponentInfoCommand() override
    {
    }

/* Private methods *******************************************************************************/
private:
    GetClubsComponentInfoCommandStub::Errors execute() override
    {
        TRACE_LOG("[GetClubsComponentInfo] start");

        mComponent->getClubsComponentInfo().copyInto(mResponse.getClubsComponentInfo());
        return commandErrorFromBlazeError(Blaze::ERR_OK);
    }

};
// static factory method impl
DEFINE_GETCLUBSCOMPONENTINFO_CREATE()

} // Clubs
} // Blaze
