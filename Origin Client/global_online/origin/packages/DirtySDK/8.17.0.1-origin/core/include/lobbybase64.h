/*H*******************************************************************/
/*!
    \File lobbybase64.h

    \Description
        This module Base-64 encoding/decoding as defined in RFC 
        989, 1040, 1113, 1421, 1521 and 2045.

    \Copyright
        Copyright (c) Electronic Arts 2003. ALL RIGHTS RESERVED.

    \Version 1.0 12/11/2003 (SJB) First Version
*/
/*******************************************************************H*/

#ifndef _lobbybase64_h
#define _lobbybase64_h

/*** Include files ***************************************************/

/*** Defines *********************************************************/

/*** Macros **********************************************************/

//
// The number of bytes it takes to Base-64 encode the given number
// of bytes.  The result includes any required padding but not a '\0'
// terminator.
//
#define LobbyBase64EncodedSize(x) ((((x)+2)/3)*4)

//
// The maximum number of bytes it takes to hold the decoded version
// of a Base-64 encoded string that is 'x' bytes int32_t.
//
// In this version of Base-64, 'x' is always a multiple of 4 and the
// result is always a multiple of 3 (i.e. there may be up to 2 padding
// bytes at the end of the decoded value).  It is assumed that the
// exact length of the string (minus any padding) is either encoded in
// the string or is external to the string.
//
#define LobbyBase64DecodedSize(x) (((x)/4)*3)

/*** Type Definitions ************************************************/

/*** Variables *******************************************************/

/*** Functions *******************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// Base64 encode a string
void LobbyBase64Encode(int32_t iInputLen, const char *pInput, char *pOutput);


// Decode a Base64 encoded string.
int32_t LobbyBase64Decode(int32_t len, const char* in, char* out);

#ifdef __cplusplus
}
#endif

#endif
