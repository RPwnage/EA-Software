#include <QTest>
#include <QNetworkReply>
#include <QDebug>

#include "../MockNetworkAccessManager.h"
#include "../MockHttpReply.h"

namespace Origin
{
	namespace Services
	{
		MockNetworkAccessManager::MockNetworkAccessManager(const QDir &fixtureRoot) :
	m_fixtureRoot(fixtureRoot)
	{
	}

	QNetworkReply *MockNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
	{
		QString scheme = req.url().scheme();

		// White list of request types we're allowed to make as they only refer
		// to local resources
		if (scheme == "file" ||
			scheme == "data"||
			scheme == "qrc")
		{
			return QNetworkAccessManager::createRequest(op, req, outgoingData);
		}
		else if (scheme == "mock")
		{
			return new MockHttpReply(m_fixtureRoot, req, op);
		}

		// Hopefully this will sufficiently ruin our caller's day
		qWarning() << "Attempted actual network access through MockNetworkAccessManager to" << req.url().toString();
		return NULL;
	}
	}
}
