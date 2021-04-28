/*************************************************************************************************/
/*!
    \file   fifa_seasonalplaydefines.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/
#include "fifa_seasonalplaydefines.h"
#include "util/utilslaveimpl.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{
	BlazeRpcError ConfigurableDefinesHelper::fetchConfig(const char *configName)
	{
		if (gController != NULL)
		{
			Blaze::Util::UtilSlave *utilComponent = static_cast<Blaze::Util::UtilSlave*>(gController->getComponent(Blaze::Util::UtilSlave::COMPONENT_ID, false));

			if (utilComponent != NULL)
			{
				Blaze::Util::FetchClientConfigRequest request;
				request.setConfigSection(configName);
				Blaze::Util::FetchConfigResponse response;

				mLoadError = utilComponent->fetchClientConfig(request, response);

				if (mLoadError == Blaze::ERR_OK)
				{
					if (!applyConfig(response.getConfig()))
					{
						mLoadError = Blaze::ERR_SYSTEM;
					}
				}
			}
			else
			{
				mLoadError = Blaze::ERR_SYSTEM;
			}

		}
		else
		{
			mLoadError = Blaze::ERR_SYSTEM;
		}

		// If error then game reports will fail
		return mLoadError;
	}

	bool ConfigurableDefinesHelper::applyConfig(const Blaze::Util::FetchConfigResponse::ConfigMap& cfg)
	{
		bool success = true;
		success = success && getConfigData("NUM_DIV", cfg, SP_NUM_DIVISIONS);
		success = success && getConfigData("TOP_DIV", cfg, SP_LAST_DIVISION);
		success = success && getConfigData("BOT_DIV", cfg, SP_FIRST_DIVISION);
		success = success && (SP_NUM_DIVISIONS == (SP_LAST_DIVISION - SP_FIRST_DIVISION + 1)) && (SP_NUM_DIVISIONS <= SP_MAX_NUM_DIVISIONS);

		EA_ASSERT(SP_NUM_DIVISIONS <= SP_MAX_NUM_DIVISIONS);

		success = success && getConfigData("MAX_GAMES", cfg, SP_MAX_NUM_GAMES_IN_SEASON);

		char temp[16] = "";

		for (int i = 0; i < SP_NUM_DIVISIONS; i++)
		{
			EA::StdC::Snprintf(temp, sizeof(temp), "DIV_%d_T", i+1);
			success = success && getConfigData(temp, cfg, SP_TITLE_TRGT_PTS_TABLE[i]);

			EA::StdC::Snprintf(temp, sizeof(temp), "DIV_%d_P", i+1);
			success = success && getConfigData(temp, cfg, SP_PROMO_TRGT_PTS_TABLE[i]);

			EA::StdC::Snprintf(temp, sizeof(temp), "DIV_%d_R", i+1);
			success = success && getConfigData(temp, cfg, SP_RELEG_TRGT_PTS_TABLE[i]);
		}

		success = success && getConfigData("IS_TEAM_LOCKED", cfg, SP_IS_TEAM_LOCKED);
		success = success && getConfigData("PERIOD_TYPE", cfg, SP_PERIOD_TYPE);

		int32_t tempVal = 0;
		success = success && getConfigData("ALLTIME_GAME_CAPPING_ENABLED", cfg, tempVal);
		SP_ALLTIME_GAME_CAPPING_ENABLED = (tempVal > 0);

		tempVal = 0;
		success = success && getConfigData("MONTLY_GAME_CAPPING_ENABLED", cfg, tempVal);
		SP_MONTHLY_GAME_CAPPING_ENABLED = (tempVal > 0);

		tempVal = 0;
		success = success && getConfigData("WEEKLY_GAME_CAPPING_ENABLED", cfg, tempVal);
		SP_WEEKLY_GAME_CAPPING_ENABLED = (tempVal > 0);

		tempVal = 0;
		success = success && getConfigData("DAILY_GAME_CAPPING_ENABLED", cfg, tempVal);
		SP_DAILY_GAME_CAPPING_ENABLED = (tempVal > 0);

		success = success && getConfigData("CAP_MAX_GAMES_ALL_TIME", cfg, SP_MAX_GAMES_ALL_TIME);
		success = success && getConfigData("CAP_MAX_GAMES_PER_MONTH", cfg, SP_MAX_GAMES_PER_MONTH);
		success = success && getConfigData("CAP_MAX_GAMES_PER_WEEK", cfg, SP_MAX_GAMES_PER_WEEK);
		success = success && getConfigData("CAP_MAX_GAMES_PER_DAY", cfg, SP_MAX_GAMES_PER_DAY);
		// Following options are hardcoded and no options are defined in seasonalplay.cfg files - uncomment and make configurable as options are made available
		//int32_t SP_CUP_RANKPTS_1ST;
		//int32_t SP_CUP_RANKPTS_2ND;
		//int32_t SP_CUP_SEMIFINALIST;
		//int32_t SP_CUP_QUARTERFINALIST;
		//int32_t SP_CUP_LAST_ROUND;
		//int32_t SP_DISCONNECT_LOSS_PENALTY;
		//bool	 SP_IS_CUPSTAGE_ON;
		//int32_t SP_POINT_PROJECTION_THRESHOLD;
		//int32_t SP_TITLE_MAP[SP_MAX_NUM_DIVISIONS+1];

		// Ranking points to grant when something happens (title won, promo, relegation, hold) - none are configurable
		//int32_t SP_TITLE_PTS_TABLE[15];
		//int32_t SP_PROMO_PTS_TABLE[15];
		//int32_t SP_RELEG_PTS_TABLE[15];
		//int32_t SP_HOLD_PTS_TABLE[15];

		return success;
	}

	bool ConfigurableDefinesHelper::getConfigData(const char* key, const Blaze::Util::FetchConfigResponse::ConfigMap& cfg, int32_t& val)
	{
		if (NULL != key)
		{
			Blaze::Util::FetchConfigResponse::ConfigMap::const_iterator it = cfg.find(key);
	
			if (it != cfg.end())
			{
				const char * stringValue = it->second.c_str();
	
				if (EA::StdC::Strcmp(stringValue,  "") != 0)
				{
					val = EA::StdC::AtoI32(stringValue);
					return true;
				}
			}
		}
		return false;
	}

}   // namespace GameReporting
}   // namespace Blaze


