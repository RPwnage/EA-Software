/*************************************************************************************************/
/*!
	\file   fetchSkillTreeConfig_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FetchSkillTreeConfigCommand

    Fetch skill tree config.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "vprogrowthunlocks/rpc/vprogrowthunlocksslave/fetchskilltreeconfig_stub.h"

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
        class FetchSkillTreeConfigCommand : public FetchSkillTreeConfigCommandStub  
        {
        public:
			FetchSkillTreeConfigCommand(Message* message, VProGrowthUnlocksSlaveImpl* componentImpl)
                : FetchSkillTreeConfigCommandStub(message), mComponent(componentImpl)
            { }

            virtual ~FetchSkillTreeConfigCommand() { }

        /* Private methods *******************************************************************************/
        private:

			FetchSkillTreeConfigCommand::Errors execute()
            {
				TRACE_LOG("[FetchSkillTreeConfigCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				mComponent->fetchSkillTreeConfig(mResponse.getSkillTree(), error);
				mResponse.setSuccess(error == Blaze::ERR_OK);				

                return FetchSkillTreeConfigCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            VProGrowthUnlocksSlaveImpl* mComponent;
        };

        // static factory method impl
        FetchSkillTreeConfigCommandStub* FetchSkillTreeConfigCommandStub::create(Message* message, VProGrowthUnlocksSlave* componentImpl)
        {
            return BLAZE_NEW FetchSkillTreeConfigCommand(message, static_cast<VProGrowthUnlocksSlaveImpl*>(componentImpl));
        }

    } // VProGrowthUnlocks
} // Blaze
