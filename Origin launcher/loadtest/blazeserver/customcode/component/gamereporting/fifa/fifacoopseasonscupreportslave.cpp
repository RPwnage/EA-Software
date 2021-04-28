/*************************************************************************************************/
/*!
    \file   fifacoopseaosnscupreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "fifacoopseasonscupreportslave.h"

namespace Blaze
{

namespace GameReporting
{

namespace FIFA
{

/*************************************************************************************************/
/*! \brief FifaCoopSeasonsCupReportSlave
    Constructor
*/
/*************************************************************************************************/
FifaCoopSeasonsCupReportSlave::FifaCoopSeasonsCupReportSlave(GameReportingSlaveImpl& component) :
FifaCoopBaseReportSlave(component)
{
}
/*************************************************************************************************/
/*! \brief FifaCoopSeasonsCupReportSlave
    Destructor
*/
/*************************************************************************************************/
FifaCoopSeasonsCupReportSlave::~FifaCoopSeasonsCupReportSlave()
{
}

/*************************************************************************************************/
/*! \brief FifaCoopSeasonsCupReportSlave
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* FifaCoopSeasonsCupReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("FifaCoopSeasonsCupReportSlave") FifaCoopSeasonsCupReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomClubGameTypeName
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
const char8_t* FifaCoopSeasonsCupReportSlave::getCustomSquadGameTypeName() const
{
    return "gameType26";
}

/*************************************************************************************************/
/*! \brief processCustomUpdatedStats
    Perform custom process for post stats update, which include the club awards and record updates
        that should happen after stats for this game report is processed

    \return bool - true, if process is done successfully
*/
/*************************************************************************************************/
bool FifaCoopSeasonsCupReportSlave::processCustomUpdatedStats()
{
    return true;
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
bool FifaCoopSeasonsCupReportSlave::setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName)
{
    // Ping doesn't send any Club end game email
    return false;
}
*/

void FifaCoopSeasonsCupReportSlave::selectCustomStats()
{
	//Seasonal Play
	CoopSeasonsExtension::ExtensionData seasonData;
	seasonData.mProcessedReport = mProcessedReport;
	seasonData.mWinCoopId = mWinCoopId;
	seasonData.mLossCoopId = mLossCoopId;
	seasonData.mTieGame = mTieGame;
	mFifaSeasonalPlayExtension.setExtensionData(&seasonData);
	mFifaSeasonalPlay.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport);

	mFifaSeasonalPlay.setExtension(&mFifaSeasonalPlayExtension);
	mFifaSeasonalPlay.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport);
	mFifaSeasonalPlay.updateCupStats();
}

void FifaCoopSeasonsCupReportSlave::updateCustomTransformStats()
{
}

void FifaCoopSeasonsCupReportSlave::updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
	TRACE_LOG("[GameClubReportSlave:" << this << "].updateCustomPlayerStats()for player " << playerId << " ");

	FifaCoopReportBase::FifaCoopPlayerReportBase* fifaCoopPlayerReportBase = static_cast<FifaCoopReportBase::FifaCoopPlayerReportBase*>(playerReport.getCustomPlayerReport());
	FifaCoopSeasonsReport::FifaCoopSeasonsPlayerReport* fifaCoopSeasonsPlayerReport =  static_cast<FifaCoopSeasonsReport::FifaCoopSeasonsPlayerReport*>(fifaCoopPlayerReportBase->getCustomCoopPlayerReport());

	updateCleanSheets(fifaCoopPlayerReportBase->getPos(), fifaCoopPlayerReportBase->getCoopId(), fifaCoopSeasonsPlayerReport);
	updatePlayerRating(&playerReport, fifaCoopSeasonsPlayerReport);
	updateMOM(&playerReport, fifaCoopSeasonsPlayerReport);
}

void FifaCoopSeasonsCupReportSlave::updateCleanSheets(uint32_t pos, CoopSeason::CoopId coopId, FifaCoopSeasonsReport::FifaCoopSeasonsPlayerReport* fifaCoopSeasonsPlayerReport)
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

void FifaCoopSeasonsCupReportSlave::updatePlayerRating(OSDKGameReportBase::OSDKPlayerReport* playerReport, FifaCoopSeasonsReport::FifaCoopSeasonsPlayerReport* fifaCoopSeasonsPlayerReport)
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

void FifaCoopSeasonsCupReportSlave::updateMOM(OSDKGameReportBase::OSDKPlayerReport* playerReport, FifaCoopSeasonsReport::FifaCoopSeasonsPlayerReport* fifaCoopSeasonsPlayerReport)
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

uint32_t FifaCoopSeasonsCupReportSlave::determineResult(CoopSeason::CoopId id, FifaCoopReportBase::FifaCoopSquadDetailsReportBase& squadDetailsReport)
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
