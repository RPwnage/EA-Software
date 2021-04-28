/*H*************************************************************************************************/
/*!
    \File    lobbytagexpr.h

    \Description
        This moduel implements a simple expression parser which uses a TagField record as
        a variable store and provides basic C type expression syntax. Assignment, comparison,
        logical, binary and basic arithmetic operations are all supported.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 02/28/2004 (gschaefer) First Version
*/
/*************************************************************************************************H*/

#ifndef _lobbytagexpr_h
#define _lobbytagexpr_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// do the expression evaluation
int32_t TagFieldExpr(char *pRecord, int32_t iReclen, const char *pExpr, char *pValue, int32_t iValue);

#ifdef __cplusplus
}
#endif

#endif // _lobbytagexpr_h

