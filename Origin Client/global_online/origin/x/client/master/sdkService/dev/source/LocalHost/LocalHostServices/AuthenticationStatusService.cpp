#include "AuthenticationStatusService.h"
#include "ClientFlow.h"
#include "engine/login/LoginController.h"
namespace Origin
{
    using namespace Engine;

	namespace Sdk
	{
		namespace LocalHost
		{
            const char sUserLoggedOut[] = "USER_LOGGED_OUT";
            const char sUserLoggedInOnline[] = "USER_LOGGED_IN_ONLINE";
            const char sUserLoggedInOffline[] = "USER_LOGGED_IN_OFFLINE";

            AuthenticationStatusService::AuthenticationStatusService(QObject *parent):IService(parent) 
            {
                mMethod = "GET";
            }
            
            AuthenticationStatusService::~AuthenticationStatusService() 
            {

            }

			ResponseInfo AuthenticationStatusService::processRequest(QUrl requestUrl)
			{
                ResponseInfo responseInfo;
                responseInfo.errorCode = HttpStatus::Forbidden;
                QString status;


                //for our purposes here, if the client flow is not instantiated we consider the user logged out
                //this way we handle the case of single login
                Origin::Client::ClientFlow *clientFlow = Origin::Client::ClientFlow::instance();
                if(!clientFlow || !LoginController::isUserLoggedIn())
                {
                    status = sUserLoggedOut;
                }
                else
                {
                    if(Origin::Services::Connection::ConnectionStatesService::isUserOnline (Origin::Engine::LoginController::instance()->currentUser()->getSession()))
                    {
                        status = sUserLoggedInOnline;
                    }
                    else
                    {
                        status = sUserLoggedInOffline;
                    }
                }

                if(!status.isEmpty())
                {
                    responseInfo.errorCode = HttpStatus::OK;
                    responseInfo.response = QString("{\"status\":\"%1\"}").arg(status);
                }          

                return responseInfo;
			}
		}
	}
}