/*H********************************************************************************/
/*!
    \File dirtyerrxenon.c

    \Description
        Dirtysock debug error routines.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 06/13/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <xtl.h>
#include <xonline.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyerr.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

#if DIRTYSOCK_ERRORNAMES

static DirtyErrT _DirtyErr_List[] =
{
    DIRTYSOCK_ErrorName(ERROR_INVALID_FUNCTION),                            // 1
    DIRTYSOCK_ErrorName(ERROR_ACCESS_DENIED),                               // 5
    DIRTYSOCK_ErrorName(ERROR_NO_MORE_FILES),                               // 18
    DIRTYSOCK_ErrorName(ERROR_INVALID_PARAMETER),                           // 87
    DIRTYSOCK_ErrorName(ERROR_INSUFFICIENT_BUFFER),                         // 122
    DIRTYSOCK_ErrorName(ERROR_IO_INCOMPLETE),                               // 996
    DIRTYSOCK_ErrorName(ERROR_IO_PENDING),                                  // 997
    DIRTYSOCK_ErrorName(ERROR_NOT_FOUND),                                   // 1168
    DIRTYSOCK_ErrorName(ERROR_CONNECTION_INVALID),                          // 1229
    DIRTYSOCK_ErrorName(ERROR_SERVICE_NOT_FOUND),                           // 1243
    DIRTYSOCK_ErrorName(ERROR_FUNCTION_FAILED),                             // 1627

    /*
        WinSock2.h
    */
    DIRTYSOCK_ErrorName(WSAEINTR),              // 10004L - A blocking operation was interrupted by a call to WSACancelBlockingCall.
    DIRTYSOCK_ErrorName(WSAEBADF),              // 10009L - The file handle supplied is not valid.
    DIRTYSOCK_ErrorName(WSAEACCES),             // 10013L - An attempt was made to access a socket in a way forbidden by its access permissions.
    DIRTYSOCK_ErrorName(WSAEFAULT),             // 10014L - The system detected an invalid pointer address in attempting to use a pointer argument in a call.
    DIRTYSOCK_ErrorName(WSAEINVAL),             // 10022L - An invalid argument was supplied.
    DIRTYSOCK_ErrorName(WSAEMFILE),             // 10024L - Too many open sockets.
    DIRTYSOCK_ErrorName(WSAEWOULDBLOCK),        // 10035L - A non-blocking socket operation could not be completed immediately.
    DIRTYSOCK_ErrorName(WSAEINPROGRESS),        // 10036L - A blocking operation is currently executing.
    DIRTYSOCK_ErrorName(WSAEALREADY),           // 10037L - An operation was attempted on a non-blocking socket that already had an operation in progress.
    DIRTYSOCK_ErrorName(WSAENOTSOCK),           // 10038L - An operation was attempted on something that is not a socket.
    DIRTYSOCK_ErrorName(WSAEDESTADDRREQ),       // 10039L - A required address was omitted from an operation on a socket.
    DIRTYSOCK_ErrorName(WSAEMSGSIZE),           // 10040L - A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram into was smaller than the datagram itself.
    DIRTYSOCK_ErrorName(WSAEPROTOTYPE),         // 10041L - A protocol was specified in the socket function call that does not support the semantics of the socket type requested.
    DIRTYSOCK_ErrorName(WSAENOPROTOOPT),        // 10042L - An unknown, invalid, or unsupported option or level was specified in a getsockopt or setsockopt call.
    DIRTYSOCK_ErrorName(WSAEPROTONOSUPPORT),    // 10043L - The requested protocol has not been configured into the system, or no implementation for it exists.
    DIRTYSOCK_ErrorName(WSAESOCKTNOSUPPORT),    // 10044L - The support for the specified socket type does not exist in this address family.
    DIRTYSOCK_ErrorName(WSAEOPNOTSUPP),         // 10045L - The attempted operation is not supported for the type of object referenced.
    DIRTYSOCK_ErrorName(WSAEPFNOSUPPORT),       // 10046L - The protocol family has not been configured into the system or no implementation for it exists.
    DIRTYSOCK_ErrorName(WSAEAFNOSUPPORT),       // 10047L - An address incompatible with the requested protocol was used.
    DIRTYSOCK_ErrorName(WSAEADDRINUSE),         // 10048L - Only one usage of each socket address (protocol/network address/port) is normally permitted.
    DIRTYSOCK_ErrorName(WSAEADDRNOTAVAIL),      // 10049L - The requested address is not valid in its context.
    DIRTYSOCK_ErrorName(WSAENETDOWN),           // 10050L - A socket operation encountered a dead network.
    DIRTYSOCK_ErrorName(WSAENETUNREACH),        // 10051L - A socket operation was attempted to an unreachable network.
    DIRTYSOCK_ErrorName(WSAENETRESET),          // 10052L - The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress.
    DIRTYSOCK_ErrorName(WSAECONNABORTED),       // 10053L - An established connection was aborted by the software in your host machine.
    DIRTYSOCK_ErrorName(WSAECONNRESET),         // 10054L - An existing connection was forcibly closed by the remote host.
    DIRTYSOCK_ErrorName(WSAENOBUFS),            // 10055L - An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full.
    DIRTYSOCK_ErrorName(WSAEISCONN),            // 10056L - A connect request was made on an already connected socket.
    DIRTYSOCK_ErrorName(WSAENOTCONN),           // 10057L - A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using a sendto call) no address was supplied.
    DIRTYSOCK_ErrorName(WSAESHUTDOWN),          // 10058L - A request to send or receive data was disallowed because the socket had already been shut down in that direction with a previous shutdown call.
    DIRTYSOCK_ErrorName(WSAETOOMANYREFS),       // 10059L - Too many references to some kernel object.
    DIRTYSOCK_ErrorName(WSAETIMEDOUT),          // 10060L - A connection attempt failed because the connected party did not properly respond after a period of time, or established connection failed because connected host has failed to respond.
    DIRTYSOCK_ErrorName(WSAECONNREFUSED),       // 10061L - No connection could be made because the target machine actively refused it.
    DIRTYSOCK_ErrorName(WSAELOOP),              // 10062L - Cannot translate name.
    DIRTYSOCK_ErrorName(WSAENAMETOOLONG),       // 10063L - Name component or name was too long.
    DIRTYSOCK_ErrorName(WSAEHOSTDOWN),          // 10064L - A socket operation failed because the destination host was down.
    DIRTYSOCK_ErrorName(WSAEHOSTUNREACH),       // 10065L - A socket operation was attempted to an unreachable host.
    DIRTYSOCK_ErrorName(WSAENOTEMPTY),          // 10066L - Cannot remove a directory that is not empty.
    DIRTYSOCK_ErrorName(WSAEPROCLIM),           // 10067L - A Windows Sockets implementation may have a limit on the number of applications that may use it simultaneously.
    DIRTYSOCK_ErrorName(WSAEUSERS),             // 10068L - Ran out of quota.
    DIRTYSOCK_ErrorName(WSAEDQUOT),             // 10069L - Ran out of disk quota.
    DIRTYSOCK_ErrorName(WSAESTALE),             // 10070L - File handle reference is no longer available.
    DIRTYSOCK_ErrorName(WSAEREMOTE),            // 10071L - Item is not available locally.
    DIRTYSOCK_ErrorName(WSASYSNOTREADY),        // 10091L - WSAStartup cannot function at this time because the underlying system it uses to provide network services is currently unavailable.
    DIRTYSOCK_ErrorName(WSAVERNOTSUPPORTED),    // 10092L - The Windows Sockets version requested is not supported.
    DIRTYSOCK_ErrorName(WSANOTINITIALISED),     // 10093L - Either the application has not called WSAStartup, or WSAStartup failed.
    DIRTYSOCK_ErrorName(WSAEDISCON),            // 10101L - Returned by WSARecv or WSARecvFrom to indicate the remote party has initiated a graceful shutdown sequence.
    DIRTYSOCK_ErrorName(WSAENOMORE),            // 10102L - No more results can be returned by WSALookupServiceNext.
    DIRTYSOCK_ErrorName(WSAECANCELLED),         // 10103L - A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.
    DIRTYSOCK_ErrorName(WSAEINVALIDPROCTABLE),  // 10104L - The procedure call table is invalid.
    DIRTYSOCK_ErrorName(WSAEINVALIDPROVIDER),   // 10105L - The requested service provider is invalid.
    DIRTYSOCK_ErrorName(WSAEPROVIDERFAILEDINIT),// 10106L - The requested service provider could not be loaded or initialized.
    DIRTYSOCK_ErrorName(WSASYSCALLFAILURE),     // 10107L - A system call that should never fail has failed.
    DIRTYSOCK_ErrorName(WSASERVICE_NOT_FOUND),  // 10108L - No such service is known. The service cannot be found in the specified name space.
    DIRTYSOCK_ErrorName(WSATYPE_NOT_FOUND),     // 10109L - The specified class was not found.
    DIRTYSOCK_ErrorName(WSA_E_NO_MORE),         // 10110L - No more results can be returned by WSALookupServiceNext.
    DIRTYSOCK_ErrorName(WSA_E_CANCELLED),       // 10111L - A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.
    DIRTYSOCK_ErrorName(WSAEREFUSED),           // 10112L - A database query failed because it was actively refused.

    // Generic errors = 0x80004xxx
    DIRTYSOCK_ErrorName(E_ABORT),                                           // 0x80004004
    DIRTYSOCK_ErrorName(E_FAIL),                                            // 0x80004005

    DIRTYSOCK_ErrorName(E_ACCESSDENIED),                                    // 0x80070005

    // Generic Errors = 0x80150XXX
    DIRTYSOCK_ErrorName(XONLINE_E_OVERFLOW),                                // 0x80150001
    DIRTYSOCK_ErrorName(XONLINE_E_NO_SESSION),                              // 0x80150002
    DIRTYSOCK_ErrorName(XONLINE_E_USER_NOT_LOGGED_ON),                      // 0x80150003
    DIRTYSOCK_ErrorName(XONLINE_E_NO_GUEST_ACCESS),                         // 0x80150004
    DIRTYSOCK_ErrorName(XONLINE_E_NOT_INITIALIZED),                         // 0x80150005
    DIRTYSOCK_ErrorName(XONLINE_E_NO_USER),                                 // 0x80150006
    DIRTYSOCK_ErrorName(XONLINE_E_INTERNAL_ERROR),                          // 0x80150007
    DIRTYSOCK_ErrorName(XONLINE_E_OUT_OF_MEMORY),                           // 0x80150008
    DIRTYSOCK_ErrorName(XONLINE_E_TASK_BUSY),                               // 0x80150009
    DIRTYSOCK_ErrorName(XONLINE_E_SERVER_ERROR),                            // 0x8015000A
    DIRTYSOCK_ErrorName(XONLINE_E_IO_ERROR),                                // 0x8015000B
    DIRTYSOCK_ErrorName(XONLINE_E_USER_NOT_PRESENT),                        // 0x8015000D
    DIRTYSOCK_ErrorName(XONLINE_E_INVALID_REQUEST),                         // 0x80150010
    DIRTYSOCK_ErrorName(XONLINE_E_TASK_THROTTLED),                          // 0x80150011
    DIRTYSOCK_ErrorName(XONLINE_E_TASK_ABORTED_BY_DUPLICATE),               // 0x80150012
    DIRTYSOCK_ErrorName(XONLINE_E_INVALID_TITLE_ID),                        // 0x80150013
    DIRTYSOCK_ErrorName(XONLINE_E_SERVER_CONFIG_ERROR),                     // 0x80150014
    DIRTYSOCK_ErrorName(XONLINE_E_END_OF_STREAM),                           // 0x80150015

    // Failures from online logon = 0x80151XXX
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_NO_NETWORK_CONNECTION),             // 0x80151000
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_CANNOT_ACCESS_SERVICE),             // 0x80151001
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_UPDATE_REQUIRED),                   // 0x80151002
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_SERVERS_TOO_BUSY),                  // 0x80151003
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_CONNECTION_LOST),                   // 0x80151004
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_KICKED_BY_DUPLICATE_LOGON),         // 0x80151005
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_INVALID_USER),                      // 0x80151006
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_FLASH_UPDATE_REQUIRED),             // 0x80151007 - XOnlineLogon task successful return states
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_SG_CONNECTION_TIMEDOUT),            // 0x8015100C
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_SERVICE_NOT_REQUESTED),             // 0x80151100
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_SERVICE_NOT_AUTHORIZED),            // 0x80151101
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_SERVICE_TEMPORARILY_UNAVAILABLE),   // 0x80151102
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_PPLOGIN_FAILED),                    // 0x80151103
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_SPONSOR_TOKEN_INVALID),             // 0x80151104
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_SPONSOR_TOKEN_BANNED),              // 0x80151105
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_SPONSOR_TOKEN_USAGE_EXCEEDED),      // 0x80151106
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_FLASH_UPDATE_NOT_DOWNLOADED),       // 0x80151107
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_UPDATE_NOT_DOWNLOADED),             // 0x80151108
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_NOT_LOGGED_ON),                     // 0x80151802
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_DNS_LOOKUP_FAILED),                 // 0x80151903
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_DNS_LOOKUP_TIMEDOUT),               // 0x80151904
    DIRTYSOCK_ErrorName(XONLINE_E_LOGON_AUTHENTICATION_TIMEDOUT),           // 0x80151909

    // Online logon success states
    DIRTYSOCK_ErrorName(XONLINE_S_LOGON_CONNECTION_ESTABLISHED),            // 0x001510F0
    DIRTYSOCK_ErrorName(XONLINE_S_LOGON_DISCONNECTED),                      // 0x001510F1

    //  Errors returned by matchmaking = 0x801551XX
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_INVALID_SESSION_ID),                // 0x80155100 - specified session id does not exist
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_INVALID_TITLE_ID),                  // 0x80155101 - specified title id is zero, or does not exist
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_INVALID_DATA_TYPE),                 // 0x80155102 - attribute ID or parameter type specifies an invalid data type
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_REQUEST_TOO_SMALL),                 // 0x80155103 - the request did not meet the minimum length for a valid request
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_REQUEST_TRUNCATED),                 // 0x80155104 - the self described length is greater than the actual buffer size
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_INVALID_SEARCH_REQ),                // 0x80155105 - the search request was invalid
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_INVALID_OFFSET),                    // 0x80155106 - one of the attribute/parameter offsets in the request was invalid.  Will be followed by the zero based offset number.
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_INVALID_ATTR_TYPE),                 // 0x80155107 - the attribute type was something other than user or session
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_INVALID_VERSION),                   // 0x80155108 - bad protocol version in request
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_OVERFLOW),                          // 0x80155109 - an attribute or parameter flowed past the end of the request
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_INVALID_RESULT_COL),                // 0x8015510A - referenced stored procedure returned a column with an unsupported data type
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_INVALID_STRING),                    // 0x8015510B - string with length-prefix of zero, or string with no terminating null
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_STRING_TOO_LONG),                   // 0x8015510C - string exceeded 400 characters
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_BLOB_TOO_LONG),                     // 0x8015510D - blob exceeded 800 bytes
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_INVALID_ATTRIBUTE_ID),              // 0x80155110 - attribute id is invalid
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_SESSION_ALREADY_EXISTS),            // 0x80155112 - session id already exists in the db
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_CRITICAL_DB_ERR),                   // 0x80155115 - critical error in db
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_NOT_ENOUGH_COLUMNS),                // 0x80155116 - search result set had too few columns
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_PERMISSION_DENIED),                 // 0x80155117 - incorrect permissions set on search sp
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_INVALID_PART_SCHEME),               // 0x80155118 - title specified an invalid partitioning scheme
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_INVALID_PARAM),                     // 0x80155119 - bad parameter passed to sp
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_DATA_TYPE_MISMATCH),                // 0x8015511D - data type specified in attr id did not match type of attr being set
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_SERVER_ERROR),                      // 0x8015511E - error on server not correctable by client
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_NO_USERS),                          // 0x8015511F - no authenticated users in search request.
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_INVALID_BLOB),                      // 0x80155120 - invalid blob attribute
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_TOO_MANY_USERS),                    // 0x80155121 - too many users in search request
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_INVALID_FLAGS),                     // 0x80155122 - invalid flags were specified in a search request
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_PARAM_MISSING),                     // 0x80155123 - required parameter not passed to sp
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_TOO_MANY_PARAM),                    // 0x80155124 - too many paramters passed to sp or in request structure
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_DUPLICATE_PARAM),                   // 0x80155125 - a paramter was passed to twice to a search procedure
    DIRTYSOCK_ErrorName(XONLINE_E_MATCH_TOO_MANY_ATTR),                     // 0x80155126 - too many attributes in the request structure

    //  Errors returned by session APIs = 0x801552XX
    DIRTYSOCK_ErrorName(XONLINE_E_SESSION_NOT_FOUND),                       // 0x80155200 - specified session id does not exist
    DIRTYSOCK_ErrorName(XONLINE_E_SESSION_INSUFFICIENT_PRIVILEGES),         // 0x80155201 - The requester does not have permissions to perform this operation
    DIRTYSOCK_ErrorName(XONLINE_E_SESSION_FULL),                            // 0x80155202 - The session is full, and the join operation failed, or joining is disallowe
    DIRTYSOCK_ErrorName(XONLINE_E_SESSION_INVITES_DISABLED),                // 0x80155203 - Invites have been disabled for this session
    DIRTYSOCK_ErrorName(XONLINE_E_SESSION_INVALID_FLAGS),                   // 0x80155204 - Invalid flags passed to XSessionCreate
    DIRTYSOCK_ErrorName(XONLINE_E_SESSION_REQUIRES_ARBITRATION),            // 0x80155205 - The Session owner has the GAME_TYPE context set to ranked, but the session is not arbitrated
    DIRTYSOCK_ErrorName(XONLINE_E_SESSION_WRONG_STATE),                     // 0x80155206 - The session is in the wrong state for the requested operation to be performed
    DIRTYSOCK_ErrorName(XONLINE_E_SESSION_INSUFFICIENT_BUFFER),             // 0x80155207 - Ran out of memory attempting to process search results
    DIRTYSOCK_ErrorName(XONLINE_E_SESSION_REGISTRATION_ERROR),              // 0x80155208 - Could not perform arbitration registration because some logged on users have not joined the session
    DIRTYSOCK_ErrorName(XONLINE_E_SESSION_NOT_LOGGED_ON),                   // 0x80155209 - User is not logged on to Live but attempted to create a session using Live features.
    DIRTYSOCK_ErrorName(XONLINE_E_SESSION_JOIN_ILLEGAL),                    // 0x8015520A - User attempted to join a USES_PRESENCE session when the user is already in a USES_PRESENCE session on the console. Can only have 1 at a time.
    DIRTYSOCK_ErrorName(XONLINE_E_SESSION_CREATE_KEY_FAILED),               // 0x8015520B - Key creation failed during session creation.
    DIRTYSOCK_ErrorName(XONLINE_E_SESSION_NOT_REGISTERED),                  // 0x8015520C - Can not start the session if registration has not completed
    DIRTYSOCK_ErrorName(XONLINE_E_SESSION_REGISTER_KEY_FAILED),             // 0x8015520D - Key registration failed during session creation.
    DIRTYSOCK_ErrorName(XONLINE_E_SESSION_UNREGISTER_KEY_FAILED),           // 0x8015520E - Key registration failed during session creation.

    // Errors returned by Query service = 0x801561XX
    DIRTYSOCK_ErrorName(XONLINE_E_QUERY_QUOTA_FULL),                        // 0x80156101 - this user or team's quota for the dataset is full.  you must remove an entity first.
    DIRTYSOCK_ErrorName(XONLINE_E_QUERY_ENTITY_NOT_FOUND),                  // 0x80156102 - the requested entity didn't exist in the provided dataset.
    DIRTYSOCK_ErrorName(XONLINE_E_QUERY_PERMISSION_DENIED),                 // 0x80156103 - the user tried to update or delete an entity that he didn't own.
    DIRTYSOCK_ErrorName(XONLINE_E_QUERY_ATTRIBUTE_TOO_LONG),                // 0x80156104 - attribute passed exceeds schema definition
    DIRTYSOCK_ErrorName(XONLINE_E_QUERY_UNEXPECTED_ATTRIBUTE),              // 0x80156105 - attribute passed was a bad param for the database operation
    DIRTYSOCK_ErrorName(XONLINE_E_QUERY_INVALID_ACTION),                    // 0x80156107 - the specified action (or dataset) doesn't have a select action associated with it.
    DIRTYSOCK_ErrorName(XONLINE_E_QUERY_SPEC_COUNT_MISMATCH),               // 0x80156108 - the provided number of QUERY_ATTRIBUTE_SPECs doesn't match the number returned by the procedure
    DIRTYSOCK_ErrorName(XONLINE_E_QUERY_DATASET_NOT_FOUND),                 // 0x80156109 - The specified dataset id was not found.
    DIRTYSOCK_ErrorName(XONLINE_E_QUERY_PROCEDURE_NOT_FOUND),               // 0x8015610A - The specified proc index was not found.

    // Errors returned by Competitions service = 0x801562XX
    DIRTYSOCK_ErrorName(XONLINE_E_COMP_ACCESS_DENIED),                      // 0x80156202 - The specified source (client) is not permitted to execute this method
    DIRTYSOCK_ErrorName(XONLINE_E_COMP_REGISTRATION_CLOSED),                // 0x80156203 - The competition is closed to registration
    DIRTYSOCK_ErrorName(XONLINE_E_COMP_FULL),                               // 0x80156204 - The competition has reached it's max enrollment
    DIRTYSOCK_ErrorName(XONLINE_E_COMP_NOT_REGISTERED),                     // 0x80156205 - The user or team isn't registered for the competition
    DIRTYSOCK_ErrorName(XONLINE_E_COMP_CANCELLED),                          // 0x80156206 - The competition has been cancelled, and the operation is invalid.
    DIRTYSOCK_ErrorName(XONLINE_E_COMP_CHECKIN_TIME_INVALID),               // 0x80156207 - The user is attempting to checkin to an event outside the allowed time.
    DIRTYSOCK_ErrorName(XONLINE_E_COMP_CHECKIN_BAD_EVENT),                  // 0x80156208 - The user is attempting to checkin to an event in which they are not a valid participant.
    DIRTYSOCK_ErrorName(XONLINE_E_COMP_EVENT_SCORED),                       // 0x80156209 - The user is attempting to checkin to an event which has already been scored by the service (user has forfeited or been ejected)
    DIRTYSOCK_ErrorName(XONLINE_S_COMP_EVENT_SCORED),                       // 0x00156209 - The user is attempting to checkin to an event but the users event has been updated. Re-query for a new event
    DIRTYSOCK_ErrorName(XONLINE_E_COMP_UNEXPECTED),                         // 0x80156210 - Results from the Database are unexpected or inconsistent with the current operation.
    DIRTYSOCK_ErrorName(XONLINE_E_COMP_TOPOLOGY_ERROR),                     // 0x80156216 - The topology request cannot be fulfilled by the server
    DIRTYSOCK_ErrorName(XONLINE_E_COMP_TOPOLOGY_PENDING),                   // 0x80156217 - The topology request has not completed yet
    DIRTYSOCK_ErrorName(XONLINE_E_COMP_CHECKIN_TOO_EARLY),                  // 0x80156218 - The user is attempting to checkin to an event before the allowed time.
    DIRTYSOCK_ErrorName(XONLINE_E_COMP_ALREADY_REGISTERED),                 // 0x80156219 - The user has already registered for this competition.
    DIRTYSOCK_ErrorName(XONLINE_E_COMP_INVALID_ENTRANT_TYPE),               // 0x8015621A - dwTeamId was non-0 for a user competition, or dwTeamId was 0 for a team competition
    DIRTYSOCK_ErrorName(XONLINE_E_COMP_TOO_LATE),                           // 0x8015621B - The time alloted for performing the requested action has already passed.
    DIRTYSOCK_ErrorName(XONLINE_E_COMP_TOO_EARLY),                          // 0x8015621C - The specified action cannot yet be peformed .
    DIRTYSOCK_ErrorName(XONLINE_E_COMP_NO_BYES_AVAILABLE),                  // 0x8015621D - No byes remain to be granted
    DIRTYSOCK_ErrorName(XONLINE_E_COMP_SERVICE_OUTAGE),                     // 0x8015621E - A service outage has occured, try again in a bit

    // Errors returned by the v1 Message Service = 0x801570XX
    DIRTYSOCK_ErrorName(XONLINE_E_MSGSVR_INVALID_REQUEST),                  // 0x80157001 - an invalid request type was received

    // Errors returned by the String Service = 0x801571XX
    DIRTYSOCK_ErrorName(XONLINE_E_STRING_TOO_LONG),                         // 0x80157101 - the string was longer than the allowed maximum
    DIRTYSOCK_ErrorName(XONLINE_E_STRING_OFFENSIVE_TEXT),                   // 0x80157102 - the string contains offensive text
    DIRTYSOCK_ErrorName(XONLINE_E_STRING_NO_DEFAULT_STRING),                // 0x80157103 - returned by AddString when no string of the language specified as the default is found
    DIRTYSOCK_ErrorName(XONLINE_E_STRING_INVALID_LANGUAGE),                 // 0x80157104 - returned by AddString when an invalid language is specified for a string

    // Errors returned by the Feedback Service = 0x801580XX
    DIRTYSOCK_ErrorName(XONLINE_E_FEEDBACK_NULL_TARGET),                    // 0x80158001 - target PUID of feedback is NULL
    DIRTYSOCK_ErrorName(XONLINE_E_FEEDBACK_BAD_TYPE),                       // 0x80158002 - bad feedback type
    DIRTYSOCK_ErrorName(XONLINE_E_FEEDBACK_CANNOT_LOG),                     // 0x80158006 - cannot write to feedback log

    // Errors returned by XAUTH = 0x801584xx
    #ifdef DIRTYSDK_XHTTP_ENABLED
    DIRTYSOCK_ErrorName(XONLINE_S_XAUTH_ALREADY_STARTED),                   // 0x00158401
    DIRTYSOCK_ErrorName(XONLINE_S_XAUTH_AUTHLIST_DOWNLOAD_IN_PROGRESS),     // 0x00158402
    DIRTYSOCK_ErrorName(XONLINE_E_XAUTH_INVALID_LIBRARY_VERSION),           // 0x80158401
    DIRTYSOCK_ErrorName(XONLINE_E_XAUTH_URL_NOT_ALLOWED),                   // 0x80158402
    DIRTYSOCK_ErrorName(XONLINE_E_XAUTH_NOT_ENOUGH_BUFFER_SPACE),           // 0x80158403
    DIRTYSOCK_ErrorName(XONLINE_E_XAUTH_NULL_SETTINGS_PASSED),              // 0x80158404
    DIRTYSOCK_ErrorName(XONLINE_E_XAUTH_INVALID_CONFIG_FLAGS_PASSED),       // 0x80158405
    DIRTYSOCK_ErrorName(XONLINE_E_XAUTH_USER_NOT_SIGNED_INTO_XBOX_LIVE),    // 0x80158406
    DIRTYSOCK_ErrorName(XONLINE_E_XAUTH_NOT_ALLOWED),                       // 0x80158407
    DIRTYSOCK_ErrorName(XONLINE_E_XAUTH_NOT_STARTED),                       // 0x80158408
    #endif

    // Errors returned by the Statistics Service = 0x80159XXX
    DIRTYSOCK_ErrorName(XONLINE_E_STAT_BAD_REQUEST),                        // 0x80159001 - server received incorrectly formatted request.
    DIRTYSOCK_ErrorName(XONLINE_E_STAT_INVALID_TITLE_OR_LEADERBOARD),       // 0x80159002 - title or leaderboard id were not recognized by the server.
    DIRTYSOCK_ErrorName(XONLINE_E_STAT_TOO_MANY_SPECS),                     // 0x80159004 - too many stat specs in a request.
    DIRTYSOCK_ErrorName(XONLINE_E_STAT_TOO_MANY_STATS),                     // 0x80159005 - too many stats in a spec or already stored for the user.
    DIRTYSOCK_ErrorName(XONLINE_E_STAT_USER_NOT_FOUND),                     // 0x80159003 - user not found.
    DIRTYSOCK_ErrorName(XONLINE_E_STAT_SET_FAILED_0),                       // 0x80159100 - set operation failed on spec index 0
    DIRTYSOCK_ErrorName(XONLINE_E_STAT_PERMISSION_DENIED),                  // 0x80159200 - operation failed because of credentials. UserId is not logged in or this operation is not supported in production (e.g. userId=0 in XOnlineStatReset)
    DIRTYSOCK_ErrorName(XONLINE_E_STAT_LEADERBOARD_WAS_RESET),              // 0x80159201 - operation failed because user was logged on before the leaderboard was reset.
    DIRTYSOCK_ErrorName(XONLINE_E_STAT_INVALID_ATTACHMENT),                 // 0x80159202 - attachment is invalid.
    DIRTYSOCK_ErrorName(XONLINE_S_STAT_CAN_UPLOAD_ATTACHMENT),              // 0x00159203 - Use XOnlineStatWriteGetResults to get a handle to upload a attachment.
    DIRTYSOCK_ErrorName(XONLINE_E_STAT_TOO_MANY_PARAMETERS),                // 0x80159204
    DIRTYSOCK_ErrorName(XONLINE_E_STAT_TOO_MANY_PROCEDURES),                // 0x80159205
    DIRTYSOCK_ErrorName(XONLINE_E_STAT_STAT_POST_PROC_ERROR),               // 0x80159206

    // Errors returned by Signature Service = 0x8015B0XX
    DIRTYSOCK_ErrorName(XONLINE_E_SIGNATURE_VER_INVALID_SIGNATURE),         // 0x8015B001 - presented signature does not match
    DIRTYSOCK_ErrorName(XONLINE_E_SIGNATURE_VER_UNKNOWN_KEY_VER),           // 0x8015B002 - signature key version specified is not found among the valid signature keys
    DIRTYSOCK_ErrorName(XONLINE_E_SIGNATURE_VER_UNKNOWN_SIGNATURE_VER),     // 0x8015B003 - signature version is unknown, currently only version 1 is supported
    DIRTYSOCK_ErrorName(XONLINE_E_SIGNATURE_BANNED_XBOX),                   // 0x8015B004 - signature is not calculated or revoked because Xbox is banned
    DIRTYSOCK_ErrorName(XONLINE_E_SIGNATURE_BANNED_USER),                   // 0x8015B005 - signature is not calculated or revoked because at least one user is banned
    DIRTYSOCK_ErrorName(XONLINE_E_SIGNATURE_BANNED_TITLE),                  // 0x8015B006 - signature is not calculated or revoked because the given title and version is banned
    DIRTYSOCK_ErrorName(XONLINE_E_SIGNATURE_BANNED_DIGEST),                 // 0x8015B007 - signature is not calculated or revoked because the digest is banned
    DIRTYSOCK_ErrorName(XONLINE_E_SIGNATURE_GET_BAD_AUTH_DATA),             // 0x8015B008 - fail to retrieve AuthData from SG, returned by GetSigningKey api
    DIRTYSOCK_ErrorName(XONLINE_E_SIGNATURE_SERVICE_UNAVAILABLE),           // 0x8015B009 - fail to retrieve a signature server master key, returned by GetSigningKey or SignOnBehalf api

    // Errors returned by Arbitration Service = 0x8015B1XX
    DIRTYSOCK_ErrorName(XONLINE_E_ARBITRATION_SERVICE_UNAVAILABLE),                     // 0x8015B101 - Service temporarily unavailable
    DIRTYSOCK_ErrorName(XONLINE_E_ARBITRATION_INVALID_REQUEST),                         // 0x8015B102 - The request is invalidly formatted
    DIRTYSOCK_ErrorName(XONLINE_E_ARBITRATION_SESSION_NOT_FOUND),                       // 0x8015B103 - The session is not found or has expired
    DIRTYSOCK_ErrorName(XONLINE_E_ARBITRATION_REGISTRATION_FLAGS_MISMATCH),             // 0x8015B104 - The session was registered with different flags by another Xbox
    DIRTYSOCK_ErrorName(XONLINE_E_ARBITRATION_REGISTRATION_SESSION_TIME_MISMATCH),      // 0x8015B105 - The session was registered with a different session time by another Xbox
    DIRTYSOCK_ErrorName(XONLINE_E_ARBITRATION_REGISTRATION_TOO_LATE),                   // 0x8015B106 - Registration came too late, the session has already been arbitrated
    DIRTYSOCK_ErrorName(XONLINE_E_ARBITRATION_NEED_TO_REGISTER_FIRST),                  // 0x8015B107 - Must register in seesion first, before any other activity
    DIRTYSOCK_ErrorName(XONLINE_E_ARBITRATION_TIME_EXTENSION_NOT_ALLOWED),              // 0x8015B108 - Time extension of this session not allowed, or session is already arbitrated
    DIRTYSOCK_ErrorName(XONLINE_E_ARBITRATION_INCONSISTENT_FLAGS),                      // 0x8015B109 - Inconsistent flags are used in the request
    DIRTYSOCK_ErrorName(XONLINE_E_ARBITRATION_INCONSISTENT_COMPETITION_STATUS),         // 0x8015B10A - Whether the session is a competition is inconsistent between registration and report
    DIRTYSOCK_ErrorName(XONLINE_E_ARBITRATION_REPORT_ALREADY_CALLED),                   // 0x8015b10B - Report call for this session already made by this client
    DIRTYSOCK_ErrorName(XONLINE_E_ARBITRATION_TOO_MANY_XBOXES_IN_SESSION),              // 0x8015b10C - Only up to 255 Xboxes can register in a session
    DIRTYSOCK_ErrorName(XONLINE_E_ARBITRATION_1_XBOX_1_USER_SESSION_NOT_ALLOWED),       // 0x8015b10D - Single Xbox single user sessions should not be arbitrated
    DIRTYSOCK_ErrorName(XONLINE_E_ARBITRATION_REPORT_TOO_LARGE),                        // 0x8015b10E - The stats or query submission is too large
    DIRTYSOCK_ErrorName(XONLINE_E_ARBITRATION_INVALID_TEAMTICKET),                      // 0x8015b10F - An invalid team ticket was submitted

    // Arbitration success HRESULTS
    DIRTYSOCK_ErrorName(XONLINE_S_ARBITRATION_INVALID_XBOX_SPECIFIED),      // 0x0015b1F0 - Invalid/duplicate Xbox specified in lost connectivity or suspicious info. Never the less, this report is accepted
    DIRTYSOCK_ErrorName(XONLINE_S_ARBITRATION_INVALID_USER_SPECIFIED),      // 0x0015b1F1 - Invalid/duplicate user specified in lost connectivity or suspicious info. Never the less, this report is accepted
    DIRTYSOCK_ErrorName(XONLINE_S_ARBITRATION_DIFFERENT_RESULTS_DETECTED),  // 0x0015b1F2 - Differing result submissions have been detected in this session. Never the less, this report submission is accepted

    // Errors returned by the Storage services = 0x8015C0XX
    DIRTYSOCK_ErrorName(XONLINE_E_STORAGE_INVALID_REQUEST),                 // 0x8015c001 - Request is invalid
    DIRTYSOCK_ErrorName(XONLINE_E_STORAGE_ACCESS_DENIED),                   // 0x8015c002 - Client doesn't have the rights to upload the file
    DIRTYSOCK_ErrorName(XONLINE_E_STORAGE_FILE_IS_TOO_BIG),                 // 0x8015c003 - File is too big
    DIRTYSOCK_ErrorName(XONLINE_E_STORAGE_FILE_NOT_FOUND),                  // 0x8015c004 - File not found
    DIRTYSOCK_ErrorName(XONLINE_E_STORAGE_INVALID_ACCESS_TOKEN),            // 0x8015c005 - Access token signature is invalid
    DIRTYSOCK_ErrorName(XONLINE_E_STORAGE_CANNOT_FIND_PATH),                // 0x8015c006 - name resolution failed
    DIRTYSOCK_ErrorName(XONLINE_E_STORAGE_FILE_IS_ELSEWHERE),               // 0x8015c007 - redirection request
    DIRTYSOCK_ErrorName(XONLINE_E_STORAGE_INVALID_STORAGE_PATH),            // 0x8015c008 - Invalid storage path
    DIRTYSOCK_ErrorName(XONLINE_E_STORAGE_INVALID_FACILITY),                // 0x8015c009 - Invalid facility code
    DIRTYSOCK_ErrorName(XONLINE_E_STORAGE_UNKNOWN_DOMAIN),                  // 0x8015c00A - Bad pathname
    DIRTYSOCK_ErrorName(XONLINE_E_STORAGE_SYNC_TIME_SKEW),                  // 0x8015c00B - SyncDomain timestamp skew
    DIRTYSOCK_ErrorName(XONLINE_E_STORAGE_SYNC_TIME_SKEW_LOCALTIME),        // 0x8015c00C - SyncDomain timestamp appears to be localtime
    DIRTYSOCK_ErrorName(XONLINE_E_STORAGE_QUOTA_EXCEEDED),                  // 0x8015c00D - Quota exceeded for storage domain
    DIRTYSOCK_ErrorName(XONLINE_E_STORAGE_UNSUPPORTED_CONTENT_TYPE),        // 0x8015c00E - The type of the content is not supported by this API
    DIRTYSOCK_ErrorName(XONLINE_E_STORAGE_FILE_ALREADY_EXISTS),             // 0x8015c011 - File already exists and storage domain does not allow overwrites
    DIRTYSOCK_ErrorName(XONLINE_E_STORAGE_DATABASE_ERROR),                  // 0x8015c012 - Unknown database error
    DIRTYSOCK_ErrorName(XONLINE_S_STORAGE_FILE_NOT_MODIFIED),               // 0x8015c013 - The file was not modified since the last installation

    // Property errors (from vfwmsgs.h)
    0x80070490, "E_PROP_ID_UNSUPPORTED",                                    // 0x80070490 - The specified property ID is not supported for the specified property set
    0x80070492, "E_PROP_SET_UNSUPPORTED",                                   // 0x80070492 - The Specified property set is not supported

    // SPA errors
    DIRTYSOCK_ErrorName(SPA_E_CORRUPT_FILE),                                // 0x8056f001
    DIRTYSOCK_ErrorName(SPA_E_NOT_LOADED),                                  // 0x8056f002
                                                                            //
    // NULL terminate
    DIRTYSOCK_ListEnd()
};

