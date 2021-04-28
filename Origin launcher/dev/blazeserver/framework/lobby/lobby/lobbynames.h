/*H*************************************************************************************************/
/*!

    \File    lobbynames.h
    
    \Description
        This module provides helper functions for manipulating persona and master
        account name strings.
        
    \Notes
        None.
            
    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002-2003.  ALL RIGHTS RESERVED.
    
    \Version    1.0        12/10/03 (DBO) First Version
    
*/
/*************************************************************************************************H*/


#ifndef _lobbynames_h
#define _lobbynames_h

/*!
\Module Lobby
*/
//@{

/*** Include files *********************************************************************/

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// Compare two names for equality.  Ignore case and strip non-printable ascii characters.
extern int32_t LobbyNameCmp ( const char *pName1, const char *pName2 );

// determine if pMatch is a substring of pSrc.  Ignore case and strip non-printable ascii characters.
extern int32_t LobbyNameSubstr ( const char *pSrc, const char *pMatch );

// Create the canonical form of the given name.  Essentially lowercases the name and strip non-printable ascii characters.
extern int32_t LobbyNameCreateCanonical(const char *pName, char * pCanonical, size_t uLen);

// Generate a hash code for the given name.  Ignore case and strip non-printable ascii characters.
extern uint32_t LobbyNameHash(const char *pName);

#ifdef __cplusplus
}
#endif

//@}

#endif // _lobbynames_h

