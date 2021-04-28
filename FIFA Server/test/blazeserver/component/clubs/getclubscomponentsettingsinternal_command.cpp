/*************************************************************************************************/
/*!
    \file   getclubscomponentsettingsinternal_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetClubsComponentSettingsInternalCommand

    Get the club component config settings

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
#include "clubs/rpc/clubsslave/getclubscomponentsettingsinternal_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

// stats includes
#include "stats/rpc/statsslave.h"
#include "stats/tdf/stats.h"
#include "stats/statscommontypes.h"
#include "stats/statsslaveimpl.h"

namespace Blaze
{
namespace Clubs
{

class GetClubsComponentSettingsInternalCommand : public GetClubsComponentSettingsInternalCommandStub, private ClubsCommandBase
{
public:
    GetClubsComponentSettingsInternalCommand(Message* message, ClubsSlaveImpl* componentImpl)
        : GetClubsComponentSettingsInternalCommandStub(message),
        ClubsCommandBase(componentImpl)
    {
    }

    ~GetClubsComponentSettingsInternalCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    GetClubsComponentSettingsInternalCommand::Errors execute() override
    {
        TRACE_LOG("[GetClubsComponentSettingsInternalCommand] start");

        return  static_cast<Errors>(mComponent->getClubsComponentSettings(mResponse));
    }

};
// static factory method impl
DEFINE_GETCLUBSCOMPONENTSETTINGSINTERNAL_CREATE()

} // Clubs
} // Blaze
