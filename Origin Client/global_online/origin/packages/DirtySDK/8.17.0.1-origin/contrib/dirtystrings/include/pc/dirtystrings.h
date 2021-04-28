/*H*************************************************************************************************/
/*!

    \File    dirtystrings.h

    \Description
        Main included file for "auxiliary" string table definitions that may optionally
        be used by an application if desired.
        
    \Notes
        The error strings in this library are implemented in seperate source files and
        are designed to be externally referenced by the application on an as-needed basis.
        This way the strings that are not required are not linked into the application.

        All strings are stored in UTF-8 format.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2003.  ALL RIGHTS RESERVED.

    \Version    1.0        12/12/2003 (JLB) First Version
*/
/*************************************************************************************************H*/


#ifndef _dirtystrings_h
#define _dirtystrings_h

/*** Include files *********************************************************************/

#include "pcisp.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Variables *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

//!< PCIsp/English alert table
extern const PCIspAlertT      _PCIsp_AlertTable_En[PCISP_NUMALERTS];

#ifdef __cplusplus
}
#endif

/*** Functions *************************************************************************/

#endif // _dirtystrings_h
