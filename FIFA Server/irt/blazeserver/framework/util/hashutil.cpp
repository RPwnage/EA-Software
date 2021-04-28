/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "hashutil.h"
#include "random.h"
#include "shared/base64.h"

#include <openssl/sha.h> // for SHA256

namespace Blaze
{
    void HashUtil::generateSalt(Int256Buffer& saltOut)
    {
        gRandom->getStrongRandomNumber256(saltOut);
    }

    void HashUtil::generateSHA256Hash(const char8_t* source, size_t sourceLength, char8_t* outBuffer, size_t bufferLength, bool addSalt, Int256Buffer* saltOut)
    {
        EA_ASSERT(bufferLength >= SHA256_STRING_OUT);

        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, source, sourceLength);

        if (addSalt)
        {
            Int256Buffer randBuf;
            generateSalt(randBuf);

            if (saltOut != nullptr)
            {
                randBuf.copyInto(*saltOut);
            }

            // hash with random 256 bits
            int64_t random[4] = { 
                randBuf.getPart1(), randBuf.getPart2(),
                randBuf.getPart3(), randBuf.getPart4() };
            SHA256_Update(&sha256, &random, sizeof(random));
        }

        unsigned char result[SHA256_DIGEST_LENGTH];
        SHA256_Final(result, &sha256);

        // convert to readable portable digits.
        sha256ResultToReadableString(result, outBuffer, bufferLength);
    }

    void HashUtil::sha256ResultToReadableString(const unsigned char (&shaResult)[SHA256_DIGEST_LENGTH], char* buf, size_t bufLen)
    {
        // store Base64 encoded to buf
        sha256ToBase64Buffer base64buf(buf, bufLen);
        Blaze::Base64::encode((uint8_t*)shaResult, (uint32_t)SHA256_DIGEST_LENGTH, &base64buf);
        buf[SHA256_STRING_OUT-1] = 0;
    }
	void HashUtil::hashSHA512HMAC(const char8_t* data, size_t dataLen, char8_t* hash)
	{
		SHA512_CTX context;
		SHA512_Init(&context);
		SHA512_Update(&context, data, dataLen);
		SHA512_Final((unsigned char*)hash, &context);
	}
	void HashUtil::initializeSha512HMACPadding(const char8_t* key, size_t keyLen, char8_t* ipad, char8_t* opad)
	{
		uint8_t pad1 = 0x36;
		uint8_t pad2 = 0x5c;
		memset( ipad, pad1, SHA512_CBLOCK );
		memset( opad, pad2, SHA512_CBLOCK );
		for (uint32_t i = 0; i < keyLen; ++i) 
		{
			ipad[i] = key[i] ^ pad1;
			opad[i] = key[i] ^ pad2;
		}
	}
	void HashUtil::generateSHA512HMACHash(const char8_t* key, size_t keyLen, const char8_t* source, size_t sourceLength, char8_t* outBuffer, size_t bufferLength)
	{
		char8_t temp[SHA512_DIGEST_LENGTH];
		memset(temp, 0, SHA512_DIGEST_LENGTH);
		if (keyLen > SHA512_CBLOCK)
		{
			hashSHA512HMAC(key, keyLen, temp);
			key = temp;
			keyLen = SHA512_DIGEST_LENGTH;
		}
		char8_t ipad[SHA512_CBLOCK];
		char8_t opad[SHA512_CBLOCK];
		initializeSha512HMACPadding(key, keyLen, ipad, opad);
		SHA512_CTX sha2;
		SHA512_Init(&sha2);
		SHA512_Update(&sha2, ipad, SHA512_CBLOCK);
		SHA512_Update(&sha2, source, sourceLength);
		SHA512_Final((unsigned char*)temp, &sha2);
		SHA512_Init(&sha2);
		SHA512_Update(&sha2, opad, SHA512_CBLOCK);
		SHA512_Update(&sha2, temp, SHA512_DIGEST_LENGTH);
		unsigned char result[SHA512_DIGEST_LENGTH];
		SHA512_Final(result, &sha2);
		sha512ResultToHexString(result, outBuffer, bufferLength);
	}
	bool HashUtil::sha512ResultToHexString(const unsigned char (&shaResult)[SHA512_DIGEST_LENGTH], char* buf, size_t bufLen)
	{
		if (bufLen <= SHA512_HEXSTRING_OUT)
			return false;
		eastl::string hexString;
		hexString.clear();
		hexString.set_capacity(SHA512_HEXSTRING_OUT);
		for(int i = 0; i < SHA512_DIGEST_LENGTH; i++)
		{
			hexString.append_sprintf("%02x", shaResult[i]);
		}
		memcpy(buf, hexString.c_str(), SHA512_HEXSTRING_OUT);
		return true;
    }

    uint32_t HashUtil::sha256ToBase64Buffer::write(uint8_t data)
    {
        if (mResultBufLen <= mNumWritten)
            return 0;

        // translate these Base64/RFC1113 chars to allow for use in WAL URL's.
        if (data == '/')
            data = '$'; // url-safe char (http://www.faqs.org/rfcs/rfc1738.html)
        if (data == '+')
            data = '*'; // also should replace '+' as it has special meaning to Blaze URL decoder

        mResultBuf[mNumWritten++] = data;
        return 1;
    }

} // Blaze

