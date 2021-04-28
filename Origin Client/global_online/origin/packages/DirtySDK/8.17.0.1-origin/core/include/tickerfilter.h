/*H********************************************************************************/
/*!
    \File tickerfilter.h

    \Description
        This module defines the values for selecting a ticker filter.

    \Copyright
        Copyright (c) Electronic Arts 2006. ALL RIGHTS RESERVED.

    \Version 01/16/2006 (doneill) First Version
*/
/********************************************************************************H*/

#ifndef _ticker_filter_h
#define _ticker_filter_h

/*** Defines **********************************************************************/

typedef enum
{
    TICKER_FILT_INVALID                     = -1,

    // Game events (pre-game, in-game, and post-game scores)
    //
    TICKER_FILT_GAME_MLB                    = 0,
    TICKER_FILT_GAME_NHL                    = 1,
    TICKER_FILT_GAME_GOLF                   = 2,

    TICKER_FILT_GAME_NFL                    = 3,
    TICKER_FILT_GAME_NCAAF                  = 4,
    TICKER_FILT_GAME_AFL                    = 5,
        
    TICKER_FILT_GAME_NBA                    = 6,
    TICKER_FILT_GAME_NCAAB                  = 7,
        
    //
    // Game events for auto racing
    //
    TICKER_FILT_GAME_NASCAR                 = 10,
    TICKER_FILT_GAME_IRL                    = 11,
    TICKER_FILT_GAME_BUSCH                  = 12,
    TICKER_FILT_GAME_TRUCK                  = 13,
    TICKER_FILT_GAME_CART                   = 14,
    TICKER_FILT_GAME_F1                     = 15,
    TICKER_FILT_GAME_NHRA                   = 16,
    TICKER_FILT_GAME_IROC                   = 17,

    //
    // FIFA leagues
    //
    TICKER_FILT_GAME_FIFA_ENGLAND_FA_CUP      = 18,
    TICKER_FILT_GAME_FIFA_ENGLAND_LOWER       = 19,
    TICKER_FILT_GAME_FIFA_ENGLAND             = 20,
    TICKER_FILT_GAME_FIFA_SCOTLAND            = 21,
    TICKER_FILT_GAME_FIFA_ITALY               = 22,
    TICKER_FILT_GAME_FIFA_SPAIN               = 23,
    TICKER_FILT_GAME_FIFA_MLS                 = 24,
    TICKER_FILT_UNUSED0                       = 25,
    TICKER_FILT_GAME_FIFA_GERMANY             = 26,
    TICKER_FILT_GAME_FIFA_MEXICO              = 27,
    TICKER_FILT_GAME_FIFA_HOLLAND             = 28,
    TICKER_FILT_GAME_FIFA_FRANCE              = 29,
    TICKER_FILT_UNUSED1                       = 30,
    TICKER_FILT_UNUSED2                       = 31,
    TICKER_FILT_GAME_FIFA_UEFA_CUP            = 32,
    TICKER_FILT_GAME_FIFA_UEFA_CHAMPIONS      = 33,
    TICKER_FILT_GAME_FIFA_FIFA_FRIENDLY       = 34,
    TICKER_FILT_GAME_FIFA_FIFA_CONFEDERATIONS = 35,
    TICKER_FILT_GAME_FIFA_FIFA_WORLD_CUP      = 36,
    TICKER_FILT_GAME_FIFA_CONMEBOL            = 37,
    TICKER_FILT_GAME_FIFA_CONCACAF            = 38,

    //!< Unclassified game events
    TICKER_FILT_UNUSED3                       = 39,

    //
    // News events for various sports
    //
    TICKER_FILT_NEWS_GOLF                   = 40,
    TICKER_FILT_NEWS_MLB                    = 41,
    TICKER_FILT_NEWS_NBA                    = 42,
    TICKER_FILT_NEWS_NCAAB                  = 43,
    TICKER_FILT_NEWS_NCAAF                  = 44,
    TICKER_FILT_NEWS_NFL                    = 45,
    TICKER_FILT_NEWS_NHL                    = 46,
    TICKER_FILT_NEWS_AUTO                   = 47,

    //!< Unclassified news events 
    TICKER_FILT_NEWS_GENERAL                = 48,

    TICKER_FILT_NEWS_FIFA                   = 49,


    //!< More soccer leagues
    TICKER_FILT_GAME_FIFA_ENGLAND_WORTHINGTON = 52,
    TICKER_FILT_GAME_FIFA_SOUTH_AMERICA       = 53,
    TICKER_FILT_GAME_FIFA_SCOTLAND_LOWER      = 54,
    TICKER_FILT_GAME_FIFA_AUSTRALIA           = 55,


	//!< ItemML Filters
	TICKER_FILT_ITEMML_1					  = 58,
	TICKER_FILT_ITEMML_2					  = 59,


    //!< Server admin messages
    TICKER_FILT_MESSAGE                     = 60,

    //!< Local general message event (client-side only)
    TICKER_FILT_LOCAL_MESSAGE               = 62,

    //!< Local buddy event (client-side only)
    TICKER_FILT_BUDDY                       = 63
} TickerFilterItemE;

