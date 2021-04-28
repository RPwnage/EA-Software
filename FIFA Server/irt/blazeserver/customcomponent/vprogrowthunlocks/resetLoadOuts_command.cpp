/*************************************************************************************************/
/*!
	\file   resetLoadOuts_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ResetLoadOutsCommand

    Reset load outs for one user

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "vprogrowthunlocks/rpc/vprogrowthunlocksslave/resetloadouts_stub.h"

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
        class ResetLoadOutsCommand : public ResetLoadOutsCommandStub  
        {
        public:
            ResetLoadOutsCommand(Message* message, ResetLoadOutsRequest* request, VProGrowthUnlocksSlaveImpl* componentImpl)
                : ResetLoadOutsCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~ResetLoadOutsCommand() { }

        /* Private methods *******************************************************************************/
        private:

            ResetLoadOutsCommand::Errors execute()
            {
				TRACE_LOG("[ResetLoadOutsCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;
				int32_t loadOutId = mRequest.getLoadOutId();

				TRACE_LOG("[ResetLoadOutsCommand].ResetLoadOutsCommand. owner blaze Id = "<< mRequest.getUserId() <<", loadOut Id = " << loadOutId);
				if (loadOutId >= 0)
				{
					mComponent->resetSingleLoadOut(mRequest.getUserId(), loadOutId, error);	
				}
				else
				{
					mComponent->resetLoadOuts(mRequest.getUserId(), error);		
				}
				
				if (Blaze::ERR_OK != error)
				{						
					error = Blaze::VPROGROWTHUNLOCKS_ERR_USER_NOT_FOUND;
					mResponse.setSuccess(false);
				}
				else
				{						
					mResponse.setSuccess(true);					
				}

                return ResetLoadOutsCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            VProGrowthUnlocksSlaveImpl* mComponent;
        };

        // static factory method impl
        ResetLoadOutsCommandStub* ResetLoadOutsCommandStub::create(Message* message, ResetLoadOutsRequest* request, VProGrowthUnlocksSlave* componentImpl)
        {
            return BLAZE_NEW ResetLoadOutsCommand(message, request, static_cast<VProGrowthUnlocksSlaveImpl*>(componentImpl));
        }

    } // VProGrowthUnlocks
} // Blaze
