/*H********************************************************************************/
/*!
    \File qosservice.h

    \Description
        This module handles communication with the QOS coordinator from the QOS server.

    \Copyright
        Copyright (c) Electronic Arts 2015. ALL RIGHTS RESERVED.

    \Version 10/23/2016 (cvienneau)
*/
/********************************************************************************H*/

#ifndef _qosservice_h
#define _qosservice_h

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Type Definitions *************************************************************/

//! opaque module ref
typedef struct QosServiceRefT QosServiceRefT;

//! forward declaration
typedef struct TokenApiRefT TokenApiRefT;
typedef struct QosCommonServerToCoordinatorRequestT QosCommonServerToCoordinatorRequestT;
/*** Functions ********************************************************************/

// create the module
QosServiceRefT *QosServiceCreate(TokenApiRefT *pTokenApi, const char *pCertFile, const char *pKeyFile);

// update the module
void QosServiceUpdate(QosServiceRefT *pQosService);

// control function
int32_t QosServiceControl(QosServiceRefT *pQosService, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// status function
int32_t QosServiceStatus(QosServiceRefT *pQosService, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize);

// registers this instance of QOS server with the QOS coordinator
int32_t QosServiceRegister(QosServiceRefT *pQosService, const QosCommonServerToCoordinatorRequestT *pRegistrationInfo);

// destroy the module
void QosServiceDestroy(QosServiceRefT *pQosService);

#endif // _qosservice_h
