#include "Origin2PassThroughService.h"
#include "PendingActionFlow.h"
#include <QUrlQuery>
#include <QUuid>
#include "LocalHost/LocalHostServiceHandler.h"
namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            bool Origin2PassThroughService::validateQueryParameters(QMap<QString, QString> params)
            {
                return true;
            }

            ResponseInfo Origin2PassThroughService::processRequest(QUrl requestUrl)
            {
                Origin::Client::PendingActionFlow *pendingActionFlow = Origin::Client::PendingActionFlow::instance();
                
                ResponseInfo responseInfo;

                if(pendingActionFlow && pendingActionFlow->canStartNewAction(requestUrl))
                {
                    QUrlQuery queryParams(requestUrl.query());

                    QUrl pendingActionUrl;

                    pendingActionUrl.setScheme(Origin::Client::PendingAction::LatestOriginScheme);
                    pendingActionUrl.setPath(requestUrl.path().toLower());
                    pendingActionUrl.setQuery(queryParams);
                    responseInfo.errorCode = HttpStatus::Accepted;
                    
                    QString uuid = QUuid::createUuid().toString().remove('-').remove('}').remove('{');

                    responseInfo.response = QString("{\"statuslink\":\"http://127.0.0.1:%1").arg(LocalHostServiceHandler::port()) + QString(requestUrl.path()) + "/status/" + uuid + "\"}";

                    //the second Q_ARG marks whether we clear the commandline params or not, which if set to true, we clear commandline params, if false we do not.
                    //we want to set this to false as Battlelog needs to launch game exes directly. We rely on whether the commandline is empty or not to determine whether to launch the alternate URL or the exe.
                    //We are trusting CORS to stop requests from malicious sites.
                    QMetaObject::invokeMethod (Origin::Client::PendingActionFlow::instance(), "startAction", Qt::QueuedConnection, Q_ARG(QUrl, pendingActionUrl), Q_ARG(bool, false), Q_ARG(QString, uuid));          
                }
                else
                {
                    responseInfo.errorCode = HttpStatus::Conflict;
                    responseInfo.response = "{\"error\":\"ANOTHER_REQUEST_IS_ACTIVE\"}";
                }
                return responseInfo;
            }
        }
    }
}