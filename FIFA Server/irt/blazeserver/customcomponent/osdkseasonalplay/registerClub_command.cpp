/*************************************************************************************************/
/*!
    \file   registerClub_command.cpp

    $Header$
    $Change$
    $DateTime$

    \attention
    (c) Electronic Arts Inc. 2012
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RegisterClubCommand

    Register a club in the season

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "osdkseasonalplay/rpc/osdkseasonalplayslave/registerclub_stub.h"

// global includes

// framework includes
#include "framework/connection/selector.h"
#include "framework/database/dbscheduler.h"

// osdkseasonalplay includes
#include "osdkseasonalplayslaveimpl.h"
#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"

namespace Blaze
{
    namespace OSDKSeasonalPlay
    {

        class RegisterClubCommand : public RegisterClubCommandStub  
        {
        public:
            RegisterClubCommand(Message* message, RegisterClubRequest* request, OSDKSeasonalPlaySlaveImpl* componentImpl)
                : RegisterClubCommandStub(message,request), mComponent(componentImpl)
            { }

            virtual ~RegisterClubCommand() { }

        /* Private methods *******************************************************************************/
        private:

            RegisterClubCommandStub::Errors execute()
            {
                TRACE_LOG("[RegisterClubCommand:" << this << "].execute()");

                Blaze::BlazeRpcError error = Blaze::ERR_OK;
				error = mComponent->registerClub(
					mRequest.getClubId(),
					mRequest.getLeagueId());
			
				return RegisterClubCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            OSDKSeasonalPlaySlaveImpl* mComponent;
        };


        // static factory method impl
        DEFINE_REGISTERCLUB_CREATE()

    } // OSDKSeasonalPlay
} // Blaze
