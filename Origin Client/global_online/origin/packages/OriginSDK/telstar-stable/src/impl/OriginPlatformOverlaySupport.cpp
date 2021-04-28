#include "stdafx.h"
#include "OriginSDK/OriginSDK.h"
#include "OriginSDK/OriginError.h"
#include "OriginSDKimpl.h"
#include "assert.h"

#include "OriginLSXRequest.h"
#include "OriginLSXEnumeration.h"

namespace Origin
{
	OriginErrorT OriginSDK::OverlayStateChanged(bool isUp, OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout)
	{
		IObjectPtr< LSXRequest<lsx::OverlayStateChangedT, lsx::ErrorSuccessT, int> > req(
			new LSXRequest<lsx::OverlayStateChangedT, lsx::ErrorSuccessT, int>
			(GetService(lsx::FACILITY_SDK), this, &OriginSDK::OverlayStateChangedConvertData, callback, pContext));

		lsx::OverlayStateChangedT& r = req->GetRequest();

		r.State = isUp ? lsx::IGOSTATE_UP : lsx::IGOSTATE_DOWN;

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

	OriginErrorT OriginSDK::OverlayStateChangedSync(bool isUp, uint32_t timeout)
	{
		IObjectPtr< LSXRequest<lsx::OverlayStateChangedT, lsx::ErrorSuccessT> > req(
			new LSXRequest<lsx::OverlayStateChangedT, lsx::ErrorSuccessT>
			(GetService(lsx::FACILITY_SDK), this));

		lsx::OverlayStateChangedT& r = req->GetRequest();

		r.State = isUp ? lsx::IGOSTATE_UP : lsx::IGOSTATE_DOWN;
		
		if (req->Execute(timeout))
		{
			return REPORTERROR(req->GetResponse().Code);
		}

		return REPORTERROR(req->GetErrorCode());
	}

	OriginErrorT OriginSDK::OverlayStateChangedConvertData(IXmlMessageHandler* /*pHandler*/, int& /*dummy*/, OriginSizeT& /*size*/, lsx::ErrorSuccessT& response)
	{
		return response.Code;
	}





	OriginErrorT OriginSDK::CompletePurchaseTransaction(uint32_t appId, uint64_t orderId, bool authorized, OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout)
	{
		IObjectPtr< LSXRequest<lsx::SteamPurchaseConfirmationT, lsx::ErrorSuccessT, int> > req(
			new LSXRequest<lsx::SteamPurchaseConfirmationT, lsx::ErrorSuccessT, int>
			(GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::CompletePurchaseTransactionConvertData, callback, pContext));

		lsx::SteamPurchaseConfirmationT& r = req->GetRequest();

		r.AppId = appId;
		r.OrderId = orderId;
		r.Authorized = authorized;

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

	OriginErrorT OriginSDK::CompletePurchaseTransactionSync(uint32_t appId, uint64_t orderId, bool authorized, uint32_t timeout)
	{
		IObjectPtr< LSXRequest<lsx::SteamPurchaseConfirmationT, lsx::ErrorSuccessT> > req(
			new LSXRequest<lsx::SteamPurchaseConfirmationT, lsx::ErrorSuccessT>
			(GetService(lsx::FACILITY_COMMERCE), this));

		lsx::SteamPurchaseConfirmationT& r = req->GetRequest();

		r.AppId = appId;
		r.OrderId = orderId;
		r.Authorized = authorized;

		if (req->Execute(timeout))
		{
			return REPORTERROR(req->GetResponse().Code);
		}

		return REPORTERROR(req->GetErrorCode());
	}

	OriginErrorT OriginSDK::CompletePurchaseTransactionConvertData(IXmlMessageHandler* /*pHandler*/, int& /*dummy*/, OriginSizeT& /*size*/, lsx::ErrorSuccessT& response)
	{
		return response.Code;
	}

	OriginErrorT OriginSDK::RefreshEntitlements(OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout)
	{
		IObjectPtr< LSXRequest<lsx::RefreshEntitlementsT, lsx::ErrorSuccessT, int> > req(
			new LSXRequest<lsx::RefreshEntitlementsT, lsx::ErrorSuccessT, int>
			(GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::RefreshEntitlementsConvertData, callback, pContext));

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

	OriginErrorT OriginSDK::RefreshEntitlementsSync(uint32_t timeout)
	{
		IObjectPtr< LSXRequest<lsx::RefreshEntitlementsT, lsx::ErrorSuccessT> > req(
			new LSXRequest<lsx::RefreshEntitlementsT, lsx::ErrorSuccessT>
			(GetService(lsx::FACILITY_COMMERCE), this));

		if (req->Execute(timeout))
		{
			return REPORTERROR(req->GetResponse().Code);
		}

		return REPORTERROR(req->GetErrorCode());
	}

	OriginErrorT OriginSDK::RefreshEntitlementsConvertData(IXmlMessageHandler* pHandler, int& dummy, OriginSizeT& size, lsx::ErrorSuccessT& response)
	{
		return response.Code;
	}

	OriginErrorT OriginSDK::SetPlatformDLC(DLC* dlc, uint32_t count, OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout)
	{
        if (dlc == nullptr)
            return ORIGIN_ERROR_INVALID_ARGUMENT;
		
		IObjectPtr< LSXRequest<lsx::SetDLCInstalledStateT, lsx::ErrorSuccessT, int> > req(
            new LSXRequest<lsx::SetDLCInstalledStateT, lsx::ErrorSuccessT, int>
            (GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::SetPlatformDLCConvertData, callback, pContext));

        lsx::SetDLCInstalledStateT& r = req->GetRequest();

        for (unsigned int i = 0; i < count; i++)
        {
            // Only sent information for non null appId's 
            if (dlc[i].Id != nullptr)
            {
                lsx::DLCT dlcInfo;
                dlcInfo.Name = dlc[i].Name != nullptr ? dlc[i].Name : "";
                dlcInfo.Id = dlc[i].Id;
                dlcInfo.Installed = dlc[i].Installed;

                r.Offers.push_back(dlcInfo);
            }
        }
        
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

	OriginErrorT OriginSDK::SetPlatformDLCSync(DLC* dlc, uint32_t count, uint32_t timeout)
	{
		if (dlc == nullptr)
			return ORIGIN_ERROR_INVALID_ARGUMENT;

        IObjectPtr< LSXRequest<lsx::SetDLCInstalledStateT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::SetDLCInstalledStateT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_COMMERCE), this));

		lsx::SetDLCInstalledStateT& r = req->GetRequest();

		for (unsigned int i = 0; i < count; i++)
		{
			// Only sent information for non null appId's 
			if (dlc[i].Id != nullptr)
			{
                lsx::DLCT dlcInfo;
                dlcInfo.Name = dlc[i].Name != nullptr ? dlc[i].Name : "";
				dlcInfo.Id = dlc[i].Id;
				dlcInfo.Installed = dlc[i].Installed;

				r.Offers.push_back(dlcInfo);
			}
		}

        if (req->Execute(timeout))
        {
            return REPORTERROR(req->GetResponse().Code);
        }

        return REPORTERROR(req->GetErrorCode());
	}

	OriginErrorT OriginSDK::SetPlatformDLCConvertData(IXmlMessageHandler* pHandler, int& dummy, OriginSizeT& size, lsx::ErrorSuccessT& response)
	{
		return response.Code;
	}

	OriginErrorT OriginSDK::DetermineCommerceCurrency(OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout)
	{
        IObjectPtr< LSXRequest<lsx::DetermineCommerceCurrencyT, lsx::ErrorSuccessT, int> > req(
            new LSXRequest<lsx::DetermineCommerceCurrencyT, lsx::ErrorSuccessT, int>
            (GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::DetermineCommerceCurrencyConvertData, callback, pContext));

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

	OriginErrorT OriginSDK::DetermineCommerceCurrencySync(uint32_t timeout)
	{
        IObjectPtr< LSXRequest<lsx::DetermineCommerceCurrencyT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::DetermineCommerceCurrencyT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_COMMERCE), this));

