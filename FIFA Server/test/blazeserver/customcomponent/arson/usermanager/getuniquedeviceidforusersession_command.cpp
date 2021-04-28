
/*** Include Files *******************************************************************************/

// framework includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getuniquedeviceidforusersession_stub.h"

namespace Blaze
{
    namespace Arson
    {
        class GetUniqueDeviceIdForUserSessionCommand : public GetUniqueDeviceIdForUserSessionCommandStub
        {
        public:
            GetUniqueDeviceIdForUserSessionCommand(Message* message, Blaze::Arson::GetUniqueDeviceIdForUserSessionRequest* request, ArsonSlaveImpl* componentImpl)
                :GetUniqueDeviceIdForUserSessionCommandStub(message, request),
                mComponent(componentImpl)
            {
            }

            ~GetUniqueDeviceIdForUserSessionCommand() override { }

        private:
            // Not owned memory
            ArsonSlaveImpl* mComponent;


            GetUniqueDeviceIdForUserSessionCommandStub::Errors execute() override
            {

                Blaze::UserSessionId uid = mRequest.getUserSessionId();
                eastl::string deviceId;
                BlazeRpcError err = Blaze::ERR_OK;

                BLAZE_TRACE_LOG(Log::USER, "[GetUniqueDeviceIdForUserSessionCommand].start()");

                deviceId = gUserSessionManager->getUniqueDeviceId(uid);

                BLAZE_TRACE_LOG(Log::USER, "[GetUniqueDeviceIdForUserSessionCommand].end: get device Id " << deviceId << "for usersessionId " << uid);

                mResponse.setUniqueDeviceId(deviceId.c_str());

                return arsonErrorFromBlazeError(err);

            }

            static Errors arsonErrorFromBlazeError(BlazeRpcError error);
        };

        DEFINE_GETUNIQUEDEVICEIDFORUSERSESSION_CREATE()

        GetUniqueDeviceIdForUserSessionCommandStub::Errors GetUniqueDeviceIdForUserSessionCommand::arsonErrorFromBlazeError(BlazeRpcError error)
        {
            Errors result = ERR_SYSTEM;
            switch (error)
            {
            case ERR_OK: result = ERR_OK; break;
            case ERR_SYSTEM: result = ERR_SYSTEM; break;
            case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
            case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
            case ERR_COMPONENT_NOT_FOUND: result = ARSON_ERR_COMPONENT_NOT_FOUND; break; 
            case USER_ERR_SESSION_NOT_FOUND: result = ARSON_USER_ERR_SESSION_NOT_FOUND; break;
            default:
                {
                    //got an error not defined in rpc definition, log it
                    TRACE_LOG("[GetUniqueDeviceIdForUserSessionCommand].arsonErrorFromBlazeError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
                    result = ERR_SYSTEM;
                    break;
                }
            };

            return result;
        }

    } //Arson
} //Blaze
