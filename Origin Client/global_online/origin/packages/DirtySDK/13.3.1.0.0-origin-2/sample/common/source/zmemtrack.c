/*H********************************************************************************/
/*!
    \File zmemtrack.c

    \Description
        Routines for tracking memory allocations.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 02/15/2005 (jbrookes) First Version, based on jfrank's implementation.
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <ctype.h>

#include "DirtySDK/platform.h"
#include "zlib.h"
#include "zmem.h"
#include "zmemtrack.h"

/*** Defines **********************************************************************/

#define ZMEMTRACK_MEMDUMPBYTES              (64)    //!< number of bytes to print around the leak
#define ZMEMTRACK_MAXALLOCATIONS            (1024)  //!< maximum list allocation size

/*** Type Definitions *************************************************************/

typedef struct ZMemtrackElemT
{
    void *pMem;
    uint32_t uMemSize;
    uint32_t uTag;
} ZMemtrackElemT;

typedef struct ZMemtrackRefT
{
    uint32_t uNumAllocations;
    uint32_t uMaxAllocations;
    uint32_t uTotalAllocations;
    uint32_t uTotalMemory;
    uint32_t uMaxMemory;
    uint32_t bOverflow;

    ZMemtrackElemT MemList[ZMEMTRACK_MAXALLOCATIONS];
} ZMemtrackRefT;

/*** Variables ********************************************************************/

