///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2011 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: SDK_ServiceArea.h
//	SDK Service Area
//	
//	Author: Hans van Veenendaal

#ifndef _SDK_SERVICE_AREA_
#define _SDK_SERVICE_AREA_

#include "Service/BaseService.h"
#include "services/rest/AvatarServiceClient.h"

#include <QObject>
#include <QPointer>
#include <QSharedPointer>
#include <QNetworkRequest.h>
#include "engine/content/Entitlement.h"
#include "engine/login/User.h"
#include "engine/achievements/achievementmanager.h"
#include "lsx.h"
#include "engine/social/AvatarManager.h"

namespace Origin
{
    namespace Engine
    {
        namespace Content
        {
            class LocalContent;
        }
    }

    namespace Services
    {
        namespace Trials
        {
            class TrialBurnTimeResponse;
        }
    }
}



namespace lsx
{
    struct QueryImageT;
    struct PostWincodesT;
}

namespace Origin
{
    namespace SDK
    {
        class SDK_ServiceArea :	public BaseService
        {
            Q_OBJECT

            struct ImageDetails
            {
                lsx::QueryImageT image;
                quint64 userId;
                Origin::Engine::Social::AvatarManager *avatarManager;
                Origin::Services::AvatarServiceClient::ImageSize imageSize; 
            };

        public:
            // Singleton functions
            static SDK_ServiceArea* instance();
            static SDK_ServiceArea* create(Lsx::LSX_Handler* handler, class XMPP_ServiceArea *xmpp_service);
            void destroy();

            virtual ~SDK_ServiceArea();

            static bool getAutostart();

        signals:
            void goOnline();
            void trialExpired(const Origin::Engine::Content::EntitlementRef entitlement);
            void trialOffline(const Origin::Engine::Content::EntitlementRef entitlement);

        private slots:
            void IgoStateChanged(bool bVisible, bool bSDKCall);
            void IgoUserOpenAttemptIgnored();
#ifdef ORIGIN_PC
            void BroadcastDialogOpen();
            void BroadcastDialogClosed();
            void BroadcastAccountLinkDialogOpen();
            void BroadcastAccountDisconnected();
            void BroadcastStarted();
            void BroadcastStopped();
            void BroadcastBlocked();
            void BroadcastStartPending();
            void BroadcastError();
#endif
            void OnLoggedIn(Origin::Engine::UserRef);
            void OnLoggedOut();
            void OnBlockListChanged();
            void onUserOnlineStatusChanged();
            void entitlementListRefreshed(const QList<Origin::Engine::Content::EntitlementRef> entitlementsAdded, const QList<Origin::Engine::Content::EntitlementRef> entitlementsRemoved);
            void entitlementUpdated(Origin::Engine::Content::EntitlementRef entitlementRef);

            lsx::GameT extractContentInformation(Origin::Engine::Content::EntitlementRef entitlement);

            void avatarUpdateEvent(quint64 userId);
            void subscriptionUpdateEvent();
            void onOriginIdChanged (QByteArray payload); // Dirtybit message
            void onPasswordChanged (QByteArray payload); // Dirtybit message
            //void PostWincode( QString key, QString value, QString authcode, QString achievementSet, QString appId, QString productId = QString("origin"));
            void achievementGranted(Origin::Engine::Achievements::AchievementSetRef, Origin::Engine::Achievements::AchievementRef);

        private:
            void OnLogin(bool loggedIn);

            // block constructors
            SDK_ServiceArea( Lsx::LSX_Handler* handler, class XMPP_ServiceArea *xmpp_service);
            SDK_ServiceArea();

            //	Handlers
            void GetConfig(Lsx::Request& request, Lsx::Response& response );

            void GoOnline(Lsx::Request& request, Lsx::Response& response );
            void CheckPermission(Lsx::Request& request, Lsx::Response& response );
            void GetProfile(Lsx::Request& request, Lsx::Response& response );
            void GetEnvironment(Lsx::Request& request, Lsx::Response& response );
            void GetSetting(Lsx::Request& request, Lsx::Response& response );
            void GetGameInfo(Lsx::Request& request, Lsx::Response& response );
            void GetSettings(Lsx::Request& request, Lsx::Response& response );

            void GetAllGameInfo(Lsx::Request& request, Lsx::Response& response );
			void GetUTCTime(Lsx::Request& request, Lsx::Response& response );
            void ShowIGO(Lsx::Request& request, Lsx::Response& response );
            void ShowIGOWindow(Lsx::Request& request, Lsx::Response& response );

            void QueryContent(Lsx::Request& request, Lsx::Response& response );
            void RestartGame(Lsx::Request& request, Lsx::Response& response );
            void StartGame(Lsx::Request& request, Lsx::Response& response);
            void ExtendTrial(Lsx::Request& request, Lsx::Response& response);
            void SendGameMessage(Lsx::Request& request, Lsx::Response& response);
            bool ExtendTrialResponse(Origin::Services::Trials::TrialBurnTimeResponse *serverReply, Lsx::Response& response);

