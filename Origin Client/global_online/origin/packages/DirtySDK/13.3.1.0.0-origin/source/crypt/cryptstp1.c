/*H*******************************************************************/
/*!
    \File cryptstp1.c

    \Description
        Implements STP1 (session ticket protocol version one) for encrypting
        session streams via shared secret encoded tickets. RC4 is used for
        encryption and MD5 for data verification. Original protocol design
        by Stephen Bevan and module packaging by Greg Schaefer.

        Module derived from sbevan encryption code originally in SessCrypt.cpp,
        in Server.cpp, ProtoAries.cpp, Dir-Main.cpp and LobbyAPI.c

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Notes
        Q: Why is there a separate bDecrypt flag?  
        A: It is a gross hack to deal with the way synthesized
           ProtoAries connect/disconnect messages on a PS2 interact with 
           encryption.  See RpcProtoAriesRecv where CryptStp1DecryptSize.
           is called to adjust the length of a packet and consider what
           happens when RpcProtoAriesRecv tries to process a synthesized
           packet that is generated as a result of a connection which,
           due to the asynchronous nature of things, has already been marked
           as encrypted *before* the packet has been processed by the caller.
           The clean fix for this is to alter a lot more ProtoAries/LobbyApi
           code so that it doesn't attempt to send any packets until the
           connection completion event has been read.

    \Version 1.0 03/28/2004 (gschaefer) Derived from sbevan encryption code
*/
/*******************************************************************H*/

    // Generate a new shared secret for this "user".
    // the SECRET consits of two session keys, one for sending
    // and one for receving :-
    //
    //    SEND = MD5(clientaddr||clientport||"sending"||random||current-time)
    //    RECV = MD5(clientaddr||clientport||"receiving"||random||current-time)
    //
    // SECRET = SEND||RECV
    //
    // The send&receive terminology is from the perspective of the client
    // so the client will send using SEND and the server will receive using
    // SEND.  The client will receiving using RECV and the server will send
    // using RECV.
    //
    // The reason for two keys is that since RC4 is used for encryption
    // using the same session key in for both directions would result in
    // the same keystream being used in both directions, and re-using a
    // keystream is a no-no for stream ciphers.
    //
    // The TICKET is RC4(SEND||RECV||current-time).  The time is in there
    // so that on receipt of a TICKET the server can check to see if the
    // ticket stale or not.  The client's IP address is not in the ticket
    // since the re-director is running behind an SSL proxy and so the
    // re-director doesn't know the client's real IP address.

/*** Include files ***************************************************/

#ifndef NO_SESSION_CRYPTO

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"

#include "DirtySDK/crypt/cryptmd5.h"
#include "DirtySDK/crypt/cryptarc4.h"

#include "DirtySDK/crypt/cryptstp1.h"

/*** Defines *********************************************************/

#define MAX_TICKET_AGE 3600

/*** Type Definitions ************************************************/

/*** Private Functions ***********************************************/

/*F*******************************************************************/
/*!
    \Function CryptStp1MakeMasterSecret

    \Description
        Set the master shared secret that is used for encrypting tickets.

    \Input *pShared       - the shared secret (CryptStp1SharedT)
    \Input *pTicket       - the ticket containing the IV to use
    \Input *pMasterSecret - where to write the master secret (at least 128 bytes)

    \Output the size of the master secret in bytes.

    \Notes
       Since we are using a stream cipher to encrypt each ticket
       we don't want to use the same shared secret (pShared) each time
       otherwise it would be relatively simple to recover the secret by
       examining multiple tickets.  So, to thwart that simple attack
       we use "secret||iv||secret" instead where IV is unique per
       generated ticket.

    \Version 1.0 05/05/2004 (sbevan) First Version
*/
/*******************************************************************F*/
static uint32_t _CryptStp1MakeMasterSecret(CryptStp1SharedT *pShared, CryptStp1TicketT *pTicket, char *pMasterSecret)
{
    memcpy(pMasterSecret, pShared->strSecret, sizeof(pShared->strSecret));
    memcpy(pMasterSecret + sizeof(pShared->strSecret), pTicket->Plain.IV, sizeof(pTicket->Plain.IV));
    memcpy(pMasterSecret + sizeof(pShared->strSecret) + sizeof(pTicket->Plain.IV), pShared->strSecret, sizeof(pShared->strSecret));
    return(2*sizeof(pShared->strSecret) + sizeof(pTicket->Plain.IV));
}


/*** Public functions ************************************************/


