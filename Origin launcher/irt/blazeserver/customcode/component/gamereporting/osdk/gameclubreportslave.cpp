/*************************************************************************************************/
/*!
    \file   gameclubreportslave.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "gamereporting/gamereportingslaveimpl.h"
#include "gamereporting/util/reportparserutil.h"
#include "gamereporting/util/gamehistoryutil.h"
#include "gamereporting/util/skilldampingutil.h"

#include "gamereporting/osdk/osdkseasonalplayutil.h"

#include "stats/updatestatsrequestbuilder.h"
#include "stats/updatestatshelper.h"

#include "gameotp.h"
#include "gameclub.h"
#include "gameclubreportslave.h"
#include "gamecommonlobbyskill.h"
#include "ping/tdf/gameclubreport.h"

//TODO: remove after refactor this file for the hierarchy change
#include "gamereporting/ping/gamecustom.h"

namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief GameClubReportSlave
    Create

    \return GameReportSlave pointer
*/
/*************************************************************************************************/
GameReportProcessor* GameClubReportSlave::create(GameReportingSlaveImpl& component)
{
    // GameClubReportSlave should not be instantiated
    return NULL;
}

/*************************************************************************************************/
/*! \brief GameClubReportSlave
    Constructor
*/
/*************************************************************************************************/
GameClubReportSlave::GameClubReportSlave(GameReportingSlaveImpl& component) :
    GameOTPReportSlave(component),
    mWinClubId(Clubs::INVALID_CLUB_ID),
    mLossClubId(Clubs::INVALID_CLUB_ID),
    mClubGameFinish(false),
    mLowestClubScore(INVALID_HIGH_SCORE),
    mHighestClubScore(INT32_MIN)
{
}

/*************************************************************************************************/
/*! \brief GameClubReportSlave
    Destructor
*/
/*************************************************************************************************/
GameClubReportSlave::~GameClubReportSlave()
{
    // Club Rank
    ClubScopeIndexMap::const_iterator scopeIter = mClubRankingKeyscopes.begin();
    ClubScopeIndexMap::const_iterator scopeIterEnd = mClubRankingKeyscopes.end();
    for (; scopeIter != scopeIterEnd; scopeIter++)
    {
        delete(scopeIter->second);
    }

    // TODO: add seasonal play hook
    // Seasonal
    scopeIter = mClubSeasonalKeyscopes.begin();
    scopeIterEnd = mClubSeasonalKeyscopes.end();
    for (; scopeIter != scopeIterEnd; scopeIter++)
    {
        delete(scopeIter->second);
    }
}

/*! ****************************************************************************/
/*! \brief Implementation performs simple validation and if necessary modifies 
        the game report.

    On success report may be submitted to master for collation or direct to 
    processing for offline or trusted reports. Behavior depends on the calling RPC.

    \param report Incoming game report from submit request
    \return ERR_OK on success. GameReporting specific error on failure.

    \Customizable - Virtual function.
********************************************************************************/
BlazeRpcError GameClubReportSlave::validate(GameReport& report) const
{
    if (NULL == report.getReport())
    {
        WARN_LOG("[GameClubReportSlave:" << this << "].validate() : NULL report.");
        return Blaze::ERR_SYSTEM; // EARLY RETURN
    }

    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*report.getReport());
    OSDKClubGameReportBase::OSDKClubReport& clubReport = static_cast<OSDKClubGameReportBase::OSDKClubReport&>(*OsdkReport.getTeamReports());
    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& OSDKClubReportMap = clubReport.getClubReports();

    if (2 != OSDKClubReportMap.size())
    {
        WARN_LOG("[GameClubReportSlave:" << this << "].validate() : There must have been exactly two clubs in this game!");
        return Blaze::ERR_SYSTEM; // EARLY RETURN
    }

    UpdateClubsUtil updateClubsUtil;
    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::const_iterator clubIter, clubIterEnd;
    clubIter = OSDKClubReportMap.begin();
    clubIterEnd = OSDKClubReportMap.end();

    for(; clubIter != clubIterEnd; ++clubIter)
    {
        // validate club id
        Clubs::ClubId clubId = clubIter->first;
        if (false == updateClubsUtil.validateClubId(clubId))
        {
			WARN_LOG("[GameClubReportSlave:" << this << "].validate() : Invalid club id " << clubId );
        }
    }

    // check membership
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    for(; playerIter != playerEnd; ++playerIter)
    {
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
        OSDKClubGameReportBase::OSDKClubPlayerReport& clubPlayerReport = static_cast<OSDKClubGameReportBase::OSDKClubPlayerReport&>(*playerReport.getCustomPlayerReport());

        Clubs::ClubId playerClubId = clubPlayerReport.getClubId();

        if (playerClubId <= 0)
        {
            WARN_LOG("[GameClubReportSlave:" << this << "].validate() : invalid player Club Id");
            return Blaze::ERR_SYSTEM; // EARLY RETURN
        }
    }

    return Blaze::ERR_OK;
}

/*! ****************************************************************************/
/*! \brief Called when stats are reported following the process() call.
 
    \param processedReport Contains the final game report and information used by game history.
    \param playerIds list of players to distribute results to.
    \return ERR_OK on success. GameReporting specific error on failure.

    \Customizable - Virtual function.
********************************************************************************/
BlazeRpcError GameClubReportSlave::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
    mProcessedReport = &processedReport;

    GameReport& report = processedReport.getGameReport();
    const GameType& gameType = processedReport.getGameType();

    BlazeRpcError processErr = Blaze::ERR_OK;

    const char8_t* CLUB_GAME_TYPE_NAME = getCustomClubGameTypeName();

    if (0 == blaze_strcmp(gameType.getGameReportName().c_str(), CLUB_GAME_TYPE_NAME))
    {
        // create the parser
        mReportParser = BLAZE_NEW Utilities::ReportParser(gameType, processedReport);
        mReportParser->setUpdateStatsRequestBuilder(&mBuilder);

        // fills in report with values via configuration
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_VALUES))
        {
            WARN_LOG("[GameClubReportSlave::" << this << "].process(): Error parsing values");

            processErr = mReportParser->getErrorCode();
            delete mReportParser;
            return processErr; // EARLY RETURN
        }

        // common processing
        processCommon();

        // initialize club utility with the Club IDs in the game report
        if (false == initClubUtil())
        {
            WARN_LOG("[GameClubReportSlave::" << this << "].process(): Error initializing club util");
            delete mReportParser;
            return Blaze::ERR_SYSTEM;  // EARLY RETURN
        }

        // initialize game parameters
        initGameParams();

        if (true == skipProcess())
        {
            delete mReportParser;
            return processErr;  // EARLY RETURN
        }

        // determine win/loss/tie
        determineGameResult(); 

        // update player keyscopes
        updatePlayerKeyscopes();

        // TODO: add custom hook to guard seasonal play logic
        // find the season state for each club in the report
        //determineClubSeasonState();

        // update club keyscopes
        updateClubKeyscopes();

        // parse the keyscopes from the configuration
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_KEYSCOPES))
        {
            WARN_LOG("[GameClubReportSlave::" << this << "].process(): Error parsing keyscopes");

            processErr = mReportParser->getErrorCode();
            delete mReportParser;
            return processErr; // EARLY RETURN
        }

        // determine conclusion type
        if(true == skipProcessConclusionType())
        {
            updateSkipProcessConclusionTypeStats();
            delete mReportParser;
            return processErr;  // EARLY RETURN
        }

        // update player stats
        updatePlayerStats();

        // update club free agent player stats
        updateClubFreeAgentPlayerStats();

        // update club stats
        if (false == updateClubStats())
        {
            // do not update the club stats if the game result is not committed to clubs component
            WARN_LOG("[GameClubReportSlave::" << this << "].process() Failed to commit stats to club component");
            delete mReportParser;
            return Blaze::ERR_SYSTEM; // EARLY RETURN
        }

        // TODO: add custom hook to guard seasonal play logic
        // update seasonal club stats
        //updateClubSeasonalStats();

		selectCustomStats();

        // extract and set stats
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_STATS))
        {
            WARN_LOG("[GameClubReportSlave::" << this << "].process(): Error parsing stats");

            processErr = mReportParser->getErrorCode();
            delete mReportParser;
            return processErr; // EARLY RETURN
        }

        bool strict = mComponent.getConfig().getBasicConfig().getStrictStatsUpdates();		
        processErr = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);

        if (Blaze::ERR_OK == processErr)
        {
            if (Blaze::ERR_OK == transformStats())  // Club Rank
            {
                transformSeasonalClubStats();       // Club Seasonal Stats
            }

            TRACE_LOG("[GameClubReportSlave:" << this << "].process() - commit stats");
            processErr = mUpdateStatsHelper.commitStats();
        }

        if (processedReport.needsHistorySave() && REPORT_TYPE_TRUSTED_MID_GAME != processedReport.getReportType())
        {
            // extract game history attributes from report.
            GameHistoryReport& historyReport = processedReport.getGameHistoryReport();
            mReportParser->setGameHistoryReport(&historyReport);
            mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_GAME_HISTORY);
            delete mReportParser;
        }
        
        //Stats were already committed and can be evaluated during that process. Fetching stats again
        //to ensure mUpdateStatsHelper cache is valid (and avoid crashing with assert)
        processErr = (DB_ERR_OK == mUpdateStatsHelper.fetchStats()) ? ERR_OK : ERR_SYSTEM;
        if(ERR_OK == processErr)
        {
            // post game reporting processing()
            processErr = (true == processUpdatedStats()) ? ERR_OK : ERR_SYSTEM;
        }

        // send end game mail
        sendEndGameMail(playerIds);
    }

    return processErr;
}
 
