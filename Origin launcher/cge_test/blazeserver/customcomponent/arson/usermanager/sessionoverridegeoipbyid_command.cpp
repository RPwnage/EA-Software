
/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/sessionoverridegeoipbyid_stub.h"

namespace Blaze
{
namespace Arson
{
class SessionOverrideGeoIPByIdCommand : public SessionOverrideGeoIPByIdCommandStub
{
public:
    SessionOverrideGeoIPByIdCommand(Message* message, Blaze::GeoLocationData* request, ArsonSlaveImpl* componentImpl)
        : SessionOverrideGeoIPByIdCommandStub(message, request),
        mComponent(componentImpl)
    {
    }
    ~SessionOverrideGeoIPByIdCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    SessionOverrideGeoIPByIdCommandStub::Errors execute() override
    {
        BlazeRpcError err = Blaze::ERR_OK;
        
        err = gUserSessionManager->sessionOverrideGeoIPById(mRequest);

        return arsonErrorFromBlazeError(err);
    }

    static Errors arsonErrorFromBlazeError(BlazeRpcError error);
};

DEFINE_SESSIONOVERRIDEGEOIPBYID_CREATE()

    SessionOverrideGeoIPByIdCommandStub::Errors SessionOverrideGeoIPByIdCommand::arsonErrorFromBlazeError(BlazeRpcError error)
    {
        Errors result = ERR_SYSTEM;
        switch (error)
        {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case ERR_AUTHORIZATION_REQUIRED: result = ERR_AUTHORIZATION_REQUIRED; break;
        case USER_ERR_USER_NOT_FOUND: result = ARSON_ERR_USER_NOT_FOUND; break; 
        default:
            {
                //got an error not defined in rpc definition, log it
                TRACE_LOG("[SessionOverrideGeoIPByIdCommand].arsonErrorFromBlazeError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
                result = ERR_SYSTEM;
                break;
            }
        };

        return result;
    }

} //Arson
} //Blaze
