/*************************************************************************************************/
/*!
	\file   fetchObjectiveProgress_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FetchObjectiveProgressCommand

    Fetch objectives progress for one user

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "vprogrowthunlocks/rpc/vprogrowthunlocksslave/fetchobjectiveprogress_stub.h"

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
        class FetchObjectiveProgressCommand : public FetchObjectiveProgressCommandStub  
        {
        public:
            FetchObjectiveProgressCommand(Message* message, FetchObjectiveProgressRequest* request, VProGrowthUnlocksSlaveImpl* componentImpl)
                : FetchObjectiveProgressCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~FetchObjectiveProgressCommand() { }

        /* Private methods *******************************************************************************/
        private:

            FetchObjectiveProgressCommand::Errors execute()
            {
				TRACE_LOG("[FetchObjectiveProgressCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				mComponent->fetchObjectiveProgress(mRequest.getUserIdList(), mResponse.getObjectiveProgressGroups(), error);
				mResponse.setSuccess(error == Blaze::ERR_OK);

                return FetchObjectiveProgressCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            VProGrowthUnlocksSlaveImpl* mComponent;
        };

        // static factory method impl
        FetchObjectiveProgressCommandStub* FetchObjectiveProgressCommandStub::create(Message* message, FetchObjectiveProgressRequest* request, VProGrowthUnlocksSlave* componentImpl)
        {
            return BLAZE_NEW FetchObjectiveProgressCommand(message, request, static_cast<VProGrowthUnlocksSlaveImpl*>(componentImpl));
        }

    } // VProGrowthUnlocks
} // Blaze
