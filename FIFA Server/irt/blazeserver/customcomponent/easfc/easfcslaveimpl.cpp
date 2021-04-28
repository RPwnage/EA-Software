/*************************************************************************************************/
/*!
    \file   easfcslaveimpl.cpp

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/easfc/easfcslaveimpl.cpp#2 $

    \attention
    (c) Electronic Arts 2010. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class EasfcSlaveImpl

    Easfc Slave implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "framework/connection/selector.h"
#include "easfcslaveimpl.h"

// blaze includes
#include "framework/controller/controller.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/database/dbscheduler.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"

#include "component/stats/statsconfig.h"
#include "component/stats/statsslaveimpl.h"
#include "component/stats/updatestatshelper.h"
#include "component/stats/updatestatsrequestbuilder.h"


// easfc includes
#include "easfc/rpc/easfcmaster.h"

namespace Blaze
{
namespace EASFC
{


// static
EasfcSlave* EasfcSlave::createImpl()
{
    return BLAZE_NEW EasfcSlaveImpl();
}


/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief  constructor

    

*/
/*************************************************************************************************/
EasfcSlaveImpl::EasfcSlaveImpl() :
    mMetrics()
{
}


/*************************************************************************************************/
/*!
    \brief  destructor

    

*/
/*************************************************************************************************/
EasfcSlaveImpl::~EasfcSlaveImpl()
{
}

/*************************************************************************************************/
/*!
    \brief  purchaseGameWin

    purchase a Game Win

*/
/*************************************************************************************************/
BlazeRpcError EasfcSlaveImpl::purchaseGameWin(PurchaseGameRequest request)
{
	TRACE_LOG("[EasfcSlaveImpl].purchaseGameWin. memberId = ");

	BlazeRpcError error = Blaze::ERR_OK;

	// record metrics
	mMetrics.mPurchaseGameWin.add(1);

	return error;
}

/*************************************************************************************************/
/*!
    \brief  purchaseGameDraw

    purchase a Game Draw

*/
/*************************************************************************************************/
BlazeRpcError EasfcSlaveImpl::purchaseGameDraw(PurchaseGameRequest request)
{
	TRACE_LOG("[EasfcSlaveImpl].purchaseGameDraw. memberId = ");

	BlazeRpcError error = Blaze::ERR_OK;

	// record metrics
	mMetrics.mPurchaseGameDraw.add(1);

	return error;
}

/*************************************************************************************************/
/*!
    \brief  purchaseGameLoss

    purchase a Game Loss

*/
/*************************************************************************************************/
BlazeRpcError EasfcSlaveImpl::purchaseGameLoss(PurchaseGameRequest request)
{
	TRACE_LOG("[EasfcSlaveImpl].purchaseGameLoss. memberId = ");

	BlazeRpcError error = Blaze::ERR_OK;

	// record metrics
	mMetrics.mPurchaseGameLoss.add(1);

	return error;
}

/*************************************************************************************************/
/*!
    \brief  purchaseGameMatch

    purchase a Game Match

*/
/*************************************************************************************************/
BlazeRpcError EasfcSlaveImpl::purchaseGameMatch(PurchaseGameRequest request)
{
	TRACE_LOG("[EasfcSlaveImpl].purchaseGameMatch. memberId = ");

	BlazeRpcError error = Blaze::ERR_OK;

	// record metrics
	mMetrics.mPurchaseGameMatch.add(1);

	return error;
}

