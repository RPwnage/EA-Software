/*************************************************************************************************/
/*!
    \file   getseasonid_command.cpp

    $Header: //gosdev/games/FIFA/2022/GenX/cge_test/blazeserver/customcomponent/osdkseasonalplay/getseasonid_command.cpp#1 $
    $Change: 1653004 $
    $DateTime: 2021/03/02 12:48:45 $

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetSeasonIdCommand

    Retrieves the seasonal id of the season the entity is registered in

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "osdkseasonalplay/rpc/osdkseasonalplayslave/getseasonid_stub.h"

// global includes

// framework includes
#include "framework/connection/selector.h"

// osdkseasonalplay includes
#include "osdkseasonalplayslaveimpl.h"
#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"
#include "osdkseasonalplay/osdkseasonalplaydb.h"

namespace Blaze
{
    namespace OSDKSeasonalPlay
    {

        class GetSeasonIdCommand : public GetSeasonIdCommandStub  
        {
        public:
            GetSeasonIdCommand(Message* message, GetSeasonIdRequest* request, OSDKSeasonalPlaySlaveImpl* componentImpl)
                : GetSeasonIdCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~GetSeasonIdCommand() { }

        /* Private methods *******************************************************************************/
        private:

            GetSeasonIdCommandStub::Errors execute()
            {
                SPAM_LOG("[GetSeasonIdCommandStub:" << this << "].execute()");
                
                SeasonId seasonId = SEASON_ID_INVALID;
                
                OSDKSeasonalPlayDb dbHelper(mComponent->getDbId());
                Blaze::BlazeRpcError error = dbHelper.getBlazeRpcError();

                if(Blaze::ERR_OK == error)
                {                    
                    error = dbHelper.fetchSeasonId(mRequest.getMemberId(), mRequest.getMemberType(), seasonId);
                }

                mResponse.setSeasonId(seasonId);

                return GetSeasonIdCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            OSDKSeasonalPlaySlaveImpl* mComponent;
        };


        // static factory method impl
        GetSeasonIdCommandStub* GetSeasonIdCommandStub::create(Message* message, GetSeasonIdRequest* request, OSDKSeasonalPlaySlave* componentImpl)
        {
            return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "GetSeasonIdCommand") GetSeasonIdCommand(message, request, static_cast<OSDKSeasonalPlaySlaveImpl*>(componentImpl));
        }

    } // OSDKSeasonalPlay
} // Blaze
