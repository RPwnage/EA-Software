#include "AchievementShareQueryOperationProxy.h"

#include <QMetaObject>

#include "services/rest/AchievementShareServiceClient.h"
#include <services/rest/AchievementShareServiceResponses.h>
#include "engine/login/LoginController.h"
#include "services/debug/DebugService.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {

            using Engine::UserRef;

            AchievementShareQueryOperationProxy::AchievementShareQueryOperationProxy(QObject *parent) :
                AchievementShareOperationProxyBase(parent)
            {
                UserRef user = Engine::LoginController::instance()->currentUser(); 
                if (!user)
                {
                    // Need a user
                    QMetaObject::invokeMethod(this, "failed", Qt::QueuedConnection);
                    return;
                }

                Services::AchievementShareServiceResponses* resp = Services::AchievementShareServiceClient::achievementsSharing(user->getSession(),user->userId());
                ORIGIN_ASSERT(resp);

                ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(onSuccess()));
                ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onError(Origin::Services::HttpStatusCode)));
                ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(failQuery()));
                ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::restError)), this, SLOT(failQuery()));

            }

            void AchievementShareQueryOperationProxy::onError(Origin::Services::HttpStatusCode sc)
            {
                Services::AchievementShareServiceResponses *resp = dynamic_cast<Services::AchievementShareServiceResponses*>(sender());

                if (!resp)
                {
                    ORIGIN_ASSERT(0);
                    return;
                }

                if (Services::Http403ClientErrorForbidden==sc)
                {
                    QString error = resp->nuke2Error().mErrorDescription;
                    if (error.contains("NO_SUCH_USER_PRIVACY_SETTINGS",Qt::CaseInsensitive))
                    {
                        QVariant achievementShareInfo;
                        QString ba = "{"\
                            "\"pidPrivacySettings\" : "\
                            "{"\
                            "\"facebookDiscoverable\" : \"PENDING\","\
                            "\"psnIdDiscoverable\" : \"PENDING\","\
                            "\"xboxTagDiscoverable\" : \"PENDING\","\
                            "\"showRealName\" : \"PENDING\","\
                            "\"shareAchievements\" : \"PENDING\""\
                            "}"\
                            "}";

                        achievementShareInfo.setValue(ba);
                        emit succeeded(achievementShareInfo);
                    }
                }

                resp->deleteLater();
                deleteLater();
            }


        }
    }
}
