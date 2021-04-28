/*!
 * @file        setstatus_command.cpp
 * @brief       This is the implementation of the thirdpartyoptin setstatus RPC
 * @copyright   Electronic Arts Inc.
 */

// Blaze includes
#include "framework/blaze.h"
#include "authentication/util/authenticationutil.h" // requires EA::TDF
#include "framework/connection/outboundhttpservice.h"
#include "framework/connection/socketutil.h"
#include "preferencecenter/rpc/preferencecenterslave.h"
#include "preferencecenter/tdf/preferencecenter.h"
#include "preferencecenter/tdf/preferencecenter_base.h"

// Component includes
#include "thirdpartyoptin/rpc/thirdpartyoptinslave/setstatus_stub.h"
#include "thirdpartyoptin/tdf/thirdpartyoptintypes.h"
#include "thirdpartyoptinslaveimpl.h"

namespace Blaze
{
    namespace ThirdPartyOptIn
    {
        //! This is the implementation of the thirdpartyoptin setstatus RPC
        class SetStatusCommand : public SetStatusCommandStub
        {
        public:
            //! Constructor
            SetStatusCommand(Message* message, ThirdPartyOptInSetStatusRequest* request, ThirdPartyOptInSlaveImpl* componentImpl)
                : SetStatusCommandStub(message, request), mComponent(componentImpl)
            { }

            //! Destructor
            virtual ~SetStatusCommand() { }

        private:

            //! Entry point for the RPC
            SetStatusCommandStub::Errors execute();

        private:
            // Not owned memory.
            ThirdPartyOptInSlaveImpl* mComponent;
        };

        //=============================================================================
        //! Static factory method impl
        DEFINE_SETSTATUS_CREATE()

        //=============================================================================
        //! Entry point for the RPC
        SetStatusCommandStub::Errors SetStatusCommand::execute()
        {
            BlazeRpcError blazeRpcError = ERR_SYSTEM;
			PreferenceCenter::PutPreferenceUserByPidIdRequest putPreferenceUserByPidIdRequest;

            // Check if the component is disabled
            const ThirdPartyOptInConfig& config = mComponent->getConfig();

            if (config.getEnableThirdPartyOptIn())
            {
				TeamId teamId = 0;
				LeagueId leagueId = 0;

				if (mRequest.isTeamIdSet() && mRequest.isLeagueIdSet())
				{
					teamId = mRequest.getTeamId();
					leagueId = mRequest.getLeagueId();
					
					BlazeRpcError uploadBlazeRpcError = mComponent->uploadAddedPreference(this, teamId, leagueId);

					if (uploadBlazeRpcError == ERR_OK)
					{
						blazeRpcError = ERR_OK;
					}
					else
					{
						ERR_LOG(THIS_FUNC << "uploadAddedPreference failed, blazeRpcError[" << uploadBlazeRpcError << "]");
						// Update it to a general error
						blazeRpcError = THIRDPARTYOPTIN_ERR_GENERAL_ERROR;
					}
				}
				else
				{
					ERR_LOG(THIS_FUNC << "mRequest.isTeamIdSet()" << mRequest.isTeamIdSet() << "mRequest.isLeagueIdSet()" << mRequest.isLeagueIdSet());
					blazeRpcError = THIRDPARTYOPTIN_ERR_INPUTOPTINSTATUS_INVALID;
				}
            }
            else
            {
                TRACE_LOG(THIS_FUNC << "component is disabled");
				blazeRpcError = THIRDPARTYOPTIN_ERR_COMPONENT_DISABLED;
            }

            return commandErrorFromBlazeError(blazeRpcError);
        }

    } // namespace ThirdPartyOptIn
} // namespace Blaze
