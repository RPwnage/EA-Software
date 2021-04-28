/*************************************************************************************************/
/*!
\file


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_AUTHENTICATION_GETTERMSOFSERVICECONTENTBASE_H
#define BLAZE_AUTHENTICATION_GETTERMSOFSERVICECONTENTBASE_H

/*** Include files *******************************************************************************/
#include "authentication/authenticationimpl.h"
#include "authentication/rpc/authenticationslave/gettermsofservicecontent_stub.h"

#include "authentication/util/getusercountryutil.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Authentication
{

class GetTermsOfServiceContentBase
{
public:
    GetTermsOfServiceContentBase(AuthenticationSlaveImpl* componentImpl, const PeerInfo* peerInfo);
    virtual ~GetTermsOfServiceContentBase() {};
    GetTermsOfServiceContentCommandStub::Errors execute(const Command* caller, const GetLegalDocContentRequest& request, GetLegalDocContentResponse& response);

private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;
    const PeerInfo* mPeerInfo;

    GetUserCountryUtil mGetUserCountryUtil;
};

} //Authentication
} //Blaze
#endif /*BLAZE_AUTHENTICATION_GETTERMSOFSERVICECONTENTBASE_H*/
