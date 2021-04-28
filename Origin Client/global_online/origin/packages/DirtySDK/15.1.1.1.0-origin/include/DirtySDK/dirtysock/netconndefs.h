/*H********************************************************************************/
/*!
    \File netconndefs.h

    \Description
        Definitions for the netconn module.

    \Copyright
        Copyright (c) 2005-2009 Electronic Arts Inc.

    \Version 09/29/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _netconndefs_h
#define _netconndefs_h

/*!
\Moduledef NetConnDefs NetConnDefs
\Modulemember DirtySock
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

// interface types, returned by NetConnStatus('type')
#define NETCONN_IFTYPE_NONE     (1)         //!< indeterminate interface type
#define NETCONN_IFTYPE_MODEM    (2)         //!< interface is a modem
#define NETCONN_IFTYPE_ETHER    (4)         //!< interface is ethernet
#define NETCONN_IFTYPE_USB      (8)         //!< interface bus type is USB
#define NETCONN_IFTYPE_PPPOE    (16)        //!< interface is PPPoE
#define NETCONN_IFTYPE_WIRELESS (32)        //!< interface is wireless (wifi)
#define NETCONN_IFTYPE_CELL     (64)        //!< interface is cellular

// EA back-end environment types, returned by NetConnStatus('envi')
#define NETCONN_PLATENV_DEV     (8)         //!< Dev environment - Note (0) is used by the 'envi' NetConnStatus selector to indicate ~inp/try again
#define NETCONN_PLATENV_TEST    (1)         //!< Test environment
#define NETCONN_PLATENV_CERT    (2)         //!< Certification environment
#define NETCONN_PLATENV_PROD    (4)         //!< Production environment

// uShutdownFlags
#define NETCONN_SHUTDOWN_NETACTIVE      (NET_SHUTDOWN_NETACTIVE)     //!< leave network active in preparation for launching to account management (Xbox 360 only)
#define NETCONN_SHUTDOWN_THREADSTARVE   (NET_SHUTDOWN_THREADSTARVE)  //!< special shutdown mode for PS3 that starves threads, allowing for quick exit to XMB

// generic netconn error responses
#define NETCONN_ERROR_ISACTIVE    (-1)      //!< the module is currently active
#define NETCONN_ERROR_NOTACTIVE   (-2)      //!< the module isn't currently active

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! network configuration entry
typedef void * NetConfigRecT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

//@}

#endif // _netconndefs_h

