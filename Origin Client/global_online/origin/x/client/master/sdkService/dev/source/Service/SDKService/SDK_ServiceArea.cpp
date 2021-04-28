///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2011 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: SDK_ServiceArea.cpp
//    SDK Service Area
//    
//    Author: Hans van Veenendaal

#include "eabase/eabase.h"
#include "lsx.h"
#include "lsxEnumStrings.h"
#include "lsxreader.h"
#include "lsxwriter.h"
#include "server.h"
#include "serverreader.h"
#include "serverwriter.h"
#include "ReaderCommon.h"
#include "LSXWrapper.h"

#include "EAIO/EAFileUtil.h"

#include "Service/SDKService/hmac_sha256.h"
#include "Service/SDKService/SDK_ServiceArea.h"
#include "EbisuError.h"

#include "Service/XMPPService/XMPP_ServiceArea.h"

#include "QImage.h"
#include "QNetworkRequest.h"
#include "QNetworkReply.h"
#include "QNetworkConfigManager.h"
#include "QFile.h"
#include <QLocale.h>
#include <QDateTime.h>
#include <QTimer>

#include "ReaderCommon.h"
#include "services/settings/SettingsManager.h"
#include "services/session/SessionService.h"
#include "LSX/LSXMessages.h"
#include "LSX/LSXConnection.h"
#include "services/network/NetworkAccessManager.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/platform/TrustedClock.h"
#include "services/common/XmlUtil.h"
#include "services/trials/TrialServiceClient.h"

#include "flows/MainFlow.h"
#include "engine/igo/IGOController.h"
#include "engine/login/User.h"
#include "engine/content/Entitlement.h"
#include "engine/content/LocalContent.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentController.h"
#include "engine/content/ContentOperationQueueController.h"
#include "engine/utilities/StringUtils.h"
#include "engine/dirtybits/DirtyBitsClient.h"
#include "services/rest/FriendServiceClient.h"
#include "services/rest/IdentityUserProfileService.h"
#include "engine/login/LoginController.h"
#include "engine/social/SocialController.h"
#include "engine/social/AvatarManager.h"
#include "chat/Blocklist.h"
#include "chat/OriginConnection.h"
#include "services/debug/DebugService.h" 
#include "services/publishing/CatalogDefinition.h"

#include "openssl/sha.h"

#include "services/log/LogService.h"
#include "services/debug/DebugService.h"

#include "flows/MainFlow.h"
#include "flows/ContentFlowController.h"

#ifdef _WIN32
#include <shlobj.h>
#include <shobjidl.h>
#endif

#include "TelemetryAPIDLL.h"

#define RECENTPLAYER_DOCUMENT_VERSION 1
#define RECENTPLAYER_MAX_RECORDS 50
#define RECENTPLAYER_MAX_AGE 30 // days

void AddHMACHeaders(QNetworkRequest &serverRequest, QByteArray code, QByteArray payload)
{
    union {
        unsigned int sec;
        char c[4];
    } time;

    time.sec = (unsigned int)(Origin::Services::TrustedClock::instance()->nowUTC().toMSecsSinceEpoch() / 1000ull);

    QByteArray data = "1234";              /// < Replace this with the bigendian unix time
    data[0] = time.c[3];
    data[1] = time.c[2];
    data[2] = time.c[1];
    data[3] = time.c[0];

    data.append(payload);

    unsigned char digest[SHA256_DIGEST_LENGTH];

    hmac_sha256((unsigned char *)data.constData(), data.length(), (unsigned char *)code.constData(), code.length(), digest);

    QByteArray Digest((char *)digest, SHA256_DIGEST_LENGTH);
    QByteArray base64Digest = Digest.toBase64();

    serverRequest.setRawHeader("X-Signature", base64Digest);
    serverRequest.setRawHeader("X-Timestamp", QString::number(time.sec).toLatin1());
}

using namespace Origin::Engine;

// External functions
extern const QString& GetEnvironmentName();

namespace Origin
{
    namespace SDK
    {
		void ConvertType(OriginTimeT &time, QDateTime t)
		{
			if(t.isValid() && (uint16_t)t.date().year() < 2500)
			{
				time.wYear = (uint16_t)t.date().year();
				time.wMonth = (uint16_t)t.date().month();
				time.wDay = (uint16_t)t.date().day();
				time.wDayOfWeek = (uint16_t)t.date().dayOfWeek();
				time.wHour = (uint16_t)t.time().hour();
				time.wMinute = (uint16_t)t.time().minute();
				time.wSecond = (uint16_t)t.time().second();
				time.wMilliseconds = 0;
			}
			else
			{
				time.wYear = 0;
				time.wMonth = 0;
				time.wDay = 0;
				time.wDayOfWeek = 0;
				time.wHour = 0;
				time.wMinute = 0;
				time.wSecond = 0;
				time.wMilliseconds = 0;
			}
		}

		void ConvertType(OriginTimeT &time, uint64_t epochtime)
		{
			QDateTime t = QDateTime::fromMSecsSinceEpoch(epochtime);
			ConvertType(time, t);
		}

		// Singleton functions
        static SDK_ServiceArea* mpSingleton = NULL;

        SDK_ServiceArea* SDK_ServiceArea::create(Lsx::LSX_Handler* handler, XMPP_ServiceArea * xmpp_service)
        {
            if (mpSingleton == NULL)
            {
                mpSingleton = new SDK_ServiceArea(handler, xmpp_service);
            }
            return mpSingleton;
        }

        SDK_ServiceArea* SDK_ServiceArea::instance()
        {
            return mpSingleton;
        }

        void SDK_ServiceArea::destroy()
        {
            delete mpSingleton;
            mpSingleton = NULL;
        }

