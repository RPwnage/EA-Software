/*H********************************************************************************/
/*!
    \File dirtylspxenon.h

    \Description
        Xenon XLSP client implementation.

    \Copyright
        Copyright (c) 2006-2010 Electronic Arts Inc.

    \Version 03/13/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _dirtylspxenon_h
#define _dirtylspxenon_h

/*** Include files ****************************************************************/
#include <xtl.h>

/*** Defines **********************************************************************/

//! maximum length of UserData string
#define DIRTYLSP_USERDATA_MAXSIZE   (64)

//! lsp query states
typedef enum DirtyLSPQueryStateE
{
    DIRTYLSP_QUERYSTATE_FAIL = -1,      //!< xlsp request failed
    DIRTYLSP_QUERYSTATE_RSLV,           //!< xlsp resolve is in progress
    DIRTYLSP_QUERYSTATE_INIT,           //!< connection to xlsp server is to be initiated
    DIRTYLSP_QUERYSTATE_CONN,           //!< connection to xlsp server is in progress
    DIRTYLSP_QUERYSTATE_DONE,           //!< connection to xlsp server has been established
} DirtyLSPQueryStateE;

//! lsp notify types
typedef enum DirtyLSPCacheEventE
{
    DIRTYLSP_CACHEEVENT_NONE = 0,       //!< placeholder
    DIRTYLSP_CACHEEVENT_EXPIRED,        //!< cache entry has expired
} DirtyLSPCacheEventE;

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! lsp query record
typedef struct DirtyLSPQueryRecT
{
    char                strName[DIRTYLSP_USERDATA_MAXSIZE]; //!< lsp name to resolve
    uint32_t            uAddress;           //!< zero if resolve is pending; else secure address
    DirtyLSPQueryStateE eQueryState;        //!< current query state
    uint32_t            uQueryError;        //!< error code if query has failed

    // private data

    struct DirtyLSPQueryRecT *pNext;        //!< next query in query list
    HANDLE              hEnumerator;        //!< query enumerator
    IN_ADDR             InAddr;             //!< address in IN_ADDR form
    XOVERLAPPED         XOverlapped;        //!< query xoverlapped structure
    uint8_t             bCleanup;           //!< set to TRUE if was DirtyLSPQueryFree() called while in RSLV state
    XTITLE_SERVER_INFO  aServerInfo[1];     //!< variable-length query buffer (must come last)
} DirtyLSPQueryRecT;

//! module state ref
typedef struct DirtyLSPRefT DirtyLSPRefT;

//! cache callback (internal use only)
typedef void (DirtyLSPCacheCbT)(uint32_t uAddress, DirtyLSPCacheEventE eCacheEvent, void *pUserRef);

//! optional notification callback
typedef void (DirtyLSPNotifyCbT)(DirtyLSPRefT *pDirtyLSP, const DirtyLSPQueryRecT *pQueryRec, void *pUserRef);


/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module
DirtyLSPRefT *DirtyLSPCreate(uint32_t uMaxQueryCount);

// set optional notification callback
void DirtyLSPCallback(DirtyLSPRefT *pDirtyLSP, DirtyLSPNotifyCbT *pNotifyCb, void *pUserRef);

// destroy the module
void DirtyLSPDestroy(DirtyLSPRefT *pDirtyLSP);

// initiate an lsp query
DirtyLSPQueryRecT *DirtyLSPQueryStart(DirtyLSPRefT *pDirtyLSP, const char *pQueryStr);

// initiate an lsp query
DirtyLSPQueryRecT *DirtyLSPQueryStart2(DirtyLSPRefT *pDirtyLSP, const char *pQueryStr, uint32_t uMaxResponses);

// free a query
void DirtyLSPQueryFree(DirtyLSPRefT *pDirtyLSP, DirtyLSPQueryRecT *pQueryRec);

// check query for completion
int32_t DirtyLSPQueryDone(DirtyLSPRefT *pDirtyLSP, DirtyLSPQueryRecT *pQueryRec);

// updates the specified connection (must be polled during use to prevent timeout)
void DirtyLSPConnectionUpdate(uint32_t uAddress);

// close the specified connection [DEPRECATED]
int32_t DirtyLSPConnectionClose(DirtyLSPRefT *pDirtyLSP, uint32_t uAddress);

// control module behavior
int32_t DirtyLSPControl(DirtyLSPRefT *pDirtyLSP, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue);

// get module status
int32_t DirtyLSPStatus(DirtyLSPRefT *pDirtyLSP, int32_t iSelect, void *pBuffer, int32_t iBufSize);

#ifdef __cplusplus
}
#endif

#endif // _dirtylspxenon_h

