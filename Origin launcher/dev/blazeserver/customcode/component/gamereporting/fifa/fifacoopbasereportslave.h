/*************************************************************************************************/
/*!
    \file   fifacoopbasereportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFACOOPBASEREPORT_SLAVE_H
#define BLAZE_CUSTOM_FIFACOOPBASEREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/fifa/fifacoopreportslave.h"
#include "gamereporting/fifa/fifabasereportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class FifaCoopBaseReportSlave
*/
class FifaCoopBaseReportSlave :	public FifaCoopReportSlave,
								public FifaBaseReportSlave<FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap, FifaCoopReportBase::FifaCoopSquadDetailsReportBase, Fifa::CommonGameReport, Fifa::CommonPlayerReport>
{
public:
    FifaCoopBaseReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~FifaCoopBaseReportSlave();

    /*! ****************************************************************************/
    /*! \brief Called when stats are reported following the process() call.
 
        \param processedReport Contains the final game report and information used by game history.
        \param playerIds list of players to distribute results to.
        \return ERR_OK on success.  GameReporting specific error on failure.
    ********************************************************************************/
    virtual BlazeRpcError process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds);

protected:
	virtual void determineGameResult();
	//virtual void updateCustomClubFreeAgentPlayerStats(OSDKGameReportBase::OSDKPlayerReport& playerReport);
	virtual bool didGameFinish() { return !mGameDNF; }
 	virtual int32_t getUnAdjustedHighScore() { return mUnadjustedHighScore; }
 	virtual int32_t getUnAdjustedLowScore() { return mUnadjustedLowScore; }

private:
	static const int DEFAULT_CONTROLS = 2;

	virtual Fifa::CommonGameReport*											getGameReport();
	virtual FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap*	getEntityReportMap();
	virtual Fifa::CommonPlayerReport*										getCommonPlayerReport(FifaCoopReportBase::FifaCoopSquadDetailsReportBase& entityReport);
	virtual bool															didEntityFinish(int64_t entityId, FifaCoopReportBase::FifaCoopSquadDetailsReportBase& entityReport);	
	virtual	FifaCoopReportBase::FifaCoopSquadDetailsReportBase*				findEntityReport(int64_t entityId);

	virtual int64_t						getWinningEntity();
	virtual int64_t						getLosingEntity();
	virtual bool						isTieGame();
	virtual bool						isEntityWinner(int64_t entityId);
	virtual bool						isEntityLoser(int64_t entityId);

	virtual int32_t						getHighestEntityScore();
	virtual int32_t						getLowestEntityScore();

	virtual void						setCustomEntityResult(FifaCoopReportBase::FifaCoopSquadDetailsReportBase& entityReport, StatGameResultE gameResult);
	virtual void						setCustomEntityScore(FifaCoopReportBase::FifaCoopSquadDetailsReportBase& entityReport, uint16_t score, uint16_t goalsFor, uint16_t goalsAgainst);

	bool								setupCoopIds();
	void								checkForSoloCoop();
	void								updateCoopDNF();

	void								updateUpperLowerBlazeIds(Blaze::BlazeId playerId, Blaze::BlazeId& upperBlazeId, Blaze::BlazeId& lowerBlazeId);
	void								updateSoloFlags(int count, bool& soloFlag, CoopSeason::CoopId coopId, Blaze::BlazeId& upperBlazeId, Blaze::BlazeId& lowerBlazeId);
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFACOOPBASEREPORT_SLAVE_H

