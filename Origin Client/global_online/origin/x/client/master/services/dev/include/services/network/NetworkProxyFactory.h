#ifndef EBISUNETWORKPROXYFACTORY_H
#define EBISUNETWORKPROXYFACTORY_H

#include <QNetworkProxyFactory>
#include <QElapsedTimer>
#include <QMutex>
#include <QAtomicInt>
#include <QAtomicPointer>
#include <QObject>

#include "services/plugin/PluginAPI.h"

class QThread;

namespace Origin
{
	namespace Services
	{
		class SystemProxyScoutThread;
		class ORIGIN_PLUGIN_API NetworkProxyFactory : public QObject, public QNetworkProxyFactory
		{
			Q_OBJECT
		public:
			friend class SystemProxyScoutThread;

			NetworkProxyFactory();

			QList<QNetworkProxy> queryProxy(const QNetworkProxyQuery &query);

			protected slots:
				void scoutThreadFinished();

		protected:
			void scoutCompleted();
			qint64 scoutTimeElapsed();

			QAtomicInt m_systemProxyTimely;

			// Protects the thread against concurrent deletion
			QMutex m_systemProxyScoutThreadMutex;
			QThread *m_systemProxyScoutThread;

			QMutex m_scoutTimeMutex;
			QElapsedTimer m_scoutTimeElapsed;
		};
	}
}

#endif