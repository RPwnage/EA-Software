/*************************************************************************************************/
/*!
	\file   debuggrantvproskillpoints_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DebugGrantVProSkillPointsCommand

    Debug Grant VPro Skill Points for the target user

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "vprogrowthunlocks/rpc/vprogrowthunlocksslave/debuggrantvproskillpoints_stub.h"

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

        class DebugGrantVProSkillPointsCommand : public DebugGrantVProSkillPointsCommandStub  
        {
        public:
            DebugGrantVProSkillPointsCommand(Message* message, DebugGrantVProSkillPointsRequest* request, VProGrowthUnlocksSlaveImpl* componentImpl)
                : DebugGrantVProSkillPointsCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~DebugGrantVProSkillPointsCommand() { }

        /* Private methods *******************************************************************************/
        private:

            DebugGrantVProSkillPointsCommand::Errors execute()
            {
				TRACE_LOG("[DebugGrantVProSkillPointsCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				TRACE_LOG("[DebugGrantVProSkillPointsCommand].DebugGrantVProSkillPointsCommand. target blaze Id = "<< mRequest.getUserId() <<" ");
				mComponent->grantSkillPoints(mRequest.getUserId(), mRequest.getSkillPoint(), error);					
				
				return DebugGrantVProSkillPointsCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            VProGrowthUnlocksSlaveImpl* mComponent;
        };


        // static factory method impl
        DebugGrantVProSkillPointsCommandStub* DebugGrantVProSkillPointsCommandStub::create(Message* message, DebugGrantVProSkillPointsRequest* request, VProGrowthUnlocksSlave* componentImpl)
        {
            return BLAZE_NEW DebugGrantVProSkillPointsCommand(message, request, static_cast<VProGrowthUnlocksSlaveImpl*>(componentImpl));
        }

    } // VProGrowthUnlocks
} // Blaze
