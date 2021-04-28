#include <QThread>
#include <QUrl>
#include <QMutexLocker>

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/network/NetworkProxyFactory.h"

namespace Origin
{
    namespace Services
    {
        // The maximum amount of time we wait for the system proxy to respond before
        // falling back to a direct connection
        const unsigned int SystemProxyGraceTime = 5000;

        class SystemProxyScoutThread : public QThread
        {
        public:
            SystemProxyScoutThread(Origin::Services::NetworkProxyFactory *homeBase) : m_homeBase(homeBase)
            {
                setObjectName("SystemProxyScoutThread");
            }

        protected:
            void run();
            Origin::Services::NetworkProxyFactory *m_homeBase;
        };

        void SystemProxyScoutThread::run()
        {
            // Try to get the proxy config for a known URL
            QNetworkProxyQuery query(QUrl("http://dm.origin.com/"));
            QNetworkProxyFactory::systemProxyForQuery(query);
            m_homeBase->scoutCompleted();
        }

        NetworkProxyFactory::NetworkProxyFactory() : m_systemProxyTimely(false)
        {
            // Start scouting to precache the system proxy and test if proxy 
            // detection completes in reasonable time
            m_systemProxyScoutThread = new SystemProxyScoutThread(this);

            // Clean up after the thread when it's done
            ORIGIN_VERIFY_CONNECT(m_systemProxyScoutThread, SIGNAL(finished()),
                this, SLOT(scoutThreadFinished()));

            // Time how long this takes
            m_scoutTimeElapsed.start();

            // Let it run
            m_systemProxyScoutThread->start();
        }


        void NetworkProxyFactory::scoutCompleted() 
        {
            // Did they finish in time?
            m_systemProxyTimely = (scoutTimeElapsed() < SystemProxyGraceTime);
        }

        void NetworkProxyFactory::scoutThreadFinished()
        {
            QMutexLocker locker(&m_systemProxyScoutThreadMutex);

            // Clean up after the thread
            delete m_systemProxyScoutThread;
            m_systemProxyScoutThread = NULL;
        }

        QList<QNetworkProxy> NetworkProxyFactory::queryProxy(const QNetworkProxyQuery &query) 
        {
            // Have we proven the system proxy to be hang-free?
            // From Thomas: Why do we use an atomic int ? Shouldn't a "int volatile" be enough for the job?
            if (m_systemProxyTimely.load())
            {
                return QNetworkProxyFactory::systemProxyForQuery(query);
            }

            {
                QMutexLocker locker(&m_systemProxyScoutThreadMutex);

                // Maybe the system proxy scout thread has finished
                if (m_systemProxyScoutThread) 
                {
                    // How long does it have before we give up?
                    qint64 graceMillisecondsLeft = SystemProxyGraceTime - scoutTimeElapsed();

                    if ((graceMillisecondsLeft > 0) &&
                        m_systemProxyScoutThread->wait(graceMillisecondsLeft) &&
                        m_systemProxyTimely.load())
                    {
                        // The scout completed successfully while we were waiting
                        return QNetworkProxyFactory::systemProxyForQuery(query);
                    }
                }
            }

            // We permanently don't trust the system proxy
            // Try a direct connection like in < 8.2
            QList<QNetworkProxy> noProxyList;
            noProxyList.append(QNetworkProxy(QNetworkProxy::NoProxy));

            return noProxyList;
        }

        qint64 NetworkProxyFactory::scoutTimeElapsed()
        {
            // Probably useless but Qt doesn't claim that QElapsedTimer::elapsed() is thread safe
            QMutexLocker locker(&m_scoutTimeMutex);
            return m_scoutTimeElapsed.elapsed();
        }
    }
}