/*************************************************************************************************/
/*!
    \brief  updateVproStats

    sets the vpro stats

*/
/*************************************************************************************************/
BlazeRpcError EasfcSlaveImpl::updateVproStats(const char* userStatsCategory, EntityId entId, const StatNameValueMapStr* vproStatMap)
{
	TRACE_LOG("[EasfcSlaveImpl].updateVproStats. memberId = ");

	BlazeRpcError error = Blaze::ERR_OK;

	Blaze::Stats::StatsSlave *statsComponent =
		static_cast<Blaze::Stats::StatsSlave*>(gController->getComponent(Blaze::Stats::StatsSlave::COMPONENT_ID,false));

	if (statsComponent == NULL)
	{
		TRACE_LOG("[EasfcSlaveImpl].updateVproStats. stats component not found.");
		error = Blaze::EASFC_ERR_GENERAL;
	}

	if (Blaze::ERR_OK == error)
	{
		Stats::StatRowUpdate* statRow = NULL;
		Stats::StatUpdate* statUpdate = NULL;

		statRow = BLAZE_NEW Stats::StatRowUpdate;
		statRow->setCategory(userStatsCategory);
		statRow->setEntityId(entId);
		
		StatNameValueMapStr::const_iterator it = vproStatMap->begin();
		StatNameValueMapStr::const_iterator itEnd = vproStatMap->end();

		for(; it != itEnd; it++)
		{
			statUpdate = BLAZE_NEW Stats::StatUpdate;
			statUpdate->setName(it->first);
			statUpdate->setUpdateType(Stats::STAT_UPDATE_TYPE_ASSIGN);
			statUpdate->setValue(it->second);
			statRow->getUpdates().push_back(statUpdate);
		}

		// upgradeTitle
		setUpgradeTitleColumn(statRow, statUpdate);

		Blaze::Stats::UpdateStatsRequest request;
		request.getStatUpdates().push_back(statRow);

		UserSession::pushSuperUserPrivilege();
		error = statsComponent->updateStats(request);
		UserSession::popSuperUserPrivilege();

		if (Blaze::ERR_OK != error)
		{
			ERR_LOG("[EasfcSlaveImpl].updateVproStats. update stats RPC failed.");
		}
	}

	// record metrics
	//mMetrics.mPurchaseGameMatch.add(1);

	return error;
}

/*************************************************************************************************/
/*!
    \brief  updateVproIntStats

    sets the vpro stats

*/
/*************************************************************************************************/
BlazeRpcError EasfcSlaveImpl::updateVproIntStats(const char* userStatsCategory, EntityId entId, const StatNameValueMapInt* vproStatMap)
{
	TRACE_LOG("[EasfcSlaveImpl].updateVproIntStats. memberId = ");

	BlazeRpcError error = Blaze::ERR_OK;

	Blaze::Stats::StatsSlave *statsComponent =
		static_cast<Blaze::Stats::StatsSlave*>(gController->getComponent(Blaze::Stats::StatsSlave::COMPONENT_ID,false));

	if (statsComponent == NULL)
	{
		TRACE_LOG("[EasfcSlaveImpl].updateVproIntStats. stats component not found.");
		error = Blaze::EASFC_ERR_GENERAL;
	}

	if (Blaze::ERR_OK == error)
	{
		Stats::StatRowUpdate* statRow;
		Stats::StatUpdate* statUpdate = 0;

		statRow = BLAZE_NEW Stats::StatRowUpdate;
		statRow->setCategory(userStatsCategory);
		statRow->setEntityId(entId);
		
		StatNameValueMapInt::const_iterator it = vproStatMap->begin();
		StatNameValueMapInt::const_iterator itEnd = vproStatMap->end();

		for(; it != itEnd; it++)
		{
			char8_t buffer[BUFFER_SIZE_32];
			blaze_snzprintf(buffer, BUFFER_SIZE_32, "%d", it->second);

			statUpdate = BLAZE_NEW Stats::StatUpdate;
			statUpdate->setName(it->first);
			statUpdate->setUpdateType(Stats::STAT_UPDATE_TYPE_ASSIGN);
			statUpdate->setValue(buffer);
			statRow->getUpdates().push_back(statUpdate);
		}

		// upgradeTitle
		setUpgradeTitleColumn(statRow, statUpdate);

		Blaze::Stats::UpdateStatsRequest request;
		request.getStatUpdates().push_back(statRow);

		UserSession::pushSuperUserPrivilege();
		error = statsComponent->updateStats(request);
		UserSession::popSuperUserPrivilege();

		if (Blaze::ERR_OK != error)
		{
			ERR_LOG("[EasfcSlaveImpl].updateVproIntStats. update stats RPC failed.");
		}
	}

	// record metrics
	//mMetrics.mPurchaseGameMatch.add(1);

	return error;
}


