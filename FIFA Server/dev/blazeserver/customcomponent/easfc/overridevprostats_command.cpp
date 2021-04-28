/*************************************************************************************************/
/*!
    \file   overridevprostats_command.cpp

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/13.x/customcomponent/easfc/overridevprostats_command.cpp#1 $

    \attention
    (c) Electronic Arts Inc. 2013
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Override VPro Stats

    Override VPro Stats

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
#include "easfc/rpc/easfcslave/overridevprostats_stub.h"

namespace Blaze
{
    namespace EASFC
    {
        class OverrideVProStatsCommand : public OverrideVProStatsCommandStub  
        {
        public:
            OverrideVProStatsCommand(Message* message, OverrideVProStatsRequest* request, EasfcSlaveImpl* componentImpl)
                : OverrideVProStatsCommandStub(message, request), mComponent(componentImpl)
            { }

            virtual ~OverrideVProStatsCommand() { }

        /* Private methods *******************************************************************************/
        private:

			OverrideVProStatsCommand::Errors execute()
            {
				TRACE_LOG("[OverrideVProStatsCommand].execute()");
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
					TRACE_LOG("[OverrideVProStatsCommand].execute(): ERROR! invalid MemberType");
					break;
				}
				
				if(Blaze::ERR_OK == error)
				{
					error = mComponent->updateVproStats("VProStats", mRequest.getMemberId(), &mRequest.getStatValues());
				}

                return OverrideVProStatsCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            EasfcSlaveImpl* mComponent;
        };


        // static factory method impl
        OverrideVProStatsCommandStub* OverrideVProStatsCommandStub::create(Message* message, OverrideVProStatsRequest* request, EasfcSlave* componentImpl)
        {
            return BLAZE_NEW OverrideVProStatsCommand(message, request, static_cast<EasfcSlaveImpl*>(componentImpl));
        }

    } // EASFC
} // Blaze
