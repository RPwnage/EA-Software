
/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/util/locales.h"

#include "authentication/authenticationimpl.h"
#include "authentication/tdf/authentication.h"
#include "authentication/rpc/authenticationslave/checklegaldoc_stub.h"
#include "authentication/util/legaldocscheckutil.h"


#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"
#include "framework/util/locales.h"
#include "framework/controller/controller.h"

namespace Blaze
{
namespace Authentication
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/


class CheckLegalDocCommand : public CheckLegalDocCommandStub
{
public:
    CheckLegalDocCommand(Message* message, CheckLegalDocRequest* request, AuthenticationSlaveImpl* componentImpl);
    CheckLegalDocCommandStub::Errors execute() override;
private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponentImpl;

    // Owned memory
    LegalDocsCheckUtil mLegalDocsCheckUtil;
};


CheckLegalDocCommand::CheckLegalDocCommand(
    Message* message, CheckLegalDocRequest* request, AuthenticationSlaveImpl* componentImpl)
    : CheckLegalDocCommandStub(message, request),
    mComponentImpl(componentImpl),
    mLegalDocsCheckUtil(getPeerInfo())
{
}

CheckLegalDocCommandStub::Errors CheckLegalDocCommand::execute()
{
    bool accepted;
    BlazeRpcError err = mLegalDocsCheckUtil.checkForLegalDoc(gCurrentUserSession->getAccountId(), mRequest.getLegalDocUri(), accepted);

    mResponse.setAccepted(accepted);

    return commandErrorFromBlazeError(err);
}

DEFINE_CHECKLEGALDOC_CREATE()

} // Authentication
} // Blaze
