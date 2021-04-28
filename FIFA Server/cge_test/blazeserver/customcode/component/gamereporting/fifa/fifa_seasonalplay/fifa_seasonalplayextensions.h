/*************************************************************************************************/
/*!
    \file   fifa_seasonalplayextensions.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFA_SEASONALPLAYEXTENSIONS_H
#define BLAZE_CUSTOM_FIFA_SEASONALPLAYEXTENSIONS_H

/*** Include files *******************************************************************************/
#include "fifacups/tdf/fifacupstypes.h"
#include "fifa_seasonalplaydefines.h"
#include "gamereporting/osdk/tdf/gameosdkreport.h"
#include "gamereporting/osdk/tdf/gameosdkclubreport.h"
#include "gamereporting/fifa/tdf/fifacoopreport.h"
#include "gamereporting/processedgamereport.h"
#include "customcomponent/osdktournaments/tdf/osdktournamentstypes_custom.h"
#include "gamereporting/fifa/fifastatsvalueutil.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace GameReporting
{

/*!
    class FifaSeasonalPlayExtension
*/
class IFifaSeasonalPlayExtension
{
public:
	IFifaSeasonalPlayExtension() {}
	virtual ~IFifaSeasonalPlayExtension() {}

	virtual BlazeRpcError configLoadStatus() const { return ERR_OK; }
	virtual void setExtensionData(void* extensionData) = 0;

	virtual const char8_t* getDivStatCategory() = 0;
	virtual const char8_t* getOverallStatCategory() = 0;
	virtual const char8_t* getDivCountStatCategory() = 0;

	virtual bool trackLastOpponentPlayed() = 0;
	virtual const char8_t* getLastOpponentStatCategory() = 0;

	virtual void getKeyScopes(Stats::ScopeNameValueMap& keyscopes) = 0;

	typedef eastl::vector<int64_t> EntityList;
	virtual void getEntityIds(EntityList& ids) = 0;

	virtual FifaCups::MemberType getMemberType() = 0;
	enum GameResult
	{
		LOSS = 0,
		TIE,
		WIN,
		INVALID
	};
	virtual GameResult getGameResult(EntityId entityId) = 0;
	virtual EntityId getOpponentId(EntityId entityId) = 0;
	virtual uint16_t getGoalsFor(EntityId entityId) = 0;
	virtual uint16_t getGoalsAgainst(EntityId entityId) = 0;
	virtual bool didUserFinish(EntityId entityId) = 0;

	virtual Stats::StatPeriodType getPeriodType() = 0;

	virtual const DefinesHelper* getDefinesHelper() const = 0;
	enum {
		HOOK_DIV_REL=-1,
		HOOK_NOTHING,
		HOOK_DIV_REP,
		HOOK_DIV_PRO 
	};
	enum {
		CUP_WINNER,
		CUP_LOSER,
		CUP_NUM_MAX 
	};
	struct CupSideInfo
	{
		CupSideInfo()
		{
			id = INVALID_BLAZE_ID;
			teamId = 0;
			score = 0;
		}

		EntityId id;
		int teamId;
		int score;
	};
	struct CupResult
	{
		CupResult()
		{
			mWentToPenalties = false;;
		}
		CupSideInfo mSideInfo[CUP_NUM_MAX];
		bool mWentToPenalties;
	};
	virtual Blaze::OSDKTournaments::TournamentModes getCupMode() = 0;
	virtual void getCupResult(CupResult& cupResult) = 0;
	virtual void incrementTitlesWon(EntityId entityId, StatValueUtil::StatValue ** svSPOTitlesWon, int64_t div)=0;
	virtual void incrementDivCount(int64_t div) = 0;
	virtual void decrementDivCount(int64_t div) = 0;
	virtual void getDivCounts(eastl::vector<int>& incrementCounts, eastl::vector<int>& decrementCounts) = 0;
	virtual void SetHookEntityCurrentDiv(int64_t div, EntityId entityId)=0;
	virtual void SetHookEntityUpdateDiv(int64_t update, EntityId entityId)=0;
	virtual void SetHookEntityWonCompetition(int32_t titleid, EntityId entityId)=0;
	virtual void SetHookEntityWonLeagueTitle(EntityId entityId)=0;
	virtual void SetHookEntityWonCup(EntityId entityId) = 0;
	virtual void SetHookEntitySeasonId(int64_t seasonid, EntityId entityId)=0;
	virtual void SetHookEntityCupId(int64_t cupid, EntityId entityId)=0;
	virtual void SetHookEntityGameNumber(int64_t gameNumber, EntityId entityId)=0;
	virtual void SetHookEntityRankingPoints(int64_t value, EntityId entityId) = 0;
	virtual int64_t GetHookEntityRankingPoints(EntityId entityId) = 0;
};

class HeadtoHeadExtension : public IFifaSeasonalPlayExtension
{
public:
	typedef eastl::hash_set<GameManager::PlayerId> ResultSet;
	struct ExtensionData
	{
		ExtensionData()
			: mProcessedReport(NULL)
			, mWinnerSet(NULL)
			, mLoserSet(NULL)
			, mTieSet(NULL)
			, mTieGame(false)
		{
		}
		ProcessedGameReport*				mProcessedReport;
		
		ResultSet*				mWinnerSet;
		ResultSet*				mLoserSet;
		ResultSet*				mTieSet;

		bool					mTieGame;
	};
	HeadtoHeadExtension();
	~HeadtoHeadExtension();

	virtual void setExtensionData(void* extensionData);

	virtual const char8_t* getDivStatCategory();
	virtual const char8_t* getOverallStatCategory();
	virtual const char8_t* getDivCountStatCategory();

	virtual bool trackLastOpponentPlayed() { return true; }
	virtual const char8_t* getLastOpponentStatCategory();

	virtual void getKeyScopes(Stats::ScopeNameValueMap& keyscopes) {}

	virtual void getEntityIds(eastl::vector<int64_t>& ids);

	virtual FifaCups::MemberType getMemberType();
	virtual GameResult getGameResult(EntityId entityId);
	virtual EntityId getOpponentId(EntityId entityId);
	virtual uint16_t getGoalsFor(EntityId entityId);
	virtual uint16_t getGoalsAgainst(EntityId entityId);
	virtual bool didUserFinish(EntityId entityId);

	virtual Stats::StatPeriodType getPeriodType();

	virtual Blaze::OSDKTournaments::TournamentModes getCupMode() { return Blaze::OSDKTournaments::MODE_FIFA_CUPS_USER; }
	virtual void getCupResult(CupResult& cupResult);
	virtual void incrementTitlesWon(EntityId entityId, StatValueUtil::StatValue ** svSPOTitlesWon, int64_t div);
	virtual void incrementDivCount(int64_t div);
	virtual void decrementDivCount(int64_t div);
	virtual void getDivCounts(eastl::vector<int>& incrementCounts, eastl::vector<int>& decrementCounts);
	virtual void SetHookEntityCurrentDiv(int64_t div, EntityId entityId);
	virtual void SetHookEntityUpdateDiv(int64_t update,EntityId entityId);
	virtual void SetHookEntityWonCompetition(int32_t titleid, EntityId entityId);
	virtual void SetHookEntityWonLeagueTitle(EntityId entityId);
	virtual void SetHookEntityWonCup(EntityId entityId) {}
	virtual void SetHookEntitySeasonId(int64_t seasonid, EntityId entityId);
	virtual void SetHookEntityCupId(int64_t cupid, EntityId entityId);
	virtual void SetHookEntityGameNumber(int64_t gameNumber, EntityId entityId);
	virtual void SetHookEntityRankingPoints(int64_t value, EntityId entityId) {}	// not needed for H2H
	virtual int64_t GetHookEntityRankingPoints(EntityId entityId) { return 0; }		// not needed for H2H

	virtual BlazeRpcError configLoadStatus() const { return mDefinesHelper.configLoadStatus(); }
	virtual const DefinesHelper* getDefinesHelper() const {return &mDefinesHelper;}

	DefinesHelper mDefinesHelper;

	enum CountType
	{
		INCREMENT,
		DECREMENT,
		COUNT_MAX
	};
	struct DivCountData
	{
	public:
		DivCountData()
		{
			memset(mDivCounts, 0, sizeof(mDivCounts));
		}
		int mDivCounts[SP_MAX_NUM_DIVISIONS][COUNT_MAX];
	};

	static DivCountData mDivCountData;
	static EA::Thread::Mutex mMutex;
	
protected:
	OSDKGameReportBase::OSDKPlayerReport* getOsdkPlayerReport(EntityId entityId);

private:
	ExtensionData mExtensionData;
	
};

class ClubSeasonsExtension : public IFifaSeasonalPlayExtension
{
public:
	struct ExtensionData
	{
		ProcessedGameReport*	mProcessedReport;

		EntityId				mWinClubId;
		EntityId				mLossClubId;
		bool					mTieGame;

		int64_t                 mWinDivId;
		int64_t                 mLossDivId;
	};
	ClubSeasonsExtension();
	~ClubSeasonsExtension();

	virtual void setExtensionData(void* extensionData);

	virtual const char8_t* getDivStatCategory();
	virtual const char8_t* getOverallStatCategory();
	virtual const char8_t* getDivCountStatCategory();

	virtual bool trackLastOpponentPlayed() { return false; }
	virtual const char8_t* getLastOpponentStatCategory();

	virtual void getKeyScopes(Stats::ScopeNameValueMap& keyscopes) {}

	virtual void getEntityIds(eastl::vector<int64_t>& ids);
	virtual int64_t getDivision(EntityId entityId);

	virtual FifaCups::MemberType getMemberType();
	virtual GameResult getGameResult(EntityId entityId);
	virtual EntityId getOpponentId(EntityId entityId) { return -1; };
	virtual uint16_t getGoalsFor(EntityId entityId);
	virtual uint16_t getGoalsAgainst(EntityId entityId);
	virtual bool didUserFinish(EntityId entityId);

	virtual Stats::StatPeriodType getPeriodType();

	virtual Blaze::OSDKTournaments::TournamentModes getCupMode() { return Blaze::OSDKTournaments::MODE_FIFA_CUPS_CLUB; }

	virtual void getCupResult(CupResult& cupResult);
	virtual void incrementTitlesWon(EntityId entityId, StatValueUtil::StatValue ** svSPOTitlesWon, int64_t div);
	virtual void incrementDivCount(int64_t div);
	virtual void decrementDivCount(int64_t div);
	virtual void getDivCounts(eastl::vector<int>& incrementCounts, eastl::vector<int>& decrementCounts);
	virtual void SetHookEntityCurrentDiv(int64_t div, EntityId entityId);
	virtual void SetHookEntityUpdateDiv(int64_t update, EntityId entityId);
	virtual void SetHookEntityWonCompetition(int32_t titleid, EntityId entityId);
	virtual void SetHookEntityWonLeagueTitle(EntityId entityId);
	virtual void SetHookEntityWonCup(EntityId entityId);
	virtual void SetHookEntitySeasonId(int64_t seasonid, EntityId entityId);
	virtual void SetHookEntityCupId(int64_t cupid, EntityId entityId);
	virtual void SetHookEntityGameNumber(int64_t gameNumber, EntityId entityId);
	virtual void SetHookEntityRankingPoints(int64_t value, EntityId entityId) {}	// not needed for Clubs
	virtual int64_t GetHookEntityRankingPoints(EntityId entityId) { return 0; }		// not needed for Clubs

	virtual const DefinesHelper* getDefinesHelper() const {return &mDefinesHelper;}
	
	void UpdateEntityFilterList(eastl::vector<int64_t> filteredIds);

	enum CountType
	{
		INCREMENT,
		DECREMENT,
		COUNT_MAX
	};


	struct DivCountData
	{
	public:
		DivCountData()
		{
			memset(mDivCounts, 0, sizeof(mDivCounts));
		}
		int mDivCounts[SP_MAX_NUM_DIVISIONS][COUNT_MAX];
	};

	DefinesHelper mDefinesHelper;
	ExtensionData mExtensionData;
	static DivCountData mDivCountData;
	static EA::Thread::Mutex mMutex;
	
	private:
	OSDKClubGameReportBase::OSDKClubClubReport* getOsdkClubReport(EntityId entityId);
	int64_t& getDivisionReference(EntityId entityId);	
	bool NeedToFilter(int64_t entityId);
		 
	eastl::vector<int64_t>	mEntityFilterList;
};

class CoopSeasonsExtension : public IFifaSeasonalPlayExtension
{
public:
	struct ExtensionData
	{
		ProcessedGameReport*	mProcessedReport;

		EntityId				mWinCoopId;
		EntityId				mLossCoopId;
		bool					mTieGame;
	};
	CoopSeasonsExtension();
	~CoopSeasonsExtension();

	virtual void setExtensionData(void* extensionData);

	virtual const char8_t* getDivStatCategory();
	virtual const char8_t* getOverallStatCategory();
	virtual const char8_t* getDivCountStatCategory();

	virtual bool trackLastOpponentPlayed() { return false; }
	virtual const char8_t* getLastOpponentStatCategory();

	virtual void getKeyScopes(Stats::ScopeNameValueMap& keyscopes) {}

	virtual void getEntityIds(eastl::vector<int64_t>& ids);

	virtual FifaCups::MemberType getMemberType();
	virtual GameResult getGameResult(EntityId entityId);
	virtual EntityId getOpponentId(EntityId entityId) { return -1; };
	virtual uint16_t getGoalsFor(EntityId entityId);
	virtual uint16_t getGoalsAgainst(EntityId entityId);
	virtual bool didUserFinish(EntityId entityId);

	virtual Stats::StatPeriodType getPeriodType();

	virtual Blaze::OSDKTournaments::TournamentModes getCupMode() { return Blaze::OSDKTournaments::MODE_FIFA_CUPS_COOP; }

	virtual void getCupResult(CupResult& cupResult);
	virtual void incrementTitlesWon(EntityId entityId, StatValueUtil::StatValue ** svSPOTitlesWon, int64_t div);
	virtual void incrementDivCount(int64_t div);
	virtual void decrementDivCount(int64_t div);
	virtual void getDivCounts(eastl::vector<int>& incrementCounts, eastl::vector<int>& decrementCounts);
	virtual void SetHookEntityCurrentDiv(int64_t div, EntityId entityId);
	virtual void SetHookEntityUpdateDiv(int64_t update, EntityId entityId);
	virtual void SetHookEntityWonCompetition(int32_t titleid, EntityId entityId);
	virtual void SetHookEntityWonLeagueTitle(EntityId entityId);
	virtual void SetHookEntityWonCup(EntityId entityId) {}
	virtual void SetHookEntitySeasonId(int64_t seasonid, EntityId entityId);
	virtual void SetHookEntityCupId(int64_t cupid, EntityId entityId);
	virtual void SetHookEntityGameNumber(int64_t gameNumber, EntityId entityId);
	virtual void SetHookEntityRankingPoints(int64_t value, EntityId entityId) {}	// not needed for Clubs
	virtual int64_t GetHookEntityRankingPoints(EntityId entityId) { return 0; }		// not needed for Clubs

	virtual const DefinesHelper* getDefinesHelper() const {return &mCoopSeasonsDefinesHelper;}

	CoopSeasonsDefinesHelper mCoopSeasonsDefinesHelper;

	enum CountType
	{
		INCREMENT,
		DECREMENT,
		COUNT_MAX
	};
	struct DivCountData
	{
	public:
		DivCountData()
		{
			memset(mDivCounts, 0, sizeof(mDivCounts));
		}
		int mDivCounts[SP_MAX_NUM_DIVISIONS][COUNT_MAX];
	};

	static DivCountData mDivCountData;
	static EA::Thread::Mutex mMutex;

private:
	FifaCoopReportBase::FifaCoopSquadDetailsReportBase* getSquadReport(EntityId entityId);

	ExtensionData mExtensionData;	
};

class LiveCompHeadtoHeadExtension : public HeadtoHeadExtension
{
public:

	struct ExtensionData
	{
		ExtensionData()
			: mProcessedReport(NULL)
			, mWinnerSet(NULL)
			, mLoserSet(NULL)
			, mTieSet(NULL)
			, mTieGame(false)
			, mCompetitionId(0)
		{
		}
		ProcessedGameReport*				mProcessedReport;

		ResultSet*				mWinnerSet;
		ResultSet*				mLoserSet;
		ResultSet*				mTieSet;
		bool					mTieGame;
		int						mCompetitionId;
	};

	LiveCompHeadtoHeadExtension();
	~LiveCompHeadtoHeadExtension();

	virtual void setExtensionData(void* extensionData);

	virtual const char8_t* getDivStatCategory();
	virtual const char8_t* getOverallStatCategory();
	virtual const char8_t* getDivCountStatCategory();

	virtual bool trackLastOpponentPlayed() { return true; }
	virtual const char8_t* getLastOpponentStatCategory();

	virtual void getKeyScopes(Stats::ScopeNameValueMap& keyscopes);

	virtual void getEntityIds(eastl::vector<int64_t>& ids);

	virtual FifaCups::MemberType getMemberType();
	virtual GameResult getGameResult(EntityId entityId);
	virtual EntityId getOpponentId(EntityId entityId);
	virtual uint16_t getGoalsFor(EntityId entityId);
	virtual uint16_t getGoalsAgainst(EntityId entityId);
	virtual bool didUserFinish(EntityId entityId);

	virtual Stats::StatPeriodType getPeriodType();

	virtual Blaze::OSDKTournaments::TournamentModes getCupMode() { return Blaze::OSDKTournaments::INVALID_MODE; }
	virtual void getCupResult(CupResult& cupResult);
	virtual void incrementTitlesWon(EntityId entityId, StatValueUtil::StatValue ** svSPOTitlesWon, int64_t div);
	virtual void incrementDivCount(int64_t div);
	virtual void decrementDivCount(int64_t div);
	virtual void getDivCounts(eastl::vector<int>& incrementCounts, eastl::vector<int>& decrementCounts);
	virtual void SetHookEntityCurrentDiv(int64_t div, EntityId entityId);
	virtual void SetHookEntityUpdateDiv(int64_t update, EntityId entityId);
	virtual void SetHookEntityWonCompetition(int32_t titleid, EntityId entityId);
	virtual void SetHookEntityWonLeagueTitle(EntityId entityId);
	virtual void SetHookEntityWonCup(EntityId entityId) {}
	virtual void SetHookEntitySeasonId(int64_t seasonid, EntityId entityId);
	virtual void SetHookEntityCupId(int64_t cupid, EntityId entityId) {}
	virtual void SetHookEntityGameNumber(int64_t gameNumber, EntityId entityId) {}
	virtual void SetHookEntityRankingPoints(int64_t value, EntityId entityId);

	virtual int64_t GetHookEntityRankingPoints(EntityId entityId);

	virtual BlazeRpcError fetchConfig(const char *configName) { return mLiveCompDefinesHelper.fetchConfig(configName); }
	virtual BlazeRpcError configLoadStatus() const { return mLiveCompDefinesHelper.configLoadStatus(); }
	virtual const DefinesHelper* getDefinesHelper() const {return &mLiveCompDefinesHelper;}

	LiveCompDefinesHelper mLiveCompDefinesHelper;

	typedef eastl::map<EntityId, int64_t> RankingPointsMap;
	RankingPointsMap mRankPointsMap;	

private:
	OSDKGameReportBase::OSDKPlayerReport* getOsdkPlayerReport(EntityId entityId);

	ExtensionData mExtensionData;

	enum CountType
	{
		INCREMENT,
		DECREMENT,
		COUNT_MAX
	};
	struct DivCountData
	{
	public:
		DivCountData()
		{
			memset(mDivCounts, 0, sizeof(mDivCounts));
		}
		int mDivCounts[SP_MAX_NUM_DIVISIONS][COUNT_MAX];
	};

	static DivCountData mDivCountData;
	static EA::Thread::Mutex mMutex;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFA_SEASONALPLAYEXTENSIONS_H

