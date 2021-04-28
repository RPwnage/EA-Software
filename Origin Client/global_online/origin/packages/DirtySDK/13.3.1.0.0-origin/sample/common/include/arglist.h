/*H********************************************************************************/
/*!
    \File arglist.h

    \Description
        Standard argument interface for converting CSV lists and (argc,argv) lists.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 05/11/2005 (jfrank) First Version, based on code from Tyrone Ebert
*/
/********************************************************************************H*/

#ifndef _arglist_h
#define _arglist_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// Convert an array of bytes in into a binary (hex) encoded string
void ArgListBinaryToString(char *pOutBuf, int32_t iOutSize,  const unsigned char *pInBuf, int32_t iInSize);

// Print a string into an arglist
int32_t ArgListSetString(char *pOutBuf, int32_t iOutSize, const char *pInString);

// Print a number into an arglist
int32_t ArgListSetNumber(char *pOutBuf, int32_t iOutSize, int32_t iValue);

// Convert a parameter array into a CSV string
int32_t ArgListCSVCreate(char *pOutCSVBuf, int32_t iOutCSVSize, char **pArgvBuf, int32_t iArgvNumParams, int32_t iArgvParamLength);

// Create and argv list from a CSV list
int32_t ArgListArgvCreate(char **pOutArgvBuf, int32_t iArgvNumParams, int32_t iArgvParamLength, const char *pInBuf);

// Print an arglist to the console
void ArgListCSVPrint(const char *pCSVList);

// Create and argv list from a CSV list
void ArgListArgvPrint(int32_t iArgc, char **pArgv);

// Set the delimiter to a character other than the default
char ArgListSetDelimiter(char cNewDelimiter);

#ifdef __cplusplus
};
#endif

#endif // _arglist_h

