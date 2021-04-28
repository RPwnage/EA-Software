/*************************************************************************************************/
/*!
    \file   getclubscomponentsettings_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetClubsComponentSettingsCommand

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
#include "clubs/rpc/clubsslave/getclubscomponentsettings_stub.h"
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

class GetClubsComponentSettingsCommand : public GetClubsComponentSettingsCommandStub, private ClubsCommandBase
{
public:
    GetClubsComponentSettingsCommand(Message* message, ClubsSlaveImpl* componentImpl)
        : GetClubsComponentSettingsCommandStub(message),
        ClubsCommandBase(componentImpl)
    {
    }

    ~GetClubsComponentSettingsCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    GetClubsComponentSettingsCommandStub::Errors execute() override
    {
        TRACE_LOG("[GetClubsComponentSettings] start");

        return  static_cast<Errors>(mComponent->getClubsComponentSettings(mResponse));
    }

};
// static factory method impl
DEFINE_GETCLUBSCOMPONENTSETTINGS_CREATE()

} // Clubs
} // Blaze
