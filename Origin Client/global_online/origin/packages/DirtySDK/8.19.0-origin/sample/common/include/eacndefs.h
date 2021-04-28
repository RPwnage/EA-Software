/*H********************************************************************************/
/*!
    \File eacndefs.h

    \Description
        Very basic ea connect parameters and definies used to start ea connect.
        Should be common to all platforms.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 05/11/2005 (jfrank) First Version, based on Tyrone Ebert's version
*/
/********************************************************************************H*/

#ifndef _eacndefs_h
#define _eacndefs_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

#define EACN_PARAM_LEN  (64)    //!< max size for each param in the CSV
#define EACN_ARG_LEN    (255)   //!< max size of arg to main program, -i.e. total length of CSV.
#define EACN_DELIM      (",")   //!< delimiter between params -- its a CSV!

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! EA Connect Arg positions in CSV. 
//! Cannot use tags due to PS2 255 char space limitations.
typedef enum EACNStartupListParamE
{
    EACN_GAME_EXE = 0,          //!< game executable path/file, used to relaunch when EA Connect is done
    EACN_DNAS_AND_AUTH_FILE,    //!< path to DNAS and AUTH encoded file
    EACN_DNAS_AND_AUTH_KEY,     //!< key used to encrypt the DNAS and AUTH data
    EACN_PASSPHRASE,            //!< DNAS passphrase for PS2 authentication
    EACN_NETGUI_FILE,           //!< Path on CDROM to NetGUI - works with CDROM only
    EACN_HOST,                  //!< Game's lobby server host, external name is usign sony test dns for DNAS
    EACN_PORT,                  //!< Game's lobby server port
    EACN_LANG,                  //!< Language to run in
    EACN_FLAGS,                 //!< Debug or other flags
    EACN_GAME_NAME,             //!< This game name (eg. "Madden 2006")
    EACN_GAME_PARAMS,           //!< Parameters this program might need on return from dashboard
    EACN_GAME_ID,               //!< The Game's Product ID
    EACN_GAME_SKU,              //!< SKU --required for web offer
    EACN_GAME_ISPAL,            //!< set to 1 if PAL game, 0 or blank otherwise
    EACN_GAME_SLUS,             //!< SLUS code -required for web offer
    EACN_NET_CONFIG_INDEX,      //!< >0 skips net config screen and use what user used in game
    EACN_USE_EXISTING_ACCOUNT,  //!< set to 1 to skip Acnt Select screen.
    EACN_NO_MODEM,              //!< set to 1 if game does NOT support modem.
    EACN_RESERVED,              //!< reserved for future use
    EACN_PARAM_COUNT            //!< how many params in csv  --always last
} EACNStartupListParamE;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
};
#endif

#endif // _eacndefs_h

