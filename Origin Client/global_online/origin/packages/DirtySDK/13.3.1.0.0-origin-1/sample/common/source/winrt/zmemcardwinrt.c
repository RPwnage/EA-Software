/*H********************************************************************************/
/*!
    \File ZMemcardpc.c

    \Description
        Basic file operations (open, read, write, close, size).

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 11/18/1004 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "platform.h"
#include "zmemcard.h"

/*** Defines **********************************************************************/

// memory card directory
#define ZMEMCARD_DIR "c:\\temp\\ZMemcard\\"

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

static const char *_ZMemcardMakePath(const char *pFileName)
{
    static char strPath[4096];

    strcpy(strPath, ZMEMCARD_DIR);
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

    \Version 1.0 11/18/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ZMemcardInit(void)
{
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
    
    \Output
        int32_t             - file descriptor, or -1 if there was an error

    \Version 1.0 11/18/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
ZFileT ZMemcardFOpen(const char *pFileName, uint32_t bRead, uint32_t bWrite, uint32_t bCreate)
{
    char strOpts[16] = "";
    FILE *pFile;

    if (bRead)
    {
        strcat(strOpts, "r");
    }

    if (bWrite)
    {
        strcat(strOpts, "w");
    }

    strcat(strOpts, "b");

    if ((pFile = fopen(_ZMemcardMakePath(pFileName), strOpts)) != NULL)
    {
        return((ZFileT)pFile);
    }
    else
    {
        return(-1);
    }
}

/*F********************************************************************************/
/*!
    \Function ZMemcardFClose

    \Description
        Close a file

    \Input iFileId  - file descriptor
    
    \Output
        None.

    \Version 1.0 11/18/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void ZMemcardFClose(ZFileT iFileId)
{
    fclose((FILE *)iFileId);
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

    \Version 1.0 11/18/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ZMemcardFSeek(ZFileT iFileId, int32_t iOffset)
{
    return(fseek((FILE *)iFileId, iOffset, SEEK_SET));
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
        Win32 version is synchronous.

    \Version 1.0 11/18/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ZMemcardFReadAsync(void *pData, int32_t iSize, ZFileT iFileId)
{
    return((int32_t)fread(pData, 1, iSize, (FILE *)iFileId));
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
        Win32 version is synchronous.

    \Version 1.0 11/18/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ZMemcardFWriteAsync(void *pData, int32_t iSize, ZFileT iFileId)
{
    return((int32_t)fwrite(pData, iSize, 1, (FILE *)iFileId));
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
int32_t ZMemcardFSync(ZFileT iFileId)
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

    \Version 1.0 11/18/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ZMemcardFSize(const char *pFileName)
{
    int32_t iFileSize = -1;
    struct stat st;

    if (stat(_ZMemcardMakePath(pFileName), &st) >= 0)
    {
        iFileSize = st.st_size;
    }
    return(iFileSize);
}

/*F********************************************************************************/
/*!
    \Function ZMemcardFDir

    \Description
        Get a memory card's directory

    \Input *pFileName   - pointer to filename to get size of
    
    \Output
        int32_t             - zero=success, negative=failure

    \Version 1.0 11/18/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ZMemcardFDir(ZMemcardDirT *pDirectory)
{
    int32_t iFileCt = 0;

#if 0   // $todo
    WIN32_FIND_DATA wfd;
    HANDLE hFind;

    // Start the find and check for failure.
    if ((hFind = FindFirstFile( ZMEMCARD_DIR "\\*", &wfd)) != INVALID_HANDLE_VALUE)
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
#endif

    pDirectory->iNumFiles = iFileCt;
    return(iFileCt);
}

int32_t ZMemcardFCd(const char *pDirName)
{
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
