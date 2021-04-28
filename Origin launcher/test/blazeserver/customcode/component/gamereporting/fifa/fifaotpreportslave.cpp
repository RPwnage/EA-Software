/*************************************************************************************************/
/*!
    \file   fifaotpreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "fifaotpreportslave.h"
#include "customcode/component/gamereporting/fifa/fifacustomclub.h"
#include "customcode/component/gamereporting/osdk/tdf/gameosdkreport.h"

namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief FifaOtpReportSlave
    Constructor
*/
/*************************************************************************************************/
FifaOtpReportSlave::FifaOtpReportSlave(GameReportingSlaveImpl& component) :
GameOTPReportSlave(component)
{
}
/*************************************************************************************************/
/*! \brief FifaOtpReportSlave
    Destructor
*/
/*************************************************************************************************/
FifaOtpReportSlave::~FifaOtpReportSlave()
{
}

/*************************************************************************************************/
/*! \brief FifaOtpReportSlave
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* FifaOtpReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("FifaOtpReportSlave") FifaOtpReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomOTPGameTypeName
    Return the game type name for OTP game used in gamereporting.cfg

    \return - the OTP game type name used in gamereporting.cfg
*/
/*************************************************************************************************/
const char8_t* FifaOtpReportSlave::getCustomOTPGameTypeName() const
{
    return "gameType5";
}

/*************************************************************************************************/
/*! \brief FifaOtpReportSlave
    Return the Stats Category name which the game report updates for

    \return - the stats category needs to be updated for OTP game
*/
/*************************************************************************************************/
const char8_t* FifaOtpReportSlave::getCustomOTPStatsCategoryName() const
{
    return "ClubOTPPlayerStats";
}

/*************************************************************************************************/
/*! \brief setCustomOTPEndGameMailParam
    A custom hook for Game team to set parameters for the end game mail, return true if sending a mail

    \param mailParamList - the parameter list for the mail to send
    \param mailTemplateName - the template name of the email
    \return bool - true if to send an end game email
*/
/*************************************************************************************************/
/*
bool FifaOtpReportSlave::setCustomOTPEndGameMailParam(Mail::HttpParamList* mailParamList, const char8_t* mailTemplateName)
{
    // Ping doesn't send any OTP end game email
    return false;
}
*/
void FifaOtpReportSlave::updatePlayerStats()
{
	TRACE_LOG("[FifaOtpReportSlave:" << this << "].updatePlayerStats()");

	// aggregate players' stats
	aggregatePlayerStats();

	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
	playerIter = OsdkPlayerReportsMap.begin();
	playerEnd = OsdkPlayerReportsMap.end();

	GameType::StatUpdate statUpdate;

	for(; playerIter != playerEnd; ++playerIter)
	{
		GameManager::PlayerId playerId = playerIter->first;
		OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;

		if (mTieGame)
		{
			updateGameModeTiePlayerStats(playerId, playerReport);
		}
		else if (mWinnerSet.find(playerId) != mWinnerSet.end())
		{
			updateGameModeWinPlayerStats(playerId, playerReport);
		}
		else if (mLoserSet.find(playerId) != mLoserSet.end())
		{
			updateGameModeLossPlayerStats(playerId, playerReport);
		}

		// update common player's stats
		updateCommonStats(playerId, playerReport);
	}
}

void FifaOtpReportSlave::updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
	playerReport.setPointsAgainst(mAggregatedPlayerStatsMap[AGGREGATE_SCORE] - playerReport.getScore());
	playerReport.setOpponentCount(mPlayerReportMapSize - 1);

	FifaOTPReportBase::PlayerReport& fifaOtpPlayerReport = static_cast<FifaOTPReportBase::PlayerReport&>(*playerReport.getCustomPlayerReport());
	fifaOtpPlayerReport.setOtpGames(1);

	// update player's DNF stat
	updatePlayerDNF(playerReport);

	updateCustomPlayerStats(playerId, playerReport);
}

void FifaOtpReportSlave::updateGameModeWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
	updateWinPlayerStats(playerId, playerReport);

	if (true == didAllPlayersFinish())
	{
		playerReport.setWinnerByDnf(0);
	}
	else
	{
		playerReport.setWinnerByDnf(1);
	}
}

void FifaOtpReportSlave::updateGameModeLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
	updateLossPlayerStats(playerId, playerReport);
	playerReport.setWinnerByDnf(0);
}

void FifaOtpReportSlave::updateGameModeTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
	updateTiePlayerStats(playerId, playerReport);
	playerReport.setWinnerByDnf(0);
}

void FifaOtpReportSlave::selectCustomStats()
{
	mFifaVpro.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport);
	mFifaVpro.setExtension(&mFifaVproExtension);

	FifaVpro::CategoryInfoVector categoryVector;

	FifaVpro::CategoryInfo clubOTPPlayer;
	clubOTPPlayer.statCategory = "ClubOTPPlayerStats";
	clubOTPPlayer.keyScopeType = FifaVpro::KEYSCOPE_POS;
	categoryVector.push_back(clubOTPPlayer);

	mFifaVpro.updateVproStats(categoryVector);
	
	//-------------------------------------------------------
	categoryVector.clear();

	FifaVpro::CategoryInfo vProCategory;
	vProCategory.statCategory = "VProStats";
	vProCategory.keyScopeType = FifaVpro::KEYSCOPE_NONE;
	categoryVector.push_back(vProCategory);

	mFifaVpro.selectStats(categoryVector);
}

void FifaOtpReportSlave::updateCustomTransformStats()
{
	FifaVpro::CategoryInfoVector categoryVector;
	FifaVpro::CategoryInfo vProCategory;
	vProCategory.statCategory = "VProStats";
	vProCategory.keyScopeType = FifaVpro::KEYSCOPE_NONE;
	categoryVector.push_back(vProCategory);

	mFifaVpro.updateVproGameStats(categoryVector);
}

