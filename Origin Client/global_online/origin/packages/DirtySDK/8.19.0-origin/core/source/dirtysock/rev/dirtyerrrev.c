/*H********************************************************************************/
/*!
    \File dirtyerrrev.c

    \Description
        Dirtysock debug error routines.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 11/27/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <revolution/os.h>
#include <revolution/so.h>

#include "dirtysock.h"
#include "dirtyerr.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

#if DIRTYSOCK_ERRORNAMES
static DirtyErrT _DirtyErr_List[] =
{
    DIRTYSOCK_ErrorName(SO_E2BIG),             // (-1)
    DIRTYSOCK_ErrorName(SO_EACCES),            // (-2)
    DIRTYSOCK_ErrorName(SO_EADDRINUSE),        // (-3)        // Address is already in use
    DIRTYSOCK_ErrorName(SO_EADDRNOTAVAIL),     // (-4)
    DIRTYSOCK_ErrorName(SO_EAFNOSUPPORT),      // (-5)        // Non-supported address family
    DIRTYSOCK_ErrorName(SO_EAGAIN),            // (-6)        // Posix.1
    DIRTYSOCK_ErrorName(SO_EALREADY),          // (-7)        // Already in progress
    DIRTYSOCK_ErrorName(SO_EBADF),             // (-8)        // Bad socket descriptor
    DIRTYSOCK_ErrorName(SO_EBADMSG),           // (-9)
    DIRTYSOCK_ErrorName(SO_EBUSY),             // (-10)
    DIRTYSOCK_ErrorName(SO_ECANCELED),         // (-11)
    DIRTYSOCK_ErrorName(SO_ECHILD),            // (-12)
    DIRTYSOCK_ErrorName(SO_ECONNABORTED),      // (-13)       // Connection aborted
    DIRTYSOCK_ErrorName(SO_ECONNREFUSED),      // (-14)       // Connection refused
    DIRTYSOCK_ErrorName(SO_ECONNRESET),        // (-15)       // Connection reset
    DIRTYSOCK_ErrorName(SO_EDEADLK),           // (-16)
    DIRTYSOCK_ErrorName(SO_EDESTADDRREQ),      // (-17)       // Not bound to a local address
    DIRTYSOCK_ErrorName(SO_EDOM),              // (-18)
    DIRTYSOCK_ErrorName(SO_EDQUOT),            // (-19)
    DIRTYSOCK_ErrorName(SO_EEXIST),            // (-20)
    DIRTYSOCK_ErrorName(SO_EFAULT),            // (-21)
    DIRTYSOCK_ErrorName(SO_EFBIG),             // (-22)
    DIRTYSOCK_ErrorName(SO_EHOSTUNREACH),      // (-23)
    DIRTYSOCK_ErrorName(SO_EIDRM),             // (-24)
    DIRTYSOCK_ErrorName(SO_EILSEQ),            // (-25)
    DIRTYSOCK_ErrorName(SO_EINPROGRESS),       // (-26)
    DIRTYSOCK_ErrorName(SO_EINTR),             // (-27)       // Canceled
    DIRTYSOCK_ErrorName(SO_EINVAL),            // (-28)       // Invalid operation
    DIRTYSOCK_ErrorName(SO_EIO),               // (-29)       // I/O error
    DIRTYSOCK_ErrorName(SO_EISCONN),           // (-30)       // Socket is already connected
    DIRTYSOCK_ErrorName(SO_EISDIR),            // (-31)
    DIRTYSOCK_ErrorName(SO_ELOOP),             // (-32)
    DIRTYSOCK_ErrorName(SO_EMFILE),            // (-33)       // No more socket descriptors
    DIRTYSOCK_ErrorName(SO_EMLINK),            // (-34)
    DIRTYSOCK_ErrorName(SO_EMSGSIZE),          // (-35)       // Too large to be sent
    DIRTYSOCK_ErrorName(SO_EMULTIHOP),         // (-36)
    DIRTYSOCK_ErrorName(SO_ENAMETOOLONG),      // (-37)
    DIRTYSOCK_ErrorName(SO_ENETDOWN),          // (-38)
    DIRTYSOCK_ErrorName(SO_ENETRESET),         // (-39)
    DIRTYSOCK_ErrorName(SO_ENETUNREACH),       // (-40)       // Unreachable
    DIRTYSOCK_ErrorName(SO_ENFILE),            // (-41)
    DIRTYSOCK_ErrorName(SO_ENOBUFS),           // (-42)       // Insufficient resources
    DIRTYSOCK_ErrorName(SO_ENODATA),           // (-43)
    DIRTYSOCK_ErrorName(SO_ENODEV),            // (-44)
    DIRTYSOCK_ErrorName(SO_ENOENT),            // (-45)
    DIRTYSOCK_ErrorName(SO_ENOEXEC),           // (-46)
    DIRTYSOCK_ErrorName(SO_ENOLCK),            // (-47)
    DIRTYSOCK_ErrorName(SO_ENOLINK),           // (-48)
    DIRTYSOCK_ErrorName(SO_ENOMEM),            // (-49)       // Insufficient memory
    DIRTYSOCK_ErrorName(SO_ENOMSG),            // (-50)
    DIRTYSOCK_ErrorName(SO_ENOPROTOOPT),       // (-51)       // Non-supported option
    DIRTYSOCK_ErrorName(SO_ENOSPC),            // (-52)
    DIRTYSOCK_ErrorName(SO_ENOSR),             // (-53)
    DIRTYSOCK_ErrorName(SO_ENOSTR),            // (-54)
    DIRTYSOCK_ErrorName(SO_ENOSYS),            // (-55)
    DIRTYSOCK_ErrorName(SO_ENOTCONN),          // (-56)       // Not connected
    DIRTYSOCK_ErrorName(SO_ENOTDIR),           // (-57)
    DIRTYSOCK_ErrorName(SO_ENOTEMPTY),         // (-58)
    DIRTYSOCK_ErrorName(SO_ENOTSOCK),          // (-59)       // Not a socket
    DIRTYSOCK_ErrorName(SO_ENOTSUP),           // (-60)
    DIRTYSOCK_ErrorName(SO_ENOTTY),            // (-61)
    DIRTYSOCK_ErrorName(SO_ENXIO),             // (-62)
    DIRTYSOCK_ErrorName(SO_EOPNOTSUPP),        // (-63)       // Non-supported operation
    DIRTYSOCK_ErrorName(SO_EOVERFLOW),         // (-64)
    DIRTYSOCK_ErrorName(SO_EPERM),             // (-65)
    DIRTYSOCK_ErrorName(SO_EPIPE),             // (-66)
    DIRTYSOCK_ErrorName(SO_EPROTO),            // (-67)
    DIRTYSOCK_ErrorName(SO_EPROTONOSUPPORT),   // (-68)       // Non-supported protocol
    DIRTYSOCK_ErrorName(SO_EPROTOTYPE),        // (-69)       // Non-supported socket type
    DIRTYSOCK_ErrorName(SO_ERANGE),            // (-70)
    DIRTYSOCK_ErrorName(SO_EROFS),             // (-71)
    DIRTYSOCK_ErrorName(SO_ESPIPE),            // (-72)
    DIRTYSOCK_ErrorName(SO_ESRCH),             // (-73)
    DIRTYSOCK_ErrorName(SO_ESTALE),            // (-74)
    DIRTYSOCK_ErrorName(SO_ETIME),             // (-75)
    DIRTYSOCK_ErrorName(SO_ETIMEDOUT),         // (-76)       // Timed out
    DIRTYSOCK_ErrorName(SO_ETXTBSY),           // (-77)
    DIRTYSOCK_ErrorName(SO_EWOULDBLOCK),       // SO_EAGAIN  // Posix.1g
    DIRTYSOCK_ErrorName(SO_EXDEV),             // (-78)

    // NULL terminate
    DIRTYSOCK_ErrorName(0)
};
#endif

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function DirtyErrName

    \Description
        This function takes as input a system-specific error code, and either
        resolves it to its define name if it is recognized or formats it as a hex
        number if not.

    \Input *pBuffer - [out] pointer to output buffer to store result
    \Input iBufSize - size of output buffer
    \Input uError   - error code to format
    
    \Output
        None.

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
void DirtyErrName(char *pBuffer, int32_t iBufSize, uint32_t uError)
{
    #if DIRTYSOCK_ERRORNAMES
    int32_t iErr;
    
    for (iErr = 0; _DirtyErr_List[iErr].uError != 0; iErr++)
    {
        if (_DirtyErr_List[iErr].uError == uError)
        {
            snzprintf(pBuffer, iBufSize, "%s/0x%08x", _DirtyErr_List[iErr].pErrorName, uError);
            return;
        }
    }
    #endif

    snzprintf(pBuffer, iBufSize, "0x%08x", uError);
}

/*F********************************************************************************/
/*!
    \Function DirtyErrGetName

    \Description
        This function takes as input a system-specific error code, and either
        resolves it to its define name if it is recognized or formats it as a hex
        number if not.

    \Input uError   - error code to format
    
    \Output
        const char *- pointer to error name or error formatted in hex

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
const char *DirtyErrGetName(uint32_t uError)
{
    static char strName[64];
    DirtyErrName(strName, sizeof(strName), uError);
    return(strName);
}
