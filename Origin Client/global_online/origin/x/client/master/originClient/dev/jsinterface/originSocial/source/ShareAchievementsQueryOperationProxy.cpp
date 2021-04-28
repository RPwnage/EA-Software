#include "ShareAchievementsQueryOperationProxy.h"

#include "services/rest/AtomServiceResponses.h"
#include "services/rest/AtomServiceClient.h"
#include "services/debug/DebugService.h"

#include "engine/login/LoginController.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            using Engine::UserRef;

            ShareAchievementsQueryOperationProxy::ShareAchievementsQueryOperationProxy(QObject *parent, quint64 nucleusId) :
                QueryOperationProxyBase(parent)
            {
                UserRef user = Engine::LoginController::instance()->currentUser(); 

                if (!user)
                {
                    // Need a user
                    QMetaObject::invokeMethod(this, "failed", Qt::QueuedConnection);
                    return;
                }

                if (static_cast<quint64>(user->userId()) == nucleusId)
                    emit succeeded(true);

                Services::ShareAchievementsResponse *resp = Services::AtomServiceClient::shareAchievements(user->getSession(), nucleusId);

                ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(onSuccess()));
                ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(failQuery()));
                ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onQueryError(Origin::Services::HttpStatusCode)));

            }

            void ShareAchievementsQueryOperationProxy::onSuccess()
            {
                Services::ShareAchievementsResponse *resp = dynamic_cast<Services::ShareAchievementsResponse*>(sender());

                if (!resp)
                {
                    ORIGIN_ASSERT(0);
                    deleteLater();
                    return;
                }

                emit succeeded(resp->isShareAchievements());

                resp->deleteLater();
                deleteLater();
            }


        }
    }
}
