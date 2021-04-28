///////////////////////////////////////////////////////////////////////////////
// LoginRegistrationServiceClientTest.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _LOGINREGISTRATIONSERVICECLIENTTEST_H
#define _LOGINREGISTRATIONSERVICECLIENTTEST_H

#include <QObject>
#include <QTest>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QSignalSpy>
#include <QNetworkReply>
#include <QDebug>
#include <QDir>
#include "services/rest/OriginServiceResponse.h"
#include "services/rest/LoginRegistrationServiceClient.h"
#include "RequestTestHelper.h"
#include "MockNetworkAccessManager.h"
#include "OriginServiceTests.h"

using namespace RequestTestHelper;
using namespace Origin::Services;

class LoginRegistrationServiceClientTest : public QObject
{
	Q_OBJECT
public:
	LoginRegistrationServiceClientTest() : nam(NULL)
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
			/// Tests user id by email
			///
			void testUserIdByEmail()
			{
				Origin::Services::PrivacyGetUserIdResponse* resp = LoginRegistrationClient::userIdByEmail(OriginServiceTests::gSession,"sr.jrivero@gmail.com");
				waitForResp(resp);
				QCOMPARE(resp->error(), restErrorSuccess);
				QCOMPARE(resp->userId(), quint64(2387213056));
			}

			///
			/// Tests user id by eaid
			///
			void testUserIdEAID()
			{
				Origin::Services::PrivacyGetUserIdResponse* resp = LoginRegistrationClient::userIdByEAID(OriginServiceTests::gSession,QString("zumba_el_gato"));
				waitForResp(resp);
				QCOMPARE(resp->error(), restErrorSuccess);
				QCOMPARE(resp->userId(), quint64(2387213056));
			}

			///
			/// Test Privacy setings
			///
			void testPrivacySettings()
			{
				Origin::Services::PrivacyGetSettingResponse* resp = LoginRegistrationClient::privacySetting(OriginServiceTests::gSession,PrivacySettingCategoryAll);
				waitForResp(resp);
				QCOMPARE(resp->error(), restErrorSuccess);
				
				QList<Origin::Services::PrivacyGetSettingResponse::UserPrivacySetting>::const_iterator iter = resp->privacySettings().begin();
				while(iter != resp->privacySettings().end())
				{
					QCOMPARE(iter->userId, quint64(2385184937));
					iter++;
				}
			}

			///
			/// Test Search options
			///
			void testSearchOptions()
			{
				Origin::Services::PrivacySearchOptionsResponse* resp = LoginRegistrationClient::searchOptions(OriginServiceTests::gSession);
				waitForResp(resp);
				QCOMPARE(resp->error(), restErrorSuccess);
			}

			/// 
			/// Test single search option
			///
			void testSingleSearchOption()
			{
				Origin::Services::PrivacySingleOptionResponse* resp = LoginRegistrationClient::singleSearchOption(OriginServiceTests::gSession,SearchOptionTypeXbox);
				waitForResp(resp);
				QCOMPARE(resp->error(), restErrorSuccess);
				QCOMPARE(resp->option(), SearchOptionTypeXbox);

			}

			/// 
			/// Test option visibility
			/// 
			void testCheckOptionVisibility()
			{
				QList<quint64> ulist;
				ulist.append(2387213056);
				ulist.append(2385184937);
				Origin::Services::PrivacyCheckOptionsResponse* resp = LoginRegistrationClient::checkOptionVisibility(OriginServiceTests::gSession,ulist, SearchOptionTypeXbox);
				waitForResp(resp);
				QCOMPARE(resp->userOption(2387213056), true);
				QCOMPARE(resp->userOption(2385184937), true);
			}

			///
			/// Update search option
			/// 
			void testUpdateSearchOption()
			{
				OriginServiceResponse * resp = LoginRegistrationClient::updateSearchOption(OriginServiceTests::gSession,SearchOptionTypeEmail, true);
				waitForResp(resp);
				QCOMPARE(resp->error(), restErrorSuccess);
			}

			///
			/// Update search option
			/// 
			void testUpdateSearchOptions()
			{
				QList<searchOptionType> postbody;
				postbody.append(SearchOptionTypeEmail);
				postbody.append(SearchOptionTypeFaceBook);
				OriginServiceResponse * resp = LoginRegistrationClient::updateSearchOptions(OriginServiceTests::gSession,postbody);
				waitForResp(resp);
				QCOMPARE(resp->error(), restErrorSuccess);
			}
private:
	MockNetworkAccessManager *nam;
};

#endif
