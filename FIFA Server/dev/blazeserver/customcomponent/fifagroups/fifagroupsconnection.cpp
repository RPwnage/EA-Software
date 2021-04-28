/*************************************************************************************************/
/*!
    \file   FifaGroupsConnection.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FifaGroupsConnection

    Creates an HTTP connection manager to the Fifa Groups system.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/util/xmlbuffer.h"
#include "framework/connection/outboundhttpservice.h"

#include "fifagroupsconnection.h"

#include "fifagroups/rpc/fifagroupsmaster.h"
#include "fifagroups/tdf/fifagroupstypes.h"

namespace Blaze
{
	namespace FifaGroups
	{
		FifaGroupsConnection::FifaGroupsConnection(const ConfigMap *configRoot)
		{
			const ConfigMap* configData = configRoot->getMap("fifagroups");

			mMaxAttempts = configData->getUInt32("maxAttempts", 3);

			mGroupsBaseUrl	= configData->getString("groupsUrlBase", "");
			mApiVersion		= configData->getString("apiVersion", "");
			mTokenScope		= configData->getString("tokenScope", "");

			mApiDeleteInstance			= configData->getString("apiDeleteInstance", "");
			mApiCreateInstance			= configData->getString("apiCreateInstance", "");
			mApiCreateAndJoinInstance	= configData->getString("apiCreateAndJoinInstance", "");
			mApiSetInstanceAttribute	= configData->getString("apiSetInstanceAttribute", "");
			mApiSetMultipleInstanceAttributes = configData->getString("apiSetMultipleInstanceAttributes", "");
			mApiJoinGroup				= configData->getString("apiJoinGroup", "");
			mApiDeleteMember			= configData->getString("apiDeleteMember", "");
			mApiRenameGroupInstanceName			= configData->getString("apiRenameGroupInstanceName", "");
			mApiRenameGroupInstanceShortName	= configData->getString("apiRenameGroupInstanceShortName", "");
		}

		//=======================================================================================
		FifaGroupsConnection::~FifaGroupsConnection()
		{
		}

		//=======================================================================================
		HttpConnectionManagerPtr FifaGroupsConnection::getHttpConnManager()
		{
			return gOutboundHttpService->getConnection("fifagroups");
		}

		void FifaGroupsConnection::getApiVersion(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_snzprintf(dstStr, dstStrLen, "%s", mApiVersion.c_str());
		}

		void FifaGroupsConnection::getTokenScope(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_snzprintf(dstStr, dstStrLen, "%s", mTokenScope.c_str());
		}

		void FifaGroupsConnection::addBaseUrl(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_strnzcat(dstStr, mGroupsBaseUrl.c_str(), dstStrLen);
		}

		void FifaGroupsConnection::addCreateInstanceAPI(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_strnzcat(dstStr, mApiCreateInstance.c_str(), dstStrLen);
		}

		void FifaGroupsConnection::addCreateAndJoinInstanceAPI(char8_t* dstStr, size_t dstStrLen)
		{
			blaze_strnzcat(dstStr, mApiCreateAndJoinInstance.c_str(), dstStrLen);
		}

		void FifaGroupsConnection::addDeleteInstanceAPI(char8_t* dstStr, size_t dstStrLen, const char8_t* groupGUId)
		{
			eastl::string temp;
			temp.sprintf(mApiDeleteInstance.c_str(), groupGUId);

			blaze_strnzcat(dstStr, temp.c_str(), dstStrLen);
		}

		void FifaGroupsConnection::addDeleteMemberAPI(char8_t* dstStr, size_t dstStrLen, const char8_t* groupGUId, const char8_t* targetUserId)
		{
			eastl::string temp;
			temp.sprintf(mApiDeleteMember.c_str(), groupGUId, targetUserId);

			blaze_strnzcat(dstStr, temp.c_str(), dstStrLen);
		}

		void FifaGroupsConnection::addJoinGroupAPI(char8_t* dstStr, size_t dstStrLen, const char8_t* groupGUId, const char8_t* targetUserId)
		{
			eastl::string temp;
			temp.sprintf(mApiJoinGroup.c_str(), groupGUId, targetUserId);

			blaze_strnzcat(dstStr, temp.c_str(), dstStrLen);
		}

		void FifaGroupsConnection::addSetInstanceAttributeAPI(char8_t* dstStr, size_t dstStrLen, const char8_t* groupGUId)
		{
			eastl::string temp;
			temp.sprintf(mApiSetInstanceAttribute.c_str(), groupGUId);

			blaze_strnzcat(dstStr, temp.c_str(), dstStrLen);
		}

		void FifaGroupsConnection::addSetMultipeInstanceAttributesAPI(char8_t* dstStr, size_t dstStrLen, const char8_t* groupGUId)
		{
			eastl::string temp;
			temp.sprintf(mApiSetMultipleInstanceAttributes.c_str(), groupGUId);

			blaze_strnzcat(dstStr, temp.c_str(), dstStrLen);
		}

		void FifaGroupsConnection::addRenameGroupInstanceNameAPI(char8_t* dstStr, size_t dstStrLen, const char8_t* groupGUId, const char8_t* newName)
		{
			eastl::string temp;
			temp.sprintf(mApiRenameGroupInstanceName.c_str(), groupGUId, newName);

			blaze_strnzcat(dstStr, temp.c_str(), dstStrLen);
		}

		void FifaGroupsConnection::addRenameGroupInstanceShortNameAPI(char8_t* dstStr, size_t dstStrLen, const char8_t* groupGUId, const char8_t* newShortName)
		{
			eastl::string temp;
			temp.sprintf(mApiRenameGroupInstanceShortName.c_str(), groupGUId, newShortName);

			blaze_strnzcat(dstStr, temp.c_str(), dstStrLen);
		}
		//=======================================================================================

	} // FifaGroups
} // Blaze