/*******************************************************************************/
/*! \brief Triggered on server reconfiguration.

    \Customizable - Virtual function.
********************************************************************************/
void GameClubReportSlave::reconfigure() const
{
}

/*************************************************************************************************/
/*! \brief initGameParams
    Initialize game parameters that are needed for later processing

    \Customizable - Virtual function to override default behavior.
    \Customizable - initCustomGameParams() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameClubReportSlave::initGameParams()
{
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKClubGameReportBase::OSDKClubReport& clubReport = static_cast<OSDKClubGameReportBase::OSDKClubReport&>(*OsdkReport.getTeamReports());
    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& OSDKClubReportMap = clubReport.getClubReports();

    mCategoryId = OsdkReport.getGameReport().getCategoryId();
    mRoomId = OsdkReport.getGameReport().getRoomId();

    // could be a tie game if there is more than one club
    mTieGame = (OSDKClubReportMap.size() > 1);

    OSDKClubGameReportBase::OSDKClubGameReport& clubGameReport = static_cast<OSDKClubGameReportBase::OSDKClubGameReport&>(*OsdkReport.getGameReport().getCustomGameReport());
    clubGameReport.setMember(mPlayerReportMapSize);

    // Get the club challenge club ID from the game attribute
    if (mProcessedReport->getGameInfo() != NULL)
    {
        const Collections::AttributeMap& gameAttributeMap = mProcessedReport->getGameInfo()->getAttributeMap();
        if(gameAttributeMap.find(ATTRIBNAME_CLUBCHALLENGE) != gameAttributeMap.end() &&
           getInt(&gameAttributeMap, ATTRIBNAME_CLUBCHALLENGE) != 0)
        {
            clubGameReport.setChallenge(1);
        }
        else
        {
            clubGameReport.setChallenge(0);
        }
    }

    initCustomGameParams();
}

/*************************************************************************************************/
/*! \brief determineGameResult
    Determine the game results

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameClubReportSlave::determineGameResult()
{
    determineWinClub();
}

/*************************************************************************************************/
/*! \brief updatePlayerKeyscopes
    Update Player Keyscopes

    \Customizable - Virtual function to override default behavior.
    \Customizable - updateCustomPlayerKeyscopes() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameClubReportSlave::updatePlayerKeyscopes() 
{ 
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    for(; playerIter != playerEnd; ++playerIter)
    {
        GameManager::PlayerId playerId = playerIter->first;
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
        OSDKClubGameReportBase::OSDKClubPlayerReport& clubPlayerReport = static_cast<OSDKClubGameReportBase::OSDKClubPlayerReport&>(*playerReport.getCustomPlayerReport());

        if (mUpdateClubsUtil.checkUserInClub(playerId) == false)
        {
            Stats::ScopeNameValueMap* indexMap = new Stats::ScopeNameValueMap();
            (*indexMap)[SCOPENAME_POS] = clubPlayerReport.getPos();
            mFreeAgentPlayerKeyscopes[playerId] = indexMap;
        }
    }

    updateCustomPlayerKeyscopes(); 
}

/*************************************************************************************************/
/*! \brief updateCommonStats
    Update common stats regardless of the game result

    \Customizable - Virtual function to override default behavior.
    \Customizable - updateCustomPlayerStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameClubReportSlave::updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    OSDKClubGameReportBase::OSDKClubPlayerReport& clubPlayerReport = static_cast<OSDKClubGameReportBase::OSDKClubPlayerReport&>(*playerReport.getCustomPlayerReport());
    clubPlayerReport.setClubGames(1);

    // Get the club challenge club ID from the game attribute
    if (mProcessedReport->getGameInfo() != NULL)
    {
        const Collections::AttributeMap& gameAttributeMap = mProcessedReport->getGameInfo()->getAttributeMap();
        if(gameAttributeMap.find(ATTRIBNAME_CLUBCHALLENGE) != gameAttributeMap.end() &&
           getInt(&gameAttributeMap, ATTRIBNAME_CLUBCHALLENGE) != 0)
        {
            clubPlayerReport.setChallenge(1);
        }
        else
        {
            clubPlayerReport.setChallenge(0);
        }
    }

    // update player's DNF stat
    updatePlayerDNF(playerReport);

    updateCustomPlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updateGameModeWinPlayerStats
    Update winner's stats

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameClubReportSlave::updateGameModeWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateWinPlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updateGameModeLossPlayerStats
    Update loser's stats

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameClubReportSlave::updateGameModeLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateLossPlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updateGameModeTiePlayerStats
    Update tied player's stats

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameClubReportSlave::updateGameModeTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateTiePlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updatePlayerStats
    Update the players' stats based on game result

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameClubReportSlave::updatePlayerStats()
{
    TRACE_LOG("[GameClubReportSlave:" << this << "].updatePlayerStats()");

    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

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

/*************************************************************************************************/
/*! \brief processUpdatedStats
    Perform post stats update processing

    \return - true if the post-game processing is performed successfully

    \Customizable - Virtual function to override default behavior.
    \Customizable - processCustomUpdatedStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
bool GameClubReportSlave::processUpdatedStats()
{
    bool success = true;

    success = success ? processCustomUpdatedStats() : success;

    // update all the club Records 
    for(uint32_t uClubIndex = 0; uClubIndex < static_cast<uint32_t>(MAX_CLUB_NUMBER); uClubIndex ++)
    {
        for (uint32_t iIndexRecords = 0; iIndexRecords < static_cast<uint32_t>(NUMBER_OF_TROPHIES); iIndexRecords++ )
        {
            if((mNewRecordArray[uClubIndex][iIndexRecords].bUpdateRecord) || 
               (mNewRecordArray[uClubIndex][iIndexRecords].bNewRecord))
            {
                // check to see that the record holder is not 0
                if(0 != mNewRecordArray[uClubIndex][iIndexRecords].uRecordHolder)
                {
                    success = success ? (mUpdateClubsUtil.updateClubRecordInt(mNewRecordArray[uClubIndex][iIndexRecords].uClubId, 
                                                                              mNewRecordArray[uClubIndex][iIndexRecords].uRecordID,
                                                                              mNewRecordArray[uClubIndex][iIndexRecords].uRecordHolder,
                                                                              mNewRecordArray[uClubIndex][iIndexRecords].currentRecordStat)) : success;
                }
            }
        }                    
    }
    mUpdateClubsUtil.completeTransaction(success);

    return success;
}

/*************************************************************************************************/
/*! \brief initClubUtil
    Initialize the club utility with the club IDs in the game report

    \Customizable - None.
*/
/*************************************************************************************************/
bool GameClubReportSlave::initClubUtil()
{
    bool success = true;

    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKClubGameReportBase::OSDKClubReport& clubReport = static_cast<OSDKClubGameReportBase::OSDKClubReport&>(*OsdkReport.getTeamReports());
    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& OSDKClubReportMap = clubReport.getClubReports();

    // Initialize the club utility
    Clubs::ClubIdList clubIdList;
    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::const_iterator clubIter, clubIterEnd;
    clubIter = OSDKClubReportMap.begin();
    clubIterEnd = OSDKClubReportMap.end();

    for (; clubIter != clubIterEnd; ++clubIter)
    {
        Clubs::ClubId clubId = clubIter->first;
        clubIdList.push_back(clubId);
    }

    success = mUpdateClubsUtil.initialize(clubIdList);
    return success;
}

