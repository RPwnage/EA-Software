/*************************************************************************************************/
/*!
	\file   fetchLoadOuts_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FetchLoadOutsCommand

    Fetch load outs for one user

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "proclubscustom/rpc/proclubscustomslave/fetchsetting_stub.h"

// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"
#include "framework/database/dbscheduler.h"

// vprogrowthunlocks includes
#include "proclubscustomslaveimpl.h"
#include "proclubscustom/tdf/proclubscustomtypes.h"

namespace Blaze
{
    namespace proclubscustom
    {
        class FetchSettingCommand : public FetchSettingCommandStub  
        {
        public:
            FetchSettingCommand(Message* message, getCustomizationsRequest* request, ProclubsCustomSlaveImpl* componentImpl)
                : FetchSettingCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~FetchSettingCommand() { }

        /* Private methods *******************************************************************************/
        private:

            FetchSettingCommand::Errors execute()
            {
				TRACE_LOG("[FetchSettingCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				TRACE_LOG("[FetchSettingCommand].FetchSettingCommand. (clubid = "<< mRequest.getClubId() <<")");
				mComponent->fetchSetting(mRequest.getClubId(), mResponse.getSettings(), error);
				
				if (Blaze::ERR_OK != error)
				{						
					mResponse.setSuccess(false);
				}
				else
				{						
					mResponse.setSuccess(true);
				}

                return FetchSettingCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            ProclubsCustomSlaveImpl* mComponent;
        };

        // static factory method impl
        FetchSettingCommandStub* FetchSettingCommandStub::create(Message* message, getCustomizationsRequest* request, proclubscustomSlave* componentImpl)
        {
            return BLAZE_NEW FetchSettingCommand(message, request, static_cast<ProclubsCustomSlaveImpl*>(componentImpl));
        }

    } // proclubsCustom
} // Blaze
