#include "stdafx.h"
#include "OriginSDK/OriginSDK.h"
#include "OriginSDK/OriginError.h"
#include "OriginSDKimpl.h"

#include "OriginLSXRequest.h"
#include "OriginLSXEnumeration.h"

namespace Origin
{

    OriginErrorT OriginSDK::QueryGroup(OriginUserT user, const OriginCharT * groupId, OriginEnumerationCallbackFunc callback, void * pContext, uint32_t timeout, OriginHandleT * handle)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if (callback == NULL && handle == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        LSXEnumeration<lsx::QueryGroupT, lsx::QueryGroupResponseT, OriginFriendT> *req =
            new LSXEnumeration<lsx::QueryGroupT, lsx::QueryGroupResponseT, OriginFriendT>
                (GetService(lsx::FACILITY_XMPP), this, &OriginSDK::QueryGroupConvertData, callback, pContext);

        lsx::QueryGroupT &r = req->GetRequest();

        r.UserId = user;
        r.GroupId = groupId == NULL ? "" : groupId;

        OriginHandleT h = RegisterResource(OriginResourceContainerT::Enumerator, req);

        if (handle != NULL)
        {
            *handle = h;
        }


        if (req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            DestroyHandle(h);
            if (handle != NULL) *handle = NULL;
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::QueryGroupSync(OriginUserT user, const OriginCharT * groupId, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT * handle)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
        if (pTotalCount == NULL || handle == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        LSXEnumeration<lsx::QueryGroupT, lsx::QueryGroupResponseT, OriginFriendT> *req =
            new LSXEnumeration<lsx::QueryGroupT, lsx::QueryGroupResponseT, OriginFriendT>
            (GetService(lsx::FACILITY_FRIENDS), this, &OriginSDK::QueryGroupConvertData);

        lsx::QueryGroupT &r = req->GetRequest();

        r.UserId = user;
        r.GroupId = groupId == NULL ? "" : groupId;

        *handle = RegisterResource(OriginResourceContainerT::Enumerator, req);

        // Dispatch the command, and wait for an reply.
        if (req->Execute(timeout, pTotalCount))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::QueryGroupConvertData(IEnumerator *pEnumerator, OriginFriendT *pMembers, size_t index, size_t count, lsx::QueryGroupResponseT &response)
    {
        if ((pMembers == NULL) && (count > 0))
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        for (uint32_t i = 0; i < count; i++)
        {
            OriginFriendT &destFriend = pMembers[i];
            lsx::FriendT &srcFriend = response.Members[i + index];

            ConvertData(destFriend, srcFriend);
        }

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::GetGroupInfo(OriginUserT user, const OriginCharT * groupId, OriginGroupInfoCallback callback, void *pContext, uint32_t timeout)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if (callback == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::GetGroupInfoT, lsx::GroupInfoT, OriginGroupInfoT, OriginGroupInfoCallback> > req(
                new LSXRequest<lsx::GetGroupInfoT, lsx::GroupInfoT, OriginGroupInfoT, OriginGroupInfoCallback>
                    (GetService(lsx::FACILITY_XMPP), this, &OriginSDK::GetGroupInfoConvertData, callback, pContext));

        lsx::GetGroupInfoT &r = req->GetRequest();

        r.UserId = user;
        r.GroupId = groupId == NULL ? "" : groupId;

        if (req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::GetGroupInfoSync(OriginUserT user, const OriginCharT * groupId, uint32_t timeout, OriginGroupInfoT * groupInfo, OriginHandleT * handle)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if (handle == NULL || groupInfo == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::GetGroupInfoT, lsx::GroupInfoT> > req(
            new LSXRequest<lsx::GetGroupInfoT, lsx::GroupInfoT>
                (GetService(lsx::FACILITY_XMPP), this));

        lsx::GetGroupInfoT &r = req->GetRequest();

        r.UserId = user;
        r.GroupId = groupId == NULL ? "" : groupId;

        if (req->Execute(timeout))
        {
            ConvertData(req, *groupInfo, req->GetResponse(), true);

            *handle = req->CopyDataStoreToHandle();

            return REPORTERROR(ORIGIN_SUCCESS);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    void OriginSDK::ConvertData(IXmlMessageHandler *pHandler, OriginGroupInfoT &dest, const lsx::GroupInfoT &src, bool bCopyStrings)
    {
        dest.GroupName = ConvertData(pHandler, src.GroupName, bCopyStrings);
        dest.GroupId = ConvertData(pHandler, src.GroupId, bCopyStrings);
        dest.GroupType = (enumGroupType)src.GroupType;
        dest.bUserCanInviteNewMembers = src.CanInviteNewMembers;
        dest.bUserCanRemoveMembers = src.CanRemoveMembers;
        dest.bUserCanSendGameInvites = src.CanSendGameInvites;
        dest.MaxGroupSize = src.MaxGroupSize;
    }

    OriginErrorT OriginSDK::GetGroupInfoConvertData(IXmlMessageHandler *pHandler, OriginGroupInfoT &groupInfo, size_t &size, lsx::GroupInfoT &response)
    {
        size = sizeof(OriginGroupInfoT);

        ConvertData(pHandler, groupInfo, response, false);

        return ORIGIN_SUCCESS;
    }


    OriginErrorT OriginSDK::SendGroupGameInvite(OriginUserT user, const OriginUserT * pUserList, size_t userCount, const OriginCharT * pMessage, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
        if ((pUserList == NULL && userCount != 0) || callback == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::SendGroupGameInviteT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback> > req(
            new LSXRequest<lsx::SendGroupGameInviteT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback>
            (GetService(lsx::FACILITY_XMPP), this, &OriginSDK::SendGroupGameInviteConvertData, callback, pContext));

        lsx::SendGroupGameInviteT &r = req->GetRequest();

        r.UserId = user;
        r.Invitees.assign(pUserList, pUserList + userCount);
        r.Message = pMessage == NULL ? "" : pMessage;
        
        if (req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::SendGroupGameInviteSync(OriginUserT user, const OriginUserT * pUserList, size_t userCount, const OriginCharT * pMessage, uint32_t timeout)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
        if (pUserList == NULL && userCount != 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::SendGroupGameInviteT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::SendGroupGameInviteT, lsx::ErrorSuccessT>
                (GetService(lsx::FACILITY_XMPP), this));

        lsx::SendGroupGameInviteT &r = req->GetRequest();

        r.UserId = user;
        r.Invitees.assign(pUserList, pUserList + userCount);
        r.Message = pMessage == NULL ? "" : pMessage;
        
        if (req->Execute(timeout))
        {
            return REPORTERROR(req->GetResponse().Code);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::SendGroupGameInviteConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response)
    {
        return response.Code;
    }


    OriginErrorT OriginSDK::CreateGroup(OriginUserT user, const OriginCharT *groupName, enumGroupType type, OriginGroupInfoCallback callback, void *pContext, uint32_t timeout)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if (callback == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::CreateGroupT, lsx::GroupInfoT, OriginGroupInfoT, OriginGroupInfoCallback> > req(
            new LSXRequest<lsx::CreateGroupT, lsx::GroupInfoT, OriginGroupInfoT, OriginGroupInfoCallback>
                (GetService(lsx::FACILITY_XMPP), this, &OriginSDK::CreateGroupConvertData, callback, pContext));

        lsx::CreateGroupT &r = req->GetRequest();

        r.UserId = user;
        r.GroupName = groupName == NULL ? "" : groupName;
        r.GroupType = (lsx::GroupTypeT)type;

        if (req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::CreateGroupSync(OriginUserT user, const OriginCharT *groupName, enumGroupType type, uint32_t timeout, OriginGroupInfoT * groupInfo, OriginHandleT * handle)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if (handle == NULL || groupInfo == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::CreateGroupT, lsx::GroupInfoT> > req(
            new LSXRequest<lsx::CreateGroupT, lsx::GroupInfoT>
                (GetService(lsx::FACILITY_XMPP), this));

        lsx::CreateGroupT &r = req->GetRequest();

        r.UserId = user;
        r.GroupName = groupName == NULL ? "" : groupName;
        r.GroupType = (lsx::GroupTypeT)type;

        if (req->Execute(timeout))
        {
            ConvertData(req, *groupInfo, req->GetResponse(), true);

            *handle = req->CopyDataStoreToHandle();

            return REPORTERROR(ORIGIN_SUCCESS);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::CreateGroupConvertData(IXmlMessageHandler *pHandler, OriginGroupInfoT &groupInfo, size_t &size, lsx::GroupInfoT &response)
    {
        ConvertData(pHandler, groupInfo, response, false);

        return ORIGIN_SUCCESS;
    }


    OriginErrorT OriginSDK::EnterGroup(OriginUserT user, const OriginCharT *groupId, OriginGroupInfoCallback callback, void *pContext, uint32_t timeout)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if (callback == NULL || groupId == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::EnterGroupT, lsx::GroupInfoT, OriginGroupInfoT, OriginGroupInfoCallback> > req(
            new LSXRequest<lsx::EnterGroupT, lsx::GroupInfoT, OriginGroupInfoT, OriginGroupInfoCallback>
                (GetService(lsx::FACILITY_XMPP), this, &OriginSDK::EnterGroupConvertData, callback, pContext));

        lsx::EnterGroupT &r = req->GetRequest();

        r.UserId = user;
        r.GroupId = groupId == NULL ? "" : groupId;
        
        if (req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::EnterGroupSync(OriginUserT user, const OriginCharT *groupId, uint32_t timeout, OriginGroupInfoT * groupInfo, OriginHandleT *handle)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if (handle == NULL || groupId == NULL || groupInfo == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::EnterGroupT, lsx::GroupInfoT> > req(
            new LSXRequest<lsx::EnterGroupT, lsx::GroupInfoT>
                (GetService(lsx::FACILITY_XMPP), this));

        lsx::EnterGroupT &r = req->GetRequest();

        r.UserId = user;
        r.GroupId = groupId;
        
        if (req->Execute(timeout))
        {
            ConvertData(req, *groupInfo, req->GetResponse(), true);

            *handle = req->CopyDataStoreToHandle();

            return REPORTERROR(ORIGIN_SUCCESS);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::EnterGroupConvertData(IXmlMessageHandler *pHandler, OriginGroupInfoT &groupInfo, size_t &size, lsx::GroupInfoT &response)
    {
        ConvertData(pHandler, groupInfo, response, false);

        return ORIGIN_SUCCESS;
    }


    OriginErrorT OriginSDK::LeaveGroup(OriginUserT user, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if (callback == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::LeaveGroupT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback> > req(
            new LSXRequest<lsx::LeaveGroupT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback>
                (GetService(lsx::FACILITY_XMPP), this, &OriginSDK::LeaveGroupConvertData, callback, pContext));

        lsx::LeaveGroupT &r = req->GetRequest();

        r.UserId = user;

        if (req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::LeaveGroupSync(OriginUserT user, uint32_t timeout)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        IObjectPtr< LSXRequest<lsx::LeaveGroupT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::LeaveGroupT, lsx::ErrorSuccessT>
                (GetService(lsx::FACILITY_XMPP), this));

        lsx::LeaveGroupT &r = req->GetRequest();

        r.UserId = user;
        
        if (req->Execute(timeout))
        {
            return REPORTERROR(req->GetResponse().Code);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::LeaveGroupConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response)
    {
        return response.Code;
    }


    OriginErrorT OriginSDK::InviteUsersToGroup(OriginUserT user, const OriginUserT *users, OriginSizeT userCount, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if (callback == NULL || users == NULL || userCount == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::InviteUsersToGroupT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback> > req(
            new LSXRequest<lsx::InviteUsersToGroupT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback>
                (GetService(lsx::FACILITY_XMPP), this, &OriginSDK::InviteUsersToGroupConvertData, callback, pContext));

        lsx::InviteUsersToGroupT &r = req->GetRequest();

        r.UserId = user;
        r.FriendId.assign(users, users + userCount);

        if (req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::InviteUsersToGroupSync(OriginUserT user, const OriginUserT *users, OriginSizeT userCount, uint32_t timeout)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if (users == NULL || userCount == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::InviteUsersToGroupT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::InviteUsersToGroupT, lsx::ErrorSuccessT>
                (GetService(lsx::FACILITY_XMPP), this));

        lsx::InviteUsersToGroupT &r = req->GetRequest();

        r.UserId = user;
        r.FriendId.assign(users, users + userCount);

        if (req->Execute(timeout))
        {
            return REPORTERROR(req->GetResponse().Code);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::InviteUsersToGroupConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response)
    {
        return response.Code;
    }


    OriginErrorT OriginSDK::RemoveUsersFromGroup(OriginUserT user, const OriginUserT *users, OriginSizeT userCount, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if (callback == NULL || users == NULL || userCount == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::RemoveUsersFromGroupT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback> > req(
            new LSXRequest<lsx::RemoveUsersFromGroupT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback>
                (GetService(lsx::FACILITY_XMPP), this, &OriginSDK::RemoveUsersFromGroupConvertData, callback, pContext));

        lsx::RemoveUsersFromGroupT &r = req->GetRequest();

        r.UserId = user;
        r.FriendId.assign(users, users + userCount);

        if (req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::RemoveUsersFromGroupSync(OriginUserT user, const OriginUserT *users, OriginSizeT userCount, uint32_t timeout)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if (users == NULL || userCount == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::RemoveUsersFromGroupT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::RemoveUsersFromGroupT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_XMPP), this));

        lsx::RemoveUsersFromGroupT &r = req->GetRequest();

        r.UserId = user;
        r.FriendId.assign(users, users + userCount);

        if (req->Execute(timeout))
        {
            return REPORTERROR(req->GetResponse().Code);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::RemoveUsersFromGroupConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response)
    {
        return response.Code;
    }


    OriginErrorT OriginSDK::EnableVoip(bool bEnable, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout)
    {
        if (callback == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::EnableVoipT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback> > req(
            new LSXRequest<lsx::EnableVoipT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback>
                (GetService(lsx::FACILITY_XMPP), this, &OriginSDK::EnableVoipConvertData, callback, pContext));

        lsx::EnableVoipT &r = req->GetRequest();

        r.Enable = bEnable;

        if (req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::EnableVoipSync(bool bEnable, uint32_t timeout)
    {
        IObjectPtr< LSXRequest<lsx::EnableVoipT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::EnableVoipT, lsx::ErrorSuccessT>
                (GetService(lsx::FACILITY_XMPP), this));

        lsx::EnableVoipT &r = req->GetRequest();

        r.Enable = bEnable;
        
        if (req->Execute(timeout))
        {
            return REPORTERROR(req->GetResponse().Code);
        }
        return REPORTERROR(req->GetErrorCode());

    }

    OriginErrorT OriginSDK::EnableVoipConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response)
    {
        return response.Code;
    }


    OriginErrorT OriginSDK::GetVoipStatus(OriginVoipStatusCallback callback, void * pContext, uint32_t timeout)
    {
        if (callback == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::GetVoipStatusT, lsx::GetVoipStatusResponseT, OriginVoipStatusT, OriginVoipStatusCallback> > req(
            new LSXRequest<lsx::GetVoipStatusT, lsx::GetVoipStatusResponseT, OriginVoipStatusT, OriginVoipStatusCallback>
                (GetService(lsx::FACILITY_XMPP), this, &OriginSDK::GetVoipStatusConvertData, callback, pContext));

        if (req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::GetVoipStatusSync(OriginVoipStatusT * status, uint32_t timeout)
    {
        if (status == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::GetVoipStatusT, lsx::GetVoipStatusResponseT> > req(
            new LSXRequest<lsx::GetVoipStatusT, lsx::GetVoipStatusResponseT>
                (GetService(lsx::FACILITY_XMPP), this));

        if (req->Execute(timeout))
        {
            status->Available = req->GetResponse().Available;
            status->Active = req->GetResponse().Active;

            return REPORTERROR(ORIGIN_SUCCESS);
        }
        return REPORTERROR(req->GetErrorCode());

    }

    OriginErrorT OriginSDK::GetVoipStatusConvertData(IXmlMessageHandler *pHandler, OriginVoipStatusT &status, size_t &size, lsx::GetVoipStatusResponseT &response)
    {
        size = sizeof(OriginVoipStatusT);

        status.Active = response.Active;
        status.Available = response.Available;

        return ORIGIN_SUCCESS;
    }

    OriginErrorT OriginSDK::MuteUser(bool bMute, OriginUserT userId, const OriginCharT *groupId, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout)
    {
        if (callback == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::MuteUserT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback> > req(
            new LSXRequest<lsx::MuteUserT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback>
            (GetService(lsx::FACILITY_XMPP), this, &OriginSDK::MuteUserConvertData, callback, pContext));

        lsx::MuteUserT &request = req->GetRequest();

        request.bMute = bMute;
        request.UserId = userId;
        request.GroupId = groupId == NULL ? "" : groupId;

        if (req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::MuteUserSync(bool bMute, OriginUserT userId, const OriginCharT *groupId, uint32_t timeout)
    {
        IObjectPtr< LSXRequest<lsx::MuteUserT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::MuteUserT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_XMPP), this));

        lsx::MuteUserT &request = req->GetRequest();

        request.bMute = bMute;
        request.UserId = userId;
        request.GroupId = groupId == NULL ? "" : groupId;

        if (req->Execute(timeout))
        {
            return REPORTERROR(req->GetResponse().Code);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::MuteUserConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response)
    {
        return response.Code;
    }

                 
    OriginErrorT OriginSDK::QueryMuteState(const OriginCharT *groupId, OriginEnumerationCallbackFunc callback, void * pContext, uint32_t timeout, OriginHandleT * handle)
    {
        if (callback == NULL && handle == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        LSXEnumeration<lsx::QueryMuteStateT, lsx::QueryMuteStateResponseT, OriginMuteStateT> *req =
            new LSXEnumeration<lsx::QueryMuteStateT, lsx::QueryMuteStateResponseT, OriginMuteStateT>
            (GetService(lsx::FACILITY_XMPP), this, &OriginSDK::QueryMuteStateConvertData, callback, pContext);

        lsx::QueryMuteStateT &r = req->GetRequest();

        r.GroupId = groupId == NULL ? "" : groupId;

        OriginHandleT h = RegisterResource(OriginResourceContainerT::Enumerator, req);

        if (handle != NULL)
        {
            *handle = h;
        }

        if (req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            DestroyHandle(h);
            if (handle != NULL) *handle = NULL;
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::QueryMuteStateSync(const OriginCharT *groupId, OriginSizeT * total, uint32_t timeout, OriginHandleT * handle)
    {
        if (total == NULL || handle == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        LSXEnumeration<lsx::QueryMuteStateT, lsx::QueryMuteStateResponseT, OriginMuteStateT> *req =
            new LSXEnumeration<lsx::QueryMuteStateT, lsx::QueryMuteStateResponseT, OriginMuteStateT>
            (GetService(lsx::FACILITY_FRIENDS), this, &OriginSDK::QueryMuteStateConvertData);

        lsx::QueryMuteStateT &r = req->GetRequest();

        r.GroupId = groupId == NULL ? "" : groupId;

        *handle = RegisterResource(OriginResourceContainerT::Enumerator, req);

        // Dispatch the command, and wait for an reply.
        if (req->Execute(timeout, total))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        return REPORTERROR(req->GetErrorCode());
    }
    
    OriginErrorT OriginSDK::QueryMuteStateConvertData(IEnumerator *pEnumerator, OriginMuteStateT *pMuteState, size_t index, size_t count, lsx::QueryMuteStateResponseT &response)
    {
		if ((pMuteState == NULL) && (count > 0))
			return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

		for (uint32_t i = 0; i < count; i++)
		{
			OriginMuteStateT &dest = pMuteState[i];
			lsx::MuteStateT &src = response.MuteStateArray[i + index];

			dest.State = static_cast<enumMuteState>(src.State);
			dest.UserId = src.UserId;
		}

		return REPORTERROR(ORIGIN_SUCCESS);
	}

}