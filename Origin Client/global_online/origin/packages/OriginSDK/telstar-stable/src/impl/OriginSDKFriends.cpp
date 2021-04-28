#include "stdafx.h"
#include "OriginSDK/OriginSDK.h"
#include "OriginSDK/OriginError.h"
#include "OriginSDKimpl.h"

#include "OriginLSXRequest.h"
#include "OriginLSXEnumeration.h"

namespace Origin
{

OriginErrorT OriginSDK::QueryFriends(OriginUserT user, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
{
	if (user == 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

	if(callback == NULL && pHandle == NULL)
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

	LSXEnumeration<lsx::QueryFriendsT, lsx::QueryFriendsResponseT, OriginFriendT> *req = 
		new LSXEnumeration<lsx::QueryFriendsT, lsx::QueryFriendsResponseT, OriginFriendT>
			(GetService(lsx::FACILITY_FRIENDS), this, &OriginSDK::QueryFriendsConvertData, callback, pContext);

	lsx::QueryFriendsT &r = req->GetRequest();

	r.UserId = user;
	
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

void OriginSDK::ConvertData(OriginFriendT &dest, const lsx::FriendT &src)
{
	dest.UserId = src.UserId;
	dest.PersonaId = src.PersonaId;
	dest.Persona = src.Persona.c_str();
	dest.AvatarId = src.AvatarId.c_str();
    dest.Group = src.Group.c_str();
    dest.GroupId = src.GroupId.c_str();
	dest.PresenceState = (enumPresence)src.Presence;
	dest.FriendState = (enumFriendState)src.State;
	dest.TitleId = src.TitleId.c_str();
	dest.Title = src.Title.c_str();
	dest.MultiplayerId = src.MultiplayerId.c_str();
	dest.Presence = src.RichPresence.c_str();
	dest.GamePresence = src.GamePresence.c_str();
}

OriginErrorT OriginSDK::QueryFriendsConvertData(IEnumerator * /*pEnumerator*/, OriginFriendT *pData, size_t index, size_t count, lsx::QueryFriendsResponseT &response)
{
	if((pData == NULL) && (count > 0))
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

	for(uint32_t i=0; i<count; i++)
	{
		OriginFriendT &destFriend = pData[i];
		lsx::FriendT &srcFriend = response.Friends[i + index];

		ConvertData(destFriend, srcFriend);
	}

	return REPORTERROR(ORIGIN_SUCCESS);
}


OriginErrorT OriginSDK::QueryFriendsSync(OriginUserT user, size_t *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
{
    if (user == 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
	if(pTotalCount == NULL || pHandle == NULL)
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

	LSXEnumeration<lsx::QueryFriendsT, lsx::QueryFriendsResponseT, OriginFriendT> *req = 
		new LSXEnumeration<lsx::QueryFriendsT, lsx::QueryFriendsResponseT, OriginFriendT>
			(GetService(lsx::FACILITY_FRIENDS), this, &OriginSDK::QueryFriendsConvertData);

	lsx::QueryFriendsT &r = req->GetRequest();

	r.UserId = user;

	*pHandle = RegisterResource(OriginResourceContainerT::Enumerator, req);

	// Dispatch the command, and wait for an reply.
	if(req->Execute(timeout, pTotalCount))
	{
		return REPORTERROR(ORIGIN_SUCCESS);
	}
	return REPORTERROR(req->GetErrorCode());
}

OriginErrorT OriginSDK::QueryAreFriends(OriginUserT user, const OriginUserT *pUserList, size_t iUserCount, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
{
    if (user == 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
	if(pUserList == NULL)
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);
	if(pHandle == NULL && callback == NULL)
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

	LSXEnumeration<lsx::QueryAreFriendsT, lsx::QueryAreFriendsResponseT, OriginIsFriendT> *req = 
		new LSXEnumeration<lsx::QueryAreFriendsT, lsx::QueryAreFriendsResponseT, OriginIsFriendT>
			(GetService(lsx::FACILITY_FRIENDS), this, &OriginSDK::QueryAreFriendsConvertData, callback, pContext);

	lsx::QueryAreFriendsT &r = req->GetRequest();

	r.UserId = user;
	r.Friends.assign(pUserList, pUserList + iUserCount);
	
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

OriginErrorT OriginSDK::QueryAreFriendsSync(OriginUserT user, const OriginUserT *pUserList, size_t iUserCount, size_t *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
{
    if (user == 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
	if(pUserList == NULL || pTotalCount == NULL || pHandle == NULL)
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

	LSXEnumeration<lsx::QueryAreFriendsT, lsx::QueryAreFriendsResponseT, OriginIsFriendT> *req = 
		new LSXEnumeration<lsx::QueryAreFriendsT, lsx::QueryAreFriendsResponseT, OriginIsFriendT>
			(GetService(lsx::FACILITY_FRIENDS), this, &OriginSDK::QueryAreFriendsConvertData);

	lsx::QueryAreFriendsT &r = req->GetRequest();

	r.UserId = user;
	r.Friends.assign(pUserList, pUserList + iUserCount);
	
	*pHandle = RegisterResource(OriginResourceContainerT::Enumerator, req);

	// Dispatch the command, and wait for an reply.
	if(req->Execute(timeout, pTotalCount))
	{
		return REPORTERROR(ORIGIN_SUCCESS);
	}
	return REPORTERROR(req->GetErrorCode());
}

OriginErrorT OriginSDK::QueryAreFriendsConvertData(IEnumerator * /*pEnumerator*/, OriginIsFriendT *pData, size_t index, size_t count, lsx::QueryAreFriendsResponseT &response)
{
	if((pData == NULL) && (count > 0))
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

	for(uint32_t i=0; i<count; i++)
	{
		OriginIsFriendT &dest = pData[i];
		lsx::FriendStatusT &src = response.Users[i + index];

		dest.UserId = src.FriendId;
		dest.FriendState = (enumFriendState)src.State;
	}

	return REPORTERROR(ORIGIN_SUCCESS);
}

OriginErrorT OriginSDK::RequestFriend(OriginUserT user, OriginUserT userToAdd, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext)
{
    if (user == 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
    if (userToAdd == 0 || userToAdd == GetDefaultUser())
        return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

    IObjectPtr< LSXRequest<lsx::RequestFriendT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> > req(
        new LSXRequest<lsx::RequestFriendT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
            (GetService(lsx::FACILITY_XMPP), this, &OriginSDK::RequestFriendConvertData, callback, pContext));

    lsx::RequestFriendT &r = req->GetRequest();

    r.UserId = user;
    r.UserToAdd = userToAdd;

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

OriginErrorT OriginSDK::RequestFriendSync(OriginUserT user, OriginUserT userToAdd, uint32_t timeout)
{
    if (user == 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
    if (userToAdd == 0 || userToAdd == GetDefaultUser())
        return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

    IObjectPtr< LSXRequest<lsx::RequestFriendT, lsx::ErrorSuccessT> > req(
        new LSXRequest<lsx::RequestFriendT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_XMPP), this));

    lsx::RequestFriendT &r = req->GetRequest();

    r.UserId = user;
    r.UserToAdd = userToAdd;

    if(req->Execute(timeout))
    {
        return REPORTERROR(req->GetResponse().Code);
    }
    return REPORTERROR(req->GetErrorCode());
}

OriginErrorT OriginSDK::RequestFriendConvertData(IXmlMessageHandler * /*pHandler*/, OriginErrorT &/*item*/, size_t &size, lsx::ErrorSuccessT &response)
{
    size = 0;
    return response.Code;
}

OriginErrorT OriginSDK::RemoveFriend(OriginUserT user, OriginUserT userToRemove, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext)
{
    if (user == 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
    if(userToRemove == 0 || userToRemove == GetDefaultUser())
        return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

    IObjectPtr< LSXRequest<lsx::RemoveFriendT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> > req(
        new LSXRequest<lsx::RemoveFriendT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
            (GetService(lsx::FACILITY_XMPP), this, &OriginSDK::RemoveFriendConvertData, callback, pContext));

    lsx::RemoveFriendT &r = req->GetRequest();

    r.UserId = user;
    r.UserToRemove = userToRemove;

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

OriginErrorT OriginSDK::RemoveFriendSync(OriginUserT user, OriginUserT userToRemove, uint32_t timeout)
{
    if (user == 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
    if(userToRemove == 0 || userToRemove == GetDefaultUser())
        return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

    IObjectPtr< LSXRequest<lsx::RemoveFriendT, lsx::ErrorSuccessT> > req(
        new LSXRequest<lsx::RemoveFriendT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_XMPP), this));

    lsx::RemoveFriendT &r = req->GetRequest();

    r.UserId = user;
    r.UserToRemove = userToRemove;

    if(req->Execute(timeout))
    {
        return REPORTERROR(req->GetResponse().Code);
    }
    return REPORTERROR(req->GetErrorCode());
}

OriginErrorT OriginSDK::RemoveFriendConvertData(IXmlMessageHandler * /*pHandler*/, OriginErrorT &/*item*/, size_t &size, lsx::ErrorSuccessT &response)
{
    size = 0;
    return response.Code;
}

OriginErrorT OriginSDK::AcceptFriendInvite(OriginUserT user, OriginUserT userToAdd, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext)
{
    if (user == 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
	if(userToAdd == 0 || userToAdd == GetDefaultUser())
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

    IObjectPtr< LSXRequest<lsx::AcceptFriendInviteT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> > req(
		new LSXRequest<lsx::AcceptFriendInviteT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
		    (GetService(lsx::FACILITY_XMPP), this, &OriginSDK::AcceptFriendInviteConvertData, callback, pContext));

	lsx::AcceptFriendInviteT &request = req->GetRequest();

	request.UserId = user;
	request.OtherId = userToAdd;

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

OriginErrorT OriginSDK::AcceptFriendInviteSync(OriginUserT user, OriginUserT userToAdd, uint32_t timeout)
{
    if (user == 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
	if(userToAdd == 0 || userToAdd == GetDefaultUser())
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

	IObjectPtr< LSXRequest<lsx::AcceptFriendInviteT, lsx::ErrorSuccessT> > req(
        new LSXRequest<lsx::AcceptFriendInviteT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_XMPP), this));

	lsx::AcceptFriendInviteT &r = req->GetRequest();

	r.UserId = user;
	r.OtherId = userToAdd;

	if(req->Execute(timeout))
	{
		return REPORTERROR(req->GetResponse().Code);
	}
	return REPORTERROR(req->GetErrorCode());
}

OriginErrorT OriginSDK::AcceptFriendInviteConvertData(IXmlMessageHandler * /*pHandler*/, OriginErrorT &/*item*/, size_t &size, lsx::ErrorSuccessT &response)
{
	size = 0;
    return response.Code;
}


}