/*************************************************************************************************/
/*!
    \brief  updateSeasonsStats

    sets the seasonsStats

*/
/*************************************************************************************************/
BlazeRpcError EasfcSlaveImpl::updateNormalGameStats(const char* userStatsCategory, EntityId entId, const NormalGameStatsList& normalGameStatsList)
{
	TRACE_LOG("[EasfcSlaveImpl].updateNormalGameStats. memberId = ");

	BlazeRpcError error = Blaze::ERR_OK;

	Blaze::Stats::StatsSlave *statsComponent =
		static_cast<Blaze::Stats::StatsSlave*>(gController->getComponent(Blaze::Stats::StatsSlave::COMPONENT_ID,false));

	if (statsComponent == NULL)
	{
		TRACE_LOG("[EasfcSlaveImpl].updateNormalGameStats. stats component not found.");
		error = Blaze::EASFC_ERR_GENERAL;
	}

	if (Blaze::ERR_OK == error)
	{
		Stats::StatRowUpdate* statRow;
		Stats::StatUpdate* statUpdate;
		Stats::UpdateStatsRequest request;
		int listsize = static_cast<int>(normalGameStatsList.size());
		for (int i = 0; i < listsize; i++)
		{
			statRow = BLAZE_NEW Stats::StatRowUpdate;
			statRow->setCategory(userStatsCategory);
			statRow->setEntityId(entId);
			// Get Keyscope data
			StatNameValueMapInt &keyscopes = normalGameStatsList[i]->getNormalKeyScopes();
			statRow->getKeyScopeNameValueMap().assign(keyscopes.begin(), keyscopes.end());

			// fill in rest of data
			StatNameValueMapInt &normalGameStatData = normalGameStatsList[i]->getNormalGameStats();
			
			StatNameValueMapInt::const_iterator it = normalGameStatData.begin();
			StatNameValueMapInt::const_iterator itEnd = normalGameStatData.end();
			
			for(; it != itEnd; it++)
			{
				char8_t buffer[BUFFER_SIZE_32];
				blaze_snzprintf(buffer, BUFFER_SIZE_32, "%d", it->second);
				
				statUpdate = BLAZE_NEW Stats::StatUpdate;
				statUpdate->setName(it->first);
				statUpdate->setUpdateType(Stats::STAT_UPDATE_TYPE_ASSIGN);
				statUpdate->setValue(buffer);
				statRow->getUpdates().push_back(statUpdate);
			}

			request.getStatUpdates().push_back(statRow);
		}

		UserSession::pushSuperUserPrivilege();
		error = statsComponent->updateStats(request);
		UserSession::popSuperUserPrivilege();

		if (Blaze::ERR_OK != error)
		{
			ERR_LOG("[EasfcSlaveImpl].updateNormalGameStats. update stats RPC failed.");
		}
	}

	// record metrics
	//mMetrics.mPurchaseGameMatch.add(1);

	return error;
}

