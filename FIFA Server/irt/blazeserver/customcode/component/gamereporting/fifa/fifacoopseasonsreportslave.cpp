/*************************************************************************************************/
/*!
    \file   fifacoopseasonsreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "fifacoopseasonsreportslave.h"

#define CATEGORYNAME_COOPRANKSTATS			"CoopRankStats"
#define CATEGORYNAME_SEASONALPLAY_OVR		"SPOverallCoopStats"

#define STATNAME_MEMBER_A					"MemberA"
#define STATNAME_MEMBER_A_WINS				"MemberA_wins"
#define STATNAME_MEMBER_A_TIES				"MemberA_ties"
#define STATNAME_MEMBER_A_LOSS				"MemberA_losses"

#define STATNAME_MEMBER_B					"MemberB"
#define STATNAME_MEMBER_B_WINS				"MemberB_wins"
#define STATNAME_MEMBER_B_TIES				"MemberB_ties"
#define STATNAME_MEMBER_B_LOSS				"MemberB_losses"

namespace Blaze
{

namespace GameReporting
{

namespace FIFA
{

/*************************************************************************************************/
/*! \brief FifaCoopSeasonsReportSlave
    Constructor
*/
/*************************************************************************************************/
FifaCoopSeasonsReportSlave::FifaCoopSeasonsReportSlave(GameReportingSlaveImpl& component) :
FifaCoopBaseReportSlave(component)
{
	mFifaSeasonalPlay.setExtension(&mFifaSeasonalPlayExtension);
	mEloRpgHybridSkill.setExtension(&mEloRpgHybridSkillExtension);
}
/*************************************************************************************************/
/*! \brief FifaCoopSeasonsReportSlave
    Destructor
*/
/*************************************************************************************************/
FifaCoopSeasonsReportSlave::~FifaCoopSeasonsReportSlave()
{
}

/*************************************************************************************************/
/*! \brief FifaCoopSeasonsReportSlave
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* FifaCoopSeasonsReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("FifaCoopSeasonsReportSlave") FifaCoopSeasonsReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomClubGameTypeName
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
const char8_t* FifaCoopSeasonsReportSlave::getCustomSquadGameTypeName() const
{
    return "gameType25";
}

/*************************************************************************************************/
/*! \brief setCustomClubEndGameMailParam
    A custom hook for Game team to set parameters for the end game mail, return true if sending a mail

    \param mailParamList - the parameter list for the mail to send
    \param mailTemplateName - the template name of the email
    \return bool - true if to send an end game email
*/
/*************************************************************************************************/
/*
bool FifaCoopSeasonsReportSlave::setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName)
{
    // Ping doesn't send any Club end game email
    return false;
}
*/

void FifaCoopSeasonsReportSlave::selectCustomStats()
{
	//Seasonal Play
	CoopSeasonsExtension::ExtensionData seasonData;
	seasonData.mProcessedReport = mProcessedReport;
	seasonData.mWinCoopId = mWinCoopId;
	seasonData.mLossCoopId = mLossCoopId;
	seasonData.mTieGame = mTieGame;
	mFifaSeasonalPlayExtension.setExtensionData(&seasonData);
	mFifaSeasonalPlay.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport);

	mFifaSeasonalPlay.updateDivCounts();
	mFifaSeasonalPlay.selectSeasonalPlayStats();

	//Elo Hybrid Skill
	CoopEloExtension::ExtensionData data;
	data.mProcessedReport = mProcessedReport;
	data.mUpdateClubsUtil = &mUpdateClubsUtil;

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	FifaCoopReportBase::FifaCoopSquadReport* squadReports = static_cast<FifaCoopReportBase::FifaCoopSquadReport*>(osdkReport.getTeamReports());
	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap& squadReportMap = squadReports->getSquadReports();
	
	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap::iterator squadIter, squadEnd;
	squadIter = squadReportMap.begin();
	squadEnd = squadReportMap.end();

	for (; squadIter != squadEnd; ++squadIter)
	{
		CoopSeason::CoopId id = squadIter->first;
		FifaCoopReportBase::FifaCoopSquadDetailsReportBase* fifaCoopSquadDetailsReportBase = squadIter->second;

		uint32_t statPerc = calcSquadDampingPercent(id);
		int32_t disc = fifaCoopSquadDetailsReportBase->getSquadDisc();
		uint32_t result = determineResult(id, *fifaCoopSquadDetailsReportBase);

		data.mCalculatedStats[id].stats[CoopEloExtension::ExtensionData::STAT_PERC] = statPerc;
		data.mCalculatedStats[id].stats[CoopEloExtension::ExtensionData::DISCONNECT] = disc;
		data.mCalculatedStats[id].stats[CoopEloExtension::ExtensionData::RESULT] = result;

	}

	mEloRpgHybridSkillExtension.setExtensionData(&data);

	mEloRpgHybridSkill.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport, mReportConfig);
	mEloRpgHybridSkill.selectEloStats();
}

