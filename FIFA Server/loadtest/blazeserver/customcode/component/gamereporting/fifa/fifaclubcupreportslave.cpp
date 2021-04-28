/*************************************************************************************************/
/*!
    \file   fifaclubclubreplortslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "fifaclubcupreportslave.h"

namespace Blaze
{

namespace GameReporting
{

namespace FIFA
{

/*************************************************************************************************/
/*! \brief FifaClubCupReportSlave
    Constructor
*/
/*************************************************************************************************/
FifaClubCupReportSlave::FifaClubCupReportSlave(GameReportingSlaveImpl& component) :
FifaClubBaseReportSlave(component)
{
}
/*************************************************************************************************/
/*! \brief FifaClubCupReportSlave
    Destructor
*/
/*************************************************************************************************/
FifaClubCupReportSlave::~FifaClubCupReportSlave()
{
}

/*************************************************************************************************/
/*! \brief FifaClubCupReportSlave
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* FifaClubCupReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("FifaClubCupReportSlave") FifaClubCupReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomClubGameTypeName
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
const char8_t* FifaClubCupReportSlave::getCustomClubGameTypeName() const
{
    return "gameType13";
}

/*************************************************************************************************/
/*! \brief updateCustomClubRecords
    A custom hook for game team to process for club records based on game report and cache the 
        result in mNewRecordArray, called during updateClubStats()

    \return bool - true, if club record update process is success
*/
/*************************************************************************************************/
bool FifaClubCupReportSlave::updateCustomClubRecords(int32_t iClubIndex, OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap)
{
    return true;
}

/*************************************************************************************************/
/*! \brief updateCustomClubAwards
    A custom hook for game team to give club awards based on game report, called during updateClubStats()

    \return bool - true, if club award update process is success
*/
/*************************************************************************************************/
bool FifaClubCupReportSlave::updateCustomClubAwards(Clubs::ClubId clubId,  OSDKClubGameReportBase::OSDKClubClubReport& clubReport, OSDKGameReportBase::OSDKReport& OsdkReport)
{
    return true;
}

/*************************************************************************************************/
/*! \brief processCustomUpdatedStats
    Perform custom process for post stats update, which include the club awards and record updates
        that should happen after stats for this game report is processed

    \return bool - true, if process is done successfully
*/
/*************************************************************************************************/
bool FifaClubCupReportSlave::processCustomUpdatedStats()
{
    return true;
}

/*************************************************************************************************/
/*! \brief updateCustomClubRivalChallengeWinnerPlayerStats
    Update the club rival challenge player stats

    \param challengeType - the rival challenge type
    \param playerReport - the winner club's player report
*/
/*************************************************************************************************/
void FifaClubCupReportSlave::updateCustomClubRivalChallengeWinnerPlayerStats(uint64_t challengeType, OSDKClubGameReportBase::OSDKClubPlayerReport& playerReport)
{
    switch (challengeType)
    {
    case CLUBS_RIVAL_TYPE_WIN:
        playerReport.setChallengePoints(1);
        break;

    case CLUBS_RIVAL_TYPE_SHUTOUT:
        if (mLowestClubScore == 0)
        {
            playerReport.setChallengePoints(1);
        }
        break;

    case CLUBS_RIVAL_TYPE_WINBY7:
        if ((mHighestClubScore - mLowestClubScore) >= 7)
        {
            playerReport.setChallengePoints(1);
        }
        break;

    case CLUBS_RIVAL_TYPE_WINBY5:
        if ((mHighestClubScore - mLowestClubScore) >= 5)
        {
            playerReport.setChallengePoints(1);
        }
        break;

    default:
        break;
    }
}

