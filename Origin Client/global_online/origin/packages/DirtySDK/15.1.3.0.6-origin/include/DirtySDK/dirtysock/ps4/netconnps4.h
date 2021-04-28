/*H********************************************************************************/
/*!
    \File netconnps4.h

    \Description
        Definitions for the netconnps4 module.

    \Copyright
        Copyright (c) 2013 Electronic Arts

    \Version 08/06/2013 (tcho) First Version
*/
/********************************************************************************H*/

#ifndef _netconnps4_h
#define _netconnps4_h

/*!
\Moduledef NetConnPS4 NetConnPS4
\Modulemember DirtySock
*/
//@{

/*** Include files ****************************************************************/

#include <np/np_common.h>

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct NetConnNpUserStatusT
{
    int32_t         iUserId;        //user id (this is not the user index)   // $todo - change this field to -->  SceUserServiceUserId sceUserServiceUserId;
    SceNpId         npId;           //user's np id
    SceNpState      state;          //user state
} NetConnNpUserStatusT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

//@}

#endif // _netconnps4_h
