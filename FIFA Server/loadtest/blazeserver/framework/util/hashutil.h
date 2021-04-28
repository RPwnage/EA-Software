/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_HASHUTIL_H
#define BLAZE_HASHUTIL_H

/*** Include files *******************************************************************************/
#include "framework/util/shared/outputstream.h" // for OutputStream in sha256ToBase64Buffer
#include "framework/tdf/usersessiontypes_server.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
    class HashUtil
    {
    public:
		// for SHA256 result as string, 44 base64 digits represent result's 256 bits, plus null terminator
		static const size_t SHA256_STRING_OUT = 44 + 1;

		static const size_t SHA512_HEXSTRING_OUT = 128 + 1;

        static void generateSalt(Int256Buffer& saltOut);
        static void generateSHA256Hash(const char8_t* source, size_t sourceLength, char8_t* outBuffer, size_t bufferLength, bool addSalt = false, Int256Buffer* saltOut = nullptr);
        static void sha256ResultToReadableString(const unsigned char (&shaResult)[SHA256_DIGEST_LENGTH], char* buf, size_t bufLen);
		static void generateSHA512HMACHash(const char8_t* key, size_t keyLen, const char8_t* source, size_t sourceLength, char8_t* outBuffer, size_t bufferLength); // Added for FUT 15 HMAC blaze match report
		static bool sha512ResultToHexString(const unsigned char (&shaResult)[SHA512_DIGEST_LENGTH], char* buf, size_t bufLen);

        // Helper class for converting sha256 result to readable, portable string
        class sha256ToBase64Buffer : public OutputStream
        {
        public:
            sha256ToBase64Buffer(char *resultBuf, size_t resultBufLen) : mResultBuf(resultBuf), mResultBufLen(resultBufLen), mNumWritten(0) {}
            ~sha256ToBase64Buffer() override {}
            uint32_t write(uint8_t data) override;

        private:
            char* mResultBuf;
            size_t mResultBufLen;
            size_t mNumWritten;
        };
	private:
		static void hashSHA512HMAC(const char8_t* data, size_t dataLen, char8_t* hash);
		static void initializeSha512HMACPadding(const char8_t* key, size_t keyLen, char8_t* ipad, char8_t* opad);
    };

} // Blaze

#endif // BLAZE_HASHUTIL_H

