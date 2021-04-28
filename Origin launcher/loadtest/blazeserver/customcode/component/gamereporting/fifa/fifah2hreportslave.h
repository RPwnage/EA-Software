/*************************************************************************************************/
/*!
    \file   fifah2hreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFAH2HREPORT_SLAVE_H
#define BLAZE_CUSTOM_FIFAH2HREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/fifa/fifacustom.h"
#include "gamereporting/fifa/fifa_seasonalplay/fifa_seasonalplay.h"
#include "gamereporting/fifa/fifa_seasonalplay/fifa_seasonalplayextensions.h"
#include "gamereporting/fifa/fifa_elo_rpghybridskill/fifa_elo_rpghybridskill.h"
#include "gamereporting/fifa/fifa_elo_rpghybridskill/fifa_elo_rpghybridskillextensions.h"

#include "gamereporting/fifa/fifah2hbasereportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class FifaH2HReportSlave
*/
class FifaH2HReportSlave: public FifaH2HBaseReportSlave
{
public:
    FifaH2HReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~FifaH2HReportSlave();

protected:

    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameH2HReportsSlave class.
    ********************************************************************************/
        virtual const char8_t* getCustomH2HGameTypeName() const;
        virtual const char8_t* getCustomH2HStatsCategoryName() const;
		virtual const uint16_t getFifaControlsSetting(OSDKGameReportBase::OSDKPlayerReport& playerReport) const;

    /*! ****************************************************************************/
    /*! \Virtual functions from GameH2HReportsSlave class.
    ********************************************************************************/
        virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);

        virtual void updateCustomH2HSeasonalPlayerStats(OSDKGameReportBase::OSDKPlayerReport& playerReport);
		
		virtual BlazeRpcError initializeExtensions();
        virtual void updateCustomGameStats(OSDKGameReportBase::OSDKReport& OsdkReport);
        virtual void updateCustomNotificationReport(const char8_t* statsCategory);
    /* A custom hook for Game team to perform post stats update processing */
        virtual bool processCustomUpdatedStats() { return true;};
    /* A custom hook for Game team to add additional player keyscopes needed for stats update */
        virtual void updateCustomPlayerKeyscopes(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap);
    /* A custom hook for Game team to set parameters for the end game mail, return true if sending a mail */
        //virtual bool setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName);
    /* A custom hook for Game team to customize lobby skill info */
        virtual void setCustomLobbySkillInfo(GameCommonLobbySkill::LobbySkillInfo* skillInfo) {};
    /* A custom hook for Game team to aggregate additional player stats */
        virtual void aggregateCustomPlayerStats(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};

		virtual void selectCustomStats();
		virtual void updateCustomTransformStats(const char8_t* statsCategory);

		virtual void getLiveCompetitionsStatsCategoryName(char8_t* pOutputBuffer, uint32_t uOutputBufferSize) const;

		uint32_t determineResult(EntityId id, OSDKGameReportBase::OSDKPlayerReport& playerReport);

private:

	void selectH2HCustomStats();
	void selectLiveCompetitionCustomStats();
	void updateLiveCompetitionCustomStats(const char8_t* statsCategory);
	virtual void customFillNrmlXmlData(uint64_t entityId, eastl::string &buffer);

	struct StatCategoryInfo
	{
		eastl::string name;
		eastl::vector<eastl::string> keyscopes;
		eastl::vector<eastl::string> stats;
	};
	typedef eastl::vector<StatCategoryInfo> StatCategoryContainer;

	void buildStatRevertList(StatCategoryContainer& outParam, Stats::StatPeriodType periodType);
	void undoStats(BlazeId playerId, const StatCategoryContainer& stats, Stats::StatPeriodType periodType);
	void preStatCommitProcess();

	FifaSeasonalPlay				mSeasonalPlay;
	HeadtoHeadExtension				mSeasonalPlayExtension;
	LiveCompHeadtoHeadExtension		mLiveCompSeasonalPlayExtension;

	FifaEloRpgHybridSkill			mEloRpgHybridSkill;
	HeadtoHeadEloExtension			mEloRpgHybridSkillExtension;
	LiveCompEloExtension			mLiveCompEloRpgHybridSkillExtension;

	// Strings for the email at EOM 
	char8_t mName0Str[32];
	char8_t mTeam0Str[32];
	char8_t mResult0Str[32];
	char8_t mScore0Str[32];
	char8_t mShotsongoal0Str[32];
	char8_t mShots0Str[32];
	char8_t mPossession0Str[32];
	char8_t mPassing0Str[32];
	char8_t mTackle0Str[32];
	char8_t mCards0Str[32];
	char8_t mFouls0Str[32];
	char8_t mCorners0Str[32];

	char8_t mName1Str[32];
	char8_t mTeam1Str[32];
	char8_t mResult1Str[32];
	char8_t mScore1Str[32];
	char8_t mShotsongoal1Str[32];
	char8_t mShots1Str[32];
	char8_t mPossession1Str[32];
	char8_t mPassing1Str[32];
	char8_t mTackle1Str[32];
	char8_t mCards1Str[32];
	char8_t mFouls1Str[32];
	char8_t mCorners1Str[32];


};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFAH2HREPORT_SLAVE_H

