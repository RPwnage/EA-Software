///////////////////////////////////////////////////////////////////////////////
// MockHttpReplyTest.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _MOCKHTTPREPLYTEST_H
#define _MOCKHTTPREPLYTEST_H

#include <QObject>
#include <QTest>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDebug>
#include "MockNetworkAccessManager.h"
#include "RequestTestHelper.h"

#include "services/rest/HttpStatusCodes.h"
using namespace Origin::Services;
using namespace RequestTestHelper;

class MockHttpReplyTest : public QObject
{
	Q_OBJECT
public:	
	MockHttpReplyTest() : nam(NULL)
	{
	}
private slots:
	void initTestCase()
	{
		QFileInfo myPath(__FILE__);
		QDir myDir = myPath.dir();

		nam = new MockNetworkAccessManager(myDir.absoluteFilePath("fixtures/mockhttpreply"));
	}

	///
	/// Tests a hello world fixture
	///
	void testSimpleFixture()
	{
		QNetworkRequest req(QUrl("mock:///hello.http"));

		QNetworkReply *reply = nam->get(req);
		ReplyState state = waitForReply(reply);
	
		QCOMPARE(state, ReplySuccess);
		QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), static_cast<int>(Http200Ok));
		QCOMPARE(reply->rawHeader("Accept-Ranges"), QByteArray("bytes"));
		QCOMPARE(reply->readAll(), QByteArray("Hello, world!\r\n"));
	}
	
	///
	/// Tests a fixture with a query string to make sure we build the file name
	/// correctly
	///
	void testQueryStringFixture()
	{
		// Put our query string out of lexical order so we can test that it ends up
		// sorted when we build the filename
		QNetworkRequest req(QUrl("mock:///querystring.http?beta=2&alpha=1"));

		QNetworkReply *reply = nam->get(req);
		ReplyState state = waitForReply(reply);
	
		QCOMPARE(state, ReplySuccess);
		QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), static_cast<int>(Http304NotModified));
	}

	void cleanupTestCase()
	{
		delete nam;
	}

private:
	MockNetworkAccessManager *nam;
};

#endif