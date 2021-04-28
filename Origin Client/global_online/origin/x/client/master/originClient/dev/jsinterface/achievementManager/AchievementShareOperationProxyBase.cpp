#include "AchievementShareOperationProxyBase.h"
#include "services/debug/DebugService.h"
#include <services/rest/AchievementShareServiceResponses.h>

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {

            AchievementShareOperationProxyBase::AchievementShareOperationProxyBase(QObject* parent) :
                QObject(parent)
            {
            }

            void AchievementShareOperationProxyBase::onSuccess()
            {
                Services::AchievementShareServiceResponses *resp = dynamic_cast<Services::AchievementShareServiceResponses*>(sender());

                if (!resp)
                {
                    ORIGIN_ASSERT(0);
                    return;
                }

                QVariant achievementShareInfo;
                QString ba = resp->payload().constData();
                achievementShareInfo.setValue(ba);
                emit succeeded(achievementShareInfo);

                resp->deleteLater();
                deleteLater();
            }

            void AchievementShareOperationProxyBase::failQuery()
            {
                emit failed();
                sender()->deleteLater();
                deleteLater();
            }
        }
    }
}
