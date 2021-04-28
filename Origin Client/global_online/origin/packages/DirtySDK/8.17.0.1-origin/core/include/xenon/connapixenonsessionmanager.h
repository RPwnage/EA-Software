/*H*************************************************************************************/
/*!
    \File connapixenonsessionmanager.h

    \Description

    \Notes

    \Copyright
        (c) Electronic Arts. All Rights Reserved.

    \Version 1.0 06/04/2009 (jrainy) first version
*/
/*************************************************************************************H*/

#ifndef _connapixenonsessionmanager_h
#define _connapixenonsessionmanager_h

/*!
\Module Game
*/
//@{

/*** Include files *********************************************************************/

#include "dirtyaddr.h"

/*** Defines ***************************************************************************/


/*** Type Definitions ******************************************************************/

typedef struct ConnApiXenonSessionManagerRefT ConnApiXenonSessionManagerRefT;

/*** Function Prototypes ***************************************************************/

// allocate module state and prepare for use
ConnApiXenonSessionManagerRefT *ConnApiXenonSessionManagerCreate(DirtyAddrT *pHostAddr, DirtyAddrT *pLocalAddr);

// destroy the module and release its state
void ConnApiXenonSessionManagerDestroy(ConnApiXenonSessionManagerRefT *pRef);

// give time to module to do its thing (should be called periodically to allow module to perform work)
void ConnApiXenonSessionManagerUpdate(ConnApiXenonSessionManagerRefT *pRef);

// Join a remote player by specifying the session and type of slot to use
void ConnApiXenonSessionManagerConnect(ConnApiXenonSessionManagerRefT *pRef, const char **pSessID, uint32_t *uSlot, uint32_t uCount);
// Join a remote player by specifying the session and type of slot to use (use for rematch)
void ConnApiXenonSessionManagerConnect2(ConnApiXenonSessionManagerRefT *pRef, const char **pSessID, uint32_t *uSlot, uint32_t uCount);

// get result
int32_t ConnApiXenonSessionManagerComplete(ConnApiXenonSessionManagerRefT *pRef, int32_t *pAddr, const char* pMachineAddress);

// ConnApiXenonSessionManager control
int32_t ConnApiXenonSessionManagerControl(ConnApiXenonSessionManagerRefT *pRef, int32_t iControl, int32_t iValue, int32_t iValue2, const void *pValue);

// get module status based on selector
int32_t ConnApiXenonSessionManagerStatus(ConnApiXenonSessionManagerRefT *pRef, int32_t iSelect, void *pBuf, int32_t iBufSize);

// create the session (previously the 'sess' control selector)
int32_t ConnApiXenonSessionManagerCreateSess(ConnApiXenonSessionManagerRefT *pRef, uint32_t bRanked, uint32_t *uUserFlags, const char *pSession, DirtyAddrT *pLocalAddrs);

//@}

#endif // _connapixenonsessionmanager_h

