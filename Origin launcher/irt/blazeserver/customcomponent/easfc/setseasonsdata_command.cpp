/*************************************************************************************************/
/*!
    \file   setseasonsdata_command.cpp

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/easfc/setseasonsdata_command.cpp#1 $

    \attention
    (c) Electronic Arts Inc. 2012
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class SetSeasonsDataCommand

    Command to set the Seasons Data

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"

// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"
#include "framework/database/dbscheduler.h"
#include "framework/controller/controller.h"

// easfc includes
#include "easfcslaveimpl.h"
#include "easfc/tdf/easfctypes.h"
#include "easfc/rpc/easfcslave/setseasonsdata_stub.h"

namespace Blaze
{
    namespace EASFC
    {
        class SetSeasonsDataCommand : public SetSeasonsDataCommandStub  
        {
        public:
            SetSeasonsDataCommand(Message* message, SetSeasonalPlayStatsRequest* request, EasfcSlaveImpl* componentImpl)
                : SetSeasonsDataCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~SetSeasonsDataCommand() { }

        /* Private methods *******************************************************************************/
        private:
            SetSeasonsDataCommand::Errors execute()
            {
				TRACE_LOG("[SetSeasonsDataCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				switch(mRequest.getMemberType())
				{
				case EASFC_MEMBERTYPE_USER:
					error = Blaze::ERR_OK;
					break;
				case EASFC_MEMBERTYPE_COOP_PAIR:
				case EASFC_MEMBERTYPE_CLUB: //lint -fallthrough
				default:
					error = Blaze::EASFC_ERR_INVALID_PARAMS;
					TRACE_LOG("[SetSeasonsDataCommand].execute(): ERROR! invalid MemberType");
					break;
				}

				if (mComponent->isUpgradeTitle(mRequest.getMemberId(), "SPOverallPlayerStats"))
				{
					error = Blaze::EASFC_ERR_UPGRADE_APPLIED;
				}
				
				if(Blaze::ERR_OK == error)
				{
					// write old SPOverallPlayerStats to SPPreviousOverallPlayerStats category
					error = mComponent->writeToSPPrevOverallStats(mRequest.getMemberId(), &mRequest.getSPOValues(), mRequest.getNormalGameStats());
				}

				if(Blaze::ERR_OK == error)
				{
					// apply reward for having played FIFA previously
					error = mComponent->applySeasonsRewards(mRequest.getMemberId(), &mRequest.getSPOValues());
				}

                return SetSeasonsDataCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            EasfcSlaveImpl* mComponent;
        };


        // static factory method impl
        SetSeasonsDataCommandStub* SetSeasonsDataCommandStub::create(Message* message, SetSeasonalPlayStatsRequest* request, EasfcSlave* componentImpl)
        {
            return BLAZE_NEW SetSeasonsDataCommand(message, request, static_cast<EasfcSlaveImpl*>(componentImpl));
        }

    } // EASFC
} // Blaze
