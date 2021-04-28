/*************************************************************************************************/
/*!
    \file   gamecommonreportslave.h
            Game modes common file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_GAMECOMMONREPORT_SLAVE_H
#define BLAZE_CUSTOM_GAMECOMMONREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "EASTL/hash_set.h"
#include "gamereporting/gamereporttdf.h"
#include "gamereporting/gamereportprocessor.h"
#include "gamereporting/osdk/gamecommon.h"
#include "gamereporting/osdk/reportconfig.h"
#include "gamereporting/osdk/tdf/gameosdkreport.h"
//#include "gamereporting/util/generatemailutil.h"
#include "gamereporting/osdk/gamecommonlobbyskill.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class GameCommonReportSlave
    This class has pure virtual functions and cannot be instantiated.
*/
class GameCommonReportSlave: public GameReportProcessor
{
public:
    GameCommonReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~GameCommonReportSlave();

    /*! ****************************************************************************/
    /*! \brief Implementation performs simple validation and if necessary modifies 
            the game report.
 
        On success report may be submitted to master for collation or direct to 
        processing for offline or trusted reports. Behavior depends on the calling RPC.

        \param report Incoming game report from submit request
        \return ERR_OK on success. GameReporting specific error on failure.
    ********************************************************************************/
    virtual BlazeRpcError validate(GameReport& report) const;
 
    /*! ****************************************************************************/
    /*! \brief Called when stats are reported following the process() call.
 
        \param processedReport Contains the final game report and information used by game history.
        \param playerIds list of players to distribute results to.
        \return ERR_OK on success.  GameReporting specific error on failure.
    ********************************************************************************/
    virtual BlazeRpcError process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds);

	/*! ****************************************************************************/
	/*! \brief Called after process() has been called

		\param processedReport Contains the final game report and information used by game history.
		\return ERR_OK on success.  GameReporting specific error on failure.
	********************************************************************************/
	virtual BlazeRpcError postProcess(ProcessedGameReport& processedReport, bool reportWasSuccessful);
 
    /*! ****************************************************************************/
    /*! \brief Triggered on server reconfiguration.
    ********************************************************************************/
    virtual void reconfigure() const;

