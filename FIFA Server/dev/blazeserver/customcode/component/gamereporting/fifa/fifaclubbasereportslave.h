/*************************************************************************************************/
/*!
    \file   fifaclubbasereportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFACLUBBASEREPORT_SLAVE_H
#define BLAZE_CUSTOM_FIFACLUBBASEREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gameclubreportslave.h"
#include "gamereporting/fifa/fifabasereportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class FifaClubBaseReportSlave
*/
class FifaClubBaseReportSlave :	public GameClubReportSlave,
								public FifaBaseReportSlave<OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap, OSDKClubGameReportBase::OSDKClubClubReport, Fifa::CommonGameReport, Fifa::CommonPlayerReport>
{
public:
    FifaClubBaseReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~FifaClubBaseReportSlave();

    /*! ****************************************************************************/
    /*! \brief Called when stats are reported following the process() call.
 
        \param processedReport Contains the final game report and information used by game history.
        \param playerIds list of players to distribute results to.
        \return ERR_OK on success.  GameReporting specific error on failure.
    ********************************************************************************/
    virtual BlazeRpcError process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds);

protected:
	virtual void determineGameResult();
	virtual void updateCustomClubFreeAgentPlayerStats(OSDKGameReportBase::OSDKPlayerReport& playerReport){};
	virtual void updateCustomGameStats() {};
	
	virtual bool processPostStatsCommit() { return true; };
	virtual bool didGameFinish() { return !mGameDNF; }
 	virtual int32_t getUnAdjustedHighScore() { return mUnadjustedHighScore; }
 	virtual int32_t getUnAdjustedLowScore() { return mUnadjustedLowScore; }

	eastl::vector<int64_t>				mEntityFilterList;
private:

	virtual Fifa::CommonGameReport*										getGameReport();
	virtual OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap*	getEntityReportMap();
	virtual Fifa::CommonPlayerReport*									getCommonPlayerReport(OSDKClubGameReportBase::OSDKClubClubReport& entityReport);
	virtual bool														didEntityFinish(int64_t entityId, OSDKClubGameReportBase::OSDKClubClubReport& entityReport);	
	virtual	OSDKClubGameReportBase::OSDKClubClubReport*					findEntityReport(int64_t entityId);

	virtual int64_t						getWinningEntity();
	virtual int64_t						getLosingEntity();
	virtual bool						isTieGame();
	virtual bool						isEntityWinner(int64_t entityId);
	virtual bool						isEntityLoser(int64_t entityId);

	virtual int32_t						getHighestEntityScore();
	virtual int32_t						getLowestEntityScore();

	virtual void						setCustomEntityResult(OSDKClubGameReportBase::OSDKClubClubReport& entityReport, StatGameResultE gameResult);
	virtual void						setCustomEntityScore(OSDKClubGameReportBase::OSDKClubClubReport& entityReport, uint16_t score, uint16_t goalsFor, uint16_t goalsAgainst);
	virtual void						sendEndGameMail(GameManager::PlayerIdList& playerIds);
	virtual void						CustomizeGameReportForEventReporting(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds);

	void								populateEntityFilterList();
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFACLUBBASEREPORT_SLAVE_H

