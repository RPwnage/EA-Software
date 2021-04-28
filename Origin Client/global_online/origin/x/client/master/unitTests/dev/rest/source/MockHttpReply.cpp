#include <QDebug>
#include <QDir>
#include <QRegExp>
#include <QMetaType>
#include <QtAlgorithms>

#include "../MockHttpReply.h"
#include "services/rest/HttpStatusCodes.h"

using namespace Origin::Services;

MockHttpReply::MockHttpReply(const QDir &fixtureRoot, const QNetworkRequest &req, QNetworkAccessManager::Operation op) :
m_state(Running)
{
	// Set up our basics
	setRequest(req);
	setOperation(op);

	// Find the path to our fixture
	QString fixturePath = fixtureRoot.absolutePath() + fixtureFileName(req.url());

	// Defer this so our caller has time to connect to our signals
	QMetaObject::invokeMethod(this, "completeRequest", Qt::QueuedConnection, Q_ARG(QString, fixturePath));

	qRegisterMetaType<QNetworkReply::NetworkError>("QNetworkReply::NetworkError");
}
	
QString MockHttpReply::fixtureFileName(const QUrl &url)
{
	QString fileName = url.path();

	// Add the query parts in sorted order so we don't depend on the order
	// they were added
	QList<QPair<QByteArray, QByteArray> > queryItems = url.encodedQueryItems();
	qSort(queryItems.begin(), queryItems.end());

	for(QList<QPair<QByteArray, QByteArray> >::ConstIterator it = queryItems.constBegin();
		it != queryItems.end();
		it++)
	{
		// Don't use ? as it's reserved - delimit all pairs with ;
		fileName += ";" + (*it).first + "=" + (*it).second; 
	}

	return fileName;
}

void MockHttpReply::abort(NetworkError code)
{
	m_state = Aborted;
	setError(code, "Transaction aborted");
	emit error(code);
	emit finished();
}

void MockHttpReply::abort()
{
	abort(OperationCanceledError);
}
	
qint64 MockHttpReply::readData(char *data, qint64 maxSize)
{
	return m_mockContent.read(data, maxSize);
}

qint64 MockHttpReply::writeData(const char *data, qint64 maxSize)
{
	qWarning() << "MockHttpReply: Attempt to write to a MockHttpReply";
	return -1;
}
	
QNetworkReply::NetworkError MockHttpReply::errorForStatusCode(unsigned int statusCode)
{
	if ((statusCode >= Http200Ok) && (statusCode < Http400ClientErrorBadRequest))
	{
		return QNetworkReply::NoError;
	}
	else if (statusCode == Http401ClientErrorUnauthorized)
	{
		return QNetworkReply::ContentAccessDenied;
	}
	else if (statusCode == Http403ClientErrorForbidden)
	{
		return QNetworkReply::ContentOperationNotPermittedError;
	}
	else if (statusCode >= Http500InternalServerError)
	{
		return QNetworkReply::UnknownContentError;
	}

	qWarning() << "MockHttpReply: Unmapped HTTP status code: " << statusCode;
	return QNetworkReply::UnknownNetworkError;
}

void MockHttpReply::completeRequest(QString fixturePath)
{
	if (!parseFixtureFile(fixturePath))
	{
		abort(QNetworkReply::ProtocolFailure);
	}
	else
	{
		emit(metaDataChanged());

		m_state = Finished;
	
		if (error() != QNetworkReply::NoError)
		{
			emit error(error());
		}
	
		emit finished();
	}
}

bool MockHttpReply::parseFixtureFile(QString fixturePath)
{
	QFile fixtureFile(fixturePath);

	if (!fixtureFile.open(QIODevice::ReadOnly))
	{
		// Bad bad bad
		qWarning() << "MockHttpReply: Fixture not found" << fixturePath;
		return false;
	}

	// Parse the HTTP status line
	QByteArray statusLine = fixtureFile.readLine();
	// Chop off the newline
	statusLine.chop(1);
	const QRegExp statusMatch("HTTP/1.[0-1] (\\d+) (.*)");

	if (!statusMatch.exactMatch(QString::fromLatin1(statusLine)))
	{
		qWarning() << "MockHttpReply: Can't parse status line" << statusLine;
		return false;
	}

	// Populate status related attributes
	int statusCode = statusMatch.cap(1).toInt();
	setAttribute(QNetworkRequest::HttpStatusCodeAttribute, QVariant(statusCode));
	setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, QVariant(statusMatch.cap(2).toLatin1()));
	
	setError(errorForStatusCode(statusCode), "Mocked error");

	// XXX: This doesn't support header continuations across lines as allowed by RFC 2616
	const QRegExp headerMatch("([a-zA-Z0-9\\-]+):(.*)");
	while(true)
	{
		QByteArray headerLine = fixtureFile.readLine();

		if (headerLine.length() == 0)
		{
			// We ran out of file. This means the response has no body
			return true;
		}
		else if (headerLine == "\r\n")
		{
			// Blank line separating the header from data

			// Read everything in to our mock content buffer and open it for reading
			m_mockContent.setData(fixtureFile.readAll());
			m_mockContent.open(QIODevice::ReadOnly);

			// Also open ourselves for reading
			setOpenMode(QIODevice::ReadOnly);
			
			return true;
		}

		// This should be a header field
		if (!headerMatch.exactMatch(QString::fromLatin1(headerLine)))
		{
			qWarning() << "MockHttpReply: Can't parse header line" << headerLine;
			return false;
		}

		setRawHeader(headerMatch.cap(1).toLatin1(), headerMatch.cap(2).trimmed().toLatin1());
	}
}