/*************************************************************************************************/
/*! \brief updateCustomClubRivalChallengeWinnerClubStats
    Update the club rival challenge club stats for the winning club

    \param challengeType - the rival challenge type
    \param clubReport - the winner club's report
*/
/*************************************************************************************************/
void FifaClubCupReportSlave::updateCustomClubRivalChallengeWinnerClubStats(uint64_t challengeType, OSDKClubGameReportBase::OSDKClubClubReport& clubReport)
{
    switch (challengeType)
    {
    case CLUBS_RIVAL_TYPE_WIN:
        clubReport.setChallengePoints(1);
        break;

    case CLUBS_RIVAL_TYPE_SHUTOUT:
        if (mLowestClubScore == 0)
        {
            clubReport.setChallengePoints(1);
        }
        break;

    case CLUBS_RIVAL_TYPE_WINBY7:
        if ((mHighestClubScore - mLowestClubScore) >= 7)
        {
            clubReport.setChallengePoints(1);
        }
        break;

    case CLUBS_RIVAL_TYPE_WINBY5:
        if ((mHighestClubScore - mLowestClubScore) >= 5)
        {
            clubReport.setChallengePoints(1);
        }
        break;

    default:
        break;
    }
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
bool FifaClubCupReportSlave::setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName)
{
    // Ping doesn't send any Club end game email
    return false;
}
*/

void FifaClubCupReportSlave::selectCustomStats()
{
	//Seasonal Play
	ClubSeasonsExtension::ExtensionData seasonData;
	seasonData.mProcessedReport = mProcessedReport;
	seasonData.mWinClubId = mWinClubId;
	seasonData.mLossClubId = mLossClubId;
	seasonData.mTieGame = mTieGame;
	mFifaSeasonalPlayExtension.setExtensionData(&seasonData);
	mFifaSeasonalPlayExtension.UpdateEntityFilterList(mEntityFilterList);
	mFifaSeasonalPlay.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport);

	mFifaSeasonalPlay.setExtension(&mFifaSeasonalPlayExtension);
	mFifaSeasonalPlay.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport);
	mFifaSeasonalPlay.updateCupStats();

	mFifaVpro.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport);
	mFifaVpro.setExtension(&mFifaVproExtension);

	mFifaVproObjectiveStatsUpdater.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport);
	mFifaVproObjectiveStatsUpdater.selectObjectiveStats();

	//VPro
	FifaVpro::CategoryInfoVector categoryVector;
	
	FifaVpro::CategoryInfo clubOTPPlayer;
	clubOTPPlayer.statCategory = "ClubOTPPlayerStats";
	clubOTPPlayer.keyScopeType = FifaVpro::KEYSCOPE_POS;
	categoryVector.push_back(clubOTPPlayer);

	FifaVpro::CategoryInfo clubMember;
	clubMember.statCategory = "ClubMemberStats";
	clubMember.keyScopeType = FifaVpro::KEYSCOPE_CLUBID;
	categoryVector.push_back(clubMember);

	FifaVpro::CategoryInfo clubUserCareer;
	clubUserCareer.statCategory = "ClubUserCareerStats";
	clubUserCareer.keyScopeType = FifaVpro::KEYSCOPE_NONE;
	categoryVector.push_back(clubUserCareer);

	mFifaVpro.updateVproStats(categoryVector);

	//-------------------------------------------------------
	categoryVector.clear();

	FifaVpro::CategoryInfo vProCategory;
	vProCategory.statCategory = "VProStats";
	vProCategory.keyScopeType = FifaVpro::KEYSCOPE_NONE;
	categoryVector.push_back(vProCategory);

	mFifaVpro.selectStats(categoryVector);
}

void FifaClubCupReportSlave::updateCustomTransformStats()
{
	FifaVpro::CategoryInfoVector categoryVector;
	FifaVpro::CategoryInfo vProCategory;
	vProCategory.statCategory = "VProStats";
	vProCategory.keyScopeType = FifaVpro::KEYSCOPE_NONE;
	categoryVector.push_back(vProCategory);

	mFifaVpro.updateVproGameStats(categoryVector);

	mFifaVproObjectiveStatsUpdater.transformObjectiveStats();
}

bool FifaClubCupReportSlave::processPostStatsCommit()
{
	mFifaVpro.updatePlayerGrowth();
	return true;
}

void FifaClubCupReportSlave::updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
	TRACE_LOG("[GameClubReportSlave:" << this << "].updateCustomPlayerStats()for player " << playerId << " ");

	OSDKClubGameReportBase::OSDKClubPlayerReport* clubPlayerReport = static_cast<OSDKClubGameReportBase::OSDKClubPlayerReport*>(playerReport.getCustomPlayerReport());
	FifaClubReportBase::FifaClubsPlayerReport* fifaClubPlayerReport =  static_cast<FifaClubReportBase::FifaClubsPlayerReport*>(clubPlayerReport->getCustomClubPlayerReport());

	updateCleanSheets(clubPlayerReport->getPos(), clubPlayerReport->getClubId(), fifaClubPlayerReport);
	updatePlayerRating(&playerReport, fifaClubPlayerReport);
	updateMOM(&playerReport, fifaClubPlayerReport);
}

