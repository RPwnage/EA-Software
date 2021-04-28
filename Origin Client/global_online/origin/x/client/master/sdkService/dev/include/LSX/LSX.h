#ifndef LSX_H
#define LSX_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <wchar.h>

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
            class LSX_Handler;
            class Server;

            class LSXThread : public QThread
            {
            public:
                LSXThread();
                virtual ~LSXThread();

                // This wil block until the middleware is initialized
                Lsx::Server *lsxServer();


                // For testing SDK games standalone.
                LSX_Handler* gethandler() {return m_handler;};


            protected:
                void run();
                void waitForReady();

                bool m_ready;
                QMutex m_readyMutex;
                QWaitCondition m_readyCondition;

                LSX_Handler *m_handler;
            };
        }
    }
}

#endif // LSX_H

