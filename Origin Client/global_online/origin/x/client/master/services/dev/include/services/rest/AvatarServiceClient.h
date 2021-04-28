///////////////////////////////////////////////////////////////////////////////
// AvatarServiceClient.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _AVATARSERVICECLIENT_H_INCLUDED_
#define _AVATARSERVICECLIENT_H_INCLUDED_

#include "services/rest/OriginAuthServiceClient.h"
#include "services/rest/AvatarServiceResponse.h"
#include "services/rest/OriginServiceMaps.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API AvatarServiceClient :	public OriginAuthServiceClient
        {
        public:
            friend class OriginClientFactory<AvatarServiceClient>;

            ///////////////////////////
            // IMAGE SIZES
            enum ImageSize
            {
                Size_40X40 = 0x1
                ,Size_208X208 = 0x2
                ,Size_416X416 = 0x4
            };

            Q_DECLARE_FLAGS(ImageSizes, ImageSize)

            //////////////////////////////////////////////////////////////////////////
            /// Avatar
            
            ///
            /// \brief Retrieves all avatar types.
            ///
            static AvatarAllAvatarTypesResponse* allAvatarTypes()
            {
                return OriginClientFactory<AvatarServiceClient>::instance()->allAvatarTypesPriv();
            }

            /// 
            /// \brief Retrieves an existing avatar type's name.
            /// 
            static AvatarTypeResponse* existingAvatarType(quint64 typeId)
            {
                return OriginClientFactory<AvatarServiceClient>::instance()->existingAvatarTypePriv(typeId);
            }

            ///
            /// \brief Gets the result of whether the user's avatar is changed, true or false.
            ///
            /// \param session TBD.
            /// \param avType The avatar's ID.
            ///
            static AvatarBooleanResponse* userAvatarChanged(Session::SessionRef session, avatarType avType)
            {
                return OriginClientFactory<AvatarServiceClient>::instance()->userAvatarChangedPriv(session, avType);
            }

            /// \brief Gets an avatar by ID.
            /// 
            /// If the size parameter is not passed, the server will return a link to the smallest (40x40) image.
            /// Size can be 0, 1, or 2. 0 means 40x40, 1 means 208x208, 2 means 416x416. The client can get multiple
            /// sizes by calling one time, for example: size=0,1,2. The server will return links for those sizes and
            /// use a semi-colon to separate them.
            /// 
            /// \param session TBD.
            /// \param avatarId The avatar's ID.
            /// \param avatarSizes The avatar size.
            ///
            static AvatarDefaultAvatarResponse* avatarById(Session::SessionRef session, quint64 avatarId, ImageSizes avatarSizes = ImageSizes(Size_40X40))
            {
                return OriginClientFactory<AvatarServiceClient>::instance()->avatarByIdPriv(session, avatarId, avatarSizes);
            }
            
            /// \brief Gets avatars by gallery IDs.
            /// 
            /// If the size parameter is not passed, the server will return a link to the smallest (40x40) image. 
            /// The size can be 0, 1, or 2. 0 means 40x40, 1 means 208x208, 2 means 416x416. The client can get
            /// multiple sizes by calling one time, for example: size=0,1,2. The server will return those size
            /// links and use a semi-colon to separate them.
            ///
            /// \param session TBD.
            /// \param galleryId The gallery id.
            /// \param avatarSize The avatar size.
            /// \param avatarStatus The avatar size.
            ///
            static AvatarsByGalleryIdResponse* avatarsByGalleryId(Session::SessionRef session, quint64 galleryId, ImageSizes avatarSize = ImageSizes(Size_40X40), imageStatus avatarStatus = ImageStatusNoStatus)
            {
                return OriginClientFactory<AvatarServiceClient>::instance()->avatarsByGalleryIdPriv(session, galleryId, avatarSize, avatarStatus);
            }

            /// \brief Gets avatars by user IDs.
            /// 
            /// If the size parameter is not passed, the server will return a link to the smallest (40x40) image. 
            /// The size can be 0, 1, or 2. 0 means 40x40, 1 means 208x208, 2 means 416x416. The client can get
            /// multiple sizes by calling one time, for example: size=0,1,2. The server will return those size 
            /// links and use a semi-colon to separate them.
            ///
            /// \param session TBD.
            /// \param usersIds A collection of users' IDs, separated with semi-colons.
            /// \param avatarSize The avatar size.
            ///
            static AvatarsByUserIdsResponse* avatarsByUserIds(Session::SessionRef session, const QList<quint64>& usersIds, ImageSizes avatarSize = ImageSizes(Size_40X40))
            {
                return OriginClientFactory<AvatarServiceClient>::instance()->avatarsByUserIdsPriv(session, usersIds, avatarSize);
            }


            /// \brief Gets the default avatar.
            ///
            /// \param session TBD.
            /// \param avatarSize The avatar's size.
            ///
            static AvatarDefaultAvatarResponse* defaultAvatar(Session::SessionRef session, ImageSizes avatarSize = ImageSizes(Size_40X40))
            {
                return OriginClientFactory<AvatarServiceClient>::instance()->defaultAvatarPriv(session, avatarSize);
            }

            /// \brief Gets the recent avatar list by special condition.
            /// 
            /// If the size parameter is not passed, the server will return a link to the smallest (40x40) image.
            /// The size can be 0, 1, or 2. 0 means 40x40, 1 means 208x208, 2 means 416x416. The client can get
            /// multiple sizes by calling one time, for example: size=0,1,2. The server will return those size
            /// links and use a semi-colon to separate them.
            /// 
            /// \param session TBD.
            /// \param avatarSize the avatar's size.
            ///
            static AvatarGetRecentResponse* recentAvatarList(Session::SessionRef session, ImageSizes avatarSize = ImageSizes(Size_40X40))
            {
                return OriginClientFactory<AvatarServiceClient>::instance()->recentAvatarListPriv(session,avatarSize);
            }

            /// \brief Gets the supported dimensions in a XML object.
            ///
            /// \param session TBD.
            static AvatarSupportedDimensionsResponse* supportedDimensions(Session::SessionRef session)
            {
                return OriginClientFactory<AvatarServiceClient>::instance()->supportedDimensionsPriv(session);
            }

        private:
            ///
            /// \brief Retrieves all avatar types.
            ///
            AvatarAllAvatarTypesResponse* allAvatarTypesPriv();

            /// \brief Retrieves an existing avatar type's name.
            /// 
            AvatarTypeResponse* existingAvatarTypePriv(quint64 typeId);

            /// \brief Gets the result of whether the user's avatar is changed, true or false.
            ///
            /// \param session TBD.
            /// \param avType TBD.
            ///
            AvatarBooleanResponse* userAvatarChangedPriv(Session::SessionRef session, avatarType avType);

            /// \brief Gets an avatar by ID.
            /// 
            /// If the size parameter is not passed, the server will return a link to the smallest (40x40) image.
            /// Size can be 0, 1, or 2. 0 means 40x40, 1 means 208x208, 2 means 416x416. The client can get multiple
            /// sizes by calling one time, for example: size=0,1,2. The server will return links for those sizes and
            /// use a semi-colon to separate them.
            /// 
            /// \param session TBD.
            /// \param avatarId The avatar id.
            /// \param avatarSizes The avatar size.
            ///
            AvatarDefaultAvatarResponse* avatarByIdPriv(Session::SessionRef session, quint64 avatarId, ImageSizes avatarSizes = ImageSizes(Size_40X40));

            /// \brief Gets avatars by gallery IDs.
            /// 
            /// If the size parameter is not passed, the server will return a link to the smallest (40x40) image. 
            /// The size can be 0, 1, or 2. 0 means 40x40, 1 means 208x208, 2 means 416x416. The client can get
            /// multiple sizes by calling one time, for example: size=0,1,2. The server will return those size
            /// links and use a semi-colon to separate them.
            ///
            /// \param session TBD.
            /// \param galleryId The gallery id.
            /// \param avatarSize The avatar size.
            /// \param avatarStatus The avatar size.
            ///
            AvatarsByGalleryIdResponse* avatarsByGalleryIdPriv(Session::SessionRef session, quint64 galleryId, ImageSizes avatarSize = ImageSizes(Size_40X40), imageStatus avatarStatus = ImageStatusNoStatus);

            /// \brief Gets avatars by user IDs.
            /// 
            /// If the size parameter is not passed, the server will return a link to the smallest (40x40) image. 
            /// The size can be 0, 1, or 2. 0 means 40x40, 1 means 208x208, 2 means 416x416. The client can get multiple
            /// sizes by calling one time, for example: size=0,1,2. The server will return those size links and use a
            /// semi-colon to separate them.
            ///
            /// \param session TBD.
            /// \param usersIds A collection of users' IDs, separated with semi-colons.
            /// \param avatarSize The avatars' size.
            ///
            AvatarsByUserIdsResponse* avatarsByUserIdsPriv(Session::SessionRef session, const QList<quint64>& usersIds, ImageSizes avatarSize = ImageSizes(Size_40X40));

            /// \brief Get default avatar
            /// 
            /// \param session TBD.
            /// \param avatarSize The avatar's size.
            ///
            AvatarDefaultAvatarResponse* defaultAvatarPriv(Session::SessionRef session, ImageSizes avatarSize = ImageSizes(Size_40X40));

            /// \brief Gets the recent avatar list by special condition.
            /// 
            /// If the size parameter is not passed, the server will return a link to the smallest (40x40) image.
            /// The size can be 0, 1, or 2. 0 means 40x40, 1 means 208x208, 2 means 416x416. The client can get
            /// multiple sizes by calling one time, for example: size=0,1,2. The server will return those size
            /// links and use a semi-colon to separate them.
            /// 
            /// \param session TBD.
            /// \param avatarSize the avatar's size.
            ///
            AvatarGetRecentResponse* recentAvatarListPriv(Session::SessionRef session, ImageSizes avatarSize = ImageSizes(Size_40X40));

            /// \brief Gets the supported dimensions in a XML object.
            ///
            AvatarSupportedDimensionsResponse* supportedDimensionsPriv(Session::SessionRef session);
            
            /// \brief Creates a new avatar service client.
            /// 
            /// \param baseUrl The base url for the service (QUrl).
            /// \param nam     A QNetworkAccessManager instance to send requests through.
            ///
            explicit AvatarServiceClient(const QUrl &baseUrl = QUrl(), NetworkAccessManager *nam = NULL);

            ///
            /// \brief Accessor for size values.
            QString getSizeValue(ImageSizes avatarSizes);
        };

        Q_DECLARE_OPERATORS_FOR_FLAGS(AvatarServiceClient::ImageSizes)

        ///
        class ImageSizesMap;
        ORIGIN_PLUGIN_API const ImageSizesMap& imageSizes();

            /// Image status map
        class ORIGIN_PLUGIN_API ImageSizesMap : public PropertiesMap<ImageSizesMap, AvatarServiceClient::ImageSize>
        {
            ImageSizesMap()
            {
                imageSizes().add(AvatarServiceClient::Size_40X40,"40x40");
                imageSizes().add(AvatarServiceClient::Size_208X208,"208x208");
                imageSizes().add(AvatarServiceClient::Size_416X416,"416x416");
            }
            ///
            /// Friend function to access the singleton
            ///
            friend const ImageSizesMap& imageSizes();
        };
    }
}


#endif

