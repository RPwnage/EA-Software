///////////////////////////////////////////////////////////////////////////////
// EntitlementsServiceClientTest.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _ENTITLEMENTSSERVICECLIENTTEST_H
#define _ENTITLEMENTSSERVICECLIENTTEST_H

#include <QObject>
#include <QTest>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QSignalSpy>
#include <QNetworkReply>
#include <QDebug>
#include <QDir>
#include "services/rest/OriginServiceResponse.h"
#include "RequestTestHelper.h"
#include "MockNetworkAccessManager.h"

using namespace RequestTestHelper;
using namespace Origin::Services;
// Test code
class EntitlementsClientTest : public QObject
{
	Q_OBJECT;
private slots:
	void initTestCase()
	{
	}

	void testSerialization()
	{
// 		AuthenticationServiceResponse* resp = AuthenticationServiceClient::authenticateUser("sr.jrivero@gmail.com", "1q2w3e4r");
// 		waitForResp(resp);
// 		QCOMPARE(resp->error(), restErrorSuccess);
// 
// 		mAuth = resp->authentication();
// 		QCOMPARE(mAuth.isValid(), true);
	}

private:

};

#endif
