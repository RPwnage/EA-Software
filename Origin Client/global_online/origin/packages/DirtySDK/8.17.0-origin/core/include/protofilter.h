/*H********************************************************************************/
/*!
    \File protofilter.h

    \Description
        Encapsulate the profanity filter logic into one place. Especially for the
        Xenon profanity filter.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 03/10/2006 (tdills) First Version
*/
/********************************************************************************H*/

#ifndef _protofilter_h_
#define _protofilter_h_

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

#define PROTOFILTER_VIRTUOUS        (0)
#define PROTOFILTER_PROFANE         (1)

#define PROTOFILTER_ERROR_PENDING   (-1)
#define PROTOFILTER_ERROR_FULL      (-2)
#define PROTOFILTER_INVALID_ID      (-3)
#define PROTOFILTER_ERROR_FAILED    (-4)

#define PROTOFILTER_TYPE_NONE       (0)
#define PROTOFILTER_TYPE_SQUELCH    (1)
#define PROTOFILTER_TYPE_PROFANE    (2)

#define PROTOFILTER_MAX_FILTEROPS   (2) //!< maximum number of concurrent filter operations allowed

#define PROTOFILTER_CALLBACKOP_FILTER   (1)
#define PROTOFILTER_CALLBACKOP_RESULT   (2)
#define PROTOFILTER_CALLBACKOP_CANCEL   (3)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! opaque module state
typedef struct ProtoFilterRefT ProtoFilterRefT;

//! filter control block, used by secondary filter providers
typedef struct ProtoFilterCtlT
{
    int32_t iFilterId;
    uint32_t uFilterOp;
    const char *pFilterText;
} ProtoFilterCtlT;

//! filter callback prototype
typedef int32_t (ProtoFilterCallbackT)(ProtoFilterRefT *pFilterRef, ProtoFilterCtlT *pFilterCtl, void *pUserData);

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create module
ProtoFilterRefT *ProtoFilterCreate(void);

// destroy module
void ProtoFilterDestroy(ProtoFilterRefT *pFilterRef);

// send profanity filter request
int32_t ProtoFilterRequest(ProtoFilterRefT *pFilterRef, const char *pData, uint32_t *pFilterReqId);

// cancel an outstanding request
void ProtoFilterRequestCancel(ProtoFilterRefT *pFilterRef, int32_t iFilterReqId);

// get profanity filter results
int32_t ProtoFilterResult(ProtoFilterRefT *pFilterRef, uint32_t uFilterReqId);

// apply a profanity filter to a string
void ProtoFilterApplyFilter(ProtoFilterRefT *pFilterRef, uint32_t uFilterType, char *pData);

// set the overrideable profane string
int32_t ProtoFilterControl(ProtoFilterRefT *pFilterRef, int32_t iControl, int32_t iValue, int32_t iValue2, const void *pValue);

// set protofilter callback function (used to override internal filtering operation)
void ProtoFilterCallback(ProtoFilterRefT *pFilterRef, ProtoFilterCallbackT *pFilterCb, void *pUserData);

#ifdef __cplusplus
}
#endif

#endif // _protofilter_h_

