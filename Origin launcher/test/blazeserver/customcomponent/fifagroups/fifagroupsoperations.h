/*************************************************************************************************/
/*!
    \file   FifaGroupsOperations.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FIFA_GROUPS_OPERATIONS_H
#define BLAZE_FIFA_GROUPS_OPERATIONS_H

/*** Include files *******************************************************************************/
/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
	namespace FifaGroups
	{
		class FifaGroupsConnection;
		class CreateInstanceRequest;
		class CreateInstanceResponse;
		class DeleteInstanceRequest;
		class DeleteInstanceResponse;	
		class DeleteMemberRequest;
		class DeleteMemberResponse;
		class JoinGroupRequest;
		class JoinGroupResponse;
		class SetInstanceAttributeRequest;
		class SetInstanceAttributeResponse;
		class SetMultipleInstanceAttributesRequest;
		class SetMultipleInstanceAttributesResponse;
		class RenameGroupInstanceRequest;
		class RenameGroupInstanceResponse;

		BlazeRpcError CreateInstanceOperation(FifaGroupsConnection& connection, CreateInstanceRequest& request, CreateInstanceResponse& response);
		BlazeRpcError DeleteInstanceOperation(FifaGroupsConnection& connection, DeleteInstanceRequest& request, DeleteInstanceResponse& response);
		BlazeRpcError DeleteInstanceOperation(FifaGroupsConnection& connection, const char* serviceName, BlazeId userId, DeleteInstanceRequest& request, DeleteInstanceResponse& response);
		BlazeRpcError DeleteMemberOperation(FifaGroupsConnection& connection, DeleteMemberRequest& request, DeleteMemberResponse& response);
		BlazeRpcError JoinGroupOperation(FifaGroupsConnection& connection, JoinGroupRequest& request, JoinGroupResponse& response);
		BlazeRpcError JoinGroupOperation(FifaGroupsConnection& connection, const char* serviceName, BlazeId userId, JoinGroupRequest& request, JoinGroupResponse& response);
		BlazeRpcError SetInstanceAttributeOperation(FifaGroupsConnection& connection, SetInstanceAttributeRequest& request, SetInstanceAttributeResponse& response);
		BlazeRpcError SetMultipleInstanceAttributesOperation(FifaGroupsConnection& connection, SetMultipleInstanceAttributesRequest& request, SetMultipleInstanceAttributesResponse& response);
		BlazeRpcError SetMultipleInstanceAttributesOperation(FifaGroupsConnection& connection, const char* serviceName, BlazeId userId, SetMultipleInstanceAttributesRequest& request, SetMultipleInstanceAttributesResponse& response);
		BlazeRpcError RenameGroupInstanceOperation(FifaGroupsConnection& connection, const char* serviceName, BlazeId userId, RenameGroupInstanceRequest& request, RenameGroupInstanceResponse& response);

		char8_t* ConstructCreateInstanceBody(CreateInstanceRequest& request, size_t& outBodySize, bool isCreateAndJoin = false);
		char8_t* ConstructJoinGroupBody(JoinGroupRequest& request, size_t& outBodySize);
		char8_t* ConstructSetMultipleInstanceAttributesBody(SetMultipleInstanceAttributesRequest& request, size_t& outBodySize);

		const char* const JSON_PASSWORD_TAG = "password";
		const char* const JSON_ATTRIBUTES_TAG = "attributes";
		const char* const JSON_CREATOR_TAG = "creator";
		const char* const JSON_DATE_CREATED_TAG = "dateCreated";
		const char* const JSON_SIZE_TAG = "size";
		const char* const JSON_LAST_ACCESS_TAG = "lastAccessDate";
		const char* const JSON_GROUP_NAME_TAG = "name";
		const char* const JSON_GROUP_TYPE_TAG = "groupTypeId";
		const char* const JSON_GROUP_GUID_TAG = "_id";
		const char* const JSON_GROUP_SHORT_NAME_TAG = "shortName";

		const char* const JSON_GROUP_TAG = "group";
		const char* const JSON_GROUP_MEMBERS_TAG = "members";

		const char* const JSON_INSTANCE_JOIN_CONFIG_TAG = "instanceJoinConfig";
		const char* const JSON_HASHED_PWD_TAG = "hashedPwd";
		const char* const JSON_INVITE_KEY_TAG = "inviteURLKey";
		const char* const JSON_PASSWORD_SHORT_TAG = "pwd";

	} // FifaGroups
} // Blaze

#endif // BLAZE_FIFA_GROUPS_OPERATIONS_H

