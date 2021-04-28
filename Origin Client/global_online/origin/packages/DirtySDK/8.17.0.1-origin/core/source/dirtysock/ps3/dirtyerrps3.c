/*H********************************************************************************/
/*!
    \File dirtyerrps3.c

    \Description
        Dirtysock debug error routines.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 10/04/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

// network includes
#include <sdk_version.h> // included so we know where to find network headers
#include <stddef.h>

#include <np/error.h>

#include <netex/libnetctl.h>
#include <netex/errno.h>

#include <cell/mic.h>
#include <cell/codec.h>
#include <cell/audio.h>

#include <sysutil/sysutil_webbrowser.h>
#if CELL_SDK_VERSION < 0x280001
#include <sysutil/sysutil_gamedata.h>
#endif

#include <sys/return_code.h>

#include "dirtysock.h"
#include "dirtyerr.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

#if DIRTYSOCK_ERRORNAMES
static DirtyErrT _DirtyErr_List[] =
{
    /*
        errors from netex\errno.h
        sys_net_errno
    */
    DIRTYSOCK_ErrorName(SYS_NET_EPERM),             // 1
    DIRTYSOCK_ErrorName(SYS_NET_ENOENT),            // 2
    DIRTYSOCK_ErrorName(SYS_NET_ESRCH),             // 3
    DIRTYSOCK_ErrorName(SYS_NET_EINTR),             // 4
    DIRTYSOCK_ErrorName(SYS_NET_EIO),               // 5
    DIRTYSOCK_ErrorName(SYS_NET_ENXIO),             // 6
    DIRTYSOCK_ErrorName(SYS_NET_E2BIG),             // 7
    DIRTYSOCK_ErrorName(SYS_NET_ENOEXEC),           // 8
    DIRTYSOCK_ErrorName(SYS_NET_EBADF),             // 9
    DIRTYSOCK_ErrorName(SYS_NET_ECHILD),            // 10
    DIRTYSOCK_ErrorName(SYS_NET_EDEADLK),           // 11
    DIRTYSOCK_ErrorName(SYS_NET_ENOMEM),            // 12
    DIRTYSOCK_ErrorName(SYS_NET_EACCES),            // 13
    DIRTYSOCK_ErrorName(SYS_NET_EFAULT),            // 14
    DIRTYSOCK_ErrorName(SYS_NET_ENOTBLK),           // 15
    DIRTYSOCK_ErrorName(SYS_NET_EBUSY),             // 16
    DIRTYSOCK_ErrorName(SYS_NET_EEXIST),            // 17
    DIRTYSOCK_ErrorName(SYS_NET_EXDEV),             // 18
    DIRTYSOCK_ErrorName(SYS_NET_ENODEV),            // 19
    DIRTYSOCK_ErrorName(SYS_NET_ENOTDIR),           // 20
    DIRTYSOCK_ErrorName(SYS_NET_EISDIR),            // 21
    DIRTYSOCK_ErrorName(SYS_NET_EINVAL),            // 22
    DIRTYSOCK_ErrorName(SYS_NET_ENFILE),            // 23
    DIRTYSOCK_ErrorName(SYS_NET_EMFILE),            // 24
    DIRTYSOCK_ErrorName(SYS_NET_ENOTTY),            // 25
    DIRTYSOCK_ErrorName(SYS_NET_ETXTBSY),           // 26
    DIRTYSOCK_ErrorName(SYS_NET_EFBIG),             // 27
    DIRTYSOCK_ErrorName(SYS_NET_ENOSPC),            // 28
    DIRTYSOCK_ErrorName(SYS_NET_ESPIPE),            // 29
    DIRTYSOCK_ErrorName(SYS_NET_EROFS),             // 30
    DIRTYSOCK_ErrorName(SYS_NET_EMLINK),            // 31
    DIRTYSOCK_ErrorName(SYS_NET_EPIPE),             // 32
    DIRTYSOCK_ErrorName(SYS_NET_EDOM),              // 33
    DIRTYSOCK_ErrorName(SYS_NET_ERANGE),            // 34
    DIRTYSOCK_ErrorName(SYS_NET_EWOULDBLOCK),       // 35
    DIRTYSOCK_ErrorName(SYS_NET_EINPROGRESS),       // 36
    DIRTYSOCK_ErrorName(SYS_NET_EALREADY),          // 37
    DIRTYSOCK_ErrorName(SYS_NET_ENOTSOCK),          // 38
    DIRTYSOCK_ErrorName(SYS_NET_EDESTADDRREQ),      // 39
    DIRTYSOCK_ErrorName(SYS_NET_EMSGSIZE),          // 40
    DIRTYSOCK_ErrorName(SYS_NET_EPROTOTYPE),        // 41
    DIRTYSOCK_ErrorName(SYS_NET_ENOPROTOOPT),       // 42
    DIRTYSOCK_ErrorName(SYS_NET_EPROTONOSUPPORT),   // 43
    DIRTYSOCK_ErrorName(SYS_NET_ESOCKTNOSUPPORT),   // 44
    DIRTYSOCK_ErrorName(SYS_NET_EOPNOTSUPP),        // 45
    DIRTYSOCK_ErrorName(SYS_NET_EPFNOSUPPORT),      // 46
    DIRTYSOCK_ErrorName(SYS_NET_EAFNOSUPPORT),      // 47
    DIRTYSOCK_ErrorName(SYS_NET_EADDRINUSE),        // 48
    DIRTYSOCK_ErrorName(SYS_NET_EADDRNOTAVAIL),     // 49
    DIRTYSOCK_ErrorName(SYS_NET_ENETDOWN),          // 50
    DIRTYSOCK_ErrorName(SYS_NET_ENETUNREACH),       // 51
    DIRTYSOCK_ErrorName(SYS_NET_ENETRESET),         // 52
    DIRTYSOCK_ErrorName(SYS_NET_ECONNABORTED),      // 53
    DIRTYSOCK_ErrorName(SYS_NET_ECONNRESET),        // 54
    DIRTYSOCK_ErrorName(SYS_NET_ENOBUFS),           // 55
    DIRTYSOCK_ErrorName(SYS_NET_EISCONN),           // 56
    DIRTYSOCK_ErrorName(SYS_NET_ENOTCONN),          // 57
    DIRTYSOCK_ErrorName(SYS_NET_ESHUTDOWN),         // 58
    DIRTYSOCK_ErrorName(SYS_NET_ETOOMANYREFS),      // 59
    DIRTYSOCK_ErrorName(SYS_NET_ETIMEDOUT),         // 60
    DIRTYSOCK_ErrorName(SYS_NET_ECONNREFUSED),      // 61
    DIRTYSOCK_ErrorName(SYS_NET_ELOOP),             // 62
    DIRTYSOCK_ErrorName(SYS_NET_ENAMETOOLONG),      // 63
    DIRTYSOCK_ErrorName(SYS_NET_EHOSTDOWN),         // 64
    DIRTYSOCK_ErrorName(SYS_NET_EHOSTUNREACH),      // 65
    DIRTYSOCK_ErrorName(SYS_NET_ENOTEMPTY),         // 66
    DIRTYSOCK_ErrorName(SYS_NET_EPROCLIM),          // 67
    DIRTYSOCK_ErrorName(SYS_NET_EUSERS),            // 68
    DIRTYSOCK_ErrorName(SYS_NET_EDQUOT),            // 69
    DIRTYSOCK_ErrorName(SYS_NET_ESTALE),            // 70
    DIRTYSOCK_ErrorName(SYS_NET_EREMOTE),           // 71
    DIRTYSOCK_ErrorName(SYS_NET_EBADRPC),           // 72
    DIRTYSOCK_ErrorName(SYS_NET_ERPCMISMATCH),      // 73
    DIRTYSOCK_ErrorName(SYS_NET_EPROGUNAVAIL),      // 74
    DIRTYSOCK_ErrorName(SYS_NET_EPROGMISMATCH),     // 75
    DIRTYSOCK_ErrorName(SYS_NET_EPROCUNAVAIL),      // 76
    DIRTYSOCK_ErrorName(SYS_NET_ENOLCK),            // 77
    DIRTYSOCK_ErrorName(SYS_NET_ENOSYS),            // 78
    DIRTYSOCK_ErrorName(SYS_NET_EFTYPE),            // 79
    DIRTYSOCK_ErrorName(SYS_NET_EAUTH),             // 80
    DIRTYSOCK_ErrorName(SYS_NET_ENEEDAUTH),         // 81
    DIRTYSOCK_ErrorName(SYS_NET_EIDRM),             // 82
    DIRTYSOCK_ErrorName(SYS_NET_ENOMSG),            // 83
    DIRTYSOCK_ErrorName(SYS_NET_EOVERFLOW),         // 84
    DIRTYSOCK_ErrorName(SYS_NET_EILSEQ),            // 85
    DIRTYSOCK_ErrorName(SYS_NET_ENOTSUP),           // 86
    DIRTYSOCK_ErrorName(SYS_NET_ECANCELED),         // 87
    DIRTYSOCK_ErrorName(SYS_NET_EBADMSG),           // 88
    DIRTYSOCK_ErrorName(SYS_NET_ENODATA),           // 89
    DIRTYSOCK_ErrorName(SYS_NET_ENOSR),             // 90
    DIRTYSOCK_ErrorName(SYS_NET_ENOSTR),            // 91
    DIRTYSOCK_ErrorName(SYS_NET_ETIME),             // 92
    DIRTYSOCK_ErrorName(SYS_NET_ELAST),             // 92

    // cell errors
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EPERM),           // 0x80010201
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENOENT),          // 0x80010202
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ESRCH),           // 0x80010203
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EINTR),           // 0x80010204
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EIO),             // 0x80010205
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENXIO),           // 0x80010206
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_E2BIG),           // 0x80010207
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENOEXEC),         // 0x80010208
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EBADF),           // 0x80010209
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ECHILD),          // 0x8001020a
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EDEADLK),         // 0x8001020b
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENOMEM),          // 0x8001020c
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EACCES),          // 0x8001020d
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EFAULT),          // 0x8001020e
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENOTBLK),         // 0x8001020f
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EBUSY),           // 0x80010210
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EEXIST),          // 0x80010211
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EXDEV),           // 0x80010212
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENODEV),          // 0x80010213
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENOTDIR),         // 0x80010214
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EISDIR),          // 0x80010215
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EINVAL),          // 0x80010216
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENFILE),          // 0x80010217
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EMFILE),          // 0x80010218
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENOTTY),          // 0x80010219
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ETXTBSY),         // 0x8001021a
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EFBIG),           // 0x8001021b
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENOSPC),          // 0x8001021c
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ESPIPE),          // 0x8001021d
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EROFS),           // 0x8001021e
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EMLINK),          // 0x8001021f
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EPIPE),           // 0x80010220
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EDOM),            // 0x80010221
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ERANGE),          // 0x80010222
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EAGAIN),          // 0x80010223
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EWOULDBLOCK),     // SYS_NET_ERROR_EAGAIN
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EINPROGRESS),     // 0x80010224
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EALREADY),        // 0x80010225
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENOTSOCK),        // 0x80010226
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EDESTADDRREQ),    // 0x80010227
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EMSGSIZE),        // 0x80010228
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EPROTOTYPE),      // 0x80010229
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENOPROTOOPT),     // 0x8001022a
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EPROTONOSUPPORT), // 0x8001022b
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ESOCKTNOSUPPORT), // 0x8001022c
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EOPNOTSUPP),      // 0x8001022d
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EPFNOSUPPORT),    // 0x8001022e
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EAFNOSUPPORT),    // 0x8001022f
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EADDRINUSE),      // 0x80010230
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EADDRNOTAVAIL),   // 0x80010231
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENETDOWN),        // 0x80010232
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENETUNREACH),     // 0x80010233
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENETRESET),       // 0x80010234
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ECONNABORTED),    // 0x80010235
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ECONNRESET),      // 0x80010236
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENOBUFS),         // 0x80010237
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EISCONN),         // 0x80010238
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENOTCONN),        // 0x80010239
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ESHUTDOWN),       // 0x8001023a
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ETOOMANYREFS),    // 0x8001023b
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ETIMEDOUT),       // 0x8001023c
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ECONNREFUSED),    // 0x8001023d
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ELOOP),           // 0x8001023e
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENAMETOOLONG),    // 0x8001023f
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EHOSTDOWN),       // 0x80010240
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EHOSTUNREACH),    // 0x80010241
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENOTEMPTY),       // 0x80010242
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EPROCLIM),        // 0x80010243
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EUSERS),          // 0x80010244
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EDQUOT),          // 0x80010245
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ESTALE),          // 0x80010246
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EREMOTE),         // 0x80010247
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EBADRPC),         // 0x80010248
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ERPCMISMATCH),    // 0x80010249
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EPROGUNAVAIL),    // 0x8001024a
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EPROGMISMATCH),   // 0x8001024b
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EPROCUNAVAIL),    // 0x8001024c
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENOLCK),          // 0x8001024d
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENOSYS),          // 0x8001024e
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EFTYPE),          // 0x8001024f
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EAUTH),           // 0x80010250
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENEEDAUTH),       // 0x80010251
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EIDRM),           // 0x80010252
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENOMSG),          // 0x80010253
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EOVERFLOW),       // 0x80010254
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EILSEQ),          // 0x80010255
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENOTSUP),         // 0x80010256
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ECANCELED),       // 0x80010257
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_EBADMSG),         // 0x80010258
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENODATA),         // 0x80010259
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENOSR),           // 0x8001025a
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ENOSTR),          // 0x8001025b
    DIRTYSOCK_ErrorName(SYS_NET_ERROR_ETIME),           // 0x8001025c

    /*
        errors from errno.h
        LV2 kernel error codes
    */
    DIRTYSOCK_ErrorName(EAGAIN),                    // 0x80010001 -- The resource is temporarily unavailable; e.g. The number of threads in the system is exceeding the limit.
    DIRTYSOCK_ErrorName(EINVAL),                    // 0x80010002 -- An invalid argument value is specified; e.g. An out-of-range argument or an invalid flag.
    DIRTYSOCK_ErrorName(ENOSYS),                    // 0x80010003 -- The feature is not yet implemented.
    DIRTYSOCK_ErrorName(ENOMEM),                    // 0x80010004 -- Memory allocation failure
    DIRTYSOCK_ErrorName(ESRCH),                     // 0x80010005 -- The resource (process, thread, synchronous object, etc) with the specified identifier does not exist.
    DIRTYSOCK_ErrorName(ENOENT),                    // 0x80010006 -- The file does not exist.
    DIRTYSOCK_ErrorName(ENOEXEC),                   // 0x80010007 -- The file is not a valid ELF file. (The file is in unrecognized format).
    DIRTYSOCK_ErrorName(EDEADLK),                   // 0x80010008 -- Resource deadlock is avoided.
    DIRTYSOCK_ErrorName(EPERM),                     // 0x80010009 -- The operation is not permitted.
    DIRTYSOCK_ErrorName(EBUSY),                     // 0x8001000A -- The device or resource is busy.
    DIRTYSOCK_ErrorName(ETIMEDOUT),                 // 0x8001000B -- The operation is timed out
    DIRTYSOCK_ErrorName(EABORT),                    // 0x8001000C -- The operation is aborted
    DIRTYSOCK_ErrorName(EFAULT),                    // 0x8001000D -- Invalid memory access
    DIRTYSOCK_ErrorName(ESTAT),                     // 0x8001000F -- State of the target thread is invalid.
    DIRTYSOCK_ErrorName(EALIGN),                    // 0x80010010 -- Alignment is invalid.
    DIRTYSOCK_ErrorName(EKRESOURCE),                // 0x80010011 -- Shortage of the kernel resources
    DIRTYSOCK_ErrorName(EISDIR),                    // 0x80010012 -- The file is a directory
    DIRTYSOCK_ErrorName(ECANCELED),                 // 0x80010013 -- Operation canceled
    DIRTYSOCK_ErrorName(EEXIST),                    // 0x80010014 -- Entry already exists
    DIRTYSOCK_ErrorName(EISCONN),                   // 0x80010015 -- Port is already connected
    DIRTYSOCK_ErrorName(ENOTCONN),                  // 0x80010016 -- Port is not connected
    DIRTYSOCK_ErrorName(EAUTHFAIL),                 // 0x80010017 -- Program authentication fail
    DIRTYSOCK_ErrorName(EDOM),                      // 0x8001001B
    DIRTYSOCK_ErrorName(ERANGE),                    // 0x8001001C
    DIRTYSOCK_ErrorName(EILSEQ),                    // 0x8001001D
    DIRTYSOCK_ErrorName(EFPOS),                     // 0x8001001E
    DIRTYSOCK_ErrorName(EINTR),                     // 0x8001001F
    DIRTYSOCK_ErrorName(EFBIG),                     // 0x80010020
    DIRTYSOCK_ErrorName(EMLINK),                    // 0x80010021
    DIRTYSOCK_ErrorName(ENFILE),                    // 0x80010022
    DIRTYSOCK_ErrorName(ENOSPC),                    // 0x80010023
    DIRTYSOCK_ErrorName(ENOTTY),                    // 0x80010024
    DIRTYSOCK_ErrorName(EPIPE),                     // 0x80010025
    DIRTYSOCK_ErrorName(EROFS),                     // 0x80010026
    DIRTYSOCK_ErrorName(ESPIPE),                    // 0x80010027
    DIRTYSOCK_ErrorName(E2BIG),                     // 0x80010028
    DIRTYSOCK_ErrorName(EACCES),                    // 0x80010029
    DIRTYSOCK_ErrorName(EBADF),                     // 0x8001002A
    DIRTYSOCK_ErrorName(EIO),                       // 0x8001002B
    DIRTYSOCK_ErrorName(EMFILE),                    // 0x8001002C
    DIRTYSOCK_ErrorName(ENODEV),                    // 0x8001002D
    DIRTYSOCK_ErrorName(ENOTDIR),                   // 0x8001002E
    DIRTYSOCK_ErrorName(ENXIO),                     // 0x8001002F
    DIRTYSOCK_ErrorName(EXDEV),                     // 0x80010030
    DIRTYSOCK_ErrorName(EBADMSG),                   // 0x80010031
    DIRTYSOCK_ErrorName(EINPROGRESS),               // 0x80010032
    DIRTYSOCK_ErrorName(EMSGSIZE),                  // 0x80010033
    DIRTYSOCK_ErrorName(ENAMETOOLONG),              // 0x80010034
    DIRTYSOCK_ErrorName(ENOLCK),                    // 0x80010035
    DIRTYSOCK_ErrorName(ENOTEMPTY),                 // 0x80010036
    DIRTYSOCK_ErrorName(ENOTSUP),                   // 0x80010037
    DIRTYSOCK_ErrorName(EFSSPECIFIC),               // 0x80010038
    DIRTYSOCK_ErrorName(EOVERFLOW),                 // 0x80010039
    DIRTYSOCK_ErrorName(ENOTMOUNTED),               // 0x8001003A

    /*
        errors from np\error.h
    */

    // NP Authentication Error (client runtime errors) */
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_EINVAL),        // 0x8002a002
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ENOMEM),        // 0x8002a004
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ESRCH),         // 0x8002a005
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_EBUSY),         // 0x8002a00a
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_EABORT),        // 0x8002a00c
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_EEXIST),        // 0x8002a014

    /* community client library error (0x8002a100 - 0x8002a1ff)*/
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_ALREADY_INITIALIZED),            // 0x8002a101 - library has already been initialized
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED),                // 0x8002a102 - the library has not yet been initialized
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_OUT_OF_MEMORY),                  // 0x8002a103 - tried to malloc, but got NULL (no memory)
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT),               // 0x8002a104
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_NO_TITLE_SET),                   // 0x8002a105
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_NO_LOGIN),                       // 0x8002a106
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS),               // 0x8002a107
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_TRANSACTION_STILL_REFERENCED),   // 0x8002a108
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_ABORTED),                        // 0x8002a109
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_NO_RESOURCE),                    // 0x8002a10a
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_BAD_RESPONSE),                   // 0x8002a10b
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_BODY_TOO_LARGE),                 // 0x8002a10c
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_HTTP_SERVER),                    // 0x8002a10d
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_INVALID_SIGNATURE),              // 0x8002a10e
    #if CELL_SDK_VERSION >= 0x180002
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_TIMEOUT),                        // 0x8002a10f
    #endif
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT),          // 0x8002a1a1
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_UNKNOWN_TYPE),                   // 0x8002a1a2
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_INVALID_ID),                     // 0x8002a1a3
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID),              // 0x8002a1a4
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_INVALID_PSHANDLE),               // 0x8002a1a4
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_INVALID_TICKET),                 // 0x8002a1a5
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_CLIENT_ALREADY_EXISTS),          // 0x8002a1a6
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_CLIENT_HANDLE_ALREADY_EXISTS),   // 0x8002a1a6
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_BUFFER),            // 0x8002a1a7
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_INVALID_TYPE),                   // 0x8002a1a8
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_TRANSACTION_ALREADY_END),        // 0x8002a1a9
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_TRANSACTION_BEFORE_END),         // 0x8002a1aa
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_BUSY_BY_ANOTEHR_TRANSACTION),    // 0x8002a1ab
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT),              // 0x8002a1ac
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID),                  // 0x8002a1ad
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_TOO_LARGE_RANGE),                // 0x8002a1ae
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_INVALID_PARTITION),              // 0x8002a1af

    /* NP Authentication Error (server returned errors) */
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_SERVICE_END),                         // 0x8002a200
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_SERVICE_DOWN),                        // 0x8002a201
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_SERVICE_BUSY),                        // 0x8002a202
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_INVALID_DATA_LENGTH),                 // 0x8002a210
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_INVALID_USER_AGENT),                  // 0x8002a211
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_INVALID_VERSION),                     // 0x8002a212
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_INVALID_SERVICE_ID),                  // 0x8002a220
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_INVALID_CREDENTIAL),                  // 0x8002a221
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_INVALID_ENTITLEMENT_ID),              // 0x8002a222
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_INVALID_CONSUMED_COUNT),              // 0x8002a223
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_CLOSED),                      // 0x8002a230
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_SUSPENDED),                   // 0x8002a231
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_EULA),                  // 0x8002a232
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT1),              // 0x8002a240
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT2),              // 0x8002a241
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT3),              // 0x8002a242
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT4),              // 0x8002a243
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT5),              // 0x8002a244
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT6),              // 0x8002a245
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT7),              // 0x8002a246
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT8),              // 0x8002a247
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT9),              // 0x8002a248
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT10),             // 0x8002a249
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT11),             // 0x8002a24a
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT12),             // 0x8002a24b
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT13),             // 0x8002a24c
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT14),             // 0x8002a24d
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT15),             // 0x8002a24e
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT16),             // 0x8002a24f
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_UNKNOWN),                             // 0x8002a280

    /* NP Core Server Error */
    DIRTYSOCK_ErrorName(SCE_NP_CORE_SERVER_ERROR_CONFLICT),                     // 0x8002a303
    DIRTYSOCK_ErrorName(SCE_NP_CORE_SERVER_ERROR_NOT_AUTHORIZED),               // 0x8002a30d
    DIRTYSOCK_ErrorName(SCE_NP_CORE_SERVER_ERROR_REMOTE_CONNECTION_FAILED),     // 0x8002a30f
    DIRTYSOCK_ErrorName(SCE_NP_CORE_SERVER_ERROR_RESOURCE_CONSTRAINT),          // 0x8002a310
    DIRTYSOCK_ErrorName(SCE_NP_CORE_SERVER_ERROR_SYSTEM_SHUTDOWN),              // 0x8002a313
    DIRTYSOCK_ErrorName(SCE_NP_CORE_SERVER_ERROR_UNSUPPORTED_CLIENT_VERSION),   // 0x8002a319

    /* community server error (0x8002a400 - 0x8002a4ff) */
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_BAD_REQUEST),                         // 0x8002a401
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_TICKET),                      // 0x8002a402
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_SIGNATURE),                   // 0x8002a403
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_EXPIRED_TICKET),                      // 0x8002a404
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_NPID),                        // 0x8002a405
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_FORBIDDEN),                           // 0x8002a406
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INTERNAL_SERVER_ERROR),               // 0x8002a407
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_VERSION_NOT_SUPPORTED),               // 0x8002a408
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_SERVICE_UNAVAILABLE),                 // 0x8002a409
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_PLAYER_BANNED),                       // 0x8002a40a
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_CENSORED),                            // 0x8002a40b
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_RECORD_FORBIDDEN),            // 0x8002a40c
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_USER_PROFILE_NOT_FOUND),              // 0x8002a40d
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_UPLOADER_DATA_NOT_FOUND),             // 0x8002a40e
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_QUOTA_MASTER_NOT_FOUND),              // 0x8002a40f
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_TITLE_NOT_FOUND),             // 0x8002a410
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_BLACKLISTED_USER_ID),                 // 0x8002a411
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_GAME_RANKING_NOT_FOUND),              // 0x8002a412
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_STORE_NOT_FOUND),             // 0x8002a414
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_NOT_BEST_SCORE),                      // 0x8002a415
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_LATEST_UPDATE_NOT_FOUND),             // 0x8002a416
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_BOARD_MASTER_NOT_FOUND),      // 0x8002a417
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_GAME_DATA_MASTER_NOT_FOUND),  // 0x8002a418
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_ANTICHEAT_DATA),              // 0x8002a419
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_TOO_LARGE_DATA),                      // 0x8002a41a
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_NO_SUCH_USER_NPID),                   // 0x8002a41b
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_ENVIRONMENT),                 // 0x8002a41d
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_ONLINE_NAME_CHARACTER),       // 0x8002a41f
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_ONLINE_NAME_LENGTH),          // 0x8002a420
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_ABOUT_ME_CHARACTER),          // 0x8002a421
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_ABOUT_ME_LENGTH),             // 0x8002a422
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_SCORE),                       // 0x8002a423
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_OVER_THE_RANKING_LIMIT),              // 0x8002a424
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_FAIL_TO_CREATE_SIGNATURE),            // 0x8002a426
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_MASTER_INFO_NOT_FOUND),       // 0x8002a427
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_OVER_THE_GAME_DATA_LIMIT),            // 0x8002a428
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_SELF_DATA_NOT_FOUND),                 // 0x8002a42a
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_USER_NOT_ASSIGNED),                   // 0x8002a42b
    #if CELL_SDK_VERSION >= 0x180002
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_GAME_DATA_ALREADY_EXISTS),            // 0x8002a42c
    #endif
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_MATCHING_BEFORE_SERVICE),             // 0x8002a4a0
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_MATCHING_END_OF_SERVICE),             // 0x8002a4a1
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_MATCHING_MAINTENANCE),                // 0x8002a4a2
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_BEFORE_SERVICE),              // 0x8002a4a3
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_END_OF_SERVICE),              // 0x8002a4a4
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_MAINTENANCE),                 // 0x8002a4a5
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_NO_SUCH_TITLE),                       // 0x8002a4a6
    /* Unspecified Error */
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_UNSPECIFIED),                         // 0x8002a4ff

    /* NP Core Error */
    DIRTYSOCK_ErrorName(SCE_NP_CORE_UTIL_ERROR_INVALID_ARGUMENT),               // 0x8002a501
    DIRTYSOCK_ErrorName(SCE_NP_CORE_UTIL_ERROR_OUT_OF_MEMORY),                  // 0x8002a502
    DIRTYSOCK_ErrorName(SCE_NP_CORE_UTIL_ERROR_INSUFFICIENT),                   // 0x8002a503
    DIRTYSOCK_ErrorName(SCE_NP_CORE_UTIL_ERROR_PARSER_FAILED),                  // 0x8002a504
    DIRTYSOCK_ErrorName(SCE_NP_CORE_UTIL_ERROR_INVALID_PROTOCOL_ID),            // 0x8002a505
    DIRTYSOCK_ErrorName(SCE_NP_CORE_UTIL_ERROR_INVALID_EXTENSION),              // 0x8002a506
    DIRTYSOCK_ErrorName(SCE_NP_CORE_UTIL_ERROR_INVALID_TEXT),                   // 0x8002a507
    DIRTYSOCK_ErrorName(SCE_NP_CORE_UTIL_ERROR_UNKNOWN_TYPE),                   // 0x8002a508
    DIRTYSOCK_ErrorName(SCE_NP_CORE_UTIL_ERROR_UNKNOWN),                        // 0x8002a509

    DIRTYSOCK_ErrorName(SCE_NP_CORE_PARSER_ERROR_NOT_INITIALIZED),              // 0x8002a511
    DIRTYSOCK_ErrorName(SCE_NP_CORE_PARSER_ERROR_ALREADY_INITIALIZED),          // 0x8002a512
    DIRTYSOCK_ErrorName(SCE_NP_CORE_PARSER_ERROR_OUT_OF_MEMORY),                // 0x8002a513
    DIRTYSOCK_ErrorName(SCE_NP_CORE_PARSER_ERROR_INSUFFICIENT),                 // 0x8002a514
    DIRTYSOCK_ErrorName(SCE_NP_CORE_PARSER_ERROR_INVALID_FORMAT),               // 0x8002a515
    DIRTYSOCK_ErrorName(SCE_NP_CORE_PARSER_ERROR_INVALID_ARGUMENT),             // 0x8002a516
    DIRTYSOCK_ErrorName(SCE_NP_CORE_PARSER_ERROR_INVALID_HANDLE),               // 0x8002a517
    DIRTYSOCK_ErrorName(SCE_NP_CORE_PARSER_ERROR_INVALID_ICON),                 // 0x8002a518
    DIRTYSOCK_ErrorName(SCE_NP_CORE_PARSER_ERROR_UNKNOWN),                      // 0x8002a519

    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_ALREADY_INITIALIZED),                 // 0x8002a521
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_NOT_INITIALIZED),                     // 0x8002a522
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_INVALID_ARGUMENT),                    // 0x8002a523
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_OUT_OF_MEMORY),                       // 0x8002a524
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_ID_NOT_AVAILABLE),                    // 0x8002a525
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_SESSION_RUNNING),                     // 0x8002a527
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_SESSION_NOT_ESTABLISHED),             // 0x8002a528
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_SESSION_INVALID_STATE),               // 0x8002a529
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_SESSION_ID_TOO_LONG),                 // 0x8002a52a
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_SESSION_INVALID_NAMESPACE),           // 0x8002a52b
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_CONNECTION_TIMEOUT),                  // 0x8002a52c
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_GETSOCKOPT),                          // 0x8002a52d
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_SSL_NOT_INITIALIZED),                 // 0x8002a52e
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_SSL_ALREADY_INITIALIZED),             // 0x8002a52f
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_SSL_NO_CERT),                         // 0x8002a530
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_SSL_NO_TRUSTWORTHY_CA),               // 0x8002a531
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_SSL_INVALID_CERT),                    // 0x8002a532
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_SSL_CERT_VERIFY),                     // 0x8002a533
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_SSL_CN_CHECK),                        // 0x8002a534
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_SSL_HANDSHAKE_FAILED),                // 0x8002a535
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_SSL_SEND),                            // 0x8002a536
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_SSL_RECV),                            // 0x8002a537
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_SSL_CREATE_CTX),                      // 0x8002a538
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_PARSE_PEM),                           // 0x8002a539
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_INVALID_INITIATE_STREAM),             // 0x8002a53a
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_SASL_NOT_SUPPORTED),                  // 0x8002a53b
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_NAMESPACE_ALREADY_EXISTS),            // 0x8002a53c
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_FROM_ALREADY_EXISTS),                 // 0x8002a53d
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_MODULE_NOT_REGISTERED),               // 0x8002a53e
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_MODULE_FROM_NOT_FOUND),               // 0x8002a53f
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_UNKNOWN_NAMESPACE),                   // 0x8002a540
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_INVALID_VERSION),                     // 0x8002a541
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_LOGIN_TIMEOUT),                       // 0x8002a542
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_TOO_MANY_SESSIONS),                   // 0x8002a543
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_SENDLIST_NOT_FOUND),                  // 0x8002a544
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_NO_ID),                               // 0x8002a545
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_LOAD_CERTS),                          // 0x8002a546
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_NET_SELECT),                          // 0x8002a547
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_DISCONNECTED),                        // 0x8002a548
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_TICKET_TOO_SMALL),                    // 0x8002a549
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_INVALID_TICKET),                      // 0x8002a54a
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_INVALID_PSHANDLE),                    // 0x8002a54b
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_GETHOSTBYNAME),                       // 0x8002a54c
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_UNDEFINED_STREAM_ERROR),              // 0x8002a54d
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_INTERNAL),                            // 0x8002a5ff

    /* NP Core DNS Error */
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_DNS_HOST_NOT_FOUND),                  // 0x8002af01
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_DNS_TRY_AGAIN),                       // 0x8002af02
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_DNS_NO_RECOVERY),                     // 0x8002af03
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_DNS_NO_DATA),                         // 0x8002af04
    DIRTYSOCK_ErrorName(SCE_NP_CORE_ERROR_DNS_NO_ADDRESS),                      // 0x8002afff

    /* NP Basic Error */
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_ALREADY_INITIALIZED),                // 0x8002a661
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_NOT_INITIALIZED),                    // 0x8002a662
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_NOT_SUPPORTED),                      // 0x8002a663
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_OUT_OF_MEMORY),                      // 0x8002a664
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_INVALID_ARGUMENT),                   // 0x8002a665
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_BAD_ID),                             // 0x8002a666
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_IDS_DIFFER),                         // 0x8002a667
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_PARSER_FAILED),                      // 0x8002a668
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_TIMEOUT),                            // 0x8002a669
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_NO_EVENT),                           // 0x8002a66a
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_EXCEEDS_MAX),                        // 0x8002a66b
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_INSUFFICIENT),                       // 0x8002a66c
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_NOT_REGISTERED),                     // 0x8002a66d
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_DATA_LOST),                          // 0x8002a66e
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_BUSY),                               // 0x8002a66f
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_STATUS),                             // 0x8002a670
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_CANCEL),                             // 0x8002a671
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_INVALID_MEMORY_CONTAINER),           // 0x8002a672
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_INVALID_DATA_ID),                    // 0x8002a673
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_BROKEN_DATA),                        // 0x8002a674
    #if CELL_SDK_VERSION >= 0x180002
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_BLOCKLIST_ADD_FAILED),               // 0x8002a675
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_BLOCKLIST_IS_FULL),                  // 0x8002a676
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_SEND_FAILED),                        // 0x8002a677
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_NOT_CONNECTED),                      // 0x8002a678
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_INSUFFICIENT_DISK_SPACE),            // 0x8002a679
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_INTERNAL_FAILURE),                   // 0x8002a67a
    #endif
    DIRTYSOCK_ErrorName(SCE_NP_BASIC_ERROR_UNKNOWN),                            // 0x8002a6bf

    DIRTYSOCK_ErrorName(SCE_NP_EXT_ERROR_CONTEXT_DOES_NOT_EXIST),               // 0x8002a6a1
    DIRTYSOCK_ErrorName(SCE_NP_EXT_ERROR_CONTEXT_ALREADY_EXISTS),               // 0x8002a6a2
    DIRTYSOCK_ErrorName(SCE_NP_EXT_ERROR_NO_CONTEXT),                           // 0x8002a6a3
    DIRTYSOCK_ErrorName(SCE_NP_EXT_ERROR_NO_ORIGIN),                            // 0x8002a6a4

    /* NP Manager Error */
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_NOT_INITIALIZED),                          // 0x8002aa01
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_ALREADY_INITIALIZED),                      // 0x8002aa02
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_INVALID_ARGUMENT),                         // 0x8002aa03
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_OUT_OF_MEMORY),                            // 0x8002aa04
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_ID_NO_SPACE),                              // 0x8002aa05
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_ID_NOT_FOUND),                             // 0x8002aa06
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_SESSION_RUNNING),                          // 0x8002aa07
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_LOGINID_ALREADY_EXISTS),                   // 0x8002aa08
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_INVALID_TICKET_SIZE),                      // 0x8002aa09
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_INVALID_STATE),                            // 0x8002aa0a
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_ABORTED),                                  // 0x8002aa0b
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_OFFLINE),                                  // 0x8002aa0c
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_VARIANT_ACCOUNT_ID),                       // 0x8002aa0d
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_GET_CLOCK),                                // 0x8002aa0e
    #if CELL_SDK_VERSION >= 0x180002
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_INSUFFICIENT_BUFFER),                      // 0x8002aa0f
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_EXPIRED_TICKET),                           // 0x8002aa10
    #endif

    /* NP Utility Error */
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_INVALID_ARGUMENT),                    // 0x8002ab01
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_OUT_OF_MEMORY),                       // 0x8002ab02
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_INSUFFICIENT),                        // 0x8002ab03
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_PARSER_FAILED),                       // 0x8002ab04
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_INVALID_PROTOCOL_ID),                 // 0x8002ab05
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_INVALID_NP_ID),                       // 0x8002ab06
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_INVALID_NP_LOBBY_ID),                 // 0x8002ab07
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_INVALID_NP_ROOM_ID),                  // 0x8002ab08
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_INVALID_NP_ENV),                      // 0x8002ab09
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_INVALID_TITLEID),                     // 0x8002ab0a
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_INVALID_CHARACTER),                   // 0x8002ab0b
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_INVALID_ESCAPE_STRING),               // 0x8002ab0c
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_UNKNOWN_TYPE),                        // 0x8002ab0d
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_UNKNOWN),                             // 0x8002ab0e
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_NOT_MATCH),                           // 0x8002ab0f

    /* NP Friendlist Error */
    #if CELL_SDK_VERSION >= 0x180002
    DIRTYSOCK_ErrorName(SCE_NP_FRIENDLIST_ERROR_ALREADY_INITIALIZED),           // 0x8002ab20
    DIRTYSOCK_ErrorName(SCE_NP_FRIENDLIST_ERROR_NOT_INITIALIZED),               // 0x8002ab21
    DIRTYSOCK_ErrorName(SCE_NP_FRIENDLIST_ERROR_OUT_OF_MEMORY),                 // 0x8002ab22
    DIRTYSOCK_ErrorName(SCE_NP_FRIENDLIST_ERROR_INVALID_MEMORY_CONTAINER),      // 0x8002ab23
    DIRTYSOCK_ErrorName(SCE_NP_FRIENDLIST_ERROR_INSUFFICIENT),                  // 0x8002ab24
    DIRTYSOCK_ErrorName(SCE_NP_FRIENDLIST_ERROR_CANCEL),                        // 0x8002ab25
    DIRTYSOCK_ErrorName(SCE_NP_FRIENDLIST_ERROR_STATUS),                        // 0x8002ab26
    DIRTYSOCK_ErrorName(SCE_NP_FRIENDLIST_ERROR_BUSY),                          // 0x8002ab27
    DIRTYSOCK_ErrorName(SCE_NP_FRIENDLIST_ERROR_INVALID_ARGUMENT),              // 0x8002ab28
    #endif

    // undocumented
    { 0x8002b001, "undoc_SCE_NP_NOT_INITALIZED" },

    /* NP Commerce Error */
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_NOT_INITIALIZED),                 // 0x80029401
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_ALREADY_INITIALIZED),             // 0x80029402
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_OUT_OF_MEMORY),                   // 0x80029403
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_UNSUPPORTED_VERSION),             // 0x80029404
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_CTX_MAX),                         // 0x80029405
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_CTX_NOT_FOUND),                   // 0x80029406
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_CTXID_NOT_AVAILABLE),             // 0x80029407
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_REQ_MAX),                         // 0x80029408
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_REQ_NOT_FOUND),                   // 0x80029409
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_REQID_NOT_AVAILABLE),             // 0x8002940a
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_INVALID_CATEGORY_ID),             // 0x8002940b
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_INVALID_LANG_CODE),               // 0x8002940c
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_REQ_BUSY),                        // 0x8002940d
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_INSUFFICIENT_BUFFER),             // 0x8002940e
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_INVALID_REQ_STATE),               // 0x8002940f
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_INVALID_CTX_STATE),               // 0x80029410
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_UNKNOWN),                         // 0x80029411
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_INVALID_REQ_TYPE),                // 0x80029412
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_INVALID_MEMORY_CONTAINER),        // 0x80029413
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_INSUFFICIENT_MEMORY_CONTAINER),   // 0x80029414
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_INVALID_DATA_FLAG_TYPE),          // 0x80029415
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_INVALID_DATA_FLAG_STATE),         // 0x80029416
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_DATA_FLAG_NUM_NOT_FOUND),         // 0x80029417
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_DATA_FLAG_INFO_NOT_FOUND),        // 0x80029418
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_INVALID_PROVIDER_ID),             // 0x80029419
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_INVALID_DATA_FLAG_NUM),           // 0x8002941a
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_INVALID_SKU_ID),                  // 0x8002941b
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_INVALID_DATA_FLAG_ID),            // 0x8002941c
    #if CELL_SDK_VERSION >= 0x180002
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_GPC_SEND_REQUEST),                // 0x8002941d
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_GDF_SEND_REQUEST),                // 0x8002941e
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_SDF_SEND_REQUEST),                // 0x8002941f
    #endif

    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_PARSE_PRODUCT_CATEGORY),          // 0x80029421
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_CURRENCY_INFO_NOT_FOUND),         // 0x80029422
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_CATEGORY_INFO_NOT_FOUND),         // 0x80029423
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_CHILD_CATEGORY_COUNT_NOT_FOUND),  // 0x80029424
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_CHILD_CATEGORY_INFO_NOT_FOUND),   // 0x80029425
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_SKU_COUNT_NOT_FOUND),             // 0x80029426
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_SKU_INFO_NOT_FOUND),              // 0x80029427
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_PLUGIN_LOAD_FAILURE),             // 0x80029428
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_INVALID_SKU_NUM),                 // 0x80029429
    #if CELL_SDK_VERSION >= 0x180002
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_INVALID_GPC_PROTOCOL_VERSION),    // 0x8002942a
    #endif

    #if CELL_SDK_VERSION >= 0x180002
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_CHECKOUT_UNEXPECTED),             // 0x80029430
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_CHECKOUT_OUT_OF_SERVICE),         // 0x80029431
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_CHECKOUT_INVALID_SKU),            // 0x80029432
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_CHECKOUT_SERVER_BUSY),            // 0x80029433
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_CHECKOUT_MAINTENANCE),            // 0x80029434
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_CHECKOUT_ACCOUNT_SUSPENDED),      // 0x80029435
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_CHECKOUT_OVER_SPENDING_LIMIT),    // 0x80029436
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_CHECKOUT_NOT_ENOUGH_MONEY),       // 0x80029437
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_ERROR_CHECKOUT_UNKNOWN),                // 0x80029438
    #endif

    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_BROWSE_SERVER_ERROR_UNKNOWN),               // 0x80029600
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_BROWSE_SERVER_ERROR_INVALID_CREDENTIALS),   // 0x80029601
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_BROWSE_SERVER_ERROR_INVALID_CATEGORY_ID),   // 0x80029602
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_BROWSE_SERVER_ERROR_SERVICE_END),           // 0x80029603
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_BROWSE_SERVER_ERROR_SERVICE_STOP),          // 0x80029604
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_BROWSE_SERVER_ERROR_SERVICE_BUSY),          // 0x80029605
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_BROWSE_SERVER_ERROR_UNSUPPORTED_VERSION),   // 0x80029606
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_BROWSE_SERVER_ERROR_INTERNAL_SERVER),       // 0x80029680

    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_GDF_SERVER_ERROR_UNKNOWN),              // 0x80029d00
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_GDF_SERVER_ERROR_INVALID_CREDENTIALS),  // 0x80029d01
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_GDF_SERVER_ERROR_INVALID_FLAGLIST),     // 0x80029d02
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_GDF_SERVER_ERROR_SERVICE_END),          // 0x80029d03
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_GDF_SERVER_ERROR_SERVICE_STOP),         // 0x80029d04
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_GDF_SERVER_ERROR_SERVICE_BUSY),         // 0x80029d05
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_GDF_SERVER_ERROR_UNSUPPORTED_VERSION),  // 0x80029d06

    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_SDF_SERVER_ERROR_UNKNOWN),              // 0x80029e00
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_SDF_SERVER_ERROR_INVALID_CREDENTIALS),  // 0x80029e01
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_SDF_SERVER_ERROR_INVALID_FLAGLIST),     // 0x80029e02
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_SDF_SERVER_ERROR_SERVICE_END),          // 0x80029e03
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_SDF_SERVER_ERROR_SERVICE_STOP),         // 0x80029e04
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_SDF_SERVER_ERROR_SERVICE_BUSY),         // 0x80029e05
    DIRTYSOCK_ErrorName(SCE_NP_COMMERCE_SDF_SERVER_ERROR_UNSUPPORTED_VERSION),  // 0x80029e06

    /* NP DRM error */
    DIRTYSOCK_ErrorName(SCE_NP_DRM_ERROR_OUT_OF_MEMORY),                        // 0x80029501
    DIRTYSOCK_ErrorName(SCE_NP_DRM_ERROR_SERVER_RESPONSE),                      // 0x80029509
    DIRTYSOCK_ErrorName(SCE_NP_DRM_ERROR_NO_ENTITLEMENT),                       // 0x80029513
    #if CELL_SDK_VERSION >= 0x180002
    DIRTYSOCK_ErrorName(SCE_NP_DRM_ERROR_BAD_ACT),                              // 0x80029514
    DIRTYSOCK_ErrorName(SCE_NP_DRM_ERROR_BAD_FORMAT),                           // 0x80029515
    DIRTYSOCK_ErrorName(SCE_NP_DRM_ERROR_NO_LOGIN),                             // 0x80029516
    #endif
    DIRTYSOCK_ErrorName(SCE_NP_DRM_ERROR_INTERNAL),                             // 0x80029517
    #if CELL_SDK_VERSION >= 0x180002
    DIRTYSOCK_ErrorName(SCE_NP_DRM_ERROR_BAD_PERM),                             // 0x80029519
    DIRTYSOCK_ErrorName(SCE_NP_DRM_ERROR_UNKNOWN_VERSION),                      // 0x8002951a
    DIRTYSOCK_ErrorName(SCE_NP_DRM_ERROR_TIME_LIMIT),                           // 0x8002951b
    DIRTYSOCK_ErrorName(SCE_NP_DRM_ERROR_DIFFERENT_ACCOUNT_ID),                 // 0x8002951c
    DIRTYSOCK_ErrorName(SCE_NP_DRM_ERROR_DIFFERENT_DRM_TYPE),                   // 0x8002951d
    DIRTYSOCK_ErrorName(SCE_NP_DRM_ERROR_SERVICE_NOT_STARTED),                  // 0x8002951e
    #endif
    #if CELL_SDK_VERSION >= 0x180002
    DIRTYSOCK_ErrorName(SCE_NP_DRM_ERROR_BUSY),                                 // 0x80029520
    DIRTYSOCK_ErrorName(SCE_NP_DRM_ERROR_IO),                                   // 0x80029525
    #endif
    DIRTYSOCK_ErrorName(SCE_NP_DRM_ERROR_FORMAT),                               // 0x80029530
    DIRTYSOCK_ErrorName(SCE_NP_DRM_ERROR_FILENAME),                             // 0x80029533
    DIRTYSOCK_ErrorName(SCE_NP_DRM_ERROR_K_LICENSEE),                           // 0x80029534

    DIRTYSOCK_ErrorName(SCE_NP_DRM_SERVER_ERROR_SERVICE_IS_END),                    // 0x80029700
    DIRTYSOCK_ErrorName(SCE_NP_DRM_SERVER_ERROR_SERVICE_STOPS_TEMPORARILY),         // 0x80029701
    DIRTYSOCK_ErrorName(SCE_NP_DRM_SERVER_ERROR_SERVICE_IS_BUSY),                   // 0x80029702
    DIRTYSOCK_ErrorName(SCE_NP_DRM_SERVER_ERROR_INVALID_USER_CREDENTIAL),           // 0x80029721
    DIRTYSOCK_ErrorName(SCE_NP_DRM_SERVER_ERROR_INVALID_PRODUCT_ID),                // 0x80029722
    DIRTYSOCK_ErrorName(SCE_NP_DRM_SERVER_ERROR_ACCOUNT_IS_CLOSED),                 // 0x80029730
    DIRTYSOCK_ErrorName(SCE_NP_DRM_SERVER_ERROR_ACCOUNT_IS_SUSPENDED),              // 0x80029731
    DIRTYSOCK_ErrorName(SCE_NP_DRM_SERVER_ERROR_ACTIVATED_CONSOLE_IS_FULL),         // 0x80029750
    DIRTYSOCK_ErrorName(SCE_NP_DRM_SERVER_ERROR_CONSOLE_NOT_ACTIVATED),             // 0x80029751
    DIRTYSOCK_ErrorName(SCE_NP_DRM_SERVER_ERROR_PRIMARY_CONSOLE_CANNOT_CHANGED),    // 0x80029752
    DIRTYSOCK_ErrorName(SCE_NP_DRM_SERVER_ERROR_UNKNOWN),                           // 0x80029780

    /*
        errors from netex\libnetctl.h
    */
    DIRTYSOCK_ErrorName(CELL_NET_CTL_ERROR_NOT_INITIALIZED),    // 0x80130101  Library module is not initialized.
    DIRTYSOCK_ErrorName(CELL_NET_CTL_ERROR_NOT_TERMINATED),     // 0x80130102  Not Terminated
    DIRTYSOCK_ErrorName(CELL_NET_CTL_ERROR_HANDLER_MAX),        // 0x80130103  there is no space for new handler
    DIRTYSOCK_ErrorName(CELL_NET_CTL_ERROR_ID_NOT_FOUND),       // 0x80130104  specified ID is not found
    DIRTYSOCK_ErrorName(CELL_NET_CTL_ERROR_INVALID_ID),         // 0x80130105  specified ID is invalid
    DIRTYSOCK_ErrorName(CELL_NET_CTL_ERROR_INVALID_CODE),       // 0x80130106  specified code is invalid
    DIRTYSOCK_ErrorName(CELL_NET_CTL_ERROR_INVALID_ADDR),       // 0x80130107  specified addr is invalid
    DIRTYSOCK_ErrorName(CELL_NET_CTL_ERROR_NOT_CONNECTED),      // 0x80130108  Not connected
    DIRTYSOCK_ErrorName(CELL_NET_CTL_ERROR_NOT_AVAIL),          // 0x80130109  Not available

    /*  error code
     *  CELL_ERROR_FACILITY_MICCAM          0x014
     *  "MicIn" interface error code:       0x8014_0101 - 0x8014_01ff
     *  "MicDSP" interface error code:      0x8014_0200 - 0x8014_02ff
     */
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_ALREADY_INIT),         // 0x80140101 - aready init
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_SYSTEM),               // 0x80140102 - error in device
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_NOT_INIT),             // 0x80140103 - not init
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_PARAM),                // 0x80140104 - param error
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_PORT_FULL),            // 0x80140105 - device port is full
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_ALREADY_OPEN),         // 0x80140106 - device is already opened
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_NOT_OPEN),             // 0x80140107 - device is already closed
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_NOT_RUN),              // 0x80140108 - device is not running
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_TRANS_EVENT),          // 0x80140109 - device trans event error
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_OPEN),                 // 0x8014010a - error in open
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_SHAREDMEMORY),         // 0x8014010b - error in shared memory
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_MUTEX),                // 0x8014010c - error in mutex
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_EVENT_QUEUE),          // 0x8014010d - error in event queue
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_SYSTEM_NOT_FOUND),     // 0x8014010e - device not found
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_FATAL),                // 0x8014010f - generic error
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_SYSTEM_NOT_SUPPORT),   // 0x80140110 - device is not supported

    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_DEVICE),               // CELL_MICIN_ERROR_SYSTEM
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_DEVICE_NOT_FOUND),     // CELL_MICIN_ERROR_SYSTEM_NOT_FOUND
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_DEVICE_NOT_SUPPORT),   // CELL_MICIN_ERROR_SYSTEM_NOT_SUPPORT

    /*
     * error code for microphone-signal-processing-library: 0x8014_0200 - 0x8014_02ff
     */
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_DSP),                  // 0x80140200 - generic error
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_DSP_ASSERT),           // 0x80140201 - assertion failure
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_DSP_PATH),             // 0x80140202 - Path is wrong in file system
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_DSP_FILE),             // 0x80140203 - File is currupped with
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_DSP_PARAM),            // 0x80140204 - called function arg/param is bad
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_DSP_MEMALLOC),         // 0x80140205 - insufficient memory
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_DSP_POINTER),          // 0x80140206 - Bad pointer to invalid memory addr
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_DSP_FUNC),             // 0x80140207 - unsupported function
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_DSP_MEM),              // 0x80140208 - bad memory region
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_DSP_ALIGN16),          // 0x80140209 - bad memory alignment in 16 bytes
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_DSP_ALIGN128),         // 0x8014020a - bad memory alignment in 128 bytes
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_DSP_EAALIGN128),       // 0x8014020b - bad effective address(EA) alignment in 128 bytes
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_DSP_LIB_HANDLER),      // 0x80140216 - invalid dsp library handler, mostly memory curruption
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_DSP_LIB_INPARAM),      // 0x80140217 - unrecognized input parameter to dsp library API
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_DSP_LIB_NOSPU),        // 0x80140218 - failure to communicate with SPU side dsp library
    DIRTYSOCK_ErrorName(CELL_MICIN_ERROR_DSP_LIB_SAMPRATE),     // 0x80140219 - this sampling rate is not supported by dsp library

    // sysutil_webbrowser error codes
    DIRTYSOCK_ErrorName(CELL_WEBBROWSER_ERROR_NOMEM),         // 0x8002b901
    DIRTYSOCK_ErrorName(CELL_WEBBROWSER_ERROR_INVALID),       // 0x8002b902
    DIRTYSOCK_ErrorName(CELL_WEBBROWSER_ERROR_EXCLUSIVE),     // 0x8002b903

    #if CELL_SDK_VERSION < 0x280001
    // game data error codes
    DIRTYSOCK_ErrorName(CELL_GAMEDATA_ERROR_ACCESS_ERROR),      // 0x8002b602 - HDD access error
    DIRTYSOCK_ErrorName(CELL_GAMEDATA_ERROR_INTERNAL),          // 0x8002b603 - Fatal internal error
    DIRTYSOCK_ErrorName(CELL_GAMEDATA_ERROR_PARAM),             // 0x8002b604 - Invalid parameter value sent to function
    DIRTYSOCK_ErrorName(CELL_GAMEDATA_ERROR_NOSPACE),           // 0x8002b605 - Insufficient free space on HDD
    DIRTYSOCK_ErrorName(CELL_GAMEDATA_ERROR_BROKEN),            // 0x8002b606 - Game data corrupted
    #endif

    DIRTYSOCK_ErrorName(CELL_AUDIO_ERROR_ALREADY_INIT),         // 0x80310701) - aready init
    DIRTYSOCK_ErrorName(CELL_AUDIO_ERROR_AUDIOSYSTEM),          // 0x80310702) - error in AudioSystem.
    DIRTYSOCK_ErrorName(CELL_AUDIO_ERROR_NOT_INIT),             // 0x80310703) - not init
    DIRTYSOCK_ErrorName(CELL_AUDIO_ERROR_PARAM),                // 0x80310704) - param error
    DIRTYSOCK_ErrorName(CELL_AUDIO_ERROR_PORT_FULL),            // 0x80310705) - audio port is full
    DIRTYSOCK_ErrorName(CELL_AUDIO_ERROR_PORT_ALREADY_RUN),     // 0x80310706) - audio port is aready run
    DIRTYSOCK_ErrorName(CELL_AUDIO_ERROR_PORT_NOT_OPEN),        // 0x80310707) - audio port is close
    DIRTYSOCK_ErrorName(CELL_AUDIO_ERROR_PORT_NOT_RUN),         // 0x80310708) - audio port is not run
    DIRTYSOCK_ErrorName(CELL_AUDIO_ERROR_TRANS_EVENT),          // 0x80310709) - trans event error
    DIRTYSOCK_ErrorName(CELL_AUDIO_ERROR_PORT_OPEN),            // 0x8031070a) - error in port open
    DIRTYSOCK_ErrorName(CELL_AUDIO_ERROR_SHAREDMEMORY),         // 0x8031070b) - error in shared memory
    DIRTYSOCK_ErrorName(CELL_AUDIO_ERROR_MUTEX),                // 0x8031070c) - error in mutex
    DIRTYSOCK_ErrorName(CELL_AUDIO_ERROR_EVENT_QUEUE),          // 0x8031070d) - error in event queue
    DIRTYSOCK_ErrorName(CELL_AUDIO_ERROR_AUDIOSYSTEM_NOT_FOUND),// 0x8031070e)
    DIRTYSOCK_ErrorName(CELL_AUDIO_ERROR_TAG_NOT_FOUND),        // 0x8031070f)

    DIRTYSOCK_ErrorName(CELL_ADEC_ERROR_FATAL),                 // 0x80610001 - Fatal ADEC Error
    DIRTYSOCK_ErrorName(CELL_ADEC_ERROR_SEQ),                   // 0x80610002 - ADEC Sequence Error
    DIRTYSOCK_ErrorName(CELL_ADEC_ERROR_ARG),                   // 0x80610003 - ADEC Argument Error
    DIRTYSOCK_ErrorName(CELL_ADEC_ERROR_BUSY),                  // 0x80610004 - ADEC Busy
    DIRTYSOCK_ErrorName(CELL_ADEC_ERROR_EMPTY),                 // 0x80610005 - ADEC Empty

    DIRTYSOCK_ErrorName(CELL_ADEC_ERROR_CELP_BUSY),         // 0x80612e01
    DIRTYSOCK_ErrorName(CELL_ADEC_ERROR_CELP_EMPTY),        // 0x80612e02
    DIRTYSOCK_ErrorName(CELL_ADEC_ERROR_CELP_ARG),          // 0x80612e03
    DIRTYSOCK_ErrorName(CELL_ADEC_ERROR_CELP_SEQ),          // 0x80612e04
    DIRTYSOCK_ErrorName(CELL_ADEC_ERROR_CELP_CORE_FATAL),   // 0x80612e81
    DIRTYSOCK_ErrorName(CELL_ADEC_ERROR_CELP_CORE_ARG),     // 0x80612e82
    DIRTYSOCK_ErrorName(CELL_ADEC_ERROR_CELP_CORE_SEQ),     // 0x80612e83

    DIRTYSOCK_ErrorName(CELL_CELPENC_ERROR_FAILED),     // 0x80614001 - CELP Encoder Failure
    DIRTYSOCK_ErrorName(CELL_CELPENC_ERROR_SEQ),        // 0x80614002 - CELP Sequence Error
    DIRTYSOCK_ErrorName(CELL_CELPENC_ERROR_ARG),        // 0x80614003 - CELP Bad Argument
    DIRTYSOCK_ErrorName(CELL_CELPENC_ERROR_CORE_FAILED),// 0x80614081 - CELP CORE_FAILED
    DIRTYSOCK_ErrorName(CELL_CELPENC_ERROR_CORE_SEQ),   // 0x80614082 - CELP CORE_SEQ
    DIRTYSOCK_ErrorName(CELL_CELPENC_ERROR_CORE_ARG),   // 0x80614083 - CELP CORE_ARG

    DIRTYSOCK_ErrorName(CELL_CELP8ENC_ERROR_FAILED),        // 0x806140a1 - CELP8 Encoder Failure
    DIRTYSOCK_ErrorName(CELL_CELP8ENC_ERROR_SEQ),           // 0x806140a2 - CELP8 Sequence Error
    DIRTYSOCK_ErrorName(CELL_CELP8ENC_ERROR_ARG),           // 0x806140a3 - CELP8 Bad Argument
    DIRTYSOCK_ErrorName(CELL_CELP8ENC_ERROR_CORE_FAILED),   // 0x806140b1 - CELP8 CORE_FAILED
    DIRTYSOCK_ErrorName(CELL_CELP8ENC_ERROR_CORE_SEQ),      // 0x806140b2 - CELP8 CORE_SEQ
    DIRTYSOCK_ErrorName(CELL_CELP8ENC_ERROR_CORE_ARG),      // 0x806140b3 - CELP8 CORE_ARG

    // NULL terminate
    DIRTYSOCK_ListEnd()
};
#endif

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function DirtyErrNameList

    \Description
        This function takes as input a system-specific error code, and either
        resolves it to its define name if it is recognized or formats it as a hex
        number if not.

    \Input *pBuffer - [out] pointer to output buffer to store result
    \Input iBufSize - size of output buffer
    \Input uError   - error code to format
    \Input *pList   - error list to use

    \Output
        None.

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
void DirtyErrNameList(char *pBuffer, int32_t iBufSize, uint32_t uError, const DirtyErrT *pList)
{
    #if DIRTYSOCK_ERRORNAMES
    int32_t iErr;

    for (iErr = 0; pList[iErr].uError != DIRTYSOCK_LISTTERM; iErr++)
    {
        if (pList[iErr].uError == uError)
        {
            snzprintf(pBuffer, iBufSize, "%s/%d", pList[iErr].pErrorName, (signed)uError);
            return;
        }
    }
    #endif

    snzprintf(pBuffer, iBufSize, "%d", uError);
}

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

    for (iErr = 0; _DirtyErr_List[iErr].uError != DIRTYSOCK_LISTTERM; iErr++)
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
    \Function DirtyErrGetNameList

    \Description
        This function takes as input a system-specific error code, and either
        resolves it to its define name if it is recognized or formats it as a hex
        number if not.

    \Input uError   - error code to format
    \Input *pList   - error list to use

    \Output
        const char *- pointer to error name or error formatted in hex

    \Version 10/04/2005 (jbrookes)
*/
/********************************************************************************F*/
const char *DirtyErrGetNameList(uint32_t uError, const DirtyErrT *pList)
{
    static char strName[8][64];
    static uint8_t uLast = 0;
    char *pName = strName[uLast++];
    if (uLast > 7) uLast = 0;
    DirtyErrNameList(pName, sizeof(strName[0]), uError, pList);
    return(pName);
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
