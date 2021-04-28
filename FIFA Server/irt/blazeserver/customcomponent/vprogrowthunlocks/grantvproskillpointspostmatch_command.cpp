/*************************************************************************************************/
/*!
	\file   grantvproskillpointspostmatch_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GrantVProSkillPointsPostMatch Command

    Grant VPro Skill Points for the target user

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "vprogrowthunlocks/rpc/vprogrowthunlocksslave/grantvproskillpointspostmatch_stub.h"

// global includes

// framework includes
#include "EATDF/tdf.h"

// vprogrowthunlocks includes
#include "vprogrowthunlocksslaveimpl.h"
#include "vprogrowthunlocks/tdf/vprogrowthunlockstypes.h"

namespace Blaze
{
    namespace VProGrowthUnlocks
    {

        class GrantVProSkillPointsPostMatchCommand : public GrantVProSkillPointsPostMatchCommandStub
        {
        public:
            GrantVProSkillPointsPostMatchCommand(Message* message, GrantVProSkillPointsPostMatchRequest* request, VProGrowthUnlocksSlaveImpl* componentImpl)
                : GrantVProSkillPointsPostMatchCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~GrantVProSkillPointsPostMatchCommand() { }

        /* Private methods *******************************************************************************/
        private:

            GrantVProSkillPointsPostMatchCommand::Errors execute()
            {
				TRACE_LOG("[GrantVProSkillPointsPostMatchCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				mComponent->grantSkillPointsPostMatch(mRequest.getUserId(), mRequest.getGrantMatchCountSP(), mRequest.getGrantObjectiveSP(), error);

				return GrantVProSkillPointsPostMatchCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            VProGrowthUnlocksSlaveImpl* mComponent;
        };


        // static factory method impl
        GrantVProSkillPointsPostMatchCommandStub* GrantVProSkillPointsPostMatchCommandStub::create(Message* message, GrantVProSkillPointsPostMatchRequest* request, VProGrowthUnlocksSlave* componentImpl)
        {
            return BLAZE_NEW GrantVProSkillPointsPostMatchCommand(message, request, static_cast<VProGrowthUnlocksSlaveImpl*>(componentImpl));
        }

    } // VProGrowthUnlocks
} // Blaze
