#include "stdafx.h"
#include "OriginSDK/OriginSDK.h"
#include "OriginSDK/OriginError.h"
#include "OriginSDKimpl.h"
#include <assert.h>
#include "OriginLSXRequest.h"
#include "OriginLSXEnumeration.h"

namespace Origin
{
    OriginErrorT OriginSDK::GrantAchievement(OriginUserT user, OriginPersonaT persona, int achievementId, int progress, const OriginCharT * achievementCode, uint32_t timeout, OriginAchievementCallback callback, void *pContext)
    {
        if(persona == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_PERSONA);

        if(achievementCode == NULL || achievementId<0 || callback == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::GrantAchievementT, lsx::AchievementT, OriginAchievementT, OriginAchievementCallback > > req(
            new LSXRequest<lsx::GrantAchievementT, lsx::AchievementT, OriginAchievementT, OriginAchievementCallback>
                (GetService(lsx::FACILITY_ACHIEVEMENT), this, &OriginSDK::GrantAchievementConvertData, callback, pContext));

        lsx::GrantAchievementT &r = req->GetRequest();

        r.UserId = user;
        r.PersonaId = persona;
        r.AchievementId = achievementId;
        r.Progress = progress;
        r.AchievementCode = achievementCode;

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

    OriginErrorT OriginSDK::GrantAchievementSync(OriginUserT user, OriginPersonaT persona, int achievementId, int progress, const OriginCharT * achievementCode, uint32_t timeout, OriginAchievementT *achievement, OriginHandleT *handle)
    {
        if(persona == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_PERSONA);

        if(achievementCode == NULL || achievement == NULL || achievementId<0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::GrantAchievementT, lsx::AchievementT> > req(
            new LSXRequest<lsx::GrantAchievementT, lsx::AchievementT>
                (GetService(lsx::FACILITY_SDK), this));

        lsx::GrantAchievementT &r = req->GetRequest();

        r.UserId = user;
        r.PersonaId = persona;
        r.AchievementId = achievementId;
        r.Progress = progress;
        r.AchievementCode = achievementCode;

        if(req->Execute(timeout))
        {
            ConvertData(req, *achievement, req->GetResponse(), true);

            *handle = req->CopyDataStoreToHandle();

            return REPORTERROR(ORIGIN_SUCCESS);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::GrantAchievementConvertData(IXmlMessageHandler *pHandler, OriginAchievementT &achievement, size_t &size, lsx::AchievementT &response)
    {
        size = sizeof(OriginAchievementT);

        ConvertData(pHandler, achievement, response, false);

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    void OriginSDK::ConvertData(IXmlMessageHandler *pHandler, OriginAchievementT &Dest, const lsx::AchievementT &Src, bool bCopyStrings)
    {
        Dest.Name = ConvertData(pHandler, Src.Name, bCopyStrings);
        Dest.Description = ConvertData(pHandler, Src.Description, bCopyStrings);
        Dest.HowTo = ConvertData(pHandler, Src.HowTo, bCopyStrings);
        Dest.ImageId = ConvertData(pHandler, Src.ImageId, bCopyStrings);
        Dest.GrantDate = Src.GrantDate;
        Dest.Expiration = Src.Expiration;
		Dest.Id = ConvertData(pHandler, Src.Id, bCopyStrings);
        Dest.Progress = Src.Progress;
        Dest.Total = Src.Total;
        Dest.Count = Src.Count;
    }

    void destroyPostAchievementEvents(OriginResourceT resource)
    {
        Allocator<lsx::PostAchievementEventsT> allocator;

        allocator.destroy((lsx::PostAchievementEventsT *)resource);
        TYPE_DELETE((lsx::PostAchievementEventsT *) resource);
    }

    OriginErrorT OriginSDK::CreateEventRecord(OriginHandleT * eventHandle)
    {
        if((eventHandle) == (OriginHandleT*)NULL)
            return ORIGIN_ERROR_INVALID_ARGUMENT;

        void *mem = TYPE_NEW(lsx::PostAchievementEventsT, 1);
        lsx::PostAchievementEventsT * req = new (mem) lsx::PostAchievementEventsT;

        *eventHandle = RegisterCustomResource(req, destroyPostAchievementEvents);

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::AddEvent(OriginHandleT eventHandle, const OriginCharT * eventName)
    {
        if(eventHandle == (OriginHandleT)NULL || eventName == NULL || eventName[0] == 0)
            return ORIGIN_ERROR_INVALID_ARGUMENT;

        lsx::PostAchievementEventsT * req = (lsx::PostAchievementEventsT *) eventHandle;

        if(req->Events.size() == 0 || (req->Events.size() > 0 && req->Events.back().Attributes.size() > 0))
        {
            lsx::EventT event;
            event.EventId = eventName;

            req->Events.push_back(event);

            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            return ORIGIN_ERROR_INVALID_OPERATION;
        }
    }

    OriginErrorT OriginSDK::AddEventParameter(OriginHandleT eventHandle, const OriginCharT * name, const OriginCharT * value)
    {
        if(eventHandle == (OriginHandleT)NULL || name == NULL || name[0] == 0 || value == NULL || value[0] == 0)
            return ORIGIN_ERROR_INVALID_ARGUMENT;

        lsx::PostAchievementEventsT * req = (lsx::PostAchievementEventsT *)eventHandle;

        if(req->Events.size() > 0)
        {
            lsx::EventParamT param;
            param.Name = name;
            param.Value = value;
            req->Events.back().Attributes.push_back(param);

            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            return ORIGIN_ERROR_INVALID_OPERATION;
        }
    }

    OriginErrorT OriginSDK::PostAchievementEvents(OriginUserT user, OriginPersonaT persona, OriginHandleT eventHandle, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext)
    {
        if(eventHandle == (OriginHandleT)NULL || persona == 0 || callback == NULL)
            return ORIGIN_ERROR_INVALID_ARGUMENT;


        IObjectPtr< LSXRequest<lsx::PostAchievementEventsT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback> > req(
            new LSXRequest<lsx::PostAchievementEventsT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback>
                (GetService(lsx::FACILITY_ACHIEVEMENT), this, &OriginSDK::PostAchievementEventsConvertData, callback, pContext));

        lsx::PostAchievementEventsT & r = req->GetRequest();

        r = *(lsx::PostAchievementEventsT *)eventHandle;

        DestroyHandle(eventHandle);

        r.UserId = user;
        r.PersonaId = persona;

        if(r.Events.size() == 0 || (r.Events.size() > 0 && r.Events.back().Attributes.size() > 0))
        {
            if(req->ExecuteAsync(timeout))
            {
                return REPORTERROR(ORIGIN_SUCCESS);
            }
        }
        else
        {
            return REPORTERROR(ORIGIN_ERROR_INVALID_OPERATION);
        }
            
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::PostAchievementEventsSync(OriginUserT user, OriginPersonaT persona, OriginHandleT eventHandle, uint32_t timeout)
    {
        if(eventHandle == (OriginHandleT)NULL || persona == 0)
            return ORIGIN_ERROR_INVALID_ARGUMENT;

        IObjectPtr< LSXRequest<lsx::PostAchievementEventsT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::PostAchievementEventsT, lsx::ErrorSuccessT>
                (GetService(lsx::FACILITY_ACHIEVEMENT), this));

        lsx::PostAchievementEventsT & r = req->GetRequest();

        r = *(lsx::PostAchievementEventsT *)eventHandle;

        DestroyHandle(eventHandle);

        r.UserId = user;
        r.PersonaId = persona;
        
        if(r.Events.size() == 0 || (r.Events.size() > 0 && r.Events.back().Attributes.size() > 0))
        {
            if(req->Execute(timeout))
            {
                OriginErrorT err = req->GetResponse().Code;
                return REPORTERROR(err);
            }

            return REPORTERROR(req->GetErrorCode());
        }
        else
        {
            return REPORTERROR(ORIGIN_ERROR_INVALID_OPERATION);
        }
    }

    OriginErrorT OriginSDK::PostAchievementEventsConvertData(IXmlMessageHandler * /*pHandler*/, int & /*dummy*/, OriginSizeT &size, lsx::ErrorSuccessT &response)
    {
        size = sizeof(int);

        return response.Code;
    }

    OriginErrorT OriginSDK::PostWincodes(OriginUserT user, OriginPersonaT persona, const OriginCharT *authcode, const OriginCharT ** keys, const OriginCharT ** values, OriginSizeT count, uint32_t timeout, OriginErrorSuccessCallback callback, void *pContext)
    {
        if(user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if(persona == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_PERSONA);

        if(keys == NULL || values == NULL || count == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::PostWincodesT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback> > req(
            new LSXRequest<lsx::PostWincodesT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback>
                (GetService(lsx::FACILITY_ACHIEVEMENT), this, &OriginSDK::PostWindcodesConvertData, callback, pContext));

        lsx::PostWincodesT &r = req->GetRequest();

        r.UserId = user;
        r.PersonaId = persona;
        r.AuthCode = authcode;

        for(OriginSizeT i=0; i<count; i++)
        {
            lsx::WincodeT code;
            code.key = keys[i];
            code.value = values[i];

            r.Code.push_back(code);
        }
        
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

    OriginErrorT OriginSDK::PostWincodesSync(OriginUserT user, OriginPersonaT persona, const OriginCharT *authcode, const OriginCharT ** keys, const OriginCharT ** values, OriginSizeT count, uint32_t timeout)
    {
        if(user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if(persona == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_PERSONA);

        if(keys == NULL || values == NULL || count == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::PostWincodesT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::PostWincodesT, lsx::ErrorSuccessT>
                (GetService(lsx::FACILITY_SDK), this));

        lsx::PostWincodesT &r = req->GetRequest();

        r.UserId = user;
        r.PersonaId = persona;
        r.AuthCode = authcode;

        for(OriginSizeT i=0; i<count; i++)
        {
            lsx::WincodeT code;
            code.key = keys[i];
            code.value = values[i];

            r.Code.push_back(code);
        }

        if(req->Execute(timeout))
        {
            return REPORTERROR(req->GetResponse().Code);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::PostWindcodesConvertData(IXmlMessageHandler * /*pHandler*/, int & /*dummy*/, size_t &size, lsx::ErrorSuccessT &response)
    {
        size = sizeof(int);

        return response.Code;
    }



    OriginErrorT OriginSDK::QueryAchievements(OriginUserT user, OriginPersonaT persona, const OriginCharT *pGameIdList[], OriginSizeT gameIdCount, enumAchievementMode mode, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle)
    {
        if(persona == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_PERSONA);

        if(gameIdCount != 0 && pGameIdList == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);
        

        LSXEnumeration<lsx::QueryAchievementsT, lsx::AchievementSetsT, OriginAchievementSetT> * req =
            new LSXEnumeration<lsx::QueryAchievementsT, lsx::AchievementSetsT, OriginAchievementSetT>
                (GetService(lsx::FACILITY_ACHIEVEMENT), this, &OriginSDK::QueryAchievementsConvertData, callback, pContext);

        lsx::QueryAchievementsT &r = req->GetRequest();

        r.UserId = user;
        r.PersonaId = persona;
        r.All = (mode == ACHIEVEMENT_MODE_ALL);
        
        for(uint32_t i=0; i<gameIdCount; i++)
        {
            if(pGameIdList[i] != NULL && pGameIdList[i][0] != 0)
            {
                r.GameId.push_back(pGameIdList[i]);
            }
        }

        OriginHandleT h = RegisterResource(OriginResourceContainerT::Enumerator, req);

        if(pHandle != NULL)
        {
            *pHandle = h;
        }

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

    OriginErrorT OriginSDK::QueryAchievementsSync(OriginUserT user, OriginPersonaT persona, const OriginCharT *pGameIdList[], OriginSizeT gameIdCount, enumAchievementMode mode, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
    {
        if(persona == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_PERSONA);

        if(gameIdCount != 0 && pGameIdList == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);


        LSXEnumeration<lsx::QueryAchievementsT, lsx::AchievementSetsT, OriginAchievementSetT> * req =
            new LSXEnumeration<lsx::QueryAchievementsT, lsx::AchievementSetsT, OriginAchievementSetT>
                (GetService(lsx::FACILITY_ACHIEVEMENT), this, &OriginSDK::QueryAchievementsConvertData);

        lsx::QueryAchievementsT &r = req->GetRequest();

        r.UserId = user;
        r.PersonaId = persona;
        r.All = (mode == ACHIEVEMENT_MODE_ALL);

        for(uint32_t i=0; i<gameIdCount; i++)
        {
            if(pGameIdList[i] != NULL && pGameIdList[i][0] != 0)
            {
                r.GameId.push_back(pGameIdList[i]);
            }
        }

        *pHandle = RegisterResource(OriginResourceContainerT::Enumerator, req);

        if(req->Execute(timeout, pTotalCount))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::QueryAchievementsConvertData(IEnumerator *pEnumerator, OriginAchievementSetT *dest, size_t index, size_t count, lsx::AchievementSetsT &response)
    {
        assert(index+count <= response.AchievementSet.size());

        for(size_t i=index; i<index + count; i++)
        {
            OriginAchievementSetT &Dest = dest[i-index];
            lsx::AchievementSetT &Src = response.AchievementSet[i];

            ConvertData(pEnumerator, Dest, Src, true);
        }
        return REPORTERROR(ORIGIN_SUCCESS);
    }

    void OriginSDK::ConvertData(IXmlMessageHandler *pHandler, OriginAchievementSetT &dest, const lsx::AchievementSetT &src, bool bCopyStrings)
    {
        dest.AchievementCount = src.Achievement.size();
        dest.Name = ConvertData(pHandler, src.Name, bCopyStrings);
        dest.GameName = ConvertData(pHandler, src.GameName, bCopyStrings);
        dest.pAchievements = NULL;

        if(dest.AchievementCount>0)
        {
            dest.pAchievements = TYPE_NEW(OriginAchievementT, dest.AchievementCount);
            
            pHandler->StoreMemory(dest.pAchievements);

            ConvertData(pHandler, dest.pAchievements, src.Achievement, 0, dest.AchievementCount, bCopyStrings);
        }
    }

    void OriginSDK::ConvertData(IXmlMessageHandler *pHandler, OriginAchievementT *dest, const std::vector<lsx::AchievementT, Origin::Allocator<lsx::AchievementT> > &src, size_t index, size_t count, bool bCopyStrings)
    {
        for(size_t i=index; i<index + count; i++)
        {
            OriginAchievementT &Dest = *(dest + i);
            const lsx::AchievementT &Src = src[i];

            ConvertData(pHandler, Dest, Src, bCopyStrings);
        }
    }
}

