/*************************************************************************************************/
/*!
    \file   FifaGroupsConnection.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FIFA_GROUPS_CONNECTION_H
#define BLAZE_FIFA_GROUPS_CONNECTION_H

/*** Include files *******************************************************************************/
#include "framework/config/config_sequence.h"
#include "framework/config/config_map.h"

#include "framework/connection/outboundhttpconnectionmanager.h"
#include "framework/protocol/outboundhttpresult.h"

#include "fifagroups/tdf/fifagroupstypes.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
	namespace FifaGroups
	{
		class FifaGroupsConnection
		{
			public:
				FifaGroupsConnection(const ConfigMap *configRoot);
				~FifaGroupsConnection();

				HttpConnectionManagerPtr getHttpConnManager();

				uint32_t getMaxAttempts() { return mMaxAttempts; }

				void getApiVersion(char8_t* dstStr, size_t dstStrLen);
				void getTokenScope(char8_t* dstStr, size_t dstStrLen);

				void addBaseUrl(char8_t* dstStr, size_t dstStrLen);		

				void addCreateInstanceAPI(char8_t* dstStr, size_t dstStrLen);
				void addCreateAndJoinInstanceAPI(char8_t* dstStr, size_t dstStrLen);
				void addDeleteInstanceAPI(char8_t* dstStr, size_t dstStrLen, const char8_t* groupGUId);
				void addDeleteMemberAPI(char8_t* dstStr, size_t dstStrLen, const char8_t* groupGUId, const char8_t* targetUserId);
				void addJoinGroupAPI(char8_t* dstStr, size_t dstStrLen, const char8_t* groupGUId, const char8_t* targetUserId);
				void addSetInstanceAttributeAPI(char8_t* dstStr, size_t dstStrLen, const char8_t* groupGUId);
				void addSetMultipeInstanceAttributesAPI(char8_t* dstStr, size_t dstStrLen, const char8_t* groupGUId);
				void addRenameGroupInstanceNameAPI(char8_t* dstStr, size_t dstStrLen, const char8_t* groupGUId, const char8_t* newName);
				void addRenameGroupInstanceShortNameAPI(char8_t* dstStr, size_t dstStrLen, const char8_t* groupGUId, const char8_t* newShortName);

			private:
				uint32_t mMaxAttempts;

				eastl::string mGroupsBaseUrl;
				eastl::string mApiVersion;
				eastl::string mTokenScope;

				eastl::string mApiDeleteInstance;
				eastl::string mApiCreateInstance;
				eastl::string mApiCreateAndJoinInstance;
				eastl::string mApiSetInstanceAttribute;
				eastl::string mApiSetMultipleInstanceAttributes;
				eastl::string mApiJoinGroup;
				eastl::string mApiDeleteMember;
				eastl::string mApiRenameGroupInstanceName;
				eastl::string mApiRenameGroupInstanceShortName;
		};

	} // FifaGroups
} // Blaze

#endif // BLAZE_FIFA_GROUPS_CONNECTION_H

