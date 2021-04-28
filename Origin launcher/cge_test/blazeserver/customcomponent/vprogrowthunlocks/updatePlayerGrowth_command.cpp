/*************************************************************************************************/
/*!
	\file   updatePlayerGrowth_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdatePlayerGrowthCommand

    Update player growth.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "vprogrowthunlocks/rpc/vprogrowthunlocksslave/updateplayergrowth_stub.h"

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

        class UpdatePlayerGrowthCommand : public UpdatePlayerGrowthCommandStub  
        {
        public:
            UpdatePlayerGrowthCommand(Message* message, UpdatePlayerGrowthRequest* request, VProGrowthUnlocksSlaveImpl* componentImpl)
                : UpdatePlayerGrowthCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~UpdatePlayerGrowthCommand() { }

        /* Private methods *******************************************************************************/
        private:

            UpdatePlayerGrowthCommand::Errors execute()
            {
				TRACE_LOG("[UpdatePlayerGrowthCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				mComponent->updatePlayerGrowth(mRequest.getUserId(), mRequest.getMatchEventsMap(), error);
				
				if (Blaze::ERR_OK != error)
				{						
					error = Blaze::VPROGROWTHUNLOCKS_ERR_UNKNOWN;
					mResponse.setSuccess(false);
				}
				else
				{						
					mResponse.setSuccess(true);					
				}

				return UpdatePlayerGrowthCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            VProGrowthUnlocksSlaveImpl* mComponent;
        };


        // static factory method impl
        UpdatePlayerGrowthCommandStub* UpdatePlayerGrowthCommandStub::create(Message* message, UpdatePlayerGrowthRequest* request, VProGrowthUnlocksSlave* componentImpl)
        {
            return BLAZE_NEW UpdatePlayerGrowthCommand(message, request, static_cast<VProGrowthUnlocksSlaveImpl*>(componentImpl));
        }

    } // VProGrowthUnlocks
} // Blaze
