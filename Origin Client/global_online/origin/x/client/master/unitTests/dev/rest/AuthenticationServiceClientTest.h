///////////////////////////////////////////////////////////////////////////////
// AuthenicationServiceClientTest.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _AUTHENTICATIONSERVICECLIENTTEST_H
#define _AUTHENTICATIONSERVICECLIENTTEST_H

#include <QObject>
#include <QTest>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QSignalSpy>
#include <QNetworkReply>
#include <QDebug>
#include <QDir>
#include "services/rest/OriginServiceResponse.h"
#include "services/rest/AuthenticationServiceClient.h"
#include "RequestTestHelper.h"
#include "engine/login/LoginController.h"
#include "OriginServiceTests.h"

using namespace RequestTestHelper;
using namespace Origin::Services;
// Test code

// void waitForSessionInit()
// {
// 	QSignalSpy signalSpy(SessionService::instance(), SIGNAL(beginSessionComplete(Origin::Services::SessionError, Origin::Services::SessionRef)));
// 	while(signalSpy.empty())
// 	{
// 		QCoreApplication::processEvents();
// 	}
// }

class AuthenticationClientTest : public QObject
{
	Q_OBJECT;
private slots:
	void initTestCase()
	{
		SessionService::init();
		LoginRegistrationCredentials credentials(OriginServiceTests::gUserName, OriginServiceTests::gPassword);

		LoginRegistrationConfiguration config(credentials, DONT_REMEMBER_LOGIN);
		SessionService::beginSessionAsync(LoginRegistrationSession::create, config);

		waitForSessionInit();
		OriginServiceTests::gSession = SessionService::currentSession();
	}

	void testAuthenticateUser()
	{


		AuthenticationServiceResponse* resp = AuthenticationServiceClient::authenticateUser(OriginServiceTests::gUserName, OriginServiceTests::gPassword);
		waitForResp(resp);
		QCOMPARE(resp->error(), restErrorSuccess);

		mAuth = resp->authentication();
		QCOMPARE(mAuth.isValid(), true);

	}

	void testAuthenticateUserEncryptedToken()
	{
		AuthenticationServiceResponse* resp = AuthenticationServiceClient::authenticateUserEncryptedToken(mAuth.encrypted.token);
		mAuth.reset();
		waitForResp(resp);
		QCOMPARE(resp->error(), restErrorSuccess);

		mAuth = resp->authentication();
		QCOMPARE(mAuth.isValid(), true);
	}

	void testAuthenticateUserAuthToken()
	{
		AuthenticationServiceResponse* resp = AuthenticationServiceClient::authenticateUserAuthToken(mAuth.auth.token);
		mAuth.reset();
		waitForResp(resp);
		QCOMPARE(resp->error(), restErrorSuccess);

		mAuth = resp->authentication();
		QCOMPARE(mAuth.isValid(), true);
	}

	void testExtendTokens()
	{
		ExtendTokenServiceResponse* resp = AuthenticationServiceClient::extendTokens(mAuth.encrypted.token);
		mAuth.reset();
		waitForResp(resp);
		QCOMPARE(resp->error(), restErrorSuccess);

		ExtendTokens extTokens = resp->tokens();
		QCOMPARE(extTokens.isValid(), true);
	}

	void testEndSession()
	{
		SessionService::release();
	}
private:
	Authentication mAuth;

};

#endif
