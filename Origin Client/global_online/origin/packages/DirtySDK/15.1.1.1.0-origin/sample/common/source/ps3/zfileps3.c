/*H********************************************************************************/
/*!
    \File zfileps3.c

    \Description
        Basic file operations (open, read, write, close, size).

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2006. ALL RIGHTS RESERVED.

    \Version 1.0 04/14/2006 (tdills) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fs.h>
#include <cell/cell_fs.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtyerr.h"

#include "zlib.h"
#include "zfile.h"
#include "zmem.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

// for PS3, set the default device to point to c:\packages on the host machine
static char strDefaultDevice[] = "/app_home";

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _ZFilePrependPath

    \Description
        prepends the device string to the user/client supplied path if necessary.

    \Input *pInputFileName   - file name to change
    \Input *pOutputFileName  - resulting filename

    \Output
        None

    \Version 1.0 05/03/2006 (tdills) First Version
*/
/********************************************************************************F*/
static void _ZFilePrependPath(const char *pInputFileName, char *pOutputFileName)
{
    // prepend the default device name?
    if (strncmp(pInputFileName, strDefaultDevice, sizeof(strDefaultDevice)-1))
    {
        strcpy(pOutputFileName, strDefaultDevice);
        if (pInputFileName[0] != '/')
        {
            strcat(pOutputFileName, "/");
        }
        strcat(pOutputFileName, pInputFileName);
    }
    else
    {
        strcpy(pOutputFileName, pInputFileName);
    }
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function ZFileOpen

    \Description
        Open a file.

    \Input *pFileName   - file name to open
    \Input uFlags       - flags dictating how to open the file

    \Output
        ZFileT         - file descriptor, or -1 if there was an error

    \Version 1.0 04/14/2006 (tdills) First Version
*/
/********************************************************************************F*/
ZFileT ZFileOpen(const char *pFileName, uint32_t uFlags)
{
    char     strFileName[ZFILE_PATHFILE_LENGTHMAX];
    int32_t  iMode = 0;
    int32_t  iFileDesc;
    CellFsErrno Result;

    if(pFileName == NULL)
        return(-1);
    if(strlen(pFileName) == 0)
        return(-1);

    _ZFilePrependPath(pFileName, strFileName);

    // map zfile flags to PS3 Mode flags
    if (uFlags & ZFILE_OPENFLAG_RDONLY)
        iMode |= CELL_FS_O_RDONLY;
    if (uFlags & ZFILE_OPENFLAG_WRONLY)
        iMode |= CELL_FS_O_WRONLY;
    if (uFlags & ZFILE_OPENFLAG_APPEND)
        iMode |= CELL_FS_O_APPEND;
    if (uFlags & ZFILE_OPENFLAG_CREATE)
        iMode |= CELL_FS_O_CREAT;

    Result = cellFsOpen(strFileName, iMode, &iFileDesc, NULL, 0);

    if (Result == CELL_FS_SUCCEEDED)
    {
        return(iFileDesc);
    }
    else
    {
        return(-1);
    }
}

/*F********************************************************************************/
/*!
    \Function ZFileClose

    \Description
        Close a file

    \Input iFileId  - file descriptor

    \Output    int32_t  - return value from fclose()

    \Version 1.0 04/14/2006 (tdills) First Version
*/
/********************************************************************************F*/
int32_t ZFileClose(ZFileT iFileId)
{
    return(cellFsClose(iFileId));
}

/*F********************************************************************************/
/*!
    \Function ZFileRead

    \Description
        Read from a file.

    \Input *pData   - pointer to buffer to read to
    \Input iSize    - amount of data to read
    \Input iFileId  - file descriptor

    \Output
        Number of bytes read

    \Version 1.0 04/14/2006 (tdills) First Version
*/
/********************************************************************************F*/
int32_t ZFileRead(ZFileT iFileId, void *pData, int32_t iSize)
{
    uint64_t iNumBytesRead;
    CellFsErrno Result;

    Result = cellFsRead(iFileId, pData, iSize, &iNumBytesRead);

    if (Result != CELL_FS_SUCCEEDED)
        return(0);

    return(iNumBytesRead);
}

/*F********************************************************************************/
/*!
    \Function ZFileWrite

    \Description
        Write to a file.

    \Input *pData   - pointer to buffer to write from
    \Input iSize    - amount of data to write
    \Input iFileId  - file descriptor

    \Output
        Number of bytes written

    \Version 1.0 04/14/2006 (tdills) First Version
*/
/********************************************************************************F*/
int32_t ZFileWrite(ZFileT iFileId, void *pData, int32_t iSize)
{
    uint64_t iNumWritten;
    CellFsErrno Result;

    if(iFileId < 0)
        return(0);
    if((pData == NULL) || (iSize == 0))
        return(0);

    Result = cellFsWrite(iFileId, pData, iSize, &iNumWritten);

    if (Result != CELL_FS_SUCCEEDED)
    {
        return(0);
    }

    return(iNumWritten);
}

/*F********************************************************************************/
/*!
    \Function ZFileSeek

    \Description
        Seek to location in file.

    \Input iFileId  - file id to seek
    \Input iOffset  - offset to seek to
    \Input uFlags   - seek mode (ZFILE_SEEKFLAG_*)

    \Output
        int64_t         - resultant seek location, or -1 on error

    \Version 1.0 04/14/2006 (tdills) First Version
*/
/********************************************************************************F*/
int64_t ZFileSeek(ZFileT iFileId, int64_t iOffset, uint32_t uFlags)
{
    CellFsErrno iResult;
    int32_t iWence=0;
    int64_t iPos;

    if (uFlags == ZFILE_SEEKFLAG_CUR)
        iWence = CELL_FS_SEEK_CUR;
    else if (uFlags == ZFILE_SEEKFLAG_END)
        iWence = CELL_FS_SEEK_END;
    else if (uFlags == ZFILE_SEEKFLAG_SET)
        iWence = CELL_FS_SEEK_SET;

    iResult = cellFsLseek(iFileId, iOffset, iWence, &iPos);
    return((iResult == CELL_FS_SUCCEEDED) ? iPos : -1);
}


/*F********************************************************************************/
/*!
    \Function ZFileDelete

    \Description
        Delete a file.

    \Input *pFileName - filename of file to delete

    \Output int32_t - 0=success, error code otherwise

    \Version 1.0 04/14/2006 (tdills) First Version
*/
/********************************************************************************F*/
int32_t ZFileDelete(const char *pFileName)
{
    CellFsErrno Result;
    char        strFileName[ZFILE_PATHFILE_LENGTHMAX];

    // check for error conditions
    if(pFileName == NULL)
        return(-1);
    if(strlen(pFileName) == 0)
        return(-1);

    // mangle the path as necessary
    _ZFilePrependPath(pFileName, strFileName);

    // delete the file
    Result = cellFsUnlink(strFileName);

    // if it worked return 0
    if (Result == CELL_FS_SUCCEEDED)
        return(0);

    // generic delete failure
    return(ZFILE_ERROR_FILEDELETE);
}


/*F********************************************************************************/
/*!
    \Function ZFileStat

    \Description
        Get File Stat information on a file/dir.

    \Input *pFileName - filename/dir to stat
    \Input *pStat

    \Output int32_t - 0=success, error code otherwise

    \Version 1.0 04/14/2006 (tdills) First Version
*/
/********************************************************************************F*/
int32_t ZFileStat(const char *pFileName, ZFileStatT *pFileStat)
{
    CellFsStat  FileStat;
    CellFsErrno Result;
    char        strFileName[ZFILE_PATHFILE_LENGTHMAX];

    // check for error conditions
    if(pFileName == NULL)
        return(ZFILE_ERROR_FILENAME);
    if(pFileStat == NULL)
        return(ZFILE_ERROR_NULLPOINTER);

    // mangle the path as necessary
    _ZFilePrependPath(pFileName, strFileName);

    // get file status
    Result = cellFsStat(strFileName, &FileStat);

    // check for some specific errors
    if(Result == CELL_FS_ENOENT)
    {
        return(ZFILE_ERROR_NOSUCHFILE);
    }
    else if(Result != CELL_FS_SUCCEEDED)
    {
        return(ZFILE_ERROR_FILESTAT);
    }

    // clear the incoming buffer
    ZMemSet(pFileStat, 0, sizeof(ZFileStatT));
    // copy from the PC-specific structures
    pFileStat->iSize = FileStat.st_size;
    pFileStat->uTimeAccess = FileStat.st_atime;
    pFileStat->uTimeCreate = FileStat.st_ctime;
    pFileStat->uTimeModify = FileStat.st_mtime;
    // get the file modes
    if(FileStat.st_mode & CELL_FS_S_IFDIR)
        pFileStat->uMode |= ZFILESTAT_MODE_DIR;
    if(FileStat.st_mode & CELL_FS_S_IFREG)
        pFileStat->uMode |= ZFILESTAT_MODE_FILE;
    if(FileStat.st_mode & CELL_FS_S_IRUSR)
        pFileStat->uMode |= ZFILESTAT_MODE_READ;
    if(FileStat.st_mode & CELL_FS_S_IWUSR)
        pFileStat->uMode |= ZFILESTAT_MODE_WRITE;
    if(FileStat.st_mode & CELL_FS_S_IXUSR)
        pFileStat->uMode |= ZFILESTAT_MODE_EXECUTE;

    // done - return no error
    return(ZFILE_ERROR_NONE);
}

/*F********************************************************************************/
/*!
    \Function ZFileRename

    \Description
        Rename a file.

    \Input *pOldname - old name
    \Input *pNewname - new name

    \Output int32_t - 0=success, error code otherwise

    \Version 1.0 04/14/2006 (tdills) First Version

    \Notes  TODO: Needs to updated to use cellFsRename when this API becomes
        availalbe from Sony. As of 04/14/06 cellFsRename returned NOTIMP (tdills).
*/
/********************************************************************************F*/
int32_t ZFileRename(const char *pOldname, const char *pNewname)
{
    int32_t iSourceFileDesc, iDestFileDesc;
    char strBuff[1024];
    int32_t iNumBytesRead;

    if ((pOldname == NULL) || (pNewname == NULL))
    {
        return(-1);
    }

    // open the source file; return if error
    iSourceFileDesc = ZFileOpen(pOldname, ZFILE_OPENFLAG_RDONLY);
    if (iSourceFileDesc == -1)
    {
        return(-1);
    }

    // open the destination file; return if error
    iDestFileDesc = ZFileOpen(pNewname, ZFILE_OPENFLAG_CREATE | ZFILE_OPENFLAG_WRONLY);
    if (iDestFileDesc == -1)
    {
        // close the source file
        ZFileClose(iSourceFileDesc);
        return(-1);
    }

    // copy the data from source to destination
    while ((iNumBytesRead = ZFileRead(iSourceFileDesc, strBuff, sizeof(strBuff))) > 0)
    {
        ZFileWrite(iDestFileDesc, strBuff, iNumBytesRead);
    }

    // close the files
    ZFileClose(iSourceFileDesc);
    ZFileClose(iDestFileDesc);

    // delete the source file
    ZFileDelete(pOldname);

    return(0);
}

/*F********************************************************************************/
/*!
    \Function ZFileMkdir

    \Description
        Make a directory, recursively

    \Input *pPathName   - directory path to create

    \Output
        int32_t         - 0=success, error code otherwise

    \Version 01/25/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t ZFileMkdir(const char *pPathName)
{
    char strPath[1024], *pPath, cTerm = '\0';

    // copy pathname
    ds_strnzcpy(strPath, pPathName, sizeof(strPath));

    // translate any backward slashes to forward slashes
    for (pPath = strPath; *pPath != '\0'; pPath += 1)
    {
        if (*pPath == '\\')
        {
            *pPath = '/';
        }
    }

    // traverse pathname, making each directory component as we go
    for (pPath = strPath; ; pPath += 1)
    {
        if (*pPath == '/')
        {
            cTerm = *pPath;
            *pPath = '\0';
        }
        if (*pPath == '\0')
        {
            int32_t iResult;
            if ((iResult = cellFsMkdir(strPath, CELL_FS_DEFAULT_CREATE_MODE_1)) != 0)
            {
                if (iResult == EEXIST)
                {
                    ZPrintfDbg(("zfilepc: directory %s already exists\n", strPath));
                }
                else
                {
                    ZPrintfDbg(("zfileps3: could not create directory '%s' err=%s\n", strPath, DirtyErrGetName(iResult)));
                    return(-1);
                }
            }
        }
        if (cTerm != '\0')
        {
            *pPath = cTerm;
            cTerm = '\0';
        }
        if (*pPath == '\0')
        {
            break;
        }
    }
    return(0);
}

