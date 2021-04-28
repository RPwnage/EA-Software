// (c) Electronic Arts.  All Rights Reserved.

#include "framework/blaze.h"
#include "heatshield/heatshieldslaveimpl.h"
#include "heatshield/tdf/heatshieldtypes.h"
#include "heatshield/rpc/heatshield_defines.h"
#include "perf/rpc/perfslave/perftune_stub.h"
#include "perf/rpc/perfmaster.h"
#include "perfslaveimpl.h"
#include "perf/tdf/perftypes.h"

// shield protobuf includes
#include "eadp/shield/Trusted.grpc.pb.h"

namespace Blaze
{
	namespace Perf
	{

		class PerfTuneCommand : public PerfTuneCommandStub
		{
		public:
			PerfTuneCommand(Message* message, PerfTuneRequest* request, PerfSlaveImpl* componentImpl)
				: PerfTuneCommandStub(message, request), mComponent(componentImpl)
			{ }

			virtual ~PerfTuneCommand() override { }

		private:
			PerfTuneCommandStub::Errors execute() override
			{
				BlazeRpcError error = ERR_OK;

				Blaze::HeatShield::HeatShieldSlave* heatShieldSlave = static_cast<Blaze::HeatShield::HeatShieldSlave*>(
					gController->getComponent(Blaze::HeatShield::HeatShieldSlave::COMPONENT_ID, false));

				if (heatShieldSlave != nullptr)
				{
					UserSession::SuperUserPermissionAutoPtr autoPtr(true);

					if (mRequest.getEnable())
					{
						// clientJoin request
						Blaze::HeatShield::ClientJoinRequest clientJoinReq;
						Blaze::HeatShield::HeatShieldResponse clientJoinResp;
						
						clientJoinReq.setPinSessionId(mRequest.getFlowId());
						clientJoinReq.setFlowType(mRequest.getFlowType());

						error = heatShieldSlave->clientJoin(clientJoinReq, clientJoinResp);

						if (error == Blaze::ERR_OK)
						{
							uint8_t* data = clientJoinResp.getData().getData();
							const Trusted::ClientShieldOnlineModeStatus* mClientShieldOnlineModeStatus = reinterpret_cast<const ::Trusted::ClientShieldOnlineModeStatus*>(data);
							bool enabled = mClientShieldOnlineModeStatus == nullptr ? true : *mClientShieldOnlineModeStatus == Trusted::ClientShieldOnlineModeStatus::ENABLED;
							
							INFO_LOG(THIS_FUNC << "Shield client online mode status from shield back end [ " << enabled << " ] Size [ " << sizeof(*data) << " ]");
							
							NotifyPerfTuneResponse perfTuneResponse;
							perfTuneResponse.setEnable(enabled);
							mComponent->sendNotifyPerfTuneToUserSessionById(gCurrentUserSession->getUserSessionId(), &perfTuneResponse);
						}
					}
					else
					{
						// clientLeave request
						Blaze::HeatShield::ClientLeaveRequest clientLeaveReq;
						Blaze::HeatShield::HeatShieldResponse clientLeaveResp;

						clientLeaveReq.setFlowType(mRequest.getFlowType());

						error = heatShieldSlave->clientLeave(clientLeaveReq, clientLeaveResp);

						// Check if heat shield is disabled for online mode
						if (error == HEAT_SHIELD_ONLINE_MODE_DISABLED)
						{
							NotifyPerfTuneResponse perfTuneResponse;
							perfTuneResponse.setEnable(false);
							mComponent->sendNotifyPerfTuneToUserSessionById(gCurrentUserSession->getUserSessionId(), &perfTuneResponse);

							// Return okay error so client does not stops core shield, it just needs to take the mEnable flag and disable the flow type
							return ERR_OK;
						}
						else if (error == Blaze::ERR_OK)
						{
							uint8_t* data = clientLeaveResp.getData().getData();
							const Trusted::ClientShieldOnlineModeStatus* mClientShieldOnlineModeStatus = reinterpret_cast<const ::Trusted::ClientShieldOnlineModeStatus*>(data);
							bool enabled = mClientShieldOnlineModeStatus == nullptr ? false : *mClientShieldOnlineModeStatus == Trusted::ClientShieldOnlineModeStatus::ENABLED;

							INFO_LOG(THIS_FUNC << "Shield client online mode status from shield back end [ " << enabled << " ] Size [ " << sizeof(*data) << " ]");
							
							NotifyPerfTuneResponse perfTuneResponse;
							perfTuneResponse.setEnable(enabled);
							mComponent->sendNotifyPerfTuneToUserSessionById(gCurrentUserSession->getUserSessionId(), &perfTuneResponse);
						}
					}

					if (error != Blaze::ERR_OK)
					{
						WARN_LOG(THIS_FUNC << "Error occurred calling perf tune [" << ErrorHelp::getErrorName(error) << "].");
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
		DEFINE_PERFTUNE_CREATE()

	} // Perf
} // Blaze
