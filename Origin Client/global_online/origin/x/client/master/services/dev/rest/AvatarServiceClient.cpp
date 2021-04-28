///////////////////////////////////////////////////////////////////////////////
// AvatarServiceClient.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "services/rest/AvatarServiceClient.h"
#include "services/settings/SettingsManager.h"

// requires definition here
template <typename T, typename U, typename V>
QHash<U, V> PropertiesMap<T, U, V>::m_Hash;

// We consider zero to be an invalid nucleus ID.
static const quint64 INVALID_NUCLEUS_ID = 0;

namespace Origin
{
    namespace Services
    {
        AvatarServiceClient::AvatarServiceClient(const QUrl &baseUrl,NetworkAccessManager *nam)
            : OriginAuthServiceClient(nam)
        {
            if (!baseUrl.isEmpty())
                setBaseUrl(baseUrl);
            else
            {
                QString strUrl = Origin::Services::readSetting(Origin::Services::SETTING_AvatarURL); 
                setBaseUrl(QUrl(strUrl + "/avatar"));
            }
        }

        /// Types
        AvatarAllAvatarTypesResponse* AvatarServiceClient::allAvatarTypesPriv()
        {
            QUrl serviceUrl(urlForServicePath("avatartypes"));
            return new AvatarAllAvatarTypesResponse(getAuth(Session::SessionService::currentSession(), QNetworkRequest(serviceUrl)) );

        }

        AvatarTypeResponse* AvatarServiceClient::existingAvatarTypePriv( quint64 typeId )
        {
            QUrl serviceUrl(urlForServicePath( "avatartype/" + QString::number(typeId)));
            return new AvatarTypeResponse(getAuth(Session::SessionService::currentSession(), QNetworkRequest(serviceUrl)) );
        }

        AvatarBooleanResponse* AvatarServiceClient::userAvatarChangedPriv(Session::SessionRef session, avatarType avType)
        {
            QUrl serviceUrl(urlForServicePath("user/" + escapedNucleusId(session) +"/avatar/" + QString::number(avType)+"/ischanged"));
            return new AvatarBooleanResponse(getAuth(session, QNetworkRequest(serviceUrl)) );
        }

        QString AvatarServiceClient::getSizeValue(ImageSizes avatarSizes)
        {
            QString sizeValue;

            if(avatarSizes.testFlag(Size_40X40))
            {
                sizeValue += "0,";
            }

            if(avatarSizes.testFlag(Size_208X208))
            {
                sizeValue += "1,";
            }

            if(avatarSizes.testFlag(Size_416X416))
            {
                sizeValue += "2,";
            }

            if(sizeValue.size() == 0)
            {
                return "0";
            }
            else // chop off the last ,
            {
                return sizeValue.left(sizeValue.size()-1);
            }
        }

        AvatarDefaultAvatarResponse* AvatarServiceClient::avatarByIdPriv(Session::SessionRef session,  quint64 avatarId, ImageSizes avatarSizes )
        {
            QUrl serviceUrl(urlForServicePath("AvatarInfo_/" + QString::number(avatarId)));
            QUrlQuery serviceUrlQuery(serviceUrl);
            serviceUrlQuery.addQueryItem("size", getSizeValue(avatarSizes));
            serviceUrl.setQuery(serviceUrlQuery);
            
            QNetworkRequest req(serviceUrl);
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new AvatarDefaultAvatarResponse(getAuth(session, req) );
        }

        AvatarsByGalleryIdResponse* AvatarServiceClient::avatarsByGalleryIdPriv(Session::SessionRef session, quint64 galleryId, ImageSizes avatarSizes, imageStatus avatarStatus )
        {
            QUrl serviceUrl(urlForServicePath("gallery/" + QString::number(galleryId) + "/avatars"));
            QUrlQuery serviceUrlQuery(serviceUrl);
            serviceUrlQuery.addQueryItem("size", getSizeValue(avatarSizes));

            if (ImageStatusNoStatus != avatarStatus)
                serviceUrlQuery.addQueryItem("status", ImageStatus().name(avatarStatus));

            serviceUrl.setQuery(serviceUrlQuery);
            
            QNetworkRequest req(serviceUrl);
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new AvatarsByGalleryIdResponse(getAuth(session, req));

        }

        AvatarsByUserIdsResponse* AvatarServiceClient::avatarsByUserIdsPriv(Session::SessionRef session, const QList<quint64>& usersIds, ImageSizes avatarSizes )
        {
            QByteArray users;
            foreach(quint64 id, usersIds)
            {
                // Filter out any invalid nucleus IDs before making the request
                if (id == INVALID_NUCLEUS_ID)
                {
                    continue;
                }

                users += QString::number(id) + ";";
            }

            // User list could be empty if either an empty list was passed to begin
            // with or the user list only contained invalid nucleus IDs (which have
            // been filtered out).
            if (users.isEmpty())
            {
                return NULL;
            }

            users.resize(users.size()-1);

            QUrl serviceUrl(urlForServicePath("user/" + users + "/avatars"));
            QUrlQuery serviceUrlQuery(serviceUrl);
            serviceUrlQuery.addQueryItem("size", getSizeValue(avatarSizes));
            serviceUrl.setQuery(serviceUrlQuery);
            
            QNetworkRequest req(serviceUrl);
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new AvatarsByUserIdsResponse(getAuth(session, req), usersIds);
        }

        /// Other
        AvatarDefaultAvatarResponse* AvatarServiceClient::defaultAvatarPriv(Session::SessionRef session, ImageSizes avatarSizes)
        {
            QUrl serviceUrl(urlForServicePath("defaultavatar"));
            QUrlQuery serviceUrlQuery(serviceUrl);
            serviceUrlQuery.addQueryItem("size", getSizeValue(avatarSizes));
            serviceUrl.setQuery(serviceUrlQuery);
            
            QNetworkRequest req(serviceUrl);
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new AvatarDefaultAvatarResponse(getAuth(session, req));
        }

        AvatarGetRecentResponse* AvatarServiceClient::recentAvatarListPriv(Session::SessionRef session, ImageSizes avatarSizes)
        {
            QUrl serviceUrl(urlForServicePath("recentavatars"));
            QUrlQuery serviceUrlQuery(serviceUrl);
            serviceUrlQuery.addQueryItem("size", getSizeValue(avatarSizes));
            serviceUrl.setQuery(serviceUrlQuery);
            
            QNetworkRequest req(serviceUrl);
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new AvatarGetRecentResponse(getAuth(session, req));
        }

        AvatarSupportedDimensionsResponse* AvatarServiceClient::supportedDimensionsPriv(Session::SessionRef session)
        {
            QUrl serviceUrl(urlForServicePath("dimensions"));
            QNetworkRequest req(serviceUrl);
            req.setRawHeader("AuthToken", sessionToken(session).toUtf8());
            return new AvatarSupportedDimensionsResponse(getAuth(session, req) );
        }

        ///
        /// Singleton to access the image status names map
        ///
        const ImageSizesMap& imageSizes()
        {
            static ImageSizesMap st;
            return st;
        }

    }
}


