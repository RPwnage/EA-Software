/*************************************************************************************************/
/*!
	\file   debuggrantvproxp_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DebugGrantVProXpCommand

    Debug Grant XP for the target user

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "vprogrowthunlocks/rpc/vprogrowthunlocksslave/debuggrantvproxp_stub.h"

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

        class DebugGrantVProXpCommand : public DebugGrantVProXpCommandStub
        {
        public:
			DebugGrantVProXpCommand(Message* message, DebugGrantVProXpRequest* request, VProGrowthUnlocksSlaveImpl* componentImpl)
                : DebugGrantVProXpCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~DebugGrantVProXpCommand() { }

        /* Private methods *******************************************************************************/
        private:

			DebugGrantVProXpCommand::Errors execute()
            {
				TRACE_LOG("[DebugGrantVProXpCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				TRACE_LOG("[DebugGrantVProXpCommand].DebugGrantVProXpCommand. target blaze Id = "<< mRequest.getUserId() <<" ");
				mComponent->grantXP(mRequest.getUserId(), mRequest.getXp(), error);
				
				return DebugGrantVProXpCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            VProGrowthUnlocksSlaveImpl* mComponent;
        };


        // static factory method impl
        DebugGrantVProXpCommandStub* DebugGrantVProXpCommandStub::create(Message* message, DebugGrantVProXpRequest* request, VProGrowthUnlocksSlave* componentImpl)
        {
            return BLAZE_NEW DebugGrantVProXpCommand(message, request, static_cast<VProGrowthUnlocksSlaveImpl*>(componentImpl));
        }

    } // VProGrowthUnlocks
} // Blaze
