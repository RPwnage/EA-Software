/*************************************************************************************************/
/*!
	\file   updateLoadOutUnlocks_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateLoadOutUnlocksCommand

    Update load out unlocks for one user

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "vprogrowthunlocks/rpc/vprogrowthunlocksslave/updateloadoutunlocks_stub.h"

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
        class UpdateLoadOutUnlocksCommand : public UpdateLoadOutUnlocksCommandStub  
        {
        public:
            UpdateLoadOutUnlocksCommand(Message* message, UpdateLoadOutUnlocksRequest* request, VProGrowthUnlocksSlaveImpl* componentImpl)
                : UpdateLoadOutUnlocksCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~UpdateLoadOutUnlocksCommand() { }

        /* Private methods *******************************************************************************/
        private:

            UpdateLoadOutUnlocksCommand::Errors execute()
            {
				TRACE_LOG("[UpdateLoadOutUnlocksCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				TRACE_LOG("[UpdateLoadOutUnlocksCommand].UpdateLoadOutUnlocksCommand. owner blaze Id = "<< mRequest.getUserId() <<" ");
				mComponent->updateLoadOutUnlocks(mRequest.getUserId(), mRequest.getLoadOut(), error);
				
				if (Blaze::ERR_OK != error)
				{						
					mResponse.setSuccess(false);
				}
				else
				{						
					mResponse.setSuccess(true);					
				}

                return UpdateLoadOutUnlocksCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            VProGrowthUnlocksSlaveImpl* mComponent;
        };

        // static factory method impl
        UpdateLoadOutUnlocksCommandStub* UpdateLoadOutUnlocksCommandStub::create(Message* message, UpdateLoadOutUnlocksRequest* request, VProGrowthUnlocksSlave* componentImpl)
        {
            return BLAZE_NEW UpdateLoadOutUnlocksCommand(message, request, static_cast<VProGrowthUnlocksSlaveImpl*>(componentImpl));
        }

    } // VProGrowthUnlocks
} // Blaze