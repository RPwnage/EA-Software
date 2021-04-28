/*H********************************************************************************/
/*!
    \File testerhistory.c

    \Description
        Maintains command history for a particular user.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 04/05/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "zlib.h"
#include "zfile.h"
#include "zmem.h"
#include "testercomm.h"
#include "testerregistry.h"
#include "testerhistory.h"

/*** Defines **********************************************************************/

#define TESTERHISTORY_SIZE_DEFAULT              (100)       //!< number of entries to save

/*** Type Definitions *************************************************************/

//! Each history entry has a structure
typedef struct TesterHistoryEntryT
{
    int32_t iCount;                                                 //!< history entry count
    char strText[TESTERCOMM_COMMANDSIZE_DEFAULT];               //!< command text
} TesterHistoryEntryT;

struct TesterHistoryT
{
    int32_t iTotalEntries;                  //!< total entries
    int32_t iHeadIndex;                     //!< head index - first valid entry
    int32_t iTailIndex;                     //!< tail index - last valid entry
    int32_t iHeadCount;                     //!< command count of the head entry
    int32_t iTailCount;                     //!< command count of the tail entry
    int32_t iCount;                         //!< history count object - keep track of how many entries have gone past
    TesterHistoryEntryT *pEntries;      //!< tester history entries
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _TesterHistoryGetFullPath

    \Description
        Get full path to history file.

    \Input *pFilename   - name of history file
    
    \Output
        const char *    - pointer to full name

