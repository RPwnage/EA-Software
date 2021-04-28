/*************************************************************************************************/
/*!
    \file   setmembertournamentstatus_command.cpp

    $Header$
    $Change$
    $DateTime$

    \attention
    (c) Electronic Arts Inc. 2012
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class SetMemberTournamentStatusCommand

    Set tournament status for a member

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "osdkseasonalplay/rpc/osdkseasonalplayslave/setmembertournamentstatus_stub.h"

// global includes

// framework includes
#include "framework/connection/selector.h"
#include "framework/database/dbscheduler.h"

// osdkseasonalplay includes
#include "osdkseasonalplayslaveimpl.h"
#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"

namespace Blaze
{
    namespace OSDKSeasonalPlay
    {

        class SetMemberTournamentStatusCommand : public SetMemberTournamentStatusCommandStub  
        {
        public:
            SetMemberTournamentStatusCommand(Message* message, SetMemberTournamentStatusRequest* request, OSDKSeasonalPlaySlaveImpl* componentImpl)
                : SetMemberTournamentStatusCommandStub(message,request), mComponent(componentImpl)
            { }

            virtual ~SetMemberTournamentStatusCommand() { }

        /* Private methods *******************************************************************************/
        private:

            SetMemberTournamentStatusCommandStub::Errors execute()
            {
                TRACE_LOG("[SetMemberTournamentStatusCommand:" << this << "].execute()");

                Blaze::BlazeRpcError error = Blaze::ERR_OK;
				error = mComponent->setMemberTournamentStatus(
					mRequest.getMemberId(),
					mRequest.getMemberType(),
					mRequest.getTournamentStatus());
			
				return SetMemberTournamentStatusCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            OSDKSeasonalPlaySlaveImpl* mComponent;
        };


        // static factory method impl
        SetMemberTournamentStatusCommandStub* SetMemberTournamentStatusCommandStub::create(
			Message* message,
			SetMemberTournamentStatusRequest* request,
			OSDKSeasonalPlaySlave* componentImpl)
        {
            return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "SetMemberTournamentStatusCommand") SetMemberTournamentStatusCommand(message, request, static_cast<OSDKSeasonalPlaySlaveImpl*>(componentImpl));
        }

    } // OSDKSeasonalPlay
} // Blaze
