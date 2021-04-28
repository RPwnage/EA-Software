/*************************************************************************************************/
/*!
    \file   fifa_seasonalplaydefines.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFA_FRIENDLIESDEFINES_H
#define BLAZE_CUSTOM_FIFA_FRIENDLIESDEFINES_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace GameReporting
{
	#define STATNAME_GOALS1			"goals1"
	#define STATNAME_GOALS2			"goals2"
	#define STATNAME_GOALS3			"goals3"
	#define STATNAME_GOALS4			"goals4"
	#define STATNAME_GOALS5			"goals5"
	#define STATNAME_GOALS6			"goals6"
	#define STATNAME_GOALS7			"goals7"
	#define STATNAME_GOALS8			"goals8"
	#define STATNAME_GOALS9			"goals9"
	#define STATNAME_GOALS10		"goals10"

	#define STATNAME_GOALSAGAINST1	"goalsAgainst1"
	#define STATNAME_GOALSAGAINST2	"goalsAgainst2"
	#define STATNAME_GOALSAGAINST3	"goalsAgainst3"
	#define STATNAME_GOALSAGAINST4	"goalsAgainst4"
	#define STATNAME_GOALSAGAINST5	"goalsAgainst5"
	#define STATNAME_GOALSAGAINST6	"goalsAgainst6"
	#define STATNAME_GOALSAGAINST7	"goalsAgainst7"
	#define STATNAME_GOALSAGAINST8	"goalsAgainst8"
	#define STATNAME_GOALSAGAINST9	"goalsAgainst9"
	#define STATNAME_GOALSAGAINST10	"goalsAgainst10"

	#define STATNAME_CURR_SEASON		"currSeason"
	
	#define STATNAME_WINS_SEASON					"winsSeason"
	#define STATNAME_LOSSES_SEASON					"lossesSeason"
	#define STATNAME_TIES_SEASON					"tiesSeason"
	#define STATNAME_FOULS_SEASON					"foulsSeason"
	#define STATNAME_FOULS_AGAINST_SEASON			"foulsAgainstSeason"
	#define STATNAME_SHOTS_ON_GOAL_SEASON			"shotsOnGoalSeason"
	#define STATNAME_SHOTS_AGAINST_ON_GOAL_SEASON	"shotsAgainstOnGoalSeason"
	#define STATNAME_POSSESSION_SEASON				"possessionSeason"


	#define STATNAME_TITLE			"title"
	#define STATNAME_TITLES			"titles"

	#define STATNAME_POINTS1			"points1"
	#define STATNAME_POINTS2			"points2"
	#define STATNAME_POINTS3			"points3"
	#define STATNAME_POINTS4			"points4"
	#define STATNAME_POINTS5			"points5"
	#define STATNAME_POINTS6			"points6"
	#define STATNAME_POINTS7			"points7"
	#define STATNAME_POINTS8			"points8"
	#define STATNAME_POINTS9			"points9"
	#define STATNAME_POINTS10			"points10"

	#define STATNAME_TEAM1			"team1"
	#define STATNAME_TEAM2			"team2"
	#define STATNAME_TEAM3			"team3"
	#define STATNAME_TEAM4			"team4"
	#define STATNAME_TEAM5			"team5"
	#define STATNAME_TEAM6			"team6"
	#define STATNAME_TEAM7			"team7"
	#define STATNAME_TEAM8			"team8"
	#define STATNAME_TEAM9			"team9"
	#define STATNAME_TEAM10			"team10"

	#define STATNAME_OPPTEAM1		"oppteam1"
	#define STATNAME_OPPTEAM2		"oppteam2"
	#define STATNAME_OPPTEAM3		"oppteam3"
	#define STATNAME_OPPTEAM4		"oppteam4"
	#define STATNAME_OPPTEAM5		"oppteam5"
	#define STATNAME_OPPTEAM6		"oppteam6"
	#define STATNAME_OPPTEAM7		"oppteam7"
	#define STATNAME_OPPTEAM8		"oppteam8"
	#define STATNAME_OPPTEAM9		"oppteam9"
	#define STATNAME_OPPTEAM10		"oppteam10"

	#define SCOPENAME_FRIEND		"friend"

	#define STATNAME_WINS					"wins"
	#define STATNAME_TIES					"ties"
	#define STATNAME_LOSSES					"losses"
	#define STATNAME_FOULS					"fouls"
	#define STATNAME_FOULS_AGAINST			"foulsAgainst"
	#define STATNAME_SHOTS_ON_GOAL			"shotsOnGoal"
	#define STATNAME_SHOTS_AGAINST_ON_GOAL	"shotsAgainstOnGoal"
	#define STATNAME_POSSESSION				"possession"


	#define STATNAME_GOALS			"goals"
	#define STATNAME_GOALS_AGAINST	"goalsAgainst"

	#define STATNAME_SCORE			"score"
	#define STATNAME_TEAM			"team"
	#define STATNAME_OPPTEAM		"oppteam"
	
	#define STATNAME_LASTMATCH		"lastMatch"

	enum {SIDE_HOME, SIDE_AWAY, SIDE_NUM};
	enum {TITLE_NONE, TITLE_WIN, TITLE_DRAW, TITLE_LOSS, TITLE_HELD};
	enum {POINTS_FOR_WIN = 3, POINTS_FOR_TIE = 1, POINTS_FOR_LOSS = 0};
	
	static const int32_t FP_MAX_NUM_GAMES_IN_SEASON = 5;

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFA_FRIENDLIESDEFINES_H

