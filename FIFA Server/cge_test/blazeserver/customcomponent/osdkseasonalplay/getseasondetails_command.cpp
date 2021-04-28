/*************************************************************************************************/
/*!
    \file   getseasondetails_command.cpp

    $Header: //gosdev/games/FIFA/2022/GenX/cge_test/blazeserver/customcomponent/osdkseasonalplay/getseasondetails_command.cpp#1 $
    $Change: 1653004 $
    $DateTime: 2021/03/02 12:48:45 $

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetSeasonDetailsCommand

    Retrieves the details of the specified season. These details are not specified to a
    registered entity

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "osdkseasonalplay/rpc/osdkseasonalplayslave/getseasondetails_stub.h"

// global includes

// framework includes
#include "framework/connection/selector.h"
#include "framework/database/dbscheduler.h"

// osdkseasonalplay includes
#include "osdkseasonalplayslaveimpl.h"
#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"
#include "osdkseasonalplay/osdkseasonalplaydb.h"
#include "osdkseasonalplay/osdkseasonalplaytimeutils.h"

namespace Blaze
{
    namespace OSDKSeasonalPlay
    {

        class GetSeasonDetailsCommand : public GetSeasonDetailsCommandStub  
        {
        public:
            GetSeasonDetailsCommand(Message* message, GetSeasonDetailsRequest* request, OSDKSeasonalPlaySlaveImpl* componentImpl)
                : GetSeasonDetailsCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~GetSeasonDetailsCommand() { }

        /* Private methods *******************************************************************************/
        private:

            GetSeasonDetailsCommandStub::Errors execute()
            {
                INFO_LOG("[GetSeasonDetailsCommandStub:" << this << "].execute()");
                
                Blaze::BlazeRpcError error = Blaze::ERR_OK;

                // establish a database connection
                OSDKSeasonalPlayDb dbHelper(mComponent->getDbId());
                error = dbHelper.getBlazeRpcError();

                // fetch the current season number
                uint32_t uSeasonNumber = 0;
                int32_t iLastRolloverStatPeriodId = 0; // don't care about this value but the db query requires it.
                if(Blaze::ERR_OK == error)
                {
                     error = dbHelper.fetchSeasonNumber(mRequest.getSeasonId(), uSeasonNumber, iLastRolloverStatPeriodId);
                }
                
                // season id
                SeasonId seasonId = mRequest.getSeasonId();
                mResponse.setSeasonId(seasonId);

                // season state
                mResponse.setSeasonState(mComponent->getSeasonState(seasonId));

                // season number
                mResponse.setSeasonNumber(uSeasonNumber);                    

                // regular season start
                mResponse.setRegularSeasonStart(mComponent->getSeasonStartTime(seasonId));

                // regular season end
                mResponse.setRegularSeasonEnd(mComponent->getRegularSeasonEndTime(seasonId));

                // playoff start time
                mResponse.setPlayOffStart(mComponent->getPlayoffStartTime(seasonId));

                // playoff end time
                mResponse.setPlayOffEnd(mComponent->getPlayoffEndTime(seasonId));

                // next regular season start time
                mResponse.setNextRegularSeasonStart(mComponent->getNextSeasonRegularSeasonStartTime(seasonId));

                // log the times for debugging purposes
                TRACE_LOG("[GetSeasonDetailsCommandStub:" << this << "].execute(): season times for season id " << seasonId << ":");
                OSDKSeasonalPlayTimeUtils::logTime("reg season start time     ", mResponse.getRegularSeasonStart());
                OSDKSeasonalPlayTimeUtils::logTime("reg season end time       ", mResponse.getRegularSeasonEnd());
                OSDKSeasonalPlayTimeUtils::logTime("playoff start time        ", mResponse.getPlayOffStart());
                OSDKSeasonalPlayTimeUtils::logTime("playoff end time          ", mResponse.getPlayOffEnd());
                OSDKSeasonalPlayTimeUtils::logTime("next reg season start time", mResponse.getNextRegularSeasonStart());

                return GetSeasonDetailsCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            OSDKSeasonalPlaySlaveImpl* mComponent;
        };


        // static factory method impl
        GetSeasonDetailsCommandStub* GetSeasonDetailsCommandStub::create(Message* message, GetSeasonDetailsRequest* request, OSDKSeasonalPlaySlave* componentImpl)
        {
            return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "GetSeasonDetailsCommand") GetSeasonDetailsCommand(message, request, static_cast<OSDKSeasonalPlaySlaveImpl*>(componentImpl));
        }

    } // OSDKSeasonalPlay
} // Blaze
