#include "stdafx.h"
#include "OriginSDK/OriginSDK.h"
#include "OriginSDK/OriginError.h"
#include "OriginSDKimpl.h"

#include "OriginLSXRequest.h"
#include "OriginLSXEnumeration.h"

namespace Origin
{
    OriginErrorT OriginSDK::ShowIGO(bool bShow, OriginErrorSuccessCallback callback, void* pContext)
    {
        IObjectPtr< LSXRequest<lsx::ShowIGOT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> > req(
            new LSXRequest<lsx::ShowIGOT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
            (GetService(lsx::FACILITY_IGO), this, &OriginSDK::ShowIGOConvertData, callback, pContext));

        FILL_REQUEST_CHECK(ShowIGOBuildRequest(req->GetRequest(), bShow));

        if (req->ExecuteAsync(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }

    }

    OriginErrorT OriginSDK::ShowIGOSync(bool bShow)
    {
        IObjectPtr< LSXRequest<lsx::ShowIGOT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::ShowIGOT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_IGO), this));

        FILL_REQUEST_CHECK(ShowIGOBuildRequest(req->GetRequest(), bShow));

        if (req->Execute(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(req->GetResponse().Code);
        }

        return REPORTERROR(req->GetErrorCode());

    }

    OriginErrorT OriginSDK::ShowIGOBuildRequest(lsx::ShowIGOT& data, bool bShow)
    {
        data.bShow = bShow;

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::ShowIGOConvertData(IXmlMessageHandler * /*pHandler*/, int &/*dummy*/, size_t &/*size*/, lsx::ErrorSuccessT &response)
    {
        return response.Code;
    }

    OriginErrorT OriginSDK::ShowIGOWindow(enumIGOWindow window, OriginUserT user, OriginErrorSuccessCallback callback, void* pContext)
    {
        IObjectPtr< LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> > req(
            new LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
            (GetService(lsx::FACILITY_IGO), this, &OriginSDK::ShowIGOConvertData, callback, pContext));

        FILL_REQUEST_CHECK(ShowIGOWindowBuildRequest(req->GetRequest(), window, user));

        if (req->ExecuteAsync(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::ShowIGOWindowSync(enumIGOWindow window, OriginUserT user)
    {

        IObjectPtr< LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_IGO), this));

        FILL_REQUEST_CHECK(ShowIGOWindowBuildRequest(req->GetRequest(), window, user));

        if (req->Execute(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(req->GetResponse().Code);
        }

        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::ShowIGOWindowBuildRequest(lsx::ShowIGOWindowT& data, enumIGOWindow window, OriginUserT user)
    {
        data.ContentId = m_ContentId;
        data.WindowId = (lsx::IGOWindowT)window;
        data.UserId = user;
        data.Show = true;

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::ShowIGOWindow(enumIGOWindow window, OriginUserT user, OriginUserT target, OriginErrorSuccessCallback callback, void* pContext)
    {
        IObjectPtr< LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> > req(
            new LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
            (GetService(lsx::FACILITY_IGO), this, &OriginSDK::ShowIGOConvertData, callback, pContext));

        FILL_REQUEST_CHECK(ShowIGOWindowBuildRequest(req->GetRequest(), window, user, target));

        if (req->ExecuteAsync(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::ShowIGOWindowSync(enumIGOWindow window, OriginUserT user, OriginUserT target)
    {
        IObjectPtr< LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_IGO), this));

        FILL_REQUEST_CHECK(ShowIGOWindowBuildRequest(req->GetRequest(), window, user, target));

        if (req->Execute(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(req->GetResponse().Code);
        }

        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::ShowIGOWindowBuildRequest(lsx::ShowIGOWindowT& data, enumIGOWindow window, OriginUserT user, OriginUserT target)
    {
        if ((user == 0) || (user != this->GetDefaultUser()))
            return ORIGIN_ERROR_INVALID_USER;

        if (target == 0)
            return ORIGIN_ERROR_INVALID_ARGUMENT;

        data.ContentId = m_ContentId;
        data.WindowId = (lsx::IGOWindowT)window;
        data.UserId = user;
        data.TargetId.push_back(target);
        data.Show = true;

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::ShowIGOWindow(enumIGOWindow window, int32_t iBrowserFeatureFlags, const OriginCharT * pURL, OriginErrorSuccessCallback callback, void* pContext)
    {
        IObjectPtr< LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> > req(
            new LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
            (GetService(lsx::FACILITY_IGO), this, &OriginSDK::ShowIGOConvertData, callback, pContext));

        FILL_REQUEST_CHECK(ShowIGOWindowBuildRequest(req->GetRequest(), window, iBrowserFeatureFlags, pURL));

        if (req->ExecuteAsync(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::ShowIGOWindowSync(enumIGOWindow window, int32_t iBrowserFeatureFlags, const OriginCharT * pURL)
    {
        IObjectPtr< LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_IGO), this));

        FILL_REQUEST_CHECK(ShowIGOWindowBuildRequest(req->GetRequest(), window, iBrowserFeatureFlags, pURL));

        if (req->Execute(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(req->GetResponse().Code);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::ShowIGOWindowBuildRequest(lsx::ShowIGOWindowT& data, enumIGOWindow window, int32_t iBrowserFeatureFlags, const OriginCharT * pURL)
    {
        if (pURL == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        data.ContentId = m_ContentId;
        data.WindowId = (lsx::IGOWindowT)window;
        data.Flags = iBrowserFeatureFlags;
        data.String = pURL;
        data.Show = true;

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::ShowIGOWindow(enumIGOWindow window, OriginUserT user, const OriginUserT * users, size_t userCount, const OriginCharT * pMessage, OriginErrorSuccessCallback callback, void* pContext)
    {
        IObjectPtr< LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> > req(
            new LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
            (GetService(lsx::FACILITY_IGO), this, &OriginSDK::ShowIGOConvertData, callback, pContext));

        FILL_REQUEST_CHECK(ShowIGOWindowBuildRequest(req->GetRequest(), window, user, users, userCount, pMessage));

        if (req->ExecuteAsync(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::ShowIGOWindowSync(enumIGOWindow window, OriginUserT user, const OriginUserT * users, size_t userCount, const OriginCharT * pMessage)
    {
        IObjectPtr< LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_IGO), this));

        FILL_REQUEST_CHECK(ShowIGOWindowBuildRequest(req->GetRequest(), window, user, users, userCount, pMessage));

        if (req->Execute(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(req->GetResponse().Code);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::ShowIGOWindowBuildRequest(lsx::ShowIGOWindowT& data, enumIGOWindow window, OriginUserT user, const OriginUserT * users, size_t userCount, const OriginCharT * pMessage)
    {
        if ((user == 0) || (user != this->GetDefaultUser()))
            return ORIGIN_ERROR_INVALID_USER;
        if ((users == NULL) && (userCount > 0))
            return ORIGIN_ERROR_INVALID_ARGUMENT;
        if (pMessage == NULL)
            pMessage = "";

        data.ContentId = m_ContentId;
        data.WindowId = (lsx::IGOWindowT)window;
        data.UserId = user;
        data.TargetId.reserve(userCount);
        data.Show = true;
        for (unsigned i = 0; i < userCount; i++)
        {
            data.TargetId.push_back(users[i]);
        }

        data.String = pMessage;

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::ShowIGOWindow(enumIGOWindow window, OriginUserT user, const OriginCharT ** ppArgs, size_t argCount, OriginErrorSuccessCallback callback, void* pContext)
    {
        IObjectPtr< LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> > req(
            new LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
            (GetService(lsx::FACILITY_IGO), this, &OriginSDK::ShowIGOConvertData, callback, pContext));

        FILL_REQUEST_CHECK(ShowIGOWindowBuildRequest(req->GetRequest(), window, user, ppArgs, argCount));

        if (req->ExecuteAsync(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::ShowIGOWindowSync(enumIGOWindow window, OriginUserT user, const OriginCharT ** ppArgs, size_t argCount)
    {
        IObjectPtr< LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::ShowIGOWindowT, lsx::ErrorSuccessT>
            (GetService(lsx::FACILITY_IGO), this));

        FILL_REQUEST_CHECK(ShowIGOWindowBuildRequest(req->GetRequest(), window, user, ppArgs, argCount));

        if (req->Execute(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(req->GetResponse().Code);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::ShowIGOWindowBuildRequest(lsx::ShowIGOWindowT& data, enumIGOWindow window, OriginUserT user, const OriginCharT ** ppArgs, size_t argCount)
    {
        if ((user == 0) || (user != this->GetDefaultUser()))
            return ORIGIN_ERROR_INVALID_USER;
        if ((ppArgs == NULL) && (argCount > 0))
            return ORIGIN_ERROR_INVALID_ARGUMENT;

        data.ContentId = m_ContentId;
        data.WindowId = (lsx::IGOWindowT)window;
        data.UserId = user;
        data.Args.reserve(argCount);
        data.Show = true;
        for (unsigned i = 0; i < argCount; i++)
        {
            data.Args.push_back(ppArgs[i] != NULL ? ppArgs[i] : "");
        }

        return REPORTERROR(ORIGIN_SUCCESS);
    }


};