/*************************************************************************************************/
/*!
	\file   debugcompletevproobjectives_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DebugCompleteVProObjectivesCommand

    Debug Complete VPro Objectives for the target user

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "vprogrowthunlocks/rpc/vprogrowthunlocksslave/debugcompletevproobjectives_stub.h"

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

        class DebugCompleteVProObjectivesCommand : public DebugCompleteVProObjectivesCommandStub  
        {
        public:
			DebugCompleteVProObjectivesCommand(Message* message, DebugCompleteVProObjectivesRequest* request, VProGrowthUnlocksSlaveImpl* componentImpl)
                : DebugCompleteVProObjectivesCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~DebugCompleteVProObjectivesCommand() { }

        /* Private methods *******************************************************************************/
        private:

			DebugCompleteVProObjectivesCommand::Errors execute()
            {
				TRACE_LOG("[DebugCompleteVProObjectivesCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				TRACE_LOG("[DebugCompleteVProObjectivesCommand].DebugCompleteVProObjectivesCommand. target blaze Id = "<< mRequest.getUserId() <<" ");
				mComponent->completeObjectives(mRequest.getUserId(), mRequest.getCategoryIds(), mRequest.getObjectiveIds(), error);
				
				return DebugCompleteVProObjectivesCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            VProGrowthUnlocksSlaveImpl* mComponent;
        };


        // static factory method impl
        DebugCompleteVProObjectivesCommandStub* DebugCompleteVProObjectivesCommandStub::create(Message* message, DebugCompleteVProObjectivesRequest* request, VProGrowthUnlocksSlave* componentImpl)
        {
            return BLAZE_NEW DebugCompleteVProObjectivesCommand(message, request, static_cast<VProGrowthUnlocksSlaveImpl*>(componentImpl));
        }

    } // VProGrowthUnlocks
} // Blaze
