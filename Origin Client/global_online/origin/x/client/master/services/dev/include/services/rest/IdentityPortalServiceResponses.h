///////////////////////////////////////////////////////////////////////////////
// IdentityPortalServiceResponses.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _IDENTITYPORTALSERVICERESPONSES_H_INCLUDED_
#define _IDENTITYPORTALSERVICERESPONSES_H_INCLUDED_

#include "OriginServiceResponse.h"
//#include "OriginServiceMaps.h"

#include <QMap>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API IdentityPortalPersonasResponse : public OriginServiceResponse
        {
        public:
            explicit IdentityPortalPersonasResponse(QNetworkReply* reply);

            const QString personaUri() const {return mPersonaUri;}

        protected:

            bool parseSuccessBody(QIODevice* body);

            QString mPersonaUri;

        };

        class ORIGIN_PLUGIN_API IdentityPortalPersonasResponse2 : public OriginServiceResponse
        {
        public:
            explicit IdentityPortalPersonasResponse2(QNetworkReply* reply);

            /// \fn setUserInfo
            /// \brief sets local pointer to map.
            void setUserInfo(QMap<QString, QString>* userInfo) { mUserInfo = userInfo; }


        protected:

            /// \brief pointer to map to fillup with data
            QMap<QString, QString>* mUserInfo;

            bool parseSuccessBody(QIODevice* body);
        };

        class ORIGIN_PLUGIN_API IdentityPortalExpandedPersonasResponse : public OriginServiceResponse
        {
        public:
            explicit IdentityPortalExpandedPersonasResponse(QNetworkReply* reply);

            const QList<QVariantMap>& personaList() const;
            QString originId() const;
            qlonglong personaId() const;

        protected:

            QList<QVariantMap> mPersonaList;

            bool parseSuccessBody(QIODevice* body);
        };

        class ORIGIN_PLUGIN_API IdentityPortalUserResponse : public OriginServiceResponse
        {
            public:

                explicit IdentityPortalUserResponse(QNetworkReply* reply);
                const QMap<QString, QString>& userInfo() const { return mUserInfo; }

                QString pidId() const;
                QString email() const;
                QString emailStatus() const;
                QString strength() const;
                QString dob() const;
                QString country() const;
                QString language() const;
                QString locale() const;
                QString status() const;
                QString reasonCode() const;
                QString tosVersion() const;
                QString parentalEmail() const;
                QString thirdPartyOptin() const;
                QString globalOptin() const;
                QString dateCreated() const;
                QString dateModified() const;
                QString lastAuthDate() const;
                QString registrationSource() const;
                QString authenticationSource() const;
                QString showEmail() const;
                QString discoverableEmail() const;
                QString anonymousPid() const;
                QString underagePid() const;
                QString defaultBillingAddressUri() const;
                QString defaultShippingAddressUri() const;

            protected:

                QString token(const QString& token_name) const;

                QMap<QString, QString> mUserInfo;

                bool parseSuccessBody(QIODevice* body);

        };

        class ORIGIN_PLUGIN_API IdentityPortalUserProfileUrlsResponse : public OriginServiceResponse
        {
            public:

                enum ProfileInfoCategory
                {
                    All = 0,
                    MobileInfo,
                    Name,
                    Address,
                    SecurityQA,
                    Gender,
                    BillingAddress,
                    ShippingAddress,
                    UserDefaults,
                    SafeChildInfo
                };

                static QMap<ProfileInfoCategory, QString> sProfileInfoCategoryToStringMap;

                explicit IdentityPortalUserProfileUrlsResponse(QNetworkReply* reply);
                const QMap<ProfileInfoCategory, QStringList>& profileInfo() const { return mProfileUrls; }

            protected:

                QMap<ProfileInfoCategory, QStringList> mProfileUrls;

                bool parseSuccessBody(QIODevice* body);
                void parseUrlArray(const QString& jsonArray);
        };

        class ORIGIN_PLUGIN_API IdentityPortalUserNameResponse : public OriginServiceResponse
        {
            public:
                explicit IdentityPortalUserNameResponse(QNetworkReply* reply);

                QString firstName() const { return mFirstName; }
                QString lastName() const { return mLastName; }

            protected:

                bool parseSuccessBody(QIODevice* body);
                QString mFirstName;
                QString mLastName;
        };

        class ORIGIN_PLUGIN_API IdentityPortalExpandedNameProfilesResponse : public OriginServiceResponse
        {
        public:
            explicit IdentityPortalExpandedNameProfilesResponse(QNetworkReply* reply);

            const QList<QVariantMap>& profileList() const;
            QString firstName() const;
            QString lastName() const;

        protected:

            QList<QVariantMap> mProfileList;

            bool parseSuccessBody(QIODevice* body);
        };

        class ORIGIN_PLUGIN_API IdentityPortalUserIdResponse  : public OriginServiceResponse
		{
		    public:
			    explicit IdentityPortalUserIdResponse(QNetworkReply* reply);

			    qint64 userId() const { return mPid; }

		    protected:

			    bool parseSuccessBody(QIODevice *body);
			    qint64 mPid;
		};
    }
}

#endif