        if (req->Execute(timeout))
        {
            return REPORTERROR(req->GetResponse().Code);
        }

        return REPORTERROR(req->GetErrorCode());
	}

	OriginErrorT OriginSDK::DetermineCommerceCurrencyConvertData(IXmlMessageHandler* pHandler, int& dummy, OriginSizeT& size, lsx::ErrorSuccessT& response)
	{
		return response.Code;
	}

	OriginErrorT OriginSDK::SendSteamAchievementErrorTelemetry(bool validStat, bool getStat, bool setStat, OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout)
	{
		IObjectPtr< LSXRequest<lsx::SteamAchievementErrorTelemetryT, lsx::ErrorSuccessT, int> > req(
			new LSXRequest<lsx::SteamAchievementErrorTelemetryT, lsx::ErrorSuccessT, int>
			(GetService(lsx::FACILITY_SDK), this, &OriginSDK::SendSteamAchievementErrorTelemetryConvertData, callback, pContext));

		lsx::SteamAchievementErrorTelemetryT& event = req->GetRequest();
		event.validStats = validStat;
		event.getStat = getStat;
		event.setStat = setStat;

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

	OriginErrorT OriginSDK::SendSteamAchievementErrorTelemetrySync(bool validStat, bool getStat, bool setStat, uint32_t timeout)
	{
		IObjectPtr< LSXRequest<lsx::SteamAchievementErrorTelemetryT, lsx::ErrorSuccessT> > req(
			new LSXRequest<lsx::SteamAchievementErrorTelemetryT, lsx::ErrorSuccessT>
			(GetService(lsx::FACILITY_SDK), this));

		lsx::SteamAchievementErrorTelemetryT& event = req->GetRequest();
		event.validStats = validStat;
		event.getStat = getStat;
		event.setStat = setStat;

		if (req->Execute(timeout))
		{
			return REPORTERROR(req->GetResponse().Code);
		}

		return REPORTERROR(req->GetErrorCode());
	}

	OriginErrorT OriginSDK::SendSteamAchievementErrorTelemetryConvertData(IXmlMessageHandler* pHandler, int& dummy, OriginSizeT& size, lsx::ErrorSuccessT& response)
	{
		return response.Code;
	}

    OriginErrorT OriginSDK::SetSteamLocale(const OriginCharT *language, OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout)
    {
        IObjectPtr< LSXRequest<lsx::SetSteamLocaleT, lsx::ErrorSuccessT, int> > req(
            new LSXRequest<lsx::SetSteamLocaleT, lsx::ErrorSuccessT, int>
            (GetService(lsx::FACILITY_COMMERCE), this, &OriginSDK::SetSteamLocaleConvertData, callback, pContext));

        lsx::SetSteamLocaleT& event = req->GetRequest();
        event.Language = language;

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

    OriginErrorT OriginSDK::SetSteamLocaleSync(const OriginCharT *language, uint32_t timeout)
    {
        IObjectPtr< LSXRequest<lsx::SetSteamLocaleT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::SetSteamLocaleT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_COMMERCE), this));

        lsx::SetSteamLocaleT& event = req->GetRequest();
        event.Language = language;

        if (req->Execute(timeout))
        {
            return REPORTERROR(req->GetResponse().Code);
        }

        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::SetSteamLocaleConvertData(IXmlMessageHandler* pHandler, int& dummy, OriginSizeT& size, lsx::ErrorSuccessT& response)
    {
        return response.Code;
    }
}
