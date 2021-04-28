
/*************************************************************************************************/
/*!
    \class GetTermsOfServiceContentCommand 

    Get terms of service content for user
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "authentication/gettermsofservicecontentbase.h"
#include "authentication/tdf/authentication.h"
#include "authentication/rpc/authenticationslave/gettermsofservicecontent_stub.h"
#include "authentication/util/authenticationutil.h"

#include "framework/controller/controller.h"

#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"

namespace Blaze
{
namespace Authentication
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

class GetTermsOfServiceContentCommand : public GetTermsOfServiceContentCommandStub
{
public:
    GetTermsOfServiceContentCommand(Message* message, GetLegalDocContentRequest* request, AuthenticationSlaveImpl* componentImpl);

    ~GetTermsOfServiceContentCommand() override {};

private:
    //owned memory.
    GetTermsOfServiceContentBase mGetTermsOfServiceContentBase;
    // States
    GetTermsOfServiceContentCommandStub::Errors execute() override;
};

DEFINE_GETTERMSOFSERVICECONTENT_CREATE()

/*** Public Methods ******************************************************************************/
GetTermsOfServiceContentCommand::GetTermsOfServiceContentCommand(
    Message* message, GetLegalDocContentRequest* request, AuthenticationSlaveImpl* componentImpl)
    : GetTermsOfServiceContentCommandStub(message, request),
    mGetTermsOfServiceContentBase(componentImpl, getPeerInfo())
{
}

/* Private methods *******************************************************************************/
GetTermsOfServiceContentCommandStub::Errors GetTermsOfServiceContentCommand::execute()
{
    return mGetTermsOfServiceContentBase.execute(this, mRequest, mResponse);
}


} // Authentication
} // Blaze
