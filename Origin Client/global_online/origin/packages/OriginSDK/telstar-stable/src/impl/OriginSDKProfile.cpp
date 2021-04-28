#include "stdafx.h"
#include "OriginSDK/OriginSDK.h"
#include "OriginSDK/OriginError.h"
#include "OriginSDKimpl.h"

#include "OriginLSXRequest.h"
#include "OriginLSXEnumeration.h"

namespace Origin
{


OriginErrorT OriginSDK::GetProfileSync(uint32_t userIndex, uint32_t timeout, OriginProfileT *pProfile, OriginHandleT *pHandle)
{
	if (pProfile == NULL || pHandle == NULL)
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

	IObjectPtr< LSXRequest<lsx::GetProfileT, lsx::GetProfileResponseT> > req(
        new LSXRequest<lsx::GetProfileT, lsx::GetProfileResponseT>
            (GetService(lsx::FACILITY_PROFILE), this));

	lsx::GetProfileT &r = req->GetRequest();
	r.index = userIndex;

	if(req->Execute(timeout))
	{
		StringContainer * strings = new StringContainer();

		*pHandle = RegisterResource(OriginResourceContainerT::StringContainer, strings);

        const lsx::GetProfileResponseT &resp = req->GetResponse();

		pProfile->UserId = resp.UserId;
		pProfile->PersonaId = resp.PersonaId;

        SetPersonaId(resp.UserIndex, resp.PersonaId);
        SetUserId(resp.UserIndex, resp.UserId);

		// The number of strings to reserve. If you do not reserve the proper amount all the strings will move in memory !!!
		strings->reserve(6);

		strings->push_back(resp.Persona);
		strings->push_back(resp.AvatarId);
		strings->push_back(resp.Country);
        strings->push_back(resp.GeoCountry);
        strings->push_back(resp.CommerceCountry);
        strings->push_back(resp.CommerceCurrency);

		pProfile->PersonaName = (*strings)[0].c_str();
		pProfile->AvatarId = (*strings)[1].c_str();
		pProfile->Country = (*strings)[2].c_str();
        pProfile->GeoCountry = (*strings)[3].c_str();
        pProfile->CommerceCountry = (*strings)[4].c_str();
        pProfile->CommerceCurrency = (*strings)[5].c_str();

        pProfile->IsUnderAge = resp.IsUnderAge;
        pProfile->IsSubscriber = resp.IsSubscriber;
        pProfile->IsTrialSubscriber = resp.IsTrialSubscriber;
        pProfile->SubscriberLevel = static_cast<enumSubscriberLevel>(resp.SubscriberLevel);
        pProfile->IsSteamSubscriber = resp.IsSteamSubscriber;

		return REPORTERROR(ORIGIN_SUCCESS);
	}
	else
	{
		return REPORTERROR(req->GetErrorCode());
	}
}

OriginErrorT OriginSDK::GetProfile(uint32_t userIndex, uint32_t timeout, OriginResourceCallback callback, void * pcontext)
{
	if (callback == NULL)
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

    IObjectPtr< LSXRequest<lsx::GetProfileT, lsx::GetProfileResponseT, OriginProfileT> > req(
		new LSXRequest<lsx::GetProfileT, lsx::GetProfileResponseT, OriginProfileT>
			(GetService(lsx::FACILITY_PROFILE), this, &OriginSDK::GetProfileConvertData, callback, pcontext));

	lsx::GetProfileT &r = req->GetRequest();
	r.index = userIndex;

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


OriginErrorT OriginSDK::GetProfileConvertData(IXmlMessageHandler * /*pHandler*/, OriginProfileT &profile, size_t &size, lsx::GetProfileResponseT &response)
{
	size = sizeof(OriginProfileT);

	profile.UserId = response.UserId;
	profile.PersonaId = response.PersonaId;

    SetPersonaId(response.UserIndex, response.PersonaId);
    SetUserId(response.UserIndex, response.UserId);

	profile.PersonaName = response.Persona.c_str();
	profile.AvatarId = response.AvatarId.c_str();
	profile.Country = response.Country.c_str();
    profile.GeoCountry = response.GeoCountry.c_str();
    profile.CommerceCountry = response.CommerceCountry.c_str();
    profile.CommerceCurrency = response.CommerceCurrency.c_str();

    profile.IsUnderAge = response.IsUnderAge;
    profile.IsSubscriber = response.IsSubscriber;
    profile.IsTrialSubscriber = response.IsTrialSubscriber;
    profile.SubscriberLevel = static_cast<enumSubscriberLevel>(response.SubscriberLevel);
    profile.IsSteamSubscriber = response.IsSteamSubscriber;

	return REPORTERROR(ORIGIN_SUCCESS);
}


}



