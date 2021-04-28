#include "stdafx.h"
#include "OriginSDK/OriginSDK.h"
#include "OriginSDK/OriginError.h"
#include "OriginSDKimpl.h"

#include "OriginLSXRequest.h"

namespace Origin
{

OriginErrorT OriginSDK::SendInvite(OriginUserT user, const OriginUserT * pUserList, size_t userCount, const OriginCharT * pMessage, uint32_t timeout, OriginErrorSuccessCallback callback, void *pContext)
{
	if (user == 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

	if(pUserList == NULL || userCount <= 0 || callback == NULL)
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

    IObjectPtr< LSXRequest<lsx::SendInviteT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> > req(
		new LSXRequest<lsx::SendInviteT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
			(GetService(lsx::FACILITY_XMPP), this, &OriginSDK::SendInviteConvertData, callback, pContext));

	lsx::SendInviteT &r = req->GetRequest();

	r.UserId = user;

	r.Invitees.reserve(userCount);
	r.Invitees.assign(pUserList, pUserList + userCount);

	r.Invitation = pMessage ? pMessage : "";
	
	if(req->ExecuteAsync(timeout))
	{
		return REPORTERROR(ORIGIN_SUCCESS);
	}
	else
	{
		OriginErrorT err = req->GetErrorCode();
		return REPORTERROR(err);
	}
}


OriginErrorT OriginSDK::SendInviteSync(OriginUserT user, const OriginUserT * pUserList, size_t userCount, const OriginCharT * pMessage, uint32_t timeout)
{
    if (user == 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

	if(pUserList == NULL || userCount <= 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

	IObjectPtr< LSXRequest<lsx::SendInviteT, lsx::ErrorSuccessT> > req(
        new LSXRequest<lsx::SendInviteT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_XMPP), this));

	lsx::SendInviteT &r = req->GetRequest();

	r.UserId = user;

	r.Invitees.reserve(userCount);
	r.Invitees.assign(pUserList, pUserList + userCount);

	r.Invitation = pMessage ? pMessage : "";
	
	if(req->Execute(timeout))
	{
		return REPORTERROR(req->GetResponse().Code);
	}
	return REPORTERROR(req->GetErrorCode());
}

OriginErrorT OriginSDK::SendInviteConvertData(IXmlMessageHandler * /*pHandler*/, OriginErrorT & /*item*/, size_t &size, lsx::ErrorSuccessT &response)
{
	size = 0;

	return response.Code;
}


OriginErrorT OriginSDK::AcceptInvite(OriginUserT user, OriginUserT other, uint32_t timeout, OriginErrorSuccessCallback callback, void *pContext)
{
    if (user == 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

    if(other == ((OriginUserT)NULL) || callback == NULL)
        return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

    IObjectPtr< LSXRequest<lsx::AcceptInviteT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> > req(
        new LSXRequest<lsx::AcceptInviteT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
            (GetService(lsx::FACILITY_XMPP), this, &OriginSDK::AcceptInviteConvertData, callback, pContext));

    lsx::AcceptInviteT &r = req->GetRequest();

    r.UserId = user;
    r.OtherId = other;

    if(req->ExecuteAsync(timeout))
    {
        return REPORTERROR(ORIGIN_SUCCESS);
    }
    else
    {
        OriginErrorT err = req->GetErrorCode();
        return REPORTERROR(err);
    }
}

OriginErrorT OriginSDK::AcceptInviteSync(OriginUserT user, OriginUserT other, uint32_t timeout)
{
    if (user == 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

    if(other == 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

    IObjectPtr< LSXRequest<lsx::AcceptInviteT, lsx::ErrorSuccessT> > req(
        new LSXRequest<lsx::AcceptInviteT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_XMPP), this));

    lsx::AcceptInviteT &r = req->GetRequest();

    r.UserId = user;
    r.OtherId = other;

    if(req->Execute(timeout))
    {
        return REPORTERROR(req->GetResponse().Code);
    }
    return REPORTERROR(req->GetErrorCode());
}

OriginErrorT OriginSDK::AcceptInviteConvertData(IXmlMessageHandler * /*pHandler*/, OriginErrorT & /*item*/, size_t &size, lsx::ErrorSuccessT &response)
{
    size = 0;

    return response.Code;
}





}