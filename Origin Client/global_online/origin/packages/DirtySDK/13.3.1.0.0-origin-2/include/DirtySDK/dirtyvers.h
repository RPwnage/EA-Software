/*H********************************************************************************/
/*!
    \File dirtyvers.h

    \Description
        DirtySock SDK version.

    \Copyright
        Copyright (c) 2003-2005 Electronic Arts Inc.

    \Version 06/13/2003 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _dirtyvers_h
#define _dirtyvers_h

/*!
\Module DirtySock
*/
//@{

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

#define DIRTYSDK_VERSION_MAKE(year, season, major, minor, patch) (((year) * 100000000) + ((season) * 1000000) + ((major) * 10000) + ((minor) * 100) + (patch))

#define DIRTYSDK_VERSION_YEAR   (13)
#define DIRTYSDK_VERSION_SEASON (3)
#define DIRTYSDK_VERSION_MAJOR  (1)
#define DIRTYSDK_VERSION_MINOR  (0)
#define DIRTYSDK_VERSION_PATCH  (0)
#define DIRTYSDK_VERSION        (DIRTYSDK_VERSION_MAKE(DIRTYSDK_VERSION_YEAR, DIRTYSDK_VERSION_SEASON, DIRTYSDK_VERSION_MAJOR, DIRTYSDK_VERSION_MINOR, DIRTYSDK_VERSION_PATCH))
#define DIRTYVERS               (DIRTYSDK_VERSION)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

//@}

#endif // _dirtyvers_h

