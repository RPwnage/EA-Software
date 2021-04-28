/*************************************************************************************************/
/*!
	\file   UpdateCustomTacticsCommand_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateCustomTacticsCommand

    updates custom tactics for a club

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "proclubscustom/rpc/proclubscustomslave/updatecustomtactics_stub.h"

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
        class UpdateCustomTacticsCommand : public UpdateCustomTacticsCommandStub
        {
        public:
			UpdateCustomTacticsCommand(Message* message, updateCustomTacticsRequest* request, ProclubsCustomSlaveImpl* componentImpl)
                : UpdateCustomTacticsCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~UpdateCustomTacticsCommand() { }

        /* Private methods *******************************************************************************/
        private:

			UpdateCustomTacticsCommand::Errors execute()
            {
				TRACE_LOG("[UpdateCustomTacticsCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;
				int32_t loadOutId = 0;

				TRACE_LOG("[UpdateCustomTacticsCommand].UpdateCustomTacticsCommand. owner blaze Id = "<< ", loadOut Id = " << loadOutId);
			

				mComponent->updateCustomTactics(mRequest.getClubId(),mRequest.getSlotId(),mRequest.getCustomTactics(),error);

				if (Blaze::ERR_OK != error)
				{						
					error = Blaze::ERR_DB_NOT_SUPPORTED;
					mResponse.setSuccess(false);
				}
				else
				{						
					mResponse.setSuccess(true);					
				}

                return UpdateCustomTacticsCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
			ProclubsCustomSlaveImpl* mComponent;
        };

        // static factory method impl
		UpdateCustomTacticsCommandStub* UpdateCustomTacticsCommandStub::create(Message* message, updateCustomTacticsRequest* request, proclubscustomSlave* componentImpl)
        {
            return BLAZE_NEW UpdateCustomTacticsCommand(message, request, static_cast<ProclubsCustomSlaveImpl*>(componentImpl));
        }

    } // ProClubsCustom
} // Blaze
