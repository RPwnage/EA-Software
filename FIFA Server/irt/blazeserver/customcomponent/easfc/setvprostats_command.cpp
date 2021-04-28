/*************************************************************************************************/
/*!
    \file   setvprostats_command.cpp

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/13.x/customcomponent/easfc/setvprostats_command.cpp#1 $

    \attention
    (c) Electronic Arts Inc. 2013
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Set VPro Stats

    Sets VPro Stats

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
#include "easfc/rpc/easfcslave/setvprostats_stub.h"

namespace Blaze
{
    namespace EASFC
    {
        class SetVProStatsCommand : public SetVProStatsCommandStub  
        {
        public:
            SetVProStatsCommand(Message* message, SetVProStatsRequest* request, EasfcSlaveImpl* componentImpl)
                : SetVProStatsCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~SetVProStatsCommand() { }

        /* Private methods *******************************************************************************/
        private:

			SetVProStatsCommand::Errors execute()
            {
				TRACE_LOG("[SetVProStatsCommand].execute()");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;
				
				switch(mRequest.getMemberType())
				{
				case EASFC_MEMBERTYPE_CLUB:
					error = Blaze::ERR_OK;
					break;
				case EASFC_MEMBERTYPE_USER:
				case EASFC_MEMBERTYPE_COOP_PAIR:
				default:
					error = Blaze::EASFC_ERR_INVALID_PARAMS;
					TRACE_LOG("[SetVProStatsCommand].execute(): ERROR! invalid MemberType");
					break;
				}
				
				if (mComponent->isUpgradeTitle(mRequest.getMemberId(), "VProStats"))
				{
					error = Blaze::EASFC_ERR_UPGRADE_APPLIED;
					TRACE_LOG("[SetVProStatsCommand].execute(): Upgrade has already been applied!  Skipping!");
				}

				if(Blaze::ERR_OK == error)
				{
					error = mComponent->updateVproStats("VProStats", mRequest.getMemberId(), &mRequest.getStatValues());
				}

				if(Blaze::ERR_OK == error)
				{
					error = mComponent->updateVproIntStats("VProStats", mRequest.getMemberId(), &mRequest.getStatIntValues());
				}

                return SetVProStatsCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            EasfcSlaveImpl* mComponent;
        };


        // static factory method impl
        SetVProStatsCommandStub* SetVProStatsCommandStub::create(Message* message, SetVProStatsRequest* request, EasfcSlave* componentImpl)
        {
            return BLAZE_NEW SetVProStatsCommand(message, request, static_cast<EasfcSlaveImpl*>(componentImpl));
        }

    } // EASFC
} // Blaze
