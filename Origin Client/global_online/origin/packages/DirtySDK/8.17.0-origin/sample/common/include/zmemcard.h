/*H********************************************************************************/
/*!
    \File ZMemcard.h

    \Description
        Basic file operations (open, read, write, close, size, dir).

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 11/24/2004 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _ZMemcard_h
#define _ZMemcard_h

/*** Include files ****************************************************************/

#include "zfile.h"

/*** Defines **********************************************************************/

#define ZMEMCARD_MAXFILES   (64)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct ZMemcardFileT
{
    char strName[32];
    int32_t  iSize;
} ZMemcardFileT;

typedef struct ZMemcardDirT
{
    int32_t iNumFiles;
    ZMemcardFileT Files[ZMEMCARD_MAXFILES];
} ZMemcardDirT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// init module
int32_t ZMemcardInit(void);

// open a file
ZFileT ZMemcardFOpen(const char *pFileName, uint32_t bRead, uint32_t bWrite, uint32_t bCreate);

// close a file
void ZMemcardFClose(ZFileT iFileId);

// seek to a place in file
int32_t ZMemcardFSeek(ZFileT iFileId, int32_t iOffset);

// read from a file
int32_t ZMemcardFReadAsync(void *pData, int32_t iSize, ZFileT iFileId);

// write to a file
int32_t ZMemcardFWriteAsync(void *pData, int32_t iSize, ZFileT iFileId);

// sync last op
int32_t ZMemcardFSync(ZFileT iFileId);

// get size of file
int32_t ZMemcardFSize(const char *pFileName);

// get memory card directory
int32_t ZMemcardFDir(ZMemcardDirT *pDirectory);

// change directory
int32_t ZMemcardFCd(const char *pDirName);

// open a window
void ZMemcardWindowOpen(const char *pTitle, const char *pBody);

// update window text
void ZMemcardWindowUpdate(const char *pBody);

// close a window
void ZMemcardWindowClose(void);

// throw up a msgbox
void ZMemcardMsgBox(void);

#ifdef __cplusplus
};
#endif

#endif // _ZMemcard_h