        SDK_ServiceArea::SDK_ServiceArea( Lsx::LSX_Handler* handler, XMPP_ServiceArea *xmpp_service)
            : BaseService( handler, "EbisuSDK" ),
            m_XMPP_service(xmpp_service)
        {
            registerHandler("GetConfig",  ( BaseService::RequestHandler ) &SDK_ServiceArea::GetConfig );

            registerHandler("GoOnline", ( BaseService::RequestHandler ) &SDK_ServiceArea::GoOnline );
            registerHandler("CheckPermission", ( BaseService::RequestHandler ) &SDK_ServiceArea::CheckPermission );
            registerHandler("GetProfile", ( BaseService::RequestHandler ) &SDK_ServiceArea::GetProfile );

			registerHandler("GetUTCTime", ( BaseService::RequestHandler ) &SDK_ServiceArea::GetUTCTime);

            registerHandler("ShowIGO", ( BaseService::RequestHandler ) &SDK_ServiceArea::ShowIGO);
            registerHandler("ShowIGOWindow", ( BaseService::RequestHandler ) &SDK_ServiceArea::ShowIGOWindow);
            registerHandler("QueryImage", ( BaseService::RequestHandler ) &SDK_ServiceArea::QueryImage);

            registerHandler("AddRecentPlayers", ( BaseService::RequestHandler ) &SDK_ServiceArea::AddRecentPlayers);
            registerHandler("QueryRecentPlayers", ( BaseService::RequestHandler ) &SDK_ServiceArea::QueryRecentPlayers);

            registerHandler("GetSetting", ( BaseService::RequestHandler ) &SDK_ServiceArea::GetSetting );
            registerHandler("GetSettings", ( BaseService::RequestHandler ) &SDK_ServiceArea::GetSettings );
            registerHandler("GetGameInfo", ( BaseService::RequestHandler ) &SDK_ServiceArea::GetGameInfo );
            registerHandler("GetAllGameInfo", ( BaseService::RequestHandler ) &SDK_ServiceArea::GetAllGameInfo );

            registerHandler("GetUserProfileByEmailorEAID",	(BaseService::RequestHandler)&SDK_ServiceArea::GetUserProfileByEmailorEAID );

            registerHandler("GetBlockList",			(BaseService::RequestHandler)&SDK_ServiceArea::QueryBlockList );	

            registerHandler("GrantAchievement", (BaseService::RequestHandler)&SDK_ServiceArea::GrantAchievement );
            registerHandler("PostAchievementEvents", (BaseService::RequestHandler) (void (SDK_ServiceArea::*)(Lsx::Request&, Lsx::Response&))(&SDK_ServiceArea::PostAchievementEvents));
            registerHandler("QueryAchievements", (BaseService::RequestHandler)&SDK_ServiceArea::QueryAchievements );

            registerHandler("QueryContent", ( BaseService::RequestHandler ) &SDK_ServiceArea::QueryContent );
            registerHandler("RestartGame", ( BaseService::RequestHandler ) &SDK_ServiceArea::RestartGame );
            registerHandler("StartGame", (BaseService::RequestHandler) &SDK_ServiceArea::StartGame);
            registerHandler("ExtendTrial", (BaseService::RequestHandler) &SDK_ServiceArea::ExtendTrial);
            registerHandler("SendGameMessage", (BaseService::RequestHandler) &SDK_ServiceArea::SendGameMessage);

#ifdef ORIGIN_PC
            registerHandler("BroadcastStart", ( BaseService::RequestHandler ) &SDK_ServiceArea::BroadcastStart );
            registerHandler("BroadcastStop", ( BaseService::RequestHandler ) &SDK_ServiceArea::BroadcastStop );
            registerHandler("GetBroadcastStatus", ( BaseService::RequestHandler ) &SDK_ServiceArea::GetBroadcastStatus );
#endif
            ORIGIN_VERIFY_CONNECT(Origin::Engine::LoginController::instance(), SIGNAL(userLoggedIn(Origin::Engine::UserRef)), this, SLOT(OnLoggedIn(Origin::Engine::UserRef)));
            ORIGIN_VERIFY_CONNECT(Origin::Engine::LoginController::instance(), SIGNAL(userLoggedOut(Origin::Engine::UserRef)), this, SLOT(OnLoggedOut()));
            ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(),
                SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)),
                this, SLOT(onUserOnlineStatusChanged()));

            m_AchievementServiceURL = Origin::Services::readSetting(Origin::Services::SETTING_AchievementServiceURL).toString();
        }

        SDK_ServiceArea::~SDK_ServiceArea()
        {
        }

        void SDK_ServiceArea::OnLoggedIn(Origin::Engine::UserRef user)
        {
            OnLogin(true);

            ORIGIN_VERIFY_CONNECT(user->contentControllerInstance(),
                SIGNAL(updateFinished(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)), 
                this, 
                SLOT(entitlementListRefreshed(const QList<Origin::Engine::Content::EntitlementRef>, const QList<Origin::Engine::Content::EntitlementRef>)));

            ORIGIN_VERIFY_CONNECT(Origin::Engine::Achievements::AchievementManager::instance(), 
                SIGNAL(granted(Origin::Engine::Achievements::AchievementSetRef, Origin::Engine::Achievements::AchievementRef)), 
                this, 
                SLOT(achievementGranted(Origin::Engine::Achievements::AchievementSetRef, Origin::Engine::Achievements::AchievementRef)));

            ORIGIN_VERIFY_CONNECT(user->socialControllerInstance()->smallAvatarManager(), SIGNAL(avatarChanged(quint64)),
                this, SLOT(avatarUpdateEvent(quint64)));

            ORIGIN_VERIFY_CONNECT(user->socialControllerInstance()->mediumAvatarManager(), SIGNAL(avatarChanged(quint64)),
                this, SLOT(avatarUpdateEvent(quint64)));

            ORIGIN_VERIFY_CONNECT(user->socialControllerInstance()->largeAvatarManager(), SIGNAL(avatarChanged(quint64)),
                this, SLOT(avatarUpdateEvent(quint64)));

            ORIGIN_VERIFY_CONNECT(Engine::Subscription::SubscriptionManager::instance(), SIGNAL(updated()),  this, SLOT(subscriptionUpdateEvent()));

            Chat::OriginConnection *connection = user->socialControllerInstance()->chatConnection();
            Chat::ConnectedUser *connectedUser = connection ? connection->currentUser() : NULL; 
            Chat::BlockList *blockList = connectedUser ? connectedUser->blockList() : NULL;

            if(blockList)
            {
                ORIGIN_VERIFY_CONNECT(blockList, SIGNAL(updated()), this, SLOT(OnBlockListChanged()));
            }

            Origin::Engine::DirtyBitsClient::instance()->registerHandler(this, "onOriginIdChanged", "originid");
            Origin::Engine::DirtyBitsClient::instance()->registerHandler(this, "onPasswordChanged", "password");
        }

        void SDK_ServiceArea::OnLoggedOut()
        {
            ORIGIN_VERIFY_DISCONNECT(Origin::Engine::Achievements::AchievementManager::instance(), 
                SIGNAL(granted(Origin::Engine::Achievements::AchievementSetRef, Origin::Engine::Achievements::AchievementRef)), 
                this, 
                SLOT(achievementGranted(Origin::Engine::Achievements::AchievementSetRef, Origin::Engine::Achievements::AchievementRef)));

            OnLogin(false);
        }

        void SDK_ServiceArea::OnLogin(bool loggedIn)
        {
            lsx::LoginT msg;
            msg.IsLoggedIn = loggedIn;

            sendEvent(msg);
        }

        void SDK_ServiceArea::onUserOnlineStatusChanged()
        {
            lsx::OnlineStatusEventT msg;

            UserRef user = LoginController::instance()->currentUser();
            if(user != NULL)
                msg.isOnline = Services::Connection::ConnectionStatesService::isUserOnline (user->getSession());
            else
                msg.isOnline = false;
            
            sendEvent(msg);
        }

        void SDK_ServiceArea::OnBlockListChanged()
        {
            lsx::BlockListUpdatedT msg;

            sendEvent(msg);
        }


        void SDK_ServiceArea::GetConfig(Lsx::Request& request, Lsx::Response& response )
        {
            lsx::GetConfigT r;

            if(lsx::Read(request.document(), r))
            {
                lsx::GetConfigResponseT q;

                lsx::ServiceT srv;

				srv.Facility = lsx::FACILITY_SDK;								srv.Name = "EbisuSDK";    q.Services.push_back(srv);	// <xs:enumeration id="SDK" value="0" />
				srv.Facility = lsx::FACILITY_PROFILE;							srv.Name = "EbisuSDK";    q.Services.push_back(srv);	// <xs:enumeration id="PROFILE" value="1" />
				srv.Facility = lsx::FACILITY_PRESENCE;							srv.Name = "XMPP";        q.Services.push_back(srv);	// <xs:enumeration id="PRESENCE" value="2" />
				srv.Facility = lsx::FACILITY_FRIENDS;							srv.Name = "XMPP";        q.Services.push_back(srv);	// <xs:enumeration id="FRIENDS" value="3" />
				srv.Facility = lsx::FACILITY_COMMERCE;							srv.Name = "Commerce";    q.Services.push_back(srv);	// <xs:enumeration id="COMMERCE" value="4" />
				srv.Facility = lsx::FACILITY_RECENTPLAYER;						srv.Name = "EbisuSDK";    q.Services.push_back(srv);	// <xs:enumeration id="RECENTPLAYER" value="5" />
				srv.Facility = lsx::FACILITY_IGO;								srv.Name = "EbisuSDK";    q.Services.push_back(srv);	// <xs:enumeration id="IGO" value="6" />
				srv.Facility = lsx::FACILITY_MISC;								srv.Name = "EbisuSDK";    q.Services.push_back(srv);	// <xs:enumeration id="MISC" value="7" />
				srv.Facility = lsx::FACILITY_LOGIN;								srv.Name = "EALS";        q.Services.push_back(srv);	// <xs:enumeration id="LOGIN" value="8" />
				srv.Facility = lsx::FACILITY_UTILITY;							srv.Name = "Utility";	  q.Services.push_back(srv);	// <xs:enumeration id="UTILITY" value="9" />
				srv.Facility = lsx::FACILITY_XMPP;								srv.Name = "XMPP";        q.Services.push_back(srv);	// <xs:enumeration id="XMPP" value="10" />
				srv.Facility = lsx::FACILITY_CHAT;								srv.Name = "XMPP";        q.Services.push_back(srv);	// <xs:enumeration id="CHAT" value="11" />
				srv.Facility = lsx::FACILITY_IGO_EVENT;							srv.Name = "EbisuSDK";    q.Services.push_back(srv);	// <xs:enumeration id="IGO_EVENT" value="12" />
				srv.Facility = lsx::FACILITY_EALS_EVENTS;						srv.Name = "EALS";        q.Services.push_back(srv);	// <xs:enumeration id="EALS_EVENTS" value="13" />
				srv.Facility = lsx::FACILITY_LOGIN_EVENT;						srv.Name = "EbisuSDK";    q.Services.push_back(srv);	// <xs:enumeration id="LOGIN_EVENT" value="14" />
				srv.Facility = lsx::FACILITY_INVITE_EVENT;						srv.Name = "XMPP";        q.Services.push_back(srv);	// <xs:enumeration id="INVITE_EVENT" value="15" />
				srv.Facility = lsx::FACILITY_PROFILE_EVENT;						srv.Name = "EbisuSDK";    q.Services.push_back(srv);	// <xs:enumeration id="PROFILE_EVENT" value="16" />
				srv.Facility = lsx::FACILITY_PRESENCE_EVENT;					srv.Name = "XMPP";        q.Services.push_back(srv);	// <xs:enumeration id="PRESENCE_EVENT" value="17" />
				srv.Facility = lsx::FACILITY_FRIENDS_EVENT;						srv.Name = "XMPP";        q.Services.push_back(srv);	// <xs:enumeration id="FRIENDS_EVENT" value="18" />
				srv.Facility = lsx::FACILITY_COMMERCE_EVENT;					srv.Name = "Commerce";    q.Services.push_back(srv);	// <xs:enumeration id="COMMERCE_EVENT" value="19" />
				srv.Facility = lsx::FACILITY_CHAT_EVENT;						srv.Name = "XMPP";        q.Services.push_back(srv);	// <xs:enumeration id="CHAT_EVENT" value="20" />
				srv.Facility = lsx::FACILITY_DOWNLOAD_EVENT;					srv.Name = "EbisuSDK";    q.Services.push_back(srv);	// <xs:enumeration id="DOWNLOAD_EVENT" value="21" />
				srv.Facility = lsx::FACILITY_PERMISSION;						srv.Name = "EbisuSDK";    q.Services.push_back(srv);	// <xs:enumeration id="PERMISSION" value="22" />
				srv.Facility = lsx::FACILITY_RESOURCES;							srv.Name = "EbisuSDK";    q.Services.push_back(srv);	// <xs:enumeration id="RESOURCES" value="23" />
				srv.Facility = lsx::FACILITY_BLOCKED_USERS;						srv.Name = "EbisuSDK";    q.Services.push_back(srv);	// <xs:enumeration id="BLOCKED_USERS" value="24" />
				srv.Facility = lsx::FACILITY_BLOCKED_USER_EVENT;				srv.Name = "EbisuSDK";    q.Services.push_back(srv);	// <xs:enumeration id="BLOCKED_USER_EVENT" value="25" />
				srv.Facility = lsx::FACILITY_GET_USERID;						srv.Name = "EbisuSDK";    q.Services.push_back(srv);	// <xs:enumeration id="GET_USERID" value="26" />
				srv.Facility = lsx::FACILITY_ONLINE_STATUS_EVENT;				srv.Name = "EbisuSDK";    q.Services.push_back(srv);	// <xs:enumeration id="ONLINE_STATUS_EVENT" value="27" />
				srv.Facility = lsx::FACILITY_ACHIEVEMENT;						srv.Name = "EbisuSDK";    q.Services.push_back(srv);	// <xs:enumeration id="ACHIEVEMENT" value="28" />
				srv.Facility = lsx::FACILITY_ACHIEVEMENT_EVENT;					srv.Name = "EbisuSDK";    q.Services.push_back(srv);	// <xs:enumeration id="ACHIEVEMENT_EVENT" value="29" />
				srv.Facility = lsx::FACILITY_BROADCAST_EVENT;					srv.Name = "EbisuSDK";    q.Services.push_back(srv);	// <xs:enumeration id="BROADCAST_EVENT" value="30" />
				
				// Make sure to only add services for newer SDKs, as otherwise things will break in games.
                if(request.connection()->CompareSDKVersion(9, 5, 0, 0) > 0)
				{
					srv.Facility = lsx::FACILITY_PROGRESSIVE_INSTALLATION;			srv.Name = "PI";		  q.Services.push_back(srv);	// <xs:enumeration id="PROGRESSIVE_INSTALLATION" value="31" />
					srv.Facility = lsx::FACILITY_PROGRESSIVE_INSTALLATION_EVENT;	srv.Name = "PI";		  q.Services.push_back(srv);	// <xs:enumeration id="PROGRESSIVE_INSTALLATION_EVENT" value="32" />
				}

				// add new service in an if statement with the proper version check.
				if(request.connection()->CompareSDKVersion(9,5,10,0)>0)
				{
                    srv.Facility = lsx::FACILITY_CONTENT; srv.Name = "EbisuSDK"; q.Services.push_back(srv);                         // <xs:enumeration id="CONTENT" value="33" />
				}

				if(request.connection()->CompareSDKVersion(9,6,0,0)>0)
				{

				}


                lsx::Write(response.document(), q);

                DeleteOldCacheFiles();
            }
        }

        void SDK_ServiceArea::CheckPermission(Lsx::Request& request, Lsx::Response& response )
        {
            lsx::CheckPermissionT p;

            if(lsx::Read(request.document(), p))
            {
                lsx::CheckPermissionResponseT r;

                switch(p.PermissionId)
                {
                case lsx::PERMISSIONTYPE_MULTIPLAYER:
                    r.Access = lsx::GRANTEDTYPE_GRANTED;
                    break;
                case lsx::PERMISSIONTYPE_PURCHASE:
                    r.Access = lsx::GRANTEDTYPE_GRANTED;
                    break;
                case lsx::PERMISSIONTYPE_TRIAL:
                    {
                        Content::ContentController *controller = Content::ContentController::currentUserContentController();

                        if(controller)
                        {
                            Content::EntitlementRef entitlement = request.connection()->entitlement();

                            if(entitlement && entitlement->contentConfiguration()->isFreeTrial())
                            {
                                r.Access = lsx::GRANTEDTYPE_GRANTED;
                            }
                            else
                            {
                                r.Access = lsx::GRANTEDTYPE_DENIED;
                            }
                            break;
                        }
                        else
                        {
                            r.Access = lsx::GRANTEDTYPE_DENIED;
                        }
                    }
                default:
                    r.Access = lsx::GRANTEDTYPE_DENIED;
                    break;
                }

                lsx::Write(response.document(), r);
            }
        }


        void SDK_ServiceArea::GetSetting(Lsx::Request& request, Lsx::Response& response )
        {
            lsx::GetSettingT p;

            if(lsx::Read(request.document(), p))
            {
                lsx::GetSettingResponseT r;

                switch(p.SettingId)
                {
                case lsx::SETTINGTYPE_LANGUAGE:
                    r.Setting = QLocale().name();
                    break;

                case lsx::SETTINGTYPE_ENVIRONMENT:
                    if(GetEnvironmentName().isEmpty())
                        r.Setting = "production";
                    else
                        r.Setting = GetEnvironmentName().toStdString().c_str();
                    break;

                case lsx::SETTINGTYPE_IS_IGO_AVAILABLE:
                    r.Setting = (IsIGOAvailable(request) ? "true" : "false");
                    break;
                case lsx::SETTINGTYPE_IS_IGO_ENABLED:
                    r.Setting = (IsIGOEnabled(request) ? "true" : "false");
                    break;
                case lsx::SETTINGTYPE_IS_TELEMETRY_ENABLED:
                    r.Setting = (Services::readSetting(Services::SETTING_ServerSideEnableTelemetry) ? "true" : "false");
                    break;
                default:
                    CreateErrorSuccess(response, EBISU_ERROR_INVALID_ARGUMENT, "");
                    return;
                }

                lsx::Write(response.document(), r);
            }
        }

        void SDK_ServiceArea::GetSettings(Lsx::Request& request, Lsx::Response& response )
        {
            lsx::GetSettingsResponseT r;

            r.Language = QLocale().name();
            if (GetEnvironmentName().isEmpty())
                r.Environment = "production";
            else
                r.Environment = GetEnvironmentName().toStdString().c_str();
            r.IsTelemetryEnabled = Services::readSetting(Services::SETTING_ServerSideEnableTelemetry);
            r.IsIGOAvailable = IsIGOAvailable(request);
            r.IsIGOEnabled = IsIGOEnabled(request);
            
            lsx::Write(response.document(), r);
        }

        static QString GetGameInfo_Languages(Content::ContentController* controller, Content::EntitlementRef entitlement)
        {
            // Get all the entitlements the user owns with the same master title id.
            QList<Content::EntitlementRef> languageEntitlements = controller->entitlementsByMasterTitleId(entitlement->contentConfiguration()->masterTitleId());

            // Get the supported locales from the entitlement that started the SDK game first.
            QStringList supportedLocales = entitlement->contentConfiguration()->supportedLocales();

            // Get the remaining supported languages.
            foreach(Content::EntitlementRef e, languageEntitlements)
            {
                if(e->contentConfiguration()->isBaseGame() && e->contentConfiguration()->itemSubType() == entitlement->contentConfiguration()->itemSubType())
                {
                    supportedLocales.append(e->contentConfiguration()->supportedLocales());
                }
            }

            // Do not include duplicated language.
            supportedLocales.removeDuplicates();

            // Send Grey Market telemetry for available language requests from the SDK
            QString offerId = entitlement->contentConfiguration()->productId();

            // For telemetry, collapse available language string list into single string
            // separated by comma
            QString availableLanguagesString = Origin::StringUtils::formatLocaleListAsQString(supportedLocales);

            // Grey Market telemetry - language selection
            GetTelemetryInterface()->Metric_GREYMARKET_LANGUAGE_LIST(
                offerId.toLocal8Bit().constData(),
                availableLanguagesString.toLocal8Bit().constData());

            return supportedLocales.join(',');
        }

        bool IsFullgamePurchased(Content::ContentController * controller, Content::EntitlementRef entitlement)
        {
            Content::EntitlementRefList fullgames = controller->entitlementsByMasterTitleId(entitlement->contentConfiguration()->masterTitleId());

            foreach(auto e, fullgames)
            {
                if (e->contentConfiguration()->isBaseGame() &&
                    e->contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeNormalGame &&
                    (e->contentConfiguration()->isPreorder() || e->contentConfiguration()->isPreload() || e->localContent()->installed()))
                {
                    return true;
                }
            }

            return false;
        }

        void SDK_ServiceArea::GetGameInfo(Lsx::Request& request, Lsx::Response& response )
        {
            lsx::GetGameInfoT p;

            if(lsx::Read(request.document(), p))
            {
                lsx::GetGameInfoResponseT r;
                Content::EntitlementRef entitlement = request.connection()->entitlement();
                if (entitlement == NULL)
                {
                    CreateErrorSuccess(response, EBISU_ERROR_NOT_FOUND, "Entitlement not found");
                    return;
                }
                Content::ContentController *controller = Content::ContentController::currentUserContentController();
                if (!controller)
                {
                    CreateErrorSuccess(response, EBISU_ERROR_NOT_LOGGED_IN, "There is no user");
                    return;
                }
                switch(p.GameInfoId)
                {
                case lsx::GAMEINFOTYPE_UPTODATE:
                    {
                        if(!entitlement->localContent()->updateAvailable())
                            r.GameInfo = "true";
                        else
                            r.GameInfo = "false";
                        break;
                    }
                case lsx::GAMEINFOTYPE_LANGUAGES:
                    {
                        r.GameInfo = GetGameInfo_Languages(controller, entitlement);
                        break;
                    }
                case lsx::GAMEINFOTYPE_FREETRIAL:
                    {
                        if(entitlement->contentConfiguration()->isFreeTrial())
                            r.GameInfo = "true";
                        else
                            r.GameInfo = "false";
                        break;
                    }
                case lsx::GAMEINFOTYPE_EXPIRATION:
                    {
                        if(entitlement->contentConfiguration()->isFreeTrial())
                        {
                            QDateTime now = Origin::Services::TrustedClock::instance()->nowUTC();
                            now = now.addMSecs(entitlement->localContent()->trialTimeRemaining());

                            r.GameInfo = now.toString(Qt::ISODate);
                        }
                        else if(entitlement->contentConfiguration()->terminationDate().isValid())
                        {
                            // return the ISO 8601 formatted date.
                            r.GameInfo = entitlement->contentConfiguration()->terminationDate().toString(Qt::ISODate);
                        }
                        else
                        {
                            // Return no expiration if termination date is not set.
                            r.GameInfo = "no expiration";
                        }
                        break;
                    }
                case lsx::GAMEINFOTYPE_EXPIRATION_DURATION:
                    {
                        if(entitlement->contentConfiguration()->isFreeTrial())
                        {
                            r.GameInfo = QString::number(entitlement->localContent()->trialTimeRemaining()/1000);
                        }
                        else if(entitlement->contentConfiguration()->terminationDate().isValid())
                        {
                            // return the remaining time in seconds.
                            qlonglong terminationDate = entitlement->contentConfiguration()->terminationDate().toMSecsSinceEpoch();
                            qlonglong currentDate = Origin::Services::TrustedClock::instance()->nowUTC().toMSecsSinceEpoch();

                            qlonglong difference = (terminationDate - currentDate) / 1000;

                            if(difference >= 0)
                            {
                                r.GameInfo = QString::number(difference);
                            }
                            else
                            {
                                r.GameInfo = "0";
                            }
                        }
                        else
                        {
                            // Return no expiration if termination date is not set.
                            r.GameInfo = "no expiration";
                        }
                        break;
                    }
                case lsx::GAMEINFOTYPE_INSTALLED_VERSION:
                    r.GameInfo = entitlement->localContent()->installedVersion();
                    break;

                case lsx::GAMEINFOTYPE_INSTALLED_LANGUAGE:
                    r.GameInfo = entitlement->localContent()->installedLocale();
                    break;

                case lsx::GAMEINFOTYPE_AVAILABLE_VERSION:
                    r.GameInfo = entitlement->contentConfiguration()->version();
                    break;

                case lsx::GAMEINFOTYPE_DISPLAY_NAME:
                    r.GameInfo = entitlement->contentConfiguration()->displayName();
                    break;

                case lsx::GAMEINFOTYPE_FULLGAME_PURCHASED:
                    {
                        if (entitlement->contentConfiguration()->isFreeTrial())
                        {
                            r.GameInfo = IsFullgamePurchased(controller, entitlement) ? "true" : "false";
                        }
                        else
                            r.GameInfo = "false";
                        break;
                    }

                default:
                    CreateErrorSuccess(response, EBISU_ERROR_INVALID_ARGUMENT, "");
                    return;
                }

                lsx::Write(response.document(), r);
            }
        }

        void SDK_ServiceArea::GetAllGameInfo(Lsx::Request& request, Lsx::Response& response )
        {
            lsx::GetAllGameInfoResponseT r;
            Content::EntitlementRef entitlement = request.connection()->entitlement();
            if (entitlement == NULL)
            {
                CreateErrorSuccess(response, EBISU_ERROR_NOT_FOUND, "Entitlement not found");
                return;
            }
            Content::ContentController *controller = Content::ContentController::currentUserContentController();
            if (!controller)
            {
                CreateErrorSuccess(response, EBISU_ERROR_NOT_LOGGED_IN, "There is no user");
                return;
            }
            r.UpToDate = !entitlement->localContent()->updateAvailable();
            r.FreeTrial = entitlement->contentConfiguration()->isFreeTrial();
            r.Languages = GetGameInfo_Languages(controller, entitlement);
            r.HasExpiration = entitlement->contentConfiguration()->terminationDate().isValid();
            if(r.HasExpiration)
            {
                Origin::SDK::ConvertType(r.Expiration, entitlement->contentConfiguration()->terminationDate());
            }

            r.FullGamePurchased = false;

            if (r.FreeTrial)
            {
                QDateTime now = Origin::Services::TrustedClock::instance()->nowUTC();
                now = now.addMSecs(entitlement->localContent()->trialTimeRemaining());
                Origin::SDK::ConvertType(r.Expiration, now);
            
                r.FullGamePurchased = IsFullgamePurchased(controller, entitlement);
            }

            Origin::SDK::ConvertType(r.SystemTime, Origin::Services::TrustedClock::instance()->nowUTC());

            r.InstalledVersion = entitlement->localContent()->installedVersion();
            r.InstalledLanguage = entitlement->localContent()->installedLocale();
            r.AvailableVersion = entitlement->contentConfiguration()->version();
            r.DisplayName = entitlement->contentConfiguration()->displayName();

            lsx::Write(response.document(), r);
        }

        void SDK_ServiceArea::GetProfile(Lsx::Request& request, Lsx::Response& response )
        {
            lsx::GetProfileResponseT r;

            UserRef user = LoginController::instance()->currentUser();

            if(user != NULL)
            {
                /* pack UserId */
                r.UserId = user->userId();

                /* pack Persona */
                r.Persona = user->eaid().toUtf8();

                /* pack AvatarId */
                Origin::Engine::Social::AvatarManager *avatarManager = user->socialControllerInstance()->smallAvatarManager();
                r.AvatarId = QDir::toNativeSeparators(avatarManager->pathForNucleusId(user->userId()));

                /* pack PersonaId */
                r.PersonaId = user->personaId();

                /* pack CountryId */
                r.Country = user->country().toUtf8();

                r.GeoCountry = Services::readSetting(Origin::Services::SETTING_GeoCountry).toString();
                r.CommerceCountry = Services::readSetting(Origin::Services::SETTING_CommerceCountry).toString();
                r.CommerceCurrency = Services::readSetting(Origin::Services::SETTING_CommerceCurrency).toString();

                ///* pack Under Age */
                r.IsUnderAge = user->isUnderAge();

                r.IsSubscriber = Origin::Engine::Subscription::SubscriptionManager::instance()->isActive();
            }

            /* profile response is packed now - wrap it and send it */
            lsx::Write(response.document(), r);
        }

		void SDK_ServiceArea::GetUTCTime(Lsx::Request& request, Lsx::Response& response )
		{
			lsx::GetUTCTimeResponseT r;

			Origin::SDK::ConvertType(r.Time, Origin::Services::TrustedClock::instance()->nowUTC());

			lsx::Write(response.document(), r);
		}

        void SDK_ServiceArea::ShowIGO(Lsx::Request& request, Lsx::Response& response )
        {
			lsx::ShowIGOT req;

			if(lsx::Read(request.document(), req))
			{
            //Is IGO enabled?
            if (IsIGOAvailable(request))
            {
					IGOController::instance()->EbisuShowIGO(req.bShow);

					CreateErrorSuccess(response, EBISU_SUCCESS, "");
            }
            else
            {
                if(IGOController::instantiated() && !IGOController::instance()->igowm()->isScreenLargeEnough())
                {
						CreateErrorSuccess(response, EBISU_ERROR_IGO_NOT_AVAILABLE, "IGO Game Screen not large enough");
                }
                else
                {
						CreateErrorSuccess(response, EBISU_ERROR_IGO_NOT_AVAILABLE, "IGO not connected");
                }
            }
            }
        }

        void SDK_ServiceArea::ShowIGOWindow(Lsx::Request& request, Lsx::Response& response )
        {
            IGOController *handle = NULL;

            int code = EBISU_SUCCESS;
            QString description;

            //Is IGO enabled?
            if (IsIGOAvailable(request))
            {
                handle = IGOController::instance();
            }
            else
            {
                code = EBISU_ERROR_IGO_NOT_AVAILABLE;
                if(IGOController::instantiated())
                {
                    if(!IGOController::instance()->isConnected())
                    {
                        description = "The IGO is not connected.";
                    }
                    else
                    {
                        if(!IGOController::instance()->igowm()->isScreenLargeEnough())
                        {
                            description = "The IGO doesn't fit on the screen.";
                        }
                        else
                        {
                            description = "The IGO is not available for undetermined reason.";
                        }
                    }
                }
                else
                {
                    description = "No IGO instance initialized.";
                }
            }

            if (handle)
            {
                lsx::ShowIGOWindowT r;

                if(lsx::Read(request.document(), r))
                {
                    switch(r.WindowId)
                    {
                    case lsx::IGOWINDOW_LOGIN:
                        code = EBISU_ERROR_NOT_IMPLEMENTED;
                        description = "Feature is not implemented.";
                        break;
                    case lsx::IGOWINDOW_PROFILE:
                        {
                            if (r.TargetId.size())
                            {
                                    for (unsigned int i = 0; i < r.TargetId.size(); i++)
                                {
                                    handle->EbisuShowFriendsProfileUI(r.UserId, r.TargetId[i]);
                                }
                            }
                            else
                            {
                                handle->EbisuShowProfileUI(r.UserId);
                            }
                        }
                        break;
                    case lsx::IGOWINDOW_RECENT:
                        {
                            code = EBISU_ERROR_NOT_IMPLEMENTED;
                            description = "OriginShowRecentUI is not implemented";
                        }
                        break;
                    case lsx::IGOWINDOW_FEEDBACK:
                        {
                            if(r.TargetId.size() > 0)
                            {
                                Origin::Engine::Content::EntitlementRef ent = request.connection()->entitlement();
                                if(ent)
                                {
                                    for (unsigned int i = 0; i < r.TargetId.size(); i++)
                                    {
                                        handle->EbisuShowReportUserUI(r.UserId, r.TargetId[i], ent->contentConfiguration()->masterTitle());
                                    }
                                }
                            }
                            else
                            {
                                code = EBISU_ERROR_INVALID_ARGUMENT;
                                description = "No target to report.";
                            }
                        break;
                        }
                    case lsx::IGOWINDOW_FRIENDS:
                        {
                            handle->EbisuShowFriendsUI(r.UserId);
                        }
                        break;

                    case lsx::IGOWINDOW_FRIEND_REQUEST:
                        {
                            QString source = "EbisuSDK-" + r.ContentId;

                                for (unsigned int i = 0; i < r.TargetId.size(); i++)
                            {
                                handle->EbisuRequestFriendUI(r.UserId, r.TargetId[i], source.toUtf8());
                            }
                        }
                        break;

                    case lsx::IGOWINDOW_CHAT:
                        {
                            //handle->EbisuShowMessagesUI(r.UserId);
                        }
                        break;

                    case lsx::IGOWINDOW_COMPOSE_CHAT:
                        {
                            if(r.TargetId.size() > 0)
                            {
                                handle->EbisuShowComposeChatUI(r.UserId, &r.TargetId[0], r.TargetId.size(), r.String.toUtf8());
                            }
                            else
                            {
                                code = EBISU_ERROR_INVALID_ARGUMENT;
                                description = "No target to chat.";
                            }
                        }
                        break;

                    case lsx::IGOWINDOW_INVITE:
                        {
                            handle->EbisuShowInviteUI();
                        }
                        break;

                    case lsx::IGOWINDOW_ACHIEVEMENTS:
                        {
                            if(r.TargetId.size() > 0)
                            {
                                handle->EbisuShowAchievementsUI(r.UserId, r.TargetId[0], r.String, true, IIGOCommandController::CallOrigin_SDK);
                            }
                            else
                            {
                                code = EBISU_ERROR_INVALID_ARGUMENT;
                                description = "No persona specified.";
                            }
                        }
                        break;
                    case lsx::IGOWINDOW_CODE_REDEMPTION:
                        {
                            handle->EbisuShowCodeRedemptionUI();
                        }
                        break;
                    case lsx::IGOWINDOW_STORE:
                    case lsx::IGOWINDOW_CHECKOUT:
                    case lsx::IGOWINDOW_BLOCKED:
                        code = EBISU_ERROR_NOT_IMPLEMENTED;
                        description = "Feature is not implemented.";
                        break;

                    case lsx::IGOWINDOW_BROWSER:
                        {
                            QString url = r.String;

                            if(url.isEmpty() && request.connection())
                            {
                                Origin::Engine::Content::EntitlementRef e = request.connection()->entitlement();

                                if(!e.isNull())
                                {
                                    url = e->contentConfiguration()->IGOBrowserDefaultURL();
                                }
                            }

                                if(request.connection()->CompareSDKVersion(1, 0, 0, 12) < 0)
                            {
                                handle->EbisuShowBrowserUI(IWebBrowserViewController::IGO_BROWSER_SHOW_NAV, url.toUtf8());
                            }
                            else
                            {
                                handle->EbisuShowBrowserUI((IWebBrowserViewController::BrowserFlags)r.Flags, url.toUtf8());
                            }
                        }
                        break;
                    case lsx::IGOWINDOW_FIND_FRIENDS:
                        {
                            handle->EbisuShowFindFriendsUI(r.UserId);
                        }
                        break;

                    case lsx::IGOWINDOW_CHANGE_AVATAR:
                        {
                            handle->EbisuShowSelectAvatarUI(r.UserId);
                        }
                        break;
                    case lsx::IGOWINDOW_GAMEDETAILS:
                        {
                            Origin::Engine::Content::EntitlementRef ent = request.connection()->entitlement();
                            if(ent)
                            {
                                handle->EbisuShowGameDetailsUI(ent->contentConfiguration()->productId(), ent->contentConfiguration()->masterTitleId());
                            }
                        }
                        break;
                    case lsx::IGOWINDOW_BROADCAST:
                        {
#ifdef ORIGIN_PC
                            handle->setBroadcastOriginatedFromSDK(true);
#endif
                            handle->EbisuShowBroadcastUI();
                        }
                        break;
                    }

                    if(code == EBISU_SUCCESS)
                    {
                        handle->EbisuShowIGO(r.Show);
                    }
                }
            }

            CreateErrorSuccess(response, code, description);
        }

        void SDK_ServiceArea::DeleteOldCacheFiles()
        {

            QString cacheDir = Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOAD_CACHEDIR);
            cacheDir += "/Images";
            QFileInfoList files = QDir(cacheDir).entryInfoList();

            for(int i = 0; i < files.size(); i++)
            {
                QFileInfo &fileInfo = files[i];
                QString filename = fileInfo.absoluteFilePath();

                if(fileInfo.lastModified() < QDateTime::currentDateTime().addDays(-1))
                {
                    QFile().remove(filename);
                }
            }
        }

        void SDK_ServiceArea::GenerateCachePath(const QString &imageId, unsigned int width, unsigned int height, QString &cachePath, QString &downloadUrl)
        {
            QString cacheDir = Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOAD_CACHEDIR);
            cacheDir += "Images";
            QDir().mkdir(cacheDir);

            downloadUrl = imageId;
            downloadUrl.replace(QString("$Width$"), QVariant(width).toString());
            downloadUrl.replace(QString("$Height$"), QVariant(height).toString());
            downloadUrl.replace(QString("{Width}"), QVariant(width).toString());
            downloadUrl.replace(QString("{Height}"), QVariant(height).toString());
            uint32_t hash = GetHash(downloadUrl.toLower().toLatin1());

            cachePath = cacheDir;
            cachePath.append(QDir::separator());
            cachePath.append(QString::number(hash));

            int where = downloadUrl.lastIndexOf(QChar('.'));
            if (where != -1)
            {
                cachePath.append(downloadUrl.mid(where));
            }
        }


        Origin::Services::AvatarServiceClient::ImageSizes SDK_ServiceArea::getImageSize(int width)
        {
            Origin::Services::AvatarServiceClient::ImageSizes size;

            if(width >= 256)
            {
                size = Origin::Services::AvatarServiceClient::Size_416X416;
            }
            else if(width >= 80)
            {
                size = Origin::Services::AvatarServiceClient::Size_208X208;
            }
            else
            {
                size = Origin::Services::AvatarServiceClient::Size_40X40;
            }

            return size;
        }

        void SDK_ServiceArea::avatarUpdateEvent(quint64 userId)
        {
            lsx::ProfileEventT event;

            event.Changed = lsx::PROFILESTATECHANGE_AVATAR;
            event.UserId = userId;

            sendEvent(event);
        }

        void SDK_ServiceArea::subscriptionUpdateEvent()
        {
            lsx::ProfileEventT event;

            event.Changed = lsx::PROFILESTATECHANGE_SUBSCRIPTION;

            Origin::Services::Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();
            QString userId = Origin::Services::Session::SessionService::nucleusUser(session);
            event.UserId = userId.toULongLong();

            sendEvent(event);
        }

        void SDK_ServiceArea::onOriginIdChanged (QByteArray payload)
        {
            // payload structure
            // {"ver":1,"uid":1000001520859,"ts":1371217132577,"ctx":"originid","verb":"upd","data":{"pid":1000001520859,"old":"jwadebuytest12b","new":"jwade_buy_test12"}}

            QScopedPointer<INodeDocument> doc(CreateJsonDocument());

            doc->Parse(payload);

            server::dirtybitChangeNotificationT packet;

            if(server::Read(doc.data(), packet))
            {
                lsx::ProfileEventT event;

                event.Changed = lsx::PROFILESTATECHANGE_EAID;
                event.UserId = packet.uid;
                
                sendEvent(event);
                }
            }

        void SDK_ServiceArea::onPasswordChanged(QByteArray payload)
        {
            // payload structure
            // {"ver":1,"uid":1000001520859,"ts":1371217132577,"ctx":"password","verb":"upd","data":{"old":"","new":""}}

            QScopedPointer<INodeDocument> doc(CreateJsonDocument());

            doc->Parse(payload);

            server::dirtybitChangeNotificationT packet;

            if(server::Read(doc.data(), packet))
            {
                // Changes in passwords will log the user out.
                OnLogin(false);
            }
        }

        void SDK_ServiceArea::QueryImageForUser(const lsx::QueryImageT &image, Lsx::Response& response )
        {
            quint64 userId = image.ImageId.mid(5).toLongLong();
            UserRef user = LoginController::instance()->currentUser();

            if ((userId != 0) && user)
            {
                ImageDetails imageRequest;

                imageRequest.image = image;
                imageRequest.userId = userId;

                if (image.Width > 256)
                {
                    imageRequest.avatarManager = user->socialControllerInstance()->largeAvatarManager();
                    imageRequest.image.Width = 416;
                    imageRequest.image.Height = 416;
                    imageRequest.imageSize = Origin::Services::AvatarServiceClient::Size_416X416;
                }
                else if (image.Width > 80)
                {
                    imageRequest.avatarManager = user->socialControllerInstance()->mediumAvatarManager();
                    imageRequest.image.Width = 208;
                    imageRequest.image.Height = 208;
                    imageRequest.imageSize = Origin::Services::AvatarServiceClient::Size_208X208;
                }
                else
                {
                    imageRequest.avatarManager = user->socialControllerInstance()->smallAvatarManager();
                    imageRequest.image.Width = 40;
                    imageRequest.image.Height = 40;
                    imageRequest.imageSize = Origin::Services::AvatarServiceClient::Size_40X40;
                }

                QString avatarPath = imageRequest.avatarManager->pathForNucleusId(userId);

                if (QFile::exists(avatarPath))
                {
                    lsx::QueryImageResponseT res;

                    res.Result = EBISU_SUCCESS;

                    // Already a cached version exists.
                    lsx::ImageT i;

                    i.ImageId = imageRequest.image.ImageId;
                    i.Width = imageRequest.image.Width;
                    i.Height = imageRequest.image.Height;
                    i.ResourcePath = QDir::toNativeSeparators(avatarPath);

                    res.Images.push_back(i);

                    lsx::Write(response.document(), res);
                }
                else
                {
                    // Request image
                    RequestUserImageUrl(imageRequest, response);
                }
            }
            else
            {
                CreateErrorSuccess(response, EBISU_ERROR_INVALID_ARGUMENT, "Not a valid userId");
            }
        }

        void SDK_ServiceArea::RequestUserImageUrl(ImageDetails & image, Lsx::Response& response)
        {
            using namespace Origin::Services;

            QList<quint64> ids;
            ids.push_back(image.userId);

            auto *resp = AvatarServiceClient::avatarsByUserIds(Session::SessionService::currentSession(), 
                                                               ids, 
                                                               image.imageSize);

            auto * handler = new ResponseWrapperEx<ImageDetails, SDK_ServiceArea, AvatarsByUserIdsResponse>(image, this, &SDK_ServiceArea::RequestUserImageUrlResponse, resp, response);

            connect(resp, SIGNAL(finished()), handler, SLOT(finished()));
        }

        bool SDK_ServiceArea::RequestUserImageUrlResponse(ImageDetails & image, Services::AvatarsByUserIdsResponse * resp, Lsx::Response &response)
        {
            int responseCode = resp->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            
            if(responseCode == 200)
            {
                QString link = resp->constAvatarInfo().value(image.userId).info.link;

                QNetworkRequest req(link);

                req.setRawHeader("Accept", "image/jpeg, image/png");
                req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);

                DoServerCall(image, req, OPERATION_GET, &SDK_ServiceArea::DownloadImageResponse, response);
            }
            else
            {
                CreateErrorSuccess(response, EBISU_ERROR_INVALID_USER, "The user couldn't be found!");
            }
            return true;
        }

        bool SDK_ServiceArea::DownloadImageResponse(ImageDetails & image, QNetworkReply * reply, Lsx::Response &response)
        {
            using namespace Origin::Services;

            int responseCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            if(responseCode == Http200Ok)
            {
                QByteArray mediaType = reply->header(QNetworkRequest::ContentTypeHeader).toByteArray();
                QByteArray data = reply->readAll();

                QString cacheFolder = avatarImageDirectory();

                QCryptographicHash hash(QCryptographicHash::Md5);
                hash.addData(reply->request().url().toEncoded());

                QString filename = hash.result().toHex();
                QString ext = mediaType == "image/jpeg" ? ".jpg" : ".png";
                QString path = cacheFolder + "/" + filename + ext;
                QFile file(path);

                if(file.open(QIODevice::WriteOnly))
                {
                    file.write(data);
                    file.close();
                }

                lsx::QueryImageResponseT r;

                r.Result = EBISU_SUCCESS;
                lsx::ImageT i;

                i.Width = image.image.Width;
                i.Height = image.image.Height;
                i.ImageId = image.image.ImageId;
                i.ResourcePath = QDir::toNativeSeparators(path);
                r.Images.push_back(i);

                image.avatarManager->loadAvatar(image.userId, reply->request().url(), mediaType, path);

                lsx::Write(response.document(), r);
            }
            else
            {
                CreateErrorSuccess(response, EBISU_ERROR_NOT_FOUND, "The image was not found!");
            }

            return true;
        }

        QString SDK_ServiceArea::cacheFolder() const
        {
            QString cacheDir = Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOAD_CACHEDIR);
            cacheDir += "/Icons";
            QDir().mkdir(cacheDir);

            return cacheDir;
        }

        void SDK_ServiceArea::QueryImageFromAchievement(const lsx::QueryImageT &image, Lsx::Response& response )
        {
            using namespace Origin::Engine::Achievements;

            QString id = image.ImageId.mid(QString("achieve:").length());
            QStringList ids = id.split('-');
            QString url;

            if(ids.size() != 2)
            {
                CreateErrorSuccess(response, EBISU_ERROR_NOT_FOUND, "The achievement image couldn't be found.");
                return;
            }

            int requestedSize = 40; 

            switch(getImageSize(image.Width))
            {
            case Origin::Services::AvatarServiceClient::Size_416X416:
                requestedSize = 416;
                break;
            case Origin::Services::AvatarServiceClient::Size_208X208:
                requestedSize = 208;
                break;
            case Origin::Services::AvatarServiceClient::Size_40X40:
                requestedSize = 40;
                break;
            }

            AchievementManager * mng = AchievementManager::instance();

            if(mng != NULL)
            {
                AchievementSetRef set = mng->currentUsersPortfolio()->getAchievementSet(ids[0]);

                if(set != NULL)
                {
                    QString filename = set->achievementSetId() + "-" + ids[1] + "-" + QString::number(image.Width) + "x" + QString::number(image.Height) + ".png";
 
                    QString cachePath = cacheFolder();
 
                    lsx::QueryImageResponseT res;

                    res.Result = EBISU_SUCCESS;

                    if(set != NULL)
                    {
                        AchievementRef achievement = set->getAchievement(ids[1]);
                        if(achievement!=NULL)
                            url = achievement->getImageUrl(requestedSize);
                    }

                    if(!url.isEmpty())
                    {
                        QNetworkRequest serverRequest(url);

                        lsx::ImageT image;

                        image.ImageId = "achieve:" + ids[0] + "-" + ids[1];
                        image.Width = requestedSize;
                        image.Height = requestedSize;
                        image.ResourcePath = cachePath + "/" + filename;

                        DoServerCall(image, serverRequest, OPERATION_GET, &SDK_ServiceArea::DownloadFromUrlProcessResponse, response);
                    }
                    else
                    {
                        lsx::Write(response.document(), res);
                    }
                    return;
                }
            }
            CreateErrorSuccess(response, EBISU_ERROR_NOT_FOUND, "Couldn't find the proper assets.");
        }

        bool SDK_ServiceArea::DownloadFromUrlProcessResponse(lsx::ImageT &image, QNetworkReply *serverReply, Lsx::Response& response)
        {
            lsx::QueryImageResponseT qir;

            int responseCode = serverReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            if(responseCode == 200)
            {
                qir.Result = EBISU_SUCCESS;
                qir.Images.push_back(image);

                QFile file(image.ResourcePath);
                if(file.open(QIODevice::WriteOnly))
                {
                    file.write(serverReply->readAll());
                    file.close();
                }
            }
            else
            {
                qir.Result = EBISU_ERROR_PROXY + responseCode;
            }

            lsx::Write(response.document(), qir);

            return true;
        }

        void SDK_ServiceArea::QueryImage(Lsx::Request& request, Lsx::Response& response )
        {
            lsx::QueryImageT r;

            bool bSuccess = false;
            bool bError = false;

            if(lsx::Read(request.document(), r))
            {
                lsx::QueryImageResponseT res;

                if(r.ImageId.startsWith("user:"))
                {
                    QueryImageForUser(r, response);

                    return;
                }
                if(r.ImageId.startsWith("achieve:"))
                {
                    QueryImageFromAchievement(r, response);

                    return;
                }
                else
                {
                    QString cachePath;
                    QString downloadUrl;
                    QString resourceHash;

                    GenerateCachePath(r.ImageId, r.Width, r.Height, cachePath, downloadUrl);

                    if(!QFile(cachePath).exists())
                    {
                        if(downloadUrl.startsWith("http"))
                        {
                            lsx::ImageT i;

                            i.Width = r.Width;
                            i.Height = r.Height;
                            i.ResourcePath = cachePath.toUtf8();
                            i.ImageId = r.ImageId;

                            QNetworkRequest serverRequest(downloadUrl);

                            DoServerCall(i, serverRequest, OPERATION_GET, &SDK_ServiceArea::DownloadFromUrlProcessResponse, response);

                            return;
                        }
                        else
                        {
                            // Is it a local file?
                            if(QFile(downloadUrl).exists())
                            {
                                lsx::ImageT i;

                                i.ImageId = r.ImageId;
                                i.Width = r.Width;
                                i.Height = r.Height;
                                i.ResourcePath = downloadUrl.toUtf8();

                                lsx::QueryImageResponseT res;
                                res.Images.push_back(i);
                                lsx::Write(response.document(), res);

                                bSuccess = true;
                            }
                        }
                    }
                    else
                    {
                        // Already a cached version exists.
                        lsx::ImageT i;

                        i.ImageId = r.ImageId;
                        i.Width = r.Width;
                        i.Height = r.Height;
                        i.ResourcePath = cachePath.toUtf8();

                        lsx::QueryImageResponseT res;
                        res.Images.push_back(i);
                        lsx::Write(response.document(), res);

                        bSuccess = true;
                    }

                    if(!bSuccess && !bError)
                    {
                        lsx::ImageT i;
                        int w = r.Width, h = r.Height;

                        i.Width = w;
                        i.Height = h;

                        QImage image(w, h, QImage::Format_ARGB32);

                        // Fill the image with magenta
                        image.fill(0xFFFF00FF);

                        image.save(cachePath, "PNG");

                        i.ImageId = r.ImageId;
                        i.ResourcePath = cachePath.toUtf8();

                        lsx::QueryImageResponseT res;
                        res.Images.push_back(i);
                        lsx::Write(response.document(), res);
                    }
                }
            }
            else
            {
				CreateErrorSuccess(response, EBISU_ERROR_LSX_INVALID_REQUEST, "");
            }
        }


        int GetPlayer( std::vector<lsx::playerT> &players, uint64_t uid )
        {
            for(unsigned int i=0; i<players.size(); i++)
            {
                if(players[i].uid == uid)
                    return i;
            }
            return -1;
            }


        void SDK_ServiceArea::AddRecentPlayers(Lsx::Request& request, Lsx::Response& response )
        {
            lsx::AddRecentPlayersT r;

            if(lsx::Read(request.document(), r))
            {
                Content::ContentController *controller = Content::ContentController::currentUserContentController();

                if(controller)
                {
                    Content::EntitlementRef entitlement = controller->entitlementByContentId(request.connection()->productId());

                    if (entitlement)
                    {
                        lsx::rpT recentPlayerInfo;

                        if(!ReadRecentPlayers(recentPlayerInfo))
                        {
                            recentPlayerInfo.version = RECENTPLAYER_DOCUMENT_VERSION;
                        }

                        // Remove old recent players 
                        lsx::gameT * currentGame = NULL;

                        QString masterTitleId = entitlement->contentConfiguration()->masterTitleId();
                        for(auto gameIterator = recentPlayerInfo.games.begin(); gameIterator != recentPlayerInfo.games.end(); ++gameIterator)
                        {
                            if(gameIterator->masterTitle == masterTitleId)
                            {
                                // Found our node.
                                currentGame = &(*gameIterator);
                                break;
                            }
                        }

                        if(currentGame == NULL)
                        {
                            lsx::gameT newGame;

                            // Configure the game node.
                            QString displayName = entitlement->contentConfiguration()->displayName();

                            newGame.name = displayName;
                            newGame.masterTitle = masterTitleId;

                            recentPlayerInfo.games.push_back(newGame);

                            currentGame = &recentPlayerInfo.games.back();
                        }

                        for(auto recent = r.Players.begin(); recent != r.Players.end(); ++recent)
                        {
                            //int index = GetPlayer(currentGame->players, recent->UserId);

                            //if(index>=0)
                            //{
                            //    // Update the existing player in the list.
                            //    lsx::playerT &player = currentGame->players[index];
                            //    player.eaid = recent->Persona;
                            //    player.ts = Origin::Services::TrustedClock::instance()->nowUTC().currentMSecsSinceEpoch();
                            //}
                            //else
                            //{
                            //    // Add a new player to the list.
                            //    lsx::playerT newPlayer;

                            //    newPlayer.uid = recent->UserId;
                            //    newPlayer.pid = recent->PersonaId;
                            //    newPlayer.eaid = recent->Persona;
                            //    newPlayer.ts = Origin::Services::TrustedClock::instance()->nowUTC().currentMSecsSinceEpoch();

                            //    currentGame->players.push_back(newPlayer);
                            //}
            }

                        // Add the recent player record here.

                        if(WriteRecentPlayers(recentPlayerInfo))
                        {
                            CreateErrorSuccess(response, EBISU_SUCCESS, "");
                            return;
                        }
                        else
                        {
                            CreateErrorSuccess(response, EBISU_ERROR_PROXY_INTERNAL_ERROR, "Couldn't send recent player information.");
                            return;
                        }
                    }
                    else
                    {
                        CreateErrorSuccess(response, EBISU_ERROR_NOT_FOUND, "Couldn't find an entitlement for the game.");
                    }
                }
                else
                {
                    CreateErrorSuccess(response, EBISU_ERROR_NOT_LOGGED_IN, "User is not logged in.");
                }
            }
            CreateErrorSuccess(response, EBISU_ERROR_LSX_INVALID_REQUEST, "Couldn't decode the request.");
            return;
        }

        void SDK_ServiceArea::QueryRecentPlayers(Lsx::Request& request, Lsx::Response& response )
        {
            lsx::rpT recentPlayerInfo;

            if(ReadRecentPlayers(recentPlayerInfo))
        {

        }
            else
            {

            }
        }

        void SDK_ServiceArea::IgoStateChanged(bool bVisible, bool sdkCall)
        {
            lsx::IGOEventT r;

            r.State = bVisible ? lsx::IGOSTATE_UP : lsx::IGOSTATE_DOWN;

            sendEvent(r);
        }

		void SDK_ServiceArea::IgoUserOpenAttemptIgnored()
		{
			const int ResolutionTooLow = 1;

			lsx::IGOUnavailableT r;

			r.Reason = ResolutionTooLow; 

			sendEvent(r);
		}

        void SDK_ServiceArea::GetUserProfileByEmailorEAID(Lsx::Request &request, Lsx::Response &response)
        {
            lsx::GetUserProfileByEmailorEAIDT req;

            if(lsx::Read(request.document(), req))
            {
                if(req.KeyWord.length() > 0)
                {
                    Origin::Services::IdentityUserProfileService* resp = Origin::Services::IdentityUserProfileService::userProfileByEmailorEAID(Origin::Services::Session::SessionService::currentSession(), req.KeyWord);
                    Q_ASSERT(resp);
                    QEventLoop loop;
                    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(userProfileFinished()), &loop, SLOT(quit()));
                    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(userProfileError(Origin::Services::restError)), &loop, SLOT(quit()));
                    loop.exec();

                    if(resp->getRestError() == Origin::Services::restErrorSuccess)
                    {
                        lsx::GetUserProfileByEmailorEAIDResponseT res;

                        res.Return = "Success";

                        lsx::UserT user;
                        Read(resp->token("USERPID").toLocal8Bit(), user.UserId);
                        Read(resp->token("PERSONAID").toLocal8Bit(), user.PersonaId);
                        user.EAID = resp->token("EAID");

                        res.User.push_back(user);

                        lsx::Write(response.document(), res);
                    }
                    else
                    {
                        CreateErrorSuccess(response, EBISU_ERROR_NOT_FOUND, "Couldn't successfully find a match.");
                    }
                }
                else
                {
                    CreateErrorSuccess(response, EBISU_ERROR_INVALID_ARGUMENT, "No proper parameter provided.");
                }
            }
        }

        void SDK_ServiceArea::CreateErrorSuccess( Lsx::Response &response, QNetworkReply * resp )
        {
			lsx::ErrorSuccessT err;

			err.Code = (int)(EBISU_ERROR_PROXY + resp->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
			err.Description = resp->errorString();

            lsx::Write(response.document(), err);
        }

        void SDK_ServiceArea::CreateErrorSuccess( Lsx::Response &response, int code, QString description)
        {
			lsx::ErrorSuccessT err;

			err.Code = code;
			err.Description = description;

			lsx::Write(response.document(), err);
        }

        void SDK_ServiceArea::QueryBlockList(Lsx::Request &request, Lsx::Response &response)
        {
            Origin::Services::BlockedUsersResponse* resp = Origin::Services::FriendServiceClient::blockedUsers(Origin::Services::Session::SessionService::currentSession());
            Q_ASSERT(resp);

            QEventLoop loop;
            connect(resp, SIGNAL(finished()), &loop, SLOT(quit()));
            loop.exec();

            if(resp->error() == Origin::Services::restErrorSuccess)
            {
                lsx::GetBlockListResponseT r;

                r.Return = "Success";

                foreach(const BlockedUser &blockedUser, resp->blockedUsers())
                {
                    lsx::UserT u;
                    u.EAID = blockedUser.name;
                    u.UserId = blockedUser.nucleusId;
                    u.PersonaId = blockedUser.personaId;

                    r.User.push_back(u);
                }

                lsx::Write(response.document(), r);
            }
            else
            {
                CreateServerErrorResponse(resp->reply(), response);
            }
        }

        void SDK_ServiceArea::entitlementListRefreshed(const QList<Origin::Engine::Content::EntitlementRef> entitlementsAdded, const QList<Origin::Engine::Content::EntitlementRef> entitlementsRemoved)
        {
            for(QList<Origin::Engine::Content::EntitlementRef>::const_iterator it = entitlementsAdded.constBegin();
                it != entitlementsAdded.constEnd();
                ++it)
            {
                bool b = connect((*it)->localContent(), SIGNAL(stateChanged(Origin::Engine::Content::EntitlementRef)), this, SLOT(entitlementUpdated(Origin::Engine::Content::EntitlementRef))); Q_ASSERT(b);
                ORIGIN_UNUSED(b);
            }

            for(QList<Origin::Engine::Content::EntitlementRef>::const_iterator it = entitlementsRemoved.constBegin();
                it != entitlementsRemoved.constEnd();
                ++it)
            {
                bool b = disconnect((*it)->localContent(), SIGNAL(stateChanged(Origin::Engine::Content::EntitlementRef)), this, SLOT(entitlementUpdated(Origin::Engine::Content::EntitlementRef))); Q_ASSERT(b);
                ORIGIN_UNUSED(b);
            }
        }


        lsx::GameT SDK_ServiceArea::extractContentInformation(Origin::Engine::Content::EntitlementRef entitlement)
        {
            lsx::GameT contentInfo;

            contentInfo.contentID = entitlement->contentConfiguration()->productId();
            contentInfo.progressValue = 0;
            contentInfo.installedVersion = entitlement->localContent()->installedVersion();
            contentInfo.availableVersion = entitlement->contentConfiguration()->version();
            contentInfo.displayName = entitlement->contentConfiguration()->displayName();

            if(entitlement->contentConfiguration()->isPULC())
            {
                contentInfo.state = lsx::CONTENTSTATE_INSTALLED;
            }
            else
            {
                Origin::Engine::Content::LocalContent::States state = entitlement->localContent()->state();

                if(state.testFlag(Origin::Engine::Content::LocalContent::ReadyToDownload))
                {
                    contentInfo.state = lsx::CONTENTSTATE_READY_TO_DOWNLOAD;
                }
                else if(state.testFlag(Origin::Engine::Content::LocalContent::Downloading))
                {
                    if(entitlement->localContent()->installFlow())
                    {
                        contentInfo.progressValue = entitlement->localContent()->installFlow()->progressDetail().mProgress;
                    }

                    if(state.testFlag(Origin::Engine::Content::LocalContent::Paused))
                    {
                        contentInfo.state = lsx::CONTENTSTATE_DOWNLOAD_PAUSED;
                    }
                    else
                    {
                        contentInfo.state = lsx::CONTENTSTATE_DOWNLOADING;
                    }
                }
                else if(state.testFlag(Origin::Engine::Content::LocalContent::ReadyToInstall))
                {
                    contentInfo.state = lsx::CONTENTSTATE_READY_TO_INSTALL;
                }
                else if(state.testFlag(Origin::Engine::Content::LocalContent::Installing))
                {
                    contentInfo.state = lsx::CONTENTSTATE_INSTALLING;
                }
                else if(state.testFlag(Origin::Engine::Content::LocalContent::Installed))
                {
                    contentInfo.state = lsx::CONTENTSTATE_INSTALLED;
                }
                else if(state.testFlag(Origin::Engine::Content::LocalContent::ReadyToPlay))
                {
                    contentInfo.state = lsx::CONTENTSTATE_READY_TO_PLAY;
                }
                else if(state.testFlag(Origin::Engine::Content::LocalContent::Playing))
                {
                    contentInfo.state = lsx::CONTENTSTATE_PLAYING;
                }
                else if(state.testFlag(Origin::Engine::Content::LocalContent::Busy))
                {
                    // This is not the proper state, but will translate to busy at the OriginSDK side.
                    contentInfo.state = lsx::CONTENTSTATE_VERIFYING;
                }
                else if(state.testFlag(Origin::Engine::Content::LocalContent::FinalizingDownload))
                {
                    contentInfo.state = lsx::CONTENTSTATE_FINALIZING_DOWNLOAD;
                }
                else if(state.testFlag(Origin::Engine::Content::LocalContent::Preparing))
                {
                    contentInfo.state = lsx::CONTENTSTATE_PREPARING_DOWNLOAD;
                }
                else if(state.testFlag(Origin::Engine::Content::LocalContent::ReadyToUnpack))
                {
                    contentInfo.state = lsx::CONTENTSTATE_READY_TO_UNPACK;
                }
                else if(state.testFlag(Origin::Engine::Content::LocalContent::ReadyToDecrypt))
                {
                    contentInfo.state = lsx::CONTENTSTATE_READY_TO_DECRYPT;
                }
                else if(state.testFlag(Origin::Engine::Content::LocalContent::Decrypting))
                {
                    contentInfo.state = lsx::CONTENTSTATE_DECRYPTING;
                }
                else
                {
                    if(entitlement->localContent()->installFlow())
                    {
                        contentInfo.progressValue = entitlement->localContent()->installFlow()->progressDetail().mProgress;
                    }

                    contentInfo.state = lsx::CONTENTSTATE_DOWNLOAD_PAUSED;
                }


                Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();

                if(queueController)
                {
                    Engine::Content::EntitlementRefList queuedEntitlements = queueController->entitlementsQueued();

                    if(queuedEntitlements.contains(entitlement))
                    {
                        if (entitlement->localContent()->installFlow())
                        {
                            Origin::Downloader::ContentInstallFlowState state = entitlement->localContent()->installFlow()->getFlowState();

                            if (state == Origin::Downloader::ContentInstallFlowState::kPaused || state == Origin::Downloader::ContentInstallFlowState::kEnqueued)
                            {
                                contentInfo.state = lsx::CONTENTSTATE_DOWNLOAD_QUEUED;
                            }
                        }
                    }
                }
            }

            return contentInfo;
        }



        void SDK_ServiceArea::entitlementUpdated(Origin::Engine::Content::EntitlementRef entitlement)
        {
            if(!entitlement.isNull() && (entitlement->contentConfiguration()->isPDLC() || entitlement->contentConfiguration()->isPULC() || entitlement->contentConfiguration()->isBaseGame()))
            {
            lsx::CoreContentUpdatedT ccu;
                ccu.Games.push_back(extractContentInformation(entitlement));
                sendEvent(ccu);
            }
        }

        /// \ingroup enums
        /// \brief QueryContent options
        enum enumQueryContentOptions
        {
            ORIGIN_QUERY_CONTENT_ALL = 0xFFFFFFFF,	    ///< All types of content will be queried.
            ORIGIN_QUERY_CONTENT_BASE_GAME = (1 << 0),         ///< Include Base Games in the query results.
            ORIGIN_QUERY_CONTENT_DLC = (1 << 1),	        ///< Include DLC in the query results
            ORIGIN_QUERY_CONTENT_ULC = (1 << 2),	        ///< Include ULC in the query results
            ORIGIN_QUERY_CONTENT_OTHER = 0xFFFFFFF8,	    ///< Include Other in the query results - In future versions this may contain less items if specific flags are introduced.
        };

        void SDK_ServiceArea::QueryContent(Lsx::Request& request, Lsx::Response& response )
        {
            using namespace Origin::Engine::Content;
            using namespace Origin::Services::Session;

            lsx::QueryContentT req;

            if(lsx::Read(request.document(), req))
            {
                SessionRef session = SessionService::currentSession();

                if(QString::number(req.UserId) != SessionService::nucleusUser(session))
                {
                    CreateErrorResponse(EBISU_ERROR_INVALID_USER, "The userId doesn't match the current users.", response);
                    return;
                }

                // If multiplayerId is specified only search basegame, else if ContentType is specified use the ContentType restriction if not search all.
                int contentType = (req.MultiplayerId.isEmpty() ? (req.ContentType == 0 ? ORIGIN_QUERY_CONTENT_ALL : req.ContentType) : ORIGIN_QUERY_CONTENT_BASE_GAME);

                lsx::QueryContentResponseT r;

                ContentController * controller = ContentController::currentUserContentController();

                EntitlementRefList entitlements = controller->entitlements();

                foreach(EntitlementRef entitlement, entitlements)
                {
                    bool isBaseGame = entitlement->contentConfiguration()->isBaseGame();
                    bool isPDLC = entitlement->contentConfiguration()->isPDLC();
                    bool isPULC = entitlement->contentConfiguration()->isPULC();

                    if((isPDLC && (contentType & ORIGIN_QUERY_CONTENT_DLC)) ||
                       (isPULC && (contentType & ORIGIN_QUERY_CONTENT_ULC)) ||
                       (isBaseGame && (contentType & ORIGIN_QUERY_CONTENT_BASE_GAME)) ||
                       ((contentType & ORIGIN_QUERY_CONTENT_OTHER) && (!isBaseGame && !isPDLC && !isPULC)))
                    {
                        if(req.GameId.size()>0)
                        {
                            if(std::find(req.GameId.begin(), req.GameId.end(), entitlement->contentConfiguration()->masterTitleId()) != req.GameId.end())
                            {
                                if(req.MultiplayerId.isEmpty() || req.MultiplayerId == entitlement->contentConfiguration()->multiplayerId())
                                {
                                    r.Content.push_back(extractContentInformation(entitlement));
                                }
                                continue;
                            }

                                // This is in case the game specifies the masterTitleId prefix.
                            if(std::find(req.GameId.begin(), req.GameId.end(), QString("masterTitleId:") + entitlement->contentConfiguration()->masterTitleId()) != req.GameId.end())
                            {
                                if(req.MultiplayerId.isEmpty() || req.MultiplayerId == entitlement->contentConfiguration()->multiplayerId())
                                {
                                    r.Content.push_back(extractContentInformation(entitlement));
                                }
                                continue;
                            }
                            if(std::find(req.GameId.begin(), req.GameId.end(), QString("contentId:") + entitlement->contentConfiguration()->contentId()) != req.GameId.end())
                            {
                                r.Content.push_back(extractContentInformation(entitlement));
                                continue;
                            }
                            if(std::find(req.GameId.begin(), req.GameId.end(), QString("offerId:") + entitlement->contentConfiguration()->productId()) != req.GameId.end())
                            {
                                r.Content.push_back(extractContentInformation(entitlement));
                                continue;
                            }
                        }
                        else
                        {
                            if(req.MultiplayerId.isEmpty() || req.MultiplayerId == entitlement->contentConfiguration()->multiplayerId())
                            {
                                r.Content.push_back(extractContentInformation(entitlement));
                                continue;
                            }
                        }
                    }
                }

                lsx::Write(response.document(), r);
            }
            else
            {
                CreateErrorResponse(EBISU_ERROR_LSX_INVALID_REQUEST, "QueryContent invalid request.", response);
            }
        }

        void SDK_ServiceArea::RestartGame(Lsx::Request& request, Lsx::Response& response )
        {
            using namespace Origin::Services::Session;
            using namespace Origin::Engine::Content;

            lsx::RestartGameT req;

            if(lsx::Read(request.document(), req))
            {
                SessionRef session = SessionService::currentSession();

                if(QString::number(req.UserId) != SessionService::nucleusUser(session))
                {
                    CreateErrorResponse(EBISU_ERROR_INVALID_USER, "The userId doesn't match the current users.", response);
                    return;
                }

                lsx::ErrorSuccessT r;

                // Create a restart request here.

                QMetaObject::invokeMethod(Origin::Client::MainFlow::instance()->contentFlowController(), "scheduleRelaunch", 
                     Q_ARG(Origin::Engine::Content::EntitlementRef, request.connection()->entitlement()),
                     Q_ARG(bool, req.Options == lsx::RESTARTOPTIONS_FORCE_UPDATE_FOR_GAME));

                r.Code = EBISU_PENDING;
                r.Description = "Origin is waiting for the game to shutdown.";

                lsx::Write(response.document(), r);
            }
            else
            {
                CreateErrorResponse(EBISU_ERROR_LSX_INVALID_REQUEST, "RestartGame invalid request.", response);
            }
        }

        void SDK_ServiceArea::StartGame(Lsx::Request& request, Lsx::Response& response)
        {
            using namespace Origin::Services::Session;
            using namespace Origin::Engine::Content;

            lsx::StartGameT req;
            int err = ERROR_GENERAL;
            QString description = "Origin cannot start the game";

            if(lsx::Read(request.document(), req))
            {
                lsx::ErrorSuccessT r;

                // Create a restart request here.
                QString offerId;
                EntitlementRef entitlement;
                EntitlementRefList trialEntitlements;

                Content::ContentController *controller = Content::ContentController::currentUserContentController();

                QStringList gameIdparts = req.GameId.split(":");
                QString prefix = gameIdparts[0];
                gameIdparts.pop_front();
                QString operand = gameIdparts.join(":");

                if(!operand.isEmpty())
                {
                    if(prefix == "contentId")
                    {
                        entitlement = controller->entitlementByContentId(operand);

                        // Get the trial games as well.
                        trialEntitlements = controller->entitlementsByTimedTrialAssociation(operand);
                    }
                    else if(prefix == "offerId")
                    {
                        entitlement = controller->entitlementByProductId(operand);
                    }
                    else if(prefix == "multiplayerId")
                    {
                        entitlement = controller->entitlementByMultiplayerId(operand);
                    }
                }
                else
                {
                    operand = prefix;
                    prefix = "masterTitleId";
                }
                
                if(prefix == "masterTitleId")
                {
                    EntitlementRefList entitlements = controller->entitlementsByMasterTitleId(operand);

                    if(entitlements.size())
                    {
                        for(int i = 0; i < entitlements.size(); i++)
                        {
                            if(entitlements[i]->contentConfiguration()->isBaseGame())
                            {
                                if(req.MultiplayerId.isEmpty() || req.MultiplayerId == entitlements[i]->contentConfiguration()->multiplayerId())
                                {
                                    if (entitlements[i]->contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeNormalGame)
                                    {
                                        entitlement = entitlements[i];
                                    }
                                    else
                                    {
                                        trialEntitlements.push_back(entitlements[i]);
                                    }
                                }
                            }
                        }
                    }
                }


                // Select the trial is no full game or the full game is not playable/playing.
                if (trialEntitlements.size() != 0)
                {
                    if (!entitlement || (!entitlement->localContent()->playable() && !entitlement->localContent()->playing()))
                    {
                        foreach(auto e, trialEntitlements)
                        {
                            if (e->contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeTimedTrial_Access ||
                                e->contentConfiguration()->itemSubType() == Origin::Services::Publishing::ItemSubTypeTimedTrial_GameTime)
                            {
                                entitlement = e;
                                break;
                            }
                        }
                    }
                }

                QString connected_entitlement = "";
                if (request.connection()->entitlement()) {
                    connected_entitlement = request.connection()->entitlement()->contentConfiguration()->productId().toLatin1();
                }

                if(entitlement)
                {
                    if(entitlement->localContent()->state() == Origin::Engine::Content::LocalContent::ReadyToPlay)
                    {
                        // Start the game.
                        Origin::Client::MainFlow *pMainFlow = Origin::Client::MainFlow::instance();
                        if(pMainFlow)
                        {
                            Origin::Client::ContentFlowController * contentFlowController = Origin::Client::MainFlow::instance()->contentFlowController();
                            if(contentFlowController)
                            {
                                GetTelemetryInterface()->setSDKStartGameLaunch(connected_entitlement.toStdString().c_str(), entitlement->contentConfiguration()->productId().toStdString().c_str());
                                QMetaObject::invokeMethod(contentFlowController, "startPlayFlow", Qt::QueuedConnection,
                                                          Q_ARG(const QString &, entitlement->contentConfiguration()->productId()),
                                                          Q_ARG(const bool, true),
                                                          Q_ARG(const QString &, req.CommandLine));

                                err = EBISU_PENDING; description = "The game is starting.";
                            }
                        }
                    }
                    else
                    {
                        if(entitlement->localContent()->state() == Origin::Engine::Content::LocalContent::Playing)
                        {
                            err = EBISU_SUCCESS; description = "The game is running";
                        }
                        else
                        {
                            err = EBISU_ERROR_NOT_READY; description = "The game is not ready to play";
                            QString statename;
                            switch (entitlement->localContent()->state()) {
                            case Origin::Engine::Content::LocalContent::State::ReadyToDownload:
                                statename = "Ready To Download"; break;
                            case Origin::Engine::Content::LocalContent::State::Downloading:
                                statename = "Downloading"; break;
                            case Origin::Engine::Content::LocalContent::State::ReadyToInstall:
                                statename = "ReadyToInstall"; break;
                            case Origin::Engine::Content::LocalContent::State::Installing:
                                statename = "Installing"; break;
                            case Origin::Engine::Content::LocalContent::State::Blocked:
                                statename = "Blocked"; break;
                            case Origin::Engine::Content::LocalContent::State::Installed:
                                statename = "Installed"; break;
                            case Origin::Engine::Content::LocalContent::State::Busy:
                                statename = "Busy"; break;
                            case Origin::Engine::Content::LocalContent::State::Paused:
                                statename = "Paused"; break;
                            case Origin::Engine::Content::LocalContent::State::Preparing:
                                statename = "Preparing"; break;
                            case Origin::Engine::Content::LocalContent::State::FinalizingDownload:
                                statename = "FinalizingDownload"; break;
                            case Origin::Engine::Content::LocalContent::State::ReadyToUnpack:
                                statename = "ReadyToUnpack"; break;
                            case Origin::Engine::Content::LocalContent::State::Unpacking:
                                statename = "Unpacking"; break;
                            case Origin::Engine::Content::LocalContent::State::ReadyToDecrypt:
                                statename = "ReadyToDecrypt"; break;
                            case Origin::Engine::Content::LocalContent::State::Decrypting:
                                statename = "Decrypting"; break;
                            case Origin::Engine::Content::LocalContent::State::Unreleased:
                                statename = "Unreleased"; break;
                            default:
                                statename = "Unknown"; break;
                            }

                            GetTelemetryInterface()->Metric_GAME_ALTLAUNCHER_FAIL(connected_entitlement.toLatin1(),
                                req.GameId.toLatin1(), statename.toLatin1());
                        }
                    }
                }
                else
                {
                    err = EBISU_ERROR_NOT_FOUND; description = "Couldn't find the game to start.";
                    GetTelemetryInterface()->Metric_GAME_ALTLAUNCHER_FAIL(connected_entitlement.toLatin1(), req.GameId.toLatin1(), "Not Available");
                }
            }
            else
            {
                err = EBISU_ERROR_LSX_INVALID_REQUEST; description = "StartGame invalid request.";
            }

            CreateErrorSuccess(response, err, description);
        }

        void SDK_ServiceArea::ExtendTrial(Lsx::Request& request, Lsx::Response& response)
        {
            lsx::ExtendTrialT req;

            if(lsx::Read(request.document(), req))
            {

                Origin::Engine::Content::EntitlementRef entitlement = request.connection()->entitlement();

                if(!entitlement.isNull())
                {
                    auto serverReply = Origin::Services::Trials::TrialServiceClient::burnTime(entitlement->contentConfiguration()->contentId(), req.RequestTicket);
                    auto resp = new ResponseWrapper<SDK_ServiceArea, Origin::Services::Trials::TrialBurnTimeResponse>(this, &SDK_ServiceArea::ExtendTrialResponse, serverReply, response);

                    ORIGIN_VERIFY_CONNECT(serverReply, SIGNAL(finished()), resp, SLOT(finished()));
                }
                else
                {
                    CreateErrorSuccess(response, EBISU_ERROR_NOT_FOUND, "User is not entitled to the game.");
                }
            }
            else
            {
                CreateErrorSuccess(response, EBISU_ERROR_LSX_INVALID_REQUEST, "Data couldn't be decoded.");
            }
        }

        bool SDK_ServiceArea::ExtendTrialResponse(Origin::Services::Trials::TrialBurnTimeResponse *serverReply, Lsx::Response& response)
        {
            auto localContent = response.connection()->entitlement()->localContent();

            if(localContent)
            {
                if(serverReply->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == Origin::Services::Http200Ok)
                {
                    // Update the entitlements time remaining.
                    localContent->setTrialTimeRemaining(serverReply->timeInitial(), serverReply->timeRemaining(), serverReply->timeSlice(), serverReply->timeStamp());

                    // Reset the disconnection timer.
                    localContent->setTrailReconnectingTimeLeft(-1);

                    lsx::ExtendTrialResponseT resp;

                    resp.Code = EBISU_SUCCESS;
                    resp.ResponseTicket = serverReply->timeTicket();
                    resp.TimeGranted = serverReply->timeSlice();

                    resp.TotalTimeRemaining = serverReply->timeRemaining() + serverReply->timeSlice();     // The SDK uses total time remaining at the time of query. (including the timeslice).

                    lsx::Write(response.document(), resp);

                    if(!serverReply->hasTimeLeft())
                    {
                        emit trialExpired(response.connection()->entitlement());
                    }
                }
                else
                {
                    // If already online then just ignore 
                    Engine::UserRef user = Engine::LoginController::instance()->currentUser();
                    if(Services::Connection::ConnectionStatesService::isUserOnline(user->getSession()) == false)
                    {
                        // Set the disconnection timer if not already running.
                        if(localContent->trialReconnectingTimeLeft() == -1)
                        {
                            localContent->setTrailReconnectingTimeLeft(60000);
                        }

                        emit trialOffline(response.connection()->entitlement());
                        ORIGIN_LOG_ACTION << "Timed Trial offline error";
                    }
                    else
                    {
                        ORIGIN_LOG_ACTION << "Timed Trial unknown error";
                    }

                    CreateServerErrorResponse(serverReply->reply(), response);
                }
            }
            else
            {
                CreateErrorSuccess(response, EBISU_ERROR_NOT_FOUND, "The entitlement couldn't be found.");
            }
            return true;
        }

        void SDK_ServiceArea::SendGameMessage(Lsx::Request& request, Lsx::Response& response)
        {
            lsx::SendGameMessageT req;

            if(lsx::Read(request.document(), req))
            {
                lsx::GameMessageEventT msg;

                msg.GameId = req.GameId;
                msg.Message = req.Message;

                sendEvent(msg);

                lsx::ErrorSuccessT resp; 
                
                unsigned int conectionCount = handler()->connectionCount();

                resp.Code = conectionCount > 1 ? EBISU_SUCCESS : EBISU_ERROR_NOT_FOUND;
                resp.Description = conectionCount > 1 ? "Message Sent" : "Not enough recipients.";

                lsx::Write(response.document(), resp);
            }
        }


        void ConvertType(lsx::AchievementT &dest, const server::AchievementT &src, QString achievementSetId)
        {
            dest.Description = src.desc;
            dest.HowTo = src.howto;
            dest.Name = src.name;
            dest.ImageId = QString("achieve:%1-%2").arg(achievementSetId).arg(dest.Id);
            dest.Progress = src.p;
            dest.Total = src.t;
            dest.Count = src.cnt;

            if(src.u > 0)
            {
                ConvertType(dest.GrantDate, src.u * 1000);
            }
            if(src.e > 0)
            {
                ConvertType(dest.Expiration, src.e * 1000);
            }
        }

        void ConvertType(lsx::AchievementT &dest, const Origin::Engine::Achievements::AchievementRef &src)
        {
            dest.Id = src->id();
            dest.Name = src->name();
            dest.Progress = src->progress();
            dest.Total = src->total();
            dest.Count = src->count();
            dest.Description = src->description();
            dest.HowTo = src->howto();
            dest.ImageId = QString("achieve:") + src->achievementSet()->achievementSetId() + "-" + src->id();
            ConvertType(dest.GrantDate, src->updateTime().toUTC());
            ConvertType(dest.Expiration, src->expirationTime().toUTC());
        }

        void ConvertType(lsx::AchievementSetT &dest, const Origin::Engine::Achievements::AchievementSetRef &src, bool bAll)
        {
            dest.Name = src->achievementSetId();
            dest.GameName = src->displayName();

            foreach(Origin::Engine::Achievements::AchievementRef achievement, src->achievements())
            {
                if(bAll || achievement->progress() > 0 || achievement->count() > 0)
                {
                    lsx::AchievementT a;

                    ConvertType(a, achievement);

                    dest.Achievement.push_back(a);
                }
            }
        }

        void SDK_ServiceArea::GrantAchievement(Lsx::Request& request, Lsx::Response& response )
        {
            lsx::GrantAchievementT ga;

            if(lsx::Read(request.document(), ga))
            {
                Content::ContentController *controller = Content::ContentController::currentUserContentController();

                if(controller)
                {
                    Content::EntitlementRef entitlement = request.connection()->entitlement();
                    if(entitlement.isNull())
                    {
                        CreateErrorResponse(ERROR_GENERAL, "The connection is not connected to a game.", response);
                        return;
                    }

                    Origin::Engine::UserRef user = Engine::LoginController::instance()->currentUser();

                    // Are we requesting for ourselves
                    if(user.isNull() || ga.PersonaId != (uint64_t)user->personaId())
                    {
                        CreateErrorResponse(EBISU_ERROR_INVALID_ARGUMENT, "The personaId doesn't match the user.", response);
                        return;
                    }

                    QString achievementSet = entitlement->contentConfiguration()->achievementSet();

                    // If Origin is online, grant the achievement.
                    if(Services::Connection::ConnectionStatesService::isUserOnline(user->getSession()))
                    {
                        QString url = m_AchievementServiceURL + "/personas/" + QString::number(ga.PersonaId) + "/" + achievementSet + "/" + ga.AchievementId + "/progress";

                        QUrl URL(url);
                        QUrlQuery URLQuery(URL);
                        URLQuery.addQueryItem("lang", QLocale().name());
                        URL.setQuery(URLQuery);

                        QNetworkRequest serverRequest(URL);
                        serverRequest.setRawHeader("X-AuthToken", Origin::Services::Session::SessionService::accessToken(Origin::Engine::LoginController::currentUser()->getSession()).toLatin1());
                        serverRequest.setRawHeader("X-Source", "Origin");
                        serverRequest.setRawHeader("X-Api-Version", "2");
                        serverRequest.setRawHeader("X-Application-Key", entitlement->contentConfiguration()->masterTitleId().toLatin1());
                        serverRequest.setRawHeader("X-Source", "Origin");
                        serverRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

                        QString points = "{ \"points\":" + QString::number(ga.Progress) + " }";

                        AddHMACHeaders(serverRequest, ga.AchievementCode.toLatin1(), points.toLatin1());

                        GrantAchievementsPayload payload;
                        payload.ga = ga;
                        payload.achievementSet = achievementSet;
                        payload.entitlement = entitlement;

                        DoServerCall(payload, serverRequest, OPERATION_POST, &SDK_ServiceArea::GrantAchievementsProcessResponse, response, points.toLatin1());
                        return;
                    }
                    else
                    {
                        Origin::Engine::Achievements::AchievementManager::instance()->queueAchievementGrant(QString::number(ga.PersonaId), entitlement->contentConfiguration()->masterTitleId(), achievementSet, ga.AchievementId, ga.AchievementCode, ga.Progress);

                        GetTelemetryInterface()->Metric_ACHIEVEMENT_ACH_POST_FAIL(entitlement->contentConfiguration()->productId().toLatin1(), achievementSet.toLatin1(), ga.AchievementId.toLatin1(), 0, "Origin is Offline");

                        CreateErrorResponse(EBISU_PENDING, "The achievement is stored in the off-line achievement cache for later posting", response);
                        return;
                    }
                }
                else
                {
                    CreateErrorSuccess(response, EBISU_ERROR_NOT_LOGGED_IN, "There is no user");
                    return;
                }
            }
            CreateErrorSuccess(response, EBISU_ERROR_LSX_INVALID_REQUEST, "Data couldn't be decoded.");
        }

        bool SDK_ServiceArea::GrantAchievementsProcessResponse(GrantAchievementsPayload & payload, QNetworkReply * serverReply, Lsx::Response& response)
        {
            int httpError = serverReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            if(httpError == 200)
            {
                QByteArray replyString = serverReply->readAll();

                server::AchievementT achievement;

                QScopedPointer<INodeDocument> json(CreateJsonDocument());
                if(json->Parse(replyString.data()))
                {
                    if(server::Read(json.data(), achievement))
                    {
                        // Compose the reply
                        lsx::AchievementT a;

                        a.Id = payload.ga.AchievementId;
                        ConvertType(a, achievement, payload.achievementSet);

                        lsx::Write(response.document(), a);

                        QMetaObject::invokeMethod(Origin::Engine::Achievements::AchievementManager::instance()->currentUsersPortfolio().data(), "refreshPoints", Qt::QueuedConnection);
                        QMetaObject::invokeMethod(Origin::Engine::Achievements::AchievementManager::instance()->currentUsersPortfolio().data(), "updateAchievementSet", Qt::QueuedConnection, Q_ARG(QString, payload.achievementSet));

                        GetTelemetryInterface()->Metric_ACHIEVEMENT_ACH_POST_SUCCESS(payload.entitlement->contentConfiguration()->productId().toLatin1(), payload.achievementSet.toLatin1(), payload.ga.AchievementId.toLatin1());

                        return true;
                    }
                }

                CreateErrorSuccess(response, EBISU_ERROR_LSX_INVALID_RESPONSE, "Server reply not understood.");
            }
            else
            {
                if(httpError != 401)
                {
                    Origin::Engine::Achievements::AchievementManager::instance()->queueAchievementGrant(QString::number(payload.ga.PersonaId), payload.entitlement->contentConfiguration()->masterTitleId(), payload.achievementSet, payload.ga.AchievementId, payload.ga.AchievementCode, payload.ga.Progress);

                    GetTelemetryInterface()->Metric_ACHIEVEMENT_ACH_POST_FAIL(payload.entitlement->contentConfiguration()->productId().toLatin1(), payload.achievementSet.toLatin1(), payload.ga.AchievementId.toLatin1(), httpError, serverReply ? serverReply->readAll().data() : "Origin is Offline");

                    CreateErrorResponse(EBISU_PENDING, "The achievement is stored in the off-line achievement cache for later posting", response);
                }
                else
                {
                    CreateServerErrorResponse(serverReply, response);
                }
            }
            return true;
        }


        void SDK_ServiceArea::PostAchievementEvents(Lsx::Request& request, Lsx::Response& response)
        {
            lsx::PostAchievementEventsT req;

            if(lsx::Read(request.document(), req))
            {
                Content::ContentController *controller = Content::ContentController::currentUserContentController();

                if(controller)
                {
                    Content::EntitlementRef entitlement = request.connection()->entitlement();
                    if(entitlement.isNull())
                    {
                        CreateErrorResponse(ERROR_GENERAL, "The connection is not connected to a game.", response);
                        return;
                    }

                    Origin::Engine::UserRef user = Engine::LoginController::instance()->currentUser();

                    // Are we requesting for ourselves
                    if(user.isNull() || req.PersonaId != (uint64_t)user->personaId())
                    {
                        CreateErrorResponse(EBISU_ERROR_INVALID_ARGUMENT, "The personaId doesn't match the user.", response);
                        return;
                    }

                    QString achievementSet = entitlement->contentConfiguration()->achievementSet();
                    QString url = m_AchievementServiceURL + "/events/persona/" + QString::number(req.PersonaId) + "/" + achievementSet;

                    QNetworkRequest serverRequest(url);
                    serverRequest.setRawHeader("X-AuthToken", Origin::Services::Session::SessionService::accessToken(Origin::Engine::LoginController::currentUser()->getSession()).toLatin1());
                    serverRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

                    QString eventsSnip = "{\"events\":[%1]}";
                    QString eventSnip = "{\"id\": \"%1\", \"data\": {%2}}";
                    QString paramSnip = "\"%1\": \"%2\"";

                    QStringList events;

                    for(size_t i = 0; i < req.Events.size(); i++)
                    {
                        lsx::EventT &event = req.Events[i];

                        QStringList params;

                        for(size_t p = 0; p < event.Attributes.size(); p++)
                        {
                            lsx::EventParamT &param = event.Attributes[p];
                            params.append(paramSnip.arg(param.Name).arg(param.Value));
                        }

                        events.push_back(eventSnip.arg(event.EventId).arg(params.join(",")));
                    }

                    QString body = eventsSnip.arg(events.join(","));

 
                    // If Origin is online, grant the achievement.

                    if(Services::Connection::ConnectionStatesService::isUserOnline(user->getSession()))
                    {
                        PostAchievementEventPayload payload;

                        payload.request = req;
                        payload.events = body;
                        payload.entitlement = entitlement;
                        payload.achievementSet = achievementSet;

                        DoServerCall(payload, serverRequest, OPERATION_POST, &SDK_ServiceArea::PostAchievementEventsProcessResponse, response, body.toUtf8());

                        return;
                    }
                    else
                    {
                        Origin::Engine::Achievements::AchievementManager::instance()->queueAchievementEvent(QString::number(req.PersonaId), entitlement->contentConfiguration()->masterTitleId(), achievementSet, body);
                        CreateErrorResponse(EBISU_PENDING, "The events are stored in the off-line achievement cache for later posting", response);
                    }
                }
                else
                {
                    CreateErrorSuccess(response, EBISU_ERROR_NOT_LOGGED_IN, "There is no user");
                }
            }
            else
            {
                CreateErrorSuccess(response, EBISU_ERROR_LSX_INVALID_REQUEST, "Data couldn't be decoded.");
            }
        }

        bool SDK_ServiceArea::PostAchievementEventsProcessResponse(PostAchievementEventPayload & payload, QNetworkReply * serverReply, Lsx::Response& response)
        {
            int httpError = serverReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            if(httpError == 200)
            {
                // Just for debugging purposes.
                QByteArray data = serverReply->readAll();

                CreateErrorSuccess(response, EBISU_SUCCESS, "Events Successfully posted.");
            }
            else
            {
                if(httpError != 401)
                {
                    Origin::Engine::Achievements::AchievementManager::instance()->queueAchievementEvent(QString::number(payload.request.PersonaId), payload.entitlement->contentConfiguration()->masterTitleId(), payload.achievementSet, payload.events);
                    CreateErrorResponse(EBISU_PENDING, "The events are stored in the off-line achievement cache for later posting", response);
                }
                else
                {
                    CreateServerErrorResponse(serverReply, response);
                }
            }
            return true;
        }

        void SDK_ServiceArea::QueryAchievements(Lsx::Request& request, Lsx::Response& response )
        {
            using namespace Origin::Engine::Achievements;

            lsx::QueryAchievementsT qa;

            if(lsx::Read(request.document(), qa))
            {                             
                QStringList gameIds;
                QString gameName;

                for (unsigned int i = 0; i < qa.GameId.size(); i++)
                {
                    gameIds.append(qa.GameId[i]);
                }

                // If the game doesn't provide any gameId we take the one of the entitlement associated with the game.

                if(gameIds.isEmpty())
                {
                    Content::ContentController *controller = Content::ContentController::currentUserContentController();

                    if(controller)
                    {
                        Content::EntitlementRef entitlement = request.connection()->entitlement();
                        if(entitlement.isNull())
                        {
                            CreateErrorResponse(EBISU_ERROR_NOT_FOUND, "The connection is not connected to a game.", response);
                            return;
                        }

                        if(entitlement->contentConfiguration()->achievementSet().isEmpty())
                        {
                            CreateErrorResponse(EBISU_ERROR_NOT_FOUND, "The entitlement doesn't contain an achievement set.", response);
                            return;
                        }

                        gameIds.append(entitlement->contentConfiguration()->achievementSet());
                        gameName = entitlement->contentConfiguration()->displayName();
                    }
                    else
                    {
                        CreateErrorSuccess(response, EBISU_ERROR_NOT_LOGGED_IN, "There is no user");
                        return;
                    }
                }

                QStringList notFound;
                lsx::AchievementSetsT * ass = new lsx::AchievementSetsT();

                Origin::Engine::UserRef user = Engine::LoginController::instance()->currentUser();

                // Are we requesting for ourselves
                if(!user.isNull() && qa.PersonaId == (uint64_t)user->personaId())
                {
                    AchievementPortfolioRef portfolio = AchievementManager::instance()->currentUsersPortfolio();

                    foreach(QString achievementSetId, gameIds)
                    {
                        AchievementSetRef achievementSet = portfolio->getAchievementSet(achievementSetId);

                        if(!achievementSet.isNull())
                        {
                            lsx::AchievementSetT set;

                            ConvertType(set, achievementSet, qa.All);

                            ass->AchievementSet.push_back(set);
                        }
                        else
                        {
                            // Get the sets that are not cached directly for GOS.
                            notFound.append(achievementSetId);
                        }
                    }
                }
                else
                {
                    notFound = gameIds;
                }


                int *count = NULL;
                ResponseWrapperEx<lsx::AchievementSetsT *, SDK_ServiceArea, int> *gatherWrapper = NULL;

                if(notFound.size())
                {
                    count = new int;
                    *count = notFound.size();
                    gatherWrapper = new ResponseWrapperEx<lsx::AchievementSetsT *, SDK_ServiceArea, int>(ass, this, &SDK_ServiceArea::GatherResponse, count, response);
                }
                    
                for(int gi = 0; gi < notFound.size(); gi++)
                {
                    QString achievementSet = notFound[gi];

                    QString url = m_AchievementServiceURL + "/personas/" + QString::number(qa.PersonaId) + "/" + achievementSet + "/" + (qa.All ? "all" : "active");
                    QUrl URL(url);
                    QUrlQuery URLQuery(URL);
                    URLQuery.addQueryItem("lang", QLocale().name());
                    URL.setQuery(URLQuery);

                    QNetworkRequest serverRequest(URL);

                    serverRequest.setRawHeader("X-Api-Version", "2");
                    serverRequest.setRawHeader("X-Application-Key", "Origin");
                    serverRequest.setRawHeader("X-AuthToken", Origin::Services::Session::SessionService::accessToken(Origin::Services::Session::SessionService::currentSession()).toLatin1());

                    QueryAchievementsPayload payload;
                    payload.sets = ass;
                    payload.achievementSet = achievementSet;
                    payload.gameName = gameName;
                    payload.gatherWrapper = gatherWrapper;

                    DoServerCall(payload, serverRequest, OPERATION_GET, &SDK_ServiceArea::QueryAchievementsProcessResponse, response);
                }

                if(notFound.size() == 0)
                {
                    lsx::Write(response.document(), *ass);
                    delete ass;
                }

                return;
            }

            CreateErrorSuccess(response, EBISU_ERROR_LSX_INVALID_REQUEST, "Data couldn't be decoded.");
        }

        template <typename T> bool SDK_ServiceArea::GatherResponse(T *& data, int * jobsPending, Lsx::Response& response)
        {
            (*jobsPending)--;

            if(*jobsPending <= 0)
            {
                // Check if there is already a response written into the response.
                if(response.isPending())
                {
                    lsx::Write(response.document(), *data);
                }

                // The gather process needs to delete the document.
                delete data;
                return true;
            }
            return false;
        }

        bool SDK_ServiceArea::QueryAchievementsProcessResponse(QueryAchievementsPayload &payload, QNetworkReply * serverReply, Lsx::Response& response)
        {
            if(serverReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200)
            {
                QByteArray replyString = serverReply->readAll();

                QScopedPointer<INodeDocument> json(CreateJsonDocument());


                if(json->Parse(replyString.data()))
                {
                    lsx::AchievementSetT set;

                    set.Name = payload.achievementSet;
                    set.GameName = payload.gameName;

                    if(json->FirstChild())
                    {
                        do
                        {
                            server::AchievementT achievement;
                            lsx::AchievementT a;

                            a.Id = QString(json->GetName());

                            if(server::ReadClass(json.data(), achievement))
                            {
                                ConvertType(a, achievement, set.Name);

                                set.Achievement.push_back(a);
                            }
                        } while(json->NextChild());
                    }
                    payload.sets->AchievementSet.push_back(set);
                }
            }
            else
            {
                CreateServerErrorResponse(serverReply, response);
            }

            payload.gatherWrapper->finished();

            return true;
        }

        void SDK_ServiceArea::achievementGranted(Origin::Engine::Achievements::AchievementSetRef achievementset, Origin::Engine::Achievements::AchievementRef achievement)
        {
            lsx::AchievementSetsT sets;
            lsx::AchievementSetT set;

            Origin::Engine::Content::EntitlementRef entitlement = achievementset->entitlement();

            if(!entitlement.isNull())
                set.GameName = entitlement->contentConfiguration()->displayName();

            set.Name = achievementset->achievementSetId();

            lsx::AchievementT a;

            ConvertType(a, achievement);

            set.Achievement.push_back(a);
            sets.AchievementSet.push_back(set);

            sendEvent(sets);
        }


        void SDK_ServiceArea::CreateServerErrorResponse(QNetworkReply * pReply, Lsx::Response& response)
        {
            uint32_t status = pReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QString error = pReply->readAll();
            QString errorString = pReply->errorString();

            QString Description = QString("Request: ") + pReply->url().toString() + "\nInvalid Response Code = " + QVariant(status).toString() + "\n" + error + "\n" + errorString;

            ORIGIN_LOG_ERROR << Description;

            CreateErrorResponse((int)(EBISU_ERROR_PROXY + status), Description.toUtf8(), response);
        }

        void SDK_ServiceArea::CreateErrorResponse(int code, const char * description, Lsx::Response& response)
        {
            lsx::ErrorSuccessT e;
            e.Code = code;
            e.Description = description;
            lsx::Write(response.document(), e);
        }

        template <typename T> void SDK_ServiceArea::DoServerCall(T & data, QNetworkRequest &serverRequest, NetworkMethod method, bool (SDK_ServiceArea::*callback)(T &, QNetworkReply * reply, Lsx::Response &response), Lsx::Response &response, const QByteArray &doc)
        {
            // Wait for the reply to come back.
            QEventLoop eventLoop;

            Origin::Services::NetworkAccessManager *pNetworkManager = Origin::Services::NetworkAccessManager::threadDefaultInstance();

            QNetworkReply * serverReply = NULL;

            switch(method)
            {
            case OPERATION_GET:
                serverReply = pNetworkManager->get(serverRequest);
                break;
            case OPERATION_PUT:
                if(doc.length() > 0)
                {
                    serverReply = pNetworkManager->put(serverRequest, doc);
                }
                else
                {
                    serverReply = pNetworkManager->put(serverRequest, QByteArray(NULL));
                }
                break;
            case OPERATION_POST:
                    if(doc.length() > 0)
                {
                    serverReply = pNetworkManager->post(serverRequest, doc);
                }
                else
                {
                    serverReply = pNetworkManager->post(serverRequest, QByteArray(NULL));
                }
                break;
            case OPERATION_DELETE:
                serverReply = pNetworkManager->deleteResource(serverRequest);
                break;
            default:
                ORIGIN_ASSERT_MESSAGE(false, "Invalid Network Method");
                break;
            }

            ResponseWrapperEx<T, SDK_ServiceArea, QNetworkReply> * resp = new ResponseWrapperEx<T, SDK_ServiceArea, QNetworkReply>(data, this, callback, serverReply, response);

            ORIGIN_VERIFY_CONNECT(serverReply, SIGNAL(finished()), resp, SLOT(finished()));
        }

        bool SDK_ServiceArea::IsIGOEnabled(Lsx::Request& request)
        {
			if(request.connection() && !request.connection()->entitlement().isNull())
			{
				return request.connection()->entitlement()->contentConfiguration()->isIGOEnabled();
			}
            else
                return IGOController::isEnabled();
        }

        bool SDK_ServiceArea::IsIGOAvailable(Lsx::Request& request)
        {                                                                              
            if (IGOController::instantiated() && IsIGOEnabled(request))
            {
                bool isScreenLargeEnough = IGOController::instance()->igowm()->isScreenLargeEnough();

                static QHash<int, bool> connectionHasRequestedThisBefore;

                // Slight change of behavior. For older SDK's return true the first time.
                if(request.connection()->CompareSDKVersion(1, 0, 0, 15) >= 0)
                {
                    return isScreenLargeEnough && IGOController::instance()->isConnected();
                }
                else
                {
                    // Check whether this is the first time this connection asked for this info.
                    if(connectionHasRequestedThisBefore[request.connection()->id()] == true)
                    {
                        return isScreenLargeEnough && IGOController::instance()->isConnected();
                    }
                    else
                    {
                        connectionHasRequestedThisBefore[request.connection()->id()] = true;
                        return true;
                    }
                }
            }
            return false;
        }

        void SDK_ServiceArea::GoOnline( Lsx::Request& request, Lsx::Response& response )
        {
            Origin::Services::Connection::ConnectionStatesService * pService = Origin::Services::Connection::ConnectionStatesService::instance();
            Origin::Services::Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();

            if(pService->isGlobalStateOnline())
            {
                if(!session.isNull() && Origin::Services::Session::SessionService::isOnline(session))
                {
                    CreateErrorSuccess(response, EBISU_SUCCESS, "Already online.");
                }
                else
                {
                    if(!session.isNull())
                    {
                        if(pService->canGoOnline(session))
                        {
                            emit goOnline();

                            QEventLoop eventLoop;
                            QTimer::singleShot(20000, &eventLoop, SLOT(quit()));

                            ORIGIN_VERIFY_CONNECT(pService, SIGNAL(nowOnlineUser()), &eventLoop, SLOT(quit()));
                            ORIGIN_VERIFY_CONNECT(pService, SIGNAL(nowOfflineUser()), &eventLoop, SLOT(quit()));
                            ORIGIN_VERIFY_CONNECT(pService, SIGNAL(authenticationRequired()), &eventLoop, SLOT(quit()));
                            ORIGIN_VERIFY_CONNECT(Origin::Client::MainFlow::instance(), SIGNAL(singleLoginAccountInUse()), &eventLoop, SLOT(quit()));

                            eventLoop.exec();

                            if(pService->isUserOnline(session))
                            {
                                CreateErrorSuccess(response, EBISU_SUCCESS, "User is online.");
                            }
                            else
                            {
                                if(pService->isRequiredUpdatePending())
                                {
                                    CreateErrorSuccess(response, EBISU_ERROR_MANDATORY_ORIGIN_UPDATE_PENDING, "Cannot go online without updating Origin.");
                                }
                                else if(!pService->isAuthenticated(session))
                                {
                                    CreateErrorSuccess(response, EBISU_ERROR_NOT_LOGGED_IN, "User couldn't be logged in.");
                                }
                                else if(!pService->isSingleLoginCheckCompleted())
                                {
                                    CreateErrorSuccess(response, EBISU_ERROR_ACCOUNT_IN_USE, "Another Origin is running with this account info.");
                                }
                                else
                                {
                                    CreateErrorSuccess(response, EBISU_ERROR, "Going online failed.");
                                }
                            }
                        }
                        else
                        {
                            CreateErrorSuccess(response, EBISU_ERROR_NO_SERVICE, "No service available.");
                        }
                    }
                    else
                    {
                        CreateErrorSuccess(response, EBISU_ERROR_NOT_LOGGED_IN, "User is not logged in.");
                    }
                }
            }
            else
            {
                if(pService->isRequiredUpdatePending())
                {
                    CreateErrorSuccess(response, EBISU_ERROR_MANDATORY_ORIGIN_UPDATE_PENDING, "Cannot go online without updating Origin.");
                }
                else
                {
                    CreateErrorSuccess(response, EBISU_ERROR_NO_NETWORK, "No internet connection available.");
                }
            }
        }

#ifdef ORIGIN_PC
        void SDK_ServiceArea::BroadcastStateChanged(int state)
        {
            lsx::BroadcastEventT r;

            r.State = (lsx::BroadcastStateT)state;

            sendEvent(r);
        }

        void SDK_ServiceArea::BroadcastDialogOpen()
        {
            BroadcastStateChanged(lsx::BROADCASTSTATE_DIALOG_OPEN);
        }

        void SDK_ServiceArea::BroadcastDialogClosed()
        {
            BroadcastStateChanged(lsx::BROADCASTSTATE_DIALOG_CLOSED);
        }

        void SDK_ServiceArea::BroadcastAccountLinkDialogOpen()
        {
            BroadcastStateChanged(lsx::BROADCASTSTATE_ACCOUNTLINKDIALOG_OPEN);
        }

        void SDK_ServiceArea::BroadcastAccountDisconnected()
        {
            BroadcastStateChanged(lsx::BROADCASTSTATE_ACCOUNT_DISCONNECTED);
        }

        void SDK_ServiceArea::BroadcastStarted()
        {
            BroadcastStateChanged(lsx::BROADCASTSTATE_STARTED);
        }

        void SDK_ServiceArea::BroadcastStopped()
        {
            BroadcastStateChanged(lsx::BROADCASTSTATE_STOPPED);
        }

        void SDK_ServiceArea::BroadcastBlocked()
        {
            BroadcastStateChanged(lsx::BROADCASTSTATE_BLOCKED);
        }

        void SDK_ServiceArea::BroadcastStartPending()
        {
            BroadcastStateChanged(lsx::BROADCASTSTATE_START_PENDING);
        }

        void SDK_ServiceArea::BroadcastError()
        {
            BroadcastStateChanged(lsx::BROADCASTSTATE_ERROR);
        }

        void SDK_ServiceArea::BroadcastStart( Lsx::Request& request, Lsx::Response& response )
        {
            //we want this to run on the main thread
            QMetaObject::invokeMethod(IGOController::instance(), "attemptBroadcastStart", Q_ARG(Origin::Engine::IIGOCommandController::CallOrigin, Origin::Engine::IIGOCommandController::CallOrigin_SDK));
           CreateErrorSuccess(response, EBISU_PENDING, "Broadcast Start Requested");
        }

        void SDK_ServiceArea::BroadcastStop( Lsx::Request& request, Lsx::Response& response )
        {
            //we want this to run on the main thread
            QMetaObject::invokeMethod(IGOController::instance(), "attemptBroadcastStop", Q_ARG(Origin::Engine::IIGOCommandController::CallOrigin, Origin::Engine::IIGOCommandController::CallOrigin_SDK));
            CreateErrorSuccess(response, EBISU_PENDING, "Broadcast Stop Requested");
        }

        void SDK_ServiceArea::GetBroadcastStatus( Lsx::Request& request, Lsx::Response& response )
        {
            lsx::BroadcastStatusT r;
            r.status  = lsx::BROADCASTSTATE_STOPPED;
            if (IGOController::instantiated())
            {
                if (IGOController::instance()->isBroadcastingBlocked())
                {
                    r.status = lsx::BROADCASTSTATE_BLOCKED;
                }
                else if (!IGOController::instance()->isBroadcastTokenValid())
                {
                    r.status = lsx::BROADCASTSTATE_ACCOUNT_DISCONNECTED;
                }
                else if (IGOController::instance()->isBroadcasting())
                {
                    r.status = lsx::BROADCASTSTATE_STARTED;
                }
                else if (IGOController::instance()->isBroadcastingPending())
                {
                    r.status = lsx::BROADCASTSTATE_START_PENDING;
                }
            }

            lsx::Write(response.document(), r);
        }
#endif

        QString SDK_ServiceArea::cacheFolder(const QString &section) const
        {
            Origin::Services::Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();

            if (!session.isNull())
            {
                QString environment = Origin::Services::readSetting(Origin::Services::SETTING_EnvironmentName);
                QString userId = Origin::Services::Session::SessionService::nucleusUser(session);
                userId = QCryptographicHash::hash(userId.toLatin1(), QCryptographicHash::Sha1).toHex().toUpper();

                // compute the path to the achievement cache, which consists of a platform dependent
                // root directory, hard-coded application specific directories, environment directory 
                // (omitted in production), and a per-user directory containing the actual cache files.
                QStringList path;
#if defined(ORIGIN_PC)
                WCHAR buffer[MAX_PATH];
                SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, buffer );
                path << QString::fromWCharArray(buffer) << "Origin";
#elif defined(ORIGIN_MAC)
                path << Origin::Services::PlatformService::commonAppDataPath();
#else
#error Must specialize for other platform.
#endif

                path.append(section);
                if ( environment != "production" )
                    path.append(environment);

                path.append(userId); 

                QString cachePath = path.join("/");

                if(QDir(cachePath).exists() || QDir().mkpath(cachePath))
                    return cachePath;

            }
            return QString();
        }

        bool SDK_ServiceArea::ReadRecentPlayers( lsx::rpT &recentPlayer )
        {
            QFile file(cacheFolder("RecentPlayers") + "/r.dat");
            if(file.open(QIODevice::ReadOnly))
            {
                QByteArray data = file.readAll();
                QList<QByteArray> items = data.split('O');

                if(items.size()==2)
                {
                    QByteArray salt = "kdjfksdufjklsdf9080934lkjd";
                    QByteArray check = QCryptographicHash::hash(items[0] + salt, QCryptographicHash::Sha1).toHex().toLower();

                    if(check == items[1])
                    {
                        QByteArray decryptedData = Origin::Services::Crypto::SimpleEncryption().decrypt(items[0].data()).c_str();

                        QDomDocument doc;
                        doc.setContent(decryptedData);
                        LSXWrapper d(doc);

                        lsx::Read(&d, recentPlayer);

                        file.close();
                        return recentPlayer.version>0;
                    }
                }
            }
            return false;
        }

        bool SDK_ServiceArea::WriteRecentPlayers( lsx::rpT &recentPlayer )
        {
            QDomDocument doc;
            LSXWrapper resp(doc);

            lsx::Write(&resp, recentPlayer);

#ifdef _DEBUG
            QFile file(cacheFolder("RecentPlayers") + "/r.xml");
            if(file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QString data;
                QTextStream dataStream(&file);
                resp.GetRootNode().save(dataStream, 2);
                file.close();
        }
#endif
            QFile encryptedFile(cacheFolder("RecentPlayers") + "/r.dat");
            if(encryptedFile.open(QIODevice::WriteOnly))
            {
                QString xml;
                QTextStream xmlStream(&xml);
                resp.GetRootNode().save(xmlStream, 0);

                QByteArray data = Origin::Services::Crypto::SimpleEncryption().encrypt(xml.toUtf8().data()).c_str();
                QByteArray salt = "kdjfksdufjklsdf9080934lkjd";
                QByteArray check = QCryptographicHash::hash(data + salt, QCryptographicHash::Sha1).toHex().toLower();

                encryptedFile.write(data);
                encryptedFile.write("O");
                encryptedFile.write(check);
                encryptedFile.close();
                return true;
            }
            return false;
        }

        void SDK_ServiceArea::MinimizeRequest()
        {
            lsx::MinimizeRequestT msg;
            sendEvent(msg);
        }

        void SDK_ServiceArea::RestoreRequest()
        {
            lsx::RestoreRequestT msg;
            sendEvent(msg);
        }

    }
}





