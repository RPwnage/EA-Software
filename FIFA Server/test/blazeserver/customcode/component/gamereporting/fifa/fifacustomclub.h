/*************************************************************************************************/
/*!
    \file   fifacustomclub.h
    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_CUSTOM_GAMEREPORTING_FIFACUSTOMCLUB_H
#define BLAZE_CUSTOM_GAMEREPORTING_FIFACUSTOMCLUB_H

namespace Blaze
{
namespace GameReporting
{
namespace FIFA
{
	#define STATNAME_GOALS				"goals"
	#define STATNAME_SHOTSFOR			"shotsFor"
	#define STATNAME_GAME_TIME			"minutes"
    #define STATNAME_GAMES_PLAYED		"gamesPlayed"
	#define STATNAME_CLEANSHEETS		"cleanSheets"

	#define CATEGORYNAME_CLUB_RANKING	"ClubRankStats"
	#define CATEGORYNAME_CLUB_MEMBERS	"ClubMemberStats"


	enum {POS_GK = 1, POS_DEF, POS_MID, POS_FWD, POS_ANY};

	enum CLUB_RECORD_ENUM
	{
		CLUB_RECORD_MOST_GOALS_ALL_TIME = 1,
		CLUB_RECORD_MOST_GOALS_SINGLE_GAME = 2,
		CLUB_RECORD_MOST_SHOTS_ALL_TIME = 3,
		CLUB_RECORD_MOST_SHOTS_SINGLE_GAME = 4,
		CLUB_RECORD_MOST_GAMES = 5,
		CLUB_RECORD_MOST_MINUTES = 6
	};

	enum CLUB_AWARD_ENUM
	{
		CLUB_AWARD_WIN_10_IN_ROW = 1,
		CLUB_AWARD_PLAY_100_GAMES = 2,
		CLUB_AWARD_WIN_GAME = 3,
		CLUB_AWARD_WIN_3_IN_ROW = 4,
		CLUB_AWARD_WIN_5_IN_ROW = 5,
		CLUB_AWARD_SCORE_5 = 6,
		CLUB_AWARD_10_CLEANSHEETS = 7,
		CLUB_AWARD_BEAT_BY_5 = 8,                                
		CLUB_AWARD_WIN_25_GAMES = 9,                            
		CLUB_AWARD_WIN_50_GAMES = 10
	};

    enum CLUB_RIVAL_CHALLENGE_TYPE_ENUM
    {
        CLUBS_RIVAL_TYPE_WIN = 0,
        CLUBS_RIVAL_TYPE_SHUTOUT,
        CLUBS_RIVAL_TYPE_WINBY7,
        CLUBS_RIVAL_TYPE_WINBY5,
        CLUBS_RIVAL_TYPES_COUNT     // The number of Rival Challenge Type to randomly assign to rival pairs
    };
}	// namespace FIFA
}   // namespace GameReporting
}   // namespace Blaze

#endif //BLAZE_CUSTOM_GAMEREPORTING_FIFACUSTOMCLUB_H

