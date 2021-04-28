///////////////////////////////////////////////////////////////////////////////
// OriginServiceMaps.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef __GCS_SERVICE_MAPS_H_INCLUDED_
#define __GCS_SERVICE_MAPS_H_INCLUDED_

#include "PropertiesMap.h"
#include "OriginServiceValues.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
	namespace Services
	{

        ORIGIN_PLUGIN_API QString getLoginErrorString(restError error);

		using namespace ApiValues;

		class ORIGIN_PLUGIN_API VisibilityMap : public PropertiesMap<VisibilityMap, visibility, QString>
		{
			///
			/// Friend function to access the visibility settings map
			///
			friend ORIGIN_PLUGIN_API const VisibilityMap& Visibility();
			///
			///
			///
			VisibilityMap() {}
            
            void initialize()
			{
				const VisibilityMap& Visibility();

				// Build our visibility setting hash
				Visibility().add(visibilityEveryone,"Everyone");
				Visibility().add(visibilityFriendsOfFriends,"FriendsOfFriends");
				Visibility().add(visibilityFriends,"Friends");
				Visibility().add(visibilityNoOne,"NoOne");
			}
		};

		/// Image types status map
		class ORIGIN_PLUGIN_API ImageTypesMap : public PropertiesMap<ImageTypesMap, imageType>
		{
			ImageTypesMap() {}
            
            void initialize()
			{
				const ImageTypesMap& imageTypes();

				imageTypes().add(ImageTypeJpeg,"JPEG");
				imageTypes().add(ImageTypeBmp,"BMP");
				imageTypes().add(ImageTypePng,"PNG");
				imageTypes().add(ImageTypeGif,"GIF");
			}
			///
			/// Friend function to access the singleton
			///
			friend ORIGIN_PLUGIN_API const ImageTypesMap& imageTypes();
		};

		/// Image status map
		class ORIGIN_PLUGIN_API ImageStatusMap : public PropertiesMap<ImageStatusMap, imageStatus>
		{

			ImageStatusMap() {}
            
            void initialize()
			{
				const ImageStatusMap& ImageStatus();

				ImageStatus().add(ImageStatusEditing,"editing");
				ImageStatus().add(ImageStatusApproved,"approved");
				ImageStatus().add(ImageStatusAll,"all");
			}
			///
			/// Friend function to access the singleton
			///
			friend ORIGIN_PLUGIN_API const ImageStatusMap& ImageStatus();
		};
		class ORIGIN_PLUGIN_API AvatarTypesMap : public PropertiesMap<AvatarTypesMap,avatarType>
		{
			AvatarTypesMap() {}
            
            void initialize()
			{
				const AvatarTypesMap& avatarTypes();

				avatarTypes().add(AvatarTypeNormal,"normal");
				avatarTypes().add(AvatarTypeOfficial,"official");
				avatarTypes().add(AvatarTypePremium,"premium");
				avatarTypes().add(AvatarTypeElite,"elite");
			}
			///
			/// Friend function to access the singleton
			///
			friend ORIGIN_PLUGIN_API const AvatarTypesMap& avatarTypes();
		};
		class ORIGIN_PLUGIN_API PrivacySearchOptionsMap 
			: public PropertiesMap<PrivacySearchOptionsMap,searchOptionType, QString>
		{
			PrivacySearchOptionsMap() {}
            
            void initialize()
			{
				const PrivacySearchOptionsMap& SearchOptions();

				SearchOptions().add(SearchOptionTypeXbox,	"XBOX" );
				SearchOptions().add(SearchOptionTypePS3,	"PS3" );
				SearchOptions().add(SearchOptionTypeFaceBook,"FACEBOOK" );
				SearchOptions().add(SearchOptionTypeFullName,"FullName" );
				SearchOptions().add(SearchOptionTypeGamerName,"GamerName" );
				SearchOptions().add(SearchOptionTypeEmail,	"Email" );
			}
			friend ORIGIN_PLUGIN_API const PrivacySearchOptionsMap& SearchOptions();
		};

		class ORIGIN_PLUGIN_API FriendsProfileMap : public PropertiesMap<FriendsProfileMap, QString, bool>
		{
            FriendsProfileMap() {}
            
            void initialize() {}
            
			///
			/// Friend function to access the singleton
			///2
			friend ORIGIN_PLUGIN_API const FriendsProfileMap& friendsProfiles();
		};

		class ORIGIN_PLUGIN_API PrivacySettingMap : public PropertiesMap<PrivacySettingMap, privacySettingCategory, QByteArray>
		{
			///
			/// Friend function to access the visibility settings map
			///
			friend ORIGIN_PLUGIN_API const PrivacySettingMap& privSettingsMap();
			///
			///
			///
			PrivacySettingMap() {}
            
            void initialize()
			{
				const PrivacySettingMap& privSettingsMap();

				// Build our visibility setting hash
				privSettingsMap().add(PrivacySettingCategoryAll,"");
				privSettingsMap().add(PrivacySettingCategoryGeneral,"GENERAL");
				privSettingsMap().add(PrivacySettingCategoryAccount,"ACCOUNT");
				privSettingsMap().add(PrivacySettingCategoryNotification,"NOTIFICATION");
				privSettingsMap().add(PrivacySettingCategoryPrivacy,"PRIVACY");
                privSettingsMap().add(PrivacySettingCategoryInGame,"INGAME");
				privSettingsMap().add(PrivacySettingCategoryHiddenGames,"HIDDENGAMES");
                privSettingsMap().add(PrivacySettingCategoryFavoriteGames, "FAVORITEGAMES");
                privSettingsMap().add(PrivacySettingCategoryTelemetryOptOut,"TELEMETRYOPTOUT");
                privSettingsMap().add(PrivacySettingCategoryEnableIGO,"ENABLEIGO");
                privSettingsMap().add(PrivacySettingCategoryEnableCloudSaving,"ENABLECLOUDSAVING");
                privSettingsMap().add(PrivacySettingCategoryBroadcasting,"BROADCASTING");
			}
		};

        class ORIGIN_PLUGIN_API ProfileResponseTypeMap 
			: public PropertiesMap<ProfileResponseTypeMap,profileResponseType, QString>
		{
			ProfileResponseTypeMap() {}
            
            void initialize()
			{
				const ProfileResponseTypeMap& profileResponseTypes();

				profileResponseTypes().add(ProfileResponseTypeFull,	"full" );
				profileResponseTypes().add(ProfileResponseTypeMin,	"min" );
			}
			friend ORIGIN_PLUGIN_API const ProfileResponseTypeMap& profileResponseTypes();
		};

		/// Http verb map
		class ORIGIN_PLUGIN_API HttpVerbMap : public PropertiesMap<HttpVerbMap, HttpVerb>
		{
			HttpVerbMap() {}
            
            void initialize()
			{
				const HttpVerbMap& httpVerbMap();

				httpVerbMap().add(HttpInvalid,"INVALID");
				httpVerbMap().add(HttpGet,"GET");
				httpVerbMap().add(HttpPut,"PUT");
				httpVerbMap().add(HttpPost,"POST");
				httpVerbMap().add(HttpDelete,"DELETE");
			}
			///
			/// Friend function to access the singleton
			///
			friend ORIGIN_PLUGIN_API const HttpVerbMap& httpVerbMap();
		};        
        
   		ORIGIN_PLUGIN_API const VisibilityMap& Visibility();
		ORIGIN_PLUGIN_API const ImageTypesMap& imageTypes();
		ORIGIN_PLUGIN_API const ImageStatusMap& ImageStatus();
		ORIGIN_PLUGIN_API const AvatarTypesMap& avatarTypes();
		ORIGIN_PLUGIN_API const PrivacySearchOptionsMap& SearchOptions();
		ORIGIN_PLUGIN_API const FriendsProfileMap& friendsProfiles();
		ORIGIN_PLUGIN_API const PrivacySettingMap& privSettingsMap();
		ORIGIN_PLUGIN_API const ProfileResponseTypeMap& profileResponseTypes();
		ORIGIN_PLUGIN_API const HttpVerbMap& httpVerbMap();

	}
}


#endif
