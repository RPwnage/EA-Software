
/*** Include Files *******************************************************************************/

// framework includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
// arson includes
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/arsonauxlookuplocalusersessionid_stub.h"

namespace Blaze
{
namespace Arson
{
class ArsonAuxlookupLocalUserSessionIdCommand : public ArsonAuxlookupLocalUserSessionIdCommandStub
{
public:
    ArsonAuxlookupLocalUserSessionIdCommand(Message* message, ArsonSlaveImpl* componentImpl)
        : ArsonAuxlookupLocalUserSessionIdCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~ArsonAuxlookupLocalUserSessionIdCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;


    ArsonAuxlookupLocalUserSessionIdCommandStub::Errors execute() override
    {
        
        BLAZE_TRACE_LOG(Log::USER, "[LookupUserSessionIdCommand].start()");

        BlazeRpcError rc = Blaze::ERR_OK;
        UserSessionIdList ids;

        const Blaze::UserSessionMasterPtrByIdMap& map = gUserSessionMaster->getOwnedUserSessionsMap();

        for(Blaze::UserSessionMasterPtrByIdMap::const_iterator it = map.begin(); it != map.end(); ++it)
        {

            UserSessionId sessionId = it->second->getSessionId();

            if(sessionId != INVALID_USER_SESSION_ID)
                mResponse.getUsersessionidList().push_back(sessionId);
        }
        return commandErrorFromBlazeError(rc);

    }
};

DEFINE_ARSONAUXLOOKUPLOCALUSERSESSIONID_CREATE()


} //Arson
} //Blaze
