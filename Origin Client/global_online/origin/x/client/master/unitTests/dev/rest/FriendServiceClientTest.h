///////////////////////////////////////////////////////////////////////////////
// FriendServiceClientTest.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _FRIENDSERVICECLIENTTEST_H
#define _FRIENDSERVICECLIENTTEST_H

#include <QObject>
#include <QTest>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QSignalSpy>
#include <QNetworkReply>
#include <QDebug>
#include <QDir>
#include "services/rest/OriginServiceResponse.h"
#include "services/rest/FriendServiceClient.h"
#include "RequestTestHelper.h"
#include "MockNetworkAccessManager.h"
#include "OriginServiceTests.h"


using namespace RequestTestHelper;
using namespace Origin::Services;

namespace Origin
{
	namespace Services
	{
		class FriendServiceClientTest : public QObject
		{
			Q_OBJECT
		public:
			FriendServiceClientTest() : nam(NULL)
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
					/// Tests a inviting a friend by Nucleus ID
					///
					void testInviteFriend()
					{
						OriginServiceResponse *resp = FriendServiceClient::inviteFriend(OriginServiceTests::gSession, 2387045441, "TEST");

						waitForResp(resp);

						QCOMPARE(resp->error(), restErrorSuccess);
					}

					///
					/// Tests rejecting an invitation
					///
					void testRejectInvitation()
					{
						OriginServiceResponse *resp = FriendServiceClient::rejectInvitation(OriginServiceTests::gSession, 2385184937);

						waitForResp(resp);

						QCOMPARE(resp->error(), restErrorSuccess);
					}

					///
					/// Tests confirming an invitation
					///
					void testConfirmInvitation()
					{
						OriginServiceResponse *resp = FriendServiceClient::confirmInvitation(OriginServiceTests::gSession, 2387213056);
						waitForResp(resp);
						QCOMPARE(resp->error(), restErrorSuccess);
					}

					///
					/// Tests inviting a friend that doesn't exist
					///
					void testInviteNonExistentFriend()
					{
						OriginServiceResponse *resp = FriendServiceClient::inviteFriend(OriginServiceTests::gSession, 123456, "TEST");

						QSignalSpy errorSpy(resp, SIGNAL(error(restError)));
						waitForResp(resp);

						QCOMPARE(1, errorSpy.length());
						QCOMPARE(resp->error(), restErrorAccountNotExist);
					}

					///
					/// Tests a inviting a friend by email
					///
					void testInviteFriendByEmail()
					{
						OriginServiceResponse *resp = FriendServiceClient::inviteFriendByEmail(OriginServiceTests::gSession, "jrivero@rocketmail.com");

						waitForResp(resp);

						QCOMPARE(resp->error(), restErrorSuccess);

					}

					///
					/// Tests retrieving a list of pending friends
					///
					void testRetrievePendingFriends()
					{
						PendingFriendsResponse *resp = FriendServiceClient::retrievePendingFriends(OriginServiceTests::gSession);

						waitForResp(resp);
						QCOMPARE(resp->error(), restErrorSuccess);

						QCOMPARE(resp->pendingFriends().length(), 2);

						QCOMPARE(resp->pendingFriends().at(0), 2387213056ULL);
						// 		QCOMPARE(resp->pendingFriends().at(1), 2373248337ULL);
						// 		QCOMPARE(resp->pendingFriends().at(2), 2273935058ULL);
					}

					///
					/// Tests retrieving a list of invitations
					///
					void testRetrieveInvitations()
					{
						RetrieveInvitationsResponse *resp = FriendServiceClient::retrieveInvitations(OriginServiceTests::gSession);

						waitForResp(resp);
						QCOMPARE(resp->error(), restErrorSuccess);

						QCOMPARE(resp->invitations().length(), 2);

						IncomingInvitation invite = resp->invitations().at(0);
						QCOMPARE(invite.nucleusId, 2387045441ULL);
						QCOMPARE(invite.comment, QString("TEST"));
					}

					///
					/// Tests deleting a friend
					///
					void testDeleteFriend()
					{
						OriginServiceResponse *resp = FriendServiceClient::deleteFriend(OriginServiceTests::gSession, 2387213056);

						waitForResp(resp);

						QCOMPARE(resp->error(), restErrorSuccess);
					}

					///
					/// Tests blocking a user
					///
					void testBlockUser()
					{
						OriginServiceResponse *resp = FriendServiceClient::blockUser(OriginServiceTests::gSession, 2387213056);

						waitForResp(resp);

						QCOMPARE(resp->error(), restErrorSuccess);
					}

					///
					/// Tests getting the current bloc list
					void testBlockedUsers()
					{
						BlockedUsersResponse *resp = FriendServiceClient::blockedUsers(OriginServiceTests::gSession);

						waitForResp(resp);

						QCOMPARE(resp->error(), restErrorSuccess);

						QCOMPARE(resp->blockedUsers().length(), 1);

						BlockedUser blocked = resp->blockedUsers().at(0);

						QCOMPARE(blocked.nucleusId, 2387213056ULL);
						QCOMPARE(blocked.email, QString("sr.jrivero@gmail.com"));
						QCOMPARE(blocked.personaId, 319356290ULL);
						QCOMPARE(blocked.name, QString("zumba_el_gato"));
					}

					///
					/// Tests unblocking a user
					///
					void testUnblockUser()
					{
						OriginServiceResponse *resp = FriendServiceClient::unblockUser(OriginServiceTests::gSession, 2387213056);

						waitForResp(resp);

						QCOMPARE(resp->error(), restErrorSuccess);
					}

					void cleanupTestCase()
					{
						delete nam;
					}

		private:
			MockNetworkAccessManager *nam;
		};
	}
}

#endif
