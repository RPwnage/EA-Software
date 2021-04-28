/*************************************************************************************************/
/*!
    \file   fifah2hbasereportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFAH2HBASEREPORT_SLAVE_H
#define BLAZE_CUSTOM_FIFAH2HBASEREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gameh2hreportslave.h"
#include "gamereporting/fifa/fifabasereportslave.h"
#include "gamereporting/fifa/fifastatsvalueutil.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class FifaH2HBaseReportSlave
*/
class FifaH2HBaseReportSlave :	public GameH2HReportSlave,
								public FifaBaseReportSlave<OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap, OSDKGameReportBase::OSDKPlayerReport, Fifa::CommonGameReport, Fifa::CommonPlayerReport>
{
public:
    FifaH2HBaseReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~FifaH2HBaseReportSlave();

    /*! ****************************************************************************/
    /*! \brief Called when stats are reported following the process() call.
 
        \param processedReport Contains the final game report and information used by game history.
        \param playerIds list of players to distribute results to.
        \return ERR_OK on success.  GameReporting specific error on failure.
    ********************************************************************************/
    virtual BlazeRpcError process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds);
	virtual void determineGameResult();
 
protected:
	/*! *************************************************************************************/
    /// Function verifies that config is properly loaded for received event.
	/// Only callable after OSDK report is given to derived classes via "updateGameStats()"
    /*! *****************************************************************************************/
	virtual BlazeRpcError initializeExtensions() { return ERR_OK; }

	typedef eastl::vector<StatValueUtil::StatUpdate> StatUpdateList;
	typedef eastl::vector<Blaze::XmsHd::XmsAttributes*> XmsRequestMetadata;

	bool didAllPlayersFinish();
	virtual void sendDataToXms();
	
	virtual void getLiveCompetitionsStatsCategoryName(char8_t* pOutputBuffer, uint32_t uOutputBufferSize) const;

	/*! ****************************************************************************/
    /*! \Virtual functions provided by GameH2HReportSlave and GameCommonReportSlave
    ********************************************************************************/
	virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
	//virtual void getGameModeGameHistoryQuery(char8_t *query, int32_t querySize);
	virtual void updateNotificationReport();
	virtual void updatePlayerKeyscopes();
	virtual void updatePlayerStats();
	virtual void updatePlayerStatsConclusionType();
	virtual bool didGameFinish() { return !mGameDNF; }
 	virtual int32_t getUnAdjustedHighScore() { return mUnadjustedHighScore; }
 	virtual int32_t getUnAdjustedLowScore() { return mUnadjustedLowScore; }

	bool								mIsSponsoredEventReport;
	bool								mIsValidSponsoredEventId;
	uint32_t							mSponsoredEventId;

private:

	virtual Fifa::CommonGameReport*									getGameReport();
	virtual OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap*	getEntityReportMap();
	virtual Fifa::CommonPlayerReport*								getCommonPlayerReport(OSDKGameReportBase::OSDKPlayerReport& entityReport);
	virtual bool													didEntityFinish(int64_t entityId, OSDKGameReportBase::OSDKPlayerReport& entityReport);	
	virtual	OSDKGameReportBase::OSDKPlayerReport*					findEntityReport(int64_t entityId);

	BlazeRpcError													initH2HGameParams();

	virtual int64_t						getWinningEntity();
	virtual int64_t						getLosingEntity();
	virtual bool						isTieGame();
	virtual bool						isEntityWinner(int64_t entityId);
	virtual bool						isEntityLoser(int64_t entityId);

	virtual int32_t						getHighestEntityScore();
	virtual int32_t						getLowestEntityScore();

	virtual void						setCustomEntityResult(OSDKGameReportBase::OSDKPlayerReport& entityReport, StatGameResultE gameResult);
	virtual void						setCustomEntityScore(OSDKGameReportBase::OSDKPlayerReport& entityReport, uint16_t score, uint16_t goalsFor, uint16_t goalsAgainst);

	virtual void						updateGameModeWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
	virtual void						updateGameModeLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
	virtual void						updateGameModeTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
	virtual void						sendEndGameMail(GameManager::PlayerIdList& playerIds);

	virtual void						readNrmlXmlData(const char8_t* statsCategory, OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator & playerIter, StatUpdateList& statUpdates, Stats::StatDescs& getStatsDescsResponse);
	virtual void						fillNrmlXmlData(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator& playerIter, eastl::string &buffer);
	virtual void						customFillNrmlXmlData(uint64_t playerId, eastl::string &buffer){};
	void								sendDataToXmsFiberCall(uint64_t nucleusId, eastl::string xmlPayload, eastl::string assetName, XmsRequestMetadata metadata);

	virtual const eastl::string			getXmsAssetName(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator& playerIter);
	virtual	XmsRequestMetadata			getXmsRequestMetadata(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator& playerIter);
	virtual const char8_t*				getXmsDataRootElementName();

	virtual bool						sendXmsDataForPlayer(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator& playerIter) { return true; }


	virtual bool wasDNFPlayerWinning();
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFAH2HBASEREPORT_SLAVE_H