static ZMemtrackRefT _ZMemtrack_Ref;

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _ZMemtrackPrintLeak

    \Description
        Print a memory leak to debug output.

    \Input *pElem   - pointer to allocation that was leaked
    
    \Output None.

    \Version 02/15/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ZMemtrackPrintLeak(ZMemtrackElemT *pElem)
{
    static const char _hex[] = "0123456789ABCDEF";
    uint32_t uBytes;
    char strOutput[128];
    int32_t iOutput = 2;

    ZPrintf("Allocated: [%d] bytes at [0x%08x]\n", pElem->uMemSize, pElem->pMem);

    ZMemSet(strOutput, ' ', sizeof(strOutput)-1);
    strOutput[sizeof(strOutput)-1] = '\0';

    for (uBytes = 0; (uBytes < pElem->uMemSize) && (uBytes < ZMEMTRACK_MEMDUMPBYTES); uBytes++, iOutput += 2)
    {
        unsigned char cByte = ((unsigned char *)(pElem->pMem))[uBytes];
        strOutput[iOutput]   = _hex[cByte>>4];
        strOutput[iOutput+1] = _hex[cByte&0xf];
        strOutput[(iOutput/2)+40] = isprint(cByte) ? cByte : '.';
        if (uBytes > 0)
        {
            if (((uBytes+1) % 16) == 0)
            {
                strOutput[(iOutput/2)+40+1] = '\0';
                ZPrintf("%s\n", strOutput);
                ZMemSet(strOutput, ' ', sizeof(strOutput)-1);
                strOutput[sizeof(strOutput)-1] = '\0';
                iOutput = 0;
            }
            else if (((uBytes+1) % 4) == 0)
            {
                iOutput++;
            }
        }
    }

    if ((uBytes % ZMEMTRACK_MEMDUMPBYTES) != 0)
    {
        strOutput[(iOutput/2)+40+1] = '\0';
        ZPrintf("%s\n", strOutput);
    }

    ZPrintf("\n");
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ZMemtrackStartup

    \Description
        Start up the ZMemtracking module.

    \Input  None.
    
    \Output None.

    \Version 02/15/2005 (jbrookes)
*/
/********************************************************************************F*/
void ZMemtrackStartup(void)
{
    ZMemtrackRefT *pRef = &_ZMemtrack_Ref;
    
    ZMemSet(pRef, 0, sizeof(*pRef));
}


/*F********************************************************************************/
/*!
    \Function ZMemtrackShutdown

    \Description
        Shut down the ZMemtracking module.

    \Input  None.
    
    \Output None.

    \Version 02/15/2005 (jbrookes)
*/
/********************************************************************************F*/
void ZMemtrackShutdown(void)
{
    // dump the current status of the entire module
    ZMemtrackPrint(ZMEMTRACK_PRINTFLAG_TRACKING, 0, NULL);
}


/*F********************************************************************************/
/*!
    \Function ZMemtrackAlloc

    \Description
        Track an allocation.

    \Input *pMem    - pointer to allocated memory block
    \Input uSize    - size of allocated memory block
    
    \Output None.

    \Version 02/15/2005 (jbrookes)
*/
/********************************************************************************F*/
void ZMemtrackAlloc(void *pMem, uint32_t uSize, uint32_t uTag)
{
    ZMemtrackRefT *pRef = &_ZMemtrack_Ref;
    uint32_t uMemEntry;
    
    // now if we got the memory, add to the list
    if (pMem == NULL)
        return;

    // find a clear spot
    for (uMemEntry = 0; uMemEntry < ZMEMTRACK_MAXALLOCATIONS; uMemEntry++)
    {
        if (pRef->MemList[uMemEntry].pMem == NULL)
        {
            // get the memory location
            pRef->uTotalMemory += uSize;
            pRef->uNumAllocations += 1;
            pRef->uTotalAllocations += 1;
            
            // update high-water tracking
            if (pRef->uMaxAllocations < pRef->uNumAllocations)
            {
                pRef->uMaxAllocations = pRef->uNumAllocations;
            }
            if (pRef->uMaxMemory < pRef->uTotalMemory)
            {
                pRef->uMaxMemory = pRef->uTotalMemory;
            }
            
            // store the info
            pRef->MemList[uMemEntry].pMem = pMem;
            pRef->MemList[uMemEntry].uMemSize = uSize;
            pRef->MemList[uMemEntry].uTag = uTag;

            break;
        }
    }
    
    // check to see if we ran out of room to store this stuff
    if (uMemEntry == ZMEMTRACK_MAXALLOCATIONS)
    {
        pRef->bOverflow = 1;
    }
}


/*F********************************************************************************/
/*!
    \Function ZMemtrackFree

    \Description
        Track a free operation.

    \Input *pMem    - pointer to allocated memory block
    \Input *pSize   - [out] storage for memory block size
    
    \Output None.

    \Version 02/15/2005 (jbrookes)
*/
/********************************************************************************F*/
void ZMemtrackFree(void *pMem, uint32_t *pSize)
{
    ZMemtrackRefT *pRef = &_ZMemtrack_Ref;
    uint32_t uMemEntry;

    for (uMemEntry = 0, *pSize = 0; uMemEntry < ZMEMTRACK_MAXALLOCATIONS; uMemEntry++)
    {
        if (pRef->MemList[uMemEntry].pMem == pMem)
        {
            pRef->uTotalMemory -= pRef->MemList[uMemEntry].uMemSize;
            pRef->uNumAllocations -= 1;
            *pSize = pRef->MemList[uMemEntry].uMemSize;
            ZMemSet(&pRef->MemList[uMemEntry], 0, sizeof(pRef->MemList[uMemEntry]));
            break;
        }
    }
}


/*F********************************************************************************/
/*!
    \Function ZMemtrackPrint

    \Description
        Print overall memory info.

    \Input uFlags       - ZMemtrack_PRINTFLAG_*
    \Input uTag         - [optional] if non-zero, only display memory leaks stamped with this tag
    \Input *pModuleName - [optional] pointer to module name

    \Output None.

    \Version 02/15/2005 (jbrookes)
*/
/********************************************************************************F*/
void ZMemtrackPrint(uint32_t uFlags, uint32_t uTag, const char *pModuleName)
{
    ZMemtrackRefT *pRef = &_ZMemtrack_Ref;
    uint32_t uMemEntry;
    
    if (uFlags & ZMEMTRACK_PRINTFLAG_TRACKING)
    {
        ZPrintf("ZMemtrack:\n");
        ZPrintf("  Maximum number of Allocations at once: [%u]\n", pRef->uMaxAllocations);
        ZPrintf("  Current number of Allocations        : [%u]\n", pRef->uNumAllocations);
        ZPrintf("  Total number of Allocations Ever     : [%u]\n", pRef->uTotalAllocations);
        ZPrintf("  Maximum Memory Allocated             : [%u] bytes\n", pRef->uMaxMemory);
        ZPrintf("  Current Memory Allocated             : [%u] bytes\n", pRef->uTotalMemory);
        ZPrintf("\n");
    }
    
    if (pRef->bOverflow)
    {
        ZPrintf("WARNING: Allocation watcher overflowed!");
    }
    
    // see if there were any leaks
    for (uMemEntry = 0; uMemEntry < ZMEMTRACK_MAXALLOCATIONS; uMemEntry++)
    {
        ZMemtrackElemT *pElem = &pRef->MemList[uMemEntry];
        if ((pElem->pMem != NULL) && ((uTag == 0) || (pElem->uTag == uTag)))
        {
            break;
        }
    }
    
    // if there were leaks, display them
    if (uMemEntry != ZMEMTRACK_MAXALLOCATIONS)
    {
        if (pModuleName != NULL)
        {
            ZPrintf("\nZMemtrack: Detected %s memory leaks!\n\n", pModuleName);
        }
        else
        {
            ZPrintf("\nZMemtrack: Detected memory leaks!\n\n");
        }
        for ( ; uMemEntry < ZMEMTRACK_MAXALLOCATIONS; uMemEntry++)
        {
            ZMemtrackElemT *pElem = &pRef->MemList[uMemEntry];
            if ((pElem->pMem != NULL) && ((uTag == 0) || (pElem->uTag == uTag)))
            {
                _ZMemtrackPrintLeak(pElem);
            }
        }
    }
   
}
