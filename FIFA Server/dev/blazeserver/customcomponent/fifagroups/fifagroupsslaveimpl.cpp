/*************************************************************************************************/
/*!
    \file   fifagroupsslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FifaGroupsSlaveImpl

    FifaGroups Slave implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "fifagroupsslaveimpl.h"

// blaze includes
#include "framework/controller/controller.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/database/dbscheduler.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"

// fifagroups includes
#include "fifagroups/fifagroupsoperations.h"
#include "fifagroups/rpc/fifagroupsmaster.h"
#include "fifagroups/tdf/fifagroupstypes.h"
#include "fifagroups/tdf/fifagroupstypes_server.h"

// fifastats includes
#include "fifastats/rpc/fifastatsslave.h"

#include "framework/tdf/userdefines.h"

namespace Blaze
{
namespace FifaGroups
{
// static
FifaGroupsSlave* FifaGroupsSlave::createImpl()
{
    return BLAZE_NEW_NAMED("FifaGroupsSlaveImpl") FifaGroupsSlaveImpl();
}

/*** Public Methods ******************************************************************************/
FifaGroupsSlaveImpl::FifaGroupsSlaveImpl()
{
}

FifaGroupsSlaveImpl::~FifaGroupsSlaveImpl()
{
}

BlazeRpcError FifaGroupsSlaveImpl::processMergeGroupInstances(MergeGroupInstancesRequest &request)
{
	gSelector->scheduleFiberCall<FifaGroupsSlaveImpl, Blaze::UserSessionId, eastl::string, BlazeId, MergeGroupInstancesRequest>(
					this,
					&FifaGroupsSlaveImpl::mergeGroups,
					UserSession::getCurrentUserSessionId(),
					gCurrentUserSession->getServiceName(),
					gCurrentUserSession->getUserId(),
					request,
					"FifaGroupsSlaveImpl::processMergeGroupInstances");

	return ERR_OK;
}

bool FifaGroupsSlaveImpl::onConfigure()
{
	bool success = configureHelper();
	TRACE_LOG("[FifaGroupsSlaveImpl:" << this << "].onConfigure: configuration " << (success ? "successful." : "failed."));
	return true;
}

bool FifaGroupsSlaveImpl::onReconfigure()
{
	bool success = configureHelper();
	TRACE_LOG("[FifaGroupsSlaveImpl:" << this << "].onReconfigure: configuration " << (success ? "successful." : "failed."));
	return true;
}

void FifaGroupsSlaveImpl::onShutdown()
{
	TRACE_LOG("[FifaGroupsSlaveImpl].onShutdown");
}

bool FifaGroupsSlaveImpl::configureHelper()
{
	TRACE_LOG("[FifaGroupsSlaveImpl].configureHelper");
	return true;
}

