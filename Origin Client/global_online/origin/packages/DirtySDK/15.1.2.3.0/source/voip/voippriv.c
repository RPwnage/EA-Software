/*H********************************************************************************/
/*!
    \File voippriv.c

    \Description
        Private functions for VoIP module use.

    \Copyright
        Copyright (c) Electronic Arts 2008. ALL RIGHTS RESERVED.

    \Version 1.0 11/26/2008 (mclouatre) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#ifdef _XBOX
#include <xtl.h>
#include <xonline.h>
#endif

#include "DirtySDK/platform.h"
#include "DirtySDK/voip/voipdef.h"
#include "voippriv.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

static int32_t _Voip_iMemGroup='dflt';
static void *_Voip_pMemGroupUserData=NULL;

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function VoipMemGroupQuery

    \Description
        Get VoIP mem group data.

    \Input *pMemGroup            - [OUT param] pointer to variable to be filled with mem group id
    \Input **ppMemGroupUserData  - [OUT param] pointer to variable to be filled with pointer to user data
    
    \Output
        None.

    \Version 1.0 11/11/2005 (jbrookes) First Version
    \Version 1.1 11/18/2008 (mclouatre) returned values now passed in [OUT] parameters
*/
/********************************************************************************F*/
void VoipMemGroupQuery(int32_t *pMemGroup, void **ppMemGroupUserData)
{
    *pMemGroup = _Voip_iMemGroup;
    *ppMemGroupUserData = _Voip_pMemGroupUserData;
}

/*F********************************************************************************/
/*!
    \Function VoipMemGroupSet

    \Description
        Set VoIP mem group data.

    \Input iMemGroup             - mem group to set
    \Input *pMemGroupUserData    - user data associated with mem group
    
    \Output
        None.

    \Version 1.0 11/11/2005 (jbrookes) First Version
    \Version 1.1 11/18/2008 (mclouatre) Adding second parameter (user data)
*/
/********************************************************************************F*/
void VoipMemGroupSet(int32_t iMemGroup, void *pMemGroupUserData)
{
    _Voip_iMemGroup = iMemGroup;
    _Voip_pMemGroupUserData = pMemGroupUserData;
}