/*************************************************************************************************/
/*! \brief determineWinClub
    Determine the winning club by the highest club score

    \Customizable - None.
*/
/*************************************************************************************************/
void GameClubReportSlave::determineWinClub()
{
    int32_t prevClubScore = INT32_MIN;
    mHighestClubScore = INT32_MIN;
    mLowestClubScore = INVALID_HIGH_SCORE;

    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKClubGameReportBase::OSDKClubReport& clubReport = static_cast<OSDKClubGameReportBase::OSDKClubReport&>(*OsdkReport.getTeamReports());
    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& OSDKClubReportMap = clubReport.getClubReports();

    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::const_iterator clubIter, clubIterFirst, clubIterEnd;
    clubIter = clubIterFirst = OSDKClubReportMap.begin();
    clubIterEnd = OSDKClubReportMap.end();

    // set the first club in the club report map as winning club
    mWinClubId = clubIterFirst->first;
    mLossClubId = clubIterFirst->first;

	mClubGameFinish = true;
    for (; clubIter != clubIterEnd; ++clubIter)
    {
        Clubs::ClubId clubId = clubIter->first;
        OSDKClubGameReportBase::OSDKClubClubReport& clubReportInner = *clubIter->second;

        bool didClubFinish = (0 == clubReportInner.getClubDisc()) ? true : false;
        int32_t clubScore = static_cast<int32_t>(clubReportInner.getScore());
		TRACE_LOG("[GameClubReportSlave:" << this << "].determineWinClub(): Club Id:" << clubId << " score:" << clubScore << " clubFinish:" << didClubFinish <<" ");

        if (true == didClubFinish && clubScore > mHighestClubScore)
        {
            // update the winning club id and highest club score
            mWinClubId = clubId;
            mHighestClubScore = clubScore;
        }
        if (true == didClubFinish && clubScore < mLowestClubScore)
        {
            // update the losing club id and lowest club score
            mLossClubId = clubId;
            mLowestClubScore = clubScore;
        }

        // if some of the scores are different, it isn't a tie.
        if (clubIter != clubIterFirst && clubScore != prevClubScore)
        {
            mTieGame = false;
        }

        prevClubScore = clubScore;
    }

	clubIter = clubIterFirst = OSDKClubReportMap.begin();
	for (; clubIter != clubIterEnd; ++clubIter)
	{
		OSDKClubGameReportBase::OSDKClubClubReport& osdkClubReport = *clubIter->second;
		if (1 == osdkClubReport.getClubDisc())
		{
			mClubGameFinish = false;
			mLossClubId = clubIter->first;
		}
	}

    adjustClubScore();

    //determine who the winners and losers are and write them into the report for stat update
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    for(; playerIter != playerEnd; ++playerIter)
    {
        GameManager::PlayerId playerId = playerIter->first;
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
        OSDKClubGameReportBase::OSDKClubPlayerReport& clubPlayerReport = static_cast<OSDKClubGameReportBase::OSDKClubPlayerReport&>(*playerReport.getCustomPlayerReport());

        Clubs::ClubId clubId = clubPlayerReport.getClubId();
        bool winner = (clubId == mWinClubId);

        if (mTieGame)
		{
			TRACE_LOG("[GameClubReportSlave:" << this << "].determineWinClub(): tie set - Player Id:" << playerId << " ");
            mTieSet.insert(playerId);
		}
        else if (winner)
		{
			TRACE_LOG("[GameClubReportSlave:" << this << "].determineWinClub(): winner set - Player Id:" << playerId << " ");
            mWinnerSet.insert(playerId);
		}
        else
		{
			TRACE_LOG("[GameClubReportSlave:" << this << "].determineWinClub(): loser set - Player Id:" << playerId << " ");
            mLoserSet.insert(playerId);
		}
    }
}

/*************************************************************************************************/
/*! \brief adjustClubScore
    Make sure there is a minimum spread between the club that disconnected and the other
    clubs find the lowest score of those who did not cheat or quit and assign a default
    score if necessary

    \Customizable - None.
*/
/*************************************************************************************************/
void GameClubReportSlave::adjustClubScore()
{
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKClubGameReportBase::OSDKClubReport& clubReportOuter = static_cast<OSDKClubGameReportBase::OSDKClubReport&>(*OsdkReport.getTeamReports());
    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& OSDKClubReportMap = clubReportOuter.getClubReports();

    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::const_iterator clubIter, clubIterEnd;
    clubIter = OSDKClubReportMap.begin();
    clubIterEnd = OSDKClubReportMap.end();

    bool zeroScore = false;

    for(; clubIter != clubIterEnd; ++clubIter)
    {              
        OSDKClubGameReportBase::OSDKClubClubReport& clubReport = *clubIter->second;

        bool didClubFinish = (0 == clubReport.getClubDisc()) ? true : false;
        if (false == didClubFinish)
        {
            int32_t clubScore = static_cast<int32_t>(clubReport.getScore());

            if (mLowestClubScore != INVALID_HIGH_SCORE && ((clubScore >= mLowestClubScore) ||
                (mLowestClubScore - clubScore) < DISC_SCORE_MIN_DIFF))
            {
                mTieGame = false;
				// new score for cheaters or quitters is 0
                    clubScore = 0;
                    zeroScore = true;
                mLowestClubScore = 0;

                clubReport.setScore(static_cast<uint32_t>(clubScore));
            }
        }
    }

    // need adjustment on scores of non-disconnected clubs
    if (zeroScore)
    {
        clubIter = OSDKClubReportMap.begin();
        clubIterEnd = OSDKClubReportMap.end();

        for (; clubIter != clubIterEnd; ++clubIter)
        {
            OSDKClubGameReportBase::OSDKClubClubReport& clubReport = *clubIter->second;

            bool didClubFinish = (0 == clubReport.getClubDisc()) ? true : false;
            if (true == didClubFinish)
            {
                int32_t clubScore = static_cast<int32_t>(clubReport.getScore());

                
                    clubScore = DISC_SCORE_MIN_DIFF;

                  mHighestClubScore = clubScore;

                clubReport.setScore(static_cast<int32_t>(clubScore));
            }
        }
    }
}

