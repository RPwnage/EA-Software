/*H********************************************************************************/
/*!
    \File subspaceapi.h

    \Description
        Module for interacting with the subspace sidekick application.  The subspace
        network is a means of improving latency by using improved, private routes,
        that are not typically available over the internet.  The subspace sidekick
        application is used to setup NAT to gain access to the subspace network.

    \Copyright
        Copyright (c) 2020 Electronic Arts Inc.

    \Version 04/24/2020 (cvienneau) First Version
*/
/********************************************************************************H*/

#ifndef _subspaceapi_h
#define _subspaceapi_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! opaque module state
typedef struct SubspaceApiRefT SubspaceApiRefT;

typedef enum SubspaceApiStateE
{
    SUBSPACE_STATE_IDLE = 0,
    SUBSPACE_STATE_IN_PROGRESS,
    SUBSPACE_STATE_DONE,
    SUBSPACE_STATE_ERROR
} SubspaceApiStateE;

//! distinguishes between the type of request we are making
typedef enum SubspaceApiRequestTypeE
{
    SUBSPACE_REQUEST_TYPE_NONE = 0,
    SUBSPACE_REQUEST_TYPE_PROVISION,
    SUBSPACE_REQUEST_TYPE_STOP
} SubspaceApiRequestTypeE;

typedef struct SubspaceApiResponseT
{
    SubspaceApiStateE eSubspaceState;
    SubspaceApiRequestTypeE eRequestType;
    uint32_t uResultAddr;
    uint16_t uResultPort;
    uint8_t uPad[2];
}SubspaceApiResponseT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module
SubspaceApiRefT *SubspaceApiCreate(const char *pSubspaceSidekickAddr, uint16_t uSubspaceSidekickPort);

// destroy the module
void SubspaceApiDestroy(SubspaceApiRefT *pSubspaceApi);

// update the module
void SubspaceApiUpdate(SubspaceApiRefT *pSubspaceApi);

// begin a request to provision/stop subspace ip/port
int32_t SubspaceApiRequest(SubspaceApiRefT *pSubspaceApi, SubspaceApiRequestTypeE eRequestType, uint32_t uServerIp, uint16_t uServerPort, char* strServerUniqueID);

// check on the status and get results from current request
SubspaceApiResponseT SubspaceApiResponse(SubspaceApiRefT* pSubspaceApi);

// control function
int32_t SubspaceApiControl(SubspaceApiRefT *pSubspaceApi, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// status function
int32_t SubspaceApiStatus(SubspaceApiRefT *pSubspaceApi, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize);


#ifdef __cplusplus
};
#endif

#endif // _subspaceapi_h

