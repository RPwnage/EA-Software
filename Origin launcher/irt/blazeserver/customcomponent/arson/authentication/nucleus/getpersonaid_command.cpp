
/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "authentication/util/getpersonautil.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getpersonaid_stub.h"

namespace Blaze
{
namespace Arson
{
class GetPersonaIdCommand : public GetPersonaIdCommandStub
{
public:
    GetPersonaIdCommand(Message* message, Blaze::Arson::GetPersonaIdREQ* request, ArsonSlaveImpl* componentImpl)
        : GetPersonaIdCommandStub(message, request),
        mComponent(componentImpl)
    {
    }
    ~GetPersonaIdCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetPersonaIdCommandStub::Errors execute() override
    {
        Authentication::GetPersonaUtil util(getPeerInfo());

        Authentication::PersonaInfo personaInfo;
        BlazeRpcError error = util.getPersona(mRequest.getPersonaName(), mRequest.getNamespaceName(), nullptr, &personaInfo);
        if(error != ERR_OK)
        {
            return(commandErrorFromBlazeError(error));
        }

        mResponse.setPersonaId(personaInfo.getPersonaId());
        return ERR_OK;
    }

    static Errors arsonErrorFromAuthenticationError(BlazeRpcError error);
};

DEFINE_GETPERSONAID_CREATE()

} //Arson
} //Blaze
