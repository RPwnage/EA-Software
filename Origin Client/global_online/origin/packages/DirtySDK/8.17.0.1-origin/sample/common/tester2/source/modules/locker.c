/*H*************************************************************************************/
/*!
    \File    locker.c

    \Description
        Reference application for the LockerApi module.

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2004.    ALL RIGHTS RESERVED.

    \Version    1.0        11/24/04 (jbrookes) First Version
*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "platform.h"
#include "dirtysock.h"
#include "netconn.h"
#include "lobbytagfield.h"
#include "lobbyhasher.h"
#include "lobbydisplist.h"
#include "lockerapi.h"

#include "zlib.h"
#include "zfile.h"
#include "zmemcard.h"

#include "testersubcmd.h"
#include "testerregistry.h"
#include "testermodules.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct LockerAppT
{
    LockerApiRefT *pLockerApi;
    
    char strPersona[16];
    char strLKey[128];
    char    strMeta[LOCKERAPI_MAX_META_SIZE];

    int32_t iFileId;
    char aFileBuf[2][LOCKERAPI_SPOOLSIZE];
    int32_t iFileBuf;
    int32_t iFileOff;
    int32_t iFileSize;
    int32_t iXferStart;
} LockerAppT;

/*** Function Prototypes ***************************************************************/

