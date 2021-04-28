#include "stdafx.h"
#include "OriginSDK/OriginSDK.h"
#include "OriginSDK/OriginError.h"
#include "OriginSDKimpl.h"

#include "OriginLSXRequest.h"
#include "OriginLSXEnumeration.h"

namespace Origin
{

OriginErrorT OriginSDK::SubscribePresence(OriginUserT user, const OriginUserT *otherUsers, size_t userCount, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout)
{
    if (user == 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

    if (otherUsers == NULL || userCount <= 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

    IObjectPtr< LSXRequest<lsx::SubscribePresenceT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback > > req(
        new LSXRequest<lsx::SubscribePresenceT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
            (GetService(lsx::FACILITY_PRESENCE), this, &OriginSDK::SubscribePresenceConvertData, callback, pContext));

    lsx::SubscribePresenceT &r = req->GetRequest();

    r.UserId = user;

    r.Users.reserve(userCount);
    r.Users.assign(otherUsers, otherUsers + userCount);

    if (req->ExecuteAsync(timeout))
    {
        return REPORTERROR(ORIGIN_SUCCESS);
    }
    else
    {
        return REPORTERROR(req->GetErrorCode());
    }
}

OriginErrorT OriginSDK::SubscribePresenceSync(OriginUserT user, const OriginUserT *otherUsers, size_t userCount, uint32_t timeout)
{
    if (user == 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

	if(otherUsers == NULL || userCount <= 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

	IObjectPtr< LSXRequest<lsx::SubscribePresenceT, lsx::ErrorSuccessT> > req(
        new LSXRequest<lsx::SubscribePresenceT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_PRESENCE), this));

	lsx::SubscribePresenceT &r = req->GetRequest();

	r.UserId = user;

	r.Users.reserve(userCount);
	r.Users.assign(otherUsers, otherUsers + userCount);
	
	if(req->Execute(timeout))
	{
		return REPORTERROR(req->GetResponse().Code);
	}
	return REPORTERROR(req->GetErrorCode());
}

OriginErrorT OriginSDK::SubscribePresenceConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response)
{
    size = 0;
    item = response.Code;
    return response.Code;
}


OriginErrorT OriginSDK::UnsubscribePresence(OriginUserT user, const OriginUserT *otherUsers, size_t userCount, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout)
{
    if (user == 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

	if(otherUsers == NULL || userCount <= 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

	IObjectPtr< LSXRequest<lsx::UnsubscribePresenceT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> > req(
        new LSXRequest<lsx::UnsubscribePresenceT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
            (GetService(lsx::FACILITY_PRESENCE), this, &OriginSDK::UnsubscribePresenceConvertData, callback, pContext));

	lsx::UnsubscribePresenceT &presence = req->GetRequest();

	presence.UserId = user;

	presence.Users.reserve(userCount);
	presence.Users.assign(otherUsers, otherUsers + userCount);
	
    if (req->ExecuteAsync(timeout))
    {
        return REPORTERROR(ORIGIN_SUCCESS);
    }
    else
    {
        return REPORTERROR(req->GetErrorCode());
    }
}

OriginErrorT OriginSDK::UnsubscribePresenceSync(OriginUserT user, const OriginUserT *otherUsers, size_t userCount, uint32_t timeout)
{
    if (user == 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

    if (otherUsers == NULL || userCount <= 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

    IObjectPtr< LSXRequest<lsx::UnsubscribePresenceT, lsx::ErrorSuccessT> > req(
        new LSXRequest<lsx::UnsubscribePresenceT, lsx::ErrorSuccessT>
        (GetService(lsx::FACILITY_PRESENCE), this));

    lsx::UnsubscribePresenceT &r = req->GetRequest();

    r.UserId = user;

    r.Users.reserve(userCount);
    r.Users.assign(otherUsers, otherUsers + userCount);

    if (req->Execute(timeout))
    {
        return REPORTERROR(req->GetResponse().Code);
    }
    return REPORTERROR(req->GetErrorCode());
}

OriginErrorT OriginSDK::UnsubscribePresenceConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response)
{
    size = 0;
    item = response.Code;
    return response.Code;
}

OriginErrorT OriginSDK::QueryPresence(OriginUserT user, const OriginUserT *otherUsers, size_t userCount, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
{
    if (user == 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
	if(callback == NULL && pHandle == NULL)
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

	LSXEnumeration<lsx::QueryPresenceT, lsx::QueryPresenceResponseT, OriginFriendT> *req = 
		new LSXEnumeration<lsx::QueryPresenceT, lsx::QueryPresenceResponseT, OriginFriendT>
			(GetService(lsx::FACILITY_PRESENCE), this, &OriginSDK::QueryPresenceConvertData, callback, pContext);

	lsx::QueryPresenceT &r = req->GetRequest();

	r.UserId = user;
	r.Users.assign(otherUsers, otherUsers + userCount);

	OriginHandleT h = RegisterResource(OriginResourceContainerT::Enumerator, req);
	
	if(pHandle != NULL)
	{
		*pHandle = h;
	}

	// Dispatch the command.
	if(req->ExecuteAsync(timeout))
	{
		return REPORTERROR(ORIGIN_SUCCESS);
	}
	else
	{
		OriginErrorT err = req->GetErrorCode();
        DestroyHandle(h);
        if (pHandle != NULL) *pHandle = NULL;        
        return REPORTERROR(err);
	}
}

OriginErrorT OriginSDK::QueryPresenceSync(OriginUserT user, const OriginUserT *otherUsers, size_t userCount, size_t * pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
{
    if (user == 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
	if(pHandle == NULL || 
		(otherUsers == NULL && userCount>0) ||
		pTotalCount == NULL)
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

	LSXEnumeration<lsx::QueryPresenceT, lsx::QueryPresenceResponseT, OriginFriendT> *req = 
        new LSXEnumeration<lsx::QueryPresenceT, lsx::QueryPresenceResponseT, OriginFriendT>
            (GetService(lsx::FACILITY_PRESENCE), this, &OriginSDK::QueryPresenceConvertData);

	lsx::QueryPresenceT &r = req->GetRequest();

	r.UserId = user;
	r.Users.assign(otherUsers, otherUsers + userCount);

	*pHandle = RegisterResource(OriginResourceContainerT::Enumerator, req);

	// Dispatch the command, and wait for an reply.
	if(req->Execute(timeout, pTotalCount))
	{
		return REPORTERROR(ORIGIN_SUCCESS);
	}
	return REPORTERROR(req->GetErrorCode());
}

OriginErrorT OriginSDK::QueryPresenceConvertData(IEnumerator * /*pEnumerator*/, OriginFriendT *pData, size_t index, size_t count, lsx::QueryPresenceResponseT &response)
{
	if((pData == NULL) && (count > 0))
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

	for(uint32_t i=0; i<count; i++)
	{
		OriginFriendT &destFriend = pData[i];
		const lsx::FriendT &srcFriend = response.Friends[i + index];

		ConvertData(destFriend, srcFriend);
	}

	return REPORTERROR(ORIGIN_SUCCESS);
}

OriginErrorT OriginSDK::SetPresence(OriginUserT user,
                                    enumPresence presence,
                                    const OriginCharT *pPresenceString,
                                    const OriginCharT *pGamePresenceString,
                                    const OriginCharT *pSession,
                                    OriginErrorSuccessCallback callback,
                                    void * pContext, 
									uint32_t timeout)
{
    if (user == 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

    IObjectPtr< LSXRequest<lsx::SetPresenceT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback> > req(
        new LSXRequest<lsx::SetPresenceT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback>
            (GetService(lsx::FACILITY_PRESENCE), this, &OriginSDK::SetPresenceConvertData, callback, pContext));

    lsx::SetPresenceT &r = req->GetRequest();

    r.UserId = user;
    r.Presence = (lsx::PresenceT)presence;
    r.RichPresence = pPresenceString != NULL ? pPresenceString : "";
    r.GamePresence = pGamePresenceString != NULL ? pGamePresenceString : "";
    r.SessionId = pSession != NULL ? pSession : "";

    if (req->ExecuteAsync(timeout))
    {
        return REPORTERROR(ORIGIN_SUCCESS);
    }
    else
    {
        return REPORTERROR(req->GetErrorCode());
    }
}

OriginErrorT OriginSDK::SetPresenceSync(OriginUserT user,
                                    enumPresence presence,
                                    const OriginCharT *pPresenceString,
                                    const OriginCharT *pGamePresenceString,
                                    const OriginCharT *pSession,
									uint32_t timeout)
{
    if (user == 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

	IObjectPtr< LSXRequest<lsx::SetPresenceT, lsx::ErrorSuccessT> > req(
        new LSXRequest<lsx::SetPresenceT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_PRESENCE), this));

	lsx::SetPresenceT &r = req->GetRequest();

	r.UserId = user;
	r.Presence = (lsx::PresenceT)presence;
	r.RichPresence = pPresenceString != NULL ? pPresenceString : "";
	r.GamePresence = pGamePresenceString != NULL ? pGamePresenceString : "";
	r.SessionId = pSession != NULL ? pSession : "";

	if (req->Execute(timeout))
	{
		return REPORTERROR(req->GetResponse().Code);
	}
	else
	{
		return REPORTERROR(req->GetErrorCode());
	}
}

OriginErrorT OriginSDK::SetPresenceConvertData(IXmlMessageHandler * /*pHandler*/, OriginErrorT & /*item*/, size_t &size, lsx::ErrorSuccessT &response)
{
    size = 0;
    return response.Code;
}


OriginErrorT OriginSDK::GetPresence(OriginUserT user, OriginGetPresenceCallback callback, void * pContext, uint32_t timeout)
{
    if (user == 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
    
    IObjectPtr< LSXRequest<lsx::GetPresenceT, lsx::GetPresenceResponseT, OriginGetPresenceT, OriginGetPresenceCallback> > req(
        new LSXRequest<lsx::GetPresenceT, lsx::GetPresenceResponseT, OriginGetPresenceT, OriginGetPresenceCallback>
            (GetService(lsx::FACILITY_PRESENCE), this, &OriginSDK::GetPresenceConvertData, callback, pContext));

    lsx::GetPresenceT &r = req->GetRequest();

    r.UserId = user;

    if (req->ExecuteAsync(timeout))
    {
        return REPORTERROR(ORIGIN_SUCCESS);
    }
    else
    {
        return REPORTERROR(req->GetErrorCode());
    }
}

OriginErrorT OriginSDK::GetPresenceSync(OriginUserT user, 
										enumPresence * pPresence, 
										OriginCharT *pPresenceString, size_t presenceStringLen, 
										OriginCharT *pGamePresenceString, size_t gamePresenceStringLen, 
										OriginCharT *pSessionString, size_t sessionStringLen, 
										uint32_t timeout)
{
    if (user == 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
	if(pPresence == NULL)
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

	IObjectPtr< LSXRequest<lsx::GetPresenceT, lsx::GetPresenceResponseT> > req(
        new LSXRequest<lsx::GetPresenceT, lsx::GetPresenceResponseT>
            (GetService(lsx::FACILITY_PRESENCE), this));

	lsx::GetPresenceT &r = req->GetRequest();

	r.UserId = user;

	if(req->Execute(timeout))
	{
		const lsx::GetPresenceResponseT &pr = req->GetResponse();

		*pPresence = (enumPresence)pr.Presence;
		if(pPresenceString != NULL)
		{
			strncpy(pPresenceString, pr.RichPresence.c_str(), presenceStringLen-1);
			pPresenceString[presenceStringLen-1] = 0;
		}
		if(pGamePresenceString != NULL)
		{
            strncpy(pGamePresenceString, pr.GamePresence.c_str(), gamePresenceStringLen-1);
			pGamePresenceString[gamePresenceStringLen-1] = 0;
		}
		if(pSessionString != NULL)
		{
            strncpy(pSessionString, pr.SessionId.c_str(), sessionStringLen-1);
			pSessionString[sessionStringLen-1] = 0;
		}
		return REPORTERROR(ORIGIN_SUCCESS);
	}
	else
	{
		return REPORTERROR(req->GetErrorCode());
	}
}

OriginErrorT OriginSDK::GetPresenceConvertData(IXmlMessageHandler *pHandler, OriginGetPresenceT &presence, size_t &size, lsx::GetPresenceResponseT &response)
{
    presence.UserId = response.UserId;
    presence.PresenceState = (enumPresence)response.Presence;
    presence.Presence = response.RichPresence.c_str();
    presence.GamePresence = response.GamePresence.c_str();
    presence.SessionInfo = response.SessionId.c_str();

    presence.Title = response.Title.c_str();
    presence.TitleId = response.TitleId.c_str();
    presence.MultiplayerId = response.MultiplayerId.c_str();

    presence.Group = response.Group.c_str();
    presence.GroupId = response.GroupId.c_str();

    return ORIGIN_SUCCESS;
}

OriginErrorT OriginSDK::SetPresenceVisibility(OriginUserT user, bool visible, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout)
{
    if (user == 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

    IObjectPtr< LSXRequest<lsx::SetPresenceVisibilityT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> > req(
        new LSXRequest<lsx::SetPresenceVisibilityT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback >
            (GetService(lsx::FACILITY_PRESENCE), this, &OriginSDK::SetPresenceVisibilityConvertData, callback, pContext));

    lsx::SetPresenceVisibilityT &r = req->GetRequest();

    r.UserId = user;
    r.Visible = visible;

    if (req->ExecuteAsync(timeout))
    {
        return REPORTERROR(ORIGIN_SUCCESS);
    }
    else
    {
        return REPORTERROR(req->GetErrorCode());
    }
}

OriginErrorT OriginSDK::SetPresenceVisibilitySync(OriginUserT user, bool visible, uint32_t timeout)
{
    if (user == 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

	IObjectPtr< LSXRequest<lsx::SetPresenceVisibilityT, lsx::ErrorSuccessT> > req(
        new LSXRequest<lsx::SetPresenceVisibilityT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_PRESENCE), this));

	lsx::SetPresenceVisibilityT &r = req->GetRequest();

	r.UserId = user;
	r.Visible = visible;

	if(req->Execute(timeout))
	{
		return REPORTERROR(req->GetResponse().Code);
	}
	return REPORTERROR(req->GetErrorCode());
}

OriginErrorT OriginSDK::SetPresenceVisibilityConvertData(IXmlMessageHandler * /*pHandler*/, OriginErrorT & /*item*/, size_t &size, lsx::ErrorSuccessT &response)
{
    size = 0;
    return response.Code;
}


OriginErrorT OriginSDK::GetPresenceVisibility(OriginUserT user, OriginGetPresenceVisibilityCallback callback, void * pContext, uint32_t timeout)
{
    if (user == 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

    IObjectPtr< LSXRequest<lsx::GetPresenceVisibilityT, lsx::GetPresenceVisibilityResponseT, bool, OriginGetPresenceVisibilityCallback> > req(
        new LSXRequest<lsx::GetPresenceVisibilityT, lsx::GetPresenceVisibilityResponseT, bool, OriginGetPresenceVisibilityCallback>
            (GetService(lsx::FACILITY_PRESENCE), this, &OriginSDK::GetPresenceVisibilityConvertData, callback, pContext));

    lsx::GetPresenceVisibilityT &r = req->GetRequest();

    r.UserId = user;

    if (req->ExecuteAsync(timeout))
    {
        return REPORTERROR(ORIGIN_SUCCESS);
    }
    else
    {
        return REPORTERROR(req->GetErrorCode());
    }
}

OriginErrorT OriginSDK::GetPresenceVisibilitySync(OriginUserT user, bool * visible, uint32_t timeout)
{
    if (user == 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

	if(visible == NULL)
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

	IObjectPtr< LSXRequest<lsx::GetPresenceVisibilityT, lsx::GetPresenceVisibilityResponseT> > req(
        new LSXRequest<lsx::GetPresenceVisibilityT, lsx::GetPresenceVisibilityResponseT>
            (GetService(lsx::FACILITY_PRESENCE), this));

	lsx::GetPresenceVisibilityT &r = req->GetRequest();

	r.UserId = user;
	
	if(req->Execute(timeout))
	{
		lsx::GetPresenceVisibilityResponseT &gpvr = req->GetResponse();

		*visible = gpvr.Visible;

		return REPORTERROR(ORIGIN_SUCCESS);
	}
	return REPORTERROR(req->GetErrorCode());
}

OriginErrorT OriginSDK::GetPresenceVisibilityConvertData(IXmlMessageHandler * /*pHandler*/, bool & pVisibility, size_t &size, lsx::GetPresenceVisibilityResponseT &response)
{
    pVisibility = response.Visible;
    size = 1;
    return ORIGIN_SUCCESS;
}

}
