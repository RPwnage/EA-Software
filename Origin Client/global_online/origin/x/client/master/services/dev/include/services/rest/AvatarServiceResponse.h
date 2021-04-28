///////////////////////////////////////////////////////////////////////////////
// AvatarServiceResponse.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _AVATARSERVICERESPONSE_H_INCLUDED_
#define _AVATARSERVICERESPONSE_H_INCLUDED_

#include "OriginAuthServiceResponse.h"
#include "OriginServiceMaps.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
	namespace Services
	{
		///
		/// Avatar information container 
		///
		/// Information example:
		/// \verbatim <avatar>
		/// 	<avatarId>599</avatarId>
		/// 	<orderNumber>2</orderNumber>
		/// 	<link>http://ea-eadm-avatar-prod.s3.amazonaws.com/prod/1/599/40x40.JPEG</link>
		///		<isRecent>false</isRecent>
		/// 	<statusName>approved</statusName>
		/// 	<statusId>2</statusId>
		/// 	<typeName>premium</typeName>
		/// 	<typeId>3</typeId>
		/// 	<galleryId>1</galleryId>
		/// 	<galleryName>default</galleryName>
		/// </avatar> \endverbatim
		struct AvatarInformation
		{
			quint64 avatarId;
			quint64 orderNumber;
			QString link;
			bool isRecent;
			QString statusName;
			imageStatus statusId;
			QString typeName;
			avatarType typeId;
			quint64 galleryId;
			QString galleryName;
			AvatarInformation() : avatarId(0),orderNumber(0),isRecent(false),statusId(ImageStatusNoStatus),typeId(AvatarTypeNoType),galleryId(0){}
		};

		class ORIGIN_PLUGIN_API AvatarBooleanResponse : public OriginAuthServiceResponse
		{
		public:
			explicit AvatarBooleanResponse(AuthNetworkRequest reply);
			bool reponseValue() const {return m_bResult;}
		protected:
			bool parseSuccessBody(QIODevice *body);
			bool m_bResult;
		};

		class ORIGIN_PLUGIN_API AvatarDefaultAvatarResponse : public OriginAuthServiceResponse
		{
		public:
			explicit AvatarDefaultAvatarResponse(AuthNetworkRequest);

			/// 
			/// Returns the avatar info data
			/// 
			const AvatarInformation& avatarInfoData() const { return m_defaultAvatarInfo;}

			///
			/// DTOR
			///
			virtual ~AvatarDefaultAvatarResponse() {}
		protected:
			bool parseSuccessBody(QIODevice *body);
			AvatarInformation m_defaultAvatarInfo;
		};

		class ORIGIN_PLUGIN_API AvatarsByGalleryIdResponse : public AvatarDefaultAvatarResponse
		{
		public:
			explicit AvatarsByGalleryIdResponse(AuthNetworkRequest reply);
			const QHash<quint64, AvatarInformation>& constAvatarInfo() const {return m_Hash;}
		protected:
			bool parseSuccessBody(QIODevice *body);
			QHash<quint64, AvatarInformation> m_Hash;
		};

		struct UserAvatarsInfo
		{
			quint64 userId;
			AvatarInformation info;
			UserAvatarsInfo() : userId(0) {}
		};

		class ORIGIN_PLUGIN_API AvatarsByUserIdsResponse : public AvatarDefaultAvatarResponse
		{
		public:
			explicit AvatarsByUserIdsResponse(AuthNetworkRequest, const QList<quint64> &requestedUserIds);
			const QHash<quint64, UserAvatarsInfo> constAvatarInfo() const {return m_Hash;}
			QList<quint64> requestedUserIds() const { return m_requestedUserIds; }

		protected:
			bool parseSuccessBody(QIODevice *body);
			QHash<quint64, UserAvatarsInfo> m_Hash;
			QList<quint64> m_requestedUserIds;
		};

		class ORIGIN_PLUGIN_API AvatarGetRecentResponse : public AvatarsByGalleryIdResponse
		{
		public:
			explicit AvatarGetRecentResponse(AuthNetworkRequest);
		};

		class ORIGIN_PLUGIN_API AvatarTypeResponse : public OriginAuthServiceResponse
		{
			struct avatarType
			{
				quint64 id;
				QString name;
			} m_avatarType;
		public:
			explicit AvatarTypeResponse(AuthNetworkRequest reply);
			QString avatarName() const { return m_avatarType.name;}
		private:
			bool parseSuccessBody(QIODevice *body);
		};

		class ORIGIN_PLUGIN_API AvatarAllAvatarTypesResponse : public OriginAuthServiceResponse
		{
		public:
			explicit AvatarAllAvatarTypesResponse(AuthNetworkRequest reply);
			QString avatarTypeName (avatarType an) const { return avatarTypes().name(an);}
		private:
			bool parseSuccessBody(QIODevice *body);
		};

		class ORIGIN_PLUGIN_API AvatarSupportedDimensionsResponse : public OriginAuthServiceResponse
		{
		public:
			explicit AvatarSupportedDimensionsResponse(AuthNetworkRequest);
			const QList<QString> constList() {return m_list;}
		private:
			bool parseSuccessBody(QIODevice *body);
			QList<QString> m_list;
		};
	}
}





#endif


