/*************************************************************************************************/
/*!
	\file   fetchLoadOuts_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FetchLoadOutsCommand

    Fetch load outs for one user

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "vprogrowthunlocks/rpc/vprogrowthunlocksslave/fetchloadouts_stub.h"

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
        class FetchLoadOutsCommand : public FetchLoadOutsCommandStub  
        {
        public:
            FetchLoadOutsCommand(Message* message, FetchLoadOutsRequest* request, VProGrowthUnlocksSlaveImpl* componentImpl)
                : FetchLoadOutsCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~FetchLoadOutsCommand() { }

        /* Private methods *******************************************************************************/
        private:

            FetchLoadOutsCommand::Errors execute()
            {
				TRACE_LOG("[FetchLoadOutsCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				TRACE_LOG("[FetchLoadOutsCommand].FetchLoadOutsCommand. id count = "<< mRequest.getUserIdList().size() <<" ");
				mComponent->fetchLoadOuts(mRequest.getUserIdList(), mResponse.getLoadOutList(), error);					
				
				if (Blaze::ERR_OK != error)
				{						
					error = Blaze::VPROGROWTHUNLOCKS_ERR_USER_NOT_FOUND;
					mResponse.setSuccess(false);
				}
				else
				{						
					mResponse.setSuccess(true);					
				}

                return FetchLoadOutsCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            VProGrowthUnlocksSlaveImpl* mComponent;
        };

        // static factory method impl
        FetchLoadOutsCommandStub* FetchLoadOutsCommandStub::create(Message* message, FetchLoadOutsRequest* request, VProGrowthUnlocksSlave* componentImpl)
        {
            return BLAZE_NEW FetchLoadOutsCommand(message, request, static_cast<VProGrowthUnlocksSlaveImpl*>(componentImpl));
        }

    } // VProGrowthUnlocks
} // Blaze