/*************************************************************************************************/
/*! \brief updateClubKeyscopes
    Set the value of club keyscopes defined in config and needed for config based stat update

    \Customizable - updateCustomClubKeyscopes() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameClubReportSlave::updateClubKeyscopes()
{
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKClubGameReportBase::OSDKClubReport& clubReportOuter = static_cast<OSDKClubGameReportBase::OSDKClubReport&>(*OsdkReport.getTeamReports());
    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& OSDKClubReportMap = clubReportOuter.getClubReports();

    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::const_iterator clubIter, clubIterEnd;
    clubIter = OSDKClubReportMap.begin();
    clubIterEnd = OSDKClubReportMap.end();

    //OSDKSeasonalPlayUtil seasonalPlayUtil;
    for(; clubIter != clubIterEnd; ++clubIter)
    {
        Clubs::ClubId clubId = clubIter->first;
        OSDKClubGameReportBase::OSDKClubClubReport& clubReport = *clubIter->second;
        Clubs::ClubSettings clubSettings;
        mUpdateClubsUtil.getClubSettings(clubId, clubSettings);

        // Region
        //Stats::ScopeNameValueMap* indexMap = new Stats::ScopeNameValueMap();
        //(*indexMap)[SCOPENAME_REGION] = clubSettings.getRegion();
        //mClubRankingKeyscopes[clubId] = indexMap;

        // Seasonal
        //seasonalPlayUtil.setMember(static_cast<OSDKSeasonalPlay::MemberId>(clubId), OSDKSeasonalPlay::SEASONALPLAY_MEMBERTYPE_CLUB);
        OSDKSeasonalPlay::SeasonId clubSeasonId = 1;//seasonalPlayUtil.getSeasonId();

        Stats::ScopeNameValueMap* indexSeasonalRankMap = new Stats::ScopeNameValueMap();
        //(*indexSeasonalRankMap)[SCOPENAME_SEASON] = clubSeasonId;
        mClubSeasonalKeyscopes[clubId] = indexSeasonalRankMap;

        clubReport.setClubRegion(clubSettings.getRegion()); // SCOPENAME_REGION
        clubReport.setClubId(clubId);                       // SCOPENAME_CLUB
        clubReport.setClubSeason(clubSeasonId);             // SCOPENAME_SEASON
    }

    updateCustomClubKeyscopes();
}

/*************************************************************************************************/
/*!
    \brief updateClubStats
    Update the club stats, records and awards based on game result 
    provided that this is a club game

    \Customizable - updateCustomClubStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
bool GameClubReportSlave::updateClubStats()
{
    bool success = true;

    // update clubs recordbook
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKClubGameReportBase::OSDKClubReport& clubReportOuter = static_cast<OSDKClubGameReportBase::OSDKClubReport&>(*OsdkReport.getTeamReports());
    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& OSDKClubReportMap = clubReportOuter.getClubReports();

	// Data validation
	if ( OSDKClubReportMap.size() > MAX_CLUB_NUMBER )
	{
		WARN_LOG("[GameClubReportSlave:" << this << "].updateClubStats() - OSDKClubReportMap.size[" << OSDKClubReportMap.size() << "] >= MAX_CLUB_NUMBER[" << MAX_CLUB_NUMBER << "]");
		return false;
	}

    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::const_iterator clubIter, clubIterEnd;
    clubIter = OSDKClubReportMap.begin();
    clubIterEnd = OSDKClubReportMap.end();

    memset(mNewRecordArray, 0, sizeof(mNewRecordArray));
    Clubs::ClubId arrClubId[MAX_CLUB_NUMBER];
    int32_t arrClubScore[MAX_CLUB_NUMBER];

    for(int32_t i = 0; clubIter != clubIterEnd; ++clubIter, i++)
    {
        Clubs::ClubId clubId = clubIter->first;
        OSDKClubGameReportBase::OSDKClubClubReport& clubReport = *clubIter->second;

        arrClubId[i] = clubId;
        arrClubScore[i] = 0;

        // fetch all records for club
        if (false == mUpdateClubsUtil.fetchClubRecords(clubId))
        {
            WARN_LOG("[GameClubReportSlave].updateClubStats() Failed to read records for club [" << clubId << "]");
			int32_t iIndexRecords;
			for (iIndexRecords = 0; iIndexRecords < NUMBER_OF_TROPHIES; iIndexRecords++)
			{
				mNewRecordArray[i][iIndexRecords].bRecordExists = false;
				mNewRecordArray[i][iIndexRecords].bNewRecord = false;
			}
			continue;
        }   
        else
        {
            // Prepare (zero out) the club record array before processing
            for (int32_t iRecordIterator = 0; iRecordIterator < NUMBER_OF_TROPHIES; iRecordIterator++)
            {
                mNewRecordArray[i][iRecordIterator].uRecordID = iRecordIterator + 1;
                mNewRecordArray[i][iRecordIterator].uClubId = clubIter->first;
            }
            
            int32_t iIndexRecords;
            for(iIndexRecords = 0; iIndexRecords < NUMBER_OF_TROPHIES; iIndexRecords++ )
            {
                Clubs::ClubRecordbook clubRecord;

                mNewRecordArray[i][iIndexRecords].bRecordExists = mUpdateClubsUtil.getClubRecord(mNewRecordArray[i][iIndexRecords].uRecordID, clubRecord);
                mNewRecordArray[i][iIndexRecords].currentRecordStat = clubRecord.getStatValueInt();

                // the record doesn't exist 
                if(false == mNewRecordArray[i][iIndexRecords].bRecordExists)
                {
                    mNewRecordArray[i][iIndexRecords].bNewRecord = true;
                }
            }

            OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();
            OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
            playerIter = OsdkPlayerReportsMap.begin();
            playerEnd = OsdkPlayerReportsMap.end();

            for(; playerIter != playerEnd; ++playerIter)
            {
                OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
                OSDKClubGameReportBase::OSDKClubPlayerReport& clubPlayerReport = static_cast<OSDKClubGameReportBase::OSDKClubPlayerReport&>(*playerReport.getCustomPlayerReport());

                Clubs::ClubId playerClubId = clubPlayerReport.getClubId();
                if (clubId == playerClubId)
                {
                    // update the member minutes
                    uint32_t uGameTimeInMinutes = (mGameTime / 60);
                    if(uGameTimeInMinutes == 0)
                    {
                        uGameTimeInMinutes = 1;
                    }
                    clubPlayerReport.setMinutes(uGameTimeInMinutes);                   
                }
            }
            success = success ? updateCustomClubRecords(i,OsdkPlayerReportsMap) : success;
        }

        success = success ? updateCustomClubAwards(clubId, clubReport, OsdkReport) : success;

        // Store the club score for later stats update
        arrClubScore[i] = static_cast<int32_t>(clubReport.getScore());

        //Stats::ScopeNameValueMap* indexMap = NULl;
        //ClubScopeIndexMap::const_iterator scopeIter = mClubRankingKeyscopes.find(clubId);
        //if (scopeIter != mClubRankingKeyscopes.end())
        //{
        //    indexMap = scopeIter->second;
        //}
        mBuilder.startStatRow(CATEGORYNAME_CLUB_RANKING, clubId, NULL);

        uint32_t resultValue = 0;
        bool didClubFinish = (clubReport.getClubDisc() == 0) ? true : false;

        if (true == mTieGame)
        {
            clubReport.setWins(0);
            clubReport.setLosses(0);
            clubReport.setTies(1);

            resultValue |= WLT_TIE;
            mBuilder.assignStat(STATNAME_WSTREAK, (int64_t)0);
        }
        else if (mWinClubId == clubId)
        {
            clubReport.setWins(1);
            clubReport.setLosses(0);
            clubReport.setTies(0);

            resultValue |= WLT_WIN;
            mBuilder.incrementStat(STATNAME_WSTREAK, 1);
        }
        else
        {
            clubReport.setWins(0);
            clubReport.setLosses(1);
            clubReport.setTies(0);

            resultValue |= WLT_LOSS;
            mBuilder.assignStat(STATNAME_WSTREAK, (int64_t)0);
        }

        if (false == didClubFinish)
        {
            resultValue |= WLT_DISC;
        }

        // Track the win-by-dnf
        if ((mWinClubId == clubId)&& (false == mClubGameFinish))
        {
            clubReport.setWinnerByDnf(1);
            resultValue |= WLT_OPPOQCTAG;
        }
        else
        {
            clubReport.setWinnerByDnf(0);
        }

        // Store the result value in the map
        clubReport.setGameResult(resultValue);  // STATNAME_RESULT

        updateCustomClubStatsPerClub(clubId, clubReport);
        mBuilder.completeStatRow();
    }

    //TODO: should add guard to club rival logic
    // update clubs rival stats
    updateClubRivalStats();

    updateCustomClubStats(OsdkReport);

    // update clubs component on result of game
    success = success ? mUpdateClubsUtil.updateClubs(arrClubId[0], arrClubScore[0], arrClubId[1], arrClubScore[1]) : success;

    // clean the db call if failed if it will success then we will call this later on 
    if(false == success)
    {
        mUpdateClubsUtil.completeTransaction(success);
        return success; // EARLY RETURN
    }

    checkPlayerClubMembership();

    TRACE_LOG("[GameClubReportSlave].updateClubStats() Processing complete");

    return success;
}

/*************************************************************************************************/
/*! \brief calcClubDampingPercent
    Calculate the club damping skills "wins" vs "losses"

    \return - damping skill percentage

    \Customizable - None.
*/
/*************************************************************************************************/
uint32_t GameClubReportSlave::calcClubDampingPercent(Clubs::ClubId clubId)
{
    TRACE_LOG("[GameClubReportSlave].calcClubDampingPercent() clubId: [" << clubId << "]");

    //set damping to 100% as default case
    uint32_t dampingPercent = 100;

    GameHistoryUtil gameHistoryUtil(mComponent);
    QueryVarValuesList queryVarValues;
    GameReportsList gameReportsList;

    if (clubId == mWinClubId)
    {
        char8_t strWinClubId[32];
        blaze_snzprintf(strWinClubId, sizeof(strWinClubId), "%" PRIu64, mWinClubId);
        queryVarValues.push_back(strWinClubId);

        if (gameHistoryUtil.getGameHistory(QUERY_CLUBGAMESTATS, 10, queryVarValues, gameReportsList))
        {
            uint32_t rematchWins = 0;
            uint32_t dnfWins = 0;

            GameReportsList rematchedWinningReportsList;
            Collections::AttributeMap lossClubMap;
            char8_t strLossClubId[32];
            blaze_snzprintf(strLossClubId, sizeof(strLossClubId), "%" PRIu64, mLossClubId);
            (lossClubMap)["club_id"] = strLossClubId;
            (lossClubMap)[STATNAME_LOSSES] = "1";

            gameHistoryUtil.getGameHistoryMatchingValues("club", lossClubMap, gameReportsList, rematchedWinningReportsList);
            rematchWins = rematchedWinningReportsList.getGameReportList().size();
            rematchedWinningReportsList.getGameReportList().release();

            // only check this if the current win is via DNF
            if (false == mProcessedReport->didAllPlayersFinish())
            {
                GameReportsList winnerByDnfReportsList;
                Collections::AttributeMap winnerByDnfSearchMap;
                winnerByDnfSearchMap["club_id"] = strWinClubId;
                winnerByDnfSearchMap[ATTRIBNAME_WINNERBYDNF] = "1";

                // could use winningGameReportsList from above if the desire was to count consecutive wins by dnf in the player's last "x" winning games, rather than all consecutive games
                gameHistoryUtil.getGameHistoryConsecutiveMatchingValues("club", winnerByDnfSearchMap, gameReportsList, winnerByDnfReportsList);
                dnfWins = winnerByDnfReportsList.getGameReportList().size();
                winnerByDnfReportsList.getGameReportList().release();
            }

            SkillDampingUtil skillDampingUtil(mComponent);
            uint32_t rematchDampingPercent = skillDampingUtil.lookupDampingPercent(rematchWins, "rematchDamping");
            uint32_t dnfWinDampingPercent = skillDampingUtil.lookupDampingPercent(dnfWins, "dnfDamping");

            TRACE_LOG("[GameClubReportSlave].calcClubDampingPercent(): rematchWins: " << rematchWins << " - rematchDampingPercent: " << rematchDampingPercent);
            TRACE_LOG("[GameClubReportSlave].calcClubDampingPercent(): dnfWins: " << dnfWins << " - dnfWinDampingPercent: " << dnfWinDampingPercent);

            dampingPercent = (rematchDampingPercent * dnfWinDampingPercent) / 100; // int division is fine here because the stat transform is expecting an int
            TRACE_LOG("[GameClubReportSlave:" << this << "].calcClubDampingPercent(): Skill damping value " << dampingPercent << " applied to club " << clubId << "'s stat transform.");
        }
        gameReportsList.getGameReportList().size();
    }

    return dampingPercent;
}

