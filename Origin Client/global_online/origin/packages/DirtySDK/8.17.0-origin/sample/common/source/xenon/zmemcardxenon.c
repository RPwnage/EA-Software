/*H********************************************************************************/
/*!
    \File ZMemcardxenon.c

    \Description
        Basic file operations (open, read, write, close, size).

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 03/16/2006 (tdills) First Version for Xenon
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <xtl.h>
#include <xbox.h>

#include "platform.h"
#include "zmemcard.h"
#include "zfile.h"

/*** Defines **********************************************************************/

// memory card directory
#define ZMEMCARD_BASE_DIR "d:\\"

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

static int32_t  _bInitialized = 0; // use 32-bit bool for byte-alignment purposes.
static char     _strCurrentDir[4096] = "";

/*** Private Functions ************************************************************/

static const char *_ZMemcardMakePath(const char *pFileName)
{
    static char strPath[4096];

    strcpy(strPath, _strCurrentDir);
    strcat(strPath, pFileName);

    return(strPath);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ZMemcardInit

    \Description
        Initialize the memory card module.

    \Input *pFileName   - file name to open
    \Input bRead        - if TRUE, open for read access
    \Input bWrite       - if TRUE, open for write access
    
    \Output
        int32_t             - file descriptor, or -1 if there was an error

    \Version 1.0 03/14/2006 (tdills)   Implemented for Xenon
*/
/********************************************************************************F*/
int32_t ZMemcardInit(void)
{
    // initialzie the current directory
    strcpy(_strCurrentDir, ZMEMCARD_BASE_DIR);
    // set the initialized flag
    _bInitialized = 1;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function ZMemcardFOpen

    \Description
        Open a file.

    \Input *pFileName   - file name to open
    \Input bRead        - if TRUE, open for read access
    \Input bWrite       - if TRUE, open for write access
    \Input bCreate      - if TRUE, create a new file
    
    \Output
        int32_t             - file descriptor, or -1 if there was an error

    \Version 1.0 03/14/2006 (tdills)   Implemented for Xenon
*/
/********************************************************************************F*/
int32_t ZMemcardFOpen(const char *pFileName, uint32_t bRead, uint32_t bWrite, uint32_t bCreate)
{
    // convert the input parameters into ZFileOpen flags.
    uint32_t uFlags = 0;

    if (bRead)
    {
        uFlags |= ZFILE_OPENFLAG_RDONLY;
    }

    if (bWrite)
    {
        uFlags |= ZFILE_OPENFLAG_WRONLY;
    }

    if (bCreate)
    {
        uFlags |= ZFILE_OPENFLAG_CREATE;
    }

    // use ZFileXenon to do file operation
    return(ZFileOpen(_ZMemcardMakePath(pFileName), uFlags));
}

/*F********************************************************************************/
/*!
    \Function ZMemcardFClose

    \Description
        Close a file

    \Input iFileId  - file descriptor
    
    \Output
        None.

    \Version 1.0 03/14/2006 (tdills)   Implemented for Xenon
*/
/********************************************************************************F*/
void ZMemcardFClose(int32_t iFileId)
{
    // use ZFileXenon to do file operation.
    ZFileClose(iFileId);
}

/*F********************************************************************************/
/*!
    \Function ZMemcardFSeek

    \Description
        Read from a file.

    \Input iFileId  - file descriptor
    \Input iOffset  - offset in file to seek to

    \Output
        int32_t         - zero=success, negative=failure

    \Version 1.0 03/14/2006 (tdills) First Version for Xenon
*/
/********************************************************************************F*/
int32_t ZMemcardFSeek(int32_t iFileId, int32_t iOffset)
{
    // use ZFileXenon to do file operation.
    return(ZFileSeek(iFileId, iOffset, ZFILE_SEEKFLAG_SET));
}

/*F********************************************************************************/
/*!
    \Function ZMemcardFReadAsync

    \Description
        Read from a file, asynchronously.

    \Input *pData   - pointer to buffer to read to
    \Input iSize    - amount of data to read
    \Input iFileId  - file descriptor
    
    \Output
        Number of bytes read

    \Notes
        Xenon version is synchronous.

    \Version 1.0 03/14/2006 (tdills)   Implemented for Xenon
*/
/********************************************************************************F*/
int32_t ZMemcardFReadAsync(void *pData, int32_t iSize, int32_t iFileId)
{
    // use ZFileXenon to do file operation.
    return(ZFileRead(iFileId, pData, iSize));
}

/*F********************************************************************************/
/*!
    \Function ZMemcardFWriteAsync

    \Description
        Write to a file, asynchronously.

    \Input *pData   - pointer to buffer to write from
    \Input iSize    - amount of data to write
    \Input iFileId  - file descriptor
    
    \Output
        Number of bytes written

    \Notes
        Xenon version is synchronous.

    \Version 1.0 03/14/2006 (tdills)   Implemented for Xenon.
*/
/********************************************************************************F*/
int32_t ZMemcardFWriteAsync(void *pData, int32_t iSize, int32_t iFileId)
{
    // use ZFileXenon to do file operation.
    return(ZFileWrite(iFileId, pData, iSize));
}

/*F********************************************************************************/
/*!
    \Function ZMemcardFSync

    \Description
        Synchronize last memcard op.

    \Input iFileId  - file descriptor
    
    \Output
        int32_t         - zero=success, negative=failure

    \Notes
        Win32 version is a stub, as read/writes are synchronous.

    \Version 1.0 11/18/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ZMemcardFSync(int32_t iFileId)
{
    return(0);
}

/*F********************************************************************************/
/*!
    \Function ZMemcardFSize

    \Description
        Get a file's size.

    \Input *pFileName   - pointer to filename to get size of
    
    \Output
        File size, or -1 if an error occurred.

    \Version 1.0 03/14/2006 (tdills)   Implemented for Xenon
*/
/********************************************************************************F*/
int32_t ZMemcardFSize(const char *pFileName)
{
    ZFileStatT StatData;

    // use ZFileXenon to retrieve file size.
    if (ZFileStat(_ZMemcardMakePath(pFileName), &StatData) == 0)
    {
        return(StatData.iSize);
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ZMemcardFDir

    \Description
        Get a memory card's directory

    \Input *pDirectory   - pointer to filename to get size of
    
    \Output
        int32_t             - zero=success, negative=failure

    \Version 1.0 03/14/2006 (tdills)   Implemented for Xenon
*/
/********************************************************************************F*/
int32_t ZMemcardFDir(ZMemcardDirT *pDirectory)
{
    WIN32_FIND_DATA wfd;
    HANDLE hFind;
    int32_t iFileCt = 0;
    char strSearchString[4096];

    if (!_bInitialized)
    {
        ZMemcardInit();
    }

    // construct the search string.
    strcpy(strSearchString,_strCurrentDir);
    strcat(strSearchString,"*");

    // Start the find and check for failure.
    if ((hFind = FindFirstFile( strSearchString, &wfd)) != INVALID_HANDLE_VALUE)
    {
        // Display each file and ask for the next.
        do
        {
            strcpy(pDirectory->Files[iFileCt].strName, wfd.cFileName);
            pDirectory->Files[iFileCt].iSize = wfd.nFileSizeLow;
            iFileCt++;
        } while(FindNextFile(hFind, &wfd));

        // Close the find handle.
        FindClose(hFind);
    }
    else
    {
        return(-1);
    }

    pDirectory->iNumFiles = iFileCt;
    return(iFileCt);
}

/*F********************************************************************************/
/*!
    \Function ZMemcardFDir

    \Description
        Change the current working directory for the mem card.

    \Input *pDirName     - the sub-directory of the current directory.
    
    \Output
        int32_t             - zero=success, negative=failure

    \Version 1.0 03/14/2006 (tdills)   Implemented for Xenon
*/
/********************************************************************************F*/
int32_t ZMemcardFCd(const char *pDirName)
{
    char strNewDir[4096];
    ZMemcardDirT DirData;

    // append the new directory to the current directory
    strcpy(strNewDir,_strCurrentDir);
    strcat(strNewDir,pDirName);
    if (strNewDir[strlen(strNewDir)-1] != '\\')
    {
        // add the directory separator to the dir name.
        strcat(strNewDir,"\\");
    }

    // test the directory
    if (0 > ZMemcardFDir(&DirData))
    {
        return(-1);
    }

    // if the new directory is good then make it the current directory
    strcpy(_strCurrentDir, strNewDir);
    return(0);
}

void ZMemcardWindowOpen(const char *pTitle, const char *pBody)
{
}

// update window text
void ZMemcardWindowUpdate(const char *pBody)
{
}

// close a window
void ZMemcardWindowClose(void)
{
}

// throw up a msgbox
void ZMemcardMsgBox(void)
{
}
