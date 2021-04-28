/*************************************************************************************************/
/*!
	\file   ssfseasonsreportslave.cpp

	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "ssfseasonsreportslave.h"


namespace Blaze
{

namespace GameReporting
{

namespace FIFA
{

/*************************************************************************************************/
/*! \brief SSFSeasonsReportSlave
	Constructor
*/
/*************************************************************************************************/
SSFSeasonsReportSlave::SSFSeasonsReportSlave(GameReportingSlaveImpl& component) :
SSFBaseReportSlave(component)
{
}
/*************************************************************************************************/
/*! \brief SSFSeasonsReportSlave
	Destructor
*/
/*************************************************************************************************/
SSFSeasonsReportSlave::~SSFSeasonsReportSlave()
{
}

/*************************************************************************************************/
/*! \brief SSFSeasonsReportSlave
	Create

	\return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* SSFSeasonsReportSlave::create(GameReportingSlaveImpl& component)
{
	return BLAZE_NEW_NAMED("SSFSeasonsReportSlave") SSFSeasonsReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomSquadGameTypeName
	Create

	\return GameReportProcessor pointer
*/
/*************************************************************************************************/
const char8_t* SSFSeasonsReportSlave::getCustomSquadGameTypeName() const
{
	return "gameType33";
}

/*************************************************************************************************/
/*! \brief setCustomEndGameMailParam
	A custom hook for Game team to set parameters for the end game mail, return true if sending a mail

	\param mailParamList - the parameter list for the mail to send
	\param mailTemplateName - the template name of the email
	\return bool - true if to send an end game email
*/
/*************************************************************************************************/
/*
bool SSFSeasonsReportSlave::setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName)
{
	return false;
}
*/
void SSFSeasonsReportSlave::selectCustomStats()
{
	mFifaLastOpponent.selectLastOpponentStats();
}

void SSFSeasonsReportSlave::updateCustomTransformStats()
{
	mFifaLastOpponent.transformLastOpponentStats();
}

void SSFSeasonsReportSlave::updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
	TRACE_LOG("[SSFSeasonsReportSlave:" << this << "].updateCustomPlayerStats()for player " << playerId << " ");
	SSFSeasonsReportBase::SSFSeasonsPlayerReport* ssfSeasonsPlayerReport = static_cast<SSFSeasonsReportBase::SSFSeasonsPlayerReport*>(playerReport.getCustomPlayerReport());

	updateCleanSheets(ssfSeasonsPlayerReport->getPos(), ssfSeasonsPlayerReport->getTeamEntityId(), ssfSeasonsPlayerReport);
	updatePlayerRating(&playerReport, ssfSeasonsPlayerReport);
	updateMOM(&playerReport, ssfSeasonsPlayerReport);

	ssfSeasonsPlayerReport->setSsfEndResult(SSFGameResultHelper<SSFReportSlave>::DetermineSSFUserResult(playerId, playerReport, false));
}

void SSFSeasonsReportSlave::updateCustomSquadStats(OSDKGameReportBase::OSDKReport& OsdkReport)
{
	SSFSeasonsReportBase::SSFTeamSummaryReport* ssfTeamSummaryReport = static_cast<SSFSeasonsReportBase::SSFTeamSummaryReport*>(OsdkReport.getTeamReports());
	SSFSeasonsReportBase::SSFTeamReportMap& ssfTeamReportMap = ssfTeamSummaryReport->getSSFTeamReportMap();

	SSFSeasonsReportBase::SSFTeamReportMap::iterator iter, end;
	iter = ssfTeamReportMap.begin();
	end = ssfTeamReportMap.end();
	for (; iter != end; iter++)
	{
//		//TODO log team gamesplayed stat
//		//int64_t teamEntityId = iter->first;
//		//mBuilder.startStatRow(CATEGORYNAME_SEASONALPLAY_OVR, teamEntityId);
//		//mBuilder.incrementStat("gamesPlayed", 1);
//		//mBuilder.completeStatRow();
	}
}