/*************************************************************************************************/
/*! \brief checkPlayerClubMembership
    Check the player club membership and if player does not belong to the club when the
    game finished, disable the updating of that players stats

    \Customizable - None.
*/
/*************************************************************************************************/
void GameClubReportSlave::checkPlayerClubMembership()
{
    // check membership
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    for(; playerIter != playerEnd; ++playerIter)
    {
        GameManager::PlayerId playerId = playerIter->first;
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
        OSDKClubGameReportBase::OSDKClubPlayerReport& clubPlayerReport = static_cast<OSDKClubGameReportBase::OSDKClubPlayerReport&>(*playerReport.getCustomPlayerReport());

        Clubs::ClubId clubId = clubPlayerReport.getClubId();
        if (false == mUpdateClubsUtil.checkMembership(clubId, playerId))
        {
            TRACE_LOG("[GameClubReportSlave].checkPlayerClubMembership() disable update stats for player: [" << playerId << "]");
            // Disable stats update of a particular playerId
            mReportParser->ignoreStatUpdatesForEntityId(ENTITY_TYPE_USER, playerId, true);
        }
    }
}

/*************************************************************************************************/
/*! \brief getClubFromPlayer
    Gets the club Id based on the player

    \return - the club id of the player

    \Customizable - None.
*/
/*************************************************************************************************/
Clubs::ClubId GameClubReportSlave::getClubFromPlayer(GameManager::PlayerId playerID)
{
    Clubs::ClubId clubId = Clubs::INVALID_CLUB_ID;

    // check membership
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    for(; playerIter != playerEnd; ++playerIter)
    {
        if (playerID == playerIter->first)
        {
            OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
            OSDKClubGameReportBase::OSDKClubPlayerReport& clubPlayerReport = static_cast<OSDKClubGameReportBase::OSDKClubPlayerReport&>(*playerReport.getCustomPlayerReport());
            clubId = clubPlayerReport.getClubId();
            break;
        }
    }

    return clubId;
}

