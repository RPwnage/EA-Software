/*H********************************************************************************/
/*!
    \File dirtyerrps4.c

    \Description
        Dirtysock debug error routines.

    \Copyright
        Copyright (c) 2012 Electronic Arts Inc.

    \Version 10/24/2012 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <sdk_version.h>

// error includes
#include <net.h>
#include <libnetctl.h>
#include <sys/sce_errno.h>
#include <libnet/errno.h>
#include <scebase_common.h>
#include <user_service/user_service_error.h>
#include <voice/voice_types.h>
#include <common_dialog/error.h>
#include <np.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyerr.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

static DirtySockAppErrorCallback pAppErrorCallback = NULL;

#if DIRTYSOCK_ERRORNAMES
static DirtyErrT _DirtyErr_List[] =
{
    /*
        libnet/errno.h error codes
    */
    DIRTYSOCK_ErrorName(SCE_NET_EPERM),                     // 1
    DIRTYSOCK_ErrorName(SCE_NET_ENOENT),                    // 2
    DIRTYSOCK_ErrorName(SCE_NET_ESRCH),                     // 3
    DIRTYSOCK_ErrorName(SCE_NET_EINTR),                     // 4
    DIRTYSOCK_ErrorName(SCE_NET_EIO),                       // 5
    DIRTYSOCK_ErrorName(SCE_NET_ENXIO),                     // 6
    DIRTYSOCK_ErrorName(SCE_NET_E2BIG),                     // 7
    DIRTYSOCK_ErrorName(SCE_NET_ENOEXEC),                   // 8
    DIRTYSOCK_ErrorName(SCE_NET_EBADF),                     // 9
    DIRTYSOCK_ErrorName(SCE_NET_ECHILD),                    // 10
    DIRTYSOCK_ErrorName(SCE_NET_EDEADLK),                   // 11
    DIRTYSOCK_ErrorName(SCE_NET_ENOMEM),                    // 12
    DIRTYSOCK_ErrorName(SCE_NET_EACCES),                    // 13
    DIRTYSOCK_ErrorName(SCE_NET_EFAULT),                    // 14
    DIRTYSOCK_ErrorName(SCE_NET_ENOTBLK),                   // 15
    DIRTYSOCK_ErrorName(SCE_NET_EBUSY),                     // 16
    DIRTYSOCK_ErrorName(SCE_NET_EEXIST),                    // 17
    DIRTYSOCK_ErrorName(SCE_NET_EXDEV),                     // 18
    DIRTYSOCK_ErrorName(SCE_NET_ENODEV),                    // 19
    DIRTYSOCK_ErrorName(SCE_NET_ENOTDIR),                   // 20
    DIRTYSOCK_ErrorName(SCE_NET_EISDIR),                    // 21
    DIRTYSOCK_ErrorName(SCE_NET_EINVAL),                    // 22
    DIRTYSOCK_ErrorName(SCE_NET_ENFILE),                    // 23
    DIRTYSOCK_ErrorName(SCE_NET_EMFILE),                    // 24
    DIRTYSOCK_ErrorName(SCE_NET_ENOTTY),                    // 25
    DIRTYSOCK_ErrorName(SCE_NET_ETXTBSY),                   // 26
    DIRTYSOCK_ErrorName(SCE_NET_EFBIG),                     // 27
    DIRTYSOCK_ErrorName(SCE_NET_ENOSPC),                    // 28
    DIRTYSOCK_ErrorName(SCE_NET_ESPIPE),                    // 29
    DIRTYSOCK_ErrorName(SCE_NET_EROFS),                     // 30
    DIRTYSOCK_ErrorName(SCE_NET_EMLINK),                    // 31
    DIRTYSOCK_ErrorName(SCE_NET_EPIPE),                     // 32
    DIRTYSOCK_ErrorName(SCE_NET_EDOM),                      // 33
    DIRTYSOCK_ErrorName(SCE_NET_ERANGE),                    // 34
    DIRTYSOCK_ErrorName(SCE_NET_EAGAIN),                    // 35
    DIRTYSOCK_ErrorName(SCE_NET_EWOULDBLOCK),               // SCE_NET_EAGAIN
    DIRTYSOCK_ErrorName(SCE_NET_EINPROGRESS),               // 36
    DIRTYSOCK_ErrorName(SCE_NET_EALREADY),                  // 37
    DIRTYSOCK_ErrorName(SCE_NET_ENOTSOCK),                  // 38
    DIRTYSOCK_ErrorName(SCE_NET_EDESTADDRREQ),              // 39
    DIRTYSOCK_ErrorName(SCE_NET_EMSGSIZE),                  // 40
    DIRTYSOCK_ErrorName(SCE_NET_EPROTOTYPE),                // 41
    DIRTYSOCK_ErrorName(SCE_NET_ENOPROTOOPT),               // 42
    DIRTYSOCK_ErrorName(SCE_NET_EPROTONOSUPPORT),           // 43
    DIRTYSOCK_ErrorName(SCE_NET_ESOCKTNOSUPPORT),           // 44
    DIRTYSOCK_ErrorName(SCE_NET_EOPNOTSUPP),                // 45
    DIRTYSOCK_ErrorName(SCE_NET_EPFNOSUPPORT),              // 46
    DIRTYSOCK_ErrorName(SCE_NET_EAFNOSUPPORT),              // 47
    DIRTYSOCK_ErrorName(SCE_NET_EADDRINUSE),                // 48
    DIRTYSOCK_ErrorName(SCE_NET_EADDRNOTAVAIL),             // 49
    DIRTYSOCK_ErrorName(SCE_NET_ENETDOWN),                  // 50
    DIRTYSOCK_ErrorName(SCE_NET_ENETUNREACH),               // 51
    DIRTYSOCK_ErrorName(SCE_NET_ENETRESET),                 // 52
    DIRTYSOCK_ErrorName(SCE_NET_ECONNABORTED),              // 53
    DIRTYSOCK_ErrorName(SCE_NET_ECONNRESET),                // 54
    DIRTYSOCK_ErrorName(SCE_NET_ENOBUFS),                   // 55
    DIRTYSOCK_ErrorName(SCE_NET_EISCONN),                   // 56
    DIRTYSOCK_ErrorName(SCE_NET_ENOTCONN),                  // 57
    DIRTYSOCK_ErrorName(SCE_NET_ESHUTDOWN),                 // 58
    DIRTYSOCK_ErrorName(SCE_NET_ETOOMANYREFS),              // 59
    DIRTYSOCK_ErrorName(SCE_NET_ETIMEDOUT),                 // 60
    DIRTYSOCK_ErrorName(SCE_NET_ECONNREFUSED),              // 61
    DIRTYSOCK_ErrorName(SCE_NET_ELOOP),                     // 62
    DIRTYSOCK_ErrorName(SCE_NET_ENAMETOOLONG),              // 63
    DIRTYSOCK_ErrorName(SCE_NET_EHOSTDOWN),                 // 64
    DIRTYSOCK_ErrorName(SCE_NET_EHOSTUNREACH),              // 65
    DIRTYSOCK_ErrorName(SCE_NET_ENOTEMPTY),                 // 66
    DIRTYSOCK_ErrorName(SCE_NET_EPROCLIM),                  // 67
    DIRTYSOCK_ErrorName(SCE_NET_EUSERS),                    // 68
    DIRTYSOCK_ErrorName(SCE_NET_EDQUOT),                    // 69
    DIRTYSOCK_ErrorName(SCE_NET_ESTALE),                    // 70
    DIRTYSOCK_ErrorName(SCE_NET_EREMOTE),                   // 71
    DIRTYSOCK_ErrorName(SCE_NET_EBADRPC),                   // 72
    DIRTYSOCK_ErrorName(SCE_NET_ERPCMISMATCH),              // 73
    DIRTYSOCK_ErrorName(SCE_NET_EPROGUNAVAIL),              // 74
    DIRTYSOCK_ErrorName(SCE_NET_EPROGMISMATCH),             // 75
    DIRTYSOCK_ErrorName(SCE_NET_EPROCUNAVAIL),              // 76
    DIRTYSOCK_ErrorName(SCE_NET_ENOLCK),                    // 77
    DIRTYSOCK_ErrorName(SCE_NET_ENOSYS),                    // 78
    DIRTYSOCK_ErrorName(SCE_NET_EFTYPE),                    // 79
    DIRTYSOCK_ErrorName(SCE_NET_EAUTH),                     // 80
    DIRTYSOCK_ErrorName(SCE_NET_ENEEDAUTH),                 // 81
    DIRTYSOCK_ErrorName(SCE_NET_EIDRM),                     // 82
    DIRTYSOCK_ErrorName(SCE_NET_ENOMSG),                    // 83
    DIRTYSOCK_ErrorName(SCE_NET_EOVERFLOW),                 // 84
    DIRTYSOCK_ErrorName(SCE_NET_ECANCELED),                 // 85
    DIRTYSOCK_ErrorName(SCE_NET_EADHOC),                    // 160
    DIRTYSOCK_ErrorName(SCE_NET_ERESERVED161),              // 161
    DIRTYSOCK_ErrorName(SCE_NET_ERESERVED162),              // 162
    DIRTYSOCK_ErrorName(SCE_NET_ENODATA),                   // 164
    DIRTYSOCK_ErrorName(SCE_NET_EDESC),                     // 165
    DIRTYSOCK_ErrorName(SCE_NET_EDESCTIMEDOUT),             // 166
    // libnet
    DIRTYSOCK_ErrorName(SCE_NET_ENOTINIT),                  // 200
    DIRTYSOCK_ErrorName(SCE_NET_ENOLIBMEM),                 // 201
    DIRTYSOCK_ErrorName(SCE_NET_ETLS),                      // 202
    DIRTYSOCK_ErrorName(SCE_NET_ECALLBACK),                 // 203
    DIRTYSOCK_ErrorName(SCE_NET_EINTERNAL),                 // 204
    DIRTYSOCK_ErrorName(SCE_NET_ERETURN),                   // 205
    DIRTYSOCK_ErrorName(SCE_NET_ENOALLOCMEM),               // 206
    // resolver
    DIRTYSOCK_ErrorName(SCE_NET_RESOLVER_EINTERNAL),        // 220
    DIRTYSOCK_ErrorName(SCE_NET_RESOLVER_EBUSY),            // 221
    DIRTYSOCK_ErrorName(SCE_NET_RESOLVER_ENOSPACE),         // 222
    DIRTYSOCK_ErrorName(SCE_NET_RESOLVER_EPACKET),          // 223
    DIRTYSOCK_ErrorName(SCE_NET_RESOLVER_ERESERVED224),     // 224
    DIRTYSOCK_ErrorName(SCE_NET_RESOLVER_ENODNS),           // 225
    DIRTYSOCK_ErrorName(SCE_NET_RESOLVER_ETIMEDOUT),        // 226
    DIRTYSOCK_ErrorName(SCE_NET_RESOLVER_ENOSUPPORT),       // 227
    DIRTYSOCK_ErrorName(SCE_NET_RESOLVER_EFORMAT),          // 228
    DIRTYSOCK_ErrorName(SCE_NET_RESOLVER_ESERVERFAILURE),   // 229
    DIRTYSOCK_ErrorName(SCE_NET_RESOLVER_ENOHOST),          // 230
    DIRTYSOCK_ErrorName(SCE_NET_RESOLVER_ENOTIMPLEMENTED),  // 231
    DIRTYSOCK_ErrorName(SCE_NET_RESOLVER_ESERVERREFUSED),   // 232
    DIRTYSOCK_ErrorName(SCE_NET_RESOLVER_ENORECORD),        // 233
    DIRTYSOCK_ErrorName(SCE_NET_RESOLVER_EALIGNMENT),       // 234

    /*
        errors from libnet/errno.h
    */
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EPERM),                   // 0x80410101
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENOENT),                  // 0x80410102
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ESRCH),                   // 0x80410103
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EINTR),                   // 0x80410104
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EIO),                     // 0x80410105
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENXIO),                   // 0x80410106
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_E2BIG),                   // 0x80410107
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENOEXEC),                 // 0x80410108
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EBADF),                   // 0x80410109
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ECHILD),                  // 0x8041010A
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EAGAIN),                  // 0x8041010B
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENOMEM),                  // 0x8041010C
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EACCES),                  // 0x8041010D
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EFAULT),                  // 0x8041010E
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENOTBLK),                 // 0x8041010F
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EBUSY),                   // 0x80410110
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EEXIST),                  // 0x80410111
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EXDEV),                   // 0x80410112
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENODEV),                  // 0x80410113
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENOTDIR),                 // 0x80410114
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EISDIR),                  // 0x80410115
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EINVAL),                  // 0x80410116
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENFILE),                  // 0x80410117
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EMFILE),                  // 0x80410118
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENOTTY),                  // 0x80410119
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ETXTBSY),                 // 0x8041011A
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EFBIG),                   // 0x8041011B
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENOSPC),                  // 0x8041011C
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ESPIPE),                  // 0x8041011D
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EROFS),                   // 0x8041011E
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EMLINK),                  // 0x8041011F
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EPIPE),                   // 0x80410120
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EDOM),                    // 0x80410121
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ERANGE),                  // 0x80410122
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EAGAIN),                  // 0x80410123
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EIDRM),                   // 0x80410124
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EALREADY),                // 0x80410125
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENOTSOCK),                // 0x80410126
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EDESTADDRREQ),            // 0x80410127
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EMSGSIZE),                // 0x80410128
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EPROTOTYPE),              // 0x80410129
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENOPROTOOPT),             // 0x8041012A
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EPROTONOSUPPORT),         // 0x8041012B
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ESOCKTNOSUPPORT),         // 0x8041012C
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EOPNOTSUPP),              // 0x8041012D
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EPFNOSUPPORT),            // 0x8041012E
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EAFNOSUPPORT),            // 0x8041012F
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EADDRINUSE),              // 0x80410130
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EADDRNOTAVAIL),           // 0x80410131
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENETDOWN),                // 0x80410132
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENETUNREACH),             // 0x80410133
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENETRESET),               // 0x80410134
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ECONNABORTED),            // 0x80410135
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ECONNRESET),              // 0x80410136
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENOBUFS),                 // 0x80410137
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EISCONN),                 // 0x80410138
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENOTCONN),                // 0x80410139
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ESHUTDOWN),               // 0x8041013A
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ETOOMANYREFS),            // 0x8041013B
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ETIMEDOUT),               // 0x8041013C
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ECONNREFUSED),            // 0x8041013D
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ELOOP),                   // 0x8041013E
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENAMETOOLONG),            // 0x8041013F
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EHOSTDOWN),               // 0x80410140
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EHOSTUNREACH),            // 0x80410141
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENOTEMPTY),               // 0x80410142
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EPROCLIM),                // 0x80410143
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EUSERS),                  // 0x80410144
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EDQUOT),                  // 0x80410145
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ESTALE),                  // 0x80410146
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EREMOTE),                 // 0x80410147
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EBADRPC),                 // 0x80410148
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ERPCMISMATCH),            // 0x80410149
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EPROGUNAVAIL),            // 0x8041014A
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EPROGMISMATCH),           // 0x8041014B
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EPROCUNAVAIL),            // 0x8041014C
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENOLCK),                  // 0x8041014D
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENOSYS),                  // 0x8041014E
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EFTYPE),                  // 0x8041014F
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EAUTH),                   // 0x80410150
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENEEDAUTH),               // 0x80410151
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EIDRM),                   // 0x80410152
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENOMS),                   // 0x80410153
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EOVERFLOW),               // 0x80410154
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ECANCELED),               // 0x80410155
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EADHOC),                  // 0x804101A0
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ERESERVED161),            // 0x804101A1
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ERESERVED162),            // 0x804101A2
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENODATA),                 // 0x804101A4
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EDESC),                   // 0x804101A5
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EDESCTIMEDOUT),           // 0x804101A6
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENOTINIT),                // 0x804101C8
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENOLIBMEM),               // 0x804101C9
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ERESERVED202),            // 0x804101CA
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ECALLBACK),               // 0x804101CB
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_EINTERNAL),               // 0x804101CC
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ERETURN),                 // 0x804101CD
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_ENOALLOCMEM),             // 0x804101CE
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_RESOLVER_EINTERNAL),      // 0x804101DC
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_RESOLVER_EBUSY),          // 0x804101DD
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_RESOLVER_ENOSPACE),       // 0x804101DE
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_RESOLVER_EPACKET),        // 0x804101DF
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_RESOLVER_ERESERVED224),   // 0x804101E0
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_RESOLVER_ENODNS),         // 0x804101E1
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_RESOLVER_ETIMEDOUT),      // 0x804101E2
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_RESOLVER_ENOSUPPORT),     // 0x804101E3
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_RESOLVER_EFORMAT),        // 0x804101E4
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_RESOLVER_ESERVERFAILURE), // 0x804101E5
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_RESOLVER_ENOHOST),        // 0x804101E6
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_RESOLVER_ENOTIMPLEMENTED),// 0x804101E7
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_RESOLVER_ESERVERREFUSED), // 0x804101E8
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_RESOLVER_ENORECORD),      // 0x804101E9
    DIRTYSOCK_ErrorName(SCE_NET_ERROR_RESOLVER_EALIGNMENT),     // 0x804101EA

    /*
        errors from libnetctl_error.h
    */
    DIRTYSOCK_ErrorName(SCE_NET_CTL_ERROR_NOT_INITIALIZED),     // 0x80412101
    DIRTYSOCK_ErrorName(SCE_NET_CTL_ERROR_INVALID_CODE),        // 0x80412106
    DIRTYSOCK_ErrorName(SCE_NET_CTL_ERROR_INVALID_ADDR),        // 0x80412107
    DIRTYSOCK_ErrorName(SCE_NET_CTL_ERROR_NOT_CONNECTED),       // 0x80412108
    DIRTYSOCK_ErrorName(SCE_NET_CTL_ERROR_NOT_AVAIL),           // 0x80412109

    /*
        errors from np_error.h
    */

    DIRTYSOCK_ErrorName(SCE_NP_ERROR_ALREADY_INITIALIZED),                                  // -2141913087/0x80550001
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_NOT_INITIALIZED),                                      // -2141913086/0x80550002
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_INVALID_ARGUMENT),                                     // -2141913085/0x80550003
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_UNKNOWN_PLATFORM_TYPE),                                // -2141913084/0x80550004
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_OUT_OF_MEMORY),                                        // -2141913083/0x80550005
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_SIGNED_OUT),                                           // -2141913082/0x80550006
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_USER_NOT_FOUND),                                       // -2141913081/0x80550007
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_CALLBACK_ALREADY_REGISTERED),                          // -2141913080/0x80550008
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_CALLBACK_NOT_REGISTERED),                              // -2141913079/0x80550009
    #if ORBIS_SDK_VERSION >= 1020
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_NOT_SIGNED_UP),                                        // -2141913078/0x8055000A
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_AGE_RESTRICTION),                                      // -2141913077/0x8055000B
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_LOGOUT),                                               // -2141913076/0x8055000C
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_LATEST_SYSTEM_SOFTWARE_EXIST),                         // -2141913075/0x8055000D
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_LATEST_SYSTEM_SOFTWARE_EXIST_FOR_TITLE),               // -2141913074/0x8055000E
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_LATEST_PATCH_PKG_EXIST),                               // -2141913073/0x8055000F
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_LATEST_PATCH_PKG_DOWNLOADED),                          // -2141913072/0x80550010
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_INVALID_SIZE),                                         // -2141913071/0x80550011
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_ABORTED),                                              // -2141913070/0x80550012
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_REQUEST_MAX),                                          // -2141913069/0x80550013
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_REQUEST_NOT_FOUND),                                    // -2141913068/0x80550014
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_INVALID_ID),                                           // -2141913067/0x80550015
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_NP_TITLE_DAT_NOT_FOUND),                               // -2141913066/0x80550016
    DIRTYSOCK_ErrorName(SCE_NP_ERROR_INCONSISTENT_NP_TITLE_ID),                             // -2141913065/0x80550017
    #endif

    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_INVALID_ARGUMENT),                                // -2141912319/0x80550301
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_INVALID_SIZE),                                    // -2141912318/0x80550302
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_OUT_OF_MEMORY),                                   // -2141912317/0x80550303
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ABORTED),                                         // -2141912316/0x80550304
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_REQUEST_MAX),                                     // -2141912315/0x80550305
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_REQUEST_NOT_FOUND),                               // -2141912314/0x80550306
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_INVALID_ID),                                      // -2141912313/0x80550307
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_SERVICE_END),                                     // -2141912064/0x80550400
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_SERVICE_DOWN),                                    // -2141912063/0x80550401
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_SERVICE_BUSY),                                    // -2141912062/0x80550402
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_SERVER_MAINTENANCE),                              // -2141912061/0x80550403
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_S_INVALID_DATA_LENGTH),                           // -2141912048/0x80550410
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_S_INVALID_USER_AGENT),                            // -2141912047/0x80550411
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_S_INVALID_VERSION),                               // -2141912046/0x80550412
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_S_INVALID_SERVICE_ID),                            // -2141912032/0x80550420
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_S_INVALID_CREDENTIAL),                            // -2141912031/0x80550421
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_S_INVALID_ENTITLEMENT_ID),                        // -2141912030/0x80550422
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_S_INVALID_CONSUMED_COUNT),                        // -2141912029/0x80550423
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_INVALID_CONSOLE_ID),                              // -2141912028/0x80550424
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_CONSOLE_ID_SUSPENDED),                            // -2141912025/0x80550427
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_CLOSED),                                  // -2141912016/0x80550430
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_SUSPENDED),                               // -2141912015/0x80550431
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_EULA),                              // -2141912014/0x80550432
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT1),                          // -2141912000/0x80550440
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT2),                          // -2141911999/0x80550441
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT3),                          // -2141911998/0x80550442
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT4),                          // -2141911997/0x80550443
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT5),                          // -2141911996/0x80550444
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT6),                          // -2141911995/0x80550445
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT7),                          // -2141911994/0x80550446
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT8),                          // -2141911993/0x80550447
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT9),                          // -2141911992/0x80550448
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT10),                         // -2141911991/0x80550449
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT11),                         // -2141911990/0x8055044A
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT12),                         // -2141911989/0x8055044B
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT13),                         // -2141911988/0x8055044C
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT14),                         // -2141911987/0x8055044D
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT15),                         // -2141911986/0x8055044E
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_ACCOUNT_RENEW_ACCOUNT16),                         // -2141911985/0x8055044F
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_SUB_ACCOUNT_RENEW_EULA),                          // -2141911985/0x8055044F
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_ERROR_UNKNOWN),                                         // -2141911936/0x80550480

    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_INVALID_ARGUMENT),                                // -2141911551/0x80550601
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_INSUFFICIENT),                                    // -2141911550/0x80550602
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_PARSER_FAILED),                                   // -2141911549/0x80550603
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_INVALID_PROTOCOL_ID),                             // -2141911548/0x80550604
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_INVALID_NP_ID),                                   // -2141911547/0x80550605
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_INVALID_NP_ENV),                                  // -2141911546/0x80550606
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_INVALID_CHARACTER),                               // -2141911544/0x80550608
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_NOT_MATCH),                                       // -2141911543/0x80550609
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_INVALID_TITLEID),                                 // -2141911542/0x8055060A
    DIRTYSOCK_ErrorName(SCE_NP_UTIL_ERROR_UNKNOWN),                                         // -2141911538/0x8055060E
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_ALREADY_INITIALIZED),                        // -2141911295/0x80550701
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_NOT_INITIALIZED),                            // -2141911294/0x80550702
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_OUT_OF_MEMORY),                              // -2141911293/0x80550703
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_INVALID_ARGUMENT),                           // -2141911292/0x80550704
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_NO_LOGIN),                                   // -2141911291/0x80550705
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS),                           // -2141911290/0x80550706
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_ABORTED),                                    // -2141911289/0x80550707
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_BAD_RESPONSE),                               // -2141911288/0x80550708
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_BODY_TOO_LARGE),                             // -2141911287/0x80550709
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_HTTP_SERVER),                                // -2141911286/0x8055070A
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_INVALID_SIGNATURE),                          // -2141911285/0x8055070B
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT),                      // -2141911284/0x8055070C
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_UNKNOWN_TYPE),                               // -2141911283/0x8055070D
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_INVALID_ID),                                 // -2141911282/0x8055070E
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID),                          // -2141911281/0x8055070F
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_INVALID_TYPE),                               // -2141911279/0x80550711
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_TRANSACTION_ALREADY_END),                    // -2141911278/0x80550712
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_INVALID_PARTITION),                          // -2141911277/0x80550713
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT),                          // -2141911276/0x80550714
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_CLIENT_HANDLE_ALREADY_EXISTS),               // -2141911275/0x80550715
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_NO_RESOURCE),                                // -2141911274/0x80550716
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_REQUEST_BEFORE_END),                         // -2141911273/0x80550717
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID),                            // -2141911272/0x80550718
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_TOO_MANY_NPID),                              // -2141911271/0x80550719
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_SCORE_INVALID_SAVEDATA_OWNER),               // -2141911270/0x8055071A
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_ERROR_TUS_INVALID_SAVEDATA_OWNER),                 // -2141911269/0x8055071B
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_BAD_REQUEST),                         // -2141911039/0x80550801
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_TICKET),                      // -2141911038/0x80550802
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_SIGNATURE),                   // -2141911037/0x80550803
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_NPID),                        // -2141911035/0x80550805
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_FORBIDDEN),                           // -2141911034/0x80550806
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INTERNAL_SERVER_ERROR),               // -2141911033/0x80550807
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_VERSION_NOT_SUPPORTED),               // -2141911032/0x80550808
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_SERVICE_UNAVAILABLE),                 // -2141911031/0x80550809
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_PLAYER_BANNED),                       // -2141911030/0x8055080A
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_CENSORED),                            // -2141911029/0x8055080B
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_RECORD_FORBIDDEN),            // -2141911028/0x8055080C
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_USER_PROFILE_NOT_FOUND),              // -2141911027/0x8055080D
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_UPLOADER_DATA_NOT_FOUND),             // -2141911026/0x8055080E
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_QUOTA_MASTER_NOT_FOUND),              // -2141911025/0x8055080F
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_TITLE_NOT_FOUND),             // -2141911024/0x80550810
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_BLACKLISTED_USER_ID),                 // -2141911023/0x80550811
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_GAME_RANKING_NOT_FOUND),              // -2141911022/0x80550812
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_STORE_NOT_FOUND),             // -2141911020/0x80550814
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_NOT_BEST_SCORE),                      // -2141911019/0x80550815
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_LATEST_UPDATE_NOT_FOUND),             // -2141911018/0x80550816
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_BOARD_MASTER_NOT_FOUND),      // -2141911017/0x80550817
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_GAME_DATA_MASTER_NOT_FOUND),  // -2141911016/0x80550818
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_ANTICHEAT_DATA),              // -2141911015/0x80550819
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_TOO_LARGE_DATA),                      // -2141911014/0x8055081A
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_NO_SUCH_USER_NPID),                   // -2141911013/0x8055081B
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_ENVIRONMENT),                 // -2141911011/0x8055081D
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_ONLINE_NAME_CHARACTER),       // -2141911009/0x8055081F
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_ONLINE_NAME_LENGTH),          // -2141911008/0x80550820
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_ABOUT_ME_CHARACTER),          // -2141911007/0x80550821
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_ABOUT_ME_LENGTH),             // -2141911006/0x80550822
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_SCORE),                       // -2141911005/0x80550823
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_OVER_THE_RANKING_LIMIT),              // -2141911004/0x80550824
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_FAIL_TO_CREATE_SIGNATURE),            // -2141911002/0x80550826
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_MASTER_INFO_NOT_FOUND),       // -2141911001/0x80550827
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_OVER_THE_GAME_DATA_LIMIT),            // -2141911000/0x80550828
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_SELF_DATA_NOT_FOUND),                 // -2141910998/0x8055082A
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_USER_NOT_ASSIGNED),                   // -2141910997/0x8055082B
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_GAME_DATA_ALREADY_EXISTS),            // -2141910996/0x8055082C
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_TOO_MANY_RESULTS),                    // -2141910995/0x8055082D
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_NOT_RECORDABLE_VERSION),              // -2141910994/0x8055082E
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_USER_STORAGE_TITLE_MASTER_NOT_FOUND), // -2141910968/0x80550848
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_INVALID_VIRTUAL_USER),                // -2141910967/0x80550849
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_USER_STORAGE_DATA_NOT_FOUND),         // -2141910966/0x8055084A
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_NON_PLUS_MEMBER),                     // -2141910947/0x8055085D
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_UNMATCH_SEQUENCE),                    // -2141910946/0x8055085E
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_SAVEDATA_NOT_FOUND),                  // -2141910945/0x8055085F
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_TOO_MANY_SAVEDATA_FILES),             // -2141910944/0x80550860
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_TOO_MUCH_TOTAL_SAVEDATA_SIZE),        // -2141910943/0x80550861
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_NOT_YET_DOWNLOADABLE),                // -2141910942/0x80550862
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_BLACKLISTED_TITLE),                   // -2141910936/0x80550868
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_TOO_LARGE_ICONDATA),                  // -2141910935/0x80550869
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_RECORD_DATE_IS_NEWER_THAN_COMP_DATE), // -2141910930/0x8055086E
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_TOO_LARGE_SAVEDATA),                  // -2141910934/0x8055086A
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_UNMATCH_SIGNATURE),                   // -2141910933/0x8055086B
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_UNMATCH_MD5SUM),                      // -2141910932/0x8055086C
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_TOO_MUCH_SAVEDATA_SIZE),              // -2141910931/0x8055086D
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_CONDITIONS_NOT_SATISFIED),            // -2141910925/0x80550873
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_UNSUPPORTED_PLATFORM),                // -2141910920/0x80550878
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_MATCHING_BEFORE_SERVICE),             // -2141910880/0x805508A0
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_MATCHING_END_OF_SERVICE),             // -2141910879/0x805508A1
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_MATCHING_MAINTENANCE),                // -2141910878/0x805508A2
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_BEFORE_SERVICE),              // -2141910877/0x805508A3
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_END_OF_SERVICE),              // -2141910876/0x805508A4
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_RANKING_MAINTENANCE),                 // -2141910875/0x805508A5
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_NO_SUCH_TITLE),                       // -2141910874/0x805508A6
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_TITLE_USER_STORAGE_BEFORE_SERVICE),   // -2141910870/0x805508AA
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_TITLE_USER_STORAGE_END_OF_SERVICE),   // -2141910869/0x805508AB
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_TITLE_USER_STORAGE_MAINTENANCE),      // -2141910868/0x805508AC
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_FSR_BEFORE_SERVICE),                  // -2141910867/0x805508AD
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_FSR_END_OF_SERVICE),                  // -2141910866/0x805508AE
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_FSR_MAINTENANCE),                     // -2141910865/0x805508AF
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_UBS_BEFORE_SERVICE),                  // -2141910864/0x805508B0
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_UBS_END_OF_SERVICE),                  // -2141910863/0x805508B1
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_UBS_MAINTENANCE),                     // -2141910862/0x805508B2
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_BASIC_BLACKLISTED_USER_ID),           // -2141910861/0x805508B3
    DIRTYSOCK_ErrorName(SCE_NP_COMMUNITY_SERVER_ERROR_UNSPECIFIED),                         // -2141910785/0x805508FF
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_OUT_OF_MEMORY),                              // -2141910015/0x80550C01
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_ALREADY_INITIALIZED),                        // -2141910014/0x80550C02
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED),                            // -2141910013/0x80550C03
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_CONTEXT_MAX),                                // -2141910012/0x80550C04
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_CONTEXT_ALREADY_EXISTS),                     // -2141910011/0x80550C05
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_FOUND),                          // -2141910010/0x80550C06
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_CONTEXT_ALREADY_STARTED),                    // -2141910009/0x80550C07
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_STARTED),                        // -2141910008/0x80550C08
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_SERVER_NOT_FOUND),                           // -2141910007/0x80550C09
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_INVALID_ARGUMENT),                           // -2141910006/0x80550C0A
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID),                         // -2141910005/0x80550C0B
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_INVALID_SERVER_ID),                          // -2141910004/0x80550C0C
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_INVALID_WORLD_ID),                           // -2141910003/0x80550C0D
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_INVALID_LOBBY_ID),                           // -2141910002/0x80550C0E
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_INVALID_ROOM_ID),                            // -2141910001/0x80550C0F
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_INVALID_MEMBER_ID),                          // -2141910000/0x80550C10
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_INVALID_ATTRIBUTE_ID),                       // -2141909999/0x80550C11
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_INVALID_CASTTYPE),                           // -2141909998/0x80550C12
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_INVALID_SORT_METHOD),                        // -2141909997/0x80550C13
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_INVALID_MAX_SLOT),                           // -2141909996/0x80550C14
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_INVALID_MATCHING_SPACE),                     // -2141909994/0x80550C16
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_INVALID_BLOCK_KICK_FLAG),                    // -2141909993/0x80550C17
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_INVALID_MESSAGE_TARGET),                     // -2141909992/0x80550C18
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_RANGE_FILTER_MAX),                           // -2141909991/0x80550C19
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_INSUFFICIENT_BUFFER),                        // -2141909990/0x80550C1A
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_DESTINATION_DISAPPEARED),                    // -2141909989/0x80550C1B
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_REQUEST_TIMEOUT),                            // -2141909988/0x80550C1C
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_INVALID_ALIGNMENT),                          // -2141909987/0x80550C1D
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_CONNECTION_CLOSED_BY_SERVER),                // -2141909986/0x80550C1E
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_SSL_VERIFY_FAILED),                          // -2141909985/0x80550C1F
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_SSL_HANDSHAKE),                              // -2141909984/0x80550C20
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_SSL_SEND),                                   // -2141909983/0x80550C21
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_SSL_RECV),                                   // -2141909982/0x80550C22
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_JOINED_SESSION_MAX),                         // -2141909981/0x80550C23
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_ALREADY_JOINED),                             // -2141909980/0x80550C24
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_INVALID_SESSION_TYPE),                       // -2141909979/0x80550C25
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_NP_SIGNED_OUT),                              // -2141909978/0x80550C26
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_BUSY),                                       // -2141909977/0x80550C27
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_SERVER_NOT_AVAILABLE),                       // -2141909976/0x80550C28
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_NOT_ALLOWED),                                // -2141909975/0x80550C29
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_ABORTED),                                    // -2141909974/0x80550C2A
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_REQUEST_NOT_FOUND),                          // -2141909973/0x80550C2B
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_SESSION_DESTROYED),                          // -2141909972/0x80550C2C
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_CONTEXT_STOPPED),                            // -2141909971/0x80550C2D
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_INVALID_REQUEST_PARAMETER),                  // -2141909970/0x80550C2E
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_NOT_NP_SIGN_IN),                             // -2141909969/0x80550C2F
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_ROOM_NOT_FOUND),                             // -2141909968/0x80550C30
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_ROOM_MEMBER_NOT_FOUND),                      // -2141909967/0x80550C31
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_LOBBY_NOT_FOUND),                            // -2141909966/0x80550C32
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_LOBBY_MEMBER_NOT_FOUND),                     // -2141909965/0x80550C33
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_KEEPALIVE_TIMEOUT),                          // -2141909964/0x80550C34
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_TIMEOUT_TOO_SHORT),                          // -2141909963/0x80550C35
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_TIMEDOUT),                                   // -2141909962/0x80550C36
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_INVALID_SLOTGROUP),                          // -2141909961/0x80550C37
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_INVALID_ATTRIBUTE_SIZE),                     // -2141909960/0x80550C38
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_CANNOT_ABORT),                               // -2141909959/0x80550C39
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_ERROR_SESSION_NOT_FOUND),                          // -2141909958/0x80550C3A
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_BAD_REQUEST),                         // -2141909759/0x80550D01
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_SERVICE_UNAVAILABLE),                 // -2141909758/0x80550D02
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_BUSY),                                // -2141909757/0x80550D03
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_END_OF_SERVICE),                      // -2141909756/0x80550D04
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_INTERNAL_SERVER_ERROR),               // -2141909755/0x80550D05
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_PLAYER_BANNED),                       // -2141909754/0x80550D06
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_FORBIDDEN),                           // -2141909753/0x80550D07
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_BLOCKED),                             // -2141909752/0x80550D08
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_UNSUPPORTED_NP_ENV),                  // -2141909751/0x80550D09
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_INVALID_TICKET),                      // -2141909750/0x80550D0A
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_INVALID_SIGNATURE),                   // -2141909749/0x80550D0B
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_EXPIRED_TICKET),                      // -2141909748/0x80550D0C
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_ENTITLEMENT_REQUIRED),                // -2141909747/0x80550D0D
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_CONTEXT),                     // -2141909746/0x80550D0E
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_CLOSED),                              // -2141909745/0x80550D0F
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_TITLE),                       // -2141909744/0x80550D10
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_WORLD),                       // -2141909743/0x80550D11
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_LOBBY),                       // -2141909742/0x80550D12
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM),                        // -2141909741/0x80550D13
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_LOBBY_INSTANCE),              // -2141909740/0x80550D14
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM_INSTANCE),               // -2141909739/0x80550D15
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_PASSWORD_MISMATCH),                   // -2141909737/0x80550D17
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_LOBBY_FULL),                          // -2141909736/0x80550D18
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_ROOM_FULL),                           // -2141909735/0x80550D19
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_GROUP_FULL),                          // -2141909733/0x80550D1B
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_USER),                        // -2141909732/0x80550D1C
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_GROUP_PASSWORD_MISMATCH),             // -2141909731/0x80550D1D
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_TITLE_PASSPHRASE_MISMATCH),           // -2141909730/0x80550D1E
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_LOBBY_ALREADY_EXIST),                 // -2141909723/0x80550D25
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_ROOM_ALREADY_EXIST),                  // -2141909722/0x80550D26
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_CONSOLE_BANNED),                      // -2141909720/0x80550D28
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_NO_ROOMGROUP),                        // -2141909719/0x80550D29
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_NO_SUCH_GROUP),                       // -2141909718/0x80550D2A
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_NO_PASSWORD),                         // -2141909717/0x80550D2B
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_INVALID_GROUP_SLOT_NUM),              // -2141909716/0x80550D2C
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_INVALID_PASSWORD_SLOT_MASK),          // -2141909715/0x80550D2D
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_DUPLICATE_GROUP_LABEL),               // -2141909714/0x80550D2E
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_REQUEST_OVERFLOW),                    // -2141909713/0x80550D2F
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_ALREADY_JOINED),                      // -2141909712/0x80550D30
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_NAT_TYPE_MISMATCH),                   // -2141909711/0x80550D31
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_ROOM_INCONSISTENCY),                  // -2141909710/0x80550D32
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SERVER_ERROR_BLOCKED_USER_IN_ROOM),                // -2141909709/0x80550D33
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_NOT_INITIALIZED),                  // -2141909503/0x80550E01
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_ALREADY_INITIALIZED),              // -2141909502/0x80550E02
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_OUT_OF_MEMORY),                    // -2141909501/0x80550E03
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_CTXID_NOT_AVAILABLE),              // -2141909500/0x80550E04
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_CTX_NOT_FOUND),                    // -2141909499/0x80550E05
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_REQID_NOT_AVAILABLE),              // -2141909498/0x80550E06
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_REQ_NOT_FOUND),                    // -2141909497/0x80550E07
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_PARSER_CREATE_FAILED),             // -2141909496/0x80550E08
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_PARSER_FAILED),                    // -2141909495/0x80550E09
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_INVALID_NAMESPACE),                // -2141909494/0x80550E0A
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_NETINFO_NOT_AVAILABLE),            // -2141909493/0x80550E0B
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_PEER_NOT_RESPONDING),              // -2141909492/0x80550E0C
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_CONNID_NOT_AVAILABLE),             // -2141909491/0x80550E0D
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_CONN_NOT_FOUND),                   // -2141909490/0x80550E0E
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_PEER_UNREACHABLE),                 // -2141909489/0x80550E0F
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_TERMINATED_BY_PEER),               // -2141909488/0x80550E10
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_TIMEOUT),                          // -2141909487/0x80550E11
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_CTX_MAX),                          // -2141909486/0x80550E12
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_RESULT_NOT_FOUND),                 // -2141909485/0x80550E13
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_CONN_IN_PROGRESS),                 // -2141909484/0x80550E14
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_INVALID_ARGUMENT),                 // -2141909483/0x80550E15
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_OWN_NP_ID),                        // -2141909482/0x80550E16
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_TOO_MANY_CONN),                    // -2141909481/0x80550E17
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_TERMINATED_BY_MYSELF),             // -2141909480/0x80550E18
    DIRTYSOCK_ErrorName(SCE_NP_MATCHING2_SIGNALING_ERROR_MATCHING2_PEER_NOT_FOUND),         // -2141909479/0x80550E19
    DIRTYSOCK_ErrorName(SCE_NP_BANDWIDTH_TEST_ERROR_NOT_INITIALIZED),                       // -2141905150/0x80551F02
    DIRTYSOCK_ErrorName(SCE_NP_BANDWIDTH_TEST_ERROR_BAD_RESPONSE),                          // -2141905149/0x80551F03
    DIRTYSOCK_ErrorName(SCE_NP_BANDWIDTH_TEST_ERROR_OUT_OF_MEMORY),                         // -2141905148/0x80551F04
    DIRTYSOCK_ErrorName(SCE_NP_BANDWIDTH_TEST_ERROR_INVALID_ARGUMENT),                      // -2141905147/0x80551F05
    DIRTYSOCK_ErrorName(SCE_NP_BANDWIDTH_TEST_ERROR_INVALID_SIZE),                          // -2141905146/0x80551F06
    DIRTYSOCK_ErrorName(SCE_NP_BANDWIDTH_TEST_ERROR_CONTEXT_NOT_AVAILABLE),                 // -2141905145/0x80551F07

    #if ORBIS_SDK_VERSION >= 1020
    DIRTYSOCK_ErrorName(SCE_NP_BANDWIDTH_TEST_ERROR_CONTEXT_NOT_AVAILABLE),                 // -2141905145/0x80551F07
    DIRTYSOCK_ErrorName(SCE_NP_PARTY_ERROR_UNKNOWN),                                        // -2141903615/0x80552501
    DIRTYSOCK_ErrorName(SCE_NP_PARTY_ERROR_ALREADY_INITIALIZED),                            // -2141903614/0x80552502
    DIRTYSOCK_ErrorName(SCE_NP_PARTY_ERROR_NOT_INITIALIZED),                                // -2141903613/0x80552503
    DIRTYSOCK_ErrorName(SCE_NP_PARTY_ERROR_INVALID_ARGUMENT),                               // -2141903612/0x80552504
    DIRTYSOCK_ErrorName(SCE_NP_PARTY_ERROR_OUT_OF_MEMORY),                                  // -2141903611/0x80552505
    DIRTYSOCK_ErrorName(SCE_NP_PARTY_ERROR_NOT_IN_PARTY),                                   // -2141903610/0x80552506
    DIRTYSOCK_ErrorName(SCE_NP_PARTY_ERROR_VOICE_NOT_ENABLED),                              // -2141903609/0x80552507
    DIRTYSOCK_ErrorName(SCE_NP_PARTY_ERROR_MEMBER_NOT_FOUND),                               // -2141903608/0x80552508
    DIRTYSOCK_ErrorName(SCE_NP_PARTY_ERROR_SEND_BUSY),                                      // -2141903607/0x80552509
    DIRTYSOCK_ErrorName(SCE_NP_PARTY_ERROR_SEND_OUT_OF_CONTEXT),                            // -2141903600/0x80552510
    DIRTYSOCK_ErrorName(SCE_NP_PARTY_ERROR_INVALID_STATE),                                  // -2141903599/0x80552511
    DIRTYSOCK_ErrorName(SCE_NP_PARTY_ERROR_INVALID_LOCAL_PARTY_MEMBER),                     // -2141903598/0x80552512
    DIRTYSOCK_ErrorName(SCE_NP_PARTY_ERROR_INVALID_PROCESS_TYPE),                           // -2141903597/0x80552513

    DIRTYSOCK_ErrorName(SCE_NP_SNS_FACEBOOK_ERROR_INVALID_ARGUMENT),                        // -2141903358/0x80552602
    DIRTYSOCK_ErrorName(SCE_NP_SNS_FACEBOOK_ERROR_OUT_OF_MEMORY),                           // -2141903357/0x80552603
    DIRTYSOCK_ErrorName(SCE_NP_SNS_FACEBOOK_ERROR_EXCEEDS_MAX),                             // -2141903356/0x80552604
    DIRTYSOCK_ErrorName(SCE_NP_SNS_FACEBOOK_ERROR_UGM_RESTRICTION),                         // -2141903355/0x80552605
    DIRTYSOCK_ErrorName(SCE_NP_SNS_FACEBOOK_ERROR_ABORTED),                                 // -2141903354/0x80552606
    DIRTYSOCK_ErrorName(SCE_NP_SNS_FACEBOOK_ERROR_ACCOUNT_NOT_BOUND),                       // -2141903353/0x80552607
    DIRTYSOCK_ErrorName(SCE_NP_SNS_FACEBOOK_ERROR_CANCELED_BY_SYSTEM),                      // -2141903352/0x80552608
    #endif

    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_NOT_INITIALIZED),                            // -2141903103/0x80552701
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_ALREADY_INITIALIZED),                        // -2141903102/0x80552702
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_OUT_OF_MEMORY),                              // -2141903101/0x80552703
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_CTXID_NOT_AVAILABLE),                        // -2141903100/0x80552704
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_CTX_NOT_FOUND),                              // -2141903099/0x80552705
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_REQID_NOT_AVAILABLE),                        // -2141903098/0x80552706
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_REQ_NOT_FOUND),                              // -2141903097/0x80552707
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_PARSER_CREATE_FAILED),                       // -2141903096/0x80552708
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_PARSER_FAILED),                              // -2141903095/0x80552709
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_INVALID_NAMESPACE),                          // -2141903094/0x8055270A
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_NETINFO_NOT_AVAILABLE),                      // -2141903093/0x8055270B
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_PEER_NOT_RESPONDING),                        // -2141903092/0x8055270C
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_CONNID_NOT_AVAILABLE),                       // -2141903091/0x8055270D
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_CONN_NOT_FOUND),                             // -2141903090/0x8055270E
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_PEER_UNREACHABLE),                           // -2141903089/0x8055270F
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_TERMINATED_BY_PEER),                         // -2141903088/0x80552710
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_TIMEOUT),                                    // -2141903087/0x80552711
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_CTX_MAX),                                    // -2141903086/0x80552712
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_RESULT_NOT_FOUND),                           // -2141903085/0x80552713
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_CONN_IN_PROGRESS),                           // -2141903084/0x80552714
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_INVALID_ARGUMENT),                           // -2141903083/0x80552715
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_OWN_NP_ID),                                  // -2141903082/0x80552716
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_TOO_MANY_CONN),                              // -2141903081/0x80552717
    DIRTYSOCK_ErrorName(SCE_NP_SIGNALING_ERROR_TERMINATED_BY_MYSELF),                       // -2141903080/0x80552718
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_OUT_OF_MEMORY),                                 // -2141902591/0x80552901
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_INVALID_ARGUMENT),                              // -2141902590/0x80552902
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_INVALID_LIB_CONTEXT_ID),                        // -2141902589/0x80552903
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND),                         // -2141902588/0x80552904
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND),                        // -2141902587/0x80552905
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND),                             // -2141902586/0x80552906
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN),                                 // -2141902585/0x80552907
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_INVALID_CONTENT_PARAMETER),                     // -2141902584/0x80552908
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_ABORTED),                                       // -2141902583/0x80552909
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_USER_CONTEXT_ALREADY_EXIST),                    // -2141902582/0x8055290A

    #if ORBIS_SDK_VERSION >= 1020
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_PUSH_EVENT_FILTER_NOT_FOUND),                   // -2141902581/0x8055290B
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_PUSH_EVENT_CALLBACK_NOT_FOUND),                 // -2141902580/0x8055290C
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND),                              // -2141902579/0x8055290D
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_SERVICE_PUSH_EVENT_FILTER_NOT_FOUND),           // -2141902578/0x8055290E
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_SERVICE_PUSH_EVENT_CALLBACK_NOT_FOUND),         // -2141902577/0x8055290F
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_SIGNED_IN_USER_NOT_FOUND),                      // -2141902576/0x80552910
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_LIB_CONTEXT_BUSY),                              // -2141902575/0x80552911
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_USER_CONTEXT_BUSY),                             // -2141902574/0x80552912
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_REQUEST_BUSY),                                  // -2141902573/0x80552913
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_INVALID_HTTP_STATUS_CODE),                      // -2141902572/0x80552914
    DIRTYSOCK_ErrorName(SCE_NP_WEBAPI_ERROR_PROHIBITED_HTTP_HEADER),                        // -2141902571/0x80552915

    DIRTYSOCK_ErrorName(SCE_NP_AUTH_SERVER_ERROR_UNKNOWN),                                  // -2141902336/0x80552A00
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_SERVER_ERROR_INVALID_REQUEST),                          // -2141902335/0x80552A01
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_SERVER_ERROR_UNAUTHORIZED_CLIENT),                      // -2141902334/0x80552A02
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_SERVER_ERROR_ACCESS_DENIED),                            // -2141902333/0x80552A03
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_SERVER_ERROR_UNSUPPORTED_RESPONSE_TYPE),                // -2141902332/0x80552A04
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_SERVER_ERROR_SERVER_ERROR),                             // -2141902330/0x80552A06
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_SERVER_ERROR_TEMPORARILY_UNAVAILABLE),                  // -2141902329/0x80552A07
    DIRTYSOCK_ErrorName(SCE_NP_AUTH_SERVER_ERROR_INVALID_GRANT),                            // -2141902327/0x80552A09

    DIRTYSOCK_ErrorName(SCE_NP_IN_GAME_MESSAGE_ERROR_OUT_OF_MEMORY),                        // -2141902079/0x80552B01
    DIRTYSOCK_ErrorName(SCE_NP_IN_GAME_MESSAGE_ERROR_INVALID_ARGUMENT),                     // -2141902078/0x80552B02
    DIRTYSOCK_ErrorName(SCE_NP_IN_GAME_MESSAGE_ERROR_LIB_CONTEXT_NOT_FOUND),                // -2141902077/0x80552B03
    DIRTYSOCK_ErrorName(SCE_NP_IN_GAME_MESSAGE_ERROR_NOT_SIGNED_IN),                        // -2141902076/0x80552B04
    DIRTYSOCK_ErrorName(SCE_NP_IN_GAME_MESSAGE_ERROR_HANDLE_NOT_FOUND),                     // -2141902075/0x80552B05
    DIRTYSOCK_ErrorName(SCE_NP_IN_GAME_MESSAGE_ERROR_ABORTED),                              // -2141902074/0x80552B06
    DIRTYSOCK_ErrorName(SCE_NP_IN_GAME_MESSAGE_ERROR_SIGNED_IN_USER_NOT_FOUND),             // -2141902073/0x80552B07
    DIRTYSOCK_ErrorName(SCE_NP_IN_GAME_MESSAGE_ERROR_NOT_PREPARED),                         // -2141902072/0x80552B08
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_UNKNOWN),                                       // -2141907456/0x80551600
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_NOT_INITIALIZED),                               // -2141907455/0x80551601
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_ALREADY_INITIALIZED),                           // -2141907454/0x80551602
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_OUT_OF_MEMORY),                                 // -2141907453/0x80551603
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_INVALID_ARGUMENT),                              // -2141907452/0x80551604
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_INSUFFICIENT_BUFFER),                           // -2141907451/0x80551605
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_EXCEEDS_MAX),                                   // -2141907450/0x80551606
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_ABORT),                                         // -2141907449/0x80551607
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_INVALID_HANDLE),                                // -2141907448/0x80551608
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_INVALID_CONTEXT),                               // -2141907447/0x80551609
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_INVALID_TROPHY_ID),                             // -2141907446/0x8055160A
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_INVALID_GROUP_ID),                              // -2141907445/0x8055160B
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_TROPHY_ALREADY_UNLOCKED),                       // -2141907444/0x8055160C
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_PLATINUM_CANNOT_UNLOCK),                        // -2141907443/0x8055160D
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_ACCOUNTID_NOT_MATCH),                           // -2141907442/0x8055160E
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_NOT_REGISTERED),                                // -2141907441/0x8055160F
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_ALREADY_REGISTERED),                            // -2141907440/0x80551610
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_BROKEN_DATA),                                   // -2141907439/0x80551611
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_INSUFFICIENT_SPACE),                            // -2141907438/0x80551612
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_CONTEXT_ALREADY_EXISTS),                        // -2141907437/0x80551613
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_ICON_FILE_NOT_FOUND),                           // -2141907436/0x80551614
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_INVALID_TRP_FILE_FORMAT),                       // -2141907434/0x80551616
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_UNSUPPORTED_TRP_FILE),                          // -2141907433/0x80551617
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_INVALID_TROPHY_CONF_FORMAT),                    // -2141907432/0x80551618
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_UNSUPPORTED_TROPHY_CONF),                       // -2141907431/0x80551619
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_TROPHY_NOT_UNLOCKED),                           // -2141907430/0x8055161A
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_USER_NOT_FOUND),                                // -2141907428/0x8055161C
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_USER_NOT_LOGGED_IN),                            // -2141907427/0x8055161D
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_CONTEXT_USER_LOGOUT),                           // -2141907426/0x8055161E
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_USE_TRP_FOR_DEVELOPMENT),                       // -2141907425/0x8055161F
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_INVALID_NP_TITLE_ID),                           // -2141907424/0x80551620
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_INVALID_NP_SERVICE_LABEL),                      // -2141907423/0x80551621
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_NOT_SUPPORTED),                                 // -2141907422/0x80551622
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_CONTEXT_EXCEEDS_MAX),                           // -2141907421/0x80551623
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_HANDLE_EXCEEDS_MAX),                            // -2141907420/0x80551624
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_INVALID_USER_ID),                               // -2141907419/0x80551625
    #if ORBIS_SDK_VERSION >= 1700
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_TITLE_CONF_NOT_INSTALLED),                      // -2141907418/0x80551626
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_BROKEN_TITLE_CONF),                             // -2141907417/0x80551627
    DIRTYSOCK_ErrorName(SCE_NP_TROPHY_ERROR_INCONSISTENT_TITLE_CONF),                       // -2141907416/0x80551628
    #endif
    #endif // ORBIS_SDK_VERSION >= 1020

    /*
        errors from user_service_error.h
    */

    DIRTYSOCK_ErrorName(SCE_USER_SERVICE_ERROR_INTERNAL),                                   // -2137653247/0x80960001
    DIRTYSOCK_ErrorName(SCE_USER_SERVICE_ERROR_NOT_INITIALIZED),                            // -2137653246/0x80960002
    DIRTYSOCK_ErrorName(SCE_USER_SERVICE_ERROR_ALREADY_INITIALIZED),                        // -2137653245/0x80960003
    DIRTYSOCK_ErrorName(SCE_USER_SERVICE_ERROR_NO_MEMORY),                                  // -2137653244/0x80960004
    DIRTYSOCK_ErrorName(SCE_USER_SERVICE_ERROR_INVALID_ARGUMENT),                           // -2137653243/0x80960005
    DIRTYSOCK_ErrorName(SCE_USER_SERVICE_ERROR_OPERATION_NOT_SUPPORTED),                    // -2137653242/0x80960006
    DIRTYSOCK_ErrorName(SCE_USER_SERVICE_ERROR_NO_EVENT),                                   // -2137653241/0x80960007
    DIRTYSOCK_ErrorName(SCE_USER_SERVICE_ERROR_NOT_LOGGED_IN),                              // -2137653239/0x80960009
    DIRTYSOCK_ErrorName(SCE_USER_SERVICE_ERROR_BUFFER_TOO_SHORT),                           // -2137653238/0x8096000A

    /*
        errors from voice_types.h
    */

    DIRTYSOCK_ErrorName(SCE_VOICE_ERROR_LIBVOICE_NOT_INIT),                                 // -2142369791/0x804E0801
    DIRTYSOCK_ErrorName(SCE_VOICE_ERROR_LIBVOICE_INITIALIZED),                              // -2142369790/0x804E0802
    DIRTYSOCK_ErrorName(SCE_VOICE_ERROR_GENERAL),                                           // -2142369789/0x804E0803
    DIRTYSOCK_ErrorName(SCE_VOICE_ERROR_PORT_INVALID),                                      // -2142369788/0x804E0804
    DIRTYSOCK_ErrorName(SCE_VOICE_ERROR_ARGUMENT_INVALID),                                  // -2142369787/0x804E0805
    DIRTYSOCK_ErrorName(SCE_VOICE_ERROR_CONTAINER_INVALID),                                 // -2142369786/0x804E0806
    DIRTYSOCK_ErrorName(SCE_VOICE_ERROR_TOPOLOGY),                                          // -2142369785/0x804E0807
    DIRTYSOCK_ErrorName(SCE_VOICE_ERROR_RESOURCE_INSUFFICIENT),                             // -2142369784/0x804E0808
    DIRTYSOCK_ErrorName(SCE_VOICE_ERROR_SERVICE_DETACHED),                                  // -2142369782/0x804E080A
    DIRTYSOCK_ErrorName(SCE_VOICE_ERROR_SERVICE_ATTACHED),                                  // -2142369781/0x804E080B

    /*
        errors from common_dialog\error.h
    */
    DIRTYSOCK_ErrorName(SCE_COMMON_DIALOG_ERROR_NOT_SYSTEM_INITIALIZED),                    // -2135425023/0x80B80001
    DIRTYSOCK_ErrorName(SCE_COMMON_DIALOG_ERROR_ALREADY_SYSTEM_INITIALIZED),                // -2135425022/0x80B80002
    DIRTYSOCK_ErrorName(SCE_COMMON_DIALOG_ERROR_NOT_INITIALIZED),                           // -2135425021/0x80B80003
    DIRTYSOCK_ErrorName(SCE_COMMON_DIALOG_ERROR_ALREADY_INITIALIZED),                       // -2135425020/0x80B80004
    DIRTYSOCK_ErrorName(SCE_COMMON_DIALOG_ERROR_NOT_FINISHED),                              // -2135425019/0x80B80005
    DIRTYSOCK_ErrorName(SCE_COMMON_DIALOG_ERROR_INVALID_STATE),                             // -2135425018/0x80B80006
    DIRTYSOCK_ErrorName(SCE_COMMON_DIALOG_ERROR_RESULT_NONE),                               // -2135425017/0x80B80007
    DIRTYSOCK_ErrorName(SCE_COMMON_DIALOG_ERROR_BUSY),                                      // -2135425016/0x80B80008
    DIRTYSOCK_ErrorName(SCE_COMMON_DIALOG_ERROR_OUT_OF_MEMORY),                             // -2135425015/0x80B80009
    DIRTYSOCK_ErrorName(SCE_COMMON_DIALOG_ERROR_PARAM_INVALID),                             // -2135425014/0x80B8000A
    DIRTYSOCK_ErrorName(SCE_COMMON_DIALOG_ERROR_NOT_RUNNING),                               // -2135425013/0x80B8000B
    DIRTYSOCK_ErrorName(SCE_COMMON_DIALOG_ERROR_ALREADY_CLOSE),                             // -2135425012/0x80B8000C
    DIRTYSOCK_ErrorName(SCE_COMMON_DIALOG_ERROR_ARG_NULL),                                  // -2135425011/0x80B8000D
    DIRTYSOCK_ErrorName(SCE_COMMON_DIALOG_ERROR_UNEXPECTED_FATAL),                          // -2135425010/0x80B8000E
    DIRTYSOCK_ErrorName(SCE_COMMON_DIALOG_ERROR_NOT_SUPPORTED),                             // -2135425009/0x80B8000F

    // NULL terminate
    DIRTYSOCK_ListEnd()
};
#endif

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/
/*F********************************************************************************/
/*!
    \Function DirtyErrAppCallbackSet

    \Description
        This function set the callback used to report Sony error code back to
        Application layer to statisfy TRC4034

    \Input pCallback  - callback function pointer

    \Version 08/07/2013 (tcho) First Version
*/
/********************************************************************************F*/
void DirtyErrAppCallbackSet(const DirtySockAppErrorCallback pCallback)
{
    pAppErrorCallback = pCallback;
}

/*F********************************************************************************/
/*!
    \Function DirtyErrAppReport

    \Description
        This function invokes the callback used to report Sony error code back to
        Application layer to statisfy TRC4034

    \Input iError - Error code to be pass back to the application layer

    \Version 08/07/2013 (tcho) First Version
*/
/********************************************************************************F*/
void DirtyErrAppReport(int32_t iError)
{
    if (pAppErrorCallback != NULL)
    {
        (*pAppErrorCallback)(iError);
    }
}

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
            ds_snzprintf(pBuffer, iBufSize, "%s/%d", pList[iErr].pErrorName, (signed)uError);
            return;
        }
    }
    #endif

    ds_snzprintf(pBuffer, iBufSize, "%d", uError);
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
            ds_snzprintf(pBuffer, iBufSize, "%s/0x%08x", _DirtyErr_List[iErr].pErrorName, uError);
            return;
        }
    }
    #endif

    ds_snzprintf(pBuffer, iBufSize, "0x%08x", uError);
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