void FifaCoopSeasonsReportSlave::updateCustomTransformStats()
{
	mFifaSeasonalPlay.transformSeasonalPlayStats();
	mEloRpgHybridSkill.transformEloStats();
}

void FifaCoopSeasonsReportSlave::updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
	TRACE_LOG("[GameClubReportSlave:" << this << "].updateCustomPlayerStats()for player " << playerId << " ");

	FifaCoopReportBase::FifaCoopPlayerReportBase* fifaCoopPlayerReportBase = static_cast<FifaCoopReportBase::FifaCoopPlayerReportBase*>(playerReport.getCustomPlayerReport());
	FifaCoopSeasonsReport::FifaCoopSeasonsPlayerReport* fifaCoopSeasonsPlayerReport =  static_cast<FifaCoopSeasonsReport::FifaCoopSeasonsPlayerReport*>(fifaCoopPlayerReportBase->getCustomCoopPlayerReport());

	updateSoloStats(playerId, playerReport);

	updateCleanSheets(fifaCoopPlayerReportBase->getPos(), fifaCoopPlayerReportBase->getCoopId(), fifaCoopSeasonsPlayerReport);
	updatePlayerRating(&playerReport, fifaCoopSeasonsPlayerReport);
	updateMOM(&playerReport, fifaCoopSeasonsPlayerReport);
}

void FifaCoopSeasonsReportSlave::updateCustomSquadStats(OSDKGameReportBase::OSDKReport& OsdkReport)
{
	FifaCoopReportBase::FifaCoopSquadReport& squadReport = static_cast<FifaCoopReportBase::FifaCoopSquadReport&>(*OsdkReport.getTeamReports());
	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap& fifaCoopSquadReportMap = squadReport.getSquadReports();

	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap::iterator iter, end;
	iter = fifaCoopSquadReportMap.begin();
	end = fifaCoopSquadReportMap.end();
	for (; iter != end; iter++)
	{
		CoopSeason::CoopId coopId = iter->first;
		mBuilder.startStatRow(CATEGORYNAME_SEASONALPLAY_OVR, coopId);
		mBuilder.incrementStat("gamesPlayed", 1);
		mBuilder.completeStatRow();
	}
}

void FifaCoopSeasonsReportSlave::updateSoloStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{

	//verify if this player is a solo coop.
	bool isSolo = mIsHomeCoopSolo;
	BlazeId lowerBlazeId = mHomeLowerBlazeId;

	if (playerReport.getHome() == 0)
	{
		isSolo = mIsAwayCoopSolo;
		lowerBlazeId = mAwayLowerBlazeId;
	}

	if (isSolo)
	{
		writeSoloStats(CATEGORYNAME_COOPRANKSTATS, playerId, playerReport, lowerBlazeId == playerId);
		writeSoloStats(CATEGORYNAME_SEASONALPLAY_OVR, playerId, playerReport, lowerBlazeId == playerId);
	}
}

void FifaCoopSeasonsReportSlave::writeSoloStats(const char* statsCategory, GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport, bool isLowerBlazeId)
{
	FifaCoopReportBase::FifaCoopPlayerReportBase* playerReportBase = static_cast<FifaCoopReportBase::FifaCoopPlayerReportBase*>(playerReport.getCustomPlayerReport());

	const char* blazeMember = STATNAME_MEMBER_A;
	const char* blazeMemberWin = STATNAME_MEMBER_A_WINS;
	const char* blazeMemberTie = STATNAME_MEMBER_A_TIES;
	const char* blazeMemberLoss = STATNAME_MEMBER_A_LOSS;
	if (isLowerBlazeId)
	{
		blazeMember = STATNAME_MEMBER_B;
		blazeMemberWin = STATNAME_MEMBER_B_WINS;
		blazeMemberTie = STATNAME_MEMBER_B_TIES;
		blazeMemberLoss = STATNAME_MEMBER_B_LOSS;
	}

	mBuilder.startStatRow(statsCategory, playerReportBase->getCoopId());

	mBuilder.assignStat(blazeMember, playerId);		
	if (mTieGame)
	{
		mBuilder.incrementStat(blazeMemberTie, 1);
	}
	else if (mWinCoopId == playerReportBase->getCoopId())
	{
		mBuilder.incrementStat(blazeMemberWin, 1);
	}
	else
	{
		mBuilder.incrementStat(blazeMemberLoss, 1);	
	}

	mBuilder.completeStatRow();
}

