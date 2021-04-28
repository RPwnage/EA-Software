/*************************************************************************************************/
/*!
    \file   purchaseoutcome_extension.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "purchaseoutcome_extension.h"

namespace Blaze
{
	namespace EASFC
	{
		/*===============================================================================================
			SeasonPurchaseOutcomeExtension
		===============================================================================================*/
		SeasonPurchaseOutcomeExtension::SeasonPurchaseOutcomeExtension() : IFifaSeasonalPlayExtension()
		{
		}
		SeasonPurchaseOutcomeExtension::~SeasonPurchaseOutcomeExtension()
		{
		}

		void SeasonPurchaseOutcomeExtension::setExtensionData(void* extensionData)
		{ 
			if (extensionData != NULL)
			{
				ExtensionData* data = reinterpret_cast<ExtensionData*>(extensionData);
				mExtensionData = *data;
			}
		}

		const char8_t* SeasonPurchaseOutcomeExtension::getDivStatCategory()
		{
			return "SPDivisionalPlayerStats";
		}

		const char8_t* SeasonPurchaseOutcomeExtension::getOverallStatCategory()
		{
			return "SPOverallPlayerStats";
		}

		const char8_t* SeasonPurchaseOutcomeExtension::getDivCountStatCategory()
		{
			return "SPDivisionCount";
		}
		
		void SeasonPurchaseOutcomeExtension::getEntityIds(eastl::vector<int64_t>& ids)
		{
			ids.push_back(mExtensionData.mEntityId);
		}
		
		void SeasonPurchaseOutcomeExtension::incrementTitlesWon(EntityId entityId, GameReporting::StatValueUtil::StatValue ** svSPOTitlesWon, int64_t div)
		{
			if(mExtensionData.mGameResult == IFifaSeasonalPlayExtension::LOSS)
			{
				return;
			}

			// map between division number (vector position) and type of title(vector value) 
			int titleMapId = getDefinesHelper()->GetIntSetting(Blaze::GameReporting::DefinesHelper::TITLE_MAP, div);

			Stats::StatPeriodType statPeriod=getPeriodType();
			// increment global titles won
			svSPOTitlesWon[0]->iAfter[statPeriod]++;

			// increment titles won of each type 
			svSPOTitlesWon[titleMapId]->iAfter[statPeriod]++;

			SetHookEntityWonCompetition(titleMapId, entityId);

			if(div == getDefinesHelper()->GetIntSetting(Blaze::GameReporting::DefinesHelper::NUM_DIVISIONS))
			{
				// top division, this is the league titles
				SetHookEntityWonLeagueTitle(entityId);
			}
		}

		void SeasonPurchaseOutcomeExtension::incrementDivCount(int64_t div) 
		{
			Blaze::GameReporting::HeadtoHeadExtension::mMutex.Lock();
			
			Blaze::GameReporting::HeadtoHeadExtension::mDivCountData.mDivCounts[div-1][Blaze::GameReporting::HeadtoHeadExtension::INCREMENT]++;

			Blaze::GameReporting::HeadtoHeadExtension::mMutex.Unlock();
		}

		void SeasonPurchaseOutcomeExtension::decrementDivCount(int64_t div) 
		{
			Blaze::GameReporting::HeadtoHeadExtension::mMutex.Lock();
			
			Blaze::GameReporting::HeadtoHeadExtension::mDivCountData.mDivCounts[div-1][Blaze::GameReporting::HeadtoHeadExtension::DECREMENT]++;

			Blaze::GameReporting::HeadtoHeadExtension::mMutex.Unlock();
		}
		
		uint16_t SeasonPurchaseOutcomeExtension::getGoalsFor(EntityId entityId)
		{
			uint16_t rez = 0;

			switch(mExtensionData.mGameResult)
			{
			case IFifaSeasonalPlayExtension::WIN:
				rez = (entityId == mExtensionData.mEntityId) ? 3 : 0;
				break;
			case IFifaSeasonalPlayExtension::TIE:
				rez = 0;
				break;
			case IFifaSeasonalPlayExtension::LOSS:
				rez = (entityId == mExtensionData.mEntityId) ? 0 : 3;;
				break;
			default:
				break;
			}

			return rez;
		}

		uint16_t SeasonPurchaseOutcomeExtension::getGoalsAgainst(EntityId entityId)
		{
			uint16_t rez = 0;

			switch(mExtensionData.mGameResult)
			{
			case IFifaSeasonalPlayExtension::WIN:
				rez = (entityId == mExtensionData.mEntityId) ? 0 : 3;
				break;
			case IFifaSeasonalPlayExtension::TIE:
				rez = 0;
				break;
			case IFifaSeasonalPlayExtension::LOSS:
				rez = (entityId == mExtensionData.mEntityId) ? 3 : 0;
				break;
			default:
				break;
			}

			return rez;
		}

		Blaze::GameReporting::IFifaSeasonalPlayExtension::GameResult SeasonPurchaseOutcomeExtension::getGameResult(EntityId entityId)
		{
			GameResult rez = IFifaSeasonalPlayExtension::INVALID;

			switch(mExtensionData.mGameResult)
			{
			case IFifaSeasonalPlayExtension::WIN:
				rez = (entityId == mExtensionData.mEntityId) ? IFifaSeasonalPlayExtension::WIN : IFifaSeasonalPlayExtension::LOSS;
				break;
			case IFifaSeasonalPlayExtension::TIE:
				rez = IFifaSeasonalPlayExtension::TIE;
				break;
			case IFifaSeasonalPlayExtension::LOSS:
				rez = (entityId == mExtensionData.mEntityId) ? IFifaSeasonalPlayExtension::LOSS : IFifaSeasonalPlayExtension::WIN;
				break;
			default:
				break;
			}

			return rez;
		}

		void SeasonPurchaseOutcomeExtension::getDivCounts(eastl::vector<int>& incrementCounts, eastl::vector<int>& decrementCounts) 
		{
			TRACE_LOG("[getDivCounts] getting div counts\n");

			Blaze::GameReporting::HeadtoHeadExtension::mMutex.Lock();

			for (int i = 0; i < getDefinesHelper()->GetIntSetting(Blaze::GameReporting::DefinesHelper::NUM_DIVISIONS); i++)
			{
				incrementCounts.push_back(Blaze::GameReporting::HeadtoHeadExtension::mDivCountData.mDivCounts[i][Blaze::GameReporting::HeadtoHeadExtension::INCREMENT]);
				decrementCounts.push_back(Blaze::GameReporting::HeadtoHeadExtension::mDivCountData.mDivCounts[i][Blaze::GameReporting::HeadtoHeadExtension::DECREMENT]);

				Blaze::GameReporting::HeadtoHeadExtension::mDivCountData.mDivCounts[i][Blaze::GameReporting::HeadtoHeadExtension::INCREMENT] = 0;
				Blaze::GameReporting::HeadtoHeadExtension::mDivCountData.mDivCounts[i][Blaze::GameReporting::HeadtoHeadExtension::DECREMENT] = 0;
			}

			Blaze::GameReporting::HeadtoHeadExtension::mMutex.Unlock();
		}
		
		/*===============================================================================================
			CoopSeasonPurchaseOutcomeExtension
		===============================================================================================*/
		CoopSeasonPurchaseOutcomeExtension::CoopSeasonPurchaseOutcomeExtension() : SeasonPurchaseOutcomeExtension()
		{
		}
		CoopSeasonPurchaseOutcomeExtension::~CoopSeasonPurchaseOutcomeExtension()
		{
		}

		const char8_t* CoopSeasonPurchaseOutcomeExtension::getDivStatCategory()
		{
			return "SPDivisionalCoopStats";
		}

		const char8_t* CoopSeasonPurchaseOutcomeExtension::getOverallStatCategory()
		{
			return "SPOverallCoopStats";
		}

		const char8_t* CoopSeasonPurchaseOutcomeExtension::getDivCountStatCategory()
		{
			return "SPCoopDivisionCount";
		}

		void CoopSeasonPurchaseOutcomeExtension::incrementTitlesWon(EntityId entityId, GameReporting::StatValueUtil::StatValue ** svSPOTitlesWon, int64_t div)
		{
			if(mExtensionData.mGameResult == IFifaSeasonalPlayExtension::LOSS)
			{
				return;
			}

			// map between division number (vector position) and type of title(vector value) 
			int titleMapId = getDefinesHelper()->GetIntSetting(Blaze::GameReporting::DefinesHelper::TITLE_MAP, div);

			Stats::StatPeriodType statPeriod=getPeriodType();
			// increment global titles won
			svSPOTitlesWon[0]->iAfter[statPeriod]++;

			// increment titles won of each type 
			svSPOTitlesWon[titleMapId]->iAfter[statPeriod]++;

			SetHookEntityWonCompetition(titleMapId, entityId);

			if(div == getDefinesHelper()->GetIntSetting(Blaze::GameReporting::DefinesHelper::NUM_DIVISIONS))
			{
				// top division, this is the league titles
				SetHookEntityWonLeagueTitle(entityId);
			}
		}

		void CoopSeasonPurchaseOutcomeExtension::incrementDivCount(int64_t div) 
		{
			Blaze::GameReporting::CoopSeasonsExtension::mMutex.Lock();

			Blaze::GameReporting::CoopSeasonsExtension::mDivCountData.mDivCounts[div-1][Blaze::GameReporting::CoopSeasonsExtension::INCREMENT]++;

			Blaze::GameReporting::CoopSeasonsExtension::mMutex.Unlock();
		}

		void CoopSeasonPurchaseOutcomeExtension::decrementDivCount(int64_t div) 
		{
			Blaze::GameReporting::CoopSeasonsExtension::mMutex.Lock();
			
			Blaze::GameReporting::CoopSeasonsExtension::mDivCountData.mDivCounts[div-1][Blaze::GameReporting::CoopSeasonsExtension::DECREMENT]++;

			Blaze::GameReporting::CoopSeasonsExtension::mMutex.Unlock();
		}

		void CoopSeasonPurchaseOutcomeExtension::getDivCounts(eastl::vector<int>& incrementCounts, eastl::vector<int>& decrementCounts) 
		{
			TRACE_LOG("[getDivCounts] getting div counts\n");

			Blaze::GameReporting::CoopSeasonsExtension::mMutex.Lock();
			
			for (int i = 0; i < getDefinesHelper()->GetIntSetting(Blaze::GameReporting::DefinesHelper::NUM_DIVISIONS); i++)
			{
				incrementCounts.push_back(Blaze::GameReporting::CoopSeasonsExtension::mDivCountData.mDivCounts[i][Blaze::GameReporting::CoopSeasonsExtension::INCREMENT]);
				decrementCounts.push_back(Blaze::GameReporting::CoopSeasonsExtension::mDivCountData.mDivCounts[i][Blaze::GameReporting::CoopSeasonsExtension::DECREMENT]);

				Blaze::GameReporting::CoopSeasonsExtension::mDivCountData.mDivCounts[i][Blaze::GameReporting::CoopSeasonsExtension::INCREMENT] = 0;
				Blaze::GameReporting::CoopSeasonsExtension::mDivCountData.mDivCounts[i][Blaze::GameReporting::CoopSeasonsExtension::DECREMENT] = 0;
			}

			Blaze::GameReporting::CoopSeasonsExtension::mMutex.Unlock();
		}

		/*===============================================================================================
			ProClubPurchaseOutcomeExtension
		===============================================================================================*/
		ProClubPurchaseOutcomeExtension::ProClubPurchaseOutcomeExtension() : SeasonPurchaseOutcomeExtension()
		{
		}
		ProClubPurchaseOutcomeExtension::~ProClubPurchaseOutcomeExtension()
		{
		}

		const char8_t* ProClubPurchaseOutcomeExtension::getDivStatCategory()
		{
			return "SPDivisionalClubsStats";
		}

		const char8_t* ProClubPurchaseOutcomeExtension::getOverallStatCategory()
		{
			return "SPOverallClubsStats";
		}

		const char8_t* ProClubPurchaseOutcomeExtension::getDivCountStatCategory()
		{
			return "SPClubsDivisionCount";
		}

		void ProClubPurchaseOutcomeExtension::incrementTitlesWon(EntityId entityId, GameReporting::StatValueUtil::StatValue ** svSPOTitlesWon, int64_t div)
		{
			if(mExtensionData.mGameResult == IFifaSeasonalPlayExtension::LOSS)
			{
				return;
			}

			// map between division number (vector position) and type of title(vector value) 
			int titleMapId = getDefinesHelper()->GetIntSetting(Blaze::GameReporting::DefinesHelper::TITLE_MAP, div);

			Stats::StatPeriodType statPeriod=getPeriodType();
			// increment global titles won
			svSPOTitlesWon[0]->iAfter[statPeriod]++;

			// increment titles won of each type 
			svSPOTitlesWon[titleMapId]->iAfter[statPeriod]++;

			SetHookEntityWonCompetition(titleMapId, entityId);

			if(div == getDefinesHelper()->GetIntSetting(Blaze::GameReporting::DefinesHelper::NUM_DIVISIONS))
			{
				// top division, this is the league titles
				SetHookEntityWonLeagueTitle(entityId);
			}
		}

		void ProClubPurchaseOutcomeExtension::incrementDivCount(int64_t div) 
		{
			Blaze::GameReporting::ClubSeasonsExtension::mMutex.Lock();

			Blaze::GameReporting::ClubSeasonsExtension::mDivCountData.mDivCounts[div-1][Blaze::GameReporting::ClubSeasonsExtension::INCREMENT]++;

			Blaze::GameReporting::ClubSeasonsExtension::mMutex.Unlock();
		}

		void ProClubPurchaseOutcomeExtension::decrementDivCount(int64_t div) 
		{
			Blaze::GameReporting::ClubSeasonsExtension::mMutex.Lock();

			Blaze::GameReporting::ClubSeasonsExtension::mDivCountData.mDivCounts[div-1][Blaze::GameReporting::ClubSeasonsExtension::DECREMENT]++;

			Blaze::GameReporting::ClubSeasonsExtension::mMutex.Unlock();
		}

		void ProClubPurchaseOutcomeExtension::getDivCounts(eastl::vector<int>& incrementCounts, eastl::vector<int>& decrementCounts) 
		{
			TRACE_LOG("[getDivCounts] getting div counts\n");

			Blaze::GameReporting::ClubSeasonsExtension::mMutex.Lock();

			for (int i = 0; i < getDefinesHelper()->GetIntSetting(Blaze::GameReporting::DefinesHelper::NUM_DIVISIONS); i++)
			{
				incrementCounts.push_back(Blaze::GameReporting::ClubSeasonsExtension::mDivCountData.mDivCounts[i][Blaze::GameReporting::ClubSeasonsExtension::INCREMENT]);
				decrementCounts.push_back(Blaze::GameReporting::ClubSeasonsExtension::mDivCountData.mDivCounts[i][Blaze::GameReporting::ClubSeasonsExtension::DECREMENT]);

				Blaze::GameReporting::ClubSeasonsExtension::mDivCountData.mDivCounts[i][Blaze::GameReporting::ClubSeasonsExtension::INCREMENT] = 0;
				Blaze::GameReporting::ClubSeasonsExtension::mDivCountData.mDivCounts[i][Blaze::GameReporting::ClubSeasonsExtension::DECREMENT] = 0;
			}

			Blaze::GameReporting::ClubSeasonsExtension::mMutex.Unlock();
		}

	} // EASFC
} // Blaze
