#include "stdafx.h"
#include "OriginSDK/OriginSDK.h"
#include "OriginSDK/OriginError.h"
#include "OriginSDKimpl.h"

#include "OriginLSXRequest.h"
#include "OriginLSXEnumeration.h"

namespace Origin
{

OriginErrorT OriginSDK::QueryBlockedUsers(OriginUserT user, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
{
    if ((user == 0) || (user != GetDefaultUser()))
		return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
	if(callback == NULL && pHandle == NULL)
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

	LSXEnumeration<lsx::GetBlockListT, lsx::GetBlockListResponseT, OriginFriendT> *req = 
		new LSXEnumeration<lsx::GetBlockListT, lsx::GetBlockListResponseT, OriginFriendT>
			(GetService(lsx::FACILITY_BLOCKED_USERS), this, &OriginSDK::QueryBlockedUsersConvertData, callback, pContext);

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

OriginErrorT OriginSDK::QueryBlockedUsersConvertData(IEnumerator * /*pEnumerator*/, OriginFriendT *pData, size_t index, size_t count, lsx::GetBlockListResponseT &response)
{
	if((pData == NULL) && (count > 0))
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

	for(size_t i=0; i<count; i++)
	{
		OriginFriendT &destFriend = pData[i];
		lsx::UserT &srcUser = response.User[i + index];

		ConvertData(destFriend, srcUser);
	}

	return REPORTERROR(ORIGIN_SUCCESS);
}

void OriginSDK::ConvertData(OriginFriendT &dest, const lsx::UserT &src)
{
	dest.UserId = src.UserId;
	dest.PersonaId = src.PersonaId;
	dest.Persona = src.EAID.c_str();
	dest.AvatarId = "";
    dest.Group = "";
	dest.GroupId = "";
	dest.PresenceState = ORIGIN_PRESENCE_OFFLINE;
	dest.FriendState = ORIGIN_FRIENDSTATE_NONE;
	dest.TitleId = "";
	dest.Title = "";
	dest.MultiplayerId = "";
	dest.Presence = "";
	dest.GamePresence = "";
}

OriginErrorT OriginSDK::QueryBlockedUsersSync(OriginUserT user, size_t *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
{
    if ((user == 0) || (user != GetDefaultUser()))
		return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
	if(pTotalCount == NULL || pHandle == NULL)
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

	LSXEnumeration<lsx::GetBlockListT, lsx::GetBlockListResponseT, OriginFriendT> *req = 
		new LSXEnumeration<lsx::GetBlockListT, lsx::GetBlockListResponseT, OriginFriendT>
			(GetService(lsx::FACILITY_BLOCKED_USERS), this, &OriginSDK::QueryBlockedUsersConvertData);

	*pHandle = RegisterResource(OriginResourceContainerT::Enumerator, req);

	// Dispatch the command, and wait for a reply.
	if(req->Execute(timeout, pTotalCount))
	{
		return REPORTERROR(ORIGIN_SUCCESS);
	}
	return REPORTERROR(req->GetErrorCode());
}

OriginErrorT OriginSDK::BlockUser(OriginUserT user, OriginUserT userToBlock, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext)
{
    if (user == 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
    if(userToBlock == 0 || userToBlock == GetDefaultUser())
        return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

    IObjectPtr< LSXRequest<lsx::BlockUserT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> > req(
        new LSXRequest<lsx::BlockUserT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
            (GetService(lsx::FACILITY_XMPP), this, &OriginSDK::BlockUserConvertData, callback, pContext));

    lsx::BlockUserT &request = req->GetRequest();

    request.UserId = user;
    request.UserIdToBlock = userToBlock;

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

OriginErrorT OriginSDK::BlockUserSync(OriginUserT user, OriginUserT userToBlock, uint32_t timeout)
{
    if (user == 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
    if(userToBlock == 0 || userToBlock == GetDefaultUser())
        return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

    IObjectPtr< LSXRequest<lsx::BlockUserT, lsx::ErrorSuccessT> > req(
        new LSXRequest<lsx::BlockUserT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_XMPP), this));

    lsx::BlockUserT &request = req->GetRequest();

    request.UserId = user;
    request.UserIdToBlock = userToBlock;

    if(req->Execute(timeout))
    {
        return REPORTERROR(req->GetResponse().Code);
    }
    return REPORTERROR(req->GetErrorCode());
}

OriginErrorT OriginSDK::BlockUserConvertData(IXmlMessageHandler * /*pHandler*/, OriginErrorT & /*item*/, size_t &size, lsx::ErrorSuccessT &response)
{
    size = 0;
    return response.Code;
}



OriginErrorT OriginSDK::UnblockUser(OriginUserT user, OriginUserT userToUnblock, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext)
{
    if (user == 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
    if(userToUnblock == 0 || userToUnblock == GetDefaultUser())
        return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

    IObjectPtr< LSXRequest<lsx::UnblockUserT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> > req(
        new LSXRequest<lsx::UnblockUserT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
            (GetService(lsx::FACILITY_XMPP), this, &OriginSDK::UnblockUserConvertData, callback, pContext));

    lsx::UnblockUserT &request = req->GetRequest();

    request.UserId = user;
    request.UserIdToUnblock = userToUnblock;

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

OriginErrorT OriginSDK::UnblockUserSync(OriginUserT user, OriginUserT userToUnblock, uint32_t timeout)
{
    if (user == 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
    if(userToUnblock == 0 || userToUnblock == GetDefaultUser())
        return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

    IObjectPtr< LSXRequest<lsx::UnblockUserT, lsx::ErrorSuccessT> > req(
        new LSXRequest<lsx::UnblockUserT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_XMPP), this));

    lsx::UnblockUserT &request = req->GetRequest();

    request.UserId = user;
    request.UserIdToUnblock = userToUnblock;

    if(req->Execute(timeout))
    {
        return REPORTERROR(req->GetResponse().Code);
    }
    return REPORTERROR(req->GetErrorCode());
}

OriginErrorT OriginSDK::UnblockUserConvertData(IXmlMessageHandler * /*pHandler*/, OriginErrorT & /*item*/, size_t &size, lsx::ErrorSuccessT &response)
{
    size = 0;
    return response.Code;
}





}