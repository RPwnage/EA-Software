/*************************************************************************************************/
/*!
    \file   getcoopiddata_command.cpp

    $Header: //gosdev/games/FIFA/2013/Gen3/DEV/blazeserver/3.x/customcomponent/coopseason/getcoopiddata_command.cpp#1 $
    $Change: 286819 $
    $DateTime: 2012/12/19 16:14:33 $

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetCoopIdDataCommand

    Retrieves the coop pair id with friend id and meta data

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "coopseason/rpc/coopseasonslave/getcoopiddata_stub.h"

// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"
#include "framework/database/dbscheduler.h"
//#include "framework/system/identity.h"

// coopseason includes
#include "coopseasonslaveimpl.h"
#include "coopseason/tdf/coopseasontypes.h"


namespace Blaze
{
    namespace CoopSeason
    {

        class GetCoopIdDataCommand : public GetCoopIdDataCommandStub  
        {
        public:
            GetCoopIdDataCommand(Message* message, GetCoopIdDataRequest* request, CoopSeasonSlaveImpl* componentImpl)
                : GetCoopIdDataCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~GetCoopIdDataCommand() { }

        /* Private methods *******************************************************************************/
        private:

            GetCoopIdDataCommand::Errors execute()
            {
				TRACE_LOG("[GetCoopIdDataCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				bool bGetDataFromDB = true;

				if(mComponent->UseCache()) //Default to false for now
				{

				}

				if (bGetDataFromDB)
				{
					TRACE_LOG("[GetCoopIdDataCommand].GetCoopIdsCommand. coopId/MemberOne/MemberTwo = "<< mRequest.getCoopId()<<"/"<< mRequest.getMemberOneBlazeId()<<"/"<< mRequest.getMemberTwoBlazeId()<< " ");

					if (mRequest.getCoopId() != 0)
					{
						mComponent->getCoopIdDataByCoopId(mRequest.getCoopId(), mResponse.getCoopPairData(), error);
					}
					else
					{
						mComponent->getCoopIdDataByBlazeIds(mRequest.getMemberOneBlazeId(), mRequest.getMemberTwoBlazeId(), mResponse.getCoopPairData(), error);
					}
					
					if (Blaze::ERR_OK != error)
					{						
						error = Blaze::COOPSEASON_ERR_PAIR_NOT_FOUND;
					}
					else
					{
					}
				}

                return GetCoopIdDataCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            CoopSeasonSlaveImpl* mComponent;
        };


        // static factory method impl
        GetCoopIdDataCommandStub* GetCoopIdDataCommandStub::create(Message* message, GetCoopIdDataRequest* request, CoopSeasonSlave* componentImpl)
        {
            return BLAZE_NEW GetCoopIdDataCommand(message, request, static_cast<CoopSeasonSlaveImpl*>(componentImpl));
        }

    } // CoopSeason
} // Blaze
