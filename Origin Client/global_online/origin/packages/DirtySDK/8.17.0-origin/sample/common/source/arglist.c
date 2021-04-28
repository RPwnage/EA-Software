/*H********************************************************************************/
/*!
    \File arglist.c

    \Description
        Standard argument interface for converting CSV lists and (argc,argv) lists.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 05/11/2005 (jfrank) First Version, based on code from Tyrone Ebert
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "dirtysock.h"
#include "arglist.h"

/*** Defines **********************************************************************/

#define ArgListPrintf(x)    printf x

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

static char g_cArgDelimiter     = ',';

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function ArgListBinaryToString

    \Description
        Print an array of binary data into a string.

    \Input *pOutBuf  - string to put the printed binary data into
    \Input  iOutSize - size, in bytes, of destination string
    \Input *pInBuf   - binary data to be printed
    \Input  iInSize  - size, in bytes, of binary data array to be printed
    
    \Output None

    \Version 05/11/2005 (jfrank)
*/
/********************************************************************************F*/
void ArgListBinaryToString(char *pOutBuf, int32_t iOutSize,  const unsigned char *pInBuf, int32_t iInSize)
{
    const char strHex[] = "0123456789ABCDEF\0";
    unsigned char uValue;
    char *pOut;
    int32_t iLoop;

    // check for errors
    if ((pOutBuf == NULL) || (iOutSize <= 0) || (pInBuf == NULL) || (iInSize <= 0))
        return;

    // print to the string
    for ( iLoop = 0; ((iLoop < iInSize) && ((iLoop*2) < iOutSize)); iLoop++)
    {
        // get the value to write
        uValue = *(pInBuf + iLoop);
        // and write to the correct location with the right character
        pOut    = pOutBuf + (iLoop*2);
        pOut[0] = strHex[((uValue & 0xF0) >> 4)];
        pOut[1] = strHex[(uValue & 0x0F)];
    }
}