void FifaClubCupReportSlave::updateCleanSheets(uint32_t pos, Clubs::ClubId clubId, FifaClubReportBase::FifaClubsPlayerReport* fifaClubPlayerReport)
{
	uint16_t cleanSheetsAny = 0;
	uint16_t cleanSheetsDef = 0;
	uint16_t cleanSheetsGK = 0;

	if (pos == POS_ANY || pos == POS_DEF || pos == POS_GK)
	{
		uint32_t locScore = 0;
		Clubs::ClubId locTeam = 0;
		uint32_t oppScore = 0;
		Clubs::ClubId oppTeam = 0;

		locScore = mLowestClubScore;
		locTeam = clubId;
		oppScore = mHighestClubScore;
		oppTeam = mWinClubId;

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

	fifaClubPlayerReport->setCleanSheetsAny(cleanSheetsAny);
	fifaClubPlayerReport->setCleanSheetsDef(cleanSheetsDef);
	fifaClubPlayerReport->setCleanSheetsGoalKeeper(cleanSheetsGK);

	TRACE_LOG("[FifaClubCupReportSlave:" << this << "].updateCleanSheets() ANY:" << cleanSheetsAny << " DEF:" << cleanSheetsDef << " GK:" << cleanSheetsGK << " ");
}

void FifaClubCupReportSlave::updatePlayerRating(OSDKGameReportBase::OSDKPlayerReport* playerReport, FifaClubReportBase::FifaClubsPlayerReport* fifaClubPlayerReport)
{
	const int DEF_PLAYER_RATING = 3;

	Collections::AttributeMap& map = playerReport->getPrivatePlayerReport().getPrivateAttributeMap();

	float rating = static_cast<float>(atof(map["VProRating"]));
	if (rating <= 0.0f)
	{
		rating = DEF_PLAYER_RATING;
	}

	fifaClubPlayerReport->getCommonPlayerReport().setRating(rating);
	
	TRACE_LOG("[FifaClubCupReportSlave:" << this << "].updatePlayerRating() rating:" << fifaClubPlayerReport->getCommonPlayerReport().getRating() << " ");
}

void FifaClubCupReportSlave::updateMOM(OSDKGameReportBase::OSDKPlayerReport* playerReport, FifaClubReportBase::FifaClubsPlayerReport* fifaClubPlayerReport)
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKGameReport osdkGameReport = osdkReport.getGameReport();
	OSDKClubGameReportBase::OSDKClubGameReport* osdkClubGameReport = static_cast<OSDKClubGameReportBase::OSDKClubGameReport*>(osdkGameReport.getCustomGameReport());
	FifaClubReportBase::FifaClubsGameReport* fifaClubReport = static_cast<FifaClubReportBase::FifaClubsGameReport*>(osdkClubGameReport->getCustomClubGameReport());

	eastl::string playerName = playerReport->getName();
	eastl::string mom = fifaClubReport->getMom();
	
	uint16_t isMOM = 0;
	if (playerName == mom)
	{
		isMOM = 1;
	}

	fifaClubPlayerReport->setManOfTheMatch(isMOM);

	TRACE_LOG("[FifaClubCupReportSlave:" << this << "].updateMOM() isMOM:" << isMOM << " ");
}

uint32_t FifaClubCupReportSlave::determineResult(Clubs::ClubId id, OSDKClubGameReportBase::OSDKClubClubReport& clubReport)
{
	//first determine if the user won, lost or draw
	uint32_t resultValue = 0;
	if (mWinClubId == id)
	{
		resultValue = WLT_WIN;
	}
	else if (mLossClubId == id)
	{
		resultValue = WLT_LOSS;
	}
	else
	{
		resultValue = WLT_TIE;
	}

	if (clubReport.getClubDisc() == 1)
	{
		resultValue |= WLT_DISC;
	}

	// Track the win-by-dnf
	if (id == mWinClubId && !mClubGameFinish)
	{
		resultValue |= WLT_OPPOQCTAG;
	}

	return resultValue;
}

} // namespace FIFA
} // namespace GameReporting
}// namespace Blaze