protected:

    /*! ****************************************************************************/
    /*! \Constants
    ********************************************************************************/
    // This flag is used to designate the opt-in flag for game reporting emails.
    // it must be in sync with the code used by the game team or samples.
    static const int32_t    EMAIL_OPTIN_GAMEREPORTING_FLAG = 0x01;
    static const uint16_t   MAX_MAIL_TEMPLATE_NAME_LEN = 32;
    // Used by both GameClubReportSlave and GameClubChampReportSlave
    static const int32_t    MAX_CLUB_NUMBER = 2;
    static const int32_t    NUMBER_OF_TROPHIES = 8;    // TODO: should move to custom later since  mNewRecordArray is using it

    static const char8_t*   SCOPENAME_SEREGION;

    /*! ****************************************************************************/
    /*! \Types
    ********************************************************************************/
    typedef eastl::hash_map<GameManager::PlayerId, Stats::ScopeNameValueMap* >  PlayerScopeIndexMap;
    typedef eastl::map<const char8_t*, uint32_t>                                AggregatePlayerStatsMap;
    typedef eastl::hash_set<GameManager::PlayerId>                              ResultSet;
    typedef int64_t RecordStat;
    typedef eastl::map<Blaze::ExternalId, uint32_t>    EntityScoreMap;

    struct NewRecordStruct
    {
        bool bNewRecord;
        bool bUpdateRecord;
        bool bRecordExists;
        RecordStat currentRecordStat;
        BlazeId uRecordHolder;
        Clubs::ClubRecordId uRecordID;
        Clubs::ClubId uClubId;

        NewRecordStruct() : 
            bNewRecord(false),
            bUpdateRecord(false),
            bRecordExists(false),
            currentRecordStat(0),
            uRecordHolder(INVALID_BLAZE_ID),
            uRecordID(0),
            uClubId(Clubs::INVALID_CLUB_ID)
        {
        }
    };

    /*! ****************************************************************************/
    /*! \Pure virtual functions to be implemented by each game mode.
        Report slave inheriting from this class will have to implemented the
        virtual functions.
    ********************************************************************************/
    virtual bool skipProcessConclusionType() = 0;
    virtual void initGameParams() = 0;
    virtual void getGameModeGameHistoryQuery(char8_t *query, int32_t querySize) = 0;
    virtual void determineGameResult() = 0;
    virtual void updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) = 0;
    virtual void updateGameModeWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) = 0;
    virtual void updateGameModeLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) = 0;
    virtual void updateGameModeTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) = 0;
    virtual void updatePlayerKeyscopes() = 0;
    virtual void updatePlayerStats() = 0;
    virtual void updatePlayerStatsConclusionType() = 0;
    virtual void updateGameStats() = 0;
    virtual bool processUpdatedStats() = 0;
    virtual void updateNotificationReport() = 0;
    virtual void sendEndGameMail(GameManager::PlayerIdList& playerIds) = 0;

    /*! ****************************************************************************/
    /*! \Virtual functions provided by GameCommonReportSlave that can be overwritten with custom functionalities
        Default implementations provided. Game teams can also override these functions if they choose to not
        use default implementations.
    ********************************************************************************/
    virtual GameCommonLobbySkill* getGameCommonLobbySkill() { return &mGameCommonLobbySkill; };
    virtual GameReportsList getGameReportListForConsecutiveMatchCount(GameReportsList& winnerByDnfReportsList, GameReportsList& gameReportsList) { return gameReportsList; };
    virtual bool skipProcess();
    virtual bool skipProcessConclusionTypeCommon();
    virtual void updateSkipProcessConclusionTypeStats();
    virtual bool didPlayerFinish(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
    virtual void setEntityScores(ProcessedGameReport& processedReport);
	virtual bool didGameFinish() { return true; }
    virtual int32_t getUnAdjustedHighScore() { return -1; }
    virtual int32_t getUnAdjustedLowScore() { return -1; }
    
    /*! ****************************************************************************/
    /*! \Functions called by each game mode and are not customizable by game teams.
    ********************************************************************************/
    void processCommon();
    void updateGlobalStats();
    int32_t getInt(const Collections::AttributeMap *map, const char *key);
    void adjustPlayerScore();
    void determineWinPlayer();
    void determinePlayerFinishReason(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
    void updateWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
    void updateLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
    void updateTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
    uint32_t calcPlayerDampingPercent(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
    void updatePlayerDNF(OSDKGameReportBase::OSDKPlayerReport& playerReport);

	virtual void selectCustomStats() {};

	ProcessedGameReport*                mProcessedReport;
    ReportConfig*                       mReportConfig;

   // GenerateMailUtil                    mGenerateMailUtil;

    Stats::UpdateStatsRequestBuilder    mBuilder;
    Stats::UpdateStatsHelper            mUpdateStatsHelper;

    PlayerScopeIndexMap                 mPlayerKeyscopes;

    NewRecordStruct                     mNewRecordArray[MAX_CLUB_NUMBER][NUMBER_OF_TROPHIES];

    ResultSet                           mTieSet;
    ResultSet                           mWinnerSet;
    ResultSet                           mLoserSet;

    bool                                mIsOfflineReport;
    bool                                mIsRankedGame;
    bool                                mTieGame;
    bool                                mIsUpdatePlayerConclusionTypeStats;

    uint32_t                            mGameTime;
    uint32_t                            mCategoryId;
    uint32_t                            mRoomId;
    uint32_t                            mWinTeamId;

    uint32_t                            mPlayerReportMapSize;

    int32_t                             mLowestPlayerScore;
    int32_t                             mHighestPlayerScore;

    EntityScoreMap                      mEntityScores;

private:

    /*! ****************************************************************************/
    /*! \Private helper functions.
    ********************************************************************************/
    void addDataForExternalSystems();
    int32_t getEntityScore(const char* entityId);

    GameCommonLobbySkill mGameCommonLobbySkill;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_GAMECOMMONREPORT_SLAVE_H