    \Version 05/11/2005 (jbrookes)
*/
/********************************************************************************F*/
static const char *_TesterHistoryGetFullPath(const char *pFilename)
{
    static char strHistoryName[128];

    // create full path
    TesterRegistryGetString("ROOT", strHistoryName, sizeof(strHistoryName));
    if (strHistoryName[0] != '\0')
    {
        ds_strnzcat(strHistoryName, "\\", sizeof(strHistoryName));
        ds_strnzcat(strHistoryName, pFilename, sizeof(strHistoryName));
    }
    else
    {
        ds_strnzcpy(strHistoryName, ".\\", sizeof(strHistoryName));
    }
    return(strHistoryName);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function TesterHistoryCreate

    \Description
        Create a tester history module

    \Input iSize - size of history to create, 0 for default
    
    \Output TesterHistoryT * - allocated tester history module

    \Version 04/05/2005 (jfrank)
*/
/********************************************************************************F*/
TesterHistoryT *TesterHistoryCreate(int32_t iSize)
{
    TesterHistoryT *pState;
    uint32_t uBufferSize;
    
    // allocate module state
    pState = ZMemAlloc(sizeof(TesterHistoryT));
    ZMemSet(pState, 0, sizeof(TesterHistoryT));
    
    // allocate history entries
    pState->iTotalEntries = (iSize <= 0) ? TESTERHISTORY_SIZE_DEFAULT : iSize;
    uBufferSize = pState->iTotalEntries * sizeof(TesterHistoryEntryT);
    pState->pEntries = ZMemAlloc(uBufferSize);   
    ZMemSet(pState->pEntries, 0, uBufferSize);
    
    TesterRegistrySetPointer("HISTORY", pState);

    return(pState);
}

/*F********************************************************************************/
/*!
    \Function TesterHistoryGet

    \Description
        Return the text for a particular tester command

    \Input *pState - module state
    \Input  iNum   - entry number requested
    \Input *pBuf   - destination buffer
    \Input  iSize  - size of destination buffer
    
    \Output int32_t    - 0=success, error code otherwise

    \Version 04/05/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterHistoryGet(TesterHistoryT *pState, int32_t iNum, char *pBuf, int32_t iSize)
{
    TesterHistoryEntryT *pEntry = NULL;
    int32_t iIndex;

    // check for errors
    if((pState == NULL) || (pBuf == NULL) || (iSize <= 0))
    {
        return(TESTERHISTORY_ERROR_NULLPOINTER);
    }

    // see if we have the entry
    for(iIndex = 0; (iIndex < pState->iTotalEntries) && (pEntry == NULL); iIndex++)
    {
        if(pState->pEntries[iIndex].iCount == iNum)
            pEntry = &(pState->pEntries[iIndex]);
    }

    // did we find it?
    if(pEntry == NULL)
    {
        return(TESTERHISTORY_ERROR_NOSUCHENTRY);
    }

    // copy in the data
    ds_strnzcpy(pBuf, pEntry->strText, iSize);
    return(TESTERHISTORY_ERROR_NONE);
}


/*F********************************************************************************/
/*!
    \Function TesterHistorySave

    \Description
        Save historical commands to a file.
        
    \Input *pState    - module state
    \Input *pFilename - filename to save the history in
    
    \Output int32_t       - number of entries saved, <0 if error

    \Version 05/05/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterHistorySave(TesterHistoryT *pState, const char *pFilename)
{
    char strCommand[TESTERCOMM_COMMANDSIZE_DEFAULT];
    int32_t iHead, iTail, iLoop, iLen, iTotalSize;
    ZFileT iFileId;

    if ((pState == NULL) || (pFilename == NULL))
    {
        return(-1);
    }

    // convert to full path
    pFilename = _TesterHistoryGetFullPath(pFilename);

    // get the count
    if(TesterHistoryHeadTailCount(pState, &iHead, &iTail))
    {
        // probably an empty list - don't save
        return(0);
    }

    // overwrite anything already there
    iTotalSize = 0;

    // try and open the file
    if ((iFileId = ZFileOpen(pFilename, ZFILE_OPENFLAG_WRONLY | ZFILE_OPENFLAG_CREATE)) < 0)
    {
        ZPrintf("testerhistory: error %d trying to save history to %s\n", iFileId, pFilename);
        return(0);
    }

    // save the history
    for (iLoop = iHead; iLoop <= iTail; iLoop++)
    {
        ZMemSet(strCommand, 0, sizeof(strCommand));
        TesterHistoryGet(pState, iLoop, strCommand, sizeof(strCommand)-2);
        iLen = (int32_t)strlen(strCommand);
        strCommand[iLen++] = '\n';
        strCommand[iLen] = '\0';
        ZFileWrite(iFileId, strCommand, iLen);
        iTotalSize ++;
    }
    ZFileClose(iFileId);

    // return the number of entries saved
    return(iTotalSize);
}


/*F********************************************************************************/
/*!
    \Function TesterHistoryLoad

    \Description
        Load historical commands into the command history from a file.
        
    \Notes
        Loads all commands in the file into memory, but based on the size 
        of the history created, only the last N number of commands 
        (where N is the number of commands requested at TesterHistoryCreate)
        will be saved in memory.

    \Input *pState    - module state
    \Input *pFilename - filename with the history, one command per line
    
    \Output int32_t       - number of entries in the file, <0 if error

    \Version 05/05/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterHistoryLoad(TesterHistoryT *pState, const char *pFilename)
{
    char *pHistory, *pCmd;
    const char strSep[] = "\r\n";
    int32_t iFileSize;
    int32_t iNumCommands = 0;

    if ((pState == NULL) || (pFilename == NULL))
    {
        return(-1);
    }

    // convert to full path
    pFilename = _TesterHistoryGetFullPath(pFilename);

    pHistory = ZFileLoad(pFilename, &iFileSize, 0);
    if (pHistory == NULL)
    {
        return(0);
    }

    pCmd = strtok(pHistory, strSep);
    while(pCmd != NULL)
    {
        TesterHistoryAdd(pState, pCmd);
        iNumCommands++;
        pCmd = strtok(NULL, strSep);
    }
    ZMemFree(pHistory);
    return(iNumCommands);
}



/*F********************************************************************************/
/*!
    \Function TesterHistoryAdd

    \Description
        Add an entry to the tester history

    \Input *pState - module state
    \Input *pBuf   - text to add
    
    \Output int32_t    - index value ( >= 0 ) or error code ( < 0 ) if error occurs

    \Version 04/05/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterHistoryAdd(TesterHistoryT *pState, const char *pBuf)
{
    TesterHistoryEntryT *pEntry;

    // check for errors
    if((pState == NULL) || (pBuf == NULL))
    {
        return(TESTERHISTORY_ERROR_NULLPOINTER);
    }

    // clear the new entry
    pEntry = &(pState->pEntries[pState->iTailIndex]);
    ZMemSet(pEntry, 0, sizeof(TesterHistoryEntryT));

    // and copy the new entry in
    ds_strnzcpy(pEntry->strText, pBuf, sizeof(pEntry->strText));
    pState->iTailCount = pState->iCount;
    pEntry->iCount     = pState->iCount;
    pState->iCount++;

    // adjust the tail pointer
    pState->iTailIndex++;
    if(pState->iTailIndex >= pState->iTotalEntries)
        pState->iTailIndex = 0;

    // see if we need to adjust the head pointer
    if(pState->iHeadIndex == pState->iTailIndex)
    {
        // we're looping around - adjust the pointer
        pState->iHeadIndex++;
        // adjust if we're looping around
        if(pState->iHeadIndex >= pState->iTotalEntries)
            pState->iHeadIndex = 0;
        // get the head count
        pState->iHeadCount = pState->pEntries[pState->iHeadIndex].iCount;
    }

    // return the index number
    return(pState->iCount);
}

/*F********************************************************************************/
/*!
    \Function TesterHistoryHeadTailCount

    \Description
        Return the head and tail history numbers

    \Input *pState   - module state
    \Input *pHeadNum - destination history entry number of the first entry in the buffer
    \Input *pTailNum - destination history entry number of the last  entry in the buffer
    
    \Output int32_t    - 0=success, error code otherwise

    \Version 04/05/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterHistoryHeadTailCount(TesterHistoryT *pState, int32_t *pHeadNum, int32_t *pTailNum)
{
    // set the incoming values to some default values, just in case we run into an error
    *pHeadNum = 0;
    *pTailNum = 0;

    // check for errors
    if((pState == NULL) || (pHeadNum == NULL) || (pTailNum == NULL))
    {
        return(TESTERHISTORY_ERROR_NULLPOINTER);
    }

    // see if the buffer is empty
    if(pState->iHeadIndex == pState->iTailIndex)
    {
        return(TESTERHISTORY_ERROR_NOSUCHENTRY);
    }

    // otherwise give out some information
    *pHeadNum = pState->iHeadCount;
    *pTailNum = pState->iTailCount;
    return(TESTERHISTORY_ERROR_NONE);
}


/*F********************************************************************************/
/*!
    \Function TesterHistoryDestroy

    \Description
        Destroy a tester history object

    \Input *pState - module state
    
    \Output None

    \Version 04/05/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterHistoryDestroy(TesterHistoryT *pState)
{
    // kill the object if non-null
    if(pState)
    {
        ZMemFree(pState->pEntries);
        ZMemFree(pState);
    }

    TesterRegistrySetPointer("HISTORY", NULL);
}