BlazeRpcError EasfcSlaveImpl::updateSPStats(const char* userStatsCategory, EntityId entId, const StatNameValueMapInt* statRequest)
{
	TRACE_LOG("[EasfcSlaveImpl].updateSPStats. memberId = " << entId);

	BlazeRpcError error = Blaze::ERR_OK;

	Blaze::Stats::StatsSlave *statsComponent =
		static_cast<Blaze::Stats::StatsSlave*>(gController->getComponent(Blaze::Stats::StatsSlave::COMPONENT_ID,false));

	if (statsComponent == NULL)
	{
		TRACE_LOG("[EasfcSlaveImpl].updateSPStats. stats component not found.");
		error = Blaze::EASFC_ERR_GENERAL;
	}

	if (Blaze::ERR_OK == error)
	{
		Stats::StatRowUpdate* statRow = NULL;
		Stats::StatUpdate* statUpdate = NULL;

		statRow = BLAZE_NEW Stats::StatRowUpdate;
		statRow->setCategory(userStatsCategory);
		statRow->setEntityId(entId);

		StatNameValueMapInt::const_iterator it = statRequest->begin();
		StatNameValueMapInt::const_iterator itEnd = statRequest->end();

		for(; it != itEnd; it++)
		{
			if(blaze_stricmp(it->first, "curDivision") == 0 && it->second < 1)
			{
				ERR_LOG("[EasfcSlaveImpl].updateSPStats.  curDivision passed in was less than 1!  Invalid info passed. failing out!");
				error = Blaze::EASFC_ERR_INVALID_PARAMS;
				break;
			}

			char8_t buffer[BUFFER_SIZE_32];
			blaze_snzprintf(buffer, BUFFER_SIZE_32, "%d", it->second);
			statUpdate = BLAZE_NEW Stats::StatUpdate;
			statUpdate->setName(it->first);
			statUpdate->setUpdateType(Stats::STAT_UPDATE_TYPE_ASSIGN);
			statUpdate->setValue(buffer);
			statRow->getUpdates().push_back(statUpdate);
		}

		if(Blaze::ERR_OK != error)
		{
			// fail out here because we are trying to set bad data
			return error;
		}

		// upgradeTitle
		if(blaze_strcmp(userStatsCategory, "SPOverallPlayerStats") == 0)
		{
			setUpgradeTitleColumn(statRow, statUpdate);
		}

		Blaze::Stats::UpdateStatsRequest request;
		request.getStatUpdates().push_back(statRow);

		UserSession::pushSuperUserPrivilege();
		error = statsComponent->updateStats(request);
		UserSession::popSuperUserPrivilege();

		if (Blaze::ERR_OK != error)
		{
			ERR_LOG("[EasfcSlaveImpl].updateSPStats. update stats RPC failed.");
		}
	}

	// record metrics
	//mMetrics.mPurchaseGameMatch.add(1);

	return error;
}


BlazeRpcError EasfcSlaveImpl::applySeasonsRewards(EntityId entId, const StatNameValueMapInt* pSPOValues)
{
	TRACE_LOG("[EasfcSlaveImpl].applySeasonsRewards. memberId = " << entId);

	Blaze::BlazeRpcError error = Blaze::ERR_OK;

	if(pSPOValues == NULL)
	{
		ERR_LOG("[EasfcSlaveImpl].applySeasonsRewards.  stat map is null!");
		error = Blaze::EASFC_ERR_INVALID_PARAMS;
		return error;
	}

	int newDivision = 1;
	StatNameValueMapInt::const_iterator iter = pSPOValues->find("curDivision");
	if(iter != pSPOValues->end())
	{
		int oldDivision = iter->second;
		if( oldDivision < 1)
		{
			ERR_LOG("[EasfcSlaveImpl].applySeasonsRewards.  curDivision passed in was less than 1!  Invalid info passed. failing out!");
			error = Blaze::EASFC_ERR_INVALID_PARAMS;
		}
		else if(oldDivision >= 1 && oldDivision <= 3)
		{
			newDivision = 1;
		}
		else if (oldDivision >= 4 && oldDivision <= 7)
		{
			newDivision = 2;
		}
		else
		{
			newDivision = 3;
		}
	}
	else
	{
		ERR_LOG("[EasfcSlaveImpl].applySeasonsRewards.  curDivsion not found! failing out!");
		error = Blaze::EASFC_ERR_INVALID_PARAMS;
	}
	

	if(error == Blaze::ERR_OK)
	{
		Blaze::Stats::StatsSlave *statsComponent =
			static_cast<Blaze::Stats::StatsSlave*>(gController->getComponent(Blaze::Stats::StatsSlave::COMPONENT_ID,false));

		const char* statCatSPOverallStats = "SPOverallPlayerStats";
		const char* statCatSPDivisionalStats = "SPDivisionalPlayerStats";

		Blaze::Stats::UpdateStatsRequest request;
		
		Stats::StatRowUpdate* statRow;
		// Write rewarded division to SPOverallStats and set the upgrade flag
		statRow = BLAZE_NEW Stats::StatRowUpdate;
		statRow->setCategory(statCatSPOverallStats);
		statRow->setEntityId(entId);
		writeToColumn("curDivision", newDivision, statRow);
		writeToColumn("maxDivision", newDivision, statRow);
		writeToColumn("bestDivision", newDivision-1, statRow);
		writeToColumn("upgradeTitle", 1, statRow);
		request.getStatUpdates().push_back(statRow);

		// Set SPDivisionalPlayerStats curDivision to upgraded div.
		statRow = BLAZE_NEW Stats::StatRowUpdate;
		statRow->setCategory(statCatSPDivisionalStats);
		statRow->setEntityId(entId);
		writeToColumn("curDivision", newDivision, statRow);
		request.getStatUpdates().push_back(statRow);

		// apply change
		UserSession::pushSuperUserPrivilege();
		error = statsComponent->updateStats(request);
		UserSession::popSuperUserPrivilege();
	}

	return error;
}

