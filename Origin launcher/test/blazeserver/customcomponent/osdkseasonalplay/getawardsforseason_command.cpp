/*************************************************************************************************/
/*!
    \file   getawardsforseason_command.cpp

    $Header$
    $Change$
    $DateTime$

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetAwardsForSeasonCommand

    Retrieves the seasonal play awards for the specified season

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "osdkseasonalplay/rpc/osdkseasonalplayslave/getawardsforseason_stub.h"

// framework includes
#include "framework/database/dbscheduler.h"

// osdkseasonalplay includes
#include "osdkseasonalplayslaveimpl.h"
#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"
#include "osdkseasonalplay/osdkseasonalplaydb.h"

namespace Blaze
{
    namespace OSDKSeasonalPlay
    {

        class GetAwardsForSeasonCommand : public GetAwardsForSeasonCommandStub  
        {
        public:
            GetAwardsForSeasonCommand(Message* message, GetAwardsForSeasonRequest* request, OSDKSeasonalPlaySlaveImpl* componentImpl)
                : GetAwardsForSeasonCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~GetAwardsForSeasonCommand() { }

        /* Private methods *******************************************************************************/
        private:

            GetAwardsForSeasonCommandStub::Errors execute()
            {
                INFO_LOG("[GetAwardsForSeasonCommand:" << this << "].execute()");
                
                Blaze::BlazeRpcError error = Blaze::ERR_OK;

                OSDKSeasonalPlayDb dbHelper(mComponent->getDbId());
                error = dbHelper.getBlazeRpcError();

                if(Blaze::ERR_OK == error)
                {                    
                    error = mComponent->getAwardsBySeason(dbHelper, mRequest.getSeasonId(), mRequest.getSeasonNumber(), mRequest.getHistoricalFilter(), mResponse.getAwardList());
                }
                
                return GetAwardsForSeasonCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            OSDKSeasonalPlaySlaveImpl* mComponent;
        };


        // static factory method impl
        GetAwardsForSeasonCommandStub* GetAwardsForSeasonCommandStub::create(Message* message, GetAwardsForSeasonRequest* request, OSDKSeasonalPlaySlave* componentImpl)
        {
            return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "GetAwardsForSeasonCommand") GetAwardsForSeasonCommand(message, request, static_cast<OSDKSeasonalPlaySlaveImpl*>(componentImpl));
        }

    } // OSDKSeasonalPlay
} // Blaze
