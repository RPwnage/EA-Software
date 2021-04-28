/*************************************************************************************************/
/*!
	\file   fetchPlayerGrowthConfig_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FetchPlayerGrowthConfigCommand

    Fetch player growth config.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "vprogrowthunlocks/rpc/vprogrowthunlocksslave/fetchplayergrowthconfig_stub.h"

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
        class FetchPlayerGrowthConfigCommand : public FetchPlayerGrowthConfigCommandStub  
        {
        public:
			FetchPlayerGrowthConfigCommand(Message* message, VProGrowthUnlocksSlaveImpl* componentImpl)
                : FetchPlayerGrowthConfigCommandStub(message), mComponent(componentImpl)
            { }

            virtual ~FetchPlayerGrowthConfigCommand() { }

        /* Private methods *******************************************************************************/
        private:

			FetchPlayerGrowthConfigCommand::Errors execute()
            {
				TRACE_LOG("[FetchPlayerGrowthConfigCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				mComponent->fetchPlayerGrowthConfig(mResponse.getPlayerGrowthConfig(), error);
				mResponse.setSuccess(error == Blaze::ERR_OK);				

                return FetchPlayerGrowthConfigCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            VProGrowthUnlocksSlaveImpl* mComponent;
        };

        // static factory method impl
        FetchPlayerGrowthConfigCommandStub* FetchPlayerGrowthConfigCommandStub::create(Message* message, VProGrowthUnlocksSlave* componentImpl)
        {
            return BLAZE_NEW FetchPlayerGrowthConfigCommand(message, static_cast<VProGrowthUnlocksSlaveImpl*>(componentImpl));
        }

    } // VProGrowthUnlocks
} // Blaze
