/*************************************************************************************************/
/*!
    \file   SSFBaseReportSlave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_SSFBASEREPORT_SLAVE_H
#define BLAZE_CUSTOM_SSFBASEREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/fifa/ssfreportslave.h"
#include "gamereporting/fifa/fifabasereportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class SSFBaseReportSlave
*/
class SSFBaseReportSlave :	public SSFReportSlave,
							public FifaBaseReportSlave<SSFSeasonsReportBase::SSFTeamReportMap, SSFSeasonsReportBase::SSFTeamReport, SSF::CommonGameReport, SSF::CommonStatsReport>
{
public:
    SSFBaseReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~SSFBaseReportSlave();

    /*! ****************************************************************************/
    /*! \brief Called when stats are reported following the process() call.
 
        \param processedReport Contains the final game report and information used by game history.
        \param playerIds list of players to distribute results to.
        \return ERR_OK on success.  GameReporting specific error on failure.
    ********************************************************************************/
    virtual BlazeRpcError process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds);

protected:
	virtual void determineGameResult();

	virtual void updateGameStats() {};
	virtual bool didGameFinish() { return !mGameDNF; }
 	virtual int32_t getUnAdjustedHighScore() { return mUnadjustedHighScore; }
 	virtual int32_t getUnAdjustedLowScore() { return mUnadjustedLowScore; }
    

private:
	static const int DEFAULT_CONTROLS = 2;

	virtual SSF::CommonGameReport*											getGameReport();
	virtual SSFSeasonsReportBase::SSFTeamReportMap*							getEntityReportMap();
	virtual SSF::CommonStatsReport*											getCommonPlayerReport(SSFSeasonsReportBase::SSFTeamReport& entityReport);
	virtual bool															didEntityFinish(int64_t entityId, SSFSeasonsReportBase::SSFTeamReport& entityReport);	
	virtual	SSFSeasonsReportBase::SSFTeamReport*							findEntityReport(int64_t entityId);

	virtual int64_t						getWinningEntity();
	virtual int64_t						getLosingEntity();
	virtual bool						isTieGame();
	virtual bool						isEntityWinner(int64_t entityId);
	virtual bool						isEntityLoser(int64_t entityId);

	virtual int32_t						getHighestEntityScore();
	virtual int32_t						getLowestEntityScore();

	virtual void						setCustomEntityResult(SSFSeasonsReportBase::SSFTeamReport& entityReport, StatGameResultE gameResult);
	virtual void						setCustomEntityScore(SSFSeasonsReportBase::SSFTeamReport& entityReport, uint16_t score, uint16_t goalsFor, uint16_t goalsAgainst);

	void								updateDNF();
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_SSFBASEREPORT_SLAVE_H

