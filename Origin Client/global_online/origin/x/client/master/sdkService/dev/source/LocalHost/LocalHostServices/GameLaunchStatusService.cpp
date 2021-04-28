#include "GameLaunchStatusService.h"
#include <QStringList>
#include "PendingActionFlow.h"
#include "MainFlow.h"
#include "RTPFlow.h"
namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            QString GameLaunchStatusService::rtpStateToStringLookup(int state)
            {
                using namespace Origin::Client;

                QString stateString;
                switch(state)
                {
                case RTPFlow::GameLaunchInactive:
                    stateString = "GAME_LAUNCH_INACTIVE";
                    break;
                case RTPFlow::GameLaunchCheckingLogin:
                    stateString = "GAME_LAUNCH_CHECKING_LOGIN";
                    break;
                case RTPFlow::GameLaunchWaitingToLogin:
                    stateString = "GAME_LAUNCH_WAITING_TO_LOGIN";
                    break;
                case RTPFlow::GameLaunchDownloading:
                    stateString = "GAME_LAUNCH_DOWNLOADING";
                    break;
                case RTPFlow::GameLaunchPending:
                    stateString = "GAME_LAUNCH_PENDING";
                    break;
                case RTPFlow::GameLaunchSuccess:
                    stateString = "GAME_LAUNCH_SUCCESS";
                    break;
                case RTPFlow::GameLaunchFailed:
                    stateString = "GAME_LAUNCH_FAILED";
                    break;
                case RTPFlow::GameLaunchAlreadyPlaying:
                    stateString = "GAME_LAUNCH_ALREADY_PLAYING";
                    break;
                }

                return stateString;
            }

            ResponseInfo GameLaunchStatusService::processRequest(QUrl requestUrl)
            {
                ResponseInfo responseInfo;

                QString opcode;
                QStringList pathPartsList = requestUrl.path().split('/');

                opcode = QUrl::fromPercentEncoding(pathPartsList[pathPartsList.size()-1].toLocal8Bit());

                if(!opcode.isEmpty())
                {
                    if(opcode.compare(Origin::Client::PendingActionFlow::instance()->activeUUID(), Qt::CaseInsensitive) == 0)
                    {
                        responseInfo.errorCode = HttpStatus::OK;

                        if(Origin::Client::PendingActionFlow::instance()->actionStatus() == Origin::Client::PendingActionFlow::ActionComplete)
                        {
                            Origin::Client::RTPFlow *rtpFlow = Origin::Client::MainFlow::instance()->rtpFlow();
                            responseInfo.response = QString("{\"status\":\"%1\"}").arg(rtpStateToStringLookup(rtpFlow->status()));
                        }
                        else
                        {
                            responseInfo.response = QString("{\"status\":\"%1\"}").arg(stateToStringLookup(Origin::Client::PendingActionFlow::instance()->actionStatus()));

                        }
                    }
                    else
                    {
                        //the uuid's don't match so a request with the op code is not allowed
                        responseInfo.errorCode = HttpStatus::Forbidden;
                        responseInfo.response = "{\"error\":\"OPCODE_INVALID\"}";
                    }
                }
                else
                {
                    responseInfo.errorCode = HttpStatus::NotFound;
                    responseInfo.response = "{\"error\":\"OPCODE_MISSING\"}";
                }
                return responseInfo;
            }
        }
    }
}