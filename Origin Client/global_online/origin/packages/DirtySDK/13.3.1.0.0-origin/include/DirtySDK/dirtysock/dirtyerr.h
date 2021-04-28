/*H********************************************************************************/
/*!
    \File dirtyerr.h

    \Description
        Dirtysock debug error routines.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 06/13/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _dirtyerr_h
#define _dirtyerr_h

/*** Include files ****************************************************************/

// included for DIRTYCODE_DEBUG definition
#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

#define DIRTYSOCK_ERRORNAMES    (DIRTYCODE_LOGGING && TRUE)
#define DIRTYSOCK_LISTTERM      (0x45454545)

/*** Macros ***********************************************************************/

#if DIRTYSOCK_ERRORNAMES
#define DIRTYSOCK_ErrorName(_iError)    { (uint32_t)_iError, #_iError }
#define DIRTYSOCK_ListEnd()             { DIRTYSOCK_LISTTERM, "" }
#endif

/*** Type Definitions *************************************************************/

typedef struct DirtyErrT
{
    uint32_t uError;
    const char  *pErrorName;
} DirtyErrT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// take a system-specific error code, and either resolve it to its define name or format it as a hex number
void DirtyErrName(char *pBuffer, int32_t iBufSize, uint32_t uError);

// same as DirtyErrName, but references the specified list
void DirtyErrNameList(char *pBuffer, int32_t iBufSize, uint32_t uError, const DirtyErrT *pList);

// same as DirtyErrName, except a pointer is returned
const char *DirtyErrGetName(uint32_t uError);

// same as DirtyErrGetName, but references the specified list
const char *DirtyErrGetNameList(uint32_t uError, const DirtyErrT *pList);

#ifdef __cplusplus
}
#endif

#endif // _dirtyerr_h