void FifaOtpReportSlave::updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
	TRACE_LOG("[FifaOtpReportSlave:" << this << "].updateCustomPlayerStats()for player");


	FifaOTPReportBase::PlayerReport* fifaPlayerReport = static_cast<FifaOTPReportBase::PlayerReport*>(playerReport.getCustomPlayerReport());
	
	updateCleanSheets(fifaPlayerReport->getPos(), &playerReport, fifaPlayerReport);
	updatePlayerRating(&playerReport, fifaPlayerReport);
	updateMOM(&playerReport, fifaPlayerReport);
}

void FifaOtpReportSlave::updateCleanSheets(uint32_t pos, OSDKGameReportBase::OSDKPlayerReport* playerReport, FifaOTPReportBase::PlayerReport* fifaPlayerReport)
{
	uint16_t cleanSheetsAny = 0;
	uint16_t cleanSheetsDef = 0;
	uint16_t cleanSheetsGK = 0;

	if (pos == FIFA::POS_ANY || pos == FIFA::POS_DEF || pos == FIFA::POS_GK)
	{
		uint32_t locScore = 0;
		uint32_t locTeam = 0;
		uint32_t oppScore = 0;
		uint32_t oppTeam = 0;

		locScore = mLowestPlayerScore;
		locTeam = playerReport->getTeam();
		oppScore = mHighestPlayerScore;
		oppTeam = mWinTeamId;

		if (locTeam == oppTeam)
		{
			oppScore = locScore;
		}
		if (!oppScore)
		{
			if (pos == FIFA::POS_ANY)
			{
				cleanSheetsAny = 1;
			}
			else if (pos == FIFA::POS_DEF)
			{
				cleanSheetsDef = 1;
			}
			else
			{
				cleanSheetsGK = 1;
			}
		}
	}

	fifaPlayerReport->setCleanSheetsAny(cleanSheetsAny);
	fifaPlayerReport->setCleanSheetsDef(cleanSheetsDef);
	fifaPlayerReport->setCleanSheetsGoalKeeper(cleanSheetsGK);

	TRACE_LOG("[FifaOtpReportSlave:" << this << "].updateCleanSheets() ANY:" << cleanSheetsAny << " DEF:" << cleanSheetsDef << " GK:" << cleanSheetsGK << " ");
}

void FifaOtpReportSlave::updatePlayerRating(OSDKGameReportBase::OSDKPlayerReport* playerReport, FifaOTPReportBase::PlayerReport* fifaPlayerReport)
{
	const int DEF_PLAYER_RATING = 3;

	Collections::AttributeMap& map = playerReport->getPrivatePlayerReport().getPrivateAttributeMap();

	float rating = static_cast<float>(atof(map["VProRating"]));
	if (rating <= 0.0f)
	{
		rating = DEF_PLAYER_RATING;
	}

	fifaPlayerReport->getCommonPlayerReport().setRating(rating);

	TRACE_LOG("[FifaOtpReportSlave:" << this << "].updatePlayerRating() rating:" << fifaPlayerReport->getCommonPlayerReport().getRating() << " ");
}

void FifaOtpReportSlave::updateMOM(OSDKGameReportBase::OSDKPlayerReport* playerReport, FifaOTPReportBase::PlayerReport* fifaPlayerReport)
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKGameReport osdkGameReport = osdkReport.getGameReport();
	FifaOTPReportBase::GameReport* fifaGameReport = static_cast<FifaOTPReportBase::GameReport*>(osdkGameReport.getCustomGameReport());

	eastl::string playerName = playerReport->getName();
	eastl::string mom = fifaGameReport->getMom();

	uint16_t isMOM = 0;
	if (playerName == mom)
	{
		isMOM = 1;
	}

	fifaPlayerReport->setManOfTheMatch(isMOM);

	TRACE_LOG("[FifaOtpReportSlave:" << this << "].updateMOM() isMOM:" << isMOM << " ");
}

bool FifaOtpReportSlave::didAllPlayersFinish()
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap = osdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::iterator iter, end;
	iter = osdkPlayerReportMap.begin();
	end = osdkPlayerReportMap.end();

	bool result = true;
	for (; iter != end; ++iter)
	{
		OSDKGameReportBase::OSDKPlayerReport* playerReport = iter->second;
		int userResult = playerReport->getUserResult();
		if (userResult != GAME_RESULT_COMPLETE_REGULATION && userResult != GAME_RESULT_COMPLETE_OVERTIME)
		{
			result = false;
		}
	}

	return result;
}

bool FifaOtpReportSlave::processPostStatsCommit()
{
	mFifaVpro.updatePlayerGrowth();
	return true;
}

void FifaOtpReportSlave::CustomizeGameReportForEventReporting(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
	// loop over all participants
	for (auto playerId : playerIds)
	{
		// Find the players specific PIN game report
		auto iter = processedReport.getPINGameReportMap().find(playerId);
		if (iter != processedReport.getPINGameReportMap().end())
		{
			OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*(iter->second.getReport()));
			OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportsMap = osdkReport.getPlayerReports();

			auto playReportiter = osdkPlayerReportsMap.begin();
			while (playReportiter != osdkPlayerReportsMap.end())
			{
				OSDKGameReportBase::OSDKPlayerReport& playerReport = *playReportiter->second;
				
				Collections::AttributeMap& map = playerReport.getPrivatePlayerReport().getPrivateAttributeMap();
				map.clearIsSet();
				
				playReportiter++;
			}
		}
	}
}

}   // namespace GameReporting

}   // namespace Blaze

