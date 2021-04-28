/*************************************************************************************************/
/*!
	\file   getxprewardsfrommatch_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetXPRewardsFromMatchCommand

    Get XP Rewards From Match

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "vprogrowthunlocks/rpc/vprogrowthunlocksslave/getxprewardsfrommatch_stub.h"

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

        class GetXPRewardsFromMatchCommand : public GetXPRewardsFromMatchCommandStub  
        {
        public:
            GetXPRewardsFromMatchCommand(Message* message, GetXPRewardsFromMatchRequest* request, VProGrowthUnlocksSlaveImpl* componentImpl)
                : GetXPRewardsFromMatchCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~GetXPRewardsFromMatchCommand() { }

        /* Private methods *******************************************************************************/
        private:

            GetXPRewardsFromMatchCommand::Errors execute()
            {
				TRACE_LOG("[GetXPRewardsFromMatchCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				mComponent->calculateXPRewardsFromMatch(mRequest.getUserId(), mRequest.getMatchingRating(), mRequest.getMatchEventsMap(), mRequest.getAttributeTypesMap(), mResponse.getAttributeXPMap(), error);					
				
				if (Blaze::ERR_OK != error)
				{						
					error = Blaze::VPROGROWTHUNLOCKS_ERR_UNKNOWN;
					mResponse.setSuccess(false);
				}
				else
				{						
					mResponse.setSuccess(true);					
				}

				return GetXPRewardsFromMatchCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            VProGrowthUnlocksSlaveImpl* mComponent;
        };


        // static factory method impl
        GetXPRewardsFromMatchCommandStub* GetXPRewardsFromMatchCommandStub::create(Message* message, GetXPRewardsFromMatchRequest* request, VProGrowthUnlocksSlave* componentImpl)
        {
            return BLAZE_NEW GetXPRewardsFromMatchCommand(message, request, static_cast<VProGrowthUnlocksSlaveImpl*>(componentImpl));
        }

    } // VProGrowthUnlocks
} // Blaze
