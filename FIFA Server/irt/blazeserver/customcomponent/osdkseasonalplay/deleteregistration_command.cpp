/*************************************************************************************************/
/*!
    \file   deleteregistration_command.cpp

    $Header$
    $Change$
    $DateTime$

    \attention
        (c) Electronic Arts Inc. 2013
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DeleteRegistrationCommand

    Delete a user or club from the registration table

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "osdkseasonalplay/rpc/osdkseasonalplayslave/deleteregistration_stub.h"

// global includes

// framework includes
#include "framework/connection/selector.h"
#include "framework/database/dbscheduler.h"

// osdkseasonalplay includes
#include "osdkseasonalplayslaveimpl.h"
#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"
#include "osdkseasonalplay/osdkseasonalplaydb.h"

namespace Blaze
{
    namespace OSDKSeasonalPlay
    {

        class DeleteRegistrationCommand : public DeleteRegistrationCommandStub  
        {
        public:
            DeleteRegistrationCommand(Message* message, DeleteRegistrationRequest* request, OSDKSeasonalPlaySlaveImpl* componentImpl)
                : DeleteRegistrationCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~DeleteRegistrationCommand() { }

        /* Private methods *******************************************************************************/
        private:

            DeleteRegistrationCommandStub::Errors execute()
            {
                TRACE_LOG("[DeleteRegistrationCommandStub:" << this << "].execute()");
                
                Blaze::BlazeRpcError error = mComponent->deleteRegistration(
                    mRequest.getMemberId(), mRequest.getMemberType());
            
                return DeleteRegistrationCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            OSDKSeasonalPlaySlaveImpl* mComponent;
        };


        // static factory method impl
        DeleteRegistrationCommandStub* DeleteRegistrationCommandStub::create(Message* message, DeleteRegistrationRequest* request, OSDKSeasonalPlaySlave* componentImpl)
        {
            return BLAZE_NEW DeleteRegistrationCommand(message, request, static_cast<OSDKSeasonalPlaySlaveImpl*>(componentImpl));
        }

    } // OSDKSeasonalPlay
} // Blaze
