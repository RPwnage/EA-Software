///////////////////////////////////////////////////////////////////////////////
//
//  Copyright c 2009 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File:	Utility_ServiceArea.cpp
//	Utility manager service area definition
//
//	Author: Sergey Zavadski


#include "Service/UtilityService/Utility_ServiceArea.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformService.h"
#include "services/connection/ConnectionStatesService.h"
#include "engine/login/LoginController.h"
#include "version/version.h"
#include "services/rest/AuthPortalServiceClient.h"

#include <lsx.h>
#include <lsxreader.h>
#include <lsxwriter.h>
#include <EbisuError.h>

#if defined(_MSC_VER)
    #pragma warning(disable: 4996) // This function or variable may be unsafe. Consider using wcscpy_s instead.
#endif

namespace Origin
{
    namespace SDK
    {
        // Singleton functions
        static Utility_ServiceArea* mpSingleton = NULL;

        Utility_ServiceArea* Utility_ServiceArea::create(Lsx::LSX_Handler* handler)
        {
	        if (mpSingleton == NULL)
	        {
		        mpSingleton = new Utility_ServiceArea(handler);
	        }
	        return mpSingleton;
        }

        Utility_ServiceArea* Utility_ServiceArea::instance()
        {
	        return mpSingleton;
        }

        void Utility_ServiceArea::destroy()
        {
	        delete mpSingleton;
	        mpSingleton = NULL;
        }

        Utility_ServiceArea::Utility_ServiceArea( Lsx::LSX_Handler* handler )
	        : BaseService( handler, "Utility" )
        {
	        //
	        //	Update  manager
	        //
	        registerHandler("GetInternetConnectedState", ( BaseService::RequestHandler ) &Utility_ServiceArea::getInternetConnectedState );
	        registerHandler("GetAuthToken", ( BaseService::RequestHandler ) &Utility_ServiceArea::getAccessToken );
            registerHandler("GetAccessToken", ( BaseService::RequestHandler ) &Utility_ServiceArea::getAccessToken );
            registerHandler("GetAuthCode", ( BaseService::RequestHandler ) &Utility_ServiceArea::getAuthCode );
        }

        Utility_ServiceArea::~Utility_ServiceArea(void)
        {
        }

        void Utility_ServiceArea::getInternetConnectedState( Lsx::Request& request, Lsx::Response& response )
        {
            Origin::Engine::UserRef user = Engine::LoginController::instance()->currentUser();

            if(user)
            {
	            int connected = Services::Connection::ConnectionStatesService::isUserOnline (Engine::LoginController::instance()->currentUser()->getSession());

				lsx::InternetConnectedStateT resp;
				resp.connected = connected;

				lsx::Write(response.document(), resp);
            }
            else
            {
                lsx::ErrorSuccessT resp;

                resp.Code = EBISU_ERROR_INVALID_USER;
                resp.Description = "The user is not logged in.";

                lsx::Write(response.document(), resp);
            }
        }

        /*void Utility_ServiceArea::getVersion( Lsx::Request& request, Lsx::Response& response )
        {
	        QDomElement contentNode = response.document().createElement( "Version" );
	        contentNode.setAttribute("text", EALS_VERSION_STRING_TEXT );	
	        contentNode.setAttribute("date", EALS_VERSION_BUILD_TIME );
	        response.addContent( contentNode  );
        }

        void Utility_ServiceArea::getAvailableDiskSpace( Lsx::Request& request, Lsx::Response& response )
        {
	        int64_t available_cache = 0;
	        int64_t available_dip = 0;

	        QString cacheDir = Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOAD_CACHEDIR);
	        ORIGIN_ASSERT( !cacheDir.isEmpty() );
	        if( !cacheDir.isEmpty() )
	        {
		        uint64_t totalBytes = 0;
		        uint64_t totalFreeBytes = 0;
		        int64_t availableFreeBytes = Services::PlatformService::GetFreeDiskSpace(cacheDir, &totalBytes, &totalFreeBytes);

		        if( availableFreeBytes >= 0 )
		        {
                    ORIGIN_LOG_DEBUG << "Available disk space (cache): available free " << availableFreeBytes << ", total available " << totalBytes << ", total free " << totalFreeBytes;
			        available_cache = availableFreeBytes;
		        }
	        }

	        QString dipDir = Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOADINPLACEDIR);
	        ORIGIN_ASSERT( !dipDir.isEmpty() );
	        if( !dipDir.isEmpty() )
	        {
		        uint64_t totalBytes = 0;
		        uint64_t totalFreeBytes = 0;
		        int64_t availableFreeBytes = Services::PlatformService::GetFreeDiskSpace(dipDir, &totalBytes, &totalFreeBytes);

		        if( availableFreeBytes >= 0 )
		        {
                    ORIGIN_LOG_DEBUG << "Available disk space (cachedip available free " << availableFreeBytes << ", total available " << totalBytes << ", total free " << totalFreeBytes;
			        available_dip = availableFreeBytes;
		        }
	        }

	        QDomElement content = response.document().createElement( "AvailableDiskSpace" );
	        content.setAttribute("available_cache", QString::number(available_cache) );
	        content.setAttribute("available_dip", QString::number(available_dip) );
	        response.addContent( content );
        }*/

