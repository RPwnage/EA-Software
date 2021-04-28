/*H********************************************************************************/
/*!
    \File zmem.c

    \Description
        Memory implementations for use on all platforms.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/16/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"

#include "zmemtrack.h"
#include "zmem.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function ZMemAlloc

    \Description
        Allocate some memory

    \Input  uSize  - amount of memory, in bytes, to allocate
    
    \Output void * - pointer to an allocated memory chunk

    \Version 03/16/2005 (jfrank)
*/
/********************************************************************************F*/
void *ZMemAlloc(uint32_t uSize)
{
    void *pMem;
    if ((pMem = (void *)malloc(uSize)) != NULL)
    {
        memset(pMem, 0xCD, uSize);
        ZMemtrackAlloc(pMem, uSize, 0);
    }
    return(pMem);
}


/*F********************************************************************************/
/*!
    \Function ZMemFree

    \Description
        Free a previously allocated chunk of memory

    \Input  void *pMem - pointer to an allocated memory chunk
    
    \Output None

    \Version 03/16/2005 (jfrank)
*/
/********************************************************************************F*/
uint32_t ZMemFree(void *pMem)
{
    uint32_t uSize;
    ZMemtrackFree(pMem, &uSize);
    memset(pMem, 0xEF, uSize);
    free(pMem);
    return(uSize);
}

/*F********************************************************************************/
/*!
    \Function ZMemSet

    \Description
        Initialize a memory chunk to a specified value.

    \Input  void           *pDest  - pointer to an allocated memory chunk
    \Input  int32_t             iVal   - value to set the memory to
    \Input  uint32_t    uCount - length, in bytes, of memory to set
    
    \Output                 void * - pointer to memory which was just initialized

    \Version 03/16/2005 (jfrank)
*/
/********************************************************************************F*/
void *ZMemSet(void *pDest, int32_t iVal, uint32_t uCount)
{
    return((void *)(memset(pDest, iVal, uCount)));
}


/*F********************************************************************************/
/*!
    \Function ZMemCpy

    \Description
        Copies characters between buffers.

    \Input  void           *pDest  - pointer to destination memory
    \Input  void           *pSrc   - pointer to source memory
    \Input  uint32_t    uCount - length, in bytes, of memory to copy
    
    \Output                 void * - pointer to resultant desintation memory

    \Version 03/24/2005 (jfrank)
*/
/********************************************************************************F*/
void *ZMemCpy(void *pDest, const void *pSrc, uint32_t uCount)
{
    return((void *)memcpy(pDest, pSrc, uCount));
}

