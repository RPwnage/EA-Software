/*************************************************************************************************/
/*!
    \file   registerclub_command..cpp

    $Header: //gosdev/games/FIFA/2013/Gen3/DEV/blazeserver/3.x/customcomponent/osdkseasonalplay/updatetournamentresult_command.cpp#3 $
    $Change: 367571 $
    $DateTime: 2012/03/15 15:45:43 $

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RegisterClubCommand

    Register the club into fifa cups

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "fifacups/rpc/fifacupsslave/registerclub_stub.h"
// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"
#include "framework/database/dbscheduler.h"

// osdkseasonalplay includes
#include "fifacupsslaveimpl.h"
#include "fifacups/tdf/fifacupstypes.h"
#include "fifacups/fifacupsdb.h"

namespace Blaze
{
    namespace FifaCups
    {

        class RegsiterClubCommand : public RegisterClubCommandStub  
        {
        public:
            RegsiterClubCommand(Message* message, RegisterClubRequest* request, FifaCupsSlaveImpl* componentImpl)
                : RegisterClubCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~RegsiterClubCommand() { }

        /* Private methods *******************************************************************************/
        private:

            RegisterClubCommandStub::Errors execute()
            {
                TRACE_LOG("[RegsiterClubCommandStub:" << this << "].execute()");
                Blaze::BlazeRpcError error = Blaze::ERR_OK;

				Blaze::FifaCups::MemberId fifaCupsMemberId = static_cast<Blaze::FifaCups::MemberId>(mRequest.getMemberId());
				Blaze::FifaCups::LeagueId leagueId = mRequest.getLeagueId();

				error = mComponent->registerClub(fifaCupsMemberId, leagueId);
                
				return RegsiterClubCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            FifaCupsSlaveImpl* mComponent;
        };


        // static factory method impl
        RegisterClubCommandStub* RegisterClubCommandStub::create(Message* message, RegisterClubRequest* request, FifaCupsSlave* componentImpl)
        {
            return BLAZE_NEW RegsiterClubCommand(message, request, static_cast<FifaCupsSlaveImpl*>(componentImpl));
        }

    } // OSDKSeasonalPlay
} // Blaze