BlazeRpcError EasfcSlaveImpl::writeToSPPrevOverallStats(EntityId entId, const StatNameValueMapInt* pSPOValues, NormalGameStatsList& normalGameStatsList)
{
	TRACE_LOG("[EasfcSlaveImpl].writeToSPPrevOverallStats. memberId = " << entId);

	Blaze::BlazeRpcError error = Blaze::ERR_OK;
	
	Blaze::Stats::StatsSlave *statsComponent =
		static_cast<Blaze::Stats::StatsSlave*>(gController->getComponent(Blaze::Stats::StatsSlave::COMPONENT_ID,false));

	if (statsComponent == NULL)
	{
		TRACE_LOG("[EasfcSlaveImpl].writeToSPPrevOverallStats. stats component not found.");
		error = Blaze::EASFC_ERR_GENERAL;
	}

	if (error == Blaze::ERR_OK)
	{
		// Get w-d-l
		int listsize = static_cast<int>(normalGameStatsList.size());
		int wins = 0;
		int ties = 0;
		int losses = 0;

		for (int i = 0; i < listsize; i++)
		{
			StatNameValueMapInt &normalGameStatData = normalGameStatsList[i]->getNormalGameStats();

			StatNameValueMapInt::iterator winsIter = normalGameStatData.find("wins");
			StatNameValueMapInt::iterator tiesIter = normalGameStatData.find("ties");
			StatNameValueMapInt::iterator lossIter = normalGameStatData.find("losses");

			if(winsIter != normalGameStatData.end() && tiesIter != normalGameStatData.end() && lossIter != normalGameStatData.end())
			{
				wins += winsIter->second;
				ties += tiesIter->second;
				losses += tiesIter->second;
			}
		}

		const char* statCatSPPrevOverallStats = "SPPreviousOverallPlayerStats";

		Stats::StatRowUpdate* statRow;
		statRow = BLAZE_NEW Stats::StatRowUpdate;
		statRow->setCategory(statCatSPPrevOverallStats);
		statRow->setEntityId(entId);
		
		writeToColumn("wins", wins, statRow);
		writeToColumn("ties", ties, statRow);
		writeToColumn("losses", losses, statRow);
				
		// Get rest of seasons overall stats data
		Stats::GetStatDescsRequest getStatsDescRequest;
		getStatsDescRequest.setCategory(statCatSPPrevOverallStats);
		Stats::StatDescs getStatsDescsResponse;
		error = statsComponent->getStatDescs(getStatsDescRequest, getStatsDescsResponse);

		if (Blaze::ERR_OK == error)
		{
			// Get category data
			const Stats::StatGroupResponse::StatDescSummaryList &statDescList = getStatsDescsResponse.getStatDescs();
			Stats::StatGroupResponse::StatDescSummaryList::const_iterator statDescItr = statDescList.begin();
			Stats::StatGroupResponse::StatDescSummaryList::const_iterator statDescItrEnd = statDescList.end();                                

			// Go through both stats data and group data lists
			if ((*statDescItr) != NULL && pSPOValues != NULL)
			{
				for(; (statDescItr != statDescItrEnd); ++statDescItr)
				{
					if (((*statDescItr) != NULL) && !(*statDescItr)->getDerived())
					{
						StatNameValueMapInt::const_iterator statMapIter = pSPOValues->find((*statDescItr)->getName());

						if(statMapIter != pSPOValues->end())
						{
							if(blaze_stricmp(statMapIter->first, "curDivision") == 0 && statMapIter->second < 1)
							{
								ERR_LOG("[EasfcSlaveImpl].writeToSPPrevOverallStats.  curDivision passed in was less than 1!  Invalid info passed. failing out!");
								error = Blaze::EASFC_ERR_INVALID_PARAMS;
								break;
							}

							writeToColumn(statMapIter->first, statMapIter->second, statRow);
						}
						else
						{
							TRACE_LOG("[EasfcSlaveImpl].writeToSPPrevOverallStats.  StatDesc Column: " << (*statDescItr)->getName() << " not found!");
						}
					}
					else
					{
						TRACE_LOG("[EasfcSlaveImpl].writeToSPPrevOverallStats.  Stat Desc Iter " << (*statDescItr)->getName() << " is either NULL or is Derived");
					}
				}
			}
			else
			{
				TRACE_LOG("[EasfcSlaveImpl].writeToSPPrevOverallStats.  Stat Desc Iter is either NULL SPOValues is NULL");
			}
		}
		else
		{
			WARN_LOG("[EasfcSlaveImpl] error - load SPPreviousOverallPlayerStats StatDesc failed (" << error << ")");
		}


		Blaze::Stats::UpdateStatsRequest request;
		request.getStatUpdates().push_back(statRow);

		UserSession::pushSuperUserPrivilege();
		error = statsComponent->updateStats(request);
		UserSession::popSuperUserPrivilege();
	}

	return error;
}

