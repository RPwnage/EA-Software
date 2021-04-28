/*************************************************************************************************/
/*!
    \file   ssfsoloreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_SSFSOLOREPORT_SLAVE_H
#define BLAZE_CUSTOM_SSFSOLOREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gamesoloreportslave.h"
#include "gamereporting/fifa/fifabasereportslave.h"
#include "fifa/tdf/ssfseasonsreport.h"
#include "gamereporting/fifa/ssfgameresulthelper.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class SSFSoloReportSlave
*/
class SSFSoloReportSlave: public GameSoloReportSlave,
						public FifaBaseReportSlave<SSFSeasonsReportBase::SSFTeamReportMap, SSFSeasonsReportBase::SSFTeamReport, SSF::CommonGameReport, SSF::CommonStatsReport>,
						public SSFGameResultHelper<SSFSoloReportSlave>
{
public:
    SSFSoloReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~SSFSoloReportSlave();

	virtual BlazeRpcError process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds);

	// SSFGameResultHelper requirement implementation
	OSDKGameReportBase::OSDKReport& getOsdkReport();
	bool& getTieGame() { return mTieGame; }
	ResultSet& getTieSet() { return mTieSet; }
	ResultSet& getWinnerSet() { return mWinnerSet; }
	ResultSet& getLoserSet() { return mLoserSet; }

protected:
    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameSoloReportsSlave class.
    ********************************************************************************/
        virtual const char8_t* getCustomSoloGameTypeName() const;
        virtual const char8_t* getCustomSoloStatsCategoryName() const;

    /*! ****************************************************************************/
    /*! \Virtual functions from GameH2HReportsSlave class.
    ********************************************************************************/
		virtual void initCustomGameParams() {};
		virtual void updatePlayerStats();
		virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
		virtual void updateCustomNotificationReport();
		/* A custom hook for Game team to perform post stats update processing */
		virtual bool processCustomUpdatedStats() { return true; };
		/* A custom hook for Game team to add additional player keyscopes needed for stats update */
		virtual void updateCustomPlayerKeyscopes(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
		/* A custom hook for Game team to set parameters for the end game mail, return true if sending a mail */
		//virtual bool setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName);

		int32_t GetTeamGoals(SSFSeasonsReportBase::GoalEventVector& goalEvents, int64_t teamId) const;
		bool isManOfMatchOnTheTeam(SSFSeasonsReportBase::SSFTeamReport& teamReport) const;

		// FifaBaseReportSlave implementation
		virtual void determineGameResult();

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

private:
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_SSFSOLOREPORT_SLAVE_H

