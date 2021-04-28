#ifndef MOCKHTTPREPLY_H
#define MOCKHTTPREPLY_H

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QFile>
#include <QBuffer>
#include <QObject>
#include <QDir>

class MockHttpReply : public QNetworkReply
{
	Q_OBJECT
public:
	MockHttpReply(const QDir &fixtureRoot, const QNetworkRequest &req, QNetworkAccessManager::Operation op);

	void abort();
	
protected slots:
	void completeRequest(QString fixturePath);

protected:
	static QString fixtureFileName(const QUrl &url);

	void abort(NetworkError code);
	bool parseFixtureFile(QString fixturePath);

	NetworkError errorForStatusCode(unsigned int statusCode);

	// Proxy for QBuffer
	qint64 readData(char *data, qint64 maxSize);
	qint64 writeData(const char *data, qint64 maxSize);

	QBuffer m_mockContent;

	enum TransactionState
	{
		Running,
		Finished,
		Aborted
	};

	TransactionState m_state;
};

#endif