/*************************************************************************************************/
/*! \brief transformStats
    calculate lobby skill and lobby skill points

    \Customizable - setCustomClubLobbySkillInfo() to provide additional custom behavior.
    \Customizable - updateCustomTransformStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
BlazeRpcError GameClubReportSlave::transformStats()
{
    BlazeRpcError processErr = Blaze::ERR_OK;
    if (DB_ERR_OK == mUpdateStatsHelper.fetchStats())
    {
     //   GameCommonLobbySkill* lobbySkill = getGameCommonLobbySkill();
     //   if(NULL != lobbySkill)
     //   {
     //       OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
     //       OSDKClubGameReportBase::OSDKClubReport& clubReportOuter = static_cast<OSDKClubGameReportBase::OSDKClubReport&>(*OsdkReport.getTeamReports());
     //       OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& OSDKClubReportMap = clubReportOuter.getClubReports();

     //       OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::const_iterator clubIter, clubIterEnd;
     //       clubIter = OSDKClubReportMap.begin();
     //       clubIterEnd = OSDKClubReportMap.end();

     //       TRACE_LOG("[GameClubReportSlave:" << this << "].transformStats() Processing club rank pts...");
     //       for(; clubIter != clubIterEnd; ++clubIter)
     //       {
     //           Clubs::ClubId clubId = clubIter->first;
     //           OSDKClubGameReportBase::OSDKClubClubReport& clubReport = *clubIter->second;

     //           Stats::UpdateRowKey* key = mBuilder.getUpdateRowKey(CATEGORYNAME_CLUB_RANKING, clubId);
     //           if (NULL != key)
     //           {
     //               // Setting the default value for Lobby Skill Info
     //               int32_t dampingPercent = static_cast<int32_t>(calcClubDampingPercent(clubId)); /* set percentage to use if pro-rating stats */
     //               int32_t weight = 15;        /* set dummy club 'weight' used to determine iAdj factor */
     //               int32_t skill = 45;         /* set club 'skill' */
     //               int32_t wlt = static_cast<int32_t>(clubReport.getGameResult());
     //               int32_t teamId = static_cast<int32_t>(clubReport.getTeam());
     //               bool home = clubReport.getHome();
     //               GameCommonLobbySkill::LobbySkillInfo skillInfo;
     //               skillInfo.setLobbySkillInfo(wlt, dampingPercent, teamId, weight, home, skill);

     //               //Allow game team to modify
     //               setCustomClubLobbySkillInfo(&skillInfo);

     //               lobbySkill->addLobbySkillInfo(clubId, skillInfo.getWLT(), skillInfo.getDampingPercent(), skillInfo.getTeamId(), skillInfo.getWeight(), skillInfo.getHome(), skillInfo.getSkill());

     //               // Loop through all the supported stats period type
     //               for (uint32_t periodType = static_cast<uint32_t>(Stats::STAT_PERIOD_ALL_TIME); periodType < Stats::STAT_NUM_PERIODS; periodType++)
     //               {
     //                   int32_t currentSkillPoints = mUpdateStatsHelper.getValueInt(key, STATNAME_OVERALLPTS, static_cast<Stats::StatPeriodType>(periodType), true);
     //                   lobbySkill->addCurrentSkillPoints(clubId, currentSkillPoints, static_cast<Stats::StatPeriodType>(periodType));

     //                   int32_t currentSkillLevel = mUpdateStatsHelper.getValueInt(key, STATNAME_SKILL, static_cast<Stats::StatPeriodType>(periodType), true);
     //                   lobbySkill->addCurrentSkillLevel(clubId, currentSkillLevel, static_cast<Stats::StatPeriodType>(periodType));
     //               }
     //           }
     //       }

     //       clubIter = OSDKClubReportMap.begin();
     //       for(; clubIter != clubIterEnd; ++clubIter)
     //       {
     //           Clubs::ClubId clubId = clubIter->first;

     //           Stats::UpdateRowKey* key = mBuilder.getUpdateRowKey(CATEGORYNAME_CLUB_RANKING, clubId);
     //           if (NULL != key)
     //           {
     //               // Loop through all the supported stats period type
     //               for (uint32_t periodType = static_cast<uint32_t>(Stats::STAT_PERIOD_ALL_TIME); periodType < Stats::STAT_NUM_PERIODS; periodType++)
     //               {
     //                   int32_t newSkillPoints = lobbySkill->getNewLobbySkillPoints(clubId, static_cast<Stats::StatPeriodType>(periodType));
     //                   mUpdateStatsHelper.setValueInt(key, STATNAME_OVERALLPTS, static_cast<Stats::StatPeriodType>(periodType), newSkillPoints);

     //                   int32_t newSkillLevel = lobbySkill->getNewLobbySkillLevel(clubId, mReportConfig, newSkillPoints);
     //                   mUpdateStatsHelper.setValueInt(key, STATNAME_SKILL, static_cast<Stats::StatPeriodType>(periodType), newSkillLevel);

     //               	TRACE_LOG("[GameClubReportSlave:" << this << "].transformStats() for clubId: " << clubId << ", newSkillPoints:" << newSkillPoints <<
     //                         ", newSkillLevel:" << newSkillLevel << ", periodType=" << periodType);
					//}
     //           }
     //       }
     //   }
        updateCustomTransformStats();
    }
    else
    {
        processErr = Blaze::ERR_SYSTEM;
    }
    return processErr;
}

