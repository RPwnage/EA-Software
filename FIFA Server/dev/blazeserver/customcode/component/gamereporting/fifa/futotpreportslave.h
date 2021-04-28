/*************************************************************************************************/
/*!
    \file   futotpreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FUTOTPREPORT_SLAVE_H
#define BLAZE_CUSTOM_FUTOTPREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gameotpreportslave.h"
#include "gamereporting/fifa/fifabasereportslave.h"

#include "gamereporting/fifa/tdf/futotpreport.h"
#include "gamereporting/fifa/tdf/futreport_server.h"

//#include "gamereporting/fifa/fifacustom.h"
#include "gamereporting/fifa/fifa_lastopponent/fifa_lastopponent.h"
#include "gamereporting/fifa/fut_lastopponent/fut_lastopponentextensions.h"

#include "gamereporting/fifa/fut_utility/fut_utility.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{
/*!
    class FutOtpReportSlave
*/
class FutOtpReportSlave: public GameOTPReportSlave, 
						 public FifaBaseReportSlave<FutOTPReportBase::FutOTPTeamReportMap, FutOTPReportBase::FutOTPTeamReport, Fifa::CommonGameReport, Fifa::CommonPlayerReport>
{
public:
	FutOtpReportSlave(GameReportingSlaveImpl& component);
	virtual ~FutOtpReportSlave();

	static GameReportProcessor* create(GameReportingSlaveImpl& component);

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

protected:
	/*! ****************************************************************************/
    /*! \Virtual functions from GameCommonReportSlave class.
    ********************************************************************************/
	virtual bool skipProcess();
	virtual bool skipProcessConclusionType();
	virtual void initGameParams();
	virtual void updatePlayerKeyscopes();
	virtual bool processUpdatedStats();
	virtual void updateNotificationReport();
	virtual bool didGameFinish() { return !mGameDNF; }
 	virtual int32_t getUnAdjustedHighScore() { return mUnadjustedHighScore; }
 	virtual int32_t getUnAdjustedLowScore() { return mUnadjustedLowScore; }

	/*! ****************************************************************************/
    /*! \Virtual functions from GameOTPReportSlave
    ********************************************************************************/
	virtual void updatePlayerStats();
	virtual void updateGameStats();
	virtual void selectCustomStats();
	virtual void updateCustomTransformStats(const char8_t* statsCategory);
	//virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);

	/*! ****************************************************************************/
    /*! \Pure virtual functions from GameOTPReportSlave to be implemented by each game team's game mode custom class.
    ********************************************************************************/
	// GameOTPReportSlave implementation start
    virtual const char8_t* getCustomOTPGameTypeName() const;
    virtual const char8_t* getCustomOTPStatsCategoryName() const;
	// GameOTPReportSlave implementation end

	/*! ****************************************************************************/
	/*! \Functions that look like they might be inherited but are not
	********************************************************************************/
	// Called from updateNotificationReport
	virtual void updateCustomNotificationReport();
	// Called from updateGameStats
	virtual void updateCustomGameStats();

	/*! ****************************************************************************/
    /*! \Pure virtual functions from FifaBaseReportSlave class.
    ********************************************************************************/
	// FifaBaseReportSlave implementation start
	virtual void determineGameResult();

	virtual Fifa::CommonGameReport*					getGameReport();
	virtual FutOTPReportBase::FutOTPTeamReportMap*	getEntityReportMap();
	virtual Fifa::CommonPlayerReport*				getCommonPlayerReport(FutOTPReportBase::FutOTPTeamReport& entityReport);
	virtual bool									didEntityFinish(int64_t entityId, FutOTPReportBase::FutOTPTeamReport& entityReport);
	virtual	FutOTPReportBase::FutOTPTeamReport*		findEntityReport(int64_t entityId);

	virtual int64_t		getWinningEntity();
	virtual int64_t		getLosingEntity();
	virtual bool		isTieGame();
	virtual bool		isEntityWinner(int64_t entityId);
	virtual bool		isEntityLoser(int64_t entityId);

	virtual int32_t		getHighestEntityScore();
	virtual int32_t		getLowestEntityScore();

	virtual void		setCustomEntityResult(FutOTPReportBase::FutOTPTeamReport& entityReport, StatGameResultE gameResult);
	virtual void		setCustomEntityScore(FutOTPReportBase::FutOTPTeamReport& entityReport, uint16_t score, uint16_t goalsFor, uint16_t goalsAgainst);

	int64_t				mWinEntityId;
	int64_t				mLossEntityId;	
	int32_t				mLowestScore;
	int32_t				mHighestScore;
	bool				mGameFinish;
	// FifaBaseReportSlave implementation end

private:
	// Port from futh2hreportslave start
	bool isFUTMode(const char8_t* gameTypeName);
	bool isValidReport();
	void updatePlayerFUTOTPMatchResult(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKReport& osdkReport, FUT::MatchResult playerMatchResult);
	void updateGameHistory(ProcessedGameReport& processedReport, GameReport& report);
	//void updateFUTUserRating(Fifa::GameEndedReason reason, FUT::IndividualPlayerReport& futPlayerReport, const FUT::CollatorConfig* collatorConfig);
	//void updatePreMatchUserRating();
	void initMatchReportingStatsSelect();
	//void updateMatchReportingStats();
	void determineFUTGameResult();
	BlazeRpcError commitMatchReportingStats();
	const FUT::CollatorConfig* GetCollatorConfig();

	FifaLastOpponent mFifaLastOpponent;
	FutOtpLastOpponent mOtpLastOpponentExtension;
	// Port from futh2hreportslave end

	// FUT OTP
	void determineTeamResult();
	bool updateSquadResults();
	void updatePlayerUnadjustedScore();

	int32_t mReportGameTypeId;
	bool mShouldLogLastOpponent;
	//bool mEnableValidation; // TODO, wendy, collator?
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FUTOTPREPORT_SLAVE_H
