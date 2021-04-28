////////////////////////////////////////////////////////////////////
//
// .\T2.spa.h
//
// Auto-generated on Wednesday, 23 May 2012 at 20:00:03
// Xbox LIVE Game Config project version 1.0.56.0
// SPA Compiler version 1.0.0.0
//
////////////////////////////////////////////////////////////////////

#ifndef __DIRTYSOCK_SAMPLE_SPA_H__
#define __DIRTYSOCK_SAMPLE_SPA_H__

#ifdef __cplusplus
extern "C" {
#endif

//
// Title info
//

#define TITLEID_DIRTYSOCK_SAMPLE                    0x454107D5

//
// Context ids
//
// These values are passed as the dwContextId to XUserSetContext.
//

#define CONTEXT_MAP                                 2

//
// Context values
//
// These values are passed as the dwContextValue to XUserSetContext.
//

// Values for CONTEXT_MAP

#define CONTEXT_MAP_STALINGRAD                      0
#define CONTEXT_MAP_LEYTE_GULF                      1
#define CONTEXT_MAP_ARDENNES                        2
#define CONTEXT_MAP_NORMANDY                        3

// Values for X_CONTEXT_PRESENCE

#define CONTEXT_PRESENCE_DIRTYSOCKSAMPLEPRESENCE    0

// Values for X_CONTEXT_GAME_MODE

#define CONTEXT_GAME_MODE_DEATHMATCH                0
#define CONTEXT_GAME_MODE_COOPERATIVE               1
#define CONTEXT_GAME_MODE_TEAM_BATTLE               2

//
// Property ids
//
// These values are passed as the dwPropertyId value to XUserSetProperty
// and as the dwPropertyId value in the XUSER_PROPERTY structure.
//

#define PROPERTY_VICTORY_POINTS                     0x10000001
#define PROPERTY_MIN_VICTORY_POINTS                 0x10000002
#define PROPERTY_MAX_VICTORY_POINTS                 0x10000003
#define PROPERTY_GAMES_PLAYED                       0x10000007
#define PROPERTY_POINTS_SCORED                      0x10000009
#define PROPERTY_GAMES_WON                          0x20000008

//
// Achievement ids
//
// These values are used in the dwAchievementId member of the
// XUSER_ACHIEVEMENT structure that is used with
// XUserWriteAchievements and XUserCreateAchievementEnumerator.
//

#define ACHIEVEMENT_SLEW_THE_RABID_MARMOT           2
#define ACHIEVEMENT_RESCUED_THE_MARQUESSA           3
#define ACHIEVEMENT_RECOVERED_THE_LAUNDRY           6
#define ACHIEVEMENT_FOUND_THE_CHIPS                 7
#define ACHIEVEMENT_KILLED_THE_EVIL_PLUMBER         8

//
// AvatarAssetAward ids
//


//
// Stats view ids
//
// These are used in the dwViewId member of the XUSER_STATS_SPEC structure
// passed to the XUserReadStats* and XUserCreateStatsEnumerator* functions.
//

// Skill leaderboards for ranked game modes

#define STATS_VIEW_SKILL_RANKED_DEATHMATCH          0xFFFF0000
#define STATS_VIEW_SKILL_RANKED_COOPERATIVE         0xFFFF0001
#define STATS_VIEW_SKILL_RANKED_TEAM_BATTLE         0xFFFF0002

// Skill leaderboards for unranked (standard) game modes

#define STATS_VIEW_SKILL_STANDARD_DEATHMATCH        0xFFFE0000
#define STATS_VIEW_SKILL_STANDARD_COOPERATIVE       0xFFFE0001
#define STATS_VIEW_SKILL_STANDARD_TEAM_BATTLE       0xFFFE0002

// Title defined leaderboards

#define STATS_VIEW_RANKED_GAMES                     1
#define STATS_VIEW_STANDARD_GAMES                   2
#define STATS_VIEW_STANDARD_TEAM_PLAY               3
#define STATS_VIEW_STANDARD_DEATHMATCH              4
#define STATS_VIEW_STANDARD_COOPERATIVE             5
#define STATS_VIEW_RANKED_TEAM_PLAY                 6
#define STATS_VIEW_RANKED_COOPERATIVE               7
#define STATS_VIEW_RANKED_DEATHMATCH                8

//
// Stats view column ids
//
// These ids are used to read columns of stats views.  They are specified in
// the rgwColumnIds array of the XUSER_STATS_SPEC structure.  Rank, rating
// and gamertag are not retrieved as custom columns and so are not included
// in the following definitions.  They can be retrieved from each row's
// header (e.g., pStatsResults->pViews[x].pRows[y].dwRank, etc.).
//

// Column ids for RANKED_GAMES

#define STATS_COLUMN_RANKED_GAMES_GAMES_PLAYED      1
#define STATS_COLUMN_RANKED_GAMES_POINTS_SCORED     2
#define STATS_COLUMN_RANKED_GAMES_LAST_MAP          3

// Column ids for STANDARD_GAMES

#define STATS_COLUMN_STANDARD_GAMES_GAMES_PLAYED    1
#define STATS_COLUMN_STANDARD_GAMES_POINTS_SCORED   2
#define STATS_COLUMN_STANDARD_GAMES_LAST_MAP        3

// Column ids for STANDARD_TEAM_PLAY

#define STATS_COLUMN_STANDARD_TEAM_PLAY_GAMES_PLAYED 1
#define STATS_COLUMN_STANDARD_TEAM_PLAY_POINTS_SCORED 2
#define STATS_COLUMN_STANDARD_TEAM_PLAY_LAST_MAP    3

// Column ids for STANDARD_DEATHMATCH

#define STATS_COLUMN_STANDARD_DEATHMATCH_GAMES_PLAYED 1
#define STATS_COLUMN_STANDARD_DEATHMATCH_POINTS_SCORED 2
#define STATS_COLUMN_STANDARD_DEATHMATCH_LAST_MAP   3

// Column ids for STANDARD_COOPERATIVE

#define STATS_COLUMN_STANDARD_COOPERATIVE_GAMES_PLAYED 1
#define STATS_COLUMN_STANDARD_COOPERATIVE_POINTS_SCORED 2
#define STATS_COLUMN_STANDARD_COOPERATIVE_LAST_MAP  3

// Column ids for RANKED_TEAM_PLAY

#define STATS_COLUMN_RANKED_TEAM_PLAY_GAMES_PLAYED  1
#define STATS_COLUMN_RANKED_TEAM_PLAY_POINTS_SCORED 2
#define STATS_COLUMN_RANKED_TEAM_PLAY_LAST_MAP      3

// Column ids for RANKED_COOPERATIVE

#define STATS_COLUMN_RANKED_COOPERATIVE_GAMES_PLAYED 1
#define STATS_COLUMN_RANKED_COOPERATIVE_POINTS_SCORED 2
#define STATS_COLUMN_RANKED_COOPERATIVE_LAST_MAP    3

// Column ids for RANKED_DEATHMATCH

#define STATS_COLUMN_RANKED_DEATHMATCH_GAMES_PLAYED 1
#define STATS_COLUMN_RANKED_DEATHMATCH_POINTS_SCORED 2
#define STATS_COLUMN_RANKED_DEATHMATCH_LAST_MAP     3

//
// Matchmaking queries
//
// These values are passed as the dwProcedureIndex parameter to
// XSessionSearch to indicate which matchmaking query to run.
//

#define SESSION_MATCH_QUERY_FIND_MATCHES            0

//
// Gamer pictures
//
// These ids are passed as the dwPictureId parameter to XUserAwardGamerTile.
//


//
// Strings
//
// These ids are passed as the dwStringId parameter to XReadStringsFromSpaFile.
//

#define SPASTRING_RANKED                            4
#define SPASTRING_STANDARD                          5
#define SPASTRING_VICTORY_POINTS_NAME               7
#define SPASTRING_MIN_VICTORY_POINTS_NAME           8
#define SPASTRING_MAX_VICTORY_POINTS_NAME           9
#define SPASTRING_MAP_NAME_NAME                     10
#define SPASTRING_DEATHMATCH                        11
#define SPASTRING_COOPERATIVE                       12
#define SPASTRING_TEAM_BATTLE                       13
#define SPASTRING_RECOVERED_THE_GOLD_RING_OF_SHAZOR_NAME 15
#define SPASTRING_RECOVERED_THE_GOLD_RING_OF_SHAZOR_DESC 16
#define SPASTRING_RECOVERED_THE_GOLD_RING_OF_SHAZOR_HOWTO 17
#define SPASTRING_SLEW_THE_RABID_MARMOT_NAME        18
#define SPASTRING_SLEW_THE_RABID_MARMOT_DESC        19
#define SPASTRING_SLEW_THE_RABID_MARMOT_HOWTO       20
#define SPASTRING_RESCUED_THE_MARQUESSA_NAME        21
#define SPASTRING_RESCUED_THE_MARQUESSA_DESC        22
#define SPASTRING_RESCUED_THE_MARQUESSA_HOWTO       23
#define SPASTRING_FOUND_THE_MISSING_PAPERCLIP_NAME  24
#define SPASTRING_FOUND_THE_MISSING_PAPERCLIP_DESC  25
#define SPASTRING_FOUND_THE_MISSING_PAPERCLIP_HOWTO 26
#define SPASTRING_HUMILIATED_THE_EVIL_PLUMBER_NAME  27
#define SPASTRING_HUMILIATED_THE_EVIL_PLUMBER_DESC  28
#define SPASTRING_HUMILIATED_THE_EVIL_PLUMBER_HOWTO 29
#define SPASTRING_RECOVERED_THE_LAUNDRY_NAME        30
#define SPASTRING_RECOVERED_THE_LAUNDRY_DESC        31
#define SPASTRING_RECOVERED_THE_LAUNDRY_HOWTO       32
#define SPASTRING_FOUND_THE_CHIPS_NAME              33
#define SPASTRING_FOUND_THE_CHIPS_DESC              34
#define SPASTRING_FOUND_THE_CHIPS_HOWTO             35
#define SPASTRING_KILLED_THE_EVIL_PLUMBER_NAME      36
#define SPASTRING_KILLED_THE_EVIL_PLUMBER_DESC      37
#define SPASTRING_KILLED_THE_EVIL_PLUMBER_HOWTO     38
#define SPASTRING_MAP_NAME                          40
#define SPASTRING_STALINGRAD                        41
#define SPASTRING_LEYTE_GULF                        42
#define SPASTRING_ARDENNES                          43
#define SPASTRING_NORMANDY                          44
#define SPASTRING_RANKED_GAMES_NAME                 45
#define SPASTRING_GAMES_PLAYED_NAME                 50
#define SPASTRING_GAMES_WON_NAME                    51
#define SPASTRING_POINTS_SCORED_NAME                52
#define SPASTRING_LB_2_NAME                         53
#define SPASTRING_LB_3_NAME                         54
#define SPASTRING_LB_4_NAME                         55
#define SPASTRING_LB_5_NAME                         56
#define SPASTRING_LB_6_NAME                         57
#define SPASTRING_LB_7_NAME                         58
#define SPASTRING_LB_8_NAME                         59
#define SPASTRING_STANDARD_GAMES_NAME               60
#define SPASTRING_RANKED_DEATHMATCH_NAME            61
#define SPASTRING_RANKED_COOPERATIVE_NAME           62
#define SPASTRING_RANKED_TEAM_PLAY_NAME             63
#define SPASTRING_STANDARD_COOPERATIVE_NAME         64
#define SPASTRING_STANDARD_DEATHMATCH_NAME          65
#define SPASTRING_STANDARD_TEAM_PLAY_NAME           66
#define SPASTRING_DEVELOPER                         67
#define SPASTRING_PUBLISHER                         68
#define SPASTRING_SELL_TEXT                         69
#define SPASTRING_GENRE                             70
#define SPASTRING_DIRTYSOCKSAMPLEPRESENCE           72


#ifdef __cplusplus
}
#endif

#endif // __DIRTYSOCK_SAMPLE_SPA_H__