static void _LockerCreate(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _LockerDestroy(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _LockerDir(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _LockerLDir(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _LockerLCd(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _LockerLSave(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _LockerPut(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _LockerGet(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _LockerDel(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _LockerInfo(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _LockerSort(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _LockerMeta(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _LockerSearch(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);

/*** Variables *************************************************************************/


// Private variables

static const char *_strLockerServerError[] = 
{
    "LOCKERAPI_ERROR_NONE",           //!< no error
    "LOCKERAPI_ERROR_SERVER",         //!< an unknown server error occurred
    "LOCKERAPI_ERROR_LKEYINVALID",    //!< invalid lkey
    "LOCKERAPI_ERROR_FILEEXISTS",     //!< file already exists
    "LOCKERAPI_ERROR_TOOMANYFILES",   //!< upload would exceed file quota
    "LOCKERAPI_ERROR_TOOMANYBYTES",   //!< upload would exceed byte quota
    "LOCKERAPI_ERROR_NAMEPROFANE",    //!< name would be profane
    "LOCKERAPI_ERROR_FILEMISSING",    //!< file missing on server
    "LOCKERAPI_ERROR_NOPERMISSION",   //!< no permission to access file
    "LOCKERAPI_ERROR_UNKNOWNOWNER"    //!< owner unrecognized/doesn't have a locker
};

static const char *_strLockerClientError[] =
{
    "LOCKERAPI_ERROR_CLIENT",
    "LOCKERAPI_ERROR_TIMEOUT"
};
static T2SubCmdT _Locker_Commands[] =
{
    { "create",     _LockerCreate   },
    { "destroy",    _LockerDestroy  },
    { "dir",        _LockerDir      },
    { "ldir",       _LockerLDir     },
    { "lcd",        _LockerLCd      },
    { "lsave",      _LockerLSave    },
    { "put",        _LockerPut      },
    { "get",        _LockerGet      },
    { "del",        _LockerDel      },
    { "info",       _LockerInfo     },
    { "sort",       _LockerSort     },
    { "meta",       _LockerMeta     },
    { "search",     _LockerSearch   },
    { "",           NULL            }
};

static LockerAppT _Locker_App = { NULL };

// Public variables


/*** Private Functions *****************************************************************/


/*

Locker file support functions

*/

/*F*************************************************************************************/
/*!
    \Function _LockerOpenFile
    
    \Description
        Open a memory card file.
    
    \Input *pFileName   - pointer to file name to open
    \Input uRead        - TRUE if file should be opened for reading, else FALSE
    \Input uWrite       - TRUE if file should be opened for writing, else FALSE
    \Input uCreate      - TRUE if file should be created
    \Input *pSize       - [out] storage for filesize
    
    \Output
        int32_t             - -1=failure, else file handle
            
    \Version 1.0 11/30/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
static int32_t _LockerOpenFile(const char *pFileName, uint32_t uRead, uint32_t uWrite, uint32_t uCreate, int32_t *pSize)
{
    int32_t iFileId, iFileSize=0;

    if ((uRead == TRUE) && ((iFileSize = ZMemcardFSize(pFileName)) == -1))
    {
        return(-1);
    }

    if ((iFileId = ZMemcardFOpen(pFileName, uRead, uWrite, uCreate)) == -1)
    {
        return(-1);
    }

    *pSize = iFileSize;
    return(iFileId);
}

/*F*************************************************************************************/
/*!
    \Function _LockerReadFileChunkAsync
    
    \Description
        Issue an async read for the next LOCKERAPI_SPOOLSIZE bytes of current file.
    
    \Input *pApp        - pointer to locker app
    \Input iLastRead    - amount of data that was previously read
    
    \Output int32_t         - zero=success, negative=failure
            
    \Version 1.0 11/30/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
static int32_t _LockerReadFileChunkAsync(LockerAppT *pApp, int32_t iLastRead)
{
    int32_t iReadSize = LOCKERAPI_SPOOLSIZE;

    pApp->iFileOff += iLastRead;
    if (ZMemcardFSeek(pApp->iFileId, pApp->iFileOff) < 0)
    {
        return(-1);
    }

    if ((pApp->iFileOff + iReadSize) > pApp->iFileSize)
    {
        iReadSize = pApp->iFileSize - pApp->iFileOff;
    }

    if (ZMemcardFReadAsync(pApp->aFileBuf[pApp->iFileBuf], iReadSize, pApp->iFileId) < 0)
    {
        return(-1);
    }
    return(0);
}

/*F*************************************************************************************/
/*!
    \Function _LockerWriteFileChunkAsync
    
    \Description
        Display files in current locker.
    
    \Input *pApp    - pointer to locker app
    
    \Output int32_t     - zero=success, negative=failure
            
    \Version 1.0 11/30/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
static int32_t _LockerWriteFileChunkAsync(LockerAppT *pApp, int32_t iWriteSize)
{
    if (ZMemcardFSeek(pApp->iFileId, pApp->iFileOff) < 0)
    {
        return(-1);
    }
    if (ZMemcardFWriteAsync(pApp->aFileBuf[pApp->iFileBuf], iWriteSize, pApp->iFileId) < 0)
    {
        return(-1);
    }
    pApp->iFileOff += iWriteSize;
    return(0);
}

/*F*************************************************************************************/
/*!
    \Function _LockerPrintLockerFile
    
    \Description
        Display info for given file
    
    \Input *pApp    - pointer to locker app
    \Input iFileId  - fileId to display info for
    
    \Output None.
            
    \Version 1.0 11/30/04 (jbrookes) First Version
    \Version 1.1 03/15/06 (tdills)   On non-PS2 platforms ctime is overloaded and returned
                                     an integer which would cause an unhandled exception when
                                     ZPrintf attempted to print a string.
*/
/**************************************************************************************F*/
static void _LockerPrintLockerFile(LockerAppT *pApp, int32_t iFileId)
{
    const LockerApiDirectoryT *pDirectory;
    const LockerApiFileT *pFile;

    if ((pDirectory = LockerApiGetDirectory(pApp->pLockerApi)) != NULL)
    {
        pFile = &pDirectory->Files[iFileId];
        ZPrintf("    name: %s\n", pFile->Info.strName);
        ZPrintf("    desc: %s\n", pFile->Info.strDesc);
        ZPrintf("    type: %s\n", pFile->Info.strType);
        ZPrintf("    size: %d\n", pFile->Info.iNetSize);
        ZPrintf("    locs: %d\n", pFile->Info.iLocSize);
        #if (DIRTYCODE_PLATFORM != DIRTYCODE_PS3)
        {
            time_t time = pFile->iDate;
            char* pstr = ctime(&time);
            ZPrintf("    date: %s", pstr);
        }
        #else // not available on ps2?
        ZPrintf("    date: %d", pFile->iDate);
        #endif
        ZPrintf("    attr: %d\n", pFile->Info.iAttr);
    }
    else
    {
        ZPrintf("locker: no directory available\n");
    }
}

/*F*************************************************************************************/
/*!
    \Function _LockerPrintLocker
    
    \Description
        Display files in current locker.
    
    \Input *pApp    - pointer to locker app
    
    \Output None.
            
    \Version 1.0 11/30/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _LockerPrintLocker(LockerAppT *pApp)
{
    const LockerApiDirectoryT *pDirectory;

    if ((pDirectory = LockerApiGetDirectory(pApp->pLockerApi)) != NULL)
    {
        int32_t iFile;

        ZPrintf("locker: directory for user '%s'\n\n", pDirectory->strOwner);
        ZPrintf("Files: %d/%d\n", pDirectory->iNumFiles, pDirectory->iMaxFiles);
        ZPrintf("Storage: %d/%d kbytes (%2.f%%)\n", pDirectory->iNumBytes/1024, pDirectory->iMaxBytes/1024,
            (((float)pDirectory->iNumBytes * 100.0f)/(float)pDirectory->iMaxBytes));
        ZPrintf("File List:\n");
        
        if (pDirectory->iNumFiles > 0)
        {
            for (iFile = 0; iFile < pDirectory->iNumFiles; iFile++)
            {
                ZPrintf(" %s  %2d) %7d/%7d %s \n",pDirectory->Files[iFile].Info.strIdnt,
                    iFile,
                    pDirectory->Files[iFile].Info.iNetSize,
                    pDirectory->Files[iFile].Info.iLocSize,
                    pDirectory->Files[iFile].Info.strName);
            }
            ZPrintf("\n");
        }
        else
        {
            ZPrintf("  No files\n");
        }
    }
    else
    {
        ZPrintf("locker: no directory available\n");
    }
}

/*F*************************************************************************************/
/*!
    \Function _LockerPrintMemcard
    
    \Description
        Display files currently on memcard.
    
    \Input *pApp    - pointer to locker app
    
    \Output None.
            
    \Version 1.0 11/30/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _LockerPrintMemcard(LockerAppT *pApp)
{
    ZMemcardDirT Directory;
    char strSize[32];
    
    // get directory
    if (ZMemcardFDir(&Directory) > 0)
    {
        int32_t iFile;
        
        ZPrintf("memcard: %d files\n");
        for (iFile = 0; iFile < Directory.iNumFiles; iFile++)
        {
            sprintf(strSize, "%d", Directory.Files[iFile].iSize);
            ZPrintf("%7s %s\n",
                (Directory.Files[iFile].iSize == 0) ? "<dir>" : strSize,
                Directory.Files[iFile].strName);
        }
        ZPrintf("\n");
    }
}

/*F*************************************************************************************/
/*!
    \Function _LockerXferWin
    
    \Description
        Manage transfer display window.
    
    \Input *pTitle      - pointer to window title
    \Input *pUpDownStr  - pointer to transfer string
    \Input *pFileName   - pointer to filename
    \Input iBytes       - number of bytes transferred
    \Input  iTiotalBytes- total number of bytes to transfer
    
    \Output None.
            
    \Version 1.0 11/30/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _LockerXferWin(const char *pTitle, const char *pUpDownStr, const char *pFileName, int32_t iBytes, int32_t iTotalBytes)
{
    char strBody[256];

    sprintf(strBody, 
        "%s %s.\n\n"
        "Please do not remove the memory card (PS2) in MEMORY CARD slot 1.\n\n"
        "........................\n",
        pUpDownStr,
        pFileName);

    if (iBytes != 0)
    {
        char *pDots = strstr(strBody, "..");
        int32_t iDot, nDots;
        
        nDots = (24 * iBytes) / iTotalBytes;
        for (iDot = 0; iDot < nDots; iDot++)
        {
            pDots[iDot] = 'o';
        }
        ZMemcardWindowUpdate(strBody);
    }
    else
    {
        ZMemcardWindowOpen(pTitle, strBody);
    }
}

/*F*************************************************************************************/
/*!
    \Function _LockerRequestDone
    
    \Description
        Handle processing on locker request completion.
    
    \Input *pApp    - pointer to locker app
    \Input iSuccess - TRUE if result was successful, else FALSE
    
    \Output None.
            
    \Version 1.0 11/30/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _LockerRequestDone(LockerAppT *pApp, int32_t iSuccess)
{
    // sync any open memcard operation
    ZMemcardFSync(pApp->iFileId);

    // if there's a file open, close it
    if (pApp->iFileId != -1)
    {
        ZMemcardFClose(pApp->iFileId);
        pApp->iFileId = -1;
    }
    
    // close transfer window, if any
    ZMemcardWindowClose();

    // did the transaction succeed?
    if (iSuccess)
    {
        // display transfer stats?
        if (pApp->iXferStart != 0)
        {
            int32_t iElapsedTime = NetTick() - pApp->iXferStart;
            ZPrintf("locker: transaction success - xfer %d bytes in %2.2f seconds (%2.2fk/sec)\n",
                pApp->iFileSize,
                (float)iElapsedTime / 1000.0f,
                ((float)pApp->iFileSize * 1000.0f) / ((float)iElapsedTime * 1024.0f));
        }
        else
        {    
            ZPrintf("locker: transaction success\n");
        }
    }
    else
    {
        ZPrintf("locker: transaction failure\n");
    }
    
    // clear stuff
    pApp->iFileOff = 0;
    pApp->iFileSize = 0;
    pApp->iXferStart = 0;
}

/*F*************************************************************************************/
/*!
    \Function _LockerAbort
    
    \Description
        Abort a get/put operation.
    
    \Input *pApp    - pointer to locker app
    
    \Output None.
            
    \Version 1.0 12/01/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _LockerAbort(LockerAppT *pApp)
{
    LockerApiAbort(pApp->pLockerApi);
    ZMemcardMsgBox();
    ZMemcardWindowClose();
    ZPrintf("locker: operation terminated due to memory card error\n");
}

/*F*************************************************************************************/
/*!
    \Function _LockerCallback
    
    \Description
        LockerApi user callback.
    
    \Input *pLockerApi  - pointer to lockerapi
    \Input eEvent       - callback event type
    \Input *pFile       - pointer to current file, or NULL
    \Input *pUserData   - user callback data
    
    \Output None.
            
    \Version 1.0 11/30/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _LockerCallback(LockerApiRefT *pLockerApi, LockerApiEventE eEvent, LockerApiFileT *pFile, void *pUserData)
{
    LockerAppT *pApp = (LockerAppT *)pUserData;
    int32_t iSize;

    switch(eEvent)
    {
        case LOCKERAPI_EVENT_DIRECTORY:
        {
            _LockerPrintLocker(pApp);

            _LockerRequestDone(pApp, TRUE);
        }
        break;

        case LOCKERAPI_EVENT_WRITEDATA:
        {
            // make sure previous memcard read is complete
            ZMemcardFSync(pApp->iFileId);

            // write data to network
            if ((iSize = LockerApiWriteData(pApp->pLockerApi, pApp->aFileBuf[pApp->iFileBuf])) > 0)
            {
                // initiate a new async memcard read
                pApp->iFileBuf = !pApp->iFileBuf;
                if (_LockerReadFileChunkAsync(pApp, iSize) < 0)
                {
                    _LockerAbort(pApp);
                }
                else
                {
                    _LockerXferWin("File Upload", "Uploading file", pFile->Info.strName, pApp->iFileOff, pApp->iFileSize);
                }
            }
        }
        break;

        case LOCKERAPI_EVENT_READDATA:
        {
            // make sure previous memcard write is complete
            ZMemcardFSync(pApp->iFileId);

            // read data from network
            if ((iSize = LockerApiReadData(pApp->pLockerApi, pApp->aFileBuf[pApp->iFileBuf])) > 0)
            {
                // initiate a new async memcard write
                if (_LockerWriteFileChunkAsync(pApp, iSize) < 0)
                {
                    _LockerAbort(pApp);
                }
                else
                {
                    _LockerXferWin("File Download", "Downloading file", pFile->Info.strName, pApp->iFileOff, pApp->iFileSize);
                }
                
                pApp->iFileBuf = !pApp->iFileBuf;
            }
        }
        break;

        case LOCKERAPI_EVENT_ERROR:
        {
            LockerApiErrorE eError;

            _LockerRequestDone(pApp, FALSE);

            // print error
            eError = LockerApiGetLastError(pApp->pLockerApi);
            if (eError < LOCKERAPI_ERROR_CLIENT)
            {
                ZPrintf("locker: got error %s\n", _strLockerServerError[eError]);
            }
            else
            {
                ZPrintf("locker: got error %s\n", _strLockerClientError[eError-LOCKERAPI_ERROR_CLIENT]);
            }
        }
        break;

        case LOCKERAPI_EVENT_DONE:
        {
            _LockerRequestDone(pApp, TRUE);
        }
        break;

        default:
            ZPrintf("locker: unhandled event %d\n", eEvent);
    }
}

/*F*************************************************************************************/
/*!
    \Function _LockerCreate
    
    \Description
        Locker subcommand - create locker
    
    \Input *pApp    - pointer to locker app
    \Input argc     - argument count
    \Input **argv   - argument list
    
    \Output None.
            
    \Version 1.0 11/30/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _LockerCreate(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    LockerAppT *pApp = (LockerAppT *)_pApp;
    char *pLockerServer = "http://sdevbesl01.online.ea.com/fileupload/locker2.jsp";

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s create <servername>\n", argv[0]);
        return;
    }

    // make sure we don't already have a ref
    if (pApp->pLockerApi != NULL)
    {
        ZPrintf("   %s: ref already created\n", argv[0]);
        return;
    }
    
    // get stuff we need from lobby
    strnzcpy(pApp->strLKey, "CHANGE ME", sizeof(pApp->strLKey));
    strnzcpy(pApp->strPersona, "CHANGE ME", sizeof(pApp->strPersona));
    
    // init memcard
    ZMemcardInit();

    // create api
    pApp->pLockerApi = LockerApiCreate(pLockerServer, _LockerCallback, pApp);
    pApp->iFileId = -1;

    // set parms
    LockerApiSetLoginInfo(pApp->pLockerApi, pApp->strLKey, pApp->strPersona);

    // override default server?
    if (argc == 3)
    {
        ZPrintf("   %s: using locker server '%s'\n", argv[0], argv[2]);
        pLockerServer = argv[2];
    }
}

/*F*************************************************************************************/
/*!
    \Function _LockerDestroy
    
    \Description
        Locker subcommand - destroy locker module
    
    \Input *pApp    - pointer to locker app
    \Input argc     - argument count
    \Input **argv   - argument list
    
    \Output None.
            
    \Version 1.0 11/30/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _LockerDestroy(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    LockerAppT *pApp = (LockerAppT *)_pApp;
    
    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s destroy\n", argv[0]);
        return;
    }

    // destroy lockerapi
    if (pApp->pLockerApi)
    {
        LockerApiDestroy(pApp->pLockerApi);
        pApp->pLockerApi = NULL;
    }
}

/*F*************************************************************************************/
/*!
    \Function _LockerDir
    
    \Description
        Locker subcommand - display current locker
    
    \Input *pApp    - pointer to locker app
    \Input argc     - argument count
    \Input **argv   - argument list
    
    \Output None.
            
    \Version 1.0 11/30/04 (jbrookes) First Version
    \Version 2.0 03/14/07 (bcox) Updated to use LockerApiFetchLocker2
*/
/**************************************************************************************F*/
static void _LockerDir(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    LockerAppT *pApp = (LockerAppT *)_pApp;
    
   if ((bHelp == TRUE) || ((argc != 2) && (argc != 4)))
    {
        ZPrintf("   usage: %s dir2 [name=<name>] [game=<gamekey>]\n", argv[0]);
        ZPrintf("   usage: gamekey is usually your domain parition you receive\n");
        ZPrintf("   usage: from the server at login\n");
        return;
    }

    if (argc == 2)
    {
        _LockerPrintLocker(pApp);
    }

    if (argc == 4)
    {
        ZPrintf("%s: fetching locker for user '%s' using gamekey '%s'\n", argv[0], argv[2], argv[3]);
        LockerApiFetchLocker(pApp->pLockerApi, argv[2], argv[3]);
    }
}

/*F*************************************************************************************/
/*!
    \Function _LockerLDir
    
    \Description
        Locker subcommand - get current directory info
    
    \Input *pApp    - pointer to locker app
    \Input argc     - argument count
    \Input **argv   - argument list
    
    \Output None.
            
    \Version 1.0 11/30/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _LockerLDir(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    LockerAppT *pApp = (LockerAppT *)_pApp;
    
    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s ldir\n", argv[0]);
        return;
    }

    _LockerPrintMemcard(pApp);
}

/*F*************************************************************************************/
/*!
    \Function _LockerLCd
    
    \Description
        Locker subcommand - execute a local (memcard) directory change
    
    \Input *pApp    - pointer to locker app
    \Input argc     - argument count
    \Input **argv   - argument list
    
    \Output None.
            
    \Version 1.0 11/30/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _LockerLCd(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    int32_t iResult;
    
    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s lcd <directory>\n", argv[0]);
        return;
    }

    iResult = ZMemcardFCd(argv[2]);
    ZPrintf("%s: chdir to '%s' %s\n", argv[0], argv[2],
        (iResult >= 0) ? "succeeded" : "failed");
}

/*F*************************************************************************************/
/*!
    \Function _LockerLSave
    
    \Description
        Locker subcommand - save a local (memcard) file to host.
    
    \Input *pApp    - pointer to locker app
    \Input argc     - argument count
    \Input **argv   - argument list
    
    \Output None.
            
    \Version 1.0 11/30/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _LockerLSave(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    LockerAppT *pApp = (LockerAppT *)_pApp;
    int32_t iMemId, iFileOff, iFileSize, iReadSize;
    // $hack code start$ by Jef Frank 04/04/2005
    //int32_t iHostId;
    // $hack code end$
    
    if ((bHelp == TRUE) || (argc != 4))
    {
        ZPrintf("   usage: %s lsave <memfile> <hostfile>\n", argv[0]);
        return;
    }

    // open input file
    if ((iMemId = _LockerOpenFile(argv[2], TRUE, FALSE, FALSE, &iFileSize)) != -1)
    {
        // open output file for writing
        // 
        // $hack code start$ by Jef Frank 04/04/2005
        //if ((iHostId = ZFileOpen(argv[3], FALSE, TRUE, TRUE)) != -1)
        // $hack code end$
        {
            // copy from memcard to host file
            for (iFileOff = 0; iFileOff < iFileSize; iFileOff += iReadSize)
            {
                // calculate read size
                if ((iReadSize = iFileSize - iFileOff) > (signed)sizeof(pApp->aFileBuf[0]))
                {
                    iReadSize = sizeof(pApp->aFileBuf[0]);
                }

                // read from memcard file
                ZMemcardFSeek(iMemId, iFileOff);
                ZMemcardFReadAsync(pApp->aFileBuf[0], sizeof(pApp->aFileBuf[0]), iMemId);
                ZMemcardFSync(iMemId);

                // write to host buffer
                // $hack code start$ by Jef Frank 04/04/2005
                //ZFileSeek(iFileOff, iHostId);
                // $hack code end$
                // $hack code start$ by Jef Frank 04/04/2005
                //ZFileWrite(pApp->aFileBuf[0], iReadSize, iHostId);
                // $hack code end$
            }

            ZPrintf("%s: file successfully saved\n", argv[0]);
            // $hack code start$ by Jef Frank 04/04/2005
            //ZFileClose(iHostId);
            // $hack code end$
        }
        // $hack code start$ by Jef Frank 04/04/2005    
        //else
        // $hack code end$
        {
            ZPrintf("_LockerLSave: unable to open host file '%s'\n", argv[3]);
        }

        ZMemcardFClose(iMemId);
    }
    else
    {
        ZPrintf("_LockerLSave: unable to open memcard file '%s'\n", argv[2]);
    }
}

/*F*************************************************************************************/
/*!
    \Function _LockerPut
    
    \Description
        Locker subcommand - upload a file.
    
    \Input *pApp    - pointer to locker app
    \Input argc     - argument count
    \Input **argv   - argument list
    
    \Output None.
            
    \Version 1.0 11/30/04 (jbrookes) First Version
    \Version 1.1 03/15/06 (tdills)   Added desc to parameters for testing filtering
*/
/**************************************************************************************F*/
static void _LockerPut(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    LockerAppT *pApp = (LockerAppT *)_pApp;
    LockerApiFileInfoT FileInfo;
    char *pUsername = NULL;
    uint32_t bForce = FALSE;
    char *pDesc = NULL;

    if ((bHelp == TRUE) || (argc < 3) || (argc > 5))
    {
        ZPrintf("   usage: %s put [filename] <username | public | force> [desc=<desc>]\n", argv[0]);
        return;
    }

    if (argc == 5)
    {
        if (!strncmp(argv[4], "desc=", sizeof("desc=")-1))
        {
            pDesc = argv[4];
            NetPrintf(("locker: desc=%s\n", pDesc));
        }
    }
    // try and open file for reading
    if ((pApp->iFileId = _LockerOpenFile(argv[2], TRUE, FALSE, FALSE, &pApp->iFileSize)) != -1)
    {
        _LockerXferWin("File Upload", "Uploading file", argv[2], pApp->iFileOff, pApp->iFileSize);

        // set up for upload
        strcpy(FileInfo.strName, argv[2]);
        if (pDesc != NULL)
        {
            strcpy(FileInfo.strDesc, pDesc);
        }
        else
        {
            strcpy(FileInfo.strDesc, "This is a test of the emergency broadcast system.  The broadcasters in your area, in cooperation with federal and state autho");
        }
        
        strcpy(FileInfo.strType, "jpg");
        FileInfo.iNetSize = pApp->iFileSize;
        FileInfo.iLocSize = pApp->iFileSize + 100;
        FileInfo.iAttr = 0;

        if (argc == 4)
        {
            if (!strcmp(argv[3], "public"))
            {
                FileInfo.iAttr = LOCKERAPI_FILEATTR_PUBLIC;
            }
            else if (!strcmp(argv[3], "force"))
            {
                bForce = TRUE;
            }
            else
            {
                pUsername = argv[3];
            }
        }

        // start reading first chunk from memcard
        _LockerReadFileChunkAsync(pApp, 0);

        // start the upload
        pApp->iXferStart = NetTick();
        LockerApiPutFile(pApp->pLockerApi, &FileInfo, pUsername, bForce, pApp->strMeta);
    }
    else
    {
        ZPrintf("%s: unable to open file '%s'\n", argv[0], argv[2]);
    }
}

/*F*************************************************************************************/
/*!
    \Function _LockerGet
    
    \Description
        Locker subcommand - get a file.
    
    \Input *pApp    - pointer to locker app
    \Input argc     - argument count
    \Input **argv   - argument list
    
    \Output None.
            
    \Version 1.0 11/30/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _LockerGet(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    const LockerApiDirectoryT *pDirectory;
    LockerAppT *pApp = (LockerAppT *)_pApp;
    int32_t iFileId;
    
    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s get <fileId>\n", argv[0]);
        return;
    }

    if ((pDirectory = LockerApiGetDirectory(pApp->pLockerApi)) == NULL)
    {
        ZPrintf("%s: no directory\n", argv[0]);
        return;
    }

    if ((iFileId = strtol(argv[2], NULL, 10)) < pDirectory->iNumFiles)
    {
        ZPrintf("%s: fileid %d does not exist\n", argv[0], iFileId);
    }

    ZPrintf("%s: downloading file #%d (%s)\n", argv[0], iFileId, pDirectory->Files[iFileId].Info.strName);

    // try and open file for writing
    if ((pApp->iFileId = _LockerOpenFile(pDirectory->Files[iFileId].Info.strName, FALSE, TRUE, TRUE, &pApp->iFileSize)) != -1)
    {
        pApp->iFileSize = pDirectory->Files[0].Info.iNetSize;
        _LockerXferWin("File Download", "Downloading file", pDirectory->Files[iFileId].Info.strName, pApp->iFileOff, pApp->iFileSize);
        
        // start the download
        pApp->iXferStart = NetTick();
        LockerApiGetFile(pApp->pLockerApi, iFileId);
    }
    else
    {
        ZPrintf("%s: unable to open file '%s'\n", argv[0], pDirectory->Files[iFileId].Info.strName);
    }
}

/*F*************************************************************************************/
/*!
    \Function _LockerDel
    
    \Description
        Locker subcommand - delete a file.
    
    \Input *pApp    - pointer to locker app
    \Input argc     - argument count
    \Input **argv   - argument list
    
    \Output None.
            
    \Version 1.0 11/30/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _LockerDel(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    LockerAppT *pApp = (LockerAppT *)_pApp;
    
    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s del <fileId>\n", argv[0]);
        return;
    }

    ZPrintf("%s: deleting file #%s\n", argv[0], argv[2]);
    LockerApiDeleteFile(pApp->pLockerApi, strtol(argv[2], NULL, 10));
}

/*F*************************************************************************************/
/*!
    \Function _LockerInfo
    
    \Description
        Locker subcommand - get/set info on file.
    
    \Input *pApp    - pointer to locker app
    \Input argc     - argument count
    \Input **argv   - argument list
    
    \Output None.
            
    \Version 1.0 11/30/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _LockerInfo(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    LockerAppT *pApp = (LockerAppT *)_pApp;
    int32_t iFileId;

    if ((bHelp == TRUE) || (argc < 3) || (argc > 5))
    {
        ZPrintf("   usage: %s info fileId [desc=<desc> attr=<attr>]\n", argv[0]);
        return;
    }

    iFileId = strtol(argv[2], NULL, 10);

    if (argc == 3)
    {
        _LockerPrintLockerFile(pApp, iFileId);
    }
    else
    {
        char *pDesc = NULL;
        int32_t iAttr = -1, iPerm = -1;
        int32_t iArg;

        for (iArg = 3; iArg < argc; iArg++)
        {
            if (!strncmp(argv[iArg], "desc=", sizeof("desc=")-1))
            {
                pDesc = &(argv[iArg][5]); // skip over the 'desc=' tag.
                NetPrintf(("locker: desc=%s\n", pDesc));
            }

            if (!strncmp(argv[iArg], "attr=", sizeof("attr=")-1))
            {
                iAttr = strtol(argv[iArg], NULL, 10);
                NetPrintf(("locker: attr=%d\n", iAttr));
            }
            
            if (!strncmp(argv[iArg], "perm=", sizeof("perm=")-1))
            {
                iPerm = strtol(argv[iArg], NULL, 10);
                NetPrintf(("locker: perm=%d\n", iPerm));
            }
        }

        LockerApiUpdateFileInfo(pApp->pLockerApi, iFileId, pDesc, iAttr, iPerm, pApp->strMeta);
    }
}

/*F*************************************************************************************/
/*!
    \Function _LockerSort
    
    \Description
        Locker subcommand - sort current directory
    
    \Input *pApp    - pointer to locker app
    \Input argc     - argument count
    \Input **argv   - argument list
    
    \Output None.
            
    \Version 1.0 11/30/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _LockerSort(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    LockerAppT *pApp = (LockerAppT *)_pApp;
    int32_t iCategory, iSortDir = LOCKERAPI_SORTDIR_ASCENDING;
    char *pArg;

    static const char *_strCategories[LOCKERAPI_NUMCATEGORIES] =
    {
        "name",
        "game",
        "type",
        "size",
        "date",
        "attr"
    };

    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s sort [-][ name | game | type | size | date | attr ]\n", argv[0]);
        return;
    }

    // check to see if they want to reverse directions
    pArg = argv[2];
    if (*pArg == '-')
    {
        pArg++;
        iSortDir = LOCKERAPI_SORTDIR_DESCENDING;
    }

    // determine category
    for (iCategory = 0; iCategory < LOCKERAPI_NUMCATEGORIES; iCategory++)
    {
        if (!strcmp(pArg, _strCategories[iCategory]))
        {
            break;
        }
    }

    // do the sort?
    if (iCategory < LOCKERAPI_NUMCATEGORIES)
    {
        LockerApiSortDirectory(pApp->pLockerApi, (LockerApiCategoryE)iCategory, iSortDir);
    }
    else
    {
        ZPrintf("%s: unrecognized sort type '%s'\n", argv[3]);
    }
}


/*F********************************************************************************/
/*!
    \function _LockerMeta
    
    \Description 
        Sets the meta data value to be used when doing search, put and modify
    
    \Input pApp    - pointer to locker app
    \Input argc     - argument count
    \Input argv   - argument list
    \Input bHelp    - show help text for this command
    
    \Output - None
    
    \Version 02/06/2007 (ozietman)
*/ 
/********************************************************************************F*/
static void _LockerMeta(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    LockerAppT *pApp = (LockerAppT *)_pApp;
    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("   usage: %s meta  [meta data]\n", argv[0]);
        return;
    }
    strnzcpy(pApp->strMeta, argv[2], sizeof(pApp->strMeta));
}

/*F********************************************************************************/
/*!
    \function _LockerSearch
    
    \Description 
        Search files using meta data 
    
    \Input pApp    - pointer to locker app
    \Input argc     - argument count
    \Input argv   - argument list
    \Input bHelp    - show help text for this command
    
    \Output - None
    
    \Version 02/06/2007 (ozietman)
*/ 
/********************************************************************************F*/
static void _LockerSearch(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    LockerAppT *pApp = (LockerAppT *)_pApp;

    // meta data is empty 
    if (pApp->strMeta[0] == 0)
    {
        ZPrintf("   usage: before calling search meta please set meta data by using %s meta command\n", argv[1]);
        return;
    }
    LockerApiMetaSearch(pApp->pLockerApi, pApp->strMeta);
}



/*F*************************************************************************************/
/*!
    \Function _CmdDownloadCb
    
    \Description
        Locker idle callback.  This callback function exists only to clean up after
        locker when tester exits.
    
    \Input *argz    - unused
    \Input argc     - argument count
    \Input **argv   - argument list
    
    \Output
        int32_t         - zero
            
    \Version 1.0 12/14/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
static int32_t _CmdLockerCb(ZContext *argz, int32_t argc, char **argv)
{
    if (argc == 0)
    {
        // shut down locker
        _LockerDestroy(&_Locker_App, 0, NULL, FALSE);
        return(0);
    }

    return(ZCallback(_CmdLockerCb, 1000));
}


/*** Public Functions ******************************************************************/


/*F*************************************************************************************/
/*!
    \Function    CmdLocker
    
    \Description
        Locker command.
    
    \Input *argz    - unused
    \Input argc     - argument count
    \Input **argv   - argument list
    
    \Output
        int32_t         - zero
            
    \Version 1.0 11/24/04 (jbrookes) First Version
*/
/**************************************************************************************F*/
int32_t CmdLocker(ZContext *argz, int32_t argc, char **argv)
{
    T2SubCmdT *pCmd;
    LockerAppT *pApp = &_Locker_App;
    unsigned char bHelp, bCreate = FALSE;

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_Locker_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   get, update, and otherwise manage an ea locker\n");
        T2SubCmdUsage(argv[0], _Locker_Commands);
        return(0);
    }

    // if no ref yet, make one
    if ((pApp->pLockerApi == NULL) && strcmp(pCmd->strName, "create"))
    {
        char *pCreate = "create";
        ZPrintf("   %s: ref has not been created - creating\n", argv[0]);
        _LockerCreate(pApp, 1, &pCreate, bHelp);
        bCreate = TRUE;
    }

    // hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

    // if we executed create, remember
    if (pCmd->pFunc == _LockerCreate)
    {
        bCreate = TRUE;
    }

    // if we executed create, install periodic callback
    return((bCreate == TRUE) ? ZCallback(_CmdLockerCb, 1000) : 0);
}
