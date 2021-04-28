/*************************************************************************************************/
/*!
    \file   getallcoopids_command.cpp

    $Header: //gosdev/games/FIFA/2013/Gen3/DEV/blazeserver/3.x/customcomponent/coopseason/getallcoopids_command.cpp#1 $
    $Change: 286819 $
    $DateTime: 2012/12/19 16:14:33 $

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetAllCoopIdsCommand

    Retrieves the coop pair id and the friend id for all coop seasons that the target user involves

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "coopseason/rpc/coopseasonslave/getallcoopids_stub.h"

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

        class GetAllCoopIdsCommand : public GetAllCoopIdsCommandStub  
        {
        public:
            GetAllCoopIdsCommand(Message* message, GetAllCoopIdsRequest* request, CoopSeasonSlaveImpl* componentImpl)
                : GetAllCoopIdsCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~GetAllCoopIdsCommand() { }

        /* Private methods *******************************************************************************/
        private:

            GetAllCoopIdsCommand::Errors execute()
            {
				TRACE_LOG("[GetAllCoopIdsCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				bool bGetDataFromDB = true;
				uint32_t count = 0;

				if(mComponent->UseCache()) //Default to false for now
				{

				}

				if (bGetDataFromDB)
				{
					TRACE_LOG("[GetAllCoopIdsCommand].GetAllCoopIdsCommand. target blaze Id = "<< mRequest.getTargetBlazeId() <<" ");

					mComponent->getAllCoopIds(mRequest.getTargetBlazeId(), mResponse.getCoopPairDataList(), count, error);
					
					if (Blaze::ERR_OK != error)
					{						
						error = Blaze::COOPSEASON_ERR_USER_NOT_FOUND;
						mResponse.setResultCount(0);
					}
					else
					{						
						mResponse.setResultCount(count);
					}
				}

                return GetAllCoopIdsCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            CoopSeasonSlaveImpl* mComponent;
        };


        // static factory method impl
        GetAllCoopIdsCommandStub* GetAllCoopIdsCommandStub::create(Message* message, GetAllCoopIdsRequest* request, CoopSeasonSlave* componentImpl)
        {
            return BLAZE_NEW GetAllCoopIdsCommand(message, request, static_cast<CoopSeasonSlaveImpl*>(componentImpl));
        }

    } // CoopSeason
} // Blaze
