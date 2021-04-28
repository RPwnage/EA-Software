/*************************************************************************************************/
/*!
	\file   updateLoadOutsPeripherals_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateLoadOutsPeripheralsCommand

    Update load outs peripherials for one user

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "vprogrowthunlocks/rpc/vprogrowthunlocksslave/updateloadoutsperipherals_stub.h"

// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"
#include "framework/database/dbscheduler.h"

// vprogrowthunlocks includes
#include "vprogrowthunlocksslaveimpl.h"
#include "vprogrowthunlocks/tdf/vprogrowthunlockstypes.h"

namespace Blaze
{
    namespace VProGrowthUnlocks
    {
        class UpdateLoadOutsPeripheralsCommand : public UpdateLoadOutsPeripheralsCommandStub  
        {
        public:
            UpdateLoadOutsPeripheralsCommand(Message* message, UpdateLoadOutsPeripheralsRequest* request, VProGrowthUnlocksSlaveImpl* componentImpl)
                : UpdateLoadOutsPeripheralsCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~UpdateLoadOutsPeripheralsCommand() { }

        /* Private methods *******************************************************************************/
        private:

            UpdateLoadOutsPeripheralsCommand::Errors execute()
            {
				TRACE_LOG("[UpdateLoadOutsPeripheralsCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				TRACE_LOG("[UpdateLoadOutsPeripheralsCommand].UpdateLoadOutsPeripheralsCommand. owner blaze Id = "<< mRequest.getUserId() <<" ");
				mComponent->updateLoadOutsPeripherals(mRequest.getUserId(), mRequest.getLoadOutList(), error);
				
				if (Blaze::ERR_OK != error)
				{						
					mResponse.setSuccess(false);
				}
				else
				{						
					mResponse.setSuccess(true);					
				}

                return UpdateLoadOutsPeripheralsCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            VProGrowthUnlocksSlaveImpl* mComponent;
        };

        // static factory method impl
        UpdateLoadOutsPeripheralsCommandStub* UpdateLoadOutsPeripheralsCommandStub::create(Message* message, UpdateLoadOutsPeripheralsRequest* request, VProGrowthUnlocksSlave* componentImpl)
        {
            return BLAZE_NEW UpdateLoadOutsPeripheralsCommand(message, request, static_cast<VProGrowthUnlocksSlaveImpl*>(componentImpl));
        }

    } // VProGrowthUnlocks
} // Blaze
