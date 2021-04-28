/*************************************************************************************************/
/*!
    \file   removecoopiddata_command.cpp

    $Header: //gosdev/games/FIFA/2013/Gen3/DEV/blazeserver/3.x/customcomponent/coopseason/removecoopiddata_command.cpp#1 $
    $Change: 286819 $
    $DateTime: 2012/12/19 16:14:33 $

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RemoveCoopIdDataCommand

    Remove the coop pair record

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "coopseason/rpc/coopseasonslave/removecoopiddata_stub.h"

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

        class RemoveCoopIdDataCommand : public RemoveCoopIdDataCommandStub  
        {
        public:
            RemoveCoopIdDataCommand(Message* message, RemoveCoopIdDataRequest* request, CoopSeasonSlaveImpl* componentImpl)
                : RemoveCoopIdDataCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~RemoveCoopIdDataCommand() { }

        /* Private methods *******************************************************************************/
        private:

            RemoveCoopIdDataCommand::Errors execute()
            {
				TRACE_LOG("[RemoveCoopIdDataCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				bool bGetDataFromDB = true;

				if(mComponent->UseCache()) //Default to false for now
				{

				}

				if (bGetDataFromDB)
				{
					TRACE_LOG("[RemoveCoopIdDataCommand].RemoveCoopIdsCommand. coopId/MemberOne/MemberTwo = "<< mRequest.getCoopId()<<"/"<< mRequest.getMemberOneBlazeId()<<"/"<< mRequest.getMemberTwoBlazeId()<< " ");

					if (mRequest.getCoopId() != 0)
					{
						mComponent->removeCoopIdDataByCoopId(mRequest.getCoopId(), error);
					}
					else
					{
						mComponent->removeCoopIdDataByBlazeIds(mRequest.getMemberOneBlazeId(), mRequest.getMemberTwoBlazeId(), error);
					}
					
					if (Blaze::ERR_OK != error)
					{						
						error = Blaze::COOPSEASON_ERR_GENERAL;
						mResponse.setResultSuccess(false);
					}
					else
					{
						mResponse.setResultSuccess(true);
					}
				}

                return RemoveCoopIdDataCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            CoopSeasonSlaveImpl* mComponent;
        };


        // static factory method impl
        RemoveCoopIdDataCommandStub* RemoveCoopIdDataCommandStub::create(Message* message, RemoveCoopIdDataRequest* request, CoopSeasonSlave* componentImpl)
        {
            return BLAZE_NEW RemoveCoopIdDataCommand(message, request, static_cast<CoopSeasonSlaveImpl*>(componentImpl));
        }

    } // CoopSeason
} // Blaze
