/*H********************************************************************************/
/*!
    \File qosstressmetrics.h

    \Description
        Qos bot metrics reporting system.

    \Copyright
        Copyright (c) 2017 Electronic Arts Inc.

    \Version 05/02/2017 (cvienneau)
*/
/********************************************************************************H*/

#ifndef _qosstressmetrics_h
#define _qosstressmetrics_h

/*** Include files ****************************************************************/
#include "qosstressconfig.h"
#include "DirtySDK/misc/qosclient.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! forward declaration
typedef struct ServerMetricsT ServerMetricsT;
typedef struct QosStressMetricsRefT QosStressMetricsRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

//create this helper module to manage metrics
QosStressMetricsRefT *QosStressMetricsCreate(QosStressConfigC* mpConfig);

//take the results and apply them to internal state, to be shared with metrics agent at next push
void QosStressMetricsAccumulate(QosStressMetricsRefT* pMetrics, QosCommonProcessedResultsT* pResults);

void QosStressMetricsUpdate(QosStressMetricsRefT* pMetrics);

//destroy this module
void QosStressMetricsDestroy(QosStressMetricsRefT *pMetrics);


#if defined(__cplusplus)
}
#endif

#endif // _qosmetrics_h


