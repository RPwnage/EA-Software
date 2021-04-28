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

#ifndef _serveraction_h
#define _serveraction_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! server action enum type
typedef struct ServerActionEnumT
{
    char strName[32];
    int32_t iData;
} ServerActionEnumT;

//! server action type
typedef struct ServerActionT
{
    char strName[32];
    char strData[32];
    
} ServerActionT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// helper function to parse action name/value pair
int32_t ServerActionParse(const char *pStrCommand, ServerActionT *pAction);

// helper function to parse an action integer value
int32_t ServerActionParseInt(char *pBuffer, int32_t iBufSize, const ServerActionT *pAction, int32_t iMinValue, int32_t iMaxValue, int32_t *pOutValue);

// helper function to parse an action enum value
int32_t ServerActionParseEnum(char *pBuffer, int32_t iBufSize, const ServerActionT *pAction, const ServerActionEnumT *pActionEnum, int32_t iNumEnums, int32_t *pOutValue);

#ifdef __cplusplus
};
#endif

#endif // _serverdiagnostic_h