//
// Group filter events together
//

//!< All auto racing leagues
#define TICKER_FILT_GAME_AUTO_ALL \
            TICKER_FILT_GAME_NASCAR, \
            TICKER_FILT_GAME_IRL, \
            TICKER_FILT_GAME_BUSCH, \
            TICKER_FILT_GAME_TRUCK, \
            TICKER_FILT_GAME_CART, \
            TICKER_FILT_GAME_F1, \
            TICKER_FILT_GAME_NHRA, \
            TICKER_FILT_GAME_IROC

//!< All soccer leagues
#define TICKER_FILT_GAME_FIFA_ALL \
            TICKER_FILT_GAME_FIFA_ENGLAND_FA_CUP, \
            TICKER_FILT_GAME_FIFA_ENGLAND_LOWER, \
            TICKER_FILT_GAME_FIFA_ENGLAND, \
            TICKER_FILT_GAME_FIFA_SCOTLAND, \
            TICKER_FILT_GAME_FIFA_ITALY, \
            TICKER_FILT_GAME_FIFA_SPAIN, \
            TICKER_FILT_GAME_FIFA_MLS, \
            TICKER_FILT_GAME_FIFA_GERMANY, \
            TICKER_FILT_GAME_FIFA_MEXICO, \
            TICKER_FILT_GAME_FIFA_HOLLAND, \
            TICKER_FILT_GAME_FIFA_FRANCE, \
            TICKER_FILT_GAME_FIFA_UEFA_CUP, \
            TICKER_FILT_GAME_FIFA_UEFA_CHAMPIONS, \
            TICKER_FILT_GAME_FIFA_FIFA_FRIENDLY, \
            TICKER_FILT_GAME_FIFA_FIFA_CONFEDERATIONS, \
            TICKER_FILT_GAME_FIFA_FIFA_WORLD_CUP, \
            TICKER_FILT_GAME_FIFA_CONMEBOL, \
            TICKER_FILT_GAME_FIFA_CONCACAF, \
            TICKER_FILT_GAME_FIFA_ENGLAND_WORTHINGTON, \
            TICKER_FILT_GAME_FIFA_SOUTH_AMERICA, \
            TICKER_FILT_GAME_FIFA_SCOTLAND_LOWER, \
            TICKER_FILT_GAME_FIFA_AUSTRALIA

//<! All news items
#define TICKER_FILT_NEWS_ALL \
            TICKER_FILT_NEWS_GOLF, \
            TICKER_FILT_NEWS_MLB, \
            TICKER_FILT_NEWS_NBA, \
            TICKER_FILT_NEWS_NCAAB, \
            TICKER_FILT_NEWS_NCAAF, \
            TICKER_FILT_NEWS_NFL, \
            TICKER_FILT_NEWS_NHL, \
            TICKER_FILT_NEWS_AUTO, \
            TICKER_FILT_NEWS_GENERAL, \
            TICKER_FILT_NEWS_FIFA

//<! All game events
#define TICKER_FILT_GAME_ALL \
            TICKER_FILT_GAME_MLB, \
            TICKER_FILT_GAME_NHL, \
            TICKER_FILT_GAME_GOLF, \
            TICKER_FILT_GAME_NFL, \
            TICKER_FILT_GAME_NCAAF, \
            TICKER_FILT_GAME_AFL, \
            TICKER_FILT_GAME_NBA, \
            TICKER_FILT_GAME_NCAAB, \
            TICKER_FILT_GAME_FIFA_ALL, \
            TICKER_FILT_GAME_AUTO_ALL


//
// Define possible states for sports score events (TickerRecordT->iState)
//

//!< Event is a pre-game update
#define TICKER_EVENT_STATE_SPORT_PREGAME               0

//!< Event is an in-game update
#define TICKER_EVENT_STATE_SPORT_INGAME                1

//!< Event is a final score
#define TICKER_EVENT_STATE_SPORT_COMPLETE              2

//!< Event indicates a postponed game
#define TICKER_EVENT_STATE_SPORT_POSTPONED             3

#endif // _ticker_filter_h

