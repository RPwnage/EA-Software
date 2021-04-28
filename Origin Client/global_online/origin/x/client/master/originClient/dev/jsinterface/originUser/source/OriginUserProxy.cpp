#include "OriginUserProxy.h"
#include "ClientFlow.h"
#include "services/session/SessionService.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {

            OriginUserProxy::OriginUserProxy(Engine::UserRef user) :
                mUser(user)
            {
                ORIGIN_VERIFY_CONNECT(
                    Services::Session::SessionService::instance(), SIGNAL(sidRenewalResponse(qint32, qint32)),
                    this, SIGNAL(sidRenewalResponseProxy(qint32, qint32)));
            }

            bool OriginUserProxy::socialAllowed()
            {
                return !mUser->isUnderAge();
            }

            bool OriginUserProxy::commerceAllowed()
            {
                return !mUser->isUnderAge();
            }

            QString OriginUserProxy::originId()
            {
                return mUser->eaid();
            }

            QString OriginUserProxy::nucleusId()
            {
                return QString("%1").arg(mUser->userId());
            }

            QString OriginUserProxy::personaId()
            {
                return QString("%1").arg(mUser->personaId());
            }

            QString OriginUserProxy::dob()
            {
                return QString("%1").arg(mUser->dob());
            }

            QVariantMap OriginUserProxy::offlineUserInfo()
            {
                //combine nucleusId, personaId and originId into one object
                QJsonObject userInfo;

                userInfo["nucleusId"] = nucleusId();
                userInfo["personaId"] = personaId();
                userInfo["originId"] = originId();
                userInfo["dob"] = dob();
                return userInfo.toVariantMap();
            }

            void OriginUserProxy::requestLogout()
            {
                if (ClientFlow::instance())
                {
                    ClientFlow::instance()->requestLogoutFromJS();
                }
            }

            void OriginUserProxy::requestSidRefresh()
            {
                Services::Session::SessionService::renewSid(Services::Session::SessionService::currentSession());
            }

        }
    }
}