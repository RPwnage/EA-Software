/*************************************************************************************************/
/*!
    \file   gamecommon.h
    \attention
        (c) Electronic Arts Inc. 2007-2008
*/
/*************************************************************************************************/
#ifndef BLAZE_GAMEREPORTING_GAMECOMMON_H
#define BLAZE_GAMEREPORTING_GAMECOMMON_H

namespace Blaze
{
namespace GameReporting
{

#define WLT_WIN          0x0001   // Bit vector: user won the game
#define WLT_LOSS         0x0002   // Bit vector: user lost the game
#define WLT_TIE          0x0004   // bit vector: user tied the game
#define WLT_DISC         0x0008   // bit vector: user disconnected during game
#define WLT_CHEAT        0x0010   // bit vector: user cheated during game
#define WLT_CONCEDE      0x0020   // Bit vector: user has conceded the game
#define WLT_NULLIFY      0x0040   // Bit vector: user nullified game (could be friendly quit)
#define WLT_QUITCNT      0x0080   // Bit vector: quit and count
#define WLT_PRORATE_PTS  0x0100   // Bit vector: prorate ranking points
#define WLT_NO_PTS       0x0200   // Bit vector: don't count ranking points
#define WLT_NO_STATS     0x0400   // Bit vector: don't count stats
#define WLT_DESYNC       0x0800   // Bit vector: (server) desync game
#define WLT_CONTESTED    0x1000   // Bit vector: (server) contested score game
#define WLT_SHORT        0x2000   // Bit vector: (server) int16_t game
#define WLT_OPPOQCTAG    0x4000   // Bit vector: (server) opponent quit and count tag

#define WLT_IGNORED_MASK (WLT_NULLIFY | WLT_DESYNC | WLT_CONTESTED | WLT_SHORT)

#define QC_Q3_PERCENT_COMP 75
#define QC_Q1_PERCENT_COMP 20

#define DISC_SCORE_MIN_DIFF 3 // minimum difference of scores for the winner and the loser in a disconnect situation
#define INVALID_HIGH_SCORE 99999999 // invalid high score, used for calculations

enum StatGameResultE
{
    GAME_RESULT_COMPLETE_REGULATION = 0, // Game completed normally in regulation time
    GAME_RESULT_COMPLETE_OVERTIME,       // Game completed normally in overtime time
    GAME_RESULT_DNF_QUIT,                // Game was terminated by opponent quits, no stats counted. DNF/loss counted.
    GAME_RESULT_FRIENDLY_QUIT,           // Game was terminated by consensual quit, no stats/DNF/loss are counted
    GAME_RESULT_CONCEDE_QUIT,            // Game was terminated when the user Conceded Defeat(Opponent agreed). Stats/loss are counted, no DNF given
    GAME_RESULT_MERCY_QUIT,              // Game was terminated when the user Granted Mercy(Opponent agreed). Stats/loss are counted, no DNF given
    GAME_RESULT_QUITANDCOUNT,            // Game was terminated when opponent quit and user wants to count game, stats/DNF/loss are counted
    GAME_RESULT_DNF_DISCONNECT,          // Game was terminated by opponent disconnects, no stats counted. DNF/loss counted.
    GAME_RESULT_FRIENDLY_DISCONNECT,     // Game was terminated when opponent disconnects, user wants no stats/DNF/loss are counted
    GAME_RESULT_CONCEDE_DISCONNECT,      // Game was terminated when the user Conceded Defeat(Opponent agreed). Stats/loss are counted, no DNF given
    GAME_RESULT_MERCY_DISCONNECT,        // Game was terminated when the user Granted Mercy(Opponent agreed). Stats/loss are counted, no DNF given
    GAME_RESULT_DISCONNECTANDCOUNT,      // Game was terminated when opponent disconnects, user wants to give Loss/DNF to opponent. All stats count
    GAME_RESULT_DESYNCED,                // Game was terminated because of a desync
    GAME_RESULT_OTHER                    // Game was terminated because of other reason
};

enum PlayerFinishReasonE
{
    PLAYER_FINISH,                      // Player finished the game
    PLAYER_FINISH_REASON_QUIT,          // Player quit from the game
    PLAYER_FINISH_REASON_DISCONNECT,    // Player disconnect from the game
    PLAYER_FINISH_REASON_OTHER          // Player with other not finish reason
};

#define ATTRIBNAME_USERRESULT      "USERRESULT"
#define ATTRIBNAME_WINNERBYDNF     "winnerByDnf"
#define ATTRIBNAME_ROOMTEAM0       "OSDK.team0"
#define ATTRIBNAME_ROOMTEAM1       "OSDK.team1"
#define ATTRIBNAME_ROOMWINS0       "OSDK.wins0"
#define ATTRIBNAME_ROOMWINS1       "OSDK.wins1"
#define ATTRIBNAME_ROOMGAMESPLAYED "OSDK.gamesPlayed"

#define STATNAME_LOSSES               "losses"
#define STATNAME_LSTREAK              "lstreak"
#define STATNAME_OPPO_LEVEL           "opponentLevel"
#define STATNAME_OVERALLPTS           "overallSkillPoints"
#define STATNAME_POINTS               "points"
#define STATNAME_PTSAGAINST           "pointsAgainst"
#define STATNAME_PTSFOR               "pointsFor"
#define STATNAME_RESULT               "result"
#define STATNAME_SKILL                "skill"
#define STATNAME_TOTAL_GAMES_PLAYED   "totalGamesPlayed"
#define STATNAME_GAMES_NOT_FINISHED   "totalGamesNotFinished"
#define STATNAME_WINS                 "wins"
#define STATNAME_WSTREAK              "wstreak"
#define STATNAME_ATTRIBUTE            "attribute"
#define STATNAME_TIES	              "ties"
#define STATNAME_USTREAK              "unbeatenstreak"

#define SCOPENAME_ACCOUNTCOUNTRY    "accountcountry"
#define SCOPENAME_COUNTRY           "country"
#define SCOPENAME_POS               "pos"

//FIFA
#define SCOPENAME_CONTROLS          "controls"
#define SCOPENAME_SEASON            "season"
#define SCOPENAME_TEAMID	        "teamid"
#define SCOPENAME_COMPETITIONID     "competitionid"

}   // namespace GameReporting
}   // namespace Blaze

#endif // BLAZE_GAMEREPORTING_GAMECOMMON_H
