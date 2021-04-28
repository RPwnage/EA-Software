#include "RestartGameTask.h"
#include "engine/content/Entitlement.h"
#include "engine/content/LocalContent.h"
#include "services/debug/DebugService.h"
#include "flows/MainFlow.h"
#include "flows/ContentFlowController.h"

#define DEFAULT_TIMEOUT 300000

namespace Origin
{
    namespace Client
    {
        namespace Tasks
        {
            RestartGameTask::RestartGameTask(Origin::Engine::Content::EntitlementRef entitlement, const QString &commandLineArgs, bool forceUpdateGame, bool forceUpdateDLC, int timeout) :
                mForceUpdateGame(forceUpdateGame),
                mForceUpdateDLC(forceUpdateDLC),
                m_commandLineArgs(commandLineArgs)
            {
                if(entitlement)
                {
                    m_timeout.singleShot(timeout ? timeout : DEFAULT_TIMEOUT, this, SLOT(deleteLater()));

                    ORIGIN_VERIFY_CONNECT(entitlement->localContent(), SIGNAL(playFinished(Origin::Engine::Content::EntitlementRef)), 
                        this, SLOT(restartGame(Origin::Engine::Content::EntitlementRef)));
                }
                else
                {
                    deleteLater();
                }
            }

            RestartGameTask::~RestartGameTask()
            {
                emit finished();
            }

            void RestartGameTask::run()
            {
                m_timeout.start();
            }

            void RestartGameTask::kill()
            {
                deleteLater();
            }

            void RestartGameTask::restartGame(Origin::Engine::Content::EntitlementRef entitlement)
            {
                Origin::Client::MainFlow::instance()->contentFlowController()->startPlayFlow(entitlement->contentConfiguration()->productId(), false, m_commandLineArgs, mForceUpdateGame, mForceUpdateDLC);
                
                deleteLater();
            }
        }
    }
}