void FifaGroupsSlaveImpl::mergeGroups(Blaze::UserSessionId userSessionId, eastl::string serviceName, BlazeId userId, MergeGroupInstancesRequest request)
{
	const ConfigMap* config = getComponentConfig();
	FifaGroupsConnection connection(config);

	MergeNotification mergeNotification;
	Blaze::FifaGroups::GroupIdVector& mergeNotificationGroupVector = mergeNotification.getGroupVector();

	Blaze::BlazeRpcError error = Blaze::ERR_OK;

	MergeUpdateOpVector mergeUpdateOperations = request.getMergeUpdateOpVector();
	mergeNotification.setMergeOpType(MergeNotification::MERGE_GROUP_COMPLETE);

	int32_t httpStatusCode = HTTP_STATUS_OK;
	MergeUpdateOpVector::iterator iter, end;
	iter = mergeUpdateOperations.begin();
	end = mergeUpdateOperations.end();
	for (; iter != end; iter++)
	{
		MergeUpdateOp* updateOp = *iter;
		
		mergeNotificationGroupVector.push_back(updateOp->getGroupGUId());

		if (updateOp->getShouldDelete())
		{
			DeleteInstanceRequest deleteInstanceRequest;
			DeleteInstanceResponse deleteInstanceResponse;
			deleteInstanceRequest.setGroupGUId(updateOp->getGroupGUId());
			error = DeleteInstanceOperation(connection, serviceName.c_str(), userId, deleteInstanceRequest, deleteInstanceResponse);

			if (error != Blaze::ERR_OK &&  httpStatusCode != HTTP_STATUS_OK)
			{
				mergeNotification.setMergeOpType(MergeNotification::MERGE_GROUP_DELETE_GROUP);
				mergeNotification.setErrorMessage(deleteInstanceResponse.getErrorMessage());
				break;
			}

			continue;
		}

		if (updateOp->getJoinPersonaId() != INVALID_BLAZE_ID)
		{
			JoinGroupRequest joinGroupRequest;
			JoinGroupResponse joinGroupResponse;
			joinGroupRequest.setGroupGUId(updateOp->getGroupGUId());
			joinGroupRequest.setTargetUserId(updateOp->getJoinPersonaId());

			error = JoinGroupOperation(connection, serviceName.c_str(), userId, joinGroupRequest, joinGroupResponse);

			if (error != Blaze::ERR_OK &&  httpStatusCode != HTTP_STATUS_OK)
			{
				mergeNotification.setMergeOpType(MergeNotification::MERGE_GROUP_JOIN_GROUP);
				mergeNotification.setErrorMessage(joinGroupResponse.getErrorMessage());
				break;
			}
		}

		AttributeList attributeList = updateOp->getAttributes();

		SetMultipleInstanceAttributesRequest setMutlipleInstanceAttributesRequest;
		SetMultipleInstanceAttributesResponse setMutlipleInstanceAttributesResponse;
		setMutlipleInstanceAttributesRequest.setGroupGUId(updateOp->getGroupGUId());


		AttributeList& requestAttributes = setMutlipleInstanceAttributesRequest.getAttributes();
		int size = attributeList.size();
		if (size > 0)
		{
			requestAttributes.initVector(size);
			for (int i = 0; i < size; i++)
			{
				requestAttributes[i] = requestAttributes.allocate_element();
				requestAttributes[i] = attributeList[i];
			}

			error = SetMultipleInstanceAttributesOperation(connection, serviceName.c_str(), userId, setMutlipleInstanceAttributesRequest, setMutlipleInstanceAttributesResponse);
			httpStatusCode = setMutlipleInstanceAttributesResponse.getHttpStatusCode();

			if (error != Blaze::ERR_OK && httpStatusCode != HTTP_STATUS_OK)
			{
				mergeNotification.setMergeOpType(MergeNotification::MERGE_GROUP_SET_ATTRIBUTES);
				mergeNotification.setErrorMessage(setMutlipleInstanceAttributesResponse.getErrorMessage());
				break;
			}
		}

		if (EA::StdC::Strlen(updateOp->getGroupName()) > 0)
		{
			RenameGroupInstanceRequest renameGroupInstanceRequest;
			RenameGroupInstanceResponse renameGroupInstanceResponse;

			renameGroupInstanceRequest.setGroupGUId(updateOp->getGroupGUId());
			renameGroupInstanceRequest.setNewName(updateOp->getGroupName());
			renameGroupInstanceRequest.setIsShortName(false);
			error = RenameGroupInstanceOperation(connection, serviceName.c_str(), userId, renameGroupInstanceRequest, renameGroupInstanceResponse);
			httpStatusCode = renameGroupInstanceResponse.getHttpStatusCode();
			if (error == Blaze::ERR_OK &&  httpStatusCode == HTTP_STATUS_OK)
			{
				renameGroupInstanceRequest.setIsShortName(true);
				RenameGroupInstanceResponse renameGroupInstanceShortResponse;
				error = RenameGroupInstanceOperation(connection, serviceName.c_str(), userId, renameGroupInstanceRequest, renameGroupInstanceShortResponse);
				httpStatusCode = renameGroupInstanceShortResponse.getHttpStatusCode();
				if (error != Blaze::ERR_OK &&  httpStatusCode != HTTP_STATUS_OK)
				{
					mergeNotification.setMergeOpType(MergeNotification::MERGE_GROUP_RENAME_SHORT_NAME);
					mergeNotification.setErrorMessage(renameGroupInstanceShortResponse.getErrorMessage());
					break;
				}
			}
			else
			{
				mergeNotification.setMergeOpType(MergeNotification::MERGE_GROUP_RENAME_NAME);
				mergeNotification.setErrorMessage(renameGroupInstanceResponse.getErrorMessage());
				break;
			}
		}
	}
	
	if (error == Blaze::ERR_OK && httpStatusCode == HTTP_STATUS_OK)
	{
		Blaze::FifaStats::FifaStatsSlave *fifaStatsComponent = static_cast<Blaze::FifaStats::FifaStatsSlave*>(gController->getComponent(Blaze::FifaStats::FifaStatsSlave::COMPONENT_ID, false));

		if (fifaStatsComponent != NULL)
		{
			Blaze::FifaStats::UpdateStatsResponse updateStatsResponse;

			request.getOverallGroupUpdates().setServiceName(serviceName.c_str());
			if (!request.getOverallGroupUpdates().getEntities().empty())
			{
				UserSession::pushSuperUserPrivilege();
				error = fifaStatsComponent->updateStatsInternal(request.getOverallGroupUpdates(), updateStatsResponse);
				UserSession::popSuperUserPrivilege();

				httpStatusCode = updateStatsResponse.getHttpStatusCode();
			}
			
			if (error == Blaze::ERR_OK && httpStatusCode == HTTP_STATUS_OK)
			{
				if (!request.getRivalGroupUpdates().getEntities().empty())
				{
					Blaze::FifaStats::UpdateStatsResponse updateRivalStatsResponse;
					request.getRivalGroupUpdates().setServiceName(serviceName.c_str());
					UserSession::pushSuperUserPrivilege();
					error = fifaStatsComponent->updateStatsInternal(request.getRivalGroupUpdates(), updateRivalStatsResponse);
					UserSession::popSuperUserPrivilege();

					httpStatusCode = updateRivalStatsResponse.getHttpStatusCode();

					if (error != Blaze::ERR_OK && httpStatusCode != HTTP_STATUS_OK)
					{
						mergeNotification.setMergeOpType(MergeNotification::MERGE_GROUP_UPDATE_STATS_RIVAL);
						mergeNotification.setErrorMessage(updateRivalStatsResponse.getErrorMessage());
					}
				}
			}
			else
			{
				mergeNotification.setMergeOpType(MergeNotification::MERGE_GROUP_UPDATE_STATS_OVERALL);
				mergeNotification.setErrorMessage(updateStatsResponse.getErrorMessage());
			}
		}
	}

	mergeNotification.setMergeOperationId(request.getMergeOperationId());
	mergeNotification.setSuccess(error == Blaze::ERR_OK && httpStatusCode == HTTP_STATUS_OK);
	mergeNotification.setErrorCode(error);
	mergeNotification.setHttpStatusCode(httpStatusCode);
	sendMergeNotificationToUserSessionById(userSessionId, &mergeNotification);
}

} // FifaGroups
} // Blaze
