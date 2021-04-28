#ifndef __RESTART_GAME_TASK__
#define __RESTART_GAME_TASK__

#include <QTimer>

#include "ScheduledTask.h"
#include "engine/content/Entitlement.h"

namespace Origin
{
    namespace Client
    {
        namespace Tasks
        {
            class RestartGameTask : public ScheduledTask
            {
                Q_OBJECT
            public:
                RestartGameTask(Origin::Engine::Content::EntitlementRef entitlement, const QString &commandLineArgs = QString(), bool forceUpdateGame = false, bool forceUpdateDLC = false, int timeout = 0);
                virtual ~RestartGameTask();

                virtual void run();
                virtual void kill();

            private slots:
                void restartGame(Origin::Engine::Content::EntitlementRef entitlement); 

            private:
                QTimer m_timeout;
                bool mForceUpdateGame;
                bool mForceUpdateDLC;
                QString m_commandLineArgs;
            };
        }
    }
}



#endif // __RESTART_GAME_TASK__