/*F*******************************************************************/
/*!
    \Function CryptStp1SetShared

    \Description
        Set the master shared secret that is used for encrypting tickets.

    \Input *pShared   - the shared secret (CryptStp1SharedT)
    \Input *pData     - the shared secret (in binary)
    \Input *uDataSize - the number of bytes in the secret

    \Output none

    \Notes
        If pData points to less than 32 bytes of cryptographically
        random data then it will result in a weak master key.

    \Version 1.0 03/28/2004 (gschaefer) First Version
    \Version 2.0 05/05/2004 (sbevan)    Changed to take uDataSize argument
*/
/*******************************************************************F*/
void CryptStp1SetShared(CryptStp1SharedT *pShared, const void *pData, uint32_t uDataSize)
{
    if (uDataSize >= sizeof(pShared->strSecret))
    {
        memcpy(pShared->strSecret, pData, sizeof(pShared->strSecret));
    }
    else
    {
        // We haven't been given enough data, so do the best we can 
        // with what we've been given by xoring the data together
        // to form a longer secret.  Ideally this code is never executed.
        uint32_t i, j;
        const char *pBytes = pData;
        for (i = 0, j = 0; i != uDataSize; i += 1)
        {
            pShared->strSecret[j] ^= pBytes[i];
            j = (j+1) % sizeof(pShared->strSecret);
        }
    }
}

/*F*******************************************************************/
/*!
    \Function CryptStp1MakeWallet

    \Description
        Create new wallet containing secret keys and a ticket

    \Input *pWallet - pointer to wallet output record
    \Input *pShared - the shared secret (for encrypting ticket)
    \Input uTime    - curent time (to expire tickets)
    \Input *pRandom - random data used in secret key generation

    \Output None

    \Version 1.0 03/28/2004 (gschaefer) First Version
    \Version 1.1 05/05/2004 (sbevan)    Now initialized ticket IV.
*/
/*******************************************************************F*/
void CryptStp1MakeWallet(CryptStp1WalletT *pWallet, CryptStp1SharedT *pShared, uint32_t uTime, const char *pRandom)
{
    CryptMD5T MD5;
    CryptArc4T Arc4;
    char strCrypt[256];
    uint32_t uSize;

    // build the client send key
    ds_snzprintf(strCrypt, sizeof(strCrypt), "send-%s-send", pRandom);
    CryptMD5Init(&MD5);
    CryptMD5Update(&MD5, strCrypt, -1);
    CryptMD5Final(&MD5, pWallet->Secret.strSend, MD5_BINARY_OUT);

    // build the client receive key
    ds_snzprintf(strCrypt, sizeof(strCrypt), "recv-%s-recv", pRandom);
    CryptMD5Init(&MD5);
    CryptMD5Update(&MD5, strCrypt, -1);
    CryptMD5Final(&MD5, pWallet->Secret.strRecv, MD5_BINARY_OUT);

    // copy secret to ticket
    memcpy(&pWallet->Ticket.Encrypted.Secret, &pWallet->Secret, sizeof(pWallet->Secret));
    // add an expiration time
    pWallet->Ticket.Encrypted.uTime = uTime;

    // build cipher IV
    ds_snzprintf(strCrypt, sizeof(strCrypt), "iv-%s-iv", pRandom);
    CryptMD5Init(&MD5);
    CryptMD5Update(&MD5, strCrypt, -1);
    CryptMD5Final(&MD5, pWallet->Ticket.Plain.IV, MD5_BINARY_OUT);

    uSize = _CryptStp1MakeMasterSecret(pShared, &pWallet->Ticket, strCrypt);
    CryptArc4Init(&Arc4, (unsigned char *)strCrypt, uSize, -1);
    CryptArc4Apply(&Arc4, (unsigned char *)&pWallet->Ticket.Encrypted, sizeof(pWallet->Ticket.Encrypted));
}

/*F*******************************************************************/
/*!
    \Function CryptStp1OpenWallet

    \Description
        Open the wallet and remove the secret and ticket

    \Input *pWallet - pointer to wallet output record
    \Input *pSecret - where to store the secret
    \Input *pTicket - where to store the ticket

    \Output <0 - error, the wallet is is malformed
            >0 - wallet is is valid

    \Notes
        pSecret and pTicket are only updated if the return value > 0.

    \Version 1.0 03/28/2004 (gschaefer) First Version
*/
/*******************************************************************F*/
int32_t CryptStp1OpenWallet(CryptStp1WalletT *pWallet, CryptStp1SecretT *pSecret, CryptStp1TicketT *pTicket)
{
    // Currently no validation is done of the wallet.
    // It is expected that in a future release some validation/version
    // check will be applied to the wallet.
    //
    if (pSecret != NULL)
    {
        memcpy(pSecret, &pWallet->Secret, sizeof(pWallet->Secret));
    }
    if (pTicket != NULL)
    {
        memcpy(pTicket, &pWallet->Ticket, sizeof(pWallet->Ticket));
    }
    return(1);
}

