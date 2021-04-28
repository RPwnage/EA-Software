
/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "authentication/util/getpersonautil.h"


// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getpersonastatus_stub.h"

namespace Blaze
{
namespace Arson
{
class GetPersonaStatusCommand : public GetPersonaStatusCommandStub
{
public:
    GetPersonaStatusCommand(Message* message, Blaze::Arson::GetPersonaStatusREQ* request, ArsonSlaveImpl* componentImpl)
        : GetPersonaStatusCommandStub(message, request),
        mComponent(componentImpl)
    {
    }
    ~GetPersonaStatusCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetPersonaStatusCommandStub::Errors execute() override
    {
        Authentication::GetPersonaUtil util(getPeerInfo());

        Authentication::PersonaInfo personaInfo;
        BlazeRpcError error = util.getPersona(mRequest.getPersonaId(), &personaInfo);
        if (error != ERR_OK)
        {
            return(commandErrorFromBlazeError(error));
        }

        mResponse.setPersonaStatus(personaInfo.getStatus());

        return ERR_OK;
    }

    static Errors arsonErrorFromAuthenticationError(BlazeRpcError error);
};

DEFINE_GETPERSONASTATUS_CREATE()

} //Arson
} //Blaze
