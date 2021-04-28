///////////////////////////////////////////////////////////////////////////////
// OriginServiceMaps.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "services/rest/OriginServiceMaps.h"
#include <QString>

namespace Origin
{
	namespace Services
	{
		///
		/// Singleton to access the visibility map
		///
		const VisibilityMap& Visibility()
		{
			static VisibilityMap vm;
            static bool initialized = false;
            if ( ! initialized ) { initialized = true; vm.initialize(); }
			return vm;
		}

		///
		/// Singleton to access the image status names map
		///
		const ImageTypesMap& imageTypes()
		{
			static ImageTypesMap st;
            static bool initialized = false;
            if ( ! initialized ) { initialized = true; st.initialize(); }
			return st;
		}
		///
		/// Singleton to access the image status names map
		///
		const ImageStatusMap& ImageStatus()
		{
			static ImageStatusMap st;
            static bool initialized = false;
            if ( ! initialized ) { initialized = true; st.initialize(); }
			return st;
		}
		///
		/// Singleton to access the Avatar type names map
		///
		const AvatarTypesMap& avatarTypes()
		{
			static AvatarTypesMap ata;
            static bool initialized = false;
            if ( ! initialized ) { initialized = true; ata.initialize(); }
			return ata;
		}

		///
		/// Singleton to access the friend's profiles map
		///
		const PrivacySearchOptionsMap& SearchOptions()
		{
			static PrivacySearchOptionsMap som;
            static bool initialized = false;
            if ( ! initialized ) { initialized = true; som.initialize(); }
			return som;
		}

		///
		/// Singleton to access the friend's profiles map
		///
		const FriendsProfileMap& friendsProfiles()
		{
			static FriendsProfileMap ata;
            static bool initialized = false;
            if ( ! initialized ) { initialized = true; ata.initialize(); }
			return ata;
		}

		///
		/// Singleton to access the visibility map
		///
		const PrivacySettingMap& privSettingsMap()
		{
			static PrivacySettingMap psm;
            static bool initialized = false;
            if ( ! initialized ) { initialized = true; psm.initialize(); }
			return psm;
		}

		///
		/// Singleton to access the friend's profiles map
		///
		const ProfileResponseTypeMap& profileResponseTypes()
		{
			static ProfileResponseTypeMap prt;
            static bool initialized = false;
            if ( ! initialized ) { initialized = true; prt.initialize(); }
			return prt;
		}

		///
		/// Singleton to access map
		///
		const HttpVerbMap& httpVerbMap()
		{
			static HttpVerbMap st;
            static bool initialized = false;
            if ( ! initialized ) { initialized = true; st.initialize(); }
			return st;
		}

        ///
        /// \brief Returns string name of given REST error.
        ///
        /// \note Even though this is in OriginServiceMaps, this function 
        ///       isn't technically implemented as a map, but it does map 
        ///       REST error codes to their string equivalent, albeit with a
        ///       big ass switch statement.
        ///
        QString getLoginErrorString(restError error)
        {
            QString errorString;
            switch (error)
            {
                #define RESTLOGINERROR(enumid, enumvalue, errorstring)\
                    case enumid: errorString = errorstring; break;
                    RESTLOGINERRORS
                #undef  RESTLOGINERROR

                default: errorString = QString("ERROR_%1").arg(error);
            }
            return errorString;
        }

	}
}

