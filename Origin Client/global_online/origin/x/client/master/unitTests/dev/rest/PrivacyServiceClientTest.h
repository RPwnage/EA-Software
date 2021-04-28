///////////////////////////////////////////////////////////////////////////////
// PrivacyServiceClientTest.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _PRIVACYSERVICECLIENTTEST_H
#define _PRIVACYSERVICECLIENTTEST_H

#include <QObject>
#include <QTest>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QSignalSpy>
#include <QNetworkReply>
#include <QDebug>
#include <QDir>

#include "services/rest/OriginServiceResponse.h"
#include "services/rest/PrivacyServiceClient.h"
#include "RequestTestHelper.h"
#include "MockNetworkAccessManager.h"
#include "OriginServiceTests.h"

using namespace RequestTestHelper;
using namespace Origin::Services;

class PrivacyServiceClientTest : public QObject
{
	Q_OBJECT
public:
	PrivacyServiceClientTest() : nam(NULL)
	{
	}
private slots:
	void initTestCase()
	{
		QFileInfo myPath(__FILE__);
		QDir myDir = myPath.dir();

		nam = new MockNetworkAccessManager(myDir.absoluteFilePath("fixtures"));
	}


	///
	/// Retrieve's friend's profiles
	///
	void testFriendsProfileVisibility()
	{
		QList<quint64> list;
		list.append(2387213056);
		list.append(2385184937);
		OriginServiceResponse *resp = PrivacyServiceClient::friendsProfileVisibility(OriginServiceTests::gSession,list);

		waitForResp(resp);

		QCOMPARE(resp->error(), restErrorSuccess);
	}
	///
	/// Tests profile privacy
	///
	void testProfilePrivacy()
	{
		OriginServiceResponse *resp = PrivacyServiceClient::profilePrivacy(OriginServiceTests::gSession);

		waitForResp(resp);

		QCOMPARE(resp->error(), restErrorSuccess);
	}

	///
	/// Test retrieving our friend's profile visibility
	/// to obtain jj_rivero's profile visibility (nucleusId: 2385184937) set nucleusId to zumba_el_gato's : 2387213056
	/// to obtain zumba_el_gato's profile visibility (nucleusId: 2387213056) set nucleusId to jj_rivero's: 2385184937
	///
	void testFriendProfileVisibility() 
	{
		// setting nucleusid to or friend so he can check out our visibility
		// (to avoid trying to use a file as a directory root)

		PrivacyFriendVisibilityResponse *resp = PrivacyServiceClient::isFriendProfileVisibility(OriginServiceTests::gSession,2385184937);
		waitForResp(resp);
		QCOMPARE(resp->error(), restErrorSuccess);
		QCOMPARE(resp->visibilityFriend(), true);
		/// Back to our nucleusid
	}

	///
	/// Rich presence setting retrieval
	///
	void testRichPresenceVisibility() 
	{
		PrivacyVisibilityResponse *resp = PrivacyServiceClient::richPresencePrivacy(OriginServiceTests::gSession);

		waitForResp(resp);

		QCOMPARE(resp->error(), restErrorSuccess);
		QCOMPARE(resp->visibilitySetting(), visibilityFriendsOfFriends);
	}

	///
	/// Test retrieving our friend's rich presence visibility
	/// to obtain jj_rivero's profile visibility (nucleusId: 2385184937) sign in as zumba_el_gato
	/// to obtain zumba_el_gato's profile visibility (nucleusId: 2387213056) sign in as jj_rivero
	///
	void testFriendRichPresenceVisibility() 
	{
		PrivacyFriendVisibilityResponse *resp = PrivacyServiceClient::friendRichPresencePrivacy(OriginServiceTests::gSession,2387213056);

		waitForResp(resp);

		QCOMPARE(resp->error(), restErrorSuccess);
		QCOMPARE(resp->visibilityFriend(), true);
	}

private:
	MockNetworkAccessManager *nam;
};

#endif
