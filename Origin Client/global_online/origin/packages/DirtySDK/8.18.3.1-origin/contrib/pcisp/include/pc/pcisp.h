/*H*************************************************************************************************/
/*!

    \File    pcisp.h

    \Description
        High-level module for reading the PC account login info from the registry.

    \Notes
        None

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2005.  ALL RIGHTS RESERVED.

    \Version 1.0 09/15/2005 (dclark) First Version
*/
/*************************************************************************************************H*/

#ifndef _pcisp_h
#define _pcisp_h

/*** Include files *********************************************************************/

/*** Defines ***************************************************************************/

//! alert styles
#define PCISP_ALERTFLAG_OK  (1)

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! alerts
typedef enum PCIspAlertE
{
    PCISP_ALERT_MISSING = 0,    //!< no account information was found
    PCISP_ALERT_ACCESS,         //!< access denied to registry entry
    PCISP_ALERT_REGERROR,       //!< problem reading account data from registry

    PCISP_NUMALERTS             //!< number of alerts
} PCIspAlertE;

//! current context of module
typedef enum PCIspContextE
{
    PCISP_CONTEXT_INACTIVE = 0, //!< starting PCIsp
    PCISP_CONTEXT_LOADACCOUNT,  //!< read account information from the registry
    PCISP_CONTEXT_DONE,         //!< PCIsp process is complete

    PCISP_NUMCONTEXTS           //!< number of PCIsp context states
} PCIspContextE;

//! current status of module
typedef enum PCIspStatusE
{
    PCISP_STATUS_IDLE = 0,      //!< no activity
    PCISP_STATUS_ALERT,         //!< need to show an alert
    PCISP_STATUS_GOTO,          //!< need to go to a different screen

    PCISP_NUMSTATUSSTATES       //!< number of PCIsp status states
} PCIspStatusE;

//! alert message structure
typedef struct PCIspAlertT
{
    int32_t uFlags;             //!< alert flags
    const char *pTitle;     //!< alert title text
    const char *pBody;      //!< alert body text
    const char *pFooter;    //!< alert footer text
} PCIspAlertT;

//! opaque module type
typedef struct PCIspT PCIspT;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module state
PCIspT *PCIspCreate(const PCIspAlertT *pAlertList);

// destroy the module state
void PCIspDestroy(PCIspT *pIsp);

// get parameters for current screen
const char *PCIspParams(PCIspT *pIsp, PCIspContextE eContext);

// return current alert message reference (call in response to PCISP_STATUS_ALERT)
const PCIspAlertT *PCIspAlert(PCIspT *pIsp, PCIspContextE eContext);

// return the current alert message enum
PCIspAlertE PCIspGetAlertEnum(PCIspT *pIsp, const PCIspAlertT *pAlert);

// get the current status
PCIspStatusE PCIspGetStatus(PCIspT * pIsp, PCIspContextE eContext);

// set a different context
void PCIspSetContext(PCIspT *pIsp, PCIspContextE eContext);

// get the current context
PCIspContextE PCIspGetContext(PCIspT *pIsp);

// returns last registry error code
int32_t PCIspGetError(PCIspT *pIsp);

#ifdef __cplusplus
}
#endif

#endif // _pcisp_h
