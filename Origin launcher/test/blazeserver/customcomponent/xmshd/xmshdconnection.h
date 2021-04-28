/*************************************************************************************************/
/*!
    \file   XmsHdConnection.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_XMSHD_CONNECTION_H
#define BLAZE_XMSHD_CONNECTION_H

/*** Include files *******************************************************************************/
#include "framework/config/config_sequence.h"
#include "framework/config/config_map.h"

#include "framework/connection/outboundhttpservice.h"
#include "framework/protocol/outboundhttpresult.h"

#include "xmshd/tdf/xmshdtypes.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
	namespace XmsHd
	{

		class XmsHdConnection
		{
			public:
				XmsHdConnection(const ConfigMap* xmsHdData);
				~XmsHdConnection();

				HttpConnectionManagerPtr getConn();

				eastl::string getPartnerToken();
				void resetPartnerToken();

				void addHttpParamAppendOp(char8_t* dstStr, size_t dstStrLen);

				void addBaseUrl(char8_t* dstStr, size_t dstStrLen);
				void addHttpParam_ApiKey(char8_t* dstStr, size_t dstStrLen);
				void addHttpParam_Sku(char8_t* dstStr, size_t dstStrLen);
				void addHttpParam_User(char8_t* dstStr, size_t dstStrLen, EntityId userId);
				void addHttpParam_Auth(char8_t* dstStr, size_t dstStrLen);

				void addHttpParam_PartnerToken_Url(char8_t* dstStr, size_t dstStrLen);
				void addHttpParam_PartnerToken_Sku(char8_t* dstStr, size_t dstStrLen);
				void addHttpParam_PartnerToken_Password(char8_t* dstStr, size_t dstStrLen);

				uint32_t getMaxAttempts(void) { return mMaxAttempts; }

			private:
				eastl::string mBaseUrl;
				eastl::string mApiKey;
				eastl::string mSku;

				eastl::string mPartnerTokenUrl;
				eastl::string mPartnerTokenSku;
				eastl::string mPartnerTokenPassword;

				eastl::string mPartnerToken;

				uint32_t mMaxAttempts;
		};

	} // XmsHd
} // Blaze

#endif // BLAZE_XMSHD_CONNECTION_H