void FifaCoopSeasonsReportSlave::updateCleanSheets(uint32_t pos, CoopSeason::CoopId coopId, FifaCoopSeasonsReport::FifaCoopSeasonsPlayerReport* fifaCoopSeasonsPlayerReport)
{
	uint16_t cleanSheetsAny = 0;
	uint16_t cleanSheetsDef = 0;
	uint16_t cleanSheetsGK = 0;

	if (pos == POS_ANY || pos == POS_DEF || pos == POS_GK)
	{
		uint32_t locScore = 0;
		CoopSeason::CoopId locTeam = 0;
		uint32_t oppScore = 0;
		CoopSeason::CoopId oppTeam = 0;

		locScore = mLowestCoopScore;
		locTeam = coopId;
		oppScore = mHighestCoopScore;
		oppTeam = mWinCoopId;

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

	fifaCoopSeasonsPlayerReport->setCleanSheetsAny(cleanSheetsAny);
	fifaCoopSeasonsPlayerReport->setCleanSheetsDef(cleanSheetsDef);
	fifaCoopSeasonsPlayerReport->setCleanSheetsGoalKeeper(cleanSheetsGK);

	TRACE_LOG("[FifaCoopSeasonsReportSlave:" << this << "].updateCleanSheets() ANY:" << cleanSheetsAny << " DEF:" << cleanSheetsDef << " GK:" << cleanSheetsGK << " ");
}

void FifaCoopSeasonsReportSlave::updatePlayerRating(OSDKGameReportBase::OSDKPlayerReport* playerReport, FifaCoopSeasonsReport::FifaCoopSeasonsPlayerReport* fifaCoopSeasonsPlayerReport)
{
	const int DEF_PLAYER_RATING = 3;

	//Collections::AttributeMap& map = playerReport->getPrivatePlayerReport().getPrivateAttributeMap();

	float rating = 0.0f;//static_cast<float>(atof(map["VProRating"]));
	if (rating <= 0.0f)
	{
		rating = DEF_PLAYER_RATING;
	}

	fifaCoopSeasonsPlayerReport->getCommonPlayerReport().setRating(rating);

	TRACE_LOG("[FifaCoopSeasonsReportSlave:" << this << "].updatePlayerRating() rating:" << fifaCoopSeasonsPlayerReport->getCommonPlayerReport().getRating() << " ");
}

void FifaCoopSeasonsReportSlave::updateMOM(OSDKGameReportBase::OSDKPlayerReport* playerReport, FifaCoopSeasonsReport::FifaCoopSeasonsPlayerReport* fifaCoopSeasonsPlayerReport)
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKGameReport osdkGameReport = osdkReport.getGameReport();
	FifaCoopReportBase::FifaCoopGameReportBase* fifaCoopGameReportBase = static_cast<FifaCoopReportBase::FifaCoopGameReportBase*>(osdkGameReport.getCustomGameReport());
	FifaCoopSeasonsReport::FifaCoopSeasonsGameReport* fifaCoopSeasonsGameReport = static_cast<FifaCoopSeasonsReport::FifaCoopSeasonsGameReport*>(fifaCoopGameReportBase->getCustomCoopGameReport());

	eastl::string playerName = playerReport->getName();
	eastl::string mom = fifaCoopSeasonsGameReport->getMom();
	
	uint16_t isMOM = 0;
	if (playerName == mom)
	{
		isMOM = 1;
	}

	fifaCoopSeasonsPlayerReport->setManOfTheMatch(isMOM);

	TRACE_LOG("[FifaCoopSeasonsReportSlave:" << this << "].updateMOM() isMOM:" << isMOM << " ");
}

uint32_t FifaCoopSeasonsReportSlave::determineResult(CoopSeason::CoopId id, FifaCoopReportBase::FifaCoopSquadDetailsReportBase& squadDetailsReport)
{
	//first determine if the user won, lost or draw
	uint32_t resultValue = 0;
	if (mWinCoopId == id)
	{
		resultValue = WLT_WIN;
	}
	else if (mLossCoopId == id)
	{
		resultValue = WLT_LOSS;
	}
	else
	{
		resultValue = WLT_TIE;
	}

	if (squadDetailsReport.getSquadDisc() == 1)
	{
		resultValue |= WLT_DISC;
	}

	// Track the win-by-dnf
	if (id == mWinCoopId && !mCoopGameFinish)
	{
		resultValue |= WLT_OPPOQCTAG;
	}

	return resultValue;
}

} // namespace FIFA
} // namespace GameReporting
}// namespace Blaze
