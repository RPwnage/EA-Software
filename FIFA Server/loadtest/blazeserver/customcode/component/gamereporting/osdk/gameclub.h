/*************************************************************************************************/
/*!
\file   gameclub.h
\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_CUSTOM_GAMEREPORTING_GAMECLUB_H
#define BLAZE_CUSTOM_GAMEREPORTING_GAMECLUB_H

namespace Blaze
{
namespace GameReporting
{
    #define CATEGORYNAME_CLUB_RANKING         "ClubRankStats"
    #define CATEGORYNAME_CLUB_SEASON          "ClubSeasonalPlayStats"
    #define CATEGORYNAME_CLUB_MEMBERS         "ClubMemberStats"

    #define ATTRIBNAME_CLUBCHALLENGE  "OSDK_ChlngrClubId"

    #define STATNAME_CLUBGAMES         "clubgames"
    #define STATNAME_POSITION          "position"
    #define STATNAME_GAME_TIME         "minutes"

    #define SCOPENAME_REGION "clubregion"
    #define SCOPENAME_SEASON "season"

    #define QUERY_CLUBGAMESTATS "club_skill_damping_query"
    
}
}


#endif //BLAZE_CUSTOM_GAMEREPORTING_GAMECLUB_H

