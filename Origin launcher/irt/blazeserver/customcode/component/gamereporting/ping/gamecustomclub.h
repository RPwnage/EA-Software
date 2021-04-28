/*************************************************************************************************/
/*!
    \file   gamecustomclub.h
    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_CUSTOM_GAMEREPORTING_GAMECUSTOMCLUB_H
#define BLAZE_CUSTOM_GAMEREPORTING_GAMECUSTOMCLUB_H

namespace Blaze
{
namespace GameReporting
{
    #define STATNAME_GAMES_PLAYED      "gamesPlayed"
    

    enum CLUB_AWARD_ENUM
    {
        CLUB_AWARD_WIN_DIVISION = 1,
        CLUB_AWARD_PLAY_GAME = 2,
        CLUB_AWARD_WIN_GAME = 3,
        CLUB_AWARD_WIN_3_IN_ROW = 4,
        CLUB_AWARD_WIN_5_IN_ROW = 5,
        CLUB_AWARD_SCORE_5 = 6,
        CLUB_AWARD_SHUTOUT = 7,
        CLUB_AWARD_BEAT_BY_5 = 8,
        CLUB_AWARD_10_PLAYERS = 9,
        CLUB_AWARD_QUICK_SUCCESSION = 10
    };

    enum CLUB_RECORD_ENUM
    {
        CLUB_RECORD_MOST_GOALS_ALL_TIME = 1,
        CLUB_RECORD_MOST_GOALS_SINGLE_GAME = 2,
        CLUB_RECORD_MOST_HITS_ALL_TIME = 3,
        CLUB_RECORD_MOST_HITS_SINGLE_GAME = 4,
        CLUB_RECORD_MOST_GAMES = 5,
        CLUB_RECORD_MOST_MINUTES = 6,
        CLUB_RECORD_CHAMP = 7
    };

    enum CLUB_RIVAL_CHALLENGE_TYPE_ENUM
    {
        CLUBS_RIVAL_TYPE_WIN = 0,
        CLUBS_RIVAL_TYPE_SHUTOUT,
        CLUBS_RIVAL_TYPE_WINBY7,
        CLUBS_RIVAL_TYPE_WINBY5,
        CLUBS_RIVAL_TYPES_COUNT     // The number of Rival Challenge Type to randomly assign to rival pairs
    };
}   // namespace GameReporting
}   // namespace Blaze

#endif //BLAZE_CUSTOM_GAMEREPORTING_GAMECUSTOMCLUB_H

