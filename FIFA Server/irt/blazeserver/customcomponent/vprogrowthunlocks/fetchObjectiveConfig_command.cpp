/*************************************************************************************************/
/*!
	\file   fetchObjectiveConfig_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FetchObjectiveConfigCommand

    Fetch objectives config.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "vprogrowthunlocks/rpc/vprogrowthunlocksslave/fetchobjectiveconfig_stub.h"

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
        class FetchObjectiveConfigCommand : public FetchObjectiveConfigCommandStub  
        {
        public:
            FetchObjectiveConfigCommand(Message* message, VProGrowthUnlocksSlaveImpl* componentImpl)
                : FetchObjectiveConfigCommandStub(message), mComponent(componentImpl)
            { }

            virtual ~FetchObjectiveConfigCommand() { }

        /* Private methods *******************************************************************************/
        private:

            FetchObjectiveConfigCommand::Errors execute()
            {
				TRACE_LOG("[FetchObjectiveConfigCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				mComponent->fetchObjectiveConfig(mResponse.getObjectiveCategories(), mResponse.getProgressionRewards(), error);
				mResponse.setSuccess(error == Blaze::ERR_OK);				

                return FetchObjectiveConfigCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            VProGrowthUnlocksSlaveImpl* mComponent;
        };

        // static factory method impl
        FetchObjectiveConfigCommandStub* FetchObjectiveConfigCommandStub::create(Message* message, VProGrowthUnlocksSlave* componentImpl)
        {
            return BLAZE_NEW FetchObjectiveConfigCommand(message, static_cast<VProGrowthUnlocksSlaveImpl*>(componentImpl));
        }

    } // VProGrowthUnlocks
} // Blaze
