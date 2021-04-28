/*************************************************************************************************/
/*!
	\file   fetchAIPlayerCustomization_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "proclubscustom/rpc/proclubscustomslave/fetchaiplayercustomization_stub.h"

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
        class FetchAIPlayerCustomizationCommand : public FetchAIPlayerCustomizationCommandStub
        {
        public:
            FetchAIPlayerCustomizationCommand(Message* message, getAIPlayerCustomizationRequest* request, ProclubsCustomSlaveImpl* componentImpl)
                : FetchAIPlayerCustomizationCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~FetchAIPlayerCustomizationCommand() { }

        /* Private methods *******************************************************************************/
        private:

            FetchAIPlayerCustomizationCommand::Errors execute()
            {
				TRACE_LOG("[FetchAIPlayerCustomizationCommand].execute() (clubid = " << mRequest.getClubId() << ")");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				mComponent->fetchAIPlayerCustomization(mRequest.getClubId(), mResponse.getAIPlayerCustomizationList(), error);

				if (Blaze::ERR_OK != error)
				{						
					error = Blaze::DB_ERR_SYSTEM;
					mResponse.setSuccess(false);
				}
				else
				{						
					mResponse.setSuccess(true);					
				}

                return FetchAIPlayerCustomizationCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            ProclubsCustomSlaveImpl* mComponent;
        };

        // static factory method impl
        FetchAIPlayerCustomizationCommandStub* FetchAIPlayerCustomizationCommandStub::create(Message* message, getAIPlayerCustomizationRequest* request, proclubscustomSlave* componentImpl)
        {
            return BLAZE_NEW FetchAIPlayerCustomizationCommand(message, request, static_cast<ProclubsCustomSlaveImpl*>(componentImpl));
        }

    } // FetchAIPlayerCustomizationCommand
} // Blaze
