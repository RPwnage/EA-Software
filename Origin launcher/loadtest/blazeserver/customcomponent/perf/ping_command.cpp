// (c) Electronic Arts.  All Rights Reserved.

#include "framework/blaze.h"
#include "heatshield/heatshieldslaveimpl.h"
#include "heatshield/tdf/heatshieldtypes.h"
#include "perf/rpc/perfslave/ping_stub.h"
#include "perf/rpc/perfmaster.h"
#include "perfslaveimpl.h"
#include "perf/tdf/perftypes.h"

namespace Blaze
{
	namespace Perf
	{

		class PingCommand : public PingCommandStub
		{
		public:
			PingCommand(Message* message, PingRequest* request, PerfSlaveImpl* componentImpl)
				: PingCommandStub(message, request), mComponent(componentImpl)
			{ }

			~PingCommand() override { }

		private:

			PingCommandStub::Errors execute() override
			{
				BlazeRpcError error = ERR_OK;

				Blaze::HeatShield::HeatShieldSlave* heatShieldSlave = static_cast<Blaze::HeatShield::HeatShieldSlave*>(
					gController->getComponent(Blaze::HeatShield::HeatShieldSlave::COMPONENT_ID, false));

				if (heatShieldSlave != nullptr)
				{
					UserSession::SuperUserPermissionAutoPtr autoPtr(true);

					Blaze::HeatShield::HeartbeatRequest heartbeatReq;
					Blaze::HeatShield::HeatShieldResponse heartbeatResp;

					heartbeatReq.getData().setData(mRequest.getData().getData(), mRequest.getData().getSize());

					error = heatShieldSlave->heartbeat(heartbeatReq, heartbeatResp);
					if (error == Blaze::ERR_OK)
					{
						NotifyPingResponse pingResponse;
						heartbeatResp.getData().copyInto(pingResponse.getData());
						mComponent->sendNotifyPingResponseToUserSessionById(gCurrentUserSession->getUserSessionId(), &pingResponse);
					}
					else
					{
						WARN_LOG(THIS_FUNC << "Error occurred calling Heat Shield heartbeat [" << ErrorHelp::getErrorName(error) << "].");
						return (error == HEAT_SHIELD_DISABLED ? PERF_FEATURE_DISABLED : PERF_ISSUE_UNKNOWN);
					}
				}

				return commandErrorFromBlazeError(error);
			}

		private:
			// Not owned memory.
			PerfSlaveImpl * mComponent;
		};


		// static factory method impl
		DEFINE_PING_CREATE()

	} // Perf
} // Blaze
