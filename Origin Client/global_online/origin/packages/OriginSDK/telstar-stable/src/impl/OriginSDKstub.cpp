// OriginSDK.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "OriginSDK/OriginSDK.h"
#include "impl/OriginSDKimpl.h"
#include "impl/OriginDebugFunctions.h"
#include "impl/OriginErrorFunctions.h"
#include "impl/OriginLSXMessage.h"

#include <LSX.h>
#include <LSXReader.h>
#include <LSXWriter.h>
#include <ReaderCommon.h>
#include <RapidXmlParser.h>


using namespace Origin;

#ifdef __cplusplus
extern "C"
{
#endif

// This will go to a status structure.

OriginErrorT ORIGIN_SDK_API OriginStartup(int32_t flags, uint16_t lsxPort, OriginStartupInputT *pInput, OriginStartupOutputT *pOutput)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginStartup entered");

    OriginErrorT err = ORIGIN_SUCCESS;

    if(!OriginSDK::Exists() || (flags & ORIGIN_FLAG_ALLOW_MULTIPLE_SDK_INSTANCES))
    {
        // In order to uniquely identify what offer started the game we have a couple of sources.

        // The EAConnectionId trumps ContentId trumps the game passed in possibly hard coded pInput->ContentId.
        // Unless the game specifies ORIGIN_FLAG_FORCE_CONTENT_ID.

        OriginStartupInputT input = *pInput;

        const char *pContentId = getenv("ContentId");
        const char *pConnectionId = getenv("EAConnectionId");
        const char *pLsxPort = getenv("EALsxPort");

        if(pContentId || pConnectionId)
        {
            if((flags & ORIGIN_FLAG_FORCE_CONTENT_ID) == 0)
            {
                input.ContentId = (pConnectionId != NULL && pConnectionId[0] != '\0') ? pConnectionId : pContentId;
            }
        }

        if (lsxPort == 0 && pLsxPort != NULL)
        {
            lsxPort = (uint16_t)atoi(pLsxPort);
        }

        err = OriginSDK::Create(flags, lsxPort, input, pOutput);

        // NOTE: We only destroy the SDK if there is an error. A warning will keep the SDK alive.
        // This may cause some issues...
        if (err < ORIGIN_SUCCESS)
        {
            OriginShutdown();
        }

        if(pOutput)
        {
            if(flags & ORIGIN_FLAG_FORCE_CONTENT_ID)
            {
                pOutput->ContentId = input.ContentId;
                pOutput->ProductId = "";
            }
            else
            {
                pOutput->ContentId = pContentId != NULL ? pContentId : input.ContentId;
                pOutput->ProductId = (pConnectionId != NULL && pConnectionId[0] != '\0') ? pConnectionId : "";
            }
        }
    }
    else
    {
        err = OriginSDK::Get()->AddRef();

        if(pOutput != NULL)
        {
            pOutput->Version = OriginSDK::Get()->GetOriginVersion();
            pOutput->Instance = reinterpret_cast<OriginInstanceT>(OriginSDK::Get());
        }
    }

    return REPORTERROR(err);
}

OriginErrorT ORIGIN_SDK_API OriginSelectInstance(OriginInstanceT instance)
{
    return OriginSDK::SetActiveInstance(instance);
}

