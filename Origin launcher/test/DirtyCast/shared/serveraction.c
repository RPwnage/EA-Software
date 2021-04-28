/*H********************************************************************************/
/*!
    \File serveraction.h

    \Description
        HTTP diagnostic page actions.

    \Copyright
        Copyright (c) 2010 Electronic Arts Inc.

    \Version 04/27/2010 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "DirtySDK/platform.h"


#include "dirtycast.h"
#include "serveraction.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ServerActionParse

    \Description
        Parse an action name and value pair.

    \Input *pStrCommand     - command string to parse name/value pair from
    \Input *pAction         - [out] storage for parsed action
    
    \Output
        int32_t             - negative=failure, else success

    \Version 10/15/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t ServerActionParse(const char *pStrCommand, ServerActionT *pAction)
{
    const char *pEquals;

    // make sure we have an action string
    if ((pStrCommand == NULL) || (strncmp(pStrCommand, "/action?", 8) != 0))
    {
        return(-1);
    }

    // skip to command
    pStrCommand += 8;
    // make sure we have an assignment operator
    if ((pEquals = strstr(pStrCommand, "=")) == NULL)
    {
        return(-1);
    }

    // parse action name
    ds_strsubzcpy(pAction->strName, sizeof(pAction->strName), pStrCommand, pEquals - pStrCommand);
    // parse action value
    ds_strnzcpy(pAction->strData, pEquals+1, sizeof(pAction->strData));
    return(0);
}

/*F********************************************************************************/
/*!
    \Function ServerActionParseInt

    \Description
        Parse an action integer value, with range validation.

    \Input *pBuffer         - output for response text
    \Input *iBufSize        - size of response text buffer
    \Input *pAction         - action to process  
    \Input iMinValue        - min value to allow
    \Input iMaxValue        - max value to allow  
    \Input *pOutValue       - [out] pointer to variable to update

    \Output
        int32_t             - updated response buffer offset    

    \Version 04/27/2010 (jbrookes)
*/
/********************************************************************************F*/
int32_t ServerActionParseInt(char *pBuffer, int32_t iBufSize, const ServerActionT *pAction, int32_t iMinValue, int32_t iMaxValue, int32_t *pOutValue)
{
    int32_t iOffset, iValue = strtol(pAction->strData, NULL, 10);
    
    if ((iValue >= iMinValue) && (iValue <= iMaxValue))
    {
        // print the action name and result
        iOffset = ds_snzprintf(pBuffer, iBufSize, "%s set to %d\n", pAction->strName, iValue);
        // also log action to debug output
        DirtyCastLogPrintf("serveraction: %s=%d\n", pAction->strName, iValue);
        // set the value
        *pOutValue = iValue;
    }
    else
    {
        iOffset = ds_snzprintf(pBuffer, iBufSize, "value %d is out of range\n", iValue);
    }
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function ServerActionParseEnum

    \Description
        Parse an action enum value.

    \Input *pBuffer         - output for response text
    \Input *iBufSize        - size of response text buffer
    \Input *pAction         - action to process  
    \Input *pEnumList       - enum list to allow
    \Input iNumEnums        - number of enum elements in list
    \Input *pOutValue       - [out] pointer to variable to update

    \Output
        int32_t             - updated response buffer offset    

    \Version 04/27/2010 (jbrookes)
*/
/********************************************************************************F*/
int32_t ServerActionParseEnum(char *pBuffer, int32_t iBufSize, const ServerActionT *pAction, const ServerActionEnumT *pEnumList, int32_t iNumEnums, int32_t *pOutValue)
{
    int32_t iEnum, iOffset;
    for (iEnum = 0, iOffset = 0; iEnum < iNumEnums; iEnum += 1)
    {
        if (!strcmp(pAction->strData, pEnumList[iEnum].strName))
        {
            // update the value
            *pOutValue = pEnumList[iEnum].iData;
            // print the action name and result
            iOffset = ds_snzprintf(pBuffer, iBufSize, "%s set to %s (%d)\n", pAction->strName, pAction->strData, *pOutValue);
            // also log action to debug output
            DirtyCastLogPrintf("serveraction: %s=%s (%d)\n", pAction->strName, pAction->strData, *pOutValue);
            break;
        }
    }
    if (pEnumList[iEnum].strName[0] == '\0')
    {
        iOffset = ds_snzprintf(pBuffer, iBufSize, "enum %s unrecognized\n", pAction->strData);
    }
    return(iOffset);
}

