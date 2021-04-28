////////////////////////////////////////////////////////////////////
//
// C:\projects\EAOS\EAPatch\DL\EAPatchClient\dev\test\source\XBox360\EATechTestApp.spa.h
//
// Auto-generated on Friday, 19 April 2013 at 01:44:22
// Xbox LIVE Game Config project version 1.0.3.0
// SPA Compiler version 1.0.0.0
//
////////////////////////////////////////////////////////////////////

#ifndef __EATECH_TEST_APP_SPA_H__
#define __EATECH_TEST_APP_SPA_H__

#ifdef __cplusplus
extern "C" {
#endif

//
// Title info
//

#define TITLEID_EATECH_TEST_APP                     0xEAEAEA01

//
// Context ids
//
// These values are passed as the dwContextId to XUserSetContext.
//


//
// Context values
//
// These values are passed as the dwContextValue to XUserSetContext.
//

// Values for X_CONTEXT_PRESENCE


// Values for X_CONTEXT_GAME_MODE


//
// Property ids
//
// These values are passed as the dwPropertyId value to XUserSetProperty
// and as the dwPropertyId value in the XUSER_PROPERTY structure.
//


//
// Achievement ids
//
// These values are used in the dwAchievementId member of the
// XUSER_ACHIEVEMENT structure that is used with
// XUserWriteAchievements and XUserCreateAchievementEnumerator.
//


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


// Skill leaderboards for unranked (standard) game modes


// Title defined leaderboards


//
// Stats view column ids
//
// These ids are used to read columns of stats views.  They are specified in
// the rgwColumnIds array of the XUSER_STATS_SPEC structure.  Rank, rating
// and gamertag are not retrieved as custom columns and so are not included
// in the following definitions.  They can be retrieved from each row's
// header (e.g., pStatsResults->pViews[x].pRows[y].dwRank, etc.).
//

//
// Matchmaking queries
//
// These values are passed as the dwProcedureIndex parameter to
// XSessionSearch to indicate which matchmaking query to run.
//


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



#ifdef __cplusplus
}
#endif

#endif // __EATECH_TEST_APP_SPA_H__


