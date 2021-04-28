/*H********************************************************************************/
/*!
    \File zmemtrack.h

    \Description
        Routines for tracking memory allocations.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 02/15/2005 (jbrookes) First Version, based on jfrank's implementation.
*/
/********************************************************************************H*/

#ifndef _zmemtrack_h
#define _zmemtrack_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

#define ZMEMTRACK_PRINTFLAG_TRACKING        (1)     //!< print more verbose output

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// init Zmemtrack module
void ZMemtrackStartup(void);

// shut down Zmemtrack module
void ZMemtrackShutdown(void);

// track an allocation
void ZMemtrackAlloc(void *pMem, uint32_t uSize, uint32_t uTag);

// track a free operation
void ZMemtrackFree(void *pMem, uint32_t *pSize);

// print current tracking info
void ZMemtrackPrint(uint32_t uFlags, uint32_t uTag, const char *pModuleName);

#ifdef __cplusplus
};
#endif

#endif // _zmemtrack_h

