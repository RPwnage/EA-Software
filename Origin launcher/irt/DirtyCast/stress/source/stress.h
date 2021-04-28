/*H********************************************************************************/
/*!
    \File stress.h

    \Description
        This module implements the main logic for the Stress client.
        
        Description forthcoming.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 03/24/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _stress_h
#define _stress_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

// shutdown flags
#define STRESS_SHUTDOWN_IMMEDIATE   (1)
#define STRESS_SHUTDOWN_IFEMPTY     (2)

/*** Type Definitions *************************************************************/

//! opaque module ref
typedef struct StressRefT StressRefT;

/*** Functions ********************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

//! stress client printf
void StressPrintf(const char *pFormat, ...);

//! initialize this instance from the provided config file.
StressRefT *StressInitialize(int32_t iArgc, const char *pArgv[], const char *pConfigFile);

//! shut this instance down
void StressShutdown(StressRefT *pStress);

//! main process loop
bool StressProcess(StressRefT *pStress, uint32_t *pSignalFlags);

//! status selector
int32_t StressStatus(StressRefT *pStress, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufLen);

#if defined(__cplusplus)
}
#endif

#endif // _stress_h

