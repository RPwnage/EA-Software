///////////////////////////////////////////////////////////////////////////////
// AvatarServiceClientTest.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _AVATARSERVICECLIENTTEST_H
#define _AVATARSERVICECLIENTTEST_H

#include <QObject>
#include <QTest>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QSignalSpy>
#include <QNetworkReply>
#include <QDebug>
#include <QDir>
#include "services/rest/OriginServiceResponse.h"
#include "services/rest/AvatarServiceClient.h"
#include "RequestTestHelper.h"
#include "MockNetworkAccessManager.h"
#include "OriginServiceTests.h"

using namespace RequestTestHelper;
using namespace Origin::Services;

class AvatarServiceClientTest : public QObject
{
	Q_OBJECT
public:
	AvatarServiceClientTest() : nam(NULL)
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
			/// Tests all avatar types
			///
			void testAllAvatarTypes()
			{
				AvatarAllAvatarTypesResponse *resp = AvatarServiceClient::allAvatarTypes();
				waitForResp(resp);
				QCOMPARE(resp->error(), restErrorSuccess);
				QCOMPARE(resp->avatarTypeName(AvatarTypeElite), QString("elite"));
				QCOMPARE(resp->avatarTypeName(AvatarTypeNormal), QString("normal"));
				QCOMPARE(resp->avatarTypeName(AvatarTypeOfficial), QString("official"));
				QCOMPARE(resp->avatarTypeName(AvatarTypePremium), QString("premium"));
			}

			///
			/// Tests existing avatar type
			///
			void testExistingAvatarType()
			{
				AvatarTypeResponse *resp = AvatarServiceClient::existingAvatarType(1);
				waitForResp(resp);
 				QCOMPARE(resp->error(), restErrorSuccess);
 				QCOMPARE(resp->avatarName(), QString("normal"));

			}

			///
			/// Tests default avatar
			///
			void testDefaultAvatar()
			{
				AvatarDefaultAvatarResponse *resp = AvatarServiceClient::defaultAvatar(OriginServiceTests::gSession);
				waitForResp(resp);
				AvatarInformation ai = resp->avatarInfoData();
 				QCOMPARE(resp->error(), restErrorSuccess);
 				QCOMPARE(ai.avatarId, quint64(599));
 				QCOMPARE(ai.statusId, ImageStatusApproved);
 				QCOMPARE(ai.typeId, AvatarTypePremium);

			}

			///
			/// Tests default avatar
			///
			void testAvatarById()
			{
				AvatarDefaultAvatarResponse *resp = AvatarServiceClient::avatarById(OriginServiceTests::gSession,1);
				waitForResp(resp);
				AvatarInformation ai = resp->avatarInfoData();
				QCOMPARE(resp->error(), restErrorSuccess);
				QCOMPARE(ai.avatarId, quint64(1));
				QCOMPARE(ai.statusId, ImageStatusEditing);
				QCOMPARE(ai.typeId, AvatarTypeNormal);

			}

			///
			/// Tests avatar by users id: NO SIZE
			///
			void testAvatarByUsersIdNoSize()
			{
				QList<quint64> list;
				list.append(2387213056);
				list.append(2385184937);
				AvatarsByUserIdsResponse *resp = AvatarServiceClient::avatarsByUserIds(OriginServiceTests::gSession,list);
				waitForResp(resp);
				QCOMPARE(resp->error(), restErrorSuccess);
				QHash<quint64, UserAvatarsInfo> hash = resp->constAvatarInfo();
				UserAvatarsInfo uai = hash[2385184937];
				QCOMPARE(uai.userId, quint64(2385184937));
				QCOMPARE(uai.info.avatarId, quint64(3));
				QCOMPARE(uai.info.orderNumber, quint64(2));
				QCOMPARE(uai.info.link, QString("http://ea-eadm-avatar-prod.s3.amazonaws.com/prod/2/3/40x40.JPEG"));
				QCOMPARE(uai.info.isRecent, false);
 				QCOMPARE(uai.info.statusName, ImageStatus().name(ImageStatusApproved));
 				QCOMPARE(uai.info.statusId, ImageStatusApproved);
 				QCOMPARE(uai.info.typeId, AvatarTypeNormal);
 				QCOMPARE(uai.info.typeName, avatarTypes().name(AvatarTypeNormal));
 				QCOMPARE(uai.info.galleryId, quint64(2));
 				QCOMPARE(uai.info.galleryName, QString("gallery.name.battlefield"));
			}

			///
			/// Tests recent avatars
			///
			void testRecentAvatars()
			{
				AvatarGetRecentResponse*  resp = AvatarServiceClient::recentAvatarList(OriginServiceTests::gSession);
				waitForResp(resp);
				QCOMPARE(resp->error(), restErrorSuccess);
				QHash<quint64, AvatarInformation> hash = resp->constAvatarInfo();
				AvatarInformation ai = hash[quint64(600)];
				QCOMPARE(ai.typeId, AvatarTypeNormal);
				QCOMPARE(ai.statusId, ImageStatusApproved);
			}

			/// 
			/// Avatars by gallery Id
			///
			void testAvatarsByGalleryId()
			{
				AvatarsByGalleryIdResponse* resp = AvatarServiceClient::avatarsByGalleryId(OriginServiceTests::gSession,1);
				waitForResp(resp);
				QCOMPARE(resp->error(), restErrorSuccess);
				QHash<quint64, AvatarInformation> hash = resp->constAvatarInfo();
				AvatarInformation ai = hash[quint64(599)];
				QCOMPARE(ai.typeId, AvatarTypePremium);
				QCOMPARE(ai.statusId, ImageStatusApproved);
			}

			/// 
			/// Test supported dimensions
			///
			void testsupportedDimensions()
			{
				AvatarSupportedDimensionsResponse* resp = AvatarServiceClient::supportedDimensions(OriginServiceTests::gSession);
				waitForResp(resp);
				QCOMPARE(resp->error(), restErrorSuccess);
				QList<QString> list = resp->constList();
				QString size = list.at(AvatarServiceClient::Size_40X40);
				QCOMPARE(size, imageSizes().name(AvatarServiceClient::Size_40X40));
			}

			/// 
			/// Test user avatar changed
			/// 
			void testUserAvatarChanged()
			{
				AvatarBooleanResponse* resp = AvatarServiceClient::userAvatarChanged(OriginServiceTests::gSession,AvatarTypeNormal);
				waitForResp(resp);
				QCOMPARE(resp->error(), restErrorSuccess);
				QCOMPARE(resp->reponseValue(), true);

			}


private:
	MockNetworkAccessManager *nam;
};

#endif
