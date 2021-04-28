/*H********************************************************************************/
/*!
    \File configfile.c

    \Description
        Simple configuration file processor with simple substitution and conditional
        actions as file include.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 01/20/2004 (gschaefer) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <LegacyDirtySDK/util/tagfield.h>

#include "lobbytagexpr.h"
#include "configfile.h"

/*** Defines **********************************************************************/

#define MAX_FILESIZE        65536       // max include file
#define MAX_ENVIRON         4096        // max tagfield environment
#define MAX_SECTIONS        128         // individual sections (main+127)

/*** Type Definitions *************************************************************/

//! stack based load records
typedef struct ConfigLoadT
{
    struct ConfigLoadT *pNext;      //!< link to next load record

    uint8_t *pData;                 //!< pointer to data buffer
    int32_t iSize;                  //!< size of data buffer

    int32_t iRead;                  //!< number of bytes read
    int32_t iParse;                 //!< number of bytes parsed

    char strName[128];              //!< filename
    uint8_t strData[MAX_FILESIZE];  //!< data buffer storage
} ConfigLoadT;

//! file data section
typedef struct ConfigSectionT
{
    char strName[64];               //!< section name
    char *pOutBuf;                  //!< output buffer
    int32_t iOutMax;                //!< buffer max (alloc size)
    int32_t iOutLen;                //!< current buffer len
} ConfigSectionT;

//! module state record
struct ConfigFileT
{
    enum { LOAD_IDLE, LOAD_FILE, LOAD_PARSE } eLoad;
    ConfigLoadT *pLoad;                 //!< file load stack
    int32_t iStatus;                    //!< parse/load status

    int32_t iSectCount;                 //!< current section count
    int32_t iSectLimit;                 //!< total section limit
    ConfigSectionT *SectList[MAX_SECTIONS];
    ConfigSectionT *pSectCurr;          //!< current section
    ConfigSectionT *pSectSave;          //!< saved section
    ConfigSectionT *pSectWork;          //!< workspace section

    char strEnv[MAX_ENVIRON];           //!< tagfield environment
    char strError[128];                 //!< error string
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _ConfigLoadFileStart

    \Description
        Load a local file

    \Input pConfig - module state from create
    \Input pUrl - pointer to file name

