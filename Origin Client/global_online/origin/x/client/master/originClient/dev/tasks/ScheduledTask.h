#ifndef __SCHEDULE_TASKS_H__
#define __SCHEDULE_TASKS_H__

#include <QObject>


namespace Origin
{
    namespace Client
    {
        namespace Tasks
        {

            class ScheduledTask : public QObject
            {
                Q_OBJECT
            public:
                ScheduledTask(){};
                virtual ~ScheduledTask(){};

                virtual void kill() = 0;
                virtual void run() = 0;

            signals:
                void finished();
            };
        }
    }
}





#endif // __SCHEDULE_TASKS_H__