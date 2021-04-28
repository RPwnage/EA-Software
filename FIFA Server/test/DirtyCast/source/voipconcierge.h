/*H********************************************************************************/
/*!
    \File voipconcierge.h

    \Description
        This contains the functions used to implement the Connection
        Concierge mode for the voipserver
        
    \Copyright
        Copyright (c) 2015 Electronic Arts Inc.

    \Version 09/24/2015 (eesponda)
*/
/********************************************************************************H*/

#ifndef _voipconcierge_h
#define _voipconcierge_h

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/
#define VOIPCONCIERGE_CCS_PRODUCTID_SIZE (64)

/*** Type Definitions *************************************************************/

//! opaque module ref
typedef struct VoipConciergeRefT VoipConciergeRefT;

//! forward declaration
typedef struct VoipServerRefT VoipServerRefT;

//! entry in the product map internally maintained by voipconcierge (can be inspected with 'prod' status selector)
typedef struct ConciergeProductMapEntryT
{
    char    strProductId[VOIPCONCIERGE_CCS_PRODUCTID_SIZE];
    int32_t iClientCount;                       //!< count of active clients for this product
} ConciergeProductMapEntryT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

// create the module
VoipConciergeRefT *VoipConciergeInitialize(VoipServerRefT *pVoipServer, const char *pCommandTags, const char *pConfigTags);

// update the module
uint8_t VoipConciergeProcess(VoipConciergeRefT *pVoipConcierge, uint32_t *pSignalFlags, uint32_t uCurTick);

// get the base to access the shared functionality
VoipServerRefT *VoipConciergeGetBase(VoipConciergeRefT *pVoipConcierge);

// status function
int32_t VoipConciergeStatus(VoipConciergeRefT *pVoipConcierge, int32_t iStatus, int32_t iValue, void *pBuf, int32_t iBufSize);

// control function
int32_t VoipConciergeControl(VoipConciergeRefT *pVoipConcierge, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// drain the module
uint32_t VoipConciergeDrain(VoipConciergeRefT *pRef);

// destroy the module
void VoipConciergeShutdown(VoipConciergeRefT *pVoipConcierge);

#endif // _voipconcierge_h
