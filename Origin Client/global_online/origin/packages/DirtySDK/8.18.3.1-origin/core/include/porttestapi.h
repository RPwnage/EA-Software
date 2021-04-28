/*H********************************************************************************/
/*!
    \File porttestapi.c

    \Description
        PortTest is used for testing port connectivity between a central server
        and a client using UDP packets.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/09/2005 (jfrank) First Version
*/
/********************************************************************************H*/

#ifndef _porttestapi_h
#define _porttestapi_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

#define PORTTESTER_ERROR_NONE                     (0)   //!< no error to report
#define PORTTESTER_ERROR_BIND                    (-1)   //!< error binding the socket(s)
#define PORTTESTER_ERROR_BADPTR                  (-2)   //!< bad ref pointer used
#define PORTTESTER_ERROR_BUSY                    (-3)   //!< busy - operation cannot be completed

/*** Type Definitions *************************************************************/

// module state
typedef struct PortTesterT PortTesterT;

// callback prototype used for status updates, if enabled
typedef void (PortTesterCallbackT)(PortTesterT *pRef, int32_t iStatus, void *pUserData);

/*** Variables ********************************************************************/

//! internal module states
typedef enum PortTesterStateE
{
    PORTTESTER_STATUS_IDLE = 0,         //!< waiting for something to do
    PORTTESTER_STATUS_SENDING,          //!< sending packets to the server
    PORTTESTER_STATUS_SENT,             //!< packets sent, waiting for response(s)
    PORTTESTER_STATUS_RECEIVING,        //!< some or all packets have been received
    PORTTESTER_STATUS_SUCCESS,          //!< all packets received successfully
    PORTTESTER_STATUS_PARTIALSUCCESS,   //!< some of the packets received successfully
    PORTTESTER_STATUS_TIMEOUT,          //!< a timeout occurred before all packets were received
    PORTTESTER_STATUS_ERROR,            //!< an error occured
    PORTTESTER_STATUS_UNKNOWN
} PortTesterStateE;

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create a port tester to talk to a server on the specified port
PortTesterT *PortTesterCreate(int32_t iServerIP, int32_t iServerPort, int32_t iTimeout);

// connect a PortTester
void PortTesterConnect(PortTesterT *pRef, void* unused);

// return info based on input selector
int32_t PortTesterStatus(PortTesterT *pRef, int32_t iSelect, void *pBuf, int32_t iBufSize);

// control module behavior
int32_t PortTesterControl(PortTesterT *pRef, int32_t iSelect, int32_t iValue);

// test a specified port based on the information already provided and use callback if non-NULL
int32_t PortTesterTestPort(PortTesterT *pRef, int32_t iPort, int32_t iNumPackets, PortTesterCallbackT *pCallback, void *pUserData);

// send a UDP packet to server over the given port
int32_t PortTesterOpenPort(PortTesterT *pRef, int32_t iPort);

// disconnect a PortTester
void PortTesterDisconnect(PortTesterT *pRef);

// destroy a PortTester module
void PortTesterDestroy(PortTesterT *pRef);

#ifdef __cplusplus
};
#endif

#endif // _porttestapi_h