/*F********************************************************************************/
/*!
    \Function ArgListSetString

    \Description
        Print a string into an argument string container

    \Input *pOutBuf   - string to put the printed data into
    \Input  iOutSize  - size, in bytes, of destination string
    \Input *pInString - source string
    
    \Output int32_t       - number of characters printed

    \Version 05/11/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t ArgListSetString(char *pOutBuf, int32_t iOutSize, const char *pInString)
{
    int32_t iOldLength, iNewLength;

    // check for errors
    if ((pOutBuf == NULL) || (iOutSize <= 0) || (pInString == NULL))
        return(0);

    // append to the end of the already-existing string
    iOldLength = (int32_t)strlen(pOutBuf);
    strncat(pOutBuf, pInString, iOutSize-1);
    iNewLength = (int32_t)strlen(pOutBuf);
    return(iNewLength - iOldLength);
}


/*F********************************************************************************/
/*!
    \Function ArgListSetNumber

    \Description
        Print an integer into an argument string container

    \Input *pOutBuf   - string to put the printed data into
    \Input  iOutSize  - size, in bytes, of destination string
    \Input  iValue    - number to print
    
    \Output int32_t       - number of characters printed

    \Version 05/11/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t ArgListSetNumber(char *pOutBuf, int32_t iOutSize, int32_t iValue)
{
    char strTemp[512];
    if (iOutSize > (signed)sizeof(strTemp))
    {
        iOutSize = sizeof(strTemp);
    }
    memset(strTemp, 0, sizeof(strTemp));
    snzprintf(strTemp, sizeof(strTemp),"%d", iValue);
    return(ArgListSetString(pOutBuf, iOutSize, strTemp));
}


/*F********************************************************************************/
/*!
    \Function ArgListCSVCreate

    \Description
        Print a parameter array into a CSV (comma separated value) string.

    \Notes
        If a string in the argument list is empty, a space will be printed
        in order to keep strtok processing working.

    \Input  *pOutBuf          - string to put the printed data into
    \Input   iOutSize         - size, in bytes, of destination string
    \Input **pArgvBuf         - source argv list (array of strings) to use
    \Input   iArgvNumParams   - number of parameters in the argv list (argc)
    \Input   iArgvParamLength - length, in bytes, of each parameter in the argv list
    
    \Output  int32_t              - size of string created

    \Version 05/11/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t ArgListCSVCreate(char *pOutBuf, int32_t iOutSize, char **pArgvBuf,  int32_t iArgvNumParams,  int32_t iArgvParamLength)
{
    char strDelim[2] = { 0, 0 };
    char *pParam;
    int32_t iLoop;
    
    // init delim
    strDelim[0] = g_cArgDelimiter;

    // check for errors
    if ((pOutBuf == NULL) || (iOutSize <= 0) || (pArgvBuf == NULL) || (iArgvNumParams <= 0) || (iArgvParamLength <= 0))
        return(0);

    // print all the parameters to the CSV string
    for (iLoop = 0; (iLoop < iArgvNumParams); iLoop++)
    {
        pParam = ((char *)pArgvBuf + (iLoop * iArgvParamLength));
        if (strlen(pParam) == 0)
        {
            // keep strtok happy by putting a space between the delimiters
            strncat(pOutBuf, " ", iOutSize);
        }
        else
        {
            strncat(pOutBuf, pParam, iOutSize);
        }
        strncat(pOutBuf, strDelim, iOutSize);
    }
    return((int32_t)strlen(pOutBuf));
}


/*F********************************************************************************/
/*!
    \Function ArgListArgvCreate

    \Description
        Exract fields from a CSV list and put them into an already-allocated
        argv list (array of strings).

    \Notes
        Depends on there being a space between delimiters for a blank field.

    \Input **pOutArgvBuf       - destination argv list (array of strings)
    \Input   iArgvNumParams    - number of strings in the array (argc)
    \Input   iArgvParamLength  - length of each string in the array
    \Input  *pInBuf            - source CSV list, must be null terminated
    
    \Output  int32_t - number of parameters successfully added to the argv list

    \Version 05/11/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t ArgListArgvCreate(char **pOutArgvBuf,  int32_t iArgvNumParams,  int32_t iArgvParamLength, const char *pInBuf)
{
    char strDelim[2] = { 0, 0 };
    char *pInParam, *pOutParam, *pSource;
    int32_t iNumParams = 0;
    int32_t iLength;

    // init delim
    strDelim[0] = g_cArgDelimiter;

    // check for errors
    if ((pInBuf == NULL) || (pOutArgvBuf == NULL) || (iArgvParamLength <= 0) || (iArgvNumParams <= 0))
        return(0);

    // get the length of the input buffer and bail if there's nothing to do
    iLength = (int32_t)strlen(pInBuf);
    if (iLength == 0)
        return(0);

    // copy the source buffer so we don't destroy what came in
    pSource = malloc(iLength);      
    memcpy(pSource, pInBuf, iLength);

    // clear the entire argv buffer
    memset(pOutArgvBuf, 0, iArgvNumParams * iArgvParamLength);
    pOutParam = (char *)pOutArgvBuf;
    pInParam  = strtok(pSource, strDelim);
    while(pInParam)
    {
        strncpy(pOutParam, pInParam, iArgvParamLength-1);
        pOutParam += iArgvParamLength;
        pInParam  = strtok(NULL, strDelim);
        iNumParams++;
    }

    // free the memory used and exit, returning the number of parameters parsed
    free(pSource);
    return(iNumParams);
}


/*F********************************************************************************/
/*!
    \Function ArgListCSVPrint

    \Description
        Print a CSV arglist to the console.

    \Input *pCSVList - CSV list to print
    
    \Output None

    \Version 05/11/2005 (jfrank)
*/
/********************************************************************************F*/
void ArgListCSVPrint(const char *pCSVList)
{
    if(pCSVList == NULL)
    {
        ArgListPrintf(("arglist: CSVList [NULL]\n"));
    }
    else
    {
        ArgListPrintf(("arglist: CSVList [%s]\n", pCSVList));
    }
    return;
}


/*F********************************************************************************/
/*!
    \Function ArgListArgvPrint

    \Description
        Print an argv list (array of strings) to the console.

    \Input   iArgc - number of arguments to print
    \Input **pArgv - array of (char *) pointers to print
    
    \Output None

    \Version 05/11/2005 (jfrank)
*/
/********************************************************************************F*/
void ArgListArgvPrint(int32_t iArgc, char **pArgv)
{
    int32_t iLoop;

    // check for errors
    if ((pArgv == NULL) || (iArgc <= 0))
    {
        ArgListPrintf(("\tArg List Empty.  argc[%d] argv[%p]\n", iArgc, (pArgv == NULL) ? 0 : pArgv));
        return;
    }

    ArgListPrintf(("arglist: Argv List [%d] entries\n", iArgc));
    for(iLoop = 0; iLoop < iArgc; iLoop++)
    {
        if(strlen(pArgv[0]) == 0)
        {
            ArgListPrintf(("\t[%3d] :\n", iLoop));
        }
        else
        {
            ArgListPrintf(("\t[%3d] : %s\n", iLoop, pArgv[iLoop]));
        }
    }
}


/*F********************************************************************************/
/*!
    \Function ArgListSetDelimiter

    \Description
        Set the delimier to a character other than the default.

    \Input cNewDelimiter - new delimiter character to use
    
    \Output char         - old delimiter

    \Version 05/11/2005 (jfrank)
*/
/********************************************************************************F*/
char ArgListSetDelimiter(char cNewDelimiter)
{
    char cOldDelimiter = g_cArgDelimiter;
    g_cArgDelimiter = cNewDelimiter;
    return(cOldDelimiter);
}