/*************************************************************************************************/
/*! \brief transformSeasonalClubStats
    calculate lobby skill and lobby skill points

    \Customizable - setCustomSeasonalClubLobbySkillInfo() to provide additional custom behavior.
    \Customizable - updateCustomTransformSeasonalClubStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameClubReportSlave::transformSeasonalClubStats()
{
    // The stats is already fetched in transformStats(), so there is no need to fetch again.
    GameCommonLobbySkill* lobbySkill = getGameCommonLobbySkill();
    if(NULL != lobbySkill)
    {
        OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
        OSDKClubGameReportBase::OSDKClubReport& clubReportOuter = static_cast<OSDKClubGameReportBase::OSDKClubReport&>(*OsdkReport.getTeamReports());
        OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& OSDKClubReportMap = clubReportOuter.getClubReports();

        OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::const_iterator clubIter, clubIterEnd;
        clubIter = OSDKClubReportMap.begin();
        clubIterEnd = OSDKClubReportMap.end();

        TRACE_LOG("[GameClubReportSlave:" << this << "].transformSeasonalClubStats() Processing club seasonal pts...");
        for(; clubIter != clubIterEnd; ++clubIter)
        {
            Clubs::ClubId clubId = clubIter->first;
            OSDKClubGameReportBase::OSDKClubClubReport& clubReport = *clubIter->second;

            Stats::UpdateRowKey* key = mBuilder.getUpdateRowKey(CATEGORYNAME_CLUB_SEASON, clubId);
            if (NULL != key)
            {
                // Setting the default value for Lobby Skill Info
                int32_t dampingPercent = static_cast<int32_t>(calcClubDampingPercent(clubId)); /* set percentage to use if pro-rating stats */
                int32_t weight = 15;        /* set dummy club 'weight' used to determine iAdj factor */
                int32_t skill = 45;         /* set club 'skill' */
                int32_t wlt = static_cast<int32_t>(clubReport.getGameResult());
                int32_t teamId = static_cast<int32_t>(clubReport.getTeam());
                bool home = clubReport.getHome();
                GameCommonLobbySkill::LobbySkillInfo skillInfo;
                skillInfo.setLobbySkillInfo(wlt, dampingPercent, teamId, weight, home, skill);

                // Allow game team to modify
                setCustomSeasonalClubLobbySkillInfo(&skillInfo);

                lobbySkill->addLobbySkillInfo(clubId, skillInfo.getWLT(), skillInfo.getDampingPercent(), skillInfo.getTeamId(), skillInfo.getWeight(), skillInfo.getHome(), skillInfo.getSkill());

                // Loop through all the supported stats period type
                for (uint32_t periodType = static_cast<uint32_t>(Stats::STAT_PERIOD_ALL_TIME); periodType < Stats::STAT_NUM_PERIODS; periodType++)
                {
                    int32_t currentSkillPoints = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(key, STATNAME_OVERALLPTS, static_cast<Stats::StatPeriodType>(periodType), true));
                    lobbySkill->addCurrentSkillPoints(clubId, currentSkillPoints, static_cast<Stats::StatPeriodType>(periodType));

                    int32_t currentSkillLevel = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(key, STATNAME_SKILL, static_cast<Stats::StatPeriodType>(periodType), true));
                    lobbySkill->addCurrentSkillLevel(clubId, currentSkillLevel, static_cast<Stats::StatPeriodType>(periodType));
                }
            }
        }

        clubIter = OSDKClubReportMap.begin();
        for(; clubIter != clubIterEnd; ++clubIter)
        {
            Clubs::ClubId clubId = clubIter->first;

            ClubSeasonStateMap::const_iterator seasonStateIter = mSeasonStateMap.find(clubId);
            if (seasonStateIter != mSeasonStateMap.end())
            {
                OSDKSeasonalPlay::SeasonState thisClubSeasonState = seasonStateIter->second;
                if (thisClubSeasonState == OSDKSeasonalPlay::SEASONALPLAY_SEASON_STATE_REGULARSEASON)
                {
                    Stats::UpdateRowKey* key = mBuilder.getUpdateRowKey(CATEGORYNAME_CLUB_SEASON, clubId);
                    if (NULL != key)
                    {
                        // Loop through all the supported stats period type
                        for (uint32_t periodType = static_cast<uint32_t>(Stats::STAT_PERIOD_ALL_TIME); periodType < Stats::STAT_NUM_PERIODS; periodType++)
                        {
                            int32_t newSkillPoints = lobbySkill->getNewLobbySkillPoints(clubId, static_cast<Stats::StatPeriodType>(periodType));
                            mUpdateStatsHelper.setValueInt(key, STATNAME_OVERALLPTS, static_cast<Stats::StatPeriodType>(periodType), newSkillPoints);

                            int32_t newSkillLevel = lobbySkill->getNewLobbySkillLevel(clubId, mReportConfig, newSkillPoints);
                            mUpdateStatsHelper.setValueInt(key, STATNAME_SKILL, static_cast<Stats::StatPeriodType>(periodType), newSkillLevel);

                        	TRACE_LOG("[GameClubReportSlave:" << this << "].transformSeasonalClubStats() for clubId: " << clubId << ", newSkillPoints:"
                                  << newSkillPoints << ", newSkillLevel:" << newSkillLevel << ", periodType=" << periodType);
						}
                    }
                }
            }
        }
    }
    updateCustomTransformSeasonalClubStats();
}

/*************************************************************************************************/
/*! \brief updateClubRecords
    Updates a record in the club, if the player has beat the record

    \playerId - the player Id made the passed in record value
    \iPlayerRecordValue - the record value that the player make in this game
    \iRecordID  - the Id of the record to check

    \return - true if the record was successfully written 

    \Customizable - None.
*/
/*************************************************************************************************/
bool GameClubReportSlave::updateClubRecords(GameManager::PlayerId playerId, int32_t iPlayerRecordValue, int32_t iRecordID)
{
    bool success = true;
    Clubs::ClubId uClubId = getClubFromPlayer(playerId);
    if(uClubId != 0)
    {
		// Data validation
		if ( iRecordID >= NUMBER_OF_TROPHIES )
		{
			WARN_LOG("[GameClubReportSlave:" << this << "].updateClubRecords() - iRecordID[" << iRecordID << "] >= NUMBER_OF_TROPHIES[" << NUMBER_OF_TROPHIES << "]");
			return false;
		}

        uint32_t uClubPos = (mNewRecordArray[0][0].uClubId == uClubId)? 0: 1;
        int64_t iCurrentClubRecord =  mNewRecordArray[uClubPos][iRecordID].currentRecordStat;

        if(iPlayerRecordValue >= iCurrentClubRecord)
        {
			// Data validation
			if ( iRecordID == 0 )
			{
				WARN_LOG("[GameClubReportSlave:" << this << "].updateClubRecords() - iRecordID[" << iRecordID << "] == 0");
				return false;
			}

            mNewRecordArray[uClubPos][iRecordID - 1].currentRecordStat = iPlayerRecordValue;
            mNewRecordArray[uClubPos][iRecordID - 1].uRecordHolder = static_cast<BlazeId>(playerId);
            mNewRecordArray[uClubPos][iRecordID - 1].bUpdateRecord = true;
        }
    }
    else
    {
        success = false;
    }

    return success;
}

