/*************************************************************************************************/
/*!
    \file   registerforseason_command.cpp

    $Header$
    $Change$
    $DateTime$

    \attention
        (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RegisterForSeasonCommand

    Registers the entity in the specified season

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "osdkseasonalplay/rpc/osdkseasonalplayslave/registerforseason_stub.h"

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

        class RegisterForSeasonCommand : public RegisterForSeasonCommandStub  
        {
        public:
            RegisterForSeasonCommand(Message* message, RegisterForSeasonRequest* request, OSDKSeasonalPlaySlaveImpl* componentImpl)
                : RegisterForSeasonCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~RegisterForSeasonCommand() { }

        /* Private methods *******************************************************************************/
        private:

            RegisterForSeasonCommandStub::Errors execute()
            {
                TRACE_LOG("[RegisterForSeasonCommandStub:" << this << "].execute()");
                
                BlazeRpcError error = Blaze::ERR_OK;

                // establish a database connection
                OSDKSeasonalPlayDb dbHelper(mComponent->getDbId());
                error = dbHelper.getBlazeRpcError();

                if(Blaze::ERR_OK == error)
                {
                    error = mComponent->registerForSeason(dbHelper, mRequest.getMemberId(), mRequest.getMemberType(), mRequest.getLeagueId(), mRequest.getSeasonId());
                }
            
                return RegisterForSeasonCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            OSDKSeasonalPlaySlaveImpl* mComponent;
        };


        // static factory method impl
        RegisterForSeasonCommandStub* RegisterForSeasonCommandStub::create(Message* message, RegisterForSeasonRequest* request, OSDKSeasonalPlaySlave* componentImpl)
        {
            return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "RegisterForSeasonCommand") RegisterForSeasonCommand(message, request, static_cast<OSDKSeasonalPlaySlaveImpl*>(componentImpl));
        }

    } // OSDKSeasonalPlay
} // Blaze
