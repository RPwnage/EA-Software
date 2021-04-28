/*H********************************************************************************/
/*!
    \File zfilexenon.c

    \Description
        Basic file operations (open, read, write, close, size).

    \Notes
        Files must be on the Xbox 360 hard drive and are referenced with a leading
        d:/, with d:/ being the executable directory.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 11/18/1004 (jbrookes) First Version
    \Version 1.1 03/16/2005 (jfrank)   Updates for common sample libraries
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <xtl.h>
#include <xbox.h>
#include "platform.h"
#include "zmem.h"
#include "zlib.h"
#include "zfile.h"

/*** Defines **********************************************************************/

#define ZFILE_XENONNUMFILES_DEFAULT     (16)        //!< maximum number of file handles

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

static int32_t _bInitialized = 0;
static HANDLE _FileHandles[ZFILE_XENONNUMFILES_DEFAULT];

/*** Private Functions ************************************************************/

static void _ZFileXenonInit(void)
{
    int32_t iFile;

    // only initialize once
    if (_bInitialized)
    {
        return;
    }

    // initialize the handle list
    for (iFile = 0; iFile < ZFILE_XENONNUMFILES_DEFAULT; iFile++)
    {
        _FileHandles[iFile] = INVALID_HANDLE_VALUE;
    }

    _bInitialized = 1;
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
        int32_t             - file descriptor, or -1 if there was an error

    \Version 1.0 03/16/2005 (jfrank) First Version
    \Version 2.0 03/14/2006 (tdills) Implemented for Xenon locker & tourney API
*/
/********************************************************************************F*/
int32_t ZFileOpen(const char *pFileName, uint32_t uFlags)
{
    DWORD dwDesiredAccess = 0;
    DWORD dwCreationDisposition = OPEN_EXISTING;
    int32_t iFile;

    _ZFileXenonInit();

    // look for an empty file handle to use
    for (iFile=0; iFile<ZFILE_XENONNUMFILES_DEFAULT; iFile++)
    {
        if (INVALID_HANDLE_VALUE == _FileHandles[iFile])
        {
            // set the read/write access flags.
            if (uFlags & ZFILE_OPENFLAG_RDONLY)
            {
                dwDesiredAccess |= GENERIC_READ;
            }
            if (uFlags & ZFILE_OPENFLAG_WRONLY)
            {
                dwDesiredAccess |= GENERIC_WRITE;
            }

            // set the creation disposition flag
            if (uFlags & ZFILE_OPENFLAG_CREATE)
            {
                dwCreationDisposition = CREATE_ALWAYS;
            }

            // open the file
            _FileHandles[iFile] = CreateFile(pFileName, dwDesiredAccess, 0, 0, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, 0);

            if (INVALID_HANDLE_VALUE == _FileHandles[iFile])
            {
                // CreateFile failed.
                return(-1);
            }
            
            return(iFile);
        }
    }

    // there are no empty handle buffers.
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ZFileClose

    \Description
        Close a file

    \Input iFileId  - file descriptor
    
    \Output    int32_t  - return value from fclose()

    \Version 1.0 11/18/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ZFileClose(int32_t iFileId)
{
    _ZFileXenonInit();

    // check error conditions
    if ((iFileId < 0) || (iFileId > ZFILE_XENONNUMFILES_DEFAULT))
        return(-1);
    // see if already closed
    if (_FileHandles[iFileId] == INVALID_HANDLE_VALUE)
        return(0);

    // otherwise close it
    CloseHandle(_FileHandles[iFileId]);
    _FileHandles[iFileId] = INVALID_HANDLE_VALUE;
    return(0);
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

    \Version 1.0 11/18/2004 (jbrookes) First Version
    \Version 2.0 03/14/2006 (tdills)   Implemented for Xenon
*/
/********************************************************************************F*/
int32_t ZFileRead(int32_t iFileId, void *pData, int32_t iSize)
{
    DWORD dBytesRead = 0;

    _ZFileXenonInit();

    // check error conditions
    if ((iFileId < 0) || (iFileId > ZFILE_XENONNUMFILES_DEFAULT))
        return(-1);
    if ((pData == NULL) || (iSize < 0))
        return(-1);
    if (_FileHandles[iFileId] == INVALID_HANDLE_VALUE)
        return(-1);

    // if there are no bytes to read then return 0 (like fread)
    if (iSize == 0)
    {
        return(0);
    }

    // attempt the read
    if (!ReadFile(_FileHandles[iFileId], pData, iSize, &dBytesRead, NULL))
    {
        // there was an error reading
        DWORD dError = GetLastError();
        ZPrintf("ZFileXenon: ZFileRead ReadFile failed with error [%d==0x%X]\n", dError, dError);
        return(-1);
    }
    // return the number of bytes we actually read
    return((int32_t)dBytesRead);
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

    \Version 1.0 11/18/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ZFileWrite(int32_t iFileId, void *pData, int32_t iSize)
{
    DWORD dBytesWritten;

    _ZFileXenonInit();

    // check for errors
    if ((iFileId < 0) || (iFileId > ZFILE_XENONNUMFILES_DEFAULT))
        return(-1);
    if ((pData == NULL) || (iSize <= 0))
        return(-1);
    if (_FileHandles[iFileId] == INVALID_HANDLE_VALUE)
        return(-1);

    // otherwise, attempt a write
    if(WriteFile(_FileHandles[iFileId], pData, iSize, &dBytesWritten, NULL) == 0)
    {
        DWORD dError;
        dError = GetLastError();
        ZPrintf("ZFileXenon: ZFileWrite WriteFile failed with error [%d==0x%X]\n", dError, dError);
        return(-1);
    }

    return((int32_t)dBytesWritten);
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
        int32_t         - resultant seek location, or -1 on error

    \Version 03/16/2005 (jfrank) First Version
*/
/********************************************************************************F*/
int32_t ZFileSeek(int32_t iFileId, int32_t iOffset, uint32_t uFlags)
{
    int32_t iRet = INVALID_SET_FILE_POINTER;
    int32_t iMoveMethod = FILE_BEGIN;

    _ZFileXenonInit();

    // check for errors
    if ((iFileId < 0) || (iFileId > ZFILE_XENONNUMFILES_DEFAULT))
        return(-1);
    if (_FileHandles[iFileId] == INVALID_HANDLE_VALUE)
        return(-1);

    // convert the ZFile uFlags into SetFilePointer dwMoveMethod values
    switch (uFlags)
    {
    case ZFILE_SEEKFLAG_CUR:
        iMoveMethod = FILE_CURRENT;
        break;
    case ZFILE_SEEKFLAG_END:
        iMoveMethod = FILE_END;
        break;
    case ZFILE_SEEKFLAG_SET:
        iMoveMethod = FILE_BEGIN;
        break;
    }

    if ((iRet = SetFilePointer(_FileHandles[iFileId], iOffset, NULL, iMoveMethod)) == INVALID_SET_FILE_POINTER)
    {
        // SetFilePointer failed
        return(-1);
    }
    return(iRet);
}


/*F********************************************************************************/
/*!
    \Function ZFileDelete

    \Description
        Delete a file.

    \Input *pFileName - filename of file to delete
    
    \Output int32_t - 0=success, error code otherwise

    \Version 03/23/2005 (jfrank) First Version
*/
/********************************************************************************F*/
int32_t ZFileDelete(const char *pFileName)
{
    BOOL bResult;

    // check for errors
    if (pFileName == NULL)
        return(-1);

    // now delete the old file
    bResult = DeleteFile(pFileName);
    if(bResult == FALSE)
        return(-1);

    // success
    return(0);
}


/*F********************************************************************************/
/*!
    \Function ZFileStat

    \Description
        Get File Stat information on a file/dir.

    \Input *pFileName - filename/dir to stat
    \Input *pStat
    
    \Output int32_t - 0=success, error code otherwise

    \Version 03/25/2005 (jfrank) First Version
*/
/********************************************************************************F*/
int32_t ZFileStat(const char *pFileName, ZFileStatT *pFileStat)
{
    uint64_t ullConvert;
    WIN32_FILE_ATTRIBUTE_DATA Attribs;

    // check for errors
    if ((pFileName == NULL) || (pFileStat == NULL))
        return(-1);

    if (GetFileAttributesEx(pFileName, GetFileExInfoStandard, (void*)&Attribs))
    {
        // we're truncating the file size to fit in 32-bits.
        pFileStat->iSize = Attribs.nFileSizeLow;

        // conver the attribute flags into ZFILESTAT_MODE flags.
        pFileStat->uMode = ZFILESTAT_MODE_READ;
        if (Attribs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            pFileStat->uMode |= ZFILESTAT_MODE_DIR;
        }
        else
        {
            pFileStat->uMode |= ZFILESTAT_MODE_FILE;
        }
        if (!(Attribs.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
        {
            pFileStat->uMode |= ZFILESTAT_MODE_WRITE;
        }

        // convert the Creation Time to seconds since epoch.
        ullConvert = (((uint64_t)Attribs.ftCreationTime.dwHighDateTime) << 32) + 
                                 Attribs.ftCreationTime.dwLowDateTime;
        ullConvert /= 1000000;
        pFileStat->uTimeCreate = (uint32_t)(ullConvert & 0xffffffff);

        // convert the Last Access Time to seconds since epoch.
        ullConvert = (((uint64_t)Attribs.ftLastAccessTime.dwHighDateTime) << 32) + 
                                 Attribs.ftLastAccessTime.dwLowDateTime;
        ullConvert /= 1000000;
        pFileStat->uTimeAccess = (uint32_t)(ullConvert & 0xffffffff);

        // convert the Last Write Time to seconds since epoch.
        ullConvert = (((uint64_t)Attribs.ftLastWriteTime.dwHighDateTime) << 32) + 
                                 Attribs.ftLastWriteTime.dwLowDateTime;
        ullConvert /= 1000000;
        pFileStat->uTimeModify = (uint32_t)(ullConvert & 0xffffffff);

        return(0);
    }
    // GetFileAttributesEx failed.
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ZFileRename

    \Description
        Rename a file.

    \Input *pOldName - old name
    \Input *pNewName - new name
    
    \Output int32_t - 0=success, error code otherwise

    \Version 03/30/2005 (jfrank) First Version
    \Version 03/14/2006 (tdills) Implemented for Xenon
*/
/********************************************************************************F*/
int32_t ZFileRename(const char *pOldName, const char *pNewName)
{
    int32_t iResult;

    // check for errors
    if((pOldName == NULL) || (pNewName == NULL))
        return(-1);

    // first copy it over
    iResult = MoveFileEx(pOldName,pNewName, MOVEFILE_COPY_ALLOWED);
    if(iResult == FALSE)
        return(-1);

    // success
    return(0);
}
