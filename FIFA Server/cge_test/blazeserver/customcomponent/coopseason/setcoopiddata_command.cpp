/*************************************************************************************************/
/*!
    \file   setcoopiddata_command.cpp

    $Header: //gosdev/games/FIFA/2013/Gen3/DEV/blazeserver/3.x/customcomponent/coopseason/setcoopiddata_command.cpp#1 $
    $Change: 286819 $
    $DateTime: 2012/12/19 16:14:33 $

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class SetCoopIdDataCommand

    Set the coop pair id with friend id and meta data

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "coopseason/rpc/coopseasonslave/setcoopiddata_stub.h"

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

        class SetCoopIdDataCommand : public SetCoopIdDataCommandStub  
        {
        public:
            SetCoopIdDataCommand(Message* message, SetCoopIdDataRequest* request, CoopSeasonSlaveImpl* componentImpl)
                : SetCoopIdDataCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~SetCoopIdDataCommand() { }

        /* Private methods *******************************************************************************/
        private:

            SetCoopIdDataCommand::Errors execute()
            {
				TRACE_LOG("[SetCoopIdDataCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				CoopSeason::CoopId coopId = CoopSeason::COOP_ID_HOME;
				bool success = false;

				bool bGetDataFromDB = true;

				if(mComponent->UseCache()) //Default to false for now
				{

				}

				if (bGetDataFromDB)
				{
					TRACE_LOG("[SetCoopIdDataCommand].SetCoopIdsCommand. MemberOne/MemberTwo/MetaData = "<< mRequest.getMemberOneBlazeId()<<"/"<< mRequest.getMemberTwoBlazeId()<<"/"<< mRequest.getMetadata()<< " ");

					CoopSeason::CoopPairData data;
					mComponent->setCoopIdData(mRequest.getMemberOneBlazeId(), mRequest.getMemberTwoBlazeId(), mRequest.getMetadata(), data, error);
					
					if (Blaze::ERR_OK != error)
					{						
						error = Blaze::COOPSEASON_ERR_PAIR_NOT_FOUND;
					}
					else
					{
						coopId = data.getCoopId();
						TRACE_LOG("[SetCoopIdDataCommand].SetCoopIdsCommand. coopId = " << coopId);
						
						if (coopId < CoopSeason::RESERVED_COOP_ID_ALL)
						{
							error = Blaze::COOPSEASON_ERR_DB;
						}
						else
						{
							success = true;
						}
					}
				}

				mResponse.setResultSuccess(success);
				mResponse.setCoopId(coopId);

                return SetCoopIdDataCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            CoopSeasonSlaveImpl* mComponent;
        };


        // static factory method impl
        SetCoopIdDataCommandStub* SetCoopIdDataCommandStub::create(Message* message, SetCoopIdDataRequest* request, CoopSeasonSlave* componentImpl)
        {
            return BLAZE_NEW SetCoopIdDataCommand(message, request, static_cast<CoopSeasonSlaveImpl*>(componentImpl));
        }

    } // CoopSeason
} // Blaze