void EasfcSlaveImpl::writeToColumn(const char* name, int value, Stats::StatRowUpdate* statRow)
{
	if(statRow != NULL && name != NULL)
	{
		char8_t buffer[BUFFER_SIZE_32];
		blaze_snzprintf(buffer, BUFFER_SIZE_32, "%d", value);
		Stats::StatUpdate *statUpdate = BLAZE_NEW Stats::StatUpdate;
		statUpdate->setName(name);
		statUpdate->setUpdateType(Stats::STAT_UPDATE_TYPE_ASSIGN);
		statUpdate->setValue(buffer);
		statRow->getUpdates().push_back(statUpdate);
	}
}

void EasfcSlaveImpl::setUpgradeTitleColumn(Stats::StatRowUpdate* statRow, Stats::StatUpdate* statUpdate)
{
	statUpdate = BLAZE_NEW Stats::StatUpdate;
	statUpdate->setName("upgradeTitle");
	statUpdate->setUpdateType(Stats::STAT_UPDATE_TYPE_ASSIGN);
	statUpdate->setValue("1");
	statRow->getUpdates().push_back(statUpdate);
}


/*************************************************************************************************/
/*!
    \brief  onConfigureReplication

    lifecycle method: called when the component is ready to listen to replicated events.
    Register as a ReplicatedSeasonConfigurationMap subscriber.

*/
/*************************************************************************************************/
bool EasfcSlaveImpl::onConfigureReplication()
{
    TRACE_LOG("[EasfcSlaveImpl].onConfigureReplication");

    return true;
}


/*************************************************************************************************/
/*!
    \brief  onConfigure

    lifecycle method: called when the slave is to configure itself. registers for stats
    and usersession events. 

*/
/*************************************************************************************************/
bool EasfcSlaveImpl::onConfigure()
{
    TRACE_LOG("[EasfcSlaveImpl].onConfigure");

    return true;
}


/*************************************************************************************************/
/*!
    \brief  onReconfigure

    lifecycle method: called when the slave is to reconfigure itself. unregisters from stats
    and usersession events before calling onConfigure

*/
/*************************************************************************************************/
bool EasfcSlaveImpl::onReconfigure()
{
    TRACE_LOG("[EasfcSlaveImpl].onReconfigure");

    // stop listening to rollover and user session events as we'll re-subscribe to them in onConfigure

    return onConfigure();
}


