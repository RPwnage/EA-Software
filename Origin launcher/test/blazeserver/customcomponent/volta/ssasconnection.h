/*************************************************************************************************/
/*!
    \file   SSASConnection.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SSAS_CONNECTION_H
#define BLAZE_SSAS_CONNECTION_H

/*** Include files *******************************************************************************/
#include "framework/config/config_sequence.h"
#include "framework/config/config_map.h"

#include "framework/connection/outboundhttpservice.h"
#include "framework/protocol/outboundhttpresult.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
	namespace Volta
	{
		class SSASConnection
		{
		public:
			SSASConnection(const SSASConfig& ssasConfig);
			~SSASConnection();

			BlazeRpcError RequestMatchEnd(const SSASMatchEndRequest& ssasMatchEndRequest);

		private:
			BlazeRpcError SendRequest(
				const EA::TDF::TdfString& sid,
				const eastl::string& url, 
				HttpProtocolUtil::HttpMethod httpMethod, 
				const EA::TDF::TdfString& json);

			eastl::string mMatchEndUrl;
			eastl::string mSkuMode;

			uint32_t mMaxAttempts;
		};

	} // Volta
} // Blaze

#endif // BLAZE_SSAS_CONNECTION_H