/*F*******************************************************************/
/*!
    \Function CryptStp1UseSecret

    \Description
        USe the secret keys to initialize the session stream

    \Input *pStream - the session stream
    \Input *pSecret - the secret keys

    \Output <0 - error, =0 - don't encrypt, >0 - encrypt

    \Version 1.0 03/28/2004 (gschaefer) First Version
*/
/*******************************************************************F*/
int32_t CryptStp1UseSecret(CryptStp1StreamT *pStream, CryptStp1SecretT *pSecret)
{
    // default to no encryption
    pStream->bEncrypt = 0;
    if (pSecret != NULL)
    {
        // enable encryption & disable decryption.
        pStream->bEncrypt = 1;
        pStream->bDecrypt = 0;
        CryptArc4Init(&pStream->SendArc4, (unsigned char *)pSecret->strSend, sizeof(pSecret->strSend), -1);
        CryptArc4Init(&pStream->RecvArc4, (unsigned char *)pSecret->strRecv, sizeof(pSecret->strRecv), -1);
    }

    return(pStream->bEncrypt);
}

/*F*******************************************************************/
/*!
    \Function CryptStp1UseTicket

    \Description
        USe the secret keys to initialize the session stream

    \Input *pStream - the session stream
    \Input *pTicket - the session ticket
    \Input *pShared - the shared secret (to decrypt the ticket)
    \Input uTime    - current time (to expire the ticket)

    \Output <0 - error, =0 - don't encrypt, >0 - encrypt

    \Version 1.0 03/28/2004 (gschaefer) First Version
*/
/*******************************************************************F*/
int32_t CryptStp1UseTicket(CryptStp1StreamT *pStream, CryptStp1TicketT *pTicket, CryptStp1SharedT *pShared, uint32_t uTime)
{
    CryptArc4T Arc4;
    CryptStp1TicketT Ticket;
    uint32_t uSize;
    char strCrypt[256];

    // null ticket means no encryption
    if (pTicket == NULL)
    {
        pStream->bEncrypt = 0;
        return(0);
    }

    // decrypt the ticket
    memcpy(&Ticket, pTicket, sizeof(Ticket));
    
    uSize = _CryptStp1MakeMasterSecret(pShared, &Ticket, strCrypt);
    CryptArc4Init(&Arc4, (unsigned char *)strCrypt, uSize, -1);
    CryptArc4Apply(&Arc4, (unsigned char *)&Ticket.Encrypted, sizeof(Ticket.Encrypted));

    // make sure ticket has not expired
    if ((Ticket.Encrypted.uTime > uTime) || (uTime-Ticket.Encrypted.uTime > MAX_TICKET_AGE))
    {
        return(-1);
    }

    // initialize the session keys and enable crypto
    CryptArc4Init(&pStream->RecvArc4, (unsigned char *)Ticket.Encrypted.Secret.strSend, sizeof(Ticket.Encrypted.Secret.strSend), -1);
    CryptArc4Init(&pStream->SendArc4, (unsigned char *)Ticket.Encrypted.Secret.strRecv, sizeof(Ticket.Encrypted.Secret.strRecv), -1);
    pStream->bEncrypt = 1;
    pStream->bDecrypt = 0;
    return(1);
}

/*F*******************************************************************/
/*!
    \Function CryptStp1DecryptData

    \Description
        Decrypt data from a stream

    \Input *pStream  - the session stream
    \Input *pData    - the data to decrypt
    \Input iSize     - size of the data

    \Output None

    \Version 1.0 03/28/2004 (gschaefer) First Version
*/
/*******************************************************************F*/
void CryptStp1DecryptData(CryptStp1StreamT *pStream, char *pData, int32_t iSize)
{
    if (pStream->bEncrypt)
    {
        CryptArc4Apply(&pStream->RecvArc4, (unsigned char *)pData, iSize);
        pStream->bDecrypt = 1;
    }
}

