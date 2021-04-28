/*************************************************************************************************/
/*!
	\file   debuggrantvpromatchratingxp_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DebugGrantVProMatchRatingXPCommand

    Debug Grant Match Rating XP for the target user

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "vprogrowthunlocks/rpc/vprogrowthunlocksslave/debuggrantvpromatchratingxp_stub.h"

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

        class DebugGrantVProMatchRatingXPCommand : public DebugGrantVProMatchRatingXPCommandStub
        {
        public:
			DebugGrantVProMatchRatingXPCommand(Message* message, DebugGrantVProMatchRatingXPRequest* request, VProGrowthUnlocksSlaveImpl* componentImpl)
                : DebugGrantVProMatchRatingXPCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~DebugGrantVProMatchRatingXPCommand() { }

        /* Private methods *******************************************************************************/
        private:

			DebugGrantVProMatchRatingXPCommand::Errors execute()
            {
				TRACE_LOG("[DebugGrantVProMatchRatingXPCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				TRACE_LOG("[DebugGrantVProMatchRatingXPCommand].DebugGrantVProMatchRatingXPCommand. target blaze Id = "<< mRequest.getUserId() <<" ");
				mComponent->grantMatchRatingXP(mRequest.getUserId(), mRequest.getMatchCount(), mRequest.getMatchRating(), mRequest.getAttributeTypes(), error);
				
				return DebugGrantVProMatchRatingXPCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            VProGrowthUnlocksSlaveImpl* mComponent;
        };


        // static factory method impl
        DebugGrantVProMatchRatingXPCommandStub* DebugGrantVProMatchRatingXPCommandStub::create(Message* message, DebugGrantVProMatchRatingXPRequest* request, VProGrowthUnlocksSlave* componentImpl)
        {
            return BLAZE_NEW DebugGrantVProMatchRatingXPCommand(message, request, static_cast<VProGrowthUnlocksSlaveImpl*>(componentImpl));
        }

    } // VProGrowthUnlocks
} // Blaze