#define DIRTYERR_NUMERRORS (sizeof(_DirtyErr_List) / sizeof(_DirtyErr_List[0]))

#endif // #if DIRTYSOCK_ERRORNAMES

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

    \Version 06/13/2005 (jbrookes)
*/
/********************************************************************************F*/
void DirtyErrNameList(char *pBuffer, int32_t iBufSize, uint32_t uError, const DirtyErrT *pList)
{
    static char strUnknown[16];

    #if DIRTYSOCK_ERRORNAMES
    int32_t iErr;

    // first try to match exactly
    for (iErr = 0; pList[iErr].uError != DIRTYSOCK_LISTTERM; iErr++)
    {
        if ((pList[iErr].uError & ~0x80000000) == (uError & ~0x80000000))
        {
            ds_snzprintf(pBuffer, iBufSize, "%s/0x%08x", pList[iErr].pErrorName, uError);
            return;
        }
    }

    // if not found, try to match lower 16 bits of error code
    for (iErr = 0; pList[iErr].uError != DIRTYSOCK_LISTTERM; iErr++)
    {
        if ((pList[iErr].uError & ~0xffff0000) == (uError & ~0xffff0000))
        {
            ds_snzprintf(pBuffer, iBufSize, "%s/0x%08x", pList[iErr].pErrorName, uError);
            return;
        }
    }
    #endif

    ds_snzprintf(pBuffer, iBufSize, "0x%08x", uError);
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

    \Version 06/13/2005 (jbrookes)
*/
/********************************************************************************F*/
void DirtyErrName(char *pBuffer, int32_t iBufSize, uint32_t uError)
{
    #if DIRTYSOCK_ERRORNAMES
    DirtyErrNameList(pBuffer, iBufSize, uError, _DirtyErr_List);
    #endif
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

    \Version 06/13/2005 (jbrookes)
*/
/********************************************************************************F*/
const char *DirtyErrGetName(uint32_t uError)
{
    static char strName[128];
    DirtyErrName(strName, sizeof(strName), uError);
    return(strName);
}