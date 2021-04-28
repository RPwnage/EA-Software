#include <QNetworkCookie>
#include <QDateTime>
#include <QUuid>
#include "PendingActionFlow.h"
#include "services/settings/SettingsManager.h"
#include "services/debug/DebugService.h"
#include "AuthenticationRequestStartService.h"
#include "AuthenticationRequestStatusService.h"
#include "LocalHost/LocalHostServiceHandler.h"
#include "TelemetryAPIDLL.h"

namespace Origin
{
    using namespace Engine;

	namespace Sdk
	{
		namespace LocalHost
		{
            const char sRequestStatusStr[] = "/authentication/requeststatus/";
            AuthenticationRequestStartService::AuthenticationRequestStartService(QObject *parent):IService(parent) 
            {
                mMethod = "POST";
            }
            
            AuthenticationRequestStartService::~AuthenticationRequestStartService() 
            {

            }

			ResponseInfo AuthenticationRequestStartService::processRequest(QUrl requestUrl)
			{
                ResponseInfo responseInfo;

                //incoming path should look like this
                //    /authentication/requeststart/{authtoken}
                //    0 - empty
                //    1 - authentication
                //    2 - requeststart
                //    3 - {authtoken}
                QString authToken;
                const int expectedPathListSize = 5;
                const int authTokenIndex = 4;
                QStringList pathPartsList = requestUrl.path().split('/');

                if(pathPartsList.size() == expectedPathListSize)
                {
                    authToken = QUrl::fromPercentEncoding(pathPartsList[authTokenIndex].toLocal8Bit());
                }

                if(!authToken.isEmpty())
                {
                    QUrl pendingActionAuthUrl;
                    pendingActionAuthUrl.setScheme(Origin::Client::PendingAction::LatestOriginScheme);
                    pendingActionAuthUrl.setHost("currentTab");

                    QUrlQuery urlQuery(pendingActionAuthUrl);

                    if(!authToken.isEmpty())
                        urlQuery.addQueryItem("authToken", authToken);

                    pendingActionAuthUrl.setQuery(urlQuery);

                    QString uuid = QUuid::createUuid().toString().remove('-').remove('}').remove('{');

                    //need to add an extra char so it matches the reg exp
                    AuthenticationRequestStatusService *authReqStatusService = static_cast<AuthenticationRequestStatusService *>(LocalHostServiceHandler::findService(QString(sRequestStatusStr) + "x"));
                    if(authReqStatusService)
                    {
                        authReqStatusService->authStarted(uuid);
                    }

                    //return a 202, accepted
                    responseInfo.errorCode = HttpStatus::Accepted;
                    responseInfo.response = QString("{\"statuslink\":\"http://127.0.0.1:%1").arg(LocalHostServiceHandler::port()) + QString(sRequestStatusStr) + uuid + "\"}";

                    QMetaObject::invokeMethod (Origin::Client::PendingActionFlow::instance(), "startAction", Qt::QueuedConnection, Q_ARG(QUrl, pendingActionAuthUrl), Q_ARG(bool, true));          
               }
                else
                {
                    responseInfo.response = "{\"error\":\"AUTHTOKEN_MISSING\"}";
                    responseInfo.errorCode = HttpStatus::NotFound;
                }
                return responseInfo;
			}
		}
	}
}