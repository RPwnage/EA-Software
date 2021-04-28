/*************************************************************************************************/
/*!
    \file   getuserids_command.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getuserids_stub.h"

namespace Blaze
{
namespace Arson
{
class GetUserIdsCommand : public GetUserIdsCommandStub
{
public:
    GetUserIdsCommand(
        Message* message, Arson::GetUserIdsRequest* request, ArsonSlaveImpl* componentImpl)
        : GetUserIdsCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~GetUserIdsCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetUserIdsCommandStub::Errors execute() override
    {

        //UserSession::setCurrentUserSessionId(INVALID_USER_SESSION_ID);
        Blaze::BlazeIdList& blazeIdList = mResponse.getUserIds();
        BlazeRpcError err = gUserSetManager->getUserBlazeIds(mRequest.getBlazeObjId(), blazeIdList);

        return commandErrorFromBlazeError(err);

    }

};

DEFINE_GETUSERIDS_CREATE()

} //Arson
} //Blaze
