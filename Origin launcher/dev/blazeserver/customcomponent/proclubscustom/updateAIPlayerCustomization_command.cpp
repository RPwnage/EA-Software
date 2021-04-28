/*************************************************************************************************/
/*!
	\file   updateAIPlayerCustomization_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class updateAIPlayerCustomizationCommand

    Reset load outs for one user

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "proclubscustom/rpc/proclubscustomslave/updateaiplayercustomization_stub.h"

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
        class UpdateAIPlayerCustomizationCommand : public UpdateAIPlayerCustomizationCommandStub
        {
        public:
			UpdateAIPlayerCustomizationCommand(Message* message, updateAIPlayerCustomizationRequest* request, ProclubsCustomSlaveImpl* componentImpl)
                : UpdateAIPlayerCustomizationCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~UpdateAIPlayerCustomizationCommand() { }

        /* Private methods *******************************************************************************/
        private:

			UpdateAIPlayerCustomizationCommand::Errors execute()
            {
				TRACE_LOG("[UpdateAIPlayerCustomizationCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				TRACE_LOG("[AIPlayerCustomisationCommand].AIPlayerCustomisationCommand.");

				mComponent->updateAIPlayerCustomization(mRequest.getClubId(), mRequest.getSlotId(), mRequest.getAIPlayerCustomization(), mRequest.getUpdatedProperties(), error);

				if (Blaze::ERR_OK != error)
				{
					error = Blaze::ERR_DB_NOT_SUPPORTED;
					mResponse.setSuccess(false);
				}
				else
				{
					mResponse.setSuccess(true);
				}

				return UpdateAIPlayerCustomizationCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
			ProclubsCustomSlaveImpl* mComponent;
        };

        // static factory method impl
		UpdateAIPlayerCustomizationCommandStub* UpdateAIPlayerCustomizationCommandStub::create(Message* message, updateAIPlayerCustomizationRequest* request, proclubscustomSlave* componentImpl)
        {
            return BLAZE_NEW UpdateAIPlayerCustomizationCommand(message, request, static_cast<ProclubsCustomSlaveImpl*>(componentImpl));
        }

    } // ProClubsCustom
} // Blaze
