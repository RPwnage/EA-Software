/*H********************************************************************************/
/*!
    \File conciergeservice.h

    \Description
        This module handles communication with the Connection Concierge Service (CCS)
        The CCS maintains the registry of all running instances of DirtyCast servers

    \Copyright
        Copyright (c) Electronic Arts 2015. ALL RIGHTS RESERVED.

    \Version 10/23/2015 (eesponda)
*/
/********************************************************************************H*/

#ifndef _conciergeservice_h
#define _conciergeservice_h

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Type Definitions *************************************************************/

//! opaque module ref
typedef struct ConciergeServiceRefT ConciergeServiceRefT;

//! forward declaration
typedef struct TokenApiRefT TokenApiRefT;

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module
ConciergeServiceRefT *ConciergeServiceCreate(TokenApiRefT *pTokenApi, const char *pCertFile, const char *pKeyFile);

// update the module
void ConciergeServiceUpdate(ConciergeServiceRefT *pConciergeService, uint32_t uCapacity);

// control function
int32_t ConciergeServiceControl(ConciergeServiceRefT *pConciergeService, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// status function
int32_t ConciergeServiceStatus(ConciergeServiceRefT *pConciergeService, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize);

// registers this instance of dirtycast with the CCS
int32_t ConciergeServiceRegister(ConciergeServiceRefT *pConciergeService, const char *pPingSite, const char *pPool, const char *pApiBase, const char *pVersions);

// destroy the module
void ConciergeServiceDestroy(ConciergeServiceRefT *pConciergeService);

#ifdef __cplusplus
}
#endif

#endif // _conciergeservice_h
