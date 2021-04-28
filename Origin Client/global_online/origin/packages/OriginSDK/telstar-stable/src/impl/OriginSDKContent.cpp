#include "stdafx.h"
#include "OriginSDK/OriginSDK.h"
#include "OriginSDK/OriginError.h"
#include "OriginSDKimpl.h"
#include "assert.h"

#include "OriginLSXRequest.h"
#include "OriginLSXEnumeration.h"

#include "encryption/base64.h"

namespace Origin
{

    OriginErrorT OriginSDK::QueryContent(OriginUserT user, const OriginCharT *gameIds[], OriginSizeT gameIdCount, const OriginCharT * multiplayerId, uint32_t contentType, OriginEnumerationCallbackFunc callback, void* context, uint32_t timeout, OriginHandleT *handle)
    {
        if (gameIdCount != 0 && gameIds == NULL)
            return ORIGIN_ERROR_INVALID_ARGUMENT;

        if (callback == NULL && handle == NULL)
            return ORIGIN_ERROR_INVALID_ARGUMENT;

        LSXEnumeration<lsx::QueryContentT, lsx::QueryContentResponseT, OriginContentT> * req =
            new LSXEnumeration<lsx::QueryContentT, lsx::QueryContentResponseT, OriginContentT>
                (GetService(lsx::FACILITY_CONTENT), this, &OriginSDK::QueryContentConvertData, callback, context);

        lsx::QueryContentT &r = req->GetRequest();

        r.UserId = user;
        r.ContentType = contentType;
        r.MultiplayerId = multiplayerId == NULL ? "" : multiplayerId;

        r.GameId.reserve(gameIdCount);
        for (unsigned int i = 0; i < gameIdCount; i++)
        {
            r.GameId.push_back(gameIds[i]);
        }


        OriginHandleT h = RegisterResource(OriginResourceContainerT::Enumerator, req);

        if (handle != NULL)
        {
            *handle = h;
        }

        // Dispatch the command.
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


    OriginErrorT OriginSDK::QueryContentSync(OriginUserT user, const OriginCharT *gameIds[], OriginSizeT gameIdCount, const OriginCharT * multiplayerId, uint32_t contentType, OriginSizeT *total, uint32_t timeout, OriginHandleT *handle)
    {
        if (gameIdCount != 0 && gameIds == NULL)
            return ORIGIN_ERROR_INVALID_ARGUMENT;

        if (handle == NULL)
            return ORIGIN_ERROR_INVALID_ARGUMENT;

        LSXEnumeration<lsx::QueryContentT, lsx::QueryContentResponseT, OriginContentT> * req =
            new LSXEnumeration<lsx::QueryContentT, lsx::QueryContentResponseT, OriginContentT>
                (GetService(lsx::FACILITY_CONTENT), this, &OriginSDK::QueryContentConvertData);

        lsx::QueryContentT &r = req->GetRequest();

        r.UserId = user;
        r.ContentType = contentType;
        r.MultiplayerId = multiplayerId == NULL ? "" : multiplayerId;

        r.GameId.reserve(gameIdCount);
        for (unsigned int i = 0; i < gameIdCount; i++)
        {
            r.GameId.push_back(gameIds[i]);
        }

        *handle = RegisterResource(OriginResourceContainerT::Enumerator, req);

        // Dispatch the command.
        if (req->Execute(timeout, total))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        return REPORTERROR(req->GetErrorCode());
    }




    OriginErrorT OriginSDK::QueryContentConvertData(IEnumerator *pEnumerator, OriginContentT *pContent, size_t index, size_t count, lsx::QueryContentResponseT &response)
    {
        ConvertData(pEnumerator, pContent, response.Content, index, count, false);

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    void OriginSDK::ConvertData(IXmlMessageHandler *pHandler, OriginContentT *dest, std::vector<lsx::GameT, Origin::Allocator<lsx::GameT> > &src, size_t index, size_t count, bool bCopyStrings)
    {
        assert(index + count <= src.size());

        for (size_t i = index; i < index + count; i++)
        {
            OriginContentT &Dest = dest[i - index];
            lsx::GameT &Src = src[i];

            Dest.ItemId = ConvertData(pHandler, Src.contentID, bCopyStrings);
            Dest.Progress = Src.progressValue;
            Dest.State = ConvertContentState(Src.state);
            Dest.InstalledVersion = ConvertData(pHandler, Src.installedVersion, bCopyStrings);
            Dest.AvailableVersion = ConvertData(pHandler, Src.availableVersion, bCopyStrings);
            Dest.DisplayName = ConvertData(pHandler, Src.displayName, bCopyStrings);
        }
    }

    enumContentState ConvertContentState(lsx::ContentStateT state)
    {
        switch (state)
        {
            case lsx::CONTENTSTATE_READY_TO_DOWNLOAD:
            case lsx::CONTENTSTATE_DOWNLOAD_PAUSED:
            case lsx::CONTENTSTATE_SERVER_QUEUED:
            case lsx::CONTENTSTATE_READY_TO_ACTIVATE:
            case lsx::CONTENTSTATE_READY_TO_DECRYPT:
            case lsx::CONTENTSTATE_READY_TO_UNPACK:
            case lsx::CONTENTSTATE_READY_TO_INSTALL:
            case lsx::CONTENTSTATE_WAITING_TO_DOWNLOAD:
            case lsx::CONTENTSTATE_WAITING_TO_DECRYPT:
            case lsx::CONTENTSTATE_PREPARING_DOWNLOAD:
            case lsx::CONTENTSTATE_UPDATE_PAUSED:
            case lsx::CONTENTSTATE_READY_TO_UPDATE:
                return CONTENT_STATE_WAITING;
            case lsx::CONTENTSTATE_DOWNLOADING:
            case lsx::CONTENTSTATE_FINALIZING_DOWNLOAD:
            case lsx::CONTENTSTATE_UPDATING:
                return CONTENT_STATE_DOWNLOADING;
            case lsx::CONTENTSTATE_DECRYPTING:
            case lsx::CONTENTSTATE_UNPACKING:
            case lsx::CONTENTSTATE_PLAYING:
            case lsx::CONTENTSTATE_VERIFYING:
                return CONTENT_STATE_BUSY;
            case lsx::CONTENTSTATE_INSTALLING:
                return CONTENT_STATE_INSTALLING;
            case lsx::CONTENTSTATE_READY_TO_PLAY:
            case lsx::CONTENTSTATE_READY_TO_USE:
            case lsx::CONTENTSTATE_INSTALLED:
                return CONTENT_STATE_READY;
            case lsx::CONTENTSTATE_DOWNLOAD_QUEUED:
                return CONTENT_STATE_QUEUED;
            case lsx::CONTENTSTATE_DOWNLOAD_EXPIRED:
            case lsx::CONTENTSTATE_DECRYPT_EXPIRED:
            case lsx::CONTENTSTATE_INVALID_CONTENT:
            case lsx::CONTENTSTATE_UNKNOWN:
            default:
                return CONTENT_STATE_UNKNOWN;
        }
    }

    OriginErrorT OriginSDK::RestartGame(OriginUserT user, enumRestartGameOptions option, OriginErrorSuccessCallback callback, void *context, uint32_t timeout)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        IObjectPtr< LSXRequest<lsx::RestartGameT, lsx::ErrorSuccessT, int> > req(
            new LSXRequest<lsx::RestartGameT, lsx::ErrorSuccessT, int>
                (GetService(lsx::FACILITY_CONTENT), this, &OriginSDK::RestartGameConvertData, callback, context));

        lsx::RestartGameT &r = req->GetRequest();

        r.UserId = user;
        r.Options = (lsx::RestartOptionsT)option;

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

    OriginErrorT OriginSDK::RestartGameSync(OriginUserT user, enumRestartGameOptions option, uint32_t timeout)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        IObjectPtr< LSXRequest<lsx::RestartGameT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::RestartGameT, lsx::ErrorSuccessT>
                (GetService(lsx::FACILITY_CONTENT), this));

        lsx::RestartGameT &r = req->GetRequest();

        r.UserId = user;
        r.Options = (lsx::RestartOptionsT)option;

        if (req->Execute(timeout))
        {
            return REPORTERROR(req->GetResponse().Code);
        }

        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::RestartGameConvertData(IXmlMessageHandler * /*pHandler*/, int &/*dummy*/, size_t &/*size*/, lsx::ErrorSuccessT &response)
    {
        return response.Code;
    }

    OriginErrorT OriginSDK::StartGame(const OriginCharT * gameId, const OriginCharT * multiplayerId, const OriginCharT * commandLineArgs, OriginErrorSuccessCallback callback, void *context, uint32_t timeout)
    {
        if((gameId == NULL || gameId[0] == 0) && (multiplayerId == NULL || multiplayerId[0] == 0))
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::StartGameT, lsx::ErrorSuccessT, int> > req(
            new LSXRequest<lsx::StartGameT, lsx::ErrorSuccessT, int>
                (GetService(lsx::FACILITY_CONTENT), this, &OriginSDK::StartGameConvertData, callback, context));

        lsx::StartGameT &r = req->GetRequest();

        r.GameId = gameId != NULL ? gameId : "";
        r.MultiplayerId = multiplayerId != NULL ? multiplayerId : "";
        r.CommandLine = commandLineArgs != NULL ? commandLineArgs : "";

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

    OriginErrorT OriginSDK::StartGameSync(const OriginCharT * gameId, const OriginCharT * multiplayerId, const OriginCharT * commandLineArgs, uint32_t timeout)
    {
        if((gameId == NULL || gameId[0] == 0) && (multiplayerId == NULL || multiplayerId[0] == 0))
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::StartGameT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::StartGameT, lsx::ErrorSuccessT>
                (GetService(lsx::FACILITY_CONTENT), this));

        lsx::StartGameT &r = req->GetRequest();

        r.GameId = gameId != NULL ? gameId : "";
        r.MultiplayerId = multiplayerId != NULL ? multiplayerId : "";
        r.CommandLine = commandLineArgs != NULL ? commandLineArgs : "";

        if(req->Execute(timeout))
        {
            return REPORTERROR(req->GetResponse().Code);
        }

        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::StartGameConvertData(IXmlMessageHandler * /*pHandler*/, int & /*dummy*/, size_t & /*size*/, lsx::ErrorSuccessT &response)
    {
        return response.Code;
    }

    OriginErrorT OriginSDK::ExtendTrial(const OriginCharT *requestTicket, int ticketEngine, int * totalTimeRemaining, int * timeGranted, OriginCharT *timeTicket, OriginSizeT * size, OriginTrialConfig &config, uint32_t timeout)
    {
        if(totalTimeRemaining == NULL || timeGranted == NULL || (timeTicket != NULL && size == NULL))
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::ExtendTrialT, lsx::ExtendTrialResponseT> > req(
            new LSXRequest<lsx::ExtendTrialT, lsx::ExtendTrialResponseT>
                (GetService(lsx::FACILITY_CONTENT), this));

        lsx::ExtendTrialT &r = req->GetRequest();

        r.UserId = GetDefaultUser();
		r.TicketEngine = ticketEngine;
        r.RequestTicket = requestTicket == NULL ? "" : requestTicket;
        
        if(req->Execute(timeout))
        {
            lsx::ExtendTrialResponseT &resp = req->GetResponse();

            if(timeTicket != NULL)
            {
                // The length of the decode text == length of encoded text/4*3
                if(*size < resp.ResponseTicket.length()/4*3)
                {
                    *size = resp.ResponseTicket.length()/4*3;
                    return REPORTERROR(ORIGIN_ERROR_BUFFER_TOO_SMALL);
                }

				if (!resp.ResponseTicket.empty())
				{
					unsigned char * ticket = base64_decode(resp.ResponseTicket.c_str(), resp.ResponseTicket.length(), size);

					memcpy(timeTicket, ticket, *size);

					TYPE_DELETE(ticket);
				}
				else
				{
					*size = 0;
				}
            }

            *totalTimeRemaining = resp.TotalTimeRemaining;
            *timeGranted = resp.TimeGranted;

            config.ExtendBeforeExpire = resp.ExtendBeforeExpireSec * 1000;
            config.RetryAfterFail = resp.RetryAfterFailSec * 1000;
            config.RetryCount = resp.RetryCount;
            config.SleepBeforeNukeTime = resp.SleepBeforeNukeSec * 1000;

            return REPORTERROR(resp.Code);
        }
		else
		{
			lsx::ExtendTrialResponseT resp;

			//Use default configuration
			config.ExtendBeforeExpire = resp.ExtendBeforeExpireSec * 1000;
			config.RetryAfterFail = resp.RetryAfterFailSec * 1000;
			config.RetryCount = resp.RetryCount;
			config.SleepBeforeNukeTime = resp.SleepBeforeNukeSec * 1000;

			return REPORTERROR(req->GetErrorCode());
		}


    }


	OriginErrorT OriginSDK::RequestLicense(const OriginCharT *requestToken, int ticketEngine, OriginCharT *license, OriginSizeT * size, uint32_t timeout)
	{
		if (requestToken == NULL)
			return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

		if (license == NULL || size == NULL)
			return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

		if (*size == 0)
			return REPORTERROR(ORIGIN_ERROR_BUFFER_TOO_SMALL);

		IObjectPtr< LSXRequest<lsx::RequestLicenseT, lsx::RequestLicenseResponseT> > req(
			new LSXRequest<lsx::RequestLicenseT, lsx::RequestLicenseResponseT>
			(GetService(lsx::FACILITY_CONTENT), this));

		lsx::RequestLicenseT &r = req->GetRequest();

		r.UserId = GetDefaultUser();
		r.TicketEngine = ticketEngine;
		r.RequestTicket = requestToken;

		if (req->Execute(timeout))
		{
			lsx::RequestLicenseResponseT &resp = req->GetResponse();

			// See if there is enough space in the buffer.
			if (*size < resp.License.length() + 1)
			{
				*size = resp.License.length() + 1;
				return REPORTERROR(ORIGIN_ERROR_BUFFER_TOO_SMALL);
			}
		
			strcpy(license, resp.License.c_str());

			*size = resp.License.length() + 1;

			return REPORTERROR(ORIGIN_SUCCESS);
		}
		return REPORTERROR(req->GetErrorCode());
	}

	OriginErrorT OriginSDK::InvalidateLicense(uint32_t timeout)
	{
		IObjectPtr< LSXRequest<lsx::InvalidateLicenseT, lsx::ErrorSuccessT> > req(
			new LSXRequest<lsx::InvalidateLicenseT, lsx::ErrorSuccessT>
			(GetService(lsx::FACILITY_CONTENT), this));

		lsx::InvalidateLicenseT &r = req->GetRequest();

		r.UserId = GetDefaultUser();

		if (req->Execute(timeout))
		{
			lsx::ErrorSuccessT &resp = req->GetResponse();

			return REPORTERROR(resp.Code);
		}
		return REPORTERROR(req->GetErrorCode());
	}

}

