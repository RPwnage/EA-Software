
/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "authentication/util/getpersonautil.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getpersona_stub.h"

namespace Blaze
{
namespace Arson
{
class GetPersonaCommand : public GetPersonaCommandStub
{
public:
    GetPersonaCommand(Message* message, Blaze::Arson::GetPersonaREQ* request, ArsonSlaveImpl* componentImpl)
        : GetPersonaCommandStub(message, request),
        mComponent(componentImpl)
    {
    }
    ~GetPersonaCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetPersonaCommandStub::Errors execute() override
    {
        Authentication::GetPersonaUtil util(getPeerInfo());

        Authentication::PersonaInfo personaInfo;
        AccountId accountId;
        BlazeRpcError error = util.getPersona(mRequest.getPersonaId(), &accountId, &personaInfo);
        if (error != ERR_OK)
        {
            return(commandErrorFromBlazeError(error));
        }

        mResponse.setDisplayName(personaInfo.getDisplayName());
        mResponse.setNamespaceName(personaInfo.getNameSpaceName());
        mResponse.setAccountId(accountId);
        mResponse.setPersonaStatus(personaInfo.getStatus());

        return ERR_OK;
    }

    static Errors arsonErrorFromAuthenticationError(BlazeRpcError error);
};

DEFINE_GETPERSONA_CREATE()

} //Arson
} //Blaze
