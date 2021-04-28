///////////////////////////////////////////////////////////////////////////////
// AtomServiceClient.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _ATOMSERVICECLIENT_H
#define _ATOMSERVICECLIENT_H

#include "RequestClientBase.h"
#include "OriginServiceMaps.h"
#include <QVector>
#include <QByteArray>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {

        class AppSettingsResponse;
        class UserResponse;
        class SearchOptionsResponse;
        class SetSearchOptionsResponse;
        class PrivacyGetSettingResponse;
        class OriginServiceResponse;
        class OriginAuthServiceResponse;
        class UserGamesResponse;
        class UserFriendsResponse;
        class ShareAchievementsResponse;

        class ORIGIN_PLUGIN_API AtomServiceClient : public RequestClientBase
        {
        public:
            friend class OriginClientFactory<AtomServiceClient>;

            /// \brief Type of content related to a reported user
            enum ReportContentType
            {
                /// \brief Origin ID or Real Name
                NameContent,
                /// \brief User avatar
                AvatarContent,
                /// \brief Custom non-Origin game name
                CustomGameNameContent,
                /// \brief Custom chat room name
                RoomNameContent,
                /// \brief In-game
                InGame
            };

            /// \brief Reason for reporting a user
            enum ReportReason
            {
                ChildSolicitationReason,
                TerroristThreatReason,
                ClientHackReason,
                HarassmentReason,
                SpamReason,
                CheatingReason,
                OffensiveContentReason,
                SuicideThreatReason
            };

            /// \brief returns user's settings
            /// \param session The user session.
            static AppSettingsResponse* appSettings(Session::SessionRef session);

            /// \brief returns user's data
            /// \param session The user session.
            static UserResponse* user(Session::SessionRef session);

            /// \brief returns user's data
            /// \param session The user session.
            /// \param nucleusId TBD.
            static UserResponse* user(Session::SessionRef session, quint64 nucleusId);

            /// \brief returns a list of UserResponse objects - each may contain profiles for up to 5 users.
            /// \param session The user session.
            /// \param nucleusIdList The list of nucleus Id's.
            static QList<UserResponse*> user(Session::SessionRef session, const QList<quint64> &nucleusIdList);

            /// \brief returns user's search options
            /// \param session The user session.
            static SearchOptionsResponse* searchOptions(Session::SessionRef session);

            /// \brief Sets user search options.
            /// \param session The user session.
            /// \param mySearchOptions The search options.
            static SetSearchOptionsResponse* setSearchOptions(Session::SessionRef session, QVector<QString>& mySearchOptions);

            /// \brief Get privacy setting By user ID and category. If category is 'ALL', it will get all privacy settings by user ID.
            /// \param session The user session.
            /// \param privacysetting Privacy Setting
            static PrivacyGetSettingResponse* privacySetting(Session::SessionRef session, privacySettingCategory privacysetting);

            /// \brief Set privacy setting By user ID and category to the provided payload.
            /// \param session The user session.
            /// \param myPrivacySetting The privacy setting.
            /// \param payload TBD.
            static OriginServiceResponse* setPrivacySetting(Session::SessionRef session, privacySettingCategory myPrivacySetting, const QString& payload );

            /// \brief Retrieve the list of games a user owns
            /// \param session    The user session.
            /// \param nucleusId  The user's Nucleus ID
            static UserGamesResponse* userGames(Session::SessionRef session, quint64 nucleusId);
            
            /// \brief Retrieve the list of friends of a user
            /// \param session    The user session.
            /// \param nucleusId  The user's Nucleus ID
            static UserFriendsResponse* userFriends(Session::SessionRef session, quint64 nucleusId);
            
            /// \brief Retrieve the list of friends of a user
            /// \param session      The user session.
            /// \param nucleusId    The user's Nucleus ID
            /// \param contentType  The type of content causing the report
            /// \param reason The reason for the report
            static OriginAuthServiceResponse* reportUser(Session::SessionRef session, quint64 nucleusId, ReportContentType contentType, ReportReason reason, QString masterTitle, QString comments);

            /// \brief Retrieve whether an user is sharing achievements or not
            /// \param session    The user session.
            /// \param nucleusId  The user's Nucleus ID
            static ShareAchievementsResponse* shareAchievements(Session::SessionRef session, quint64 nucleusId);

        private:

            /// \brief Creates a new Atom service client
            /// \param baseUrl       Base URL for the friends service API
            /// \param nam           QNetworkAccessManager instance to send requests through
            explicit AtomServiceClient(const QUrl &baseUrl = QUrl(),NetworkAccessManager *nam = NULL);

            // Privacy
            QNetworkRequest privacyRequest(Session::SessionRef session ,privacySettingCategory privacySetting);
            PrivacyGetSettingResponse* privacySettingPriv(Session::SessionRef session, privacySettingCategory privacysetting);
            OriginServiceResponse* setPrivacySettingPriv(Session::SessionRef session, privacySettingCategory privacySetting, const QString& payload );
            // App settings
            AppSettingsResponse* appSettingsPriv(Session::SessionRef session);
            // Report user
            OriginAuthServiceResponse* reportUserPriv(Session::SessionRef session, quint64 nucleusId, ReportContentType contentType, ReportReason reason, QString masterTitle, QString comments);
        };
    }
}

#endif
