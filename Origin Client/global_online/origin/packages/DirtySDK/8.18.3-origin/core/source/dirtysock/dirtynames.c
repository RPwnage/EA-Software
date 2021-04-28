/*H*************************************************************************************************/
/*!

    \File    dirtynames.c
    
    \Description
        This module provides helper functions for manipulating persona and master
        account name strings.
        
    \Notes
        None.
            
    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002-2003.  ALL RIGHTS RESERVED.
    
    \Version    1.0        12/10/02 (DBO) First Version
    
*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>

#include "platform.h"
#include "dirtynames.h"

/*** Defines ***************************************************************************/

#ifndef TRUE
#define TRUE (1)
#define FALSE (0)
#endif

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

static const unsigned char xlat[256] ={
        0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01, 
        0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01, 
        0x01,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
        0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
        0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
        0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
        0x60,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
        0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x7b,0x7c,0x7d,0x7e,0x01,
        0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01, 
        0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01, 
        0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01, 
        0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01, 
        0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01, 
        0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01, 
        0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01, 
        0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01
    };

// Public variables


/*** Private Functions *****************************************************************/

/*** Public Functions ******************************************************************/


/*F************************************************************************************************/
/*!
    \Function   DirtyUsernameCompare
 
    \Description
        Compare two master account names.  Ignore case, whitespace, and
        non-ascii characters.
 
    \Input pName1 - Location to store converted name
    \Input pName2 - Input account name
    
    \Output int32_t - < 0 = pName1 < pName2
                  = 0 = pName1 = pName2
                  > 0 = pName1 > pName2

    \Version    1.0     05/01/2003 (DBO) Taken from LobbyHasher (HashStrCmp)
*/
/************************************************************************************************F*/
int32_t DirtyUsernameCompare(const char *pName1, const char *pName2)
{
    int32_t iCmp;
    unsigned char cCh1, cCh2;

    // compare the strings
    do {
        // grab the data
        while ((cCh1 = xlat[*(uint8_t*)pName1++]) == 1)
            ;
        while ((cCh2 = xlat[*(uint8_t*)pName2++]) == 1)
            ;
        // see if different
        if ((iCmp = cCh1-cCh2) != 0)
            return(iCmp);
    } while (cCh1 != 0);

    // they are the same
    return(0);
}


/*F************************************************************************************************/
/*!
    \Function   DirtyUsernameSubstr
 
    \Description
        Determine if pMatch is a substring of pSrc.  Ignore case, whitespace,
        and non-ascii characters.
 
    \Input pSrc - Location to store converted name
    \Input pMatch - Input account name
    
    \Output int32_t - TRUE if pMatch is a substring of pSrc; FALSE otherwise

    \Version    1.0     05/02/2003 (DBO) Initial version.
*/
/************************************************************************************************F*/
int32_t DirtyUsernameSubstr ( const char *pSrc, const char *pMatch )
{
    int32_t iSrcIdx;
    int32_t iMatchIdx1;
    int32_t iMatchIdx2;
    char cCh1;
    char cCh2;

    if ( (pMatch == NULL) || (pMatch[0] == '\0') )
        return(TRUE);
    if ( (pSrc == NULL) || (pSrc[0] == '\0') )
        return(FALSE);

    for(iSrcIdx = 0; pSrc[iSrcIdx] != '\0'; iSrcIdx++)
    {
        iMatchIdx1 = iSrcIdx;
        iMatchIdx2 = 0;

        do
        {
            // Skip over whitespace and invalid characters
            while ( (cCh1 = xlat[(uint32_t)pSrc[iMatchIdx1++]]) == 1 )
                ;
            while ( (cCh2 = xlat[(uint32_t)pMatch[iMatchIdx2++]]) == 1 )
                ;

            if ( cCh2 == 0 )
                return(TRUE);
        }
        while ( (cCh1 != 0) && (cCh1 == cCh2) );
    }
    return(FALSE);
}