        void Utility_ServiceArea::getAccessToken( Lsx::Request& request, Lsx::Response& response )
        {
            QString authtoken = Origin::Services::Session::SessionService::accessToken(Origin::Services::Session::SessionService::currentSession());

			lsx::AuthTokenT resp;
			resp.value = authtoken;

			lsx::Write(response.document(), resp);
        }

        void Utility_ServiceArea::getAuthCode(Lsx::Request& request, Lsx::Response& response)
        {
            using namespace Origin::Services;

            Origin::Engine::UserRef user = Origin::Engine::LoginController::instance()->currentUser();
            if(Services::Connection::ConnectionStatesService::isUserOnline(user->getSession()))
            {
                lsx::GetAuthCodeT payload;

                if(lsx::Read(request.document(), payload))
                {
                    QString accessToken = Origin::Services::Session::SessionService::accessToken(Origin::Services::Session::SessionService::currentSession());

                    QStringList scopeList;

                    if(!payload.Scope.isEmpty())
                    {
                        scopeList = payload.Scope.split(" ");
                    }

                    auto * resp = AuthPortalServiceClient::retrieveAuthorizationCode(accessToken, payload.ClientId, scopeList);
                    auto * handler = new ResponseWrapper<Utility_ServiceArea, AuthPortalAuthorizationCodeResponse>(this, &Utility_ServiceArea::AuthCodeResponse, resp, response);

                    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), handler, SLOT(finished()));
                }
            }
            else
            {
                lsx::ErrorSuccessT err;

                err.Code = EBISU_ERROR_NO_SERVICE;
                err.Description = "You need to be online to get an AuthCode.";

                lsx::Write(response.document(), err);
            }
        }

        bool Utility_ServiceArea::AuthCodeResponse(Origin::Services::AuthPortalAuthorizationCodeResponse *resp, Lsx::Response& response)
        {
            if(resp->httpStatus() == Origin::Services::Http302Found)
            {
                if (resp->authCodeErrorCode() == "")
                {
                    lsx::AuthCodeT code;

                    code.value = resp->authorizationCode().toLatin1();

                    lsx::Write(response.document(), code);
                }
                else
                {
                    lsx::ErrorSuccessT err;

                    if (resp->authCodeErrorCode() == "user_underage")
                    {
                        err.Code = EBISU_ERROR_AGE_RESTRICTED;
                        err.Description = resp->reply()->header(QNetworkRequest::LocationHeader).toString();
                    }
                    else if (resp->authCodeErrorCode() == "user_banned")
                    {
                        err.Code = EBISU_ERROR_BANNED;
                        err.Description = resp->reply()->header(QNetworkRequest::LocationHeader).toString();
                    }
                    else
                    {
                        err.Code = EBISU_ERROR_INVALID_USER;
                        err.Description = resp->reply()->header(QNetworkRequest::LocationHeader).toString();
                    }

                    lsx::Write(response.document(), err);
                }
            }
            else
            {
                lsx::ErrorSuccessT err;

                err.Code = EBISU_ERROR_PROXY + resp->httpStatus();
                err.Description = resp->reply()->readAll();

                lsx::Write(response.document(), err);
            }

            return true;
        }
    }
}
