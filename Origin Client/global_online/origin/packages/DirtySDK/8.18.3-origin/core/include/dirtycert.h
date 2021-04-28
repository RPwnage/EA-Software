/*H********************************************************************************/
/*!
    \File dirtycert.h

    \Description
        This module defines the CA fallback mechanism which is used by ProtoSSL.

    \Copyright
        Copyright (c) 2012 Electronic Arts Inc.

    \Version 01/23/2012 (szhu)
*/
/********************************************************************************H*/

#ifndef _dirtycert_h
#define _dirtycert_h

/*** Include files *********************************************************************/
#include "protossl.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

// opaque module state ref
typedef struct DirtyCertRefT DirtyCertRefT;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create dirtycert module
int32_t DirtyCertCreate(void);

// release resources and destroy module
int32_t DirtyCertDestroy(void);

// initiate a CA fetch request
int32_t DirtyCertCARequestCert(const ProtoSSLCertInfoT *pCertInfo);

// initiate a CA prefetch request
void DirtyCertCAPreloadCerts(const char *pServiceName);

// if a CA fetch request is complete
int32_t DirtyCertCARequestDone(int32_t iRequestId);

// release resources used by a CA fetch request
int32_t DirtyCertCARequestFree(int32_t iRequestId);

// control module behavior
int32_t DirtyCertControl(int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

#ifdef __cplusplus
}
#endif

#endif // _dirtycert_h
