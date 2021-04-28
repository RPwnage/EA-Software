/*************************************************************************************************/
/*!
    \class FetchCustomTacticsCommand

    Fetch the custom tactics for club id

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "proclubscustom/rpc/proclubscustomslave/fetchcustomtactics_stub.h"

// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"
#include "framework/database/dbscheduler.h"

#include "proclubscustomslaveimpl.h"
#include "proclubscustom/tdf/proclubscustomtypes.h"

namespace Blaze
{
    namespace proclubscustom
    {
        class FetchCustomTacticsCommand : public FetchCustomTacticsCommandStub
        {
        public:
            FetchCustomTacticsCommand(Message* message, getCustomTacticsRequest* request, ProclubsCustomSlaveImpl* componentImpl)
                : FetchCustomTacticsCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~FetchCustomTacticsCommand() { }

        /* Private methods *******************************************************************************/
        private:

            FetchCustomTacticsCommand::Errors execute()
            {
				TRACE_LOG("[FetchCustomTacticsCommand].execute() (clubid = " << mRequest.getClubId() << ")");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;
					
				mComponent->fetchCustomTactics(mRequest.getClubId(), mResponse.getCustomTactics(), error);

				if (Blaze::ERR_OK != error)
				{
					mResponse.setSuccess(false);
				}
				else
				{
					mResponse.setSuccess(true);
				}

                return FetchCustomTacticsCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            ProclubsCustomSlaveImpl* mComponent;
        };

        // static factory method impl
        FetchCustomTacticsCommandStub* FetchCustomTacticsCommandStub::create(Message* message, getCustomTacticsRequest* request, proclubscustomSlave* componentImpl)
        {
            return BLAZE_NEW FetchCustomTacticsCommand(message, request, static_cast<ProclubsCustomSlaveImpl*>(componentImpl));
        }

    } // FetchCustomTacticsCommand
} // Blaze