/*F*******************************************************************/
/*!
    \Function CryptStp1DecryptHash

    \Description
        Decrypt data from a stream. Return code indicates error. You
        must check and obey this result.

    \Input *pStream  - the session stream
    \Input *pData    - the data to decrypt
    \Input iSize     - size of the data

    \Output int32_t - negative=error, zero=success

    \Version 1.0 03/28/2004 (gschaefer) First Version
*/
/*******************************************************************F*/
int32_t CryptStp1DecryptHash(CryptStp1StreamT *pStream, char *pData, int32_t iSize)
{
    CryptMD5T RecvHash;
    char strHash[MD5_BINARY_OUT];

    // handle non-encrypt case
    if ((pStream->bEncrypt == 0) || (pStream->bDecrypt == 0))
    {
        return(0);
    }

    // make sure enough data for hash
    if (iSize < MD5_BINARY_OUT/2)
    {
        return(-1);
    }

    // calculate and check the hash
    CryptMD5Init(&RecvHash);
    CryptMD5Update(&RecvHash, (unsigned char *)pData, iSize-MD5_BINARY_OUT/2);
    CryptMD5Final(&RecvHash, strHash, MD5_BINARY_OUT);
    if (memcmp(pData+iSize-MD5_BINARY_OUT/2, strHash, MD5_BINARY_OUT/2) != 0)
    {
        return(-2);
    }
    
    // packet is valid
    return(0);
}

/*F*******************************************************************/
/*!
    \Function CryptStp1DecryptSize

    \Description
        Return the updated packet size (will remove hash if present)

    \Input *pStream  - the session stream
    \Input iSize     - size of the data

    \Output int32_t - updated packet size

    \Version 1.0 03/28/2004 (gschaefer) First Version
*/
/*******************************************************************F*/
int32_t CryptStp1DecryptSize(CryptStp1StreamT *pStream, int32_t iSize)
{
    if (pStream->bEncrypt && pStream->bDecrypt && (iSize >= MD5_BINARY_OUT/2))
    {
        iSize -= MD5_BINARY_OUT/2;
    }
    return(iSize);
}

/*F*******************************************************************/
/*!
    \Function CryptStp1EncryptSize

    \Description
        Return the updated packet size (will remove hash if present)

    \Input *pStream  - the session stream
    \Input iSize     - size of the data

    \Output int32_t - updated packet size

    \Version 1.0 03/28/2004 (gschaefer) First Version
*/
/*******************************************************************F*/
int32_t CryptStp1EncryptSize(CryptStp1StreamT *pStream, int32_t iSize)
{
    // adjust data size if encrypting
    if (pStream->bEncrypt)
    {
        iSize += MD5_BINARY_OUT/2;
    }
    return(iSize);
}

/*F*******************************************************************/
/*!
    \Function CryptStp1EncryptHash

    \Description
        Calculate and append the packet hash. Call after CryptStp1EncryptSize.

    \Input *pStream  - the session stream
    \Input *pData    - the data contaning hash
    \Input iSize     - size of the data

    \Output int32_t - negative=error, zero=success

    \Version 1.0 03/28/2004 (gschaefer) First Version
*/
/*******************************************************************F*/
int32_t CryptStp1EncryptHash(CryptStp1StreamT *pStream, char *pData, int32_t iSize)
{
    CryptMD5T SendHash;
    int32_t iHash = iSize-MD5_BINARY_OUT/2;

    // only hash if encrypted
    if (!pStream->bEncrypt)
    {
        return(0);
    }

    // make sure size is valid
    if (iHash < 0)
    {
        return(-1);
    }

    // calculate and store the hash
    // (though entire is calculated, only first 1/2 is kept due to size adjustment)
    CryptMD5Init(&SendHash);
    CryptMD5Update(&SendHash, (unsigned char*)pData, iHash);
    CryptMD5Final(&SendHash, (unsigned char *)pData+iHash, MD5_BINARY_OUT/2);
    return(0);
}

/*F*******************************************************************/
/*!
    \Function CryptStp1EncryptData

    \Description
        Encrypt the packet data

    \Input *pStream  - the session stream
    \Input *pData    - the data contaning hash
    \Input iSize     - size of the data

    \Output None

    \Version 1.0 03/28/2004 (gschaefer) First Version
*/
/*******************************************************************F*/
void CryptStp1EncryptData(CryptStp1StreamT *pStream, char *pData, int32_t iSize)
{
    // encrypt the packet if needed
    if (pStream->bEncrypt)
    {
        CryptArc4Apply(&pStream->SendArc4, (unsigned char *)pData, iSize);
    }
}

/*F********************************************************************************/
/*!
     \Function CryptStp1Enabled
 
     \Description
          Determine if encryption is enabled for the given instance.
  
     \Input pStream - Stream to check for encryption status.
 
     \Output int32_t - 0=disabled; else enabled
 
     \Version 05/31/2005 (doneill)
*/
/********************************************************************************F*/
int32_t CryptStp1Enabled(CryptStp1StreamT *pStream)
{
    return(pStream->bEncrypt && pStream->bDecrypt);
}

#endif // NO_CRYPTO

