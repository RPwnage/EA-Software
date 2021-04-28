#include "OfferLaunchGameService.h"
#include "PendingActionFlow.h"
#include "MainFlow.h"
#include "ClientFlow.h"
#include "RTPFlow.h"
#include "engine/content/ContentController.h"

namespace Origin
{
	namespace Sdk
	{
		namespace LocalHost
		{
            OfferLaunchGameService::OfferLaunchGameService(QObject *parent):IService(parent)
            {
                mMethod = "POST";
            }

			ResponseInfo OfferLaunchGameService::processRequest(QUrl requestUrl)
			{
                Origin::Client::PendingActionFlow *pendingActionFlow = Origin::Client::PendingActionFlow::instance();

                ResponseInfo responseInfo;

                if(pendingActionFlow && pendingActionFlow->canStartNewAction(requestUrl))
                {
                    QUrlQuery queryParams(requestUrl.query());

                    QString offerIds = queryParams.queryItemValue(Origin::Client::PendingAction::ParamOfferIdsId, QUrl::FullyDecoded);

                    if(!offerIds.isEmpty())
                    {
                        //we are going to build a origin:// that we pass to handle message, by doing it this way we can
                        //run through the exact same code the origin:// uses to launch rtp
                        QUrl pendingActionGameLaunchUrl;
                        
                        pendingActionGameLaunchUrl.setScheme(Origin::Client::PendingAction::LatestOriginScheme);
                        pendingActionGameLaunchUrl.setHost("game");
                        pendingActionGameLaunchUrl.setPath("/launch");
                        pendingActionGameLaunchUrl.setQuery(queryParams);
                        responseInfo.errorCode = HttpStatus::Accepted;

                        //the second Q_ARG marks whether we clear the commandline params or not, which if set to true, we clear commandline params, if false we do not.
                        //we want to set this to false as Battlelog needs to launch game exes directly. We rely on whether the commandline is empty or not to determine whether to launch the alternate URL or the exe.
                        //We are trusting CORS to stop requests from malicious sites.
                        QMetaObject::invokeMethod (Origin::Client::PendingActionFlow::instance(), "startAction", Qt::QueuedConnection, Q_ARG(QUrl, pendingActionGameLaunchUrl), Q_ARG(bool, false));          

                    }
                    else
                    {
                        //invalid params
                        responseInfo.errorCode = HttpStatus::BadRequest;
                        responseInfo.response = "{\"error\":\"OFFERIDS_MISSING\"}";
                    }

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