            void BroadcastStateChanged(int state);

            Origin::Services::AvatarServiceClient::ImageSizes getImageSize(int width);
            void QueryImageForUser(const lsx::QueryImageT &image, Lsx::Response& response );
            void QueryImageFromAchievement(const lsx::QueryImageT &image, Lsx::Response& response );
            bool DownloadFromUrlProcessResponse(lsx::ImageT &image, QNetworkReply *serverReply, Lsx::Response& response);

            void QueryImage(Lsx::Request& request, Lsx::Response& response );

            //int DownloadFromUrl( QString downloadUrl, QString cachePath, bool &bSuccess);

            void AddRecentPlayers(Lsx::Request& request, Lsx::Response& response );

            bool WriteRecentPlayers( struct lsx::rpT &recentPlayers );
            bool ReadRecentPlayers( struct lsx::rpT &recentPlayers );

            void QueryRecentPlayers(Lsx::Request& request, Lsx::Response& response );


            void DeleteOldCacheFiles();
            void GenerateCachePath(const QString &imageId, unsigned int width, unsigned int height, QString &cachePath, QString &downloadUrl);


            void GetUserProfileByEmailorEAID(Lsx::Request &request, Lsx::Response &response);

            void QueryBlockList(Lsx::Request &request, Lsx::Response &response);

            void CreateErrorSuccess( Lsx::Response &response, class QNetworkReply * resp );
            void CreateErrorSuccess( Lsx::Response &response, int code, QString description);

            struct GrantAchievementsPayload
            {
                lsx::GrantAchievementT ga;
                QString achievementSet;
                Origin::Engine::Content::EntitlementRef entitlement;
            };

            void GrantAchievement(Lsx::Request& request, Lsx::Response& response );
            bool GrantAchievementsProcessResponse(GrantAchievementsPayload & payload, QNetworkReply * serverReply, Lsx::Response& response);

            struct PostAchievementEventPayload
            {
                lsx::PostAchievementEventsT request;
                QString events;
                QString achievementSet;
                Origin::Engine::Content::EntitlementRef entitlement;
            };

            void PostAchievementEvents(Lsx::Request& request, Lsx::Response& response);
            bool PostAchievementEventsProcessResponse(PostAchievementEventPayload & payload, QNetworkReply * serverReply, Lsx::Response& response);

            //int PostWincodes( lsx::PostWincodesT &wincodes, QString achievementSet, QString productId, QString appId, QNetworkReply *&serverResponse );
            //void PostWincodes(Lsx::Request& request, Lsx::Response& response );

            struct QueryAchievementsPayload
            {
                lsx::AchievementSetsT * sets;
                QString achievementSet;
                QString gameName;

                ResponseWrapperEx<lsx::AchievementSetsT *, SDK_ServiceArea, int> * gatherWrapper;
            };

            void QueryAchievements(Lsx::Request& request, Lsx::Response& response );
            bool QueryAchievementsProcessResponse(QueryAchievementsPayload & payload, QNetworkReply * serverReply, Lsx::Response& response);
            template <typename T> bool GatherResponse(T *& data, int * jobsPending, Lsx::Response & response);

            bool IsIGOEnabled(Lsx::Request& request);
            bool IsIGOAvailable(Lsx::Request& request);
#ifdef ORIGIN_PC
            void BroadcastStart( Lsx::Request& request, Lsx::Response& response );
            void BroadcastStop( Lsx::Request& request, Lsx::Response& response );
            void GetBroadcastStatus( Lsx::Request& request, Lsx::Response& response );
#endif
        private:
            enum NetworkMethod
            {
                OPERATION_GET,
                OPERATION_PUT,
                OPERATION_POST,
                OPERATION_DELETE,
            };

            template <typename T> void DoServerCall(T & data, QNetworkRequest &serverRequest, NetworkMethod method, bool (SDK_ServiceArea::*callback)(T &, QNetworkReply * reply, Lsx::Response &response), Lsx::Response &response, const QByteArray &pDoc = QByteArray());

            void CreateServerErrorResponse(QNetworkReply * pReply, Lsx::Response& response);
            void CreateErrorResponse(int code, const char * description, Lsx::Response& response);

            QString cacheFolder(const QString &section) const;
            QString cacheFolder() const;

            void RequestUserImageUrl(ImageDetails & image, Lsx::Response& response);

        public:
            bool RequestUserImageUrlResponse(ImageDetails & image, Services::AvatarsByUserIdsResponse * resp, Lsx::Response &response);
            bool DownloadImageResponse(ImageDetails & image, QNetworkReply * resp, Lsx::Response &response);

            void MinimizeRequest();
            void RestoreRequest();

        private:

            class XMPP_ServiceArea * m_XMPP_service;

            QString m_AchievementServiceURL;
            QString m_trialContentId;
        };
    }
}

#endif
