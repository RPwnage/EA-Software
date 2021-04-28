/*************************************************************************************************/
/*!
    \file   futh2hreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FUTH2HREPORT_SLAVE_H
#define BLAZE_CUSTOM_FUTH2HREPORT_SLAVE_H

/*** Include files *******************************************************************************/
//#include "gamereporting/fifa/fifacustom.h"
#include "gamereporting/fifa/fifa_lastopponent/fifa_lastopponent.h"
#include "gamereporting/fifa/fifa_lastopponent/fifa_lastopponentextensions.h"

#include "gamereporting/fifa/fifah2hbasereportslave.h"
#include "gamereporting/fifa/fut_utility/fut_utility.h"

#include "EASTL/fixed_map.h"

namespace Blaze
{
namespace GameReporting
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

typedef eastl::fixed_map<GameManager::PlayerId, FUT::MatchResult, 2> FUTMatchResultMap;

/*!
    class FutH2HReportSlave
*/
class FutH2HReportSlave: public FifaH2HBaseReportSlave
{
public:
    FutH2HReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~FutH2HReportSlave();

protected:
	/*! ****************************************************************************/
    /*! \Virtual functions from GameCommonReportSlave class.
    ********************************************************************************/
		virtual bool skipProcess();
		virtual bool skipProcessConclusionType();
		virtual void initGameParams();
		virtual void updatePlayerKeyscopes();

    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameH2HReportsSlave class.
    ********************************************************************************/
        virtual const char8_t* getCustomH2HGameTypeName() const;
        virtual const char8_t* getCustomH2HStatsCategoryName() const;
		virtual const uint16_t getFifaControlsSetting(OSDKGameReportBase::OSDKPlayerReport& playerReport) const;

    /*! ****************************************************************************/
    /*! \Virtual functions from GameH2HReportsSlave class.
    ********************************************************************************/
		virtual BlazeRpcError process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds);

        virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateCustomH2HSeasonalPlayerStats(OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateCustomGameStats(OSDKGameReportBase::OSDKReport& OsdkReport);
        virtual void updateCustomNotificationReport(const char8_t* statsCategory);
		/* A custom hook for Game team to add additional player keyscopes needed for stats update */
        virtual void updateCustomPlayerKeyscopes(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
    /* A custom hook for Game team to set parameters for the end game mail, return true if sending a mail */
        //virtual bool setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName);
    /* A custom hook for Game team to customize lobby skill info */
        virtual void setCustomLobbySkillInfo(GameCommonLobbySkill::LobbySkillInfo* skillInfo) {};
    /* A custom hook for Game team to aggregate additional player stats */
        virtual void aggregateCustomPlayerStats(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
	/* A custom hook  to set win/draw/loose in gamereport */	
		virtual void updatePlayerStats();

		virtual void selectCustomStats();
		virtual void updateCustomTransformStats(const char8_t* statsCategory);

private:

	bool isFUTMode(const char8_t* gameTypeName);
	bool isValidReport();
	void updatePlayerFUTMatchResult(GameManager::PlayerId playerId, GameManager::PlayerId opponentId, OSDKGameReportBase::OSDKReport& osdkReport, FUTMatchResultMap& matchResultMap, FUT::CollatorReport& collatorReport);
	void updateFUTMatchResult(GameManager::PlayerId homePlayerId, GameManager::PlayerId awayPlayerId, OSDKGameReportBase::OSDKReport& osdkReport, FUTMatchResultMap& matchResultMap);
	void updateGameHistory(ProcessedGameReport& processedReport, GameReport& report);
	void updateFUTUserRating(Fifa::GameEndedReason reason, FUT::IndividualPlayerReport& futPlayerReport, const FUT::CollatorConfig* collatorConfig);
	void updatePreMatchUserRating();
	void initMatchReportingStatsSelect();
	void updateMatchReportingStats();
	void determineFUTGameResult();
	BlazeRpcError commitMatchReportingStats();
	const FUT::CollatorConfig* GetCollatorConfig();

	FifaLastOpponent mFifaLastOpponent;
	HeadToHeadLastOpponent mH2HLastOpponentExtension;
	FUTMatchResultMap mMatchResultMap;

	int32_t mReportGameTypeId;
	bool mShouldLogLastOpponent;
	bool mEnableValidation;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FUTH2HREPORT_SLAVE_H

