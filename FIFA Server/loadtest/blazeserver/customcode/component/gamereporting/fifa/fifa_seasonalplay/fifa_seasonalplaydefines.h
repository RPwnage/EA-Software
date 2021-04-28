/*************************************************************************************************/
/*!
    \file   fifa_seasonalplaydefines.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFA_SEASONALPLAYDEFINES_H
#define BLAZE_CUSTOM_FIFA_SEASONALPLAYDEFINES_H

/*** Include files *******************************************************************************/
#include "fifa_seasonalplayextensions.h"
#include "gamereporting/fifa/fifastatsvalueutil.h"
#include "util/utilslaveimpl.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace GameReporting
{
	#define STATNAME_GAMES_PLAYED			"gamesPlayed"
	#define STATNAME_GOALS					"goals"
	#define STATNAME_GOALSAGAINST			"goalsAgainst"
	#define STATNAME_ALLTIMEGOALS			"alltimeGoals"
	#define STATNAME_ALLTIMEGOALSAGAINST	"alltimeGoalsAgainst"
	#define STATNAME_CURR_SEASON			"currSeason"
	#define STATNAME_SEASONS				"seasons"
	#define STATNAME_RANKINGPOINTS			"rankingPoints"
	#define STATNAME_CURDIVISION			"curDivision"
	#define STATNAME_PREVDIVISION			"prevDivision"
	#define STATNAME_MAXDIVISION			"maxDivision"
	#define STATNAME_BESTDIVISION			"bestDivision"
	#define STATNAME_BESTPOINTS				"bestPoints"
	#define STATNAME_CUPWINS				"cupWins"
	#define STATNAME_CUPLOSSES				"cupLosses"
	#define STATNAME_CUPROUND				"cupRound"
	#define STATNAME_CUPSWON1				"cupsWon1"
	#define STATNAME_CUPSWON2				"cupsWon2"
	#define STATNAME_CUPSWON3				"cupsWon3"
	#define STATNAME_CUPSWON4				"cupsWon4"
	#define STATNAME_TITLESWON				"titlesWon"
	#define STATNAME_LEAGUESWON				"leaguesWon"
	#define STATNAME_DIVSWON1				"divsWon1"
	#define STATNAME_DIVSWON2				"divsWon2"
	#define STATNAME_DIVSWON3				"divsWon3"
	#define STATNAME_DIVSWON4				"divsWon4"
	#define STATNAME_PROMOTIONS				"promotions"
	#define STATNAME_HOLDS					"holds"
	#define STATNAME_RELEGATIONS			"relegations"
	#define STATNAME_LASTMATCH				"lastMatch"
	#define STATNAME_LASTMATCH0				"lastMatch0"
	#define STATNAME_LASTMATCH1				"lastMatch1"
	#define STATNAME_LASTMATCH2				"lastMatch2"
	#define STATNAME_LASTMATCH3				"lastMatch3"
	#define STATNAME_LASTMATCH4				"lastMatch4"
	#define STATNAME_LASTMATCH5				"lastMatch5"
	#define STATNAME_LASTMATCH6				"lastMatch6"
	#define STATNAME_LASTMATCH7				"lastMatch7"
	#define STATNAME_LASTMATCH8				"lastMatch8"
	#define STATNAME_LASTMATCH9				"lastMatch9"	
	#define STATNAME_LASTOPPONENT			"lastOpponent"
	#define STATNAME_LASTOPPONENT0			"lastOpponent0"
	#define STATNAME_LASTOPPONENT1			"lastOpponent1"
	#define STATNAME_LASTOPPONENT2			"lastOpponent2"
	#define STATNAME_LASTOPPONENT3			"lastOpponent3"
	#define STATNAME_LASTOPPONENT4			"lastOpponent4"
	#define STATNAME_LASTOPPONENT5			"lastOpponent5"
	#define STATNAME_LASTOPPONENT6			"lastOpponent6"
	#define STATNAME_LASTOPPONENT7			"lastOpponent7"
	#define STATNAME_LASTOPPONENT8			"lastOpponent8"
	#define STATNAME_LASTOPPONENT9			"lastOpponent9"
	#define STATNAME_SEASONMOV				"curSeasonMov"
	#define STATNAME_SPPOINTS				"points"
	#define STATNAME_SEASONWINS				"seasonWins"
	#define STATNAME_SEASONLOSSES			"seasonLosses"
	#define STATNAME_SEASONTIES				"seasonTies"
	#define STATNAME_PREVSEASONWINS			"prevSeasonWins"
	#define STATNAME_PREVSEASONLOSSES		"prevSeasonLosses"
	#define STATNAME_PREVSEASONTIES			"prevSeasonTies"
	#define STATNAME_PREVPOINTS				"prevPoints"

	#define STATNAME_PROJPTS				"projectedPts"
	#define STATNAME_PREVPROJPTS			"prevProjectedPts"

	#define STATNAME_EXTRAMATCHES			"extraMatches"
	#define STATNAME_WINREDEEMED			"winRedeemed"
	#define STATNAME_DRAWREDEEMED			"drawRedeemed"
	#define STATNAME_MATCHREDEEMED			"matchRedeemed"
	#define STATNAME_SPRANKINGPOINTS		"SPRankingPoints"
    #define STATNAME_TEAMID          		"team_id"

	#define STATNAME_TOTALCUPSWON      		"cupsWon"
		
	static const int32_t NUM_LAST_MATCHES = 10;					// same for all types
	static const int32_t SP_MAX_NUM_DIVISIONS = 10;				// need this because there are places that use these to declare arrays

	enum SeasonMovement
	{
		SSN_END_LGETITLE_EARLY = -10,
		SSN_END_LGETITLE = -9,
		SSN_END_PROMOTED_EARLY = -8,
		CUP_END_4TH_PLACE = -7,
		CUP_END_2ND_PLACE = -5,
		CUP_END_1ST_PLACE = -4,
		CUP_END_NOPLACE = -3,
		MID_SEASON_NXT_GAME = -2,
		SSN_END_PROMOTED = -1,
		START_CUPSTAGE_NXT_GAME = 0,
		SSN_END_RELEGATED = 1,
		START_CUPSTAGE_NXT_GAME_EARLY = 2,
		SSN_END_RELEGATED_EARLY = 6,
		SSN_END_HELD = 7,
		SSN_END_HELD_EARLY = 8
	};

	class DefinesHelper
	{
		public:
			enum DivisionConfigParameters
			{
				MAX_NUM_GAMES_IN_SEASON,
				FIRST_DIVISION,
				LAST_DIVISION,
				NUM_DIVISIONS,
				CUP_RANKPTS_1ST,
				CUP_RANKPTS_2ND,
				CUP_SEMIFINALIST,
				CUP_QUARTERFINALIST,
				CUP_LAST_ROUND,
				TITLE_PTS_TABLE,
				PROMO_PTS_TABLE,
				RELEG_PTS_TABLE,
				HOLD_PTS_TABLE,
				TITLE_TRGT_PTS_TABLE,
				PROMO_TRGT_PTS_TABLE,
				RELEG_TRGT_PTS_TABLE,
				DISCONNECT_LOSS_PENALTY,
				IS_CUPSTAGE_ON,
				POINT_PROJECTION_THRESHOLD,
				E_NUM_LAST_MATCHES,
				TITLE_MAP,
				PERIOD_TYPE,
				IS_TEAM_LOCKED,
				ALLTIME_GAME_CAPPING_ENABLED,
				MONTHLY_GAME_CAPPING_ENABLED,
				WEEKLY_GAME_CAPPING_ENABLED,
				DAILY_GAME_CAPPING_ENABLED,
				MAX_GAMES_ALL_TIME,
				MAX_GAMES_PER_MONTH,
				MAX_GAMES_PER_WEEK,
				MAX_GAMES_PER_DAY,
				MAX_PARAMETERS
			};

			DefinesHelper()		
			{
				// TEMP LOADER UNTIL WE GET THIS LOADING FROM CONFIG FILE
				SP_MAX_NUM_GAMES_IN_SEASON = 10;
				SP_FIRST_DIVISION = 1;
				SP_LAST_DIVISION = 10;
				SP_NUM_DIVISIONS = SP_LAST_DIVISION - SP_FIRST_DIVISION + 1;
				EA_ASSERT(SP_NUM_DIVISIONS <= SP_MAX_NUM_DIVISIONS);
				SP_CUP_RANKPTS_1ST = 125;
				SP_CUP_RANKPTS_2ND = 75;
				SP_CUP_SEMIFINALIST = 50;
				SP_CUP_QUARTERFINALIST = 20;
				SP_CUP_LAST_ROUND = 4;
				SP_DISCONNECT_LOSS_PENALTY = 5;
				SP_IS_CUPSTAGE_ON = false;
				SP_POINT_PROJECTION_THRESHOLD = 3;

				SP_TITLE_PTS_TABLE[0] = 6;
				SP_TITLE_PTS_TABLE[1] = 12;
				SP_TITLE_PTS_TABLE[2] = 18;
				SP_TITLE_PTS_TABLE[3] = 24;
				SP_TITLE_PTS_TABLE[4] = 30;
				SP_TITLE_PTS_TABLE[5] = 36;
				SP_TITLE_PTS_TABLE[6] = 45;
				SP_TITLE_PTS_TABLE[7] = 54;
				SP_TITLE_PTS_TABLE[8] = 63;
				SP_TITLE_PTS_TABLE[9] = 72;
				SP_TITLE_PTS_TABLE[10] = 84;
				SP_TITLE_PTS_TABLE[11] = 96;
				SP_TITLE_PTS_TABLE[12] = 111;
				SP_TITLE_PTS_TABLE[13] = 129;
				SP_TITLE_PTS_TABLE[14] = 150;

				SP_PROMO_PTS_TABLE[0] = 20;
				SP_PROMO_PTS_TABLE[1] = 40;
				SP_PROMO_PTS_TABLE[2] = 60;
				SP_PROMO_PTS_TABLE[3] = 80;
				SP_PROMO_PTS_TABLE[4] = 100;
				SP_PROMO_PTS_TABLE[5] = 120;
				SP_PROMO_PTS_TABLE[6] = 150;
				SP_PROMO_PTS_TABLE[7] = 180;
				SP_PROMO_PTS_TABLE[8] = 210;
				SP_PROMO_PTS_TABLE[9] = 240;
				SP_PROMO_PTS_TABLE[10] = 280;
				SP_PROMO_PTS_TABLE[11] = 320;
				SP_PROMO_PTS_TABLE[12] = 370;
				SP_PROMO_PTS_TABLE[13] = 430;
				SP_PROMO_PTS_TABLE[14] = 500;

				SP_RELEG_PTS_TABLE[0] = 0;
				SP_RELEG_PTS_TABLE[1] = 14;
				SP_RELEG_PTS_TABLE[2] = 28;
				SP_RELEG_PTS_TABLE[3] = 42;
				SP_RELEG_PTS_TABLE[4] = 56;
				SP_RELEG_PTS_TABLE[5] = 70;
				SP_RELEG_PTS_TABLE[6] = 84;
				SP_RELEG_PTS_TABLE[7] = 105;
				SP_RELEG_PTS_TABLE[8] = 126;
				SP_RELEG_PTS_TABLE[9] = 147;
				SP_RELEG_PTS_TABLE[10] = 168;
				SP_RELEG_PTS_TABLE[11] = 196;
				SP_RELEG_PTS_TABLE[12] = 224;
				SP_RELEG_PTS_TABLE[13] = 259;
				SP_RELEG_PTS_TABLE[14] = 301;

				SP_HOLD_PTS_TABLE[0] = 12;
				SP_HOLD_PTS_TABLE[1] = 24;
				SP_HOLD_PTS_TABLE[2] = 36;
				SP_HOLD_PTS_TABLE[3] = 48;
				SP_HOLD_PTS_TABLE[4] = 60;
				SP_HOLD_PTS_TABLE[5] = 72;
				SP_HOLD_PTS_TABLE[6] = 90;
				SP_HOLD_PTS_TABLE[7] = 108;
				SP_HOLD_PTS_TABLE[8] = 126;
				SP_HOLD_PTS_TABLE[9] = 144;
				SP_HOLD_PTS_TABLE[10] = 168;
				SP_HOLD_PTS_TABLE[11] = 192;
				SP_HOLD_PTS_TABLE[12] = 222;
				SP_HOLD_PTS_TABLE[13] = 258;
				SP_HOLD_PTS_TABLE[14] = 300;
				
				SP_TITLE_TRGT_PTS_TABLE[0] = 12;
				SP_TITLE_TRGT_PTS_TABLE[1] = 13;
				SP_TITLE_TRGT_PTS_TABLE[2] = 15;
				SP_TITLE_TRGT_PTS_TABLE[3] = 17;
				SP_TITLE_TRGT_PTS_TABLE[4] = 19;
				SP_TITLE_TRGT_PTS_TABLE[5] = 19;
				SP_TITLE_TRGT_PTS_TABLE[6] = 19;
				SP_TITLE_TRGT_PTS_TABLE[7] = 21;
				SP_TITLE_TRGT_PTS_TABLE[8] = 21;
				SP_TITLE_TRGT_PTS_TABLE[9] = 23;
				SP_TITLE_TRGT_PTS_TABLE[10] = 23;
				SP_TITLE_TRGT_PTS_TABLE[11] = 23;
				SP_TITLE_TRGT_PTS_TABLE[12] = 23;
				SP_TITLE_TRGT_PTS_TABLE[13] = 23;
				SP_TITLE_TRGT_PTS_TABLE[14] = 23;

				SP_PROMO_TRGT_PTS_TABLE[0] = 9;
				SP_PROMO_TRGT_PTS_TABLE[1] = 10;
				SP_PROMO_TRGT_PTS_TABLE[2] = 12;
				SP_PROMO_TRGT_PTS_TABLE[3] = 14;
				SP_PROMO_TRGT_PTS_TABLE[4] = 16;
				SP_PROMO_TRGT_PTS_TABLE[5] = 16;
				SP_PROMO_TRGT_PTS_TABLE[6] = 16;
				SP_PROMO_TRGT_PTS_TABLE[7] = 18;
				SP_PROMO_TRGT_PTS_TABLE[8] = 18;
				SP_PROMO_TRGT_PTS_TABLE[9] = 23;
				SP_PROMO_TRGT_PTS_TABLE[10] = -1;
				SP_PROMO_TRGT_PTS_TABLE[11] = -1;
				SP_PROMO_TRGT_PTS_TABLE[12] = -1;
				SP_PROMO_TRGT_PTS_TABLE[13] = -1;
				SP_PROMO_TRGT_PTS_TABLE[14] = -1;

				SP_RELEG_TRGT_PTS_TABLE[0] = -1;
				SP_RELEG_TRGT_PTS_TABLE[1] = 6;
				SP_RELEG_TRGT_PTS_TABLE[2] = 8;
				SP_RELEG_TRGT_PTS_TABLE[3] = 8;
				SP_RELEG_TRGT_PTS_TABLE[4] = 10;
				SP_RELEG_TRGT_PTS_TABLE[5] = 10;
				SP_RELEG_TRGT_PTS_TABLE[6] = 10;
				SP_RELEG_TRGT_PTS_TABLE[7] = 12;
				SP_RELEG_TRGT_PTS_TABLE[8] = 12;
				SP_RELEG_TRGT_PTS_TABLE[9] = 14;
				SP_RELEG_TRGT_PTS_TABLE[10] = 14;
				SP_RELEG_TRGT_PTS_TABLE[11] = 14;
				SP_RELEG_TRGT_PTS_TABLE[12] = 14;
				SP_RELEG_TRGT_PTS_TABLE[13] = 14;
				SP_RELEG_TRGT_PTS_TABLE[14] = 14;

				SP_TITLE_MAP[0] = 0;
				SP_TITLE_MAP[1] = 4;
				SP_TITLE_MAP[2] = 4;
				SP_TITLE_MAP[3] = 4;
				SP_TITLE_MAP[4] = 3;
				SP_TITLE_MAP[5] = 3;
				SP_TITLE_MAP[6] = 3;
				SP_TITLE_MAP[7] = 3;
				SP_TITLE_MAP[8] = 2;
				SP_TITLE_MAP[9] = 2;
				SP_TITLE_MAP[10] = 1;

				SP_PERIOD_TYPE = Stats::STAT_PERIOD_ALL_TIME;
				
				SP_IS_TEAM_LOCKED = 0;
				SP_ALLTIME_GAME_CAPPING_ENABLED = false;
				SP_MONTHLY_GAME_CAPPING_ENABLED = false;
				SP_WEEKLY_GAME_CAPPING_ENABLED = false;
				SP_DAILY_GAME_CAPPING_ENABLED = false;
				SP_MAX_GAMES_ALL_TIME = 0;
				SP_MAX_GAMES_PER_MONTH = 0;
				SP_MAX_GAMES_PER_WEEK = 0;
				SP_MAX_GAMES_PER_DAY = 0;
			}

			virtual ~DefinesHelper() {}
		
		public:
						
			int32_t GetIntSetting(DivisionConfigParameters configParam) const
			{
				switch(configParam)
				{
				case MAX_NUM_GAMES_IN_SEASON:
					return SP_MAX_NUM_GAMES_IN_SEASON;
					break;
				case FIRST_DIVISION:
					return SP_FIRST_DIVISION;
					break;
				case LAST_DIVISION:
					return SP_LAST_DIVISION;
					break;
				case NUM_DIVISIONS:
					return SP_NUM_DIVISIONS;
					break;
				case CUP_RANKPTS_1ST:
					return SP_CUP_RANKPTS_1ST;
					break;
				case CUP_RANKPTS_2ND:
					return SP_CUP_RANKPTS_2ND;
					break;
				case CUP_SEMIFINALIST:
					return SP_CUP_SEMIFINALIST;
					break;
				case CUP_QUARTERFINALIST:
					return SP_CUP_QUARTERFINALIST;
					break;
				case CUP_LAST_ROUND:
					return SP_CUP_LAST_ROUND;
					break;
				case DISCONNECT_LOSS_PENALTY:
					return SP_DISCONNECT_LOSS_PENALTY;
					break;
				case POINT_PROJECTION_THRESHOLD:
					return SP_POINT_PROJECTION_THRESHOLD;
					break;
				case PERIOD_TYPE:
					return SP_PERIOD_TYPE;
					break;
				case IS_TEAM_LOCKED:
					return SP_IS_TEAM_LOCKED;
					break;
				case MAX_GAMES_ALL_TIME:
					return SP_MAX_GAMES_ALL_TIME;
					break;
				case MAX_GAMES_PER_MONTH:
					return SP_MAX_GAMES_PER_MONTH;
					break;
				case MAX_GAMES_PER_WEEK:
					return SP_MAX_GAMES_PER_WEEK;
					break;
				case MAX_GAMES_PER_DAY:
					return SP_MAX_GAMES_PER_DAY;
					break;
				default:
					return 0;
				}
			}

			int32_t GetIntSetting(DivisionConfigParameters configParam, int64_t index) const
			{
				switch(configParam)
				{
				case TITLE_PTS_TABLE:      return SP_TITLE_PTS_TABLE[index];      break;
				case PROMO_PTS_TABLE:      return SP_PROMO_PTS_TABLE[index];      break;
				case RELEG_PTS_TABLE:      return SP_RELEG_PTS_TABLE[index];      break;
				case HOLD_PTS_TABLE:       return SP_HOLD_PTS_TABLE[index];       break;
				case TITLE_TRGT_PTS_TABLE: return SP_TITLE_TRGT_PTS_TABLE[index]; break;
				case PROMO_TRGT_PTS_TABLE: return SP_PROMO_TRGT_PTS_TABLE[index]; break;
				case RELEG_TRGT_PTS_TABLE: return SP_RELEG_TRGT_PTS_TABLE[index]; break;
				case TITLE_MAP:            return SP_TITLE_MAP[index];            break;
				default:                   return 0;                              break;
				}
			}

			bool GetBoolSetting(DivisionConfigParameters configParam) const
			{
				switch(configParam)
				{
				case IS_CUPSTAGE_ON:
					return SP_IS_CUPSTAGE_ON;
					break;
				case ALLTIME_GAME_CAPPING_ENABLED:
					return SP_ALLTIME_GAME_CAPPING_ENABLED;
					break;
				case MONTHLY_GAME_CAPPING_ENABLED:
					return SP_MONTHLY_GAME_CAPPING_ENABLED;
					break;
				case WEEKLY_GAME_CAPPING_ENABLED:
					return SP_WEEKLY_GAME_CAPPING_ENABLED;
					break;
				case DAILY_GAME_CAPPING_ENABLED:
					return SP_DAILY_GAME_CAPPING_ENABLED;
					break;
				default:
					return false;
				}
			}

			virtual Stats::StatPeriodType GetPeriodType() const
			{
				return static_cast<Stats::StatPeriodType>(SP_PERIOD_TYPE);
			}

			virtual BlazeRpcError configLoadStatus() const { return ERR_OK; }
		protected:
			int32_t SP_MAX_NUM_GAMES_IN_SEASON;
			int32_t SP_FIRST_DIVISION;
			int32_t SP_LAST_DIVISION;
			int32_t SP_NUM_DIVISIONS;
			int32_t SP_CUP_RANKPTS_1ST;
			int32_t SP_CUP_RANKPTS_2ND;
			int32_t SP_CUP_SEMIFINALIST;
			int32_t SP_CUP_QUARTERFINALIST;
			int32_t SP_CUP_LAST_ROUND;
			int32_t SP_DISCONNECT_LOSS_PENALTY;
			bool	SP_IS_CUPSTAGE_ON;
			int32_t SP_POINT_PROJECTION_THRESHOLD;

			int32_t SP_TITLE_PTS_TABLE[15];
			int32_t SP_PROMO_PTS_TABLE[15];
			int32_t SP_RELEG_PTS_TABLE[15];
			int32_t SP_HOLD_PTS_TABLE[15];
			int32_t SP_TITLE_TRGT_PTS_TABLE[15];
			int32_t SP_PROMO_TRGT_PTS_TABLE[15];
			int32_t SP_RELEG_TRGT_PTS_TABLE[15];
			int32_t SP_TITLE_MAP[SP_MAX_NUM_DIVISIONS+1];

			//  needed for live competitions
			int32_t	SP_PERIOD_TYPE;
			int32_t	SP_IS_TEAM_LOCKED;	
			bool SP_ALLTIME_GAME_CAPPING_ENABLED;
			bool SP_MONTHLY_GAME_CAPPING_ENABLED;
			bool SP_WEEKLY_GAME_CAPPING_ENABLED;
			bool SP_DAILY_GAME_CAPPING_ENABLED;
			int32_t SP_MAX_GAMES_ALL_TIME;
			int32_t SP_MAX_GAMES_PER_MONTH;
			int32_t SP_MAX_GAMES_PER_WEEK;
			int32_t SP_MAX_GAMES_PER_DAY;

	}; // Defines Helper

	class CoopSeasonsDefinesHelper : public DefinesHelper
	{

	public:
		CoopSeasonsDefinesHelper() 
		{
			DefinesHelper::SP_MAX_NUM_GAMES_IN_SEASON = 10;
			DefinesHelper::SP_FIRST_DIVISION = 1;
			DefinesHelper::SP_LAST_DIVISION = 5;
			DefinesHelper::SP_NUM_DIVISIONS = SP_LAST_DIVISION - SP_FIRST_DIVISION + 1;
			EA_ASSERT(SP_NUM_DIVISIONS <= SP_MAX_NUM_DIVISIONS);

			DefinesHelper::SP_CUP_RANKPTS_1ST = 125;				// dont need
			DefinesHelper::SP_CUP_RANKPTS_2ND = 75;					// dont need
			DefinesHelper::SP_CUP_SEMIFINALIST = 50;				// dont need
			DefinesHelper::SP_CUP_QUARTERFINALIST = 20;				// dont need
			DefinesHelper::SP_CUP_LAST_ROUND = 4;					// dont need
			DefinesHelper::SP_DISCONNECT_LOSS_PENALTY = 5;
			DefinesHelper::SP_IS_CUPSTAGE_ON = false;
			DefinesHelper::SP_POINT_PROJECTION_THRESHOLD = 3;


			// TEMP LOADER UNTIL WE GET THIS LOADING FROM CONFIG FILE
			DefinesHelper::SP_TITLE_PTS_TABLE[0] = 6;
			DefinesHelper::SP_TITLE_PTS_TABLE[1] = 12;
			DefinesHelper::SP_TITLE_PTS_TABLE[2] = 18;
			DefinesHelper::SP_TITLE_PTS_TABLE[3] = 24;
			DefinesHelper::SP_TITLE_PTS_TABLE[4] = 30;

			DefinesHelper::SP_PROMO_PTS_TABLE[0] = 20;
			DefinesHelper::SP_PROMO_PTS_TABLE[1] = 40;
			DefinesHelper::SP_PROMO_PTS_TABLE[2] = 60;
			DefinesHelper::SP_PROMO_PTS_TABLE[3] = 80;
			DefinesHelper::SP_PROMO_PTS_TABLE[4] = 100;

			DefinesHelper::SP_RELEG_PTS_TABLE[0] = 0;
			DefinesHelper::SP_RELEG_PTS_TABLE[1] = 14;
			DefinesHelper::SP_RELEG_PTS_TABLE[2] = 28;
			DefinesHelper::SP_RELEG_PTS_TABLE[3] = 42;
			DefinesHelper::SP_RELEG_PTS_TABLE[4] = 56;

			DefinesHelper::SP_HOLD_PTS_TABLE[0] = 12;
			DefinesHelper::SP_HOLD_PTS_TABLE[1] = 24;
			DefinesHelper::SP_HOLD_PTS_TABLE[2] = 36;
			DefinesHelper::SP_HOLD_PTS_TABLE[3] = 48;
			DefinesHelper::SP_HOLD_PTS_TABLE[4] = 60;

			DefinesHelper::SP_TITLE_TRGT_PTS_TABLE[0] = 13;
			DefinesHelper::SP_TITLE_TRGT_PTS_TABLE[1] = 16;
			DefinesHelper::SP_TITLE_TRGT_PTS_TABLE[2] = 19;
			DefinesHelper::SP_TITLE_TRGT_PTS_TABLE[3] = 21;
			DefinesHelper::SP_TITLE_TRGT_PTS_TABLE[4] = 23;

			DefinesHelper::SP_PROMO_TRGT_PTS_TABLE[0] = 10;
			DefinesHelper::SP_PROMO_TRGT_PTS_TABLE[1] = 13;
			DefinesHelper::SP_PROMO_TRGT_PTS_TABLE[2] = 16;
			DefinesHelper::SP_PROMO_TRGT_PTS_TABLE[3] = 18;
			DefinesHelper::SP_PROMO_TRGT_PTS_TABLE[4] = 23;

			DefinesHelper::SP_RELEG_TRGT_PTS_TABLE[0] = -1;
			DefinesHelper::SP_RELEG_TRGT_PTS_TABLE[1] = 8;
			DefinesHelper::SP_RELEG_TRGT_PTS_TABLE[2] = 10;
			DefinesHelper::SP_RELEG_TRGT_PTS_TABLE[3] = 12;
			DefinesHelper::SP_RELEG_TRGT_PTS_TABLE[4] = 14;

			SP_TITLE_MAP[0] = 0;
			SP_TITLE_MAP[1] = 4;
			SP_TITLE_MAP[2] = 4;
			SP_TITLE_MAP[3] = 3;
			SP_TITLE_MAP[4] = 2;
			SP_TITLE_MAP[5] = 1;
			SP_TITLE_MAP[6] = 0;
			SP_TITLE_MAP[7] = 0;
			SP_TITLE_MAP[8] = 0;
			SP_TITLE_MAP[9] = 0;
			SP_TITLE_MAP[10] = 0;

			SP_PERIOD_TYPE = Stats::STAT_PERIOD_ALL_TIME;
			
			SP_IS_TEAM_LOCKED = 0;
		}

		virtual ~CoopSeasonsDefinesHelper() {}

		virtual Stats::StatPeriodType GetPeriodType() const
		{
			return static_cast<Stats::StatPeriodType>(SP_PERIOD_TYPE);
		}

	};	// Coop Seasons Defines Helper

	class ConfigurableDefinesHelper : public DefinesHelper
	{
	public:
		ConfigurableDefinesHelper() : mLoadError(Blaze::ERR_OK)
		{
		}
		~ConfigurableDefinesHelper() {}

		BlazeRpcError fetchConfig(const char *configName);

		virtual BlazeRpcError configLoadStatus() const { return mLoadError; }
	private:
		bool applyConfig(const Blaze::Util::FetchConfigResponse::ConfigMap& cfg);
		bool getConfigData(const char* key, const Blaze::Util::FetchConfigResponse::ConfigMap& cfg, int32_t& val);

		BlazeRpcError mLoadError;
	};	// Configurable seasons defines helper - to be used with live competitions

	class LiveCompDefinesHelper : public ConfigurableDefinesHelper
	{

	public:
		LiveCompDefinesHelper() 
		{
			DefinesHelper::SP_MAX_NUM_GAMES_IN_SEASON = 10;
			DefinesHelper::SP_FIRST_DIVISION = 1;
			DefinesHelper::SP_LAST_DIVISION = 5;
			DefinesHelper::SP_NUM_DIVISIONS = SP_LAST_DIVISION - SP_FIRST_DIVISION + 1;
			EA_ASSERT(SP_NUM_DIVISIONS <= SP_MAX_NUM_DIVISIONS);

			DefinesHelper::SP_CUP_RANKPTS_1ST = 125;				// dont need
			DefinesHelper::SP_CUP_RANKPTS_2ND = 75;					// dont need
			DefinesHelper::SP_CUP_SEMIFINALIST = 50;				// dont need
			DefinesHelper::SP_CUP_QUARTERFINALIST = 20;				// dont need
			DefinesHelper::SP_CUP_LAST_ROUND = 4;					// dont need
			DefinesHelper::SP_DISCONNECT_LOSS_PENALTY = 5;
			DefinesHelper::SP_IS_CUPSTAGE_ON = false;
			DefinesHelper::SP_POINT_PROJECTION_THRESHOLD = 3;


			DefinesHelper::SP_TITLE_PTS_TABLE[0] = 12;
			DefinesHelper::SP_TITLE_PTS_TABLE[1] = 24;
			DefinesHelper::SP_TITLE_PTS_TABLE[2] = 36;
			DefinesHelper::SP_TITLE_PTS_TABLE[3] = 54;
			DefinesHelper::SP_TITLE_PTS_TABLE[4] = 72;

			DefinesHelper::SP_PROMO_PTS_TABLE[0] = 40;
			DefinesHelper::SP_PROMO_PTS_TABLE[1] = 80;
			DefinesHelper::SP_PROMO_PTS_TABLE[2] = 120;
			DefinesHelper::SP_PROMO_PTS_TABLE[3] = 180;
			DefinesHelper::SP_PROMO_PTS_TABLE[4] = 240;

			DefinesHelper::SP_RELEG_PTS_TABLE[0] = 0;
			DefinesHelper::SP_RELEG_PTS_TABLE[1] = 28;
			DefinesHelper::SP_RELEG_PTS_TABLE[2] = 56;
			DefinesHelper::SP_RELEG_PTS_TABLE[3] = 84;
			DefinesHelper::SP_RELEG_PTS_TABLE[4] = 126;

			DefinesHelper::SP_HOLD_PTS_TABLE[0] = 24;
			DefinesHelper::SP_HOLD_PTS_TABLE[1] = 48;
			DefinesHelper::SP_HOLD_PTS_TABLE[2] = 72;
			DefinesHelper::SP_HOLD_PTS_TABLE[3] = 108;
			DefinesHelper::SP_HOLD_PTS_TABLE[4] = 144;

			DefinesHelper::SP_TITLE_TRGT_PTS_TABLE[0] = 13;
			DefinesHelper::SP_TITLE_TRGT_PTS_TABLE[1] = 16;
			DefinesHelper::SP_TITLE_TRGT_PTS_TABLE[2] = 19;
			DefinesHelper::SP_TITLE_TRGT_PTS_TABLE[3] = 21;
			DefinesHelper::SP_TITLE_TRGT_PTS_TABLE[4] = 23;

			DefinesHelper::SP_PROMO_TRGT_PTS_TABLE[0] = 10;
			DefinesHelper::SP_PROMO_TRGT_PTS_TABLE[1] = 13;
			DefinesHelper::SP_PROMO_TRGT_PTS_TABLE[2] = 16;
			DefinesHelper::SP_PROMO_TRGT_PTS_TABLE[3] = 18;
			DefinesHelper::SP_PROMO_TRGT_PTS_TABLE[4] = 23;

			DefinesHelper::SP_RELEG_TRGT_PTS_TABLE[0] = -1;
			DefinesHelper::SP_RELEG_TRGT_PTS_TABLE[1] = 8;
			DefinesHelper::SP_RELEG_TRGT_PTS_TABLE[2] = 10;
			DefinesHelper::SP_RELEG_TRGT_PTS_TABLE[3] = 12;
			DefinesHelper::SP_RELEG_TRGT_PTS_TABLE[4] = 14;

			SP_TITLE_MAP[0] = 0;
			SP_TITLE_MAP[1] = 4;
			SP_TITLE_MAP[2] = 4;
			SP_TITLE_MAP[3] = 3;
			SP_TITLE_MAP[4] = 2;
			SP_TITLE_MAP[5] = 1;
			SP_TITLE_MAP[6] = 0;
			SP_TITLE_MAP[7] = 0;
			SP_TITLE_MAP[8] = 0;
			SP_TITLE_MAP[9] = 0;
			SP_TITLE_MAP[10] = 0;
			
			SP_PERIOD_TYPE = Stats::STAT_PERIOD_MONTHLY;

			SP_IS_TEAM_LOCKED = 0;
		}

		virtual ~LiveCompDefinesHelper() {}

		virtual Stats::StatPeriodType GetPeriodType() const
		{
			return static_cast<Stats::StatPeriodType>(SP_PERIOD_TYPE);
		}

	};	// Live Competitions Defines Helper


}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFA_SEASONALPLAYDEFINES_H

