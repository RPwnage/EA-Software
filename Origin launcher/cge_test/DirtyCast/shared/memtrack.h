/*H********************************************************************************/
/*!
    \File memtrack.h

    \Description
        Routines for tracking memory allocations.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 02/15/2005 (jbrookes) First Version, based on jfrank's implementation.
*/
/********************************************************************************H*/

#ifndef _memtrack_h
#define _memtrack_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

#define MEMTRACK_PRINTFLAG_TRACKING        (1)     //!< print more verbose output

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// init memtrack module
void MemtrackStartup(void);

// shut down memtrack module
void MemtrackShutdown(void);

// track an allocation
void MemtrackAlloc(void *pMem, uint32_t uSize, uint32_t uTag);

// track a free operation
void MemtrackFree(void *pMem, uint32_t *pSize);

// print current tracking info
void MemtrackPrint(uint32_t uFlags, uint32_t uTag, const char *pModuleName);

#ifdef __cplusplus
};
#endif

#endif // _memtrack_h

