/*************************************************************************************************/
/*!
	\file   UpdateSettingCommand_command.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateSettingCommand

    Reset load outs for one user

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "proclubscustom/rpc/proclubscustomslave/updatesetting_stub.h"

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
        class UpdateSettingCommand : public UpdateSettingCommandStub
        {
        public:
			UpdateSettingCommand(Message* message, updateCustomizationsRequest* request, ProclubsCustomSlaveImpl* componentImpl)
                : UpdateSettingCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~UpdateSettingCommand() { }

        /* Private methods *******************************************************************************/
        private:

			UpdateSettingCommand::Errors execute()
            {
				TRACE_LOG("[UpdateSettingCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				mComponent->updateSetting(mRequest.getClubId(), mRequest.getSettings(), error);
				mResponse.setSuccess(error == Blaze::ERR_OK);

                return UpdateSettingCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
			ProclubsCustomSlaveImpl* mComponent; 
        };

        // static factory method impl
		UpdateSettingCommandStub* UpdateSettingCommandStub::create(Message* message, updateCustomizationsRequest* request, proclubscustomSlave* componentImpl)
        {
            return BLAZE_NEW UpdateSettingCommand(message, request, static_cast<ProclubsCustomSlaveImpl*>(componentImpl));
        }

    } // ProClubsCustom
} // Blaze
