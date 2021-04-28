/*************************************************************************************************/
/*!
	\file   fetchPerksConfig_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FetchPerksConfigCommand

    Fetch perks and perk slots config.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "vprogrowthunlocks/rpc/vprogrowthunlocksslave/fetchperksconfig_stub.h"

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
        class FetchPerksConfigCommand : public FetchPerksConfigCommandStub  
        {
        public:
			FetchPerksConfigCommand(Message* message, VProGrowthUnlocksSlaveImpl* componentImpl)
                : FetchPerksConfigCommandStub(message), mComponent(componentImpl)
            { }

            virtual ~FetchPerksConfigCommand() { }

        /* Private methods *******************************************************************************/
        private:

			FetchPerksConfigCommand::Errors execute()
            {
				TRACE_LOG("[FetchPerksConfigCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				mComponent->fetchPerksConfig(mResponse.getPerkSlots(), mResponse.getPerks(), mResponse.getPerkCategories(), error);
				mResponse.setSuccess(error == Blaze::ERR_OK);				

                return FetchPerksConfigCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            VProGrowthUnlocksSlaveImpl* mComponent;
        };

        // static factory method impl
        FetchPerksConfigCommandStub* FetchPerksConfigCommandStub::create(Message* message, VProGrowthUnlocksSlave* componentImpl)
        {
            return BLAZE_NEW FetchPerksConfigCommand(message, static_cast<VProGrowthUnlocksSlaveImpl*>(componentImpl));
        }

    } // VProGrowthUnlocks
} // Blaze