void SSFSeasonsReportSlave::updateCleanSheets(uint32_t pos, int64_t entityId, SSFSeasonsReportBase::SSFSeasonsPlayerReport* ssfSeasonsPlayerReport)
{
	uint16_t cleanSheetsAny = 0;
	uint16_t cleanSheetsDef = 0;
	uint16_t cleanSheetsGK = 0;

	if (pos == POS_ANY || pos == POS_DEF || pos == POS_GK)
	{
		uint32_t locScore = 0;
		int64_t locTeam = 0;
		uint32_t oppScore = 0;
		int64_t oppTeam = 0;

		locScore = mLowestScore;
		locTeam = entityId;
		oppScore = mHighestScore;
		oppTeam = mWinEntityId;

		if (locTeam == oppTeam)
		{
			oppScore = locScore;
		}
		if (!oppScore)
		{
			if (pos == POS_ANY)
			{
				cleanSheetsAny = 1;
			}
			else if (pos == POS_DEF)
			{
				cleanSheetsDef = 1;
			}
			else
			{
				cleanSheetsGK = 1;
			}
		}
	}

	ssfSeasonsPlayerReport->setCleanSheetsAny(cleanSheetsAny);
	ssfSeasonsPlayerReport->setCleanSheetsDef(cleanSheetsDef);
	ssfSeasonsPlayerReport->setCleanSheetsGoalKeeper(cleanSheetsGK);

	TRACE_LOG("[SSFSeasonsReportSlave:" << this << "].updateCleanSheets() ANY:" << cleanSheetsAny << " DEF:" << cleanSheetsDef << " GK:" << cleanSheetsGK << " ");
}

void SSFSeasonsReportSlave::updatePlayerRating(OSDKGameReportBase::OSDKPlayerReport* playerReport, SSFSeasonsReportBase::SSFSeasonsPlayerReport* ssfSeasonsPlayerReport)
{
	const int DEF_PLAYER_RATING = 3;

	//TODO: get from private data or not?
	//Collections::AttributeMap& map = playerReport->getPrivatePlayerReport().getPrivateAttributeMap();

	float rating = ssfSeasonsPlayerReport->getCommonPlayerReport().getRating();
	if (rating <= 0.0f)
	{
		rating = DEF_PLAYER_RATING;
	}

	ssfSeasonsPlayerReport->getCommonPlayerReport().setRating(rating);

	TRACE_LOG("[SSFSeasonsReportSlave:" << this << "].updatePlayerRating() rating:" << ssfSeasonsPlayerReport->getCommonPlayerReport().getRating() << " ");
}

void SSFSeasonsReportSlave::updateMOM(OSDKGameReportBase::OSDKPlayerReport* playerReport, SSFSeasonsReportBase::SSFSeasonsPlayerReport* ssfSeasonsPlayerReport)
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKGameReport& osdkGameReport = osdkReport.getGameReport();
	SSFSeasonsReportBase::SSFSeasonsGameReport* ssfSeasonsGameReport = static_cast<SSFSeasonsReportBase::SSFSeasonsGameReport*>(osdkGameReport.getCustomGameReport());

	eastl::string playerName = playerReport->getName();
	eastl::string mom = ssfSeasonsGameReport->getMom();
	
	uint16_t isMOM = 0;
	if (playerName == mom)
	{
		isMOM = 1;
	}

	ssfSeasonsPlayerReport->setManOfTheMatch(isMOM);

	TRACE_LOG("[SSFSeasonsReportSlave:" << this << "].updateMOM() isMOM:" << isMOM << " ");
}

