/*************************************************************************************************/
/*!
    \file   gamecoopreportslave.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "gamereporting/gamereportingslaveimpl.h"
#include "gamereporting/util/reportparserutil.h"

#include "stats/updatestatsrequestbuilder.h"
#include "stats/updatestatshelper.h"

#include "gameclubchampreportslave.h"
#include "gamecommonlobbyskill.h"
#include "ping/tdf/gameclubchampreport.h"

//TODO: remove after changing the inheritance
#include "gamereporting/ping/gamecustomclub.h"
namespace Blaze
{

namespace GameReporting
{

GameReportProcessor* GameClubChampReportSlave::create(GameReportingSlaveImpl& component)
{
    // GameClubChampReportSlave should not be instantiated
    return NULL;
}

GameClubChampReportSlave::GameClubChampReportSlave(GameReportingSlaveImpl& component) :
    GameH2HReportSlave(component)
{
}

GameClubChampReportSlave::~GameClubChampReportSlave()
{
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
BlazeRpcError GameClubChampReportSlave::validate(GameReport& report) const
{
    return Blaze::ERR_OK;
}

 /*! ****************************************************************************/
 /*! \brief Called when stats are reported following the process() call.
 
    \param processedReport Contains the final game report and information used by game history.
    \param playerIds list of players to distribute results to.
    \return ERR_OK on success. GameReporting specific error on failure.

    \Customizable - Virtual function.
********************************************************************************/
BlazeRpcError GameClubChampReportSlave::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
    mProcessedReport = &processedReport;

    GameReport& report = processedReport.getGameReport();
    const GameType& gameType = processedReport.getGameType();

    BlazeRpcError processErr = Blaze::ERR_OK;

    if (0 == blaze_strcmp(gameType.getGameReportName().c_str(), getCustomClubChampGameTypeName()))
    {
        // create the parser
        mReportParser = BLAZE_NEW Utilities::ReportParser(gameType, processedReport);
        mReportParser->setUpdateStatsRequestBuilder(&mBuilder);

        // fills in report with values via configuration
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_VALUES))
        {
            WARN_LOG("[GameClubChampReportSlave::" << this << "].process(): Error parsing values");

            processErr = mReportParser->getErrorCode();
            delete mReportParser;
            return processErr; // EARLY RETURN
        }

        // common processing
        processCommon();

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

        // parse the keyscopes from the configuration
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_KEYSCOPES))
        {
            WARN_LOG("[GameClubChampReportSlave::" << this << "].process(): Error parsing keyscopes");

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

        // update club belt stats
        updateClubChampStats();

        //  extract and set stats
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_STATS))
        {
            WARN_LOG("[GameClubChampReportSlave::" << this << "].process(): Error parsing stats");

            processErr = mReportParser->getErrorCode();
            delete mReportParser;
            return processErr; // EARLY RETURN
        }

        bool strict = mComponent.getConfig().getBasicConfig().getStrictStatsUpdates();
        processErr = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);

        if (processErr == ERR_OK)
        {
            // Club Champ does not have skill points and skill stats so do not call transformStats
            processErr = mUpdateStatsHelper.calcDerivedStats();
            TRACE_LOG("[GameClubChampReportSlave:" << this << "].process() - calcDerivedStats() returned: " << processErr);
        }

        if (processedReport.needsHistorySave() && processedReport.getReportType() != REPORT_TYPE_TRUSTED_MID_GAME)
        {
            //  extract game history attributes from report.
            GameHistoryReport& historyReport = processedReport.getGameHistoryReport();
            mReportParser->setGameHistoryReport(&historyReport);
            mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_GAME_HISTORY);
            delete mReportParser;
        }

        if(Blaze::ERR_OK == processErr)
        {
            // post game reporting processing()
            if (false == processUpdatedStats()) 
            {
                processErr = Blaze::ERR_SYSTEM;
                TRACE_LOG("[GameClubChampReportSlave:" << this << "].process() - processUpdatedStats() returned false. Failing with ERR_SYSTEM.");
            }
            else 
            {
                processErr = mUpdateStatsHelper.commitStats();  
                TRACE_LOG("[GameClubChampReportSlave:" << this << "].process() - commitStats() returned: " << processErr);
            }            
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
void GameClubChampReportSlave::reconfigure() const
{
}

/*************************************************************************************************/
/*! \brief updateCommonStats
    Update common stats regardless of the game result

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameClubChampReportSlave::updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
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
void GameClubChampReportSlave::updateGameModeWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateWinPlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updateGameModeWinPlayerStats
    Update winner's stats

    \Customizable - Virtual function.
 */
/*************************************************************************************************/
void GameClubChampReportSlave::updateGameModeLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateLossPlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updateGameModeWinPlayerStats
    Update winner's stats

    \Customizable - Virtual function.
 */
/*************************************************************************************************/
void GameClubChampReportSlave::updateGameModeTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateTiePlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updatePlayerKeyscopes
    Set the value of player keyscopes defined in config and needed for config based stat update
 */
/*************************************************************************************************/
void GameClubChampReportSlave::updatePlayerKeyscopes()
{
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    updateCustomPlayerKeyscopes(OsdkPlayerReportsMap);
}

/*************************************************************************************************/
/*! \brief sendEndGameMail
    Send end of game email/sms
*/
/*************************************************************************************************/
void GameClubChampReportSlave::sendEndGameMail(GameManager::PlayerIdList& playerIds)
{
    // if this is not an offline or not a normal game
    if(true == mIsOfflineReport)
    {
        return;
    }
/*
    // prepare mail generation
    // additional parameters are any additional template parameters that will be identical for all players
    Mail::HttpParamList mailParams;

    // allow game team to add custom parameters
    char8_t mailTemplateName[MAX_MAIL_TEMPLATE_NAME_LEN];
    bool bSendMail = setCustomEndGameMailParam(&mailParams, mailTemplateName);
	bool bSendMail = false;

    if(true == bSendMail)
    {
        // set which opt-in flags control preferences for this email.
        Mail::EmailOptInFlags gameReportingOptInFlags;
        gameReportingOptInFlags.setEmailOptinTitleFlags(EMAIL_OPTIN_GAMEREPORTING_FLAG);
        mGenerateMailUtil.generateMail(mailTemplateName, &mailParams, &gameReportingOptInFlags, playerIds);
    }
*/
}

/*************************************************************************************************/
/*! \brief updatePlayerStats
    Update the players' stats based on game result

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameClubChampReportSlave::updatePlayerStats()
{
    // aggregate players' stats
    aggregatePlayerStats();

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
/*! \brief updateClubChampStats
    Update the club champ stats which is a H-H game.

    \Customizable - None.
*/
/*************************************************************************************************/
void GameClubChampReportSlave::updateClubChampStats()
{
    bool success = true;

    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    GameClubChampReportBase::GameReport& clubChampPlayerReport = static_cast<GameClubChampReportBase::GameReport&>(*OsdkReport.getGameReport().getCustomGameReport());
    Clubs::ClubId clubId = clubChampPlayerReport.getClubChampId();

    // Initialize the club utility
    Clubs::ClubIdList clubIdList;
    clubIdList.push_back(clubId);
    if (false == mUpdateClubsUtil.initialize(clubIdList))
    {
        WARN_LOG("[GameClubChampReportSlave:" << this << "].updateClubChampStats() Failed to initialize club util for club " << clubId);
        return; // EARLY RETURN
    }

    // Check if the players in the club champ game belong to the club.
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();
    GameManager::PlayerId playerId = INVALID_BLAZE_ID;

    for(; playerIter != playerEnd; ++playerIter)
    {
        playerId = playerIter->first;
        if (false == mUpdateClubsUtil.checkMembership(clubId, playerId))
        {
            TRACE_LOG("[GameClubChampReportSlave:" << this << "].updateClubChampStats() Player Id " << playerId << " not in club " << clubId);
            return; // EARLY RETURN
        }
    }

    if (false == mUpdateClubsUtil.fetchClubRecords(clubId))
    {
        WARN_LOG("[GameClubChampReportSlave:" << this << "].updateClubChampStats() Failed to read records for club " << clubId);
        success = false;
    }   
    else
    {
        bool updateWinner = false;
        int32_t iClubIndex = 0;
        BlazeId winnerBlazeId = INVALID_BLAZE_ID;
        BlazeId opponentBlazeId = INVALID_BLAZE_ID;
        BlazeId previousWinnerBlazeId = INVALID_BLAZE_ID;

        mNewRecordArray[iClubIndex][CLUB_RECORD_CHAMP].uRecordID = CLUB_RECORD_CHAMP;
        mNewRecordArray[iClubIndex][CLUB_RECORD_CHAMP].uClubId = clubId;
        Clubs::ClubRecordbook clubRecord;

        mNewRecordArray[iClubIndex][CLUB_RECORD_CHAMP].bRecordExists = mUpdateClubsUtil.getClubRecord(mNewRecordArray[iClubIndex][CLUB_RECORD_CHAMP].uRecordID, clubRecord);
        mNewRecordArray[iClubIndex][CLUB_RECORD_CHAMP].uRecordHolder = clubRecord.getUser().getBlazeId();

        if (mNewRecordArray[iClubIndex][CLUB_RECORD_CHAMP].bRecordExists == true)
        {
            previousWinnerBlazeId = mNewRecordArray[iClubIndex][CLUB_RECORD_CHAMP].uRecordHolder;
        }

        if (mNewRecordArray[iClubIndex][CLUB_RECORD_CHAMP].bRecordExists == false ||
            mNewRecordArray[iClubIndex][CLUB_RECORD_CHAMP].uRecordHolder == INVALID_BLAZE_ID)
        {
            // If the record does not exist or currently the record does not belong to any user, then always update the
            // club champ record winner. There is no need to check if one of the users in the game report currently has the champ.
            updateWinner = true;
        }

        playerIter = OsdkPlayerReportsMap.begin();
        for(; playerIter != playerEnd; ++playerIter)
        {
            playerId = playerIter->first;
            OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;

            // Club Champ is always a H-H game, so can set the opponentBlazeId if that user does not have the highest score.
            if (mHighestPlayerScore == static_cast<int32_t>(playerReport.getScore()))
            {
                winnerBlazeId = playerId;
            }
            else
            {
                if (previousWinnerBlazeId != INVALID_BLAZE_ID)
                {
                    // Only set the opponentBlazeId if the club champ is not from escrow.
                    opponentBlazeId = playerId;
                }
            }

            if (mNewRecordArray[iClubIndex][CLUB_RECORD_CHAMP].uRecordHolder == playerId)
            {
                // One of the players in the game report is the owner of the club champ
                updateWinner = true;
            }
        }

        if (updateWinner == true && winnerBlazeId != INVALID_BLAZE_ID)
        {
            // Store the previous belt winner to the currentRecordStat
            mNewRecordArray[iClubIndex][CLUB_RECORD_CHAMP].uRecordHolder = winnerBlazeId;
            mNewRecordArray[iClubIndex][CLUB_RECORD_CHAMP].currentRecordStat = static_cast<RecordStat>(opponentBlazeId);
            success = mUpdateClubsUtil.updateClubRecordInt(mNewRecordArray[iClubIndex][CLUB_RECORD_CHAMP].uClubId, 
                mNewRecordArray[iClubIndex][CLUB_RECORD_CHAMP].uRecordID,
                mNewRecordArray[iClubIndex][CLUB_RECORD_CHAMP].uRecordHolder,
                static_cast<RecordStat>(mNewRecordArray[iClubIndex][CLUB_RECORD_CHAMP].currentRecordStat));
            mUpdateClubsUtil.completeTransaction(success);
        }
    }
}

}   // namespace GameReporting

}   // namespace Blaze
