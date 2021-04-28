///////////////////////////////////////////////////////////////////////////////
// AtomServiceResponses.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _ATOMSERVICESRESPONSES_H
#define _ATOMSERVICESRESPONSES_H

#include "OriginAuthServiceResponse.h"
#include "AtomServiceClient.h"
#include "OriginServiceMaps.h"
#include "AuthenticationData.h"

#include <QVector>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        struct UserGameData
        {
            QString productId;
            QString displayProductName;
            QString achievementSetId;
            QString cdnAssetRoot;
            QString imageServer;
            QString backgroundImage;
            QString packArtSmall;
            QString packArtMedium;
            QString packArtLarge;
        };

        struct UserFriendData
        {
            quint64 userId;
            quint64 personaId;
            QString EAID;
            QString firstName;
            QString lastName;
        };

        class ORIGIN_PLUGIN_API PrivacyGetSettingResponse  : public OriginAuthServiceResponse
        {
        public:
            /// \brief A struct to mimic the following response:
            /// \verbatim   <privacySetting>
            ///         <userId>123456</userId>
            ///         <category>GENERAL</category>
            ///         <payload>sample general</payload>
            ///     </privacySetting> \endverbatim
            struct UserPrivacySetting
            {
                quint64 userId;
                QString category;
                QString payload;
                UserPrivacySetting() : userId (0){}

            };
            explicit PrivacyGetSettingResponse(AuthNetworkRequest);

            /// \brief If found, returns privacy settings per userId; otherwise, it returns an empty list.
            /// \return A struct containing the privacy settings for the user.
            ///
            const QList<UserPrivacySetting> privacySettings() const
            {
                return m_privacySettings;
            }
        private:

            /// \brief Parser the 'ALL' settings responses.
            bool parseSuccessBody(QIODevice *body);

            /// \brief Parser for the single setting responses.
            bool parseSuccessBodySingle( QIODevice *body );
            QList<UserPrivacySetting> m_privacySettings;
        };

        class ORIGIN_PLUGIN_API UserResponse : public OriginAuthServiceResponse
        {
            public:

                explicit UserResponse(AuthNetworkRequest);

                /// \brief Gives access to retrieved user data.
                const User& user() const {return mUser;}

                /// \brief Gives access to retrieved users data.
                const QList<User>& users() const {return mUsers;}

            protected:

                bool parseSuccessBody(QIODevice* body);

                /// \brief User settings.
                User mUser;
                /// \brief If more than one user is returned, mUsers contains the settings for all the users.
                QList<User> mUsers;
        };

        class ORIGIN_PLUGIN_API AppSettingsResponse : public OriginServiceResponse
        {
        public:

            explicit AppSettingsResponse(QNetworkReply * reply);

            /// \brief gives access to retrieved user data
            const AppSettings& appSettings() const {return mUserSettings;}

        protected:

            bool parseSuccessBody(QIODevice* body);

            /// \brief User settings vector.
            AppSettings mUserSettings;

        };

        /// \brief Base class for search options.
        class ORIGIN_PLUGIN_API SearchOptionsResponseBase : public OriginAuthServiceResponse
        {
        public:

            /// \brief Gives access to retrieved user data.
            const QStringList& values() const {return mValues;}

        protected:

            /// \brief Constructor.
            explicit SearchOptionsResponseBase(AuthNetworkRequest);

            bool parseSuccessBody(QIODevice* body);

            /// \brief Search values vector.
            QStringList mValues;

        };


        class ORIGIN_PLUGIN_API SearchOptionsResponse : public SearchOptionsResponseBase
        {
        public:

            explicit SearchOptionsResponse(AuthNetworkRequest);

        };

        class ORIGIN_PLUGIN_API SetSearchOptionsResponse : public SearchOptionsResponseBase
        {
        public:

            explicit SetSearchOptionsResponse(AuthNetworkRequest);

        };


        class ORIGIN_PLUGIN_API UserGamesResponseBase : public OriginAuthServiceResponse 
        {
        public:

            virtual QList<UserGameData> userGames() const;

            virtual ~UserGamesResponseBase() = 0;

        protected:
            explicit UserGamesResponseBase(AuthNetworkRequest);

            QList<UserGameData> mUserGames;

        private:
            virtual bool parseSuccessBody(QIODevice* body);

        };

        class ORIGIN_PLUGIN_API UserGamesResponse : public UserGamesResponseBase 
        {
        public:
            explicit UserGamesResponse(AuthNetworkRequest);

        private:
            bool parseSuccessBody(QIODevice* body);

        };

        class ORIGIN_PLUGIN_API UserFriendsResponse : public OriginAuthServiceResponse 
        {
        public:
            explicit UserFriendsResponse(AuthNetworkRequest);

            QList<UserFriendData> userFriends() const { return mUserFriends; }

        private:
            bool parseSuccessBody(QIODevice* body);

            QList<UserFriendData> mUserFriends;
        };

        class ORIGIN_PLUGIN_API ShareAchievementsResponse : public OriginAuthServiceResponse 
        {
        public:
            explicit ShareAchievementsResponse(AuthNetworkRequest);

            bool isShareAchievements() const;
            QString userId() const;

        private:
            bool parseSuccessBody(QIODevice* body);

            bool mIsShareAchievements;

            QString mUserId;
        };


    }
}



#endif