/*************************************************************************************************/
/*! \brief updateClubRivalStats
    Update the club rival stats
    TODO: should add guard to club rival logic

    \Customizable - updateCustomClubRivalChallengeWinnerClubStats() to provide additional custom behavior.
    \Customizable - updateCustomClubRivalChallengeWinnerPlayerStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameClubReportSlave::updateClubRivalStats()
{
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKClubGameReportBase::OSDKClubReport& clubReportOuter = static_cast<OSDKClubGameReportBase::OSDKClubReport&>(*OsdkReport.getTeamReports());
    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& OSDKClubReportMap = clubReportOuter.getClubReports();

    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::const_iterator clubIter, clubIterEnd;
    clubIter = OSDKClubReportMap.begin();
    clubIterEnd = OSDKClubReportMap.end();

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    // Initialize the playerMap with default values for STATNAME_RIVALCHALLENGEPTS and STATNAME_RIVALH2HPTS
    for(; playerIter != playerEnd; ++playerIter)
    {
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
        OSDKClubGameReportBase::OSDKClubPlayerReport& clubPlayerReport = static_cast<OSDKClubGameReportBase::OSDKClubPlayerReport&>(*playerReport.getCustomPlayerReport());
        clubPlayerReport.setChallengePoints(0);
        clubPlayerReport.setH2hPoints(0);
    }

    Clubs::ClubsComponentSettings clubsComponentSettings;
    bool getClubsComponentSettingsResult = mUpdateClubsUtil.getClubsComponentSettings(clubsComponentSettings);

    for(uint32_t index = 0; clubIter != clubIterEnd; ++clubIter, index++)
    {
        Clubs::ClubId clubId = clubIter->first;
        OSDKClubGameReportBase::OSDKClubClubReport& clubReport = *clubIter->second;
        clubReport.setChallengePoints(0);
        clubReport.setH2hPoints(0);

        if (mWinClubId == clubId)
        {
            Clubs::ClubRivalList winClubRivalList;
            mUpdateClubsUtil.listClubRivals(mWinClubId, winClubRivalList);

            // Check if the winner club has rival in the current rival period.
            // If winner club has rival assigned, check if club rival challenge stats need to be updated.
            if (winClubRivalList.size() != 0 && true == getClubsComponentSettingsResult)
            {
                Clubs::ClubRival *winRival = *(winClubRivalList.begin());
                if (static_cast<int64_t>(winRival->getCreationTime()) > clubsComponentSettings.getSeasonStartTime())
                {
                    // These is a rival assigned for the current rival period.
                    updateCustomClubRivalChallengeWinnerClubStats(winRival->getCustOpt1(), clubReport);

                    playerIter = OsdkPlayerReportsMap.begin();
                    playerEnd = OsdkPlayerReportsMap.end();
                    for(; playerIter != playerEnd; ++playerIter)
                    {
                        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
                        OSDKClubGameReportBase::OSDKClubPlayerReport& clubPlayerReport = static_cast<OSDKClubGameReportBase::OSDKClubPlayerReport&>(*playerReport.getCustomPlayerReport());

                        Clubs::ClubId playerClubId = clubPlayerReport.getClubId();
                        if (playerClubId == mWinClubId)
                        {
                            updateCustomClubRivalChallengeWinnerPlayerStats(winRival->getCustOpt1(), clubPlayerReport);

                            if (winRival->getRivalClubId() == mLossClubId)
                            {
                                // Check if the clubs are rival against each other in the current rival period.
                                clubPlayerReport.setH2hPoints(1);
                            }
                        }
                    }

                    if (winRival->getRivalClubId() == mLossClubId)
                    {
                        clubReport.setH2hPoints(1);

                        // Set the rival custom option 2 with the winner clubId
                        // static_cast to be removed after GOSOPS-34273 is resolved.
                        winRival->setCustOpt2(static_cast<uint32_t>(mWinClubId));

                        // Set the rival custom option 3 with the award time
                        // static_cast to be removed after GOSOPS-34273 is resolved.
                        winRival->setCustOpt3(static_cast<uint32_t>(TimeValue::getTimeOfDay().getSec()));

                        // Update the club trophy award for winning club.
                        mUpdateClubsUtil.updateClubRival(mWinClubId, *winRival);

                        // Update the club trophy award for losing club.
                        Blaze::Clubs::ClubRivalList lossClubRivalList;
                        mUpdateClubsUtil.listClubRivals(mLossClubId, lossClubRivalList);

                        if (lossClubRivalList.size() != 0 && true == getClubsComponentSettingsResult)
                        {
                            Blaze::Clubs::ClubRival *lossRival = *(lossClubRivalList.begin());
                            if (static_cast<int64_t>(lossRival->getCreationTime()) > clubsComponentSettings.getSeasonStartTime())
                            {
                                // Set the rival custom option 2 with the winner clubId
                                // static_cast to be removed after GOSOPS-34273 is resolved.
                                lossRival->setCustOpt2(static_cast<uint32_t>(mWinClubId));

                                // Set the rival custom option 3 with the award time
                                lossRival->setCustOpt3(static_cast<uint32_t>(TimeValue::getTimeOfDay().getSec()));

                                mUpdateClubsUtil.updateClubRival(mLossClubId, *lossRival);
                            }
                        }
                    }
                }
            }
        }
    }
}

/*************************************************************************************************/
/*! \brief updateClubSeasonalStats
    Sets up a map of club id to season state.

    \Customizable - None.
*/
/*************************************************************************************************/
void GameClubReportSlave::determineClubSeasonState()
{
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKClubGameReportBase::OSDKClubReport& clubReport = static_cast<OSDKClubGameReportBase::OSDKClubReport&>(*OsdkReport.getTeamReports());
    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& OSDKClubReportMap = clubReport.getClubReports();

    OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::const_iterator clubIter, clubIterEnd;
    clubIter = OSDKClubReportMap.begin();
    clubIterEnd = OSDKClubReportMap.end();

    OSDKSeasonalPlayUtil seasonalPlayUtil;
    for(; clubIter != clubIterEnd; ++clubIter)
    {
        seasonalPlayUtil.setMember(static_cast<OSDKSeasonalPlay::MemberId>(clubIter->first), OSDKSeasonalPlay::SEASONALPLAY_MEMBERTYPE_CLUB);

        // determine season state
        OSDKSeasonalPlay::SeasonState clubSeasonState = seasonalPlayUtil.getSeasonState();
        mSeasonStateMap.insert(eastl::make_pair(clubIter->first, clubSeasonState));
    }
}

/*************************************************************************************************/
/*! \brief updateClubSeasonalStats
    Updates club stats within a season.

    \Customizable - None.
*/
/*************************************************************************************************/
void GameClubReportSlave::updateClubSeasonalStats()
{
    //TRACE_LOG("[GameClubReportSlave:" << this << "].updateClubSeasonalStats()");

    //OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    //OSDKClubGameReportBase::OSDKClubReport& clubReportOuter = static_cast<OSDKClubGameReportBase::OSDKClubReport&>(*OsdkReport.getTeamReports());
    //OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& OSDKClubReportMap = clubReportOuter.getClubReports();

    //OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::const_iterator clubIter, clubIterEnd;
    //clubIter = OSDKClubReportMap.begin();
    //clubIterEnd = OSDKClubReportMap.end();
    //
    //OSDKSeasonalPlayUtil seasonalPlayUtil;
    //for(; clubIter != clubIterEnd; ++clubIter)
    //{
    //    Blaze::Clubs::ClubId clubId = clubIter->first;
    //    ClubSeasonStateMap::const_iterator seasonStateIter = mSeasonStateMap.find(clubId);
    //    if (seasonStateIter != mSeasonStateMap.end())
    //    {
    //        seasonalPlayUtil.setMember(static_cast<OSDKSeasonalPlay::MemberId>(clubIter->first), OSDKSeasonalPlay::SEASONALPLAY_MEMBERTYPE_CLUB);

    //        // only regular season stats are recorded
    //        if (OSDKSeasonalPlay::SEASONALPLAY_SEASON_STATE_REGULARSEASON == seasonalPlayUtil.getSeasonState())
    //        {
    //            Stats::ScopeNameValueMap* indexSeasonalRankMap = NULL;
    //            ClubScopeIndexMap::const_iterator scopeIter = mClubSeasonalKeyscopes.find(clubId);
    //            if (scopeIter != mClubSeasonalKeyscopes.end())
    //            {
    //                indexSeasonalRankMap = scopeIter->second;
    //            }
	//
    //            mBuilder.startStatRow(CATEGORYNAME_CLUB_SEASON, clubId, indexSeasonalRankMap);
    //            if (clubId == mWinClubId)
    //            {
    //                mBuilder.incrementStat(STATNAME_WINS, 1);
    //            }
    //            else
    //            {
    //                mBuilder.incrementStat(STATNAME_LOSSES, 1);
    //            }
    //            mBuilder.incrementStat(STATNAME_POINTS, clubReport.getScore());
    //            mBuilder.completeStatRow();
    //        }
    //    }
    //}
}

/*************************************************************************************************/
/*! \brief getCustomOTPGameTypeName
    Update the club free agent players' stats based on game result for OTP and Club games

    \Customizable - Virtual function to override default behavior.
    \Customizable - getCustomClubGameTypeName() to provide additional custom behavior.
*/
/*************************************************************************************************/
const char8_t* GameClubReportSlave::getCustomOTPGameTypeName() const 
{ 
    return getCustomClubGameTypeName(); 
};

/*************************************************************************************************/
/*! \brief getCustomOTPStatsCategoryName
    Update the club free agent players' stats based on game result for OTP and Club games

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
const char8_t* GameClubReportSlave::getCustomOTPStatsCategoryName() const 
{ 
    // Clubs has it's own stats to update, doesn't need to update the OTP stats category
    return ""; 
};

}   // namespace GameReporting

}   // namespace Blaze
