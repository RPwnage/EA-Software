/*************************************************************************************************/
/*!
    \file   purchaseoutcome_extension.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef EASFC_PURCHASE_OUTCOME_EXTENSION_H
#define EASFC_PURCHASE_OUTCOME_EXTENSION_H

/*** Include files *******************************************************************************/
#include "customcode/component/gamereporting/fifa/fifa_seasonalplay/fifa_seasonalplay.h"
#include "customcode/component/gamereporting/fifa/fifa_seasonalplay/fifa_seasonalplayextensions.h"
#include "gamereporting/fifa/fifastatsvalueutil.h"

namespace Blaze
{
	namespace EASFC
	{
		/*===============================================================================================
			SeasonPurchaseOutcomeExtension
		===============================================================================================*/
		class SeasonPurchaseOutcomeExtension : public Blaze::GameReporting::IFifaSeasonalPlayExtension
		{
		public:
			struct ExtensionData
			{
				EntityId mEntityId;
				GameResult mGameResult;
			};

			SeasonPurchaseOutcomeExtension();
			~SeasonPurchaseOutcomeExtension();

			virtual void setExtensionData(void* extensionData);

			virtual const char8_t* getDivStatCategory();
			virtual const char8_t* getOverallStatCategory();
			virtual const char8_t* getDivCountStatCategory();
		
			virtual bool trackLastOpponentPlayed() { return false; }
			virtual const char8_t* getLastOpponentStatCategory() { return ""; }

			virtual void getKeyScopes(Stats::ScopeNameValueMap& keyscopes) {}
		
			virtual void getEntityIds(eastl::vector<int64_t>& ids);

			virtual FifaCups::MemberType getMemberType() { return FifaCups::FIFACUPS_MEMBERTYPE_USER; }
			virtual EntityId getOpponentId(EntityId entityId) { return -1; };
			virtual bool didUserFinish(EntityId entityId) { return true; }

			virtual Stats::StatPeriodType getPeriodType() { return Stats::STAT_PERIOD_ALL_TIME; }

			virtual Blaze::OSDKTournaments::TournamentModes getCupMode() { return Blaze::OSDKTournaments::INVALID_MODE; }
			virtual void getCupResult(CupResult& cupResult) {}

			virtual void incrementTitlesWon(EntityId entityId, GameReporting::StatValueUtil::StatValue ** svSPOTitlesWon, int64_t div);

			virtual void incrementDivCount(int64_t div);
			virtual void decrementDivCount(int64_t div);

			virtual uint16_t getGoalsFor(EntityId entityId);
			virtual uint16_t getGoalsAgainst(EntityId entityId);
			virtual GameResult getGameResult(EntityId entityId);

			virtual void SetHookEntityCurrentDiv(int64_t div, EntityId entityId) {}
			virtual void SetHookEntityUpdateDiv(int64_t update, EntityId entityId) {}
			virtual void SetHookEntityWonCompetition(int32_t titleid, EntityId entityId) {}
			virtual void SetHookEntityWonLeagueTitle(EntityId entityId) {}
			virtual void SetHookEntityWonCup(EntityId entityId) {}
			virtual void SetHookEntitySeasonId(int64_t seasonid, EntityId entityId) {}
			virtual void SetHookEntityCupId(int64_t cupid, EntityId entityId) {}
			virtual void SetHookEntityGameNumber(int64_t gameNumber, EntityId entityId) {}
			virtual void SetHookEntityRankingPoints(int64_t value, EntityId entityId) {}
			virtual int64_t GetHookEntityRankingPoints(EntityId entityId) { return 0; }
			
			virtual void getDivCounts(eastl::vector<int>& incrementCounts, eastl::vector<int>& decrementCounts);

			virtual const Blaze::GameReporting::DefinesHelper* getDefinesHelper() const { return &mDefinesHelper; }

		protected:
			ExtensionData mExtensionData;
			Blaze::GameReporting::DefinesHelper mDefinesHelper;
		};

		/*===============================================================================================
			CoopSeasonPurchaseOutcomeExtension
		===============================================================================================*/
		class CoopSeasonPurchaseOutcomeExtension : public SeasonPurchaseOutcomeExtension
		{
		public:

			CoopSeasonPurchaseOutcomeExtension();
			~CoopSeasonPurchaseOutcomeExtension();
			
			virtual const char8_t* getDivStatCategory();
			virtual const char8_t* getOverallStatCategory();
			virtual const char8_t* getDivCountStatCategory();
			
			virtual FifaCups::MemberType getMemberType() { return FifaCups::FIFACUPS_MEMBERTYPE_COOP; }
			
			virtual void incrementTitlesWon(EntityId entityId, GameReporting::StatValueUtil::StatValue ** svSPOTitlesWon, int64_t div);

			virtual void incrementDivCount(int64_t div);
			virtual void decrementDivCount(int64_t div);
			
			virtual void getDivCounts(eastl::vector<int>& incrementCounts, eastl::vector<int>& decrementCounts);
			virtual const Blaze::GameReporting::DefinesHelper* getDefinesHelper() const { return &mCoopDefinesHelper; }

		protected:
			Blaze::GameReporting::CoopSeasonsDefinesHelper mCoopDefinesHelper;

		};

		/*===============================================================================================
			ProClubPurchaseOutcomeExtension
		===============================================================================================*/
		class ProClubPurchaseOutcomeExtension : public SeasonPurchaseOutcomeExtension
		{
		public:

			ProClubPurchaseOutcomeExtension();
			~ProClubPurchaseOutcomeExtension();

			virtual const char8_t* getDivStatCategory();
			virtual const char8_t* getOverallStatCategory();
			virtual const char8_t* getDivCountStatCategory();

			virtual FifaCups::MemberType getMemberType() { return FifaCups::FIFACUPS_MEMBERTYPE_CLUB; }

			virtual void incrementTitlesWon(EntityId entityId, GameReporting::StatValueUtil::StatValue ** svSPOTitlesWon, int64_t div);

			virtual void incrementDivCount(int64_t div);
			virtual void decrementDivCount(int64_t div);

			virtual void getDivCounts(eastl::vector<int>& incrementCounts, eastl::vector<int>& decrementCounts);

		};

	} // EASFC
} // Blaze

#endif // EASFC_PURCHASE_OUTCOME_EXTENSION_H
