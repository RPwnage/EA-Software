#ifndef MOCKNETWORKACCESSMANAGER_H
#define MOCKNETWORKACCESSMANAGER_H

#include "services/rest/NetworkAccessManager.h"
#include <QDir>

namespace Origin
{
	namespace Services
	{
		///
		/// Class for supporting our mock:// protocol
		///
		class MockNetworkAccessManager : public NetworkAccessManager
		{
		public:
			MockNetworkAccessManager(const QDir &fixtureRoot);

		protected:
			virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &req, QIODevice * outgoingData = 0);

			QDir m_fixtureRoot;
		};
	}
}

#endif