    \Output None

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static void _ConfigLoadFileStart(ConfigFileT *pConfig, const char *pUrl)
{
    FILE *fd;

    // read in the file contents
    if ((pUrl == NULL) || (pUrl[0] == 0))
    {
        pConfig->iStatus = CONFIGFILE_STAT_OPENERR;
        ds_snzprintf(pConfig->strError, sizeof(pConfig->strError), "File not found: <null filename>");
    }
    else if ((fd = fopen(pUrl, "rb")) != NULL)
    {
        pConfig->pLoad->iRead = (int32_t)fread(pConfig->pLoad->pData, 1, pConfig->pLoad->iSize, fd);
        fclose(fd);
        if (pConfig->pLoad->iRead < 0)
        {
            // flag error and reset buffer size
            pConfig->iStatus = CONFIGFILE_STAT_READERR;
            ds_snzprintf(pConfig->strError, sizeof(pConfig->strError), "File read error: %s", pConfig->pLoad->strName);
            pConfig->pLoad->iRead = 0;
        }
    }
    else
    {
        pConfig->iStatus = CONFIGFILE_STAT_OPENERR;
        ds_snzprintf(pConfig->strError, sizeof(pConfig->strError), "File not found: %s", pConfig->pLoad->strName);
    }
}

/*F********************************************************************************/
/*!
    \Function _ConfigPush

    \Description
        Push the current load record, create a new one on top of stack

    \Input pConfig - module state from create

    \Output None

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static void _ConfigPush(ConfigFileT *pConfig)
{
    ConfigLoadT *pLoad;

    // allocate new load buffer
    pLoad = (ConfigLoadT*)malloc(sizeof(*pLoad));
    ds_memclr(pLoad, sizeof(*pLoad));
    pLoad->pData = pLoad->strData;
    pLoad->iSize = sizeof(pLoad->strData);

    // insert on top of stack
    pLoad->pNext = pConfig->pLoad;
    pConfig->pLoad = pLoad;

    // prepare to parse (will likely get reset to http/file load)
    pConfig->eLoad = LOAD_PARSE;
}

/*F********************************************************************************/
/*!
    \Function _ConfigPop

    \Description
        Pop the previous load record from stack and discard current

    \Input pConfig - module state from create

    \Output None

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static void _ConfigPop(ConfigFileT *pConfig)
{
    ConfigLoadT *pLoad;

    // make sure there is an element
    if (pConfig->pLoad != NULL)
    {
        // remove from stack and release memory
        pLoad = pConfig->pLoad;
        pConfig->pLoad = pLoad->pNext;
        free(pLoad);
    }

    // change mode to done if this is last pop
    pConfig->eLoad = (pConfig->pLoad ? LOAD_PARSE : LOAD_IDLE);
}

/*F********************************************************************************/
/*!
    \Function _ConfigSectionFind

    \Description
        Locate an output section

    \Input pConfig - module state from create
    \Input pName - name of section (""/NULL=main section)

    \Output None

    \Version 1.0 02/18/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static ConfigSectionT *_ConfigSectionFind(ConfigFileT *pConfig, const char *pName)
{
    int32_t iIndex;
    ConfigSectionT *pSection = NULL;

    // special case main section
    if ((pName == NULL) || (*pName == 0))
    {
        pSection = pConfig->SectList[0];
    }
    else
    {
        // search for matching section name (can optimize if more performance needed later)
        for (iIndex = 0; iIndex < pConfig->iSectCount; ++iIndex)
        {
            if (ds_stricmp(pName, pConfig->SectList[iIndex]->strName) == 0)
            {
                pSection = pConfig->SectList[iIndex];
                break;
            }
        }
    }
    return(pSection);
}

/*F********************************************************************************/
/*!
    \Function _ConfigSectionAdd

    \Description
        Add a new section

    \Input pConfig - module state from create
    \Input pName - name of section (""/NULL=main section)

    \Output None

    \Version 1.0 02/18/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static ConfigSectionT *_ConfigSectionAdd(ConfigFileT *pConfig, const char *pName)
{
    ConfigSectionT *pSection = NULL;

    // main section is called ""
    if (pName == NULL)
        pName = "";

    // make sure name is valid and there is an open slot
    if (pConfig->iSectCount < pConfig->iSectLimit)
    {
        // allocate new section
        pSection = (ConfigSectionT *)malloc(sizeof(*pSection));
        ds_memclr(pSection, sizeof(*pSection));
        // setup name and empty data
        strcpy(pSection->strName, pName);
        pSection->pOutBuf = NULL;
        // add to section list
        pConfig->SectList[pConfig->iSectCount++] = pSection;
    }
    return(pSection);
}

/*F********************************************************************************/
/*!
    \Function _ConfigSectionFree

    \Description
        Free all the sections

    \Input pConfig - module state from create

    \Output None

    \Version 1.0 02/18/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static void _ConfigSectionFree(ConfigFileT *pConfig)
{
    int32_t iIndex;
    ConfigSectionT *pSection;

    // walk the section list, freeing each one
    for (iIndex = 0; iIndex < pConfig->iSectCount; ++iIndex)
    {
        pSection = pConfig->SectList[iIndex];
        if (pSection->iOutMax > 0)
        {
            free(pSection->pOutBuf);
        }
        free(pSection);
        pConfig->SectList[iIndex] = NULL;
    }
    pConfig->iSectCount = 0;
}

/*F********************************************************************************/
/*!
    \Function _ConfigSectionClear

    \Description
        Clear data from a section

    \Input pSection - which section to clear

    \Output None

    \Version 1.0 02/18/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static void _ConfigSectionClear(ConfigSectionT *pSection)
{
    if (pSection->iOutMax > 0)
    {
        free(pSection->pOutBuf);
        pSection->iOutMax = 0;
        pSection->iOutLen = 0;
        pSection->pOutBuf = NULL;
    }
}

/*F********************************************************************************/
/*!
    \Function _ConfigSectionChar

    \Description
        Add a character to the section

    \Input pSection - which section to append
    \Input iCh - character to append to section

    \Output None

    \Version 1.0 02/18/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static void _ConfigSectionChar(ConfigSectionT *pSection, char iCh)
{
    int32_t iLength;
    char *pBuffer;

    // ignore NULL section
    if (pSection == NULL)
        return;

    // allocate new buffer if needed
    if (pSection->iOutLen >= pSection->iOutMax)
    {
        // allocate larger buffer
        iLength = pSection->iOutMax + (pSection->iOutLen < 8192 ? 1024 : 8192);
        pBuffer = (char *)malloc(iLength+1);
        // only copy if there is data (avoid null ref in some libs)
        if (pSection->iOutLen > 0)
        {
            ds_memcpy(pBuffer, pSection->pOutBuf, pSection->iOutLen);
        }
        // free any old buffer
        if (pSection->iOutMax > 0)
        {
            free(pSection->pOutBuf);
        }
        pSection->pOutBuf = pBuffer;
        pSection->iOutMax = iLength;
    }

    // save character in buffer
    pSection->pOutBuf[pSection->iOutLen++] = iCh;
    pSection->pOutBuf[pSection->iOutLen] = 0;
}

/*F********************************************************************************/
/*!
    \Function _ConfigSectionString

    \Description
        Output a string to a section

    \Input pSection - which section to append
    \Input pString - ASCIIZ string to output

    \Output None

    \Version 1.0 02/18/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static void _ConfigSectionString(ConfigSectionT *pSection, const char *pString)
{
    while (*pString != 0)
    {
        _ConfigSectionChar(pSection, *pString++);
    }
}

/*F********************************************************************************/
/*!
    \Function _ConfigLoadUpdate

    \Description
        Handle HTTP load progress

    \Input pConfig - module state from create
    \Input iBlocking - make it a blocking call

    \Output None

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static void _ConfigLoadUpdate(ConfigFileT *pConfig, int32_t iBlocking)
{
    // if there was an error, stop loading and parse
    if (pConfig->iStatus < 0)
    {
        pConfig->eLoad = LOAD_PARSE;
    }
}

/*F********************************************************************************/
/*!
    \Function _ConfigLoadStart

    \Description
        Handle HTTP load progress

    \Input pConfig - module state from create
    \Input pName - name of resource
    \Input iOptions - load specific options (passed down to handler)

    \Output None

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static void _ConfigLoadStart(ConfigFileT *pConfig, const char *pName)
{
    // create a new context
    _ConfigPush(pConfig);
    // save the URL
    ds_strnzcpy(pConfig->pLoad->strName, pName, sizeof(pConfig->pLoad->strName));

    // dispatch to load function
    _ConfigLoadFileStart(pConfig, pName);
}

/*F********************************************************************************/
/*!
    \Function _ConfigParseComments

    \Description
        Remove comments from loaded data, convert tabs to spaces

    \Input pConfig - module state from create
    \Input pData - pointer to data buffer

    \Output int32_t - number of characters in buffer

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static int32_t _ConfigParseComments(ConfigFileT *pConfig, unsigned char *pData)
{
    unsigned char *pLine;
    unsigned char *pSrc, *pDst;

    // remove invalid control chars, convert tabs to spaces
    for (pSrc = pDst = pData; *pSrc != 0; ++pSrc)
    {
        if (*pSrc == '\t')
        {
            *pDst++ = ' ';
        }
        else if ((*pSrc == '\n') || (*pSrc >= ' '))
        {
            *pDst++ = *pSrc;
        }
    }
    *pDst = 0;

    // remove comment lines that start with #
    for (pLine = pSrc = pDst = pData; *pSrc != 0; ++pSrc)
    {
        // comments are # with preceding whitespace
        if ((*pSrc == '#') && (pLine != NULL))
        {
            pDst = pLine;
            while (*pSrc >= ' ')
            {
                ++pSrc;
            }
            if (*pSrc == 0)
            {
                break;
            }
        }
        // newline will reset line marker
        else if (*pSrc == '\n')
        {
            *pDst++ = *pSrc;
            pLine = pDst;
        }
        // whitespace is copied normal
        else if (*pSrc == ' ')
        {
            *pDst++ = *pSrc;
        }
        // copy over non-whitespace data and mark as non-comment line
        else
        {
            *pDst++ = *pSrc;
            pLine = NULL;
        }
    }
    *pDst = 0;

    // return size of data
    return(pDst-pData);
}

/*F********************************************************************************/
/*!
    \Function _ConfigParseCommand

    \Description
        Parse a preprocessor command name

    \Input pConfig - module state from create
    \Input pData - pointer to input file
    \Input pCmd - pointer to output command buffer
    \Input iLen - length of command buffer
    
    \Output Number of bytes to parsed

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static int32_t _ConfigParseCommand(ConfigFileT *pConfig, unsigned char *pData, char *pCmd, int32_t iLen)
{
    int32_t iCount;

    // copy over the command
    for (iCount = 0; (pData[iCount] >= 'A') && (iCount < iLen-1); ++iCount)
    {
        pCmd[iCount] = tolower(pData[iCount]);
    }
    pCmd[iCount] = 0;

    // skip trailing space
    if (pData[iCount] == ' ')
    {
        ++iCount;
    }

    // return the parse length
    return(iCount);
}


/*F********************************************************************************/
/*!
    \Function _ParseConditionSkip

    \Description
        Skip a conditional block

    \Input pConfig - module state from create
    \Input pData - pointer to input file
    \Input bSkipElse - should elif/else be skipped?
    
    \Output Number of bytes to skip

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static int32_t _ParseConditionSkip(ConfigFileT *pConfig, unsigned char *pData, int32_t bSkipElse)
{
    int32_t iPrior;
    int32_t iParse;
    int32_t iLevel = 1;
    char strCmd[64];

    // look for markers
    for (iParse = 0; pData[iParse] != 0; ++iParse)
    {
        if ((pData[iParse+0] == '%') && (pData[iParse+1] == '%'))
        {
            iPrior = iParse;
            iParse += 2;
            iParse += _ConfigParseCommand(pConfig, pData+iParse, strCmd, sizeof(strCmd));

            // an if statement decends one level
            if (strcmp(strCmd, "if") == 0)
            {
                ++iLevel;
            }
            // an else is an end marker at the outer level
            else if ((strcmp(strCmd, "else") == 0) || (strcmp(strCmd, "elif") == 0))
            {
                if ((iLevel == 1) && !bSkipElse)
                {
                    // point to else command and break
                    iParse = iPrior;
                    break;
                }
            }
            // an endif returns one level
            else if (strcmp(strCmd, "endif") == 0)
            {
                if (--iLevel < 1)
                {
                    iParse = iPrior;
                    break;
                }
            }
        }
    }

    // return number of bytes parsed
    return(iParse);
}


/*F********************************************************************************/
/*!
    \Function _ParseControl

    \Description
        Parse a control function

    \Input pConfig - module state from create
    \Input pData - pointer to input file
    
    \Output Number of bytes to skip

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static int32_t _ParseControl(ConfigFileT *pConfig, unsigned char *pData)
{
    int32_t iSize;
    char strCmd[64];
    char strParm[1024];

    // make sure there is a lead-in
    if ((pData[0] != '%') || (pData[1] != '%'))
    {
        return(1);
    }

    // parse the command
    iSize = 2;
    iSize += _ConfigParseCommand(pConfig, pData+iSize, strCmd, sizeof(strCmd));

    // an elif/else at this point means we just finished processing a block
    if ((strcmp(strCmd, "else") == 0) || (strcmp(strCmd, "elif") == 0))
    {
        iSize += _ParseConditionSkip(pConfig, pData+iSize, 1);
    }
    // handle if
    else if (strcmp(strCmd, "if") == 0)
    {
        // evaluate expression and look for else clause if false
        iSize += TagFieldExpr(pConfig->strEnv, sizeof(pConfig->strEnv), (char *)pData+iSize, strParm, sizeof(strParm));
        while (atoi(strParm) == 0)
        {
            // skip past this if-clause, but don't skip past any elif/else clauses
            iSize += _ParseConditionSkip(pConfig, pData+iSize, 0);
            if ((pData[iSize+0] != '%') || (pData[iSize+1] != '%'))
            {
                break;
            }

            // parse next command (should be else/elif)
            iSize += 2;
            iSize += _ConfigParseCommand(pConfig, pData+iSize, strCmd, sizeof(strCmd));

            // if not an elif, then we are done
            if (strcmp(strCmd, "elif") != 0)
            {
                break;
            }

            // evaluate expression and break loop (process following code) if true
            iSize += TagFieldExpr(pConfig->strEnv, sizeof(pConfig->strEnv), (char *)pData+iSize, strParm, sizeof(strParm));
        }
    }
    // main command parser
    else if (strcmp(strCmd, "endif") == 0)
    {
        // just gobble endif
    }
    else if (strcmp(strCmd, "include") == 0)
    {
        // get include parameter and process
        iSize += TagFieldExpr(pConfig->strEnv, sizeof(pConfig->strEnv), (char *)pData+iSize, strParm, sizeof(strParm));
        if (strParm[0] != 0)
        {
            _ConfigLoadStart(pConfig, strParm);
        }
    }
    else if (strcmp(strCmd, "env") == 0)
    {
        iSize += TagFieldExpr(pConfig->strEnv, sizeof(pConfig->strEnv), (char *)pData+iSize, strParm, sizeof(strParm));
    }
    else if ((strcmp(strCmd, "expr") == 0) || (strcmp(strCmd, "var") == 0))
    {
        iSize += TagFieldExpr(pConfig->strEnv, sizeof(pConfig->strEnv), (char *)pData+iSize, strParm, sizeof(strParm));
        _ConfigSectionString(pConfig->pSectCurr, strParm);
    }
    else if (strcmp(strCmd, "parm") == 0)
    {
        ConfigSectionT *pSection = (pConfig->pSectSave ? pConfig->pSectSave : pConfig->pSectCurr);
        iSize += TagFieldExpr(pSection->pOutBuf, 0, (char *)pData+iSize, strParm, sizeof(strParm));
        _ConfigSectionString(pConfig->pSectCurr, strParm);
    }
    else if (strcmp(strCmd, "section") == 0)
    {
        // set to the desired section
        iSize += TagFieldExpr(pConfig->strEnv, sizeof(pConfig->strEnv), (char *)pData+iSize, strParm, sizeof(strParm));
        pConfig->pSectCurr = _ConfigSectionFind(pConfig, strParm);
        // auto-create if not present
        if (pConfig->pSectCurr == NULL)
        {
            pConfig->pSectCurr = _ConfigSectionAdd(pConfig, strParm);
        }
    }
    else if (strcmp(strCmd, "endsection") == 0)
    {
        // endsection always returns to main section
        pConfig->pSectCurr = _ConfigSectionFind(pConfig, "");
    }
    else if ((strcmp(strCmd, "{") == 0) && (pConfig->pSectSave == NULL))
    {
        // save current section so we can restore later
        pConfig->pSectSave = pConfig->pSectCurr;
        pConfig->pSectCurr = pConfig->pSectWork;
    }
    else if ((strcmp(strCmd, "*") == 0) && (pConfig->pSectSave != NULL))
    {
        TagFieldMerge(pConfig->strEnv, sizeof(pConfig->strEnv), pConfig->pSectWork->pOutBuf);
        if (pConfig->pSectWork->iOutLen > 0)
        {
            pConfig->pSectWork->iOutLen = 0;
            pConfig->pSectWork->pOutBuf[0] = 0;
        }
    }
    else if ((strcmp(strCmd, "}") == 0) && (pConfig->pSectSave != NULL))
    {
        TagFieldMerge(pConfig->strEnv, sizeof(pConfig->strEnv), pConfig->pSectWork->pOutBuf);
        if (pConfig->pSectWork->iOutLen > 0)
        {
            pConfig->pSectWork->iOutLen = 0;
            pConfig->pSectWork->pOutBuf[0] = 0;
        }
        pConfig->pSectCurr = pConfig->pSectSave;
        pConfig->pSectSave = NULL;
    }
    else
    {
        // pass unhandled sequence through unmolested
        _ConfigSectionChar(pConfig->pSectCurr, pData[0]);
        _ConfigSectionChar(pConfig->pSectCurr, pData[0]);
        // only gobble lead-in sequence
        iSize = 2;
    }

    // return the parse length
    return(iSize);
}


/*F********************************************************************************/
/*!
    \Function _ConfigParseData

    \Description
        Parse the input file

    \Input pConfig - module state from create
    \Input pData - pointer to input file

    \Output Number of bytes to skip

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static int32_t _ConfigParseData(ConfigFileT *pConfig, unsigned char *pData)
{
    int32_t iSize;

    // scan the data
    for (iSize = 0; pData[iSize] != 0; ++iSize)
    {
        // look for control sequences
        if ((pData[iSize+0] == '%') && (pData[iSize+1] == '%'))
        {
            iSize += _ParseControl(pConfig, pData+iSize);
            break;
        }

        // copy to output buffer
        _ConfigSectionChar(pConfig->pSectCurr, pData[iSize]);
    }
    return(iSize);
}


/*F********************************************************************************/
/*!
    \Function _ConfigParse

    \Description
        Run the parser for tor the current load level

    \Input pConfig - module state from create

    \Output Flag indicating if any parsing took place

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
static int32_t _ConfigParse(ConfigFileT *pConfig)
{
    int32_t iParsed = 0;
    ConfigLoadT *pLoad;

    // only perform work in busy mode
    if (pConfig->iStatus == CONFIGFILE_STAT_BUSY)
    {
        if (pConfig->eLoad == LOAD_IDLE)
        {
            // if busy but everything loaded, we must be done
            pConfig->iStatus = CONFIGFILE_STAT_DONE;
        }
        else if (pConfig->eLoad == LOAD_PARSE)
        {
            pLoad = pConfig->pLoad;

            // strip comments before processing rest of data
            if (pLoad->iParse == 0)
            {
                pLoad->iRead = _ConfigParseComments(pConfig, pLoad->pData);
            }

            // parse the data
            pLoad->iParse += _ConfigParseData(pConfig, pLoad->pData+pLoad->iParse);

            // pop at end of file
            if (pLoad->iParse >= pLoad->iRead)
            {
                _ConfigPop(pConfig);
            }
            iParsed = 1;
        }
    }
    return(iParsed);
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function ConfigFileCreate

    \Description
        Setup state for config file module

    \Output Pointer to config module state (pass to other functions)

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
ConfigFileT *ConfigFileCreate(void)
{
    ConfigFileT *pConfig;

    // allocate the module state
    pConfig = (ConfigFileT*)malloc(sizeof(*pConfig));
    ds_memclr(pConfig, sizeof(*pConfig));
    pConfig->iSectLimit = sizeof(pConfig->SectList)/sizeof(pConfig->SectList[0]);

    return(pConfig);
}

/*F********************************************************************************/
/*!
    \Function ConfigFileDestroy

    \Description
        Free resources and destroy module

    \Input pConfig - module state from create

    \Output NULL

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
ConfigFileT *ConfigFileDestroy(ConfigFileT *pConfig)
{
    // allow NULL to be passed as reference
    if (pConfig != NULL)
    {
        // clear to empty file
        ConfigFileClear(pConfig);
        // free all the sections
        _ConfigSectionFree(pConfig);
        // free module state
        free(pConfig);
    }

    // return null pointer to allow inline destroy/variable reset
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function ConfigFileClear

    \Description
        Reset to base state (no config data loaded)

    \Input pConfig - module state from create

    \Output None

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
void ConfigFileClear(ConfigFileT *pConfig)
{
    // pop everything from load stack
    while (pConfig->pLoad != NULL)
    {
        _ConfigPop(pConfig);
    }

    // clear all sections
    _ConfigSectionFree(pConfig);
    // create empty main/workspace
    pConfig->pSectCurr = _ConfigSectionAdd(pConfig, "");
    pConfig->pSectWork = _ConfigSectionAdd(pConfig, "$");
    pConfig->pSectSave = NULL;

    // reset the environment
    pConfig->strEnv[0] = 0;

    // no error
    pConfig->strError[0] = 0;
    pConfig->iStatus = CONFIGFILE_STAT_DONE;
}

/*F********************************************************************************/
/*!
    \Function ConfigFileLoad

    \Description
        Reset to base state (no config data loaded)

    \Input pConfig - module state from create
    \Input pUrl - pointer to resource
    \Input pEnv - pointer to environment

    \Output None

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
void ConfigFileLoad(ConfigFileT *pConfig, const char *pUrl, const char *pEnv)
{
    // reset to nothing loaded
    ConfigFileClear(pConfig);
    // setup the environment
    ds_strnzcpy(pConfig->strEnv, pEnv, sizeof(pConfig->strEnv));
    // we are busy
    pConfig->iStatus = CONFIGFILE_STAT_BUSY;
    // start the load
    _ConfigLoadStart(pConfig, pUrl);
}


/*F********************************************************************************/
/*!
    \Function ConfigFileUpdate

    \Description
        Reset to base state (no config data loaded)

    \Input pConfig - module state from create

    \Output None

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
void ConfigFileUpdate(ConfigFileT *pConfig)
{
    // keep looping as int32_t as there is work to do
    do {
        _ConfigLoadUpdate(pConfig, 0);
    } while (_ConfigParse(pConfig) > 0);
}

/*F********************************************************************************/
/*!
    \Function ConfigFileWait

    \Description
        Blocking wait for completion

    \Input pConfig - module state from create

    \Output None

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
void ConfigFileWait(ConfigFileT *pConfig)
{
    while (pConfig->iStatus == CONFIGFILE_STAT_BUSY)
    {
        _ConfigLoadUpdate(pConfig, 1);
        _ConfigParse(pConfig);
    }
}

/*F********************************************************************************/
/*!
    \Function ConfigFileError

    \Description
        Return error string

    \Input pConfig - module state from create

    \Output Error string (NULL=busy loading, ""=no error)

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
const char *ConfigFileError(ConfigFileT *pConfig)
{
    const char *pError = NULL;

    // give chance to do some work
    ConfigFileUpdate(pConfig);

    // see if load complete
    if (pConfig->iStatus == CONFIGFILE_STAT_DONE)
    {
        pError = "";
    }
    else if (pConfig->iStatus != CONFIGFILE_STAT_BUSY)
    {
        pError = pConfig->strError;
        if (*pError == 0)
        {
            pError = "Misc Error";
        }
    }
    return(pError);
}

/*F********************************************************************************/
/*!
    \Function ConfigFileData

    \Description
        Return the data that was loaded

    \Input pConfig - module state from create
    \Input pName - name of file section to return (""/NULL=main)

    \Output Pointer to file data (NULL=nothing loaded, ""=error loading)

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
const char *ConfigFileData(ConfigFileT *pConfig, const char *pName)
{
    const char *pData = NULL;
    ConfigSectionT *pSection;

    // give chance to do some work
    ConfigFileUpdate(pConfig);

    // only return if load is complete
    if (pConfig->eLoad == LOAD_IDLE)
    {
        pSection = _ConfigSectionFind(pConfig, pName);
        if (pSection != NULL)
        {
            pData = pSection->pOutBuf;
        }
    }
    // any error means no data
    if (pConfig->iStatus != CONFIGFILE_STAT_DONE)
    {
        pData = "";
    }
    return(pData);
}

/*F********************************************************************************/
/*!
    \Function ConfigFileIndex

    \Description
        Return the data that was loaded by section index (0=main)

    \Input pConfig - module state from create
    \Input iIndex - index of section to return (0=main, 1-x=sections)
    \Input pName - name of section

    \Output Pointer to file data (NULL=invalid section)

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
const char *ConfigFileIndex(ConfigFileT *pConfig, int32_t iIndex, const char **pName)
{
    ConfigSectionT *pSection;
    const char *pData = NULL;

    // make sure index is in allocated range
    if ((iIndex >= 0) && (iIndex < pConfig->iSectCount))
    {
        pSection = pConfig->SectList[iIndex];
        // return name if desired
        if (pName != NULL)
            *pName = pSection->strName;
        // translate into name lookup
        pData = ConfigFileData(pConfig, pSection->strName);
    }

    return(pData);
}

/*F********************************************************************************/
/*!
    \Function ConfigFileControl

    \Description
        Perform control/status updates on configfile channel. 'stat' returns the
        current load status. 'http' will enable/disable http fetch based on iParm.

    \Input pConfig - module state from create
    \Input iSelect - 'abcd' style selector
    \Input iParm - selector specific
    \Input pData - selector specific

    \Output Status code (CONFIGFILE_STAT_xxxx)

    \Version 1.0 03/06/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
int32_t ConfigFileControl(ConfigFileT *pConfig, int32_t iSelect, int32_t iParm, void *pData)
{
    int32_t iResult = 0;

    // handle status query
    if (iSelect == 'stat')
    {
        if (pConfig->iStatus == CONFIGFILE_STAT_BUSY)
        {
            ConfigFileUpdate(pConfig);
        }
        iResult = pConfig->iStatus;
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ConfigFileImmed

    \Description
        Do immediate load (blocking until completion)

    \Input pUrl - pointer to resource
    \Input pEnv - pointer to environment

    \Output Pointer to file data

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
char *ConfigFileImmed(const char *pUrl, const char *pEnv)
{
    int32_t iSize;
    char *pReturn;
    const char *pData;
    ConfigFileT *pConfig;

    // setup the config
    pConfig = ConfigFileCreate();
    ConfigFileLoad(pConfig, pUrl, pEnv);
    ConfigFileWait(pConfig);
    pData = ConfigFileData(pConfig, "");
    // make private copy
    iSize = (int32_t)strlen(pData)+1;
    pReturn = (char *)malloc(iSize);
    ds_memcpy(pReturn, pData, iSize);
    // done with state(char *)
    ConfigFileDestroy(pConfig);
    return(pReturn);
}

/*F********************************************************************************/
/*!
    \Function ConfigFileImport

    \Description
        Import data into a config file section

    \Input pConfig - module state from create
    \Input pName - name of file section to import (""/NULL=main)
    \Input pData - data to import (null terminated)

    \Output Pointer to file data

    \Version 1.0 02/13/2004 (gschaefer) First Version
*/
/********************************************************************************F*/
void ConfigFileImport(ConfigFileT *pConfig, const char *pName, const char *pData)
{
    ConfigSectionT *pSection;

    // create new section if one does not exist
    pSection = _ConfigSectionFind(pConfig, pName);
    if (pSection == NULL)
    {
        pSection = _ConfigSectionAdd(pConfig, pName);
    }

    // clear old data and import new
    if (pSection != NULL)
    {
        _ConfigSectionClear(pSection);
        _ConfigSectionString(pSection, pData);
    }
}


