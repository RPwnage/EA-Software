/*H*************************************************************************************************/
/*!

    \File    dirtynames.h

    \Description
        This module provides helper functions for manipulating persona and master
        account name strings.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002-2003.  ALL RIGHTS RESERVED.

    \Version    1.0        12/10/03 (DBO) First Version
    
*/
/*************************************************************************************************H*/


#ifndef _lobbynames_h
#define _lobbynames_h

/*!
    \Moduledef Lobby Lobby

    \Description
        Lobby functionality 
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

// Compare two names for equality.  Ignore case and whitespace.
extern int32_t DirtyUsernameCompare ( const char *pName1, const char *pName2 );

// determine if pMatch is a substring of pSrc.  Ignore case and whitespace
extern int32_t DirtyUsernameSubstr ( const char *pSrc, const char *pMatch );

#ifdef __cplusplus
}
#endif

//@}

#endif // _lobbynames_h