/*************************************************************************************************/
/*!
    \brief  onResolve

    lifecycle method: called when the slave is to resolve itself. registers for master 
    notifications

*/
/*************************************************************************************************/
bool EasfcSlaveImpl::onResolve()
{
    TRACE_LOG("[EasfcSlaveImpl].onResolve");

    return true;
}


/*************************************************************************************************/
/*!
    \brief  onShutdown

    lifecycle method: called when the slave is shutting down. need to unhook our event listeners

*/
/*************************************************************************************************/
void EasfcSlaveImpl::onShutdown()
{
    TRACE_LOG("[EasfcSlaveImpl].onShutdown");
}

/*************************************************************************************************/
/*!
    \brief  getStatusINFO_LOG(

    Called periodically to collect status and statistics about this component that can be used 
    to determine what future improvements should be made.

*/
/*************************************************************************************************/
void EasfcSlaveImpl::getStatusInfo(ComponentStatus& status) const
{
    TRACE_LOG("[EasfcSlaveImpl].getStatusInfo");

    mMetrics.report(&status);
}


/*************************************************************************************************/
/*!
    \brief  onUserSessionExistence

    called when a user logs into the slave. checks if we should register the user in
    seasonal play and does so if needed.

*/
/*************************************************************************************************/
void EasfcSlaveImpl::onUserSessionExistence(const UserSession& userSession)
{
	//TRACE_LOG("[EasfcSlaveImpl].onUserSessionLogin. mAutoUserRegistrationEnabled "<<mAutoUserRegistrationEnabled<<"  user.isLocal() "<<user.isLocal()<<" CLIENT_TYPE_CONSOLE_USER == user.getClientType() "<<CLIENT_TYPE_GAMEPLAY_USER == user.getClientType()<<" " );

    // only care about this login event if the user is logged into this slave from a console
    if (true == userSession.isLocal() && 
        CLIENT_TYPE_GAMEPLAY_USER == userSession.getClientType())
    {
        TRACE_LOG("[EasfcSlaveImpl].verifyUserRegistration. User logged in. User id = "<<userSession.getUserId() );
    }
}


/*************************************************************************************************/
/*!
    \brief  isUpgradeTitle

    Does the work of checking if a user has upgraded their title from hd to kc based on stat category

*/
/*************************************************************************************************/
bool EasfcSlaveImpl::isUpgradeTitle(EntityId id, const char* statCategoryName)
{
	Blaze::BlazeRpcError error = Blaze::ERR_OK;
	bool retval = true;		// Setting default to true to avoid any potential stat overwrite by mistake.
	
	Stats::UpdateStatsRequestBuilder builder;
	builder.startStatRow(statCategoryName, id, NULL);
	builder.selectStat("upgradeTitle");
	builder.completeStatRow();

	Stats::UpdateStatsHelper statHelper;
	
	error = statHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)builder);
	
	if (error == Blaze::ERR_OK)	// Checking the return status of the statHelper.initializeStatUpdate call
	{
		error = statHelper.fetchStats();
		
		if (error == Blaze::ERR_OK)  // Checking the return status of the statHelper.fetchStats call
		{
			Stats::UpdateRowKey* key = builder.getUpdateRowKey(statCategoryName, id);
			int64_t upgradeTitle = statHelper.getValueInt(key, "upgradeTitle", Stats::STAT_PERIOD_ALL_TIME, true);
			
			retval = (upgradeTitle == 1);
		}
		else
		{
			TRACE_LOG("[EasfcSlaveImpl].isUpgradeTitle. Error in calling statHelper.fetchStats" );
		}
	}
	else
	{
		TRACE_LOG("[EasfcSlaveImpl].isUpgradeTitle. Error in calling statHelper.initializeStatUpdate" );
	}

	return retval;
}



} // Easfc
} // Blaze
