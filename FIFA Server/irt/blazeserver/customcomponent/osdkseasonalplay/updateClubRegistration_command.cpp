/*************************************************************************************************/
/*!
    \file   updateclubregistration_command.cpp

    $Header$
    $Change$
    $DateTime$

    \attention
    (c) Electronic Arts Inc. 2012
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateClubRegistrationCommand

    Update a club's registration

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "osdkseasonalplay/rpc/osdkseasonalplayslave/updateclubregistration_stub.h"

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

        class UpdateClubRegistrationCommand : public UpdateClubRegistrationCommandStub  
        {
        public:
            UpdateClubRegistrationCommand(Message* message, UpdateClubRegistrationRequest* request, OSDKSeasonalPlaySlaveImpl* componentImpl)
                : UpdateClubRegistrationCommandStub(message,request), mComponent(componentImpl)
            { }

            virtual ~UpdateClubRegistrationCommand() { }

        /* Private methods *******************************************************************************/
        private:

            UpdateClubRegistrationCommandStub::Errors execute()
            {
                TRACE_LOG("[UpdateClubRegistrationCommand:" << this << "].execute()");

                Blaze::BlazeRpcError error = Blaze::ERR_OK;
				error = mComponent->updateClubRegistration(
					mRequest.getClubId(),
					mRequest.getLeagueId());
			
				return UpdateClubRegistrationCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            OSDKSeasonalPlaySlaveImpl* mComponent;
        };


        // static factory method impl
        DEFINE_UPDATECLUBREGISTRATION_CREATE()

    } // OSDKSeasonalPlay
} // Blaze
