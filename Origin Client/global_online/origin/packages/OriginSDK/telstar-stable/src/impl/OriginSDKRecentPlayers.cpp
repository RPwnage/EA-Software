#include "stdafx.h"
#include "OriginSDK/OriginSDK.h"
#include "OriginSDK/OriginError.h"
#include "OriginSDKimpl.h"

#include "OriginLSXRequest.h"
#include "OriginLSXEnumeration.h"

namespace Origin
{

OriginErrorT OriginSDK::AddRecentPlayers(OriginUserT user, const OriginUserT *pRecentList, size_t recentCount, uint32_t timeout, OriginErrorSuccessCallback callback, void *pContext)
{
    if (user == 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

    if(pRecentList == NULL || recentCount <= 0)
        return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

    LSXRequest<lsx::AddRecentPlayersT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> *req = 
        new LSXRequest<lsx::AddRecentPlayersT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
            (GetService(lsx::FACILITY_SDK), this, &OriginSDK::AddRecentPlayersConvertData, callback, pContext);

    lsx::AddRecentPlayersT &r = req->GetRequest();

    r.UserId = user;
    r.Player.assign(pRecentList, pRecentList + recentCount);

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

OriginErrorT OriginSDK::AddRecentPlayersSync(OriginUserT user, const OriginUserT *pRecentList, size_t recentCount, uint32_t timeout)
{
    if (user == 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

	if(pRecentList == NULL || recentCount <= 0)
		return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

	IObjectPtr< LSXRequest<lsx::AddRecentPlayersT, lsx::ErrorSuccessT> > req(
        new LSXRequest<lsx::AddRecentPlayersT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_RECENTPLAYER), this));

    lsx::AddRecentPlayersT &r = req->GetRequest();

	r.UserId = user;
    r.Player.assign(pRecentList, pRecentList + recentCount);
	
	if(req->Execute(timeout))
	{
		return REPORTERROR(req->GetResponse().Code);
	}
	else
	{
		return REPORTERROR(req->GetErrorCode());
	}
}

OriginErrorT OriginSDK::AddRecentPlayersConvertData(IXmlMessageHandler * /*pHandler*/, OriginErrorT & /*item*/, size_t &size, lsx::ErrorSuccessT &response)
{
    size = 0;

    return response.Code;
}

} // namespace Origin