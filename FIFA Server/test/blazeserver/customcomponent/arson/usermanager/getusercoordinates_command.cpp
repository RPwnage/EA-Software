
/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getusercoordinates_stub.h"

namespace Blaze
{
namespace Arson
{
class GetUserCoordinatesCommand : public GetUserCoordinatesCommandStub
{
public:
    GetUserCoordinatesCommand(Message* message, Blaze::Arson::GetCoordsREQ* request, ArsonSlaveImpl* componentImpl)
        : GetUserCoordinatesCommandStub(message, request),
        mComponent(componentImpl)
    {
    }
    ~GetUserCoordinatesCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetUserCoordinatesCommandStub::Errors execute() override
    {
        if(gCurrentLocalUserSession == nullptr)
        {
            ERR_LOG("[GetUserCoordinatesCommandStub].execute: No current usersession.");
            return ERR_SYSTEM;
        }

        mResponse.setLatitude(gCurrentLocalUserSession->getLatitude());
        mResponse.setLongitude(gCurrentLocalUserSession->getLongitude());

        return ERR_OK;
    }
};

DEFINE_GETUSERCOORDINATES_CREATE()

} //Arson
} //Blaze
