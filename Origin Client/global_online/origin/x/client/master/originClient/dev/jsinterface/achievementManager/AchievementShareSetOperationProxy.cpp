#include "AchievementShareSetOperationProxy.h"

#include <QMetaObject>

#include "services/debug/DebugService.h"
#include "services/rest/AchievementShareServiceClient.h"
#include <services/rest/AchievementShareServiceResponses.h>
#include "engine/login/LoginController.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {

            using Engine::UserRef;

            AchievementShareSetOperationProxy::AchievementShareSetOperationProxy(QObject* parent, const QVariant&  data) :
                AchievementShareOperationProxyBase(parent)
            {
                UserRef user = Engine::LoginController::instance()->currentUser(); 
                if (!user)
                {
                    // Need a user
                    QMetaObject::invokeMethod(this, "failed", Qt::QueuedConnection);
                    return;
                }

                QByteArray pBa = data.toByteArray();
                Services::AchievementShareServiceResponses* resp = Services::AchievementShareServiceClient::setAchievementsSharing(user->getSession(),user->userId(), pBa);
                ORIGIN_ASSERT(resp);

                ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(failQuery()));
                ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::restError)), this, SLOT(failQuery()));
                ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(onSuccess()));
            }

        }
    }
}
