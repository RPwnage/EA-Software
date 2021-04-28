#include "Origin2StatusService.h"
#include <QStringList>
#include "PendingActionFlow.h"
namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {

            QString Origin2StatusService::stateToStringLookup(int state)
            {
                using namespace Origin::Client;

                QString stateString;
                switch(state)
                {
                case PendingActionFlow::Inactive:
                    stateString = "INACTIVE";
                    break;
                    case PendingActionFlow::WaitingForLogin:
                    stateString = "WAITING_FOR_LOGIN";
                    break;
                case PendingActionFlow::WaitingForMyGames:
                    stateString = "WAITING_FOR_MYGAMES";
                    break;
                case PendingActionFlow::ActionComplete:
                    stateString = "ACTION_COMPLETE";
                    break;
                case PendingActionFlow::ErrorInvalidParams:
                    stateString = "ERROR_INVALID_PARAMS";
                    break;
                }

                return stateString;
            }

            ResponseInfo Origin2StatusService::processRequest(QUrl requestUrl)
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
                        responseInfo.response = QString("{\"status\":\"%1\"}").arg(stateToStringLookup(Origin::Client::PendingActionFlow::instance()->actionStatus()));
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