OriginErrorT ORIGIN_SDK_API OriginRequestTicket(OriginUserT user, OriginTicketCallback callback, void * pcontext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginRequestTicket entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->RequestTicket(user, callback, pcontext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginRequestTicketSync(OriginUserT user, OriginHandleT *ticket, size_t *length)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginRequestTicketSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->RequestTicket(user, ticket, length);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginRequestAuthCodeSync(OriginUserT user, const OriginCharT *clientId, OriginHandleT *authcode, OriginSizeT *length, const OriginCharT *scope, const bool appendAuthSource)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginRequestAuthCodeSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->RequestAuthCodeSync(user, clientId, scope, authcode, length, appendAuthSource);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginRequestAuthCode(OriginUserT user, const OriginCharT *clientId, OriginAuthCodeCallback callback, void * pcontext, uint32_t timeout, const OriginCharT *scope, const bool appendAuthSource)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginRequestAuthCode entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->RequestAuthCode(user, clientId, scope, callback, pcontext, timeout, appendAuthSource);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetSetting(enumSettings settingId, OriginSettingCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetSetting entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetSetting(settingId, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetSettingSync(enumSettings settingId, OriginCharT *pSetting, size_t *length)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetSettingSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetSettingSync(settingId, pSetting, length);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetSettings(OriginSettingsCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetSettings entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetSettings(callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetSettingsSync(OriginSettingsT* info, OriginHandleT* handle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetSettingsSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetSettingsSync(info, handle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}
OriginErrorT ORIGIN_SDK_API OriginGetGameInfo(enumGameInfo gameInfoId, OriginSettingCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetGameInfo entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetGameInfo(gameInfoId, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetGameInfoSync(enumGameInfo gameInfoId, OriginCharT *pInfo, size_t *length)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetGameInfoSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetGameInfoSync(gameInfoId, pInfo, length);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetAllGameInfo(OriginGameInfoCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetAllGameInfo entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetAllGameInfo(callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetAllGameInfoSync(OriginGameInfoT* info, OriginHandleT* handle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetAllGameInfoSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetAllGameInfoSync(info, handle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginDestroyHandle(OriginHandleT handle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginDestroyHandle entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->DestroyHandle(handle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShutdown()
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShutdown entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->Release();
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginUpdate()
{
    REPORTDEBUG(ORIGIN_LEVEL_4, "OriginUpdate entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->Update();
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginUserT ORIGIN_SDK_API OriginGetDefaultUser()
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetDefaultUser entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetDefaultUser();
    }
    return 0;
}

OriginUserT ORIGIN_SDK_API OriginGetUser(int userIndex)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetUser entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetUserId(userIndex);
    }
    return 0;
}

OriginUserT ORIGIN_SDK_API OriginGetDefaultPersona()
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetDefaultPersona entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetDefaultPersona();
    }
    return 0;
}

OriginUserT ORIGIN_SDK_API OriginGetPersona(int userIndex)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetPersona entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetPersonaId(userIndex);
    }
    return 0;
}

OriginErrorT ORIGIN_SDK_API OriginCheckOnline(int8_t *pbOnline)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginCheckOnline entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->CheckOnline(pbOnline);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGoOnline(uint32_t timeout, OriginErrorSuccessCallback callback, void * context)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGoOnline entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GoOnline(timeout, callback, context);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetUTCTime(OriginTimeT * time)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetUTCTime entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetUTCTime(time);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}


OriginErrorT ORIGIN_SDK_API OriginBroadcastCheckStatus(int8_t *pbStatus)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginCheckOnline entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->BroadcastCheckStatus(pbStatus);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginBroadcastStart()
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginBroadcastStart entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->BroadcastStart();
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginBroadcastStop()
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginBroadcastStop entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->BroadcastStop();
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginSendGameMessage(const OriginCharT * gameId, const OriginCharT * message, OriginErrorSuccessCallback callback, void * pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSendGameMessage entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->SendGameMessage(gameId, message, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

const char ORIGIN_SDK_API * OriginGetErrorInfo(OriginErrorT err)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetErrorInfo entered");

    ErrorRecord *pRecord = GetErrorInfo(err);

    if(pRecord)
    {
        return pRecord->name;
    }
    else
    {
        return NULL;
    }
}

const char ORIGIN_SDK_API * OriginGetErrorDescription(OriginErrorT err)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetErrorDescription entered");

    return GetErrorDescription(err);
}



OriginErrorT OriginCheckPermission(OriginUserT user, enumPermission permission)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginCheckPermission entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->CheckPermission(user, permission);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

/// Profile Functions

OriginErrorT OriginGetProfileSync(uint32_t userIndex, uint32_t timeout, OriginProfileT *pProfile, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetProfileSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetProfileSync(userIndex, timeout, pProfile, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetProfile(uint32_t userIndex, uint32_t timeout, OriginResourceCallback callback, void *pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetProfile entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetProfile(userIndex, timeout, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}


/// Presence Functions
OriginErrorT OriginSubscribePresence(OriginUserT user, const OriginUserT *otherUsers, OriginSizeT userCount, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSubscribePresenceSync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->SubscribePresence(user, otherUsers, userCount, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT OriginSubscribePresenceSync(OriginUserT user, const OriginUserT *otherUsers, OriginSizeT userCount, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSubscribePresenceSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->SubscribePresenceSync(user, otherUsers, userCount, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT OriginUnsubscribePresence(OriginUserT user, const OriginUserT *otherUsers, OriginSizeT userCount, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginUnsubscribePresence entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->UnsubscribePresence(user, otherUsers, userCount, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);

}

OriginErrorT OriginUnsubscribePresenceSync(OriginUserT user, const OriginUserT *otherUsers, OriginSizeT userCount, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginUnsubscribePresenceSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->UnsubscribePresenceSync(user, otherUsers, userCount, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);

}

OriginErrorT OriginQueryPresence(OriginUserT user, const OriginUserT *otherUsers, OriginSizeT userCount, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryPresence entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryPresence(user, otherUsers, userCount, callback, pContext, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT OriginQueryPresenceSync(OriginUserT user, const OriginUserT *otherUsers, OriginSizeT userCount, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryPresenceSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryPresenceSync(user, otherUsers, userCount, pTotalCount, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT OriginSetPresence(OriginUserT user, enumPresence presence, const OriginCharT *pPresenceString, const OriginCharT* pGamePresenceString, const OriginCharT *pSessionString, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSetPresence entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->SetPresence(user, presence, pPresenceString, pGamePresenceString, pSessionString, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT OriginSetPresenceSync(OriginUserT user, enumPresence presence, const OriginCharT *pPresenceString, const OriginCharT* pGamePresenceString, const OriginCharT *pSessionString, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSetPresenceSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->SetPresenceSync(user, presence, pPresenceString, pGamePresenceString, pSessionString, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT OriginGetPresence(OriginUserT user, OriginGetPresenceCallback callback, void * pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetPresence entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetPresence(user, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT OriginGetPresenceSync(OriginUserT user, enumPresence * pPresence, OriginCharT *pPresenceString, OriginSizeT presenceStringLen, OriginCharT *pGamePresenceString, OriginSizeT gamePresenceStringLen, OriginCharT *pSessionString, OriginSizeT sessionStringLen, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetPresenceSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetPresenceSync(user, pPresence, pPresenceString, presenceStringLen, pGamePresenceString, gamePresenceStringLen, pSessionString, sessionStringLen, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginSetPresenceVisibility(OriginUserT user, bool bVisibility, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSetPresenceVisibilitySync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->SetPresenceVisibility(user, bVisibility, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginSetPresenceVisibilitySync(OriginUserT user, bool visible, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSetPresenceVisibilitySync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->SetPresenceVisibilitySync(user, visible, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetPresenceVisibility(OriginUserT user, OriginGetPresenceVisibilityCallback callback, void *pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetPresenceVisibility entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetPresenceVisibility(user, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetPresenceVisibilitySync(OriginUserT user, bool * visible, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetPresenceVisibility entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetPresenceVisibilitySync(user, visible, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}



// Friends functions

OriginErrorT ORIGIN_SDK_API OriginQueryFriends(OriginUserT user, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryFriends entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryFriends(user, callback, pContext, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryFriendsSync(OriginUserT user, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryFriendsSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryFriendsSync(user, pTotalCount, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryAreFriends(OriginUserT user, const OriginUserT *pUserList, OriginSizeT iUserCount, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryAreFriends entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryAreFriends(user, pUserList, iUserCount, callback, pContext, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryAreFriendsSync(OriginUserT user, const OriginUserT *pUserList, OriginSizeT iUserCount, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryAreFriendsSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryAreFriendsSync(user, pUserList, iUserCount, pTotalCount, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginRequestFriend(OriginUserT user, OriginUserT userToAdd, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginRequestFriend entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->RequestFriend(user, userToAdd, timeout, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginRequestFriendSync(OriginUserT user, OriginUserT userToAdd, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginRequestFriendSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->RequestFriendSync(user, userToAdd, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginRemoveFriend(OriginUserT user, OriginUserT userToRemove, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginRemoveFriend entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->RemoveFriend(user, userToRemove, timeout, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginRemoveFriendSync(OriginUserT user, OriginUserT userToUnblock, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginRemoveFriendSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->RemoveFriendSync(user, userToUnblock, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginAcceptFriendInvite(OriginUserT user, OriginUserT userToAccept, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginAcceptFriendInvite entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->AcceptFriendInvite(user, userToAccept, timeout, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginAcceptFriendInviteSync(OriginUserT user, OriginUserT userToAccept, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginAcceptFriendInviteSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->AcceptFriendInviteSync(user, userToAccept, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

// Enumerate Functions

OriginErrorT ORIGIN_SDK_API OriginReadEnumeration(OriginHandleT hHandle, void *pBufPtr, OriginSizeT bufSize, OriginIndexT startIndex, OriginSizeT count, OriginSizeT *countRead)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginReadEnumeration entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ReadEnumeration(hHandle, pBufPtr, bufSize, startIndex, count, countRead);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginReadEnumerationSync(OriginHandleT hHandle, void *pBufPtr, OriginSizeT bufSize, OriginIndexT startIndex, OriginSizeT count, OriginSizeT *countRead)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginReadEnumerationSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ReadEnumerationSync(hHandle, pBufPtr, bufSize, startIndex, count, countRead);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetEnumerateStatus(OriginHandleT hHandle, OriginSizeT *total, OriginSizeT *available)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetEnumerateStatus entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetEnumerateStatus(hHandle, total, available);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

// Event functions


OriginErrorT ORIGIN_SDK_API OriginRegisterEventCallback(int32_t iEvents, OriginEventCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginRegisterEventCallback entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->RegisterEventCallback(iEvents, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginUnregisterEventCallback(int32_t iEvents, OriginEventCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginUnregisterEventCallback entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->UnregisterEventCallback(iEvents, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

// IGO functions

OriginErrorT OriginShowIGO(bool bShow, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowIGO entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGO(bShow, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowIGOSync(bool bShow)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowIGOSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOSync(bShow);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowLoginUI(int userIndex, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowLoginUI entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindow(IGOWINDOW_LOGIN, static_cast<OriginUserT>(userIndex), callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowLoginUISync(int userIndex)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowLoginUISync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindowSync(IGOWINDOW_LOGIN, static_cast<OriginUserT>(userIndex));
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginLogout(int userIndex, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginLogout entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->OriginLogout(userIndex, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginLogoutSync(int userIndex)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginLogoutSync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->OriginLogoutSync(userIndex);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowProfileUI(OriginUserT user, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowProfileUI entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindow(IGOWINDOW_PROFILE, user, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowProfileUISync(OriginUserT user)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowProfileUISync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindowSync(IGOWINDOW_PROFILE, user);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowFriendsProfileUI(OriginUserT user, OriginUserT target, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowFriendsProfileUI entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindow(IGOWINDOW_PROFILE, user, target, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowFriendsProfileUISync(OriginUserT user, OriginUserT target)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowFriendsProfileUISync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindowSync(IGOWINDOW_PROFILE, user, target);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowRecentUI(OriginUserT user, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowRecentUI entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindow(IGOWINDOW_RECENT, user, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowRecentUISync(OriginUserT user)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowRecentUISync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindowSync(IGOWINDOW_RECENT, user);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}


OriginErrorT ORIGIN_SDK_API OriginShowFeedbackUI(OriginUserT user, OriginUserT target, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowFeedbackUI entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindow(IGOWINDOW_FEEDBACK, user, target, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowFeedbackUISync(OriginUserT user, OriginUserT target)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowFeedbackUISync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindowSync(IGOWINDOW_FEEDBACK, user, target);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowBrowserUI(int32_t iBrowserFeatureFlags, const OriginCharT * pURL, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowBrowserUI entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindow(IGOWINDOW_BROWSER, iBrowserFeatureFlags, pURL, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowBrowserUISync(int32_t iBrowserFeatureFlags, const OriginCharT * pURL)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowBrowserUISync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindowSync(IGOWINDOW_BROWSER, iBrowserFeatureFlags, pURL);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowFriendsUI(OriginUserT user, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowFriendsUI entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindow(IGOWINDOW_FRIENDS, user, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowFriendsUISync(OriginUserT user)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowFriendsUISync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindowSync(IGOWINDOW_FRIENDS, user);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowFindFriendsUI(OriginUserT user, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowFindFriendsUI entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindow(IGOWINDOW_FIND_FRIENDS, user, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowFindFriendsUISync(OriginUserT user)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowFindFriendsUISync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindowSync(IGOWINDOW_FIND_FRIENDS, user);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowChangeAvatarUI(OriginUserT user,  OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowChangeAvatarUI entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindow(IGOWINDOW_CHANGE_AVATAR, user, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowChangeAvatarUISync(OriginUserT user)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowChangeAvatarUISync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindowSync(IGOWINDOW_CHANGE_AVATAR, user);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowGameDetailsUI(OriginUserT user, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowGameDetailsUI entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindow(IGOWINDOW_GAMEDETAILS, user, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowGameDetailsUISync(OriginUserT user)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowGameDetailsUISync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindowSync(IGOWINDOW_GAMEDETAILS, user);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}


OriginErrorT ORIGIN_SDK_API OriginShowBroadcastUI(OriginUserT user, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowBroadcastUI entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindow(IGOWINDOW_BROADCAST, user, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowBroadcastUISync(OriginUserT user)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowBroadcastUISync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindowSync(IGOWINDOW_BROADCAST, user);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}


OriginErrorT ORIGIN_SDK_API OriginRequestFriendUI(OriginUserT user, OriginUserT other, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginRequestFriendUI entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindow(IGOWINDOW_FRIEND_REQUEST, user, other, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginRequestFriendUISync(OriginUserT user, OriginUserT other)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginRequestFriendUISync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindowSync(IGOWINDOW_FRIEND_REQUEST, user, other);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}



OriginErrorT ORIGIN_SDK_API OriginShowComposeChatUI(OriginUserT user, const OriginUserT *pUserList, OriginSizeT iUserCount, const OriginCharT *pMessage, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowComposeChatUI entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindow(IGOWINDOW_COMPOSE_CHAT, user, pUserList, iUserCount, pMessage, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowComposeChatUISync(OriginUserT user, const OriginUserT *pUserList, OriginSizeT iUserCount, const OriginCharT *pMessage)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowComposeChatUISync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindowSync(IGOWINDOW_COMPOSE_CHAT, user, pUserList, iUserCount, pMessage);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}


OriginErrorT ORIGIN_SDK_API OriginShowInviteUI(OriginUserT user, const OriginUserT *pInviteList, OriginSizeT iInviteCount, const OriginCharT *pMessage, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowInviteUI entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindow(IGOWINDOW_INVITE, user, pInviteList, iInviteCount, pMessage, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowInviteUISync(OriginUserT user, const OriginUserT *pInviteList, OriginSizeT iInviteCount, const OriginCharT *pMessage)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowInviteUISync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindowSync(IGOWINDOW_INVITE, user, pInviteList, iInviteCount, pMessage);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}


OriginErrorT ORIGIN_SDK_API OriginShowAchievementUI(OriginUserT user, OriginPersonaT persona, const char *pGameId, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowAchievementUI entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindow(IGOWINDOW_ACHIEVEMENTS, user, &persona, 1, pGameId, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowAchievementUISync(OriginUserT user, OriginPersonaT persona, const char *pGameId)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowAchievementUISync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowIGOWindowSync(IGOWINDOW_ACHIEVEMENTS, user, &persona, 1, pGameId);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowUpsellUI(OriginUserT user, const OriginCharT * pType, const OriginCharT *pUri, const OriginCharT *pParams, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowUpsellUI entered");

    if (OriginSDK::Exists())
    {
        const OriginCharT * args[3];
        args[0] = pType;
        args[1] = pUri;
        args[2] = pParams;

        return OriginSDK::Get()->ShowIGOWindow(IGOWINDOW_UPSELL, user, args, 3, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowUpsellUISync(OriginUserT user, const OriginCharT * pType, const OriginCharT *pUri, const OriginCharT *pParams)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowUpsellUISync entered");

    if (OriginSDK::Exists())
    {
        const OriginCharT * args[3];
        args[0] = pType;
        args[1] = pUri;
        args[2] = pParams;

        return OriginSDK::Get()->ShowIGOWindowSync(IGOWINDOW_UPSELL, user, args, 3);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}


// Image Functions

OriginErrorT ORIGIN_SDK_API OriginQueryImage(const OriginCharT *pImageId, uint32_t width, uint32_t height, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryImage entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryImage(pImageId, width, height, callback, pContext, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryImageSync(const OriginCharT *pImageId, uint32_t width, uint32_t height, OriginSizeT * pTotal, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryImageSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryImageSync(pImageId, width, height, pTotal, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

// Origin Commerce functions

OriginErrorT ORIGIN_SDK_API OriginSelectStore(uint64_t storeId, uint64_t catalogId, const OriginCharT * pLockboxUrl, const OriginCharT *pSuccessUrl, const OriginCharT * pFailedUrl)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSelectStore entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->SelectStore(storeId, catalogId, 0, NULL, pLockboxUrl, pSuccessUrl, pFailedUrl);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}


OriginErrorT ORIGIN_SDK_API OriginSelectStoreEX(uint64_t storeId, uint64_t catalogId, uint64_t eWalletCategoryId, const OriginCharT *pVirtualCurrency, const OriginCharT *pLockboxUrl, const OriginCharT *pSuccessUrl, const OriginCharT * pFailedUrl)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSelectStore entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->SelectStore(storeId, catalogId, eWalletCategoryId, pVirtualCurrency, pLockboxUrl, pSuccessUrl, pFailedUrl);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}


OriginErrorT ORIGIN_SDK_API OriginGetStore(OriginUserT user, uint64_t storeId, OriginStoreCallback callback, void *pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetStore entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetStore(user, storeId, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetStoreSync(OriginUserT user, uint64_t storeId, OriginStoreT ** ppStore, OriginSizeT *pNumberOfStores, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetStoreSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetStoreSync(user, storeId, ppStore, pNumberOfStores, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}



OriginErrorT ORIGIN_SDK_API OriginGetCatalog(OriginUserT user, OriginCatalogCallback callback, void *pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetCatalog entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetCatalog(user, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetCatalogSync(OriginUserT user, OriginCategoryT **pCategories, OriginSizeT *pCategoryCount, uint32_t timeout, OriginHandleT *handle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetCatalogSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetCatalogSync(user, pCategories, pCategoryCount, timeout, handle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetWalletBalance(OriginUserT user, const OriginCharT *currency, OriginWalletCallback callback, void *pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetWalletBalance entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetWalletBalance(user, currency, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetWalletBalanceSync(OriginUserT user, const OriginCharT *Currency, int64_t *pBalance, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetWalletBalanceSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetWalletBalanceSync(user, Currency, pBalance, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowStoreUI(OriginUserT user, const OriginCharT *pCategoryList[], OriginSizeT categoryCount, const OriginCharT *pOfferList[], OriginSizeT offerCount, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowStoreUI entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowStoreUI(user, pCategoryList, categoryCount, NULL, 0, pOfferList, offerCount, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowStoreUISync(OriginUserT user, const OriginCharT *pCategoryList[], OriginSizeT categoryCount, const OriginCharT *pOfferList[], OriginSizeT offerCount)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowStoreUISync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowStoreUISync(user, pCategoryList, categoryCount, NULL, 0, pOfferList, offerCount);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowStoreUIEX(OriginUserT user, const OriginCharT *pCategoryList[], OriginSizeT categoryCount, const OriginCharT *pMasterTitleIdList[], OriginSizeT MasterTitleIdCount, const OriginCharT *pOfferList[], OriginSizeT offerCount, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowStoreUIEx entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowStoreUI(user, pCategoryList, categoryCount, pMasterTitleIdList, MasterTitleIdCount, pOfferList, offerCount, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowStoreUIEXSync(OriginUserT user, const OriginCharT *pCategoryList[], OriginSizeT categoryCount, const OriginCharT *pMasterTitleIdList[], OriginSizeT MasterTitleIdCount, const OriginCharT *pOfferList[], OriginSizeT offerCount)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowStoreUIExSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowStoreUISync(user, pCategoryList, categoryCount, pMasterTitleIdList, MasterTitleIdCount, pOfferList, offerCount);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowCodeRedemptionUI(OriginUserT user, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowCodeRedemptionUI entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowCodeRedemptionUI(user, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowCodeRedemptionUISync(OriginUserT user)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowCodeRedemptionUISync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowCodeRedemptionUISync(user);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryCategories(OriginUserT user, const OriginCharT *pParentCategoryList[], OriginSizeT parentCategoryCount, OriginEnumerationCallbackFunc callback, void * pContext, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryCategories entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryCategories(user, pParentCategoryList, parentCategoryCount, callback, pContext, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryCategoriesSync(OriginUserT user, const OriginCharT *pParentCategoryList[], OriginSizeT parentCategoryCount, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryCategoriesSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryCategoriesSync(user, pParentCategoryList, parentCategoryCount, pTotalCount, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryOffers(OriginUserT user, const OriginCharT *pCategoryList[], OriginSizeT categoryCount, const OriginCharT *pOfferList[], OriginSizeT offerCount, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryOffers entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryOffers(user, pCategoryList, categoryCount, NULL, 0, pOfferList, offerCount, callback, pContext, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryOffersSync(OriginUserT user, const OriginCharT *pCategoryList[], OriginSizeT categoryCount, const OriginCharT *pOfferList[], OriginSizeT offerCount, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryOffersSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryOffersSync(user, pCategoryList, categoryCount, NULL, 0, pOfferList, offerCount, pTotalCount, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryOffersEX(OriginUserT user, const OriginCharT *pCategoryList[], OriginSizeT categoryCount, const OriginCharT *pMasterTitleIdList[], OriginSizeT MasterTitleIdCount, const OriginCharT *pOfferList[], OriginSizeT offerCount, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryOffersEX entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryOffers(user, pCategoryList, categoryCount, pMasterTitleIdList, MasterTitleIdCount, pOfferList, offerCount, callback, pContext, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryOffersSyncEX(OriginUserT user, const OriginCharT *pCategoryList[], OriginSizeT categoryCount, const OriginCharT *pMasterTitleIdList[], OriginSizeT MasterTitleIdCount, const OriginCharT *pOfferList[], OriginSizeT offerCount, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryOffersSyncEX entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryOffersSync(user, pCategoryList, categoryCount, pMasterTitleIdList, MasterTitleIdCount, pOfferList, offerCount, pTotalCount, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryContent(OriginUserT user, const OriginCharT *gameIds[], OriginSizeT masterTitleIdCount, const OriginCharT * multiplayerId, uint32_t contentType, OriginEnumerationCallbackFunc callback, void* context, uint32_t timeout, OriginHandleT *handle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryContent entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryContent(user, gameIds, masterTitleIdCount, multiplayerId, contentType, callback, context, timeout, handle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryContentSync(OriginUserT user, const OriginCharT *gameIds[], OriginSizeT masterTitleIdCount, const OriginCharT * multiplayerId, uint32_t contentType, OriginSizeT *total, uint32_t timeout, OriginHandleT *handle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryContentSync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryContentSync(user, gameIds, masterTitleIdCount, multiplayerId, contentType, total, timeout, handle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}


OriginErrorT ORIGIN_SDK_API OriginRestartGame(OriginUserT user, enumRestartGameOptions option, OriginErrorSuccessCallback callback, void *context, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginRestartGame entered");

    if (OriginSDK::Exists())
    {
        // Set the command line argument to "" for now.
        return OriginSDK::Get()->RestartGame(user, option, callback, context, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginRestartGameSync(OriginUserT user, enumRestartGameOptions option, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginRestartGameSync entered");

    if (OriginSDK::Exists())
    {
        // Set the command line argument to "" for now.
        return OriginSDK::Get()->RestartGameSync(user, option, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}


OriginErrorT ORIGIN_SDK_API OriginStartGame(const OriginCharT * gameId, const OriginCharT * multiplayerId, const OriginCharT * commandLineArgs, OriginErrorSuccessCallback callback, void *context, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginStartGame entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->StartGame(gameId, multiplayerId, commandLineArgs, callback, context, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginStartGameSync(const OriginCharT * gameId, const OriginCharT * multiplayerId, const OriginCharT * commandLineArgs, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginStartGameSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->StartGameSync(gameId, multiplayerId, commandLineArgs, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryManifest(OriginUserT user, const OriginCharT* manifest, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryManifest entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryManifest(user, manifest, callback, pContext, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryManifestSync(OriginUserT user, const OriginCharT* manifest, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryManifestSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryManifestSync(user, manifest, pTotalCount, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowCheckoutUI(OriginUserT user, const OriginCharT *pOfferList[], OriginSizeT offerCount, OriginErrorSuccessCallback callback, void* pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowCheckoutUI entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowCheckoutUI(user, pOfferList, offerCount, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginShowCheckoutUISync(OriginUserT user, const OriginCharT *pOfferList[], OriginSizeT offerCount)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginShowCheckoutUISync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ShowCheckoutUISync(user, pOfferList, offerCount);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryEntitlements(OriginUserT user, const OriginCharT *pCategoryList[], OriginSizeT categoryCount, const OriginCharT *pOfferList[], OriginSizeT offerCount, const OriginCharT *pEntitlementTagList[], OriginSizeT entitlementTagCount, const OriginCharT * pGroup, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryEntitlements entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryEntitlements(user, pCategoryList, categoryCount, pOfferList, offerCount, pEntitlementTagList, entitlementTagCount, pGroup, callback, pContext, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryEntitlementsSync(OriginUserT user, const OriginCharT *pCategoryList[], OriginSizeT categoryCount, const OriginCharT *pOfferList[], OriginSizeT offerCount, const OriginCharT *pEntitlementTagList[], OriginSizeT entitlementTagCount, const OriginCharT * pGroup, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryEntitlementsSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryEntitlementsSync(user, pCategoryList, categoryCount, pOfferList, offerCount, pEntitlementTagList, entitlementTagCount, pGroup, pTotalCount, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryEntitlementsEX(OriginUserT user, const OriginCharT *pOfferList[], OriginSizeT offerCount, const OriginCharT *pEntitlementTagList[], OriginSizeT entitlementTagCount, const OriginCharT *pGroupList[], OriginSizeT groupCount, bool includeChildGroups, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryEntitlementsEx entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryEntitlements(user, pOfferList, offerCount, pEntitlementTagList, entitlementTagCount, pGroupList, groupCount, includeChildGroups, false, callback, pContext, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryEntitlementsSyncEX(OriginUserT user, const OriginCharT *pOfferList[], OriginSizeT offerCount, const OriginCharT *pEntitlementTagList[], OriginSizeT entitlementTagCount, const OriginCharT *pGroupList[], OriginSizeT groupCount, bool includeChildGroups, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryEntitlementsSyncEx entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryEntitlementsSync(user, pOfferList, offerCount, pEntitlementTagList, entitlementTagCount, pGroupList, groupCount, includeChildGroups, false, pTotalCount, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryEntitlementsEX2(OriginUserT user, const OriginCharT *pOfferList[], OriginSizeT offerCount, const OriginCharT *pEntitlementTagList[], OriginSizeT entitlementTagCount, const OriginCharT *pGroupList[], OriginSizeT groupCount, bool includeChildGroups, bool includeExpiredTrialDLC, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryEntitlementsEx entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryEntitlements(user, pOfferList, offerCount, pEntitlementTagList, entitlementTagCount, pGroupList, groupCount, includeChildGroups, includeExpiredTrialDLC, callback, pContext, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryEntitlementsSyncEX2(OriginUserT user, const OriginCharT *pOfferList[], OriginSizeT offerCount, const OriginCharT *pEntitlementTagList[], OriginSizeT entitlementTagCount, const OriginCharT *pGroupList[], OriginSizeT groupCount, bool includeChildGroups, bool includeExpiredTrialDLC, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryEntitlementsSyncEx entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryEntitlementsSync(user, pOfferList, offerCount, pEntitlementTagList, entitlementTagCount, pGroupList, groupCount, includeChildGroups, includeExpiredTrialDLC, pTotalCount, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginConsumeEntitlement(OriginUserT user, uint32_t uses, bool bOveruse, OriginItemT *pItem, OriginEntitlementCallback callback, void *pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginConsumeEntitlement entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ConsumeEntitlement(user, uses, bOveruse, pItem, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}


OriginErrorT ORIGIN_SDK_API OriginConsumeEntitlementSync(OriginUserT user, uint32_t uses, bool bOveruse, OriginItemT *pItem, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginConsumeEntitlementSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->ConsumeEntitlementSync(user, uses, bOveruse, pItem, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginCheckout(OriginUserT user, const OriginCharT * Currency, const OriginCharT *pOfferList[], OriginSizeT offerCount, OriginResourceCallback callback, void *pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginCheckout entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->Checkout(user, Currency, pOfferList, offerCount, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginCheckoutSync(OriginUserT user, const OriginCharT * Currency, const OriginCharT *pOfferList[], OriginSizeT offerCount, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginCheckoutSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->CheckoutSync(user, Currency, pOfferList, offerCount, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}


// Players Functions


OriginErrorT ORIGIN_SDK_API OriginAddRecentPlayers(OriginUserT user, const OriginUserT *pRecentList, OriginSizeT recentCount, uint32_t timeout, OriginErrorSuccessCallback callback, void *pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginAddRecentPlayers entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->AddRecentPlayers(user, pRecentList, recentCount, timeout, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}


OriginErrorT ORIGIN_SDK_API OriginAddRecentPlayersSync(OriginUserT user, const OriginUserT *pRecentList, OriginSizeT recentCount, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginAddRecentPlayers entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->AddRecentPlayersSync(user, pRecentList, recentCount, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginSendChatMessage(OriginUserT fromUser, OriginUserT toUser, const OriginCharT * thread, const OriginCharT* message, const OriginCharT* groupId)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSendChatMessage entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->SendChatMessage(fromUser, toUser, thread, message, groupId);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

// Groups
OriginErrorT ORIGIN_SDK_API OriginQueryGroup(OriginUserT user, const OriginCharT * groupId, OriginEnumerationCallbackFunc callback, void * pContext, uint32_t timeout, OriginHandleT * handle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryGroup entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryGroup(user, groupId, callback, pContext, timeout, handle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryGroupSync(OriginUserT user, const OriginCharT * groupId, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT * handle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryGroupSync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryGroupSync(user, groupId, pTotalCount, timeout, handle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetGroupInfo(OriginUserT user, const OriginCharT * groupId, OriginGroupInfoCallback callback, void *pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetGroupInfo entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetGroupInfo(user, groupId, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetGroupInfoSync(OriginUserT user, const OriginCharT * groupId, uint32_t timeout, OriginGroupInfoT * groupInfo, OriginHandleT * handle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetGroupInfoSync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetGroupInfoSync(user, groupId, timeout, groupInfo, handle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginSendGroupGameInvite(OriginUserT user, const OriginUserT * pUserList, size_t userCount, const OriginCharT * pMessage, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSendGroupGameInvite entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->SendGroupGameInvite(user, pUserList, userCount, pMessage, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginSendGroupGameInviteSync(OriginUserT user, const OriginUserT * pUserList, size_t userCount, const OriginCharT * pMessage, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSendGroupGameInviteSync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->SendGroupGameInviteSync(user, pUserList, userCount, pMessage, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginCreateGroup(OriginUserT user, const OriginCharT *groupName, enumGroupType type, OriginGroupInfoCallback callback, void *pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginCreateGroup entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->CreateGroup(user, groupName, type, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginCreateGroupSync(OriginUserT user, const OriginCharT *groupName, enumGroupType type, uint32_t timeout, OriginGroupInfoT * groupInfo, OriginHandleT * handle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginCreateGroupSync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->CreateGroupSync(user, groupName, type, timeout, groupInfo, handle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginEnterGroup(OriginUserT user, const OriginCharT *groupId, OriginGroupInfoCallback callback, void *pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginEnterGroup entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->EnterGroup(user, groupId, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginEnterGroupSync(OriginUserT user, const OriginCharT *groupId, uint32_t timeout, OriginGroupInfoT * groupInfo, OriginHandleT *handle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginEnterGroupSync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->EnterGroupSync(user, groupId, timeout, groupInfo, handle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginLeaveGroup(OriginUserT user, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginLeaveGroup entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->LeaveGroup(user, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginLeaveGroupSync(OriginUserT user, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginLeaveGroupSync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->LeaveGroupSync(user, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginInviteUsersToGroup(OriginUserT user, const OriginUserT *users, OriginSizeT userCount, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginInviteUsersToGroup entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->InviteUsersToGroup(user, users, userCount, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginInviteUsersToGroupSync(OriginUserT user, const OriginUserT *users, OriginSizeT userCount, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginInviteUsersToGroupSync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->InviteUsersToGroupSync(user, users, userCount, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginRemoveUsersFromGroup(OriginUserT user, const OriginUserT *users, OriginSizeT userCount, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginRemoveUsersFromGroup entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->RemoveUsersFromGroup(user, users, userCount, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginRemoveUsersFromGroupSync(OriginUserT user, const OriginUserT *users, OriginSizeT userCount, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginRemoveUsersFromGroupSync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->RemoveUsersFromGroupSync(user, users, userCount, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}


OriginErrorT ORIGIN_SDK_API OriginEnableVoip(bool bEnable, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginEnableVoip entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->EnableVoip(bEnable, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginEnableVoipSync(bool bEnable, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginEnableVoipSync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->EnableVoipSync(bEnable, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetVoipStatus(OriginVoipStatusCallback callback, void * pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetVoipStatus entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetVoipStatus(callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetVoipStatusSync(OriginVoipStatusT * config, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetVoipStatusSync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetVoipStatusSync(config, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginMuteUser(bool bMute, OriginUserT userId, const OriginCharT *groupId, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginMuteUser entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->MuteUser(bMute, userId, groupId, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginMuteUserSync(bool bMute, OriginUserT userId, const OriginCharT *groupId, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginMuteUserSync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->MuteUserSync(bMute, userId, groupId, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}


OriginErrorT ORIGIN_SDK_API OriginQueryMuteState(const OriginCharT *groupId, OriginEnumerationCallbackFunc callback, void * pContext, uint32_t timeout, OriginHandleT * handle) 
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginMuteUserSync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryMuteState(groupId, callback, pContext, timeout, handle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryMuteStateSync(const OriginCharT *groupId, OriginSizeT * total, uint32_t timeout, OriginHandleT * handle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginMuteUserSync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryMuteStateSync(groupId, total, timeout, handle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}



// Invite
OriginErrorT ORIGIN_SDK_API OriginSendInvite(OriginUserT user, const OriginUserT *pUserList, OriginSizeT userCount, const OriginCharT * pMessage, uint32_t timeout, OriginErrorSuccessCallback callback, void *pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSendInvite entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->SendInvite(user, pUserList, userCount, pMessage, timeout, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginSendInviteSync(OriginUserT user, const OriginUserT *pUserList, OriginSizeT userCount, const OriginCharT * pMessage, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSendInviteSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->SendInviteSync(user, pUserList, userCount, pMessage, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginAcceptInvite(OriginUserT user, OriginUserT other, uint32_t timeout, OriginErrorSuccessCallback callback, void *pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginAcceptInvite entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->AcceptInvite(user, other, timeout, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginAcceptInviteSync(OriginUserT user, OriginUserT other, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginAcceptInviteSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->AcceptInviteSync(user, other, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryBlockedUsers(OriginUserT user, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryBlockedUsers entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryBlockedUsers(user, callback, pContext, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryBlockedUsersSync(OriginUserT user, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryBlockedUsersSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryBlockedUsersSync(user, pTotalCount, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginBlockUser(OriginUserT user, OriginUserT userToBlock, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginBlockUser entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->BlockUser(user, userToBlock, timeout, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginBlockUserSync(OriginUserT user, OriginUserT userToBlock, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginBlockUserSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->BlockUserSync(user, userToBlock, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginUnblockUser(OriginUserT user, OriginUserT userToUnblock, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginUnblockUser entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->UnblockUser(user, userToUnblock, timeout, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginUnblockUserSync(OriginUserT user, OriginUserT userToUnblock, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginUnblockUserSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->UnblockUserSync(user, userToUnblock, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryUserIdSync(const OriginCharT *pIdentifier, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryUserIdSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryUserIdSync(pIdentifier, pTotalCount, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryUserId(const OriginCharT *pIdentifier, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryUserId entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryUserId(pIdentifier, callback, pContext, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGrantAchievement(OriginUserT user, OriginPersonaT persona, int achievementId, int progress, const OriginCharT * achievementCode, uint32_t timeout, OriginAchievementCallback callback, void *pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGrantAchievement entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GrantAchievement(user, persona, achievementId, progress, achievementCode, timeout, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGrantAchievementSync(OriginUserT user, OriginPersonaT persona, int achievementId, int progress, const OriginCharT * achievementCode, uint32_t timeout, OriginAchievementT *achievement, OriginHandleT *handle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGrantAchievementSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GrantAchievementSync(user, persona, achievementId, progress, achievementCode, timeout, achievement, handle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}


OriginErrorT ORIGIN_SDK_API OriginCreateEventRecord(OriginHandleT * eventHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginCreateEventRecord entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->CreateEventRecord(eventHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginAddEvent(OriginHandleT eventHandle, const OriginCharT * eventName)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginAddEvent entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->AddEvent(eventHandle, eventName);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginAddEventParameter(OriginHandleT eventHandle, const OriginCharT *name, const OriginCharT *value)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginAddEventParameter entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->AddEventParameter(eventHandle, name, value);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginPostAchievementEvents(OriginUserT user, OriginPersonaT persona, OriginHandleT eventHandle, uint32_t timeout, OriginErrorSuccessCallback callback, void *pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginPostAchievementEvents entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->PostAchievementEvents(user, persona, eventHandle, timeout, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginPostAchievementEventsSync(OriginUserT user, OriginPersonaT persona, OriginHandleT eventHandle, uint32_t timeout/*, OriginAchievementT *achievement, OriginHandleT *achievementHandle*/)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginPostAchievementEventsSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->PostAchievementEventsSync(user, persona, eventHandle, timeout/*, callback, pContext*/);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}



OriginErrorT ORIGIN_SDK_API OriginPostWincodes(OriginUserT user, OriginPersonaT persona, const OriginCharT *authcode, const OriginCharT ** keys, const OriginCharT ** values, OriginSizeT count, uint32_t timeout, OriginErrorSuccessCallback callback, void *pContext)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginPostWincodes entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->PostWincodes(user, persona, authcode, keys, values, count, timeout, callback, pContext);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginPostWincodesSync(OriginUserT user, OriginPersonaT persona, const OriginCharT *authcode, const OriginCharT ** keys, const OriginCharT ** values, OriginSizeT count, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginPostWincodesSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->PostWincodesSync(user, persona, authcode, keys, values, count, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryAchievements(OriginUserT user, OriginPersonaT persona, const OriginCharT *pGameIdList[], OriginSizeT gameIdCount, enumAchievementMode mode, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryAchievements entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryAchievements(user, persona, pGameIdList, gameIdCount, mode, callback, pContext, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryAchievementsSync(OriginUserT user, OriginPersonaT persona, const OriginCharT *pGameIdList[], OriginSizeT gameIdCount, enumAchievementMode mode, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryAchievementsSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryAchievementsSync(user, persona, pGameIdList, gameIdCount, mode, pTotalCount, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

//////////////////////////////////////////////////////////////////////////
// Progressive installation functions.

OriginErrorT ORIGIN_SDK_API OriginIsProgressiveInstallationAvailable(const OriginCharT *itemId, OriginIsProgressiveInstallationAvailableCallback callback, void * pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginIsProgressiveInstallationAvailable entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->IsProgressiveInstallationAvailable(itemId, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginIsProgressiveInstallationAvailableSync(const OriginCharT *itemId, bool *bAvailable, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginIsProgressiveInstallationAvailableSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->IsProgressiveInstallationAvailableSync(itemId, bAvailable, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginAreChunksInstalled(const OriginCharT *itemId, int32_t * pChunkIds, OriginSizeT chunkCount, OriginAreChunksInstalledCallback callback, void *pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginAreChunksInstalled entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->AreChunksInstalled(itemId, pChunkIds, chunkCount, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginAreChunksInstalledSync(const OriginCharT *itemId, int32_t * pChunkIds, OriginSizeT chunkCount, bool *installed, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginAreChunksInstalledSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->AreChunksInstalledSync(itemId, pChunkIds, chunkCount, installed, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryChunkStatus(const OriginCharT *itemId, OriginEnumerationCallbackFunc callback, void *pContext, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetChunkStatus entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryChunkStatus(itemId, callback, pContext, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryChunkStatusSync(const OriginCharT *itemId, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetChunkStatusSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryChunkStatusSync(itemId, pTotalCount, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginIsFileDownloaded(const OriginCharT *itemId, const char *filePath, OriginIsFileDownloadedCallback callback, void *pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginIsFileDownloaded entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->IsFileDownloaded(itemId, filePath, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginIsFileDownloadedSync(const OriginCharT *itemId, const char *filePath, bool *downloaded, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginIsFileDownloadedSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->IsFileDownloadedSync(itemId, filePath, downloaded, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginSetChunkPriority(const OriginCharT *itemId, int32_t * pChunckIds, OriginSizeT chunkCount, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSetChunkPriority entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->SetChunkPriority(itemId, pChunckIds, chunkCount, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginSetChunkPrioritySync(const OriginCharT *itemId, int32_t * pChunckIds, OriginSizeT chunkCount, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSetChunkPrioritySync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->SetChunkPrioritySync(itemId, pChunckIds, chunkCount, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetChunkPriority(const OriginCharT *itemId, OriginChunkPriorityCallback callback, void *pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetChunkPriority entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetChunkPriority(itemId, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetChunkPrioritySync(const OriginCharT *itemId, OriginCharT * itemIdOut, OriginSizeT * itemIdOutSize, int32_t * pChunckIds, OriginSizeT * chunkCount, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetChunkPrioritySync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetChunkPrioritySync(itemId, itemIdOut, itemIdOutSize, pChunckIds, chunkCount, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryChunkFiles(const OriginCharT *itemId, int32_t chunkId, OriginEnumerationCallbackFunc callback, void *pContext, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryChunkFiles entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryChunkFiles(itemId, chunkId, callback, pContext, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginQueryChunkFilesSync(const OriginCharT *itemId, int32_t chunkId, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginQueryChunkFilesSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->QueryChunkFilesSync(itemId, chunkId, pTotalCount, timeout, pHandle);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginCreateChunk(const OriginCharT *itemId, const OriginCharT **files, OriginSizeT filesCount, OriginCreateChunkCallback callback, void *pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginCreateChunk entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->CreateChunk(itemId, files, filesCount, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginCreateChunkSync(const OriginCharT *itemId, const OriginCharT **files, OriginSizeT filesCount, int32_t *pChunkId, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginCreateChunkSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->CreateChunkSync(itemId, files, filesCount, pChunkId, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginStartDownload(const OriginCharT *itemId, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginStartDownload entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->StartDownload(itemId, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginStartDownloadSync(const OriginCharT *itemId, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginStartDownloadSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->StartDownloadSync(itemId, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginSetDownloadUtilization(float utilization, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSetDownloadUtilization entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->SetDownloadUtilization(utilization, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginSetDownloadUtilizationSync(float utilization, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSetDownloadUtilizationSync entered");

    if(OriginSDK::Exists())
    {
        return OriginSDK::Get()->SetDownloadUtilizationSync(utilization, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginOverlayStateChanged(bool isUp, OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout)
{
	REPORTDEBUG(ORIGIN_LEVEL_3, "OriginOverlayStateChanged entered");

	if (OriginSDK::Exists())
	{
		return OriginSDK::Get()->OverlayStateChanged(isUp, callback, pContext, timeout);
	}
	return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginCompletePurchaseTransaction(uint32_t appId, uint64_t orderId, bool authorized, OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout)
{
	REPORTDEBUG(ORIGIN_LEVEL_3, "OriginCompletePurchaseTransaction entered");

	if (OriginSDK::Exists())
	{
		return OriginSDK::Get()->CompletePurchaseTransaction(appId, orderId, authorized, callback, pContext, timeout);
	}
	return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginRefreshEntitlements(OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout)
{
	REPORTDEBUG(ORIGIN_LEVEL_3, "OriginRefereshEntitlements entered");

	if (OriginSDK::Exists())
	{
		return OriginSDK::Get()->RefreshEntitlements(callback, pContext, timeout);
	}
	return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginSetPlatformDLC(DLC* dlc, uint32_t count, OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSetPlatformDLC entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->SetPlatformDLC(dlc, count, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginSetPlatformDLCSync(DLC* dlc, uint32_t count, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSetPlatformDLCSync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->SetPlatformDLCSync(dlc, count, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginDetermineCommerceCurrencySync(uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginDetermineCommerceCurrencySync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->DetermineCommerceCurrencySync(timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginSendSteamAchievementErrorTelemetry(bool validStat, bool getStat, bool setStat, OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout)
{
	REPORTDEBUG(ORIGIN_LEVEL_3, "SendSteamAchievementErrorTelemetry entered");

	if (OriginSDK::Exists())
	{
		return OriginSDK::Get()->SendSteamAchievementErrorTelemetry(validStat, getStat, setStat, callback, pContext, timeout);
	}
	return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginSendSteamAchievementErrorTelemetrySync(bool validStat, bool getStat, bool setStat, uint32_t timeout)
{
	REPORTDEBUG(ORIGIN_LEVEL_3, "SendSteamAchievementErrorTelemetry entered");

	if (OriginSDK::Exists())
	{
		return OriginSDK::Get()->SendSteamAchievementErrorTelemetrySync(validStat, getStat, setStat, timeout);
	}
	return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginSetSteamLocale(const OriginCharT *language, OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSetSteamLocale entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->SetSteamLocale(language, callback, pContext, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginSetSteamLocaleSync(const OriginCharT *language, uint32_t timeout)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSetSteamLocaleSync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->SetSteamLocaleSync(language, timeout);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

#ifdef __cplusplus
}
#endif

// Deprecated functions having C++ bindings.

OriginErrorT OriginSetPresence(OriginUserT user, enumPresence presence, const OriginCharT *pPresenceString, const OriginCharT* pGamePresenceString, const OriginCharT *pSessionString)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSetPresenceSync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->SetPresenceSync(user, presence, pPresenceString, pGamePresenceString, pSessionString, ORIGIN_LSX_TIMEOUT);
    }

    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT OriginGetPresence(OriginUserT user, enumPresence * pPresence, OriginCharT *pPresenceString, OriginSizeT presenceStringLen, OriginCharT *pGamePresenceString, OriginSizeT gamePresenceStringLen, OriginCharT *pSessionString, OriginSizeT sessionStringLen)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetPresenceSync entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetPresenceSync(user, pPresence, pPresenceString, presenceStringLen, pGamePresenceString, gamePresenceStringLen, pSessionString, sessionStringLen, ORIGIN_LSX_TIMEOUT);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginSetPresenceVisibility(OriginUserT user, bool visible)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSetPresenceVisibility entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->SetPresenceVisibilitySync(user, visible, ORIGIN_LSX_TIMEOUT);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT ORIGIN_SDK_API OriginGetPresenceVisibility(OriginUserT user, bool * visible)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginGetPresenceVisibility entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->GetPresenceVisibilitySync(user, visible, ORIGIN_LSX_TIMEOUT);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT OriginSubscribePresence(OriginUserT user, const OriginUserT *otherUsers, OriginSizeT userCount)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginSubscribePresence entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->SubscribePresenceSync(user, otherUsers, userCount, ORIGIN_LSX_TIMEOUT);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
}

OriginErrorT OriginUnsubscribePresence(OriginUserT user, const OriginUserT *otherUsers, OriginSizeT userCount)
{
    REPORTDEBUG(ORIGIN_LEVEL_3, "OriginUnsubscribePresence entered");

    if (OriginSDK::Exists())
    {
        return OriginSDK::Get()->UnsubscribePresenceSync(user, otherUsers, userCount, ORIGIN_LSX_TIMEOUT);
    }
    return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);

}