void SSFSeasonsReportSlave::updateNotificationReport()
{
	// Obtain the custom data for report notification
	OSDKGameReportBase::OSDKNotifyReport *OsdkReportNotification = static_cast<OSDKGameReportBase::OSDKNotifyReport*>(mProcessedReport->getCustomData());
	Blaze::GameReporting::SSFSeasonsReportBase::SSFNotificationCustomGameData *gameCustomData = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SSFNotificationCustomGameData();

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	SSFSeasonsReportBase::SSFTeamSummaryReport* ssfTeamSummaryReport = static_cast<SSFSeasonsReportBase::SSFTeamSummaryReport*>(osdkReport.getTeamReports());
	SSFSeasonsReportBase::SSFSeasonsGameReport* ssfSeasonsGameReport = static_cast<SSFSeasonsReportBase::SSFSeasonsGameReport*>(osdkReport.getGameReport().getCustomGameReport());

	gameCustomData->setSecondsPlayed(osdkReport.getGameReport().getGameTime());
	gameCustomData->setMatchEndReason(osdkReport.getGameReport().getFinishedStatus());

	int64_t matchStatusHome = 0;
	int64_t matchStatusAway = 0;

	const OSDKReport::OSDKPlayerReportsMap& playerReportMap = osdkReport.getPlayerReports();
	for (const OSDKReport::OSDKPlayerReportsMap::value_type playerReportEntry : playerReportMap)
	{
		const OSDKPlayerReport * playerReport = playerReportEntry.second;
		
		Blaze::GameReporting::SSFSeasonsReportBase::SSFNotificationUserReportPtr notifyUserReport = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SSFNotificationUserReport();
		notifyUserReport->setPersonaId( playerReportEntry.first );
		notifyUserReport->setTeamSide(playerReport->getHome() ? 0 : 1);
		notifyUserReport->setSsfEndResult((static_cast<const SSFSeasonsReportBase::SSFSeasonsPlayerReport*>(playerReport->getCustomPlayerReport()))->getSsfEndResult());
		gameCustomData->getSSFUserReport().push_back(notifyUserReport);

		const OSDKGameReportBase::IntegerAttributeMap& privateIntMap = playerReport->getPrivatePlayerReport().getPrivateIntAttributeMap();
		OSDKGameReportBase::IntegerAttributeMap::const_iterator iter = privateIntMap.find("matchStatus");
		if (iter != privateIntMap.end())
		{
			if (playerReport->getHome())
			{
				matchStatusHome = iter->second;
			}
			else
			{
				matchStatusAway = iter->second;
			}
		}
	}

	eastl::string matchStatusStr;
	matchStatusStr.sprintf("%" PRId64 "_%" PRId64 "", matchStatusHome, matchStatusAway);
	gameCustomData->setMatchHash(matchStatusStr.c_str());


	for (SSFSeasonsReportBase::SSFTeamReportMap::value_type &teamReportMapEntry : ssfTeamSummaryReport->getSSFTeamReportMap())
	{
		SSFSeasonsReportBase::SSFTeamReport& teamReport = *(teamReportMapEntry.second);
		Blaze::GameReporting::SSFSeasonsReportBase::SSFNotificationTeamReportPtr notifyTeamReport = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SSFNotificationTeamReport();
		
		notifyTeamReport->setTeamId(teamReportMapEntry.first);
		notifyTeamReport->setGoals(teamReportMapEntry.second->getScore());
		notifyTeamReport->setPlayOfTheMatch(isManOfMatchOnTheTeam(teamReport));

		for (SSFSeasonsReportBase::AvatarEntry* avatar : teamReport.getAvatarVector())
		{
			Blaze::GameReporting::SSFSeasonsReportBase::SSFNotificationAvatarStatsPtr notifyAvatarReport = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SSFNotificationAvatarStats();
			avatar->getAvatarId().copyInto(notifyAvatarReport->getAvatarId());
			notifyAvatarReport->setGoals(avatar->getAvatarStatReport().getGoals());
			notifyAvatarReport->setAssists(avatar->getAvatarStatReport().getAssists());
			notifyAvatarReport->setShots(avatar->getAvatarStatReport().getShots());
			notifyAvatarReport->setPasses(avatar->getAvatarStatReport().getPassesMade());
			notifyAvatarReport->setTackles(avatar->getAvatarStatReport().getTacklesMade());
			notifyAvatarReport->setBlocks(avatar->getAvatarStatReport().getBlocks());
			notifyAvatarReport->setAvatarRating(avatar->getAvatarStatReport().getRating());
			notifyAvatarReport->setAvatarType(-1); //SSFTODO 
			avatar->getSkillTimeBucketMap().copyInto(notifyAvatarReport->getSkillTimeBucketMap());
			notifyTeamReport->getAvatarVector().push_back(notifyAvatarReport);
		}

		int32_t teamSide = teamReport.getHome() ? 0 : 1;
		gameCustomData->getSSFTeamReport().insert(eastl::make_pair(teamSide, notifyTeamReport));

	}
	ssfSeasonsGameReport->getGoalSummary().copyInto(gameCustomData->getGoalSummary());

	// Set Report Id.
	gameCustomData->setGameReportingId(mProcessedReport->getGameReport().getGameReportingId());

	// Set the gameCustomData to the OsdkReportNotification
	OsdkReportNotification->setCustomDataReport(*gameCustomData);
}

int32_t SSFSeasonsReportSlave::GetTeamGoals(SSFSeasonsReportBase::GoalEventVector& goalEvents, int64_t teamId) const
{
	int32_t goals = 0;
	for (SSFSeasonsReportBase::GoalEvent *goalEvent : goalEvents)
	{
		if (goalEvent->getScoringTeam() == teamId)
		{
			goals++;
		}
	}
	return goals;
}
bool SSFSeasonsReportSlave::isManOfMatchOnTheTeam(SSFSeasonsReportBase::SSFTeamReport& teamReport) const
{
	bool isMom = false;
	for (SSFSeasonsReportBase::AvatarEntry* avatar : teamReport.getAvatarVector())
	{
		isMom = isMom || avatar->getAvatarStatReport().getHasMOTM();
	}
	return isMom;
}

void SSFSeasonsReportSlave::updateGameStats()
{
	HeadToHeadLastOpponent::ExtensionData data;
	data.mProcessedReport = mProcessedReport;
	mH2HLastOpponentExtension.setExtensionData(&data);
	mFifaLastOpponent.setExtension(&mH2HLastOpponentExtension);
	mFifaLastOpponent.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport);
}
} // namespace FIFA
} // namespace GameReporting
}// namespace Blaze
