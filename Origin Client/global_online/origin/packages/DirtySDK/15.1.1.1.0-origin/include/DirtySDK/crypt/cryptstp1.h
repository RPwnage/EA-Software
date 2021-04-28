/*H*******************************************************************/
/*!
    \File cryptstp1.h

    \Description
        Implements STP1 (session ticket protocol version one) for encrypting
        session streams via shared secret encoded tickets. RC4 is used for
        encryption and MD5 for data verification. Original protocol design
        by Stephen Bevan and module packaging by Greg Schaefer.

        Module derived from sbevan encryption code originally in SessCrypt.cpp,
        in Server.cpp, ProtoAries.cpp, Dir-Main.cpp and LobbyAPI.c

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 03/28/2004 (gschaefer) Derived from sbevan encryption code
*/
/*******************************************************************H*/

#ifndef _CryptStp1_h
#define _CryptStp1_h

/*!
\Moduledef CryptStp1 CryptStp1
\Modulemember Crypt
*/
//@{

/*** Include files ***************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/crypt/cryptarc4.h"
#include "DirtySDK/crypt/cryptmd5.h"

/*** Defines *********************************************************/

/*** Macros **********************************************************/

/*** Type Definitions ************************************************/

//! stream encryption state
typedef struct CryptStp1StreamT
{
    int32_t bEncrypt;               // PRIVATE: encryption enable flag
    int32_t bDecrypt;               // PRIVATE: decryption enable flag
    CryptArc4T RecvArc4;            // PRIVATE: state for receive stream
    CryptArc4T SendArc4;            // PRIVATE: state for send stream
} CryptStp1StreamT;

//! shared secret used to generate tickets
typedef struct CryptStp1SharedT
{
    unsigned char strSecret[32];    // PRIVATE
} CryptStp1SharedT;

//! private secret used to stream encryption
typedef struct CryptStp1SecretT
{
    char strSend[MD5_BINARY_OUT];   // PRIVATE: send stream key
    char strRecv[MD5_BINARY_OUT];   // PRIVATE: receive stream key
} CryptStp1SecretT;

//! ticket used to pass stream crypto keys
typedef struct CryptStp1TicketT
{
    struct                          // this part is sent as plain text
    {
        char IV[MD5_BINARY_OUT];    // PRIVATE: cipher initialization vector
    } Plain;
    struct                          // this part is sent encrypted
    {
        CryptStp1SecretT Secret;    // PRIVATE: actual secret keys
        uint32_t uTime;         // PRIVATE: ticket issue time in host order
    } Encrypted;
} CryptStp1TicketT;

//! wallet containing seret keys & ticket
typedef struct CryptStp1WalletT
{
    CryptStp1SecretT Secret;        // PRIVATE: secret keys for encypting stream
    CryptStp1TicketT Ticket;        // PRIVATE: ticket for passing keys to server
} CryptStp1WalletT;

/*** Variables *******************************************************/

/*** Functions *******************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NO_SESSION_CRYPTO

// Set the master shared secret that is used for encrypting tickets.
DIRTYCODE_API void CryptStp1SetShared(CryptStp1SharedT *pShared, const void *pData,
                        uint32_t uDataSize);

// Create new wallet containing secret keys and a ticket
DIRTYCODE_API void CryptStp1MakeWallet(CryptStp1WalletT *pWallet, CryptStp1SharedT *pShared, uint32_t uTime, const char *pRandom);

// Open the wallet and remove the secret and ticket
DIRTYCODE_API int32_t CryptStp1OpenWallet(CryptStp1WalletT *pWallet, CryptStp1SecretT *pSecret, CryptStp1TicketT *pTicket);

// Use the secret keys to initialize the session stream
DIRTYCODE_API int32_t CryptStp1UseSecret(CryptStp1StreamT *pStream, CryptStp1SecretT *pSecret);

// USe the secret keys to initialize the session stream
DIRTYCODE_API int32_t CryptStp1UseTicket(CryptStp1StreamT *pStream, CryptStp1TicketT *pTicket, CryptStp1SharedT *pShared, uint32_t uTime);

// Decrypt data from a stream
DIRTYCODE_API void CryptStp1DecryptData(CryptStp1StreamT *pStream, char *pData, int32_t iSize);

// Decrypt data from a stream. Return code indicates error. You must check and obey this result.
DIRTYCODE_API int32_t CryptStp1DecryptHash(CryptStp1StreamT *pStream, char *pData, int32_t iSize);

// Return the updated packet size (will remove hash if present)
DIRTYCODE_API int32_t CryptStp1DecryptSize(CryptStp1StreamT *pStream, int32_t iSize);

// Return the updated packet size (will remove hash if present)
DIRTYCODE_API int32_t CryptStp1EncryptSize(CryptStp1StreamT *pStream, int32_t iSize);

// Calculate and append the packet hash. Call after CryptStp1EncryptSize.
DIRTYCODE_API int32_t CryptStp1EncryptHash(CryptStp1StreamT *pStream, char *pData, int32_t iSize);

// Encrypt the packet data
DIRTYCODE_API void CryptStp1EncryptData(CryptStp1StreamT *pStream, char *pData, int32_t iSize);

// Determine if encrypting is enabled
DIRTYCODE_API int32_t CryptStp1Enabled(CryptStp1StreamT *pStream);

#else

// Set the master shared secret that is used for encrypting tickets.
#define CryptStp1SetShared(pShared, pData, uDataSize)

// Create new wallet containing secret keys and a ticket
#define CryptStp1MakeWallet(pWallet, pShared, uTime, pRandom)

// Open the wallet and remove the secret and ticket
#define CryptStp1OpenWallet(pWallet, pSecret, pTicket)  1

// Use the secret keys to initialize the session stream
#define CryptStp1UseSecret(pStream, pSecret) 0

// USe the secret keys to initialize the session stream
#define CryptStp1UseTicket(pStream, pTicket, pShared, uTime) 0

// Decrypt data from a stream
#define CryptStp1DecryptData(pStream, pData, iSize)

// Decrypt data from a stream. Return code indicates error. You must check and obey this result.
#define CryptStp1DecryptHash(pStream, pData, iSize) 0

// Return the updated packet size (will remove hash if present)
#define CryptStp1DecryptSize(pStream, iSize) (iSize)

// Return the updated packet size (will remove hash if present)
#define CryptStp1EncryptSize(pStream, iSize) (iSize)

// Calculate and append the packet hash. Call after CryptStp1EncryptSize.
#define CryptStp1EncryptHash(pStream, pData, iSize) 0

// Encrypt the packet data
#define CryptStp1EncryptData(pStream, pData, iSize)

// Determine if encrypting is enabled
#define CryptStp1Enabled(pStream) 0

#endif // NO_CRYPTO

#ifdef __cplusplus
}
#endif

//@}

#endif
