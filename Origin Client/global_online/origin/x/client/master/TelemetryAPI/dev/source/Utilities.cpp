///////////////////////////////////////////////////////////////////////////////
// Utilities.cpp
//
//
//  Created by Kiss, Bela
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "Utilities.h"
#include "EAStdC/EAString.h"
#include "EAIO/EAFileUtil.h"
#include "netconn.h"

#if defined(ORIGIN_PC)
#include <windows.h>
#include <sddl.h>
#include <shlobj.h>
#elif defined(__linux__)
#include <assert.h>
#endif
#include "DirtySDK/crypt/cryptsha1.h"  // from DirtySDK 
#include <cryptmd5.h>  // from DirtySDK 

// external prototypes
#if defined(ORIGIN_MAC)
    // provided by dyld
    extern "C" { int _NSGetExecutablePath(char* buf, uint* bufsize); }
#endif

    namespace NonQTOriginServices
    {
        namespace Utilities
        {
            FILE* openFile(eastl::wstring const& fileName, eastl::wstring const& permissions)
            {
                FILE* file;

#if defined(ORIGIN_PC)
                file = _wfopen(fileName.c_str(), permissions.c_str());
#elif defined(ORIGIN_MAC) || defined(__linux__)
                eastl::string8 utf8FileName(EA::StdC::Strlcpy<eastl::string8, eastl::wstring>(fileName));
                eastl::string8 utf8Permissions(EA::StdC::Strlcpy<eastl::string8,  eastl::wstring>(permissions));
                file = fopen(utf8FileName.c_str(), utf8Permissions.c_str());
#else
#error "Requires implementation.")
#endif

                return file;
            }

            void getExecutablePath(char16_t& executablePath, size_t executablePathLenInChars)
            {
#if defined(ORIGIN_PC)

                GetModuleFileNameW(NULL, &executablePath, executablePathLenInChars);

#elif defined(ORIGIN_MAC)

                char buffer[4096];
                uint bufferLength = sizeof(buffer);
                int result = _NSGetExecutablePath(buffer, &bufferLength);
                if (result != 0)
                    return;
                char resolved_name[4096];
                realpath(buffer, resolved_name);
                EA::StdC::Strlcpy(&executablePath, resolved_name, executablePathLenInChars);

#elif defined(__linux__)
                assert(false);
#else
#error "Require platform implementation."
#endif
            }

            // gets directory to write binary/XML file to.
            void getIOLocation(const eastl::wstring &sDataPath, eastl::wstring &sFilePath)
            {
                const size_t kPathCapacity = 512;
                wchar_t appDirectory[kPathCapacity];

#if defined(ORIGIN_PC)
                EA::IO::GetSpecialDirectory(EA::IO::kSpecialDirectoryCommonApplicationData, appDirectory, true, kPathCapacity);
#elif defined(ORIGIN_MAC) || defined(__linux__)
                // on Mac, the telemetry data folder location is user specific, to minimize the number of directory creations
                // that require the escalation client.
                EA::IO::GetSpecialDirectory(EA::IO::kSpecialDirectoryUserApplicationData, appDirectory, true, kPathCapacity);
#elif defined(__linux__)
                assert(false);
#else
#error "Require platform implementation."
#endif

                sFilePath = appDirectory;
                sFilePath += sDataPath;

#if defined(ORIGIN_PC) || defined(_WIN32)

                // not working from installerdll.dll
                //EA::IO::Directory::Create(sFilePath.c_str());

                SECURITY_ATTRIBUTES sa;
                memset(&sa, 0, sizeof(sa));

                sa.nLength = sizeof(SECURITY_ATTRIBUTES);
                sa.bInheritHandle = FALSE;

                /* unused var
                wchar_t* pSD = L"D:"                   // Discretionary ACL
                L"(D;OICI;GA;;;BG)"     // Deny access to Built-in Guests
                L"(D;OICI;GA;;;AN)"     // Deny access to Anonymous Logon
                L"(A;OICI;GA;;;AU)" // Allow full control to Authenticated Users
                L"(A;OICI;GA;;;BA)";    // Allow full control to Administrators
                */

                //ConvertStringSecurityDescriptorToSecurityDescriptorW(pSD, SDDL_REVISION_1, &sa.lpSecurityDescriptor, NULL);
                SHCreateDirectoryExW(NULL, sFilePath.c_str(), NULL);//&sa);

                //if(sa.lpSecurityDescriptor) // This should always be true.
                //	LocalFree(sa.lpSecurityDescriptor);

#elif defined(ORIGIN_MAC) || defined(__linux__)
                EA::IO::Directory::Create(sFilePath.c_str());
#else
#error "Need platform implementation"
#endif			
            }

            void escape(std::string &str)
            {
                char c[] = { '&', '>', '<', '"', '\'' };
                const char *esc[] = { "amp;", "gt;", "lt;", "quot;", "apos;" };

                for (unsigned int j = 0; j < sizeof(c) / sizeof(char); ++j)
                {
                    for (unsigned int position = str.find(c[j], 0);
                        position != std::string::npos;
                        position = str.find(c[j], position + 1))
                    {
                        str[position] = '&';
                        str.insert(position + 1, esc[j]);
                    }
                }
            }

            void FourChars_2_UINT32(char8_t *dst, const char8_t *src, size_t len)
            {
                // fill with upper case 'X' placeholders - we always need 4 alphanumeric characters !!!
                // Julian Leong: I think that’s fine. I’ll just make that the new standard =)

                dst[0] = 'X';
                dst[1] = 'X';
                dst[2] = 'X';
                dst[3] = 'X';

                switch (len)
                {
                case 4:
                    dst[0] = src[3];
                    dst[1] = src[2];
                    dst[2] = src[1];
                    dst[3] = src[0];
                    break;
                case 3:
                    dst[1] = src[2];
                    dst[2] = src[1];
                    dst[3] = src[0];
                    break;
                case 2:
                    dst[2] = src[1];
                    dst[3] = src[0];
                    break;
                case 1:
                    dst[3] = src[0];
                    break;
                }
            }

            void CalculcateIncrementalChecksum(const void *data, uint64_t dataSize, TelemetryCheckSum &crc)
            {
                // simple fletcher checksum
                const char8_t *ip = (char8_t *)data;
                const char8_t *ipend = ip + dataSize;

                uint64_t a, b, c, d;

                a = crc.A;
                b = crc.B;
                c = crc.C;
                d = crc.D;

                for (; ip < ipend; ip++)
                {
                    a += ip[0];
                    b += a;
                    c += b;
                    d += c;
                }

                crc.A = a;
                crc.B = b;
                crc.C = c;
                crc.D = d;
            }

            // this code is wrong, PRIME and INIT are swapped, we may want to fix that at some point...
            //const uint64_t kFNV_64_PRIME = UINT64_C(0xcbf29ce484222325);
            //const uint64_t kFNV1A_64_INIT = UINT64_C(0x100000001b3);

            // From: http://isthe.com/chongo/tech/comp/fnv/
#define FNV_64_PRIME ((u_int64_t)1099511628211)
#define FNV1_64_INIT ((u_int64_t)14695981039346656037)

            const uint64_t kFNV_64_PRIME = UINT64_C(1099511628211);
            const uint64_t kFNV1A_64_INIT = UINT64_C(14695981039346656037);

            uint64_t calculateFNV1A(const char8_t *str, size_t size)
            {
                uint64_t hval = kFNV1A_64_INIT;
                char8_t* s = (char8_t*)str;

                //FNV-1a hash each octet of the string    
                while (size > 0)
                {
                    // xor the bottom with the current octet
                    hval ^= (uint64_t)*s++;
                    //multiply by the 64 bit FNV magic prime mod 2^64
                    hval *= kFNV_64_PRIME;
                    size--;
                }

                return hval;
            }

            uint64_t calculateFNV1A(const char8_t *str)
            {
                size_t s = EA::StdC::Strlen(str);
                if (!str || 0 == s)
                    return 0;

                return calculateFNV1A(str, s);
            }

            const eastl::string BinaryToHexHashConverter(uint8_t *pBinary, int32_t uLength, const int hashLength)
            {
                static const char8_t *const_HexChars = "0123456789abcdef";
                char *hex = new char[hashLength];
                int index = 0;
                for (int32_t i = 0; i < uLength; i++)
                {
                    hex[index++] = const_HexChars[(pBinary[i]>>4) & 0xF];
                    hex[index++] = const_HexChars[pBinary[i] & 0xF];
                }
                hex[hashLength-1] = '\0';
                eastl::string8 hexString(hex);
                delete[] hex;
                return hexString;
            }


            const eastl::string8 encryptSHA1(const eastl::string8& toHash)
            {
                CryptSha1T SHAContext;
                const int LENGTH_OF_SHA1_HASH = 20;
                const int SHA1_MAX_HEX_STRING_SIZE = 41;
                uint8_t hashedValue[LENGTH_OF_SHA1_HASH] = {0};

                const unsigned char* concatChar = reinterpret_cast<const unsigned char*>(toHash.c_str());
                const int length = toHash.length();

                CryptSha1Init(&SHAContext);
                CryptSha1Update(&SHAContext,concatChar,length);
                CryptSha1Final(&SHAContext,hashedValue,LENGTH_OF_SHA1_HASH);
                return BinaryToHexHashConverter(hashedValue,sizeof(hashedValue),SHA1_MAX_HEX_STRING_SIZE);
            }

            const eastl::string8 MD5Hash(eastl::string8 stringToHash)
            {
                const int SHA1_MAX_HEX_STRING_SIZE = 33;
                if(stringToHash.empty())
                    return stringToHash;
                CryptMD5T md5X;
                uint8_t data[16] = {0};
                CryptMD5Init(&md5X);
                CryptMD5Update(&md5X, stringToHash.c_str(), (int32_t)stringToHash.length());
                CryptMD5Final(&md5X, data, sizeof(data));
                return BinaryToHexHashConverter(data,sizeof(data),SHA1_MAX_HEX_STRING_SIZE);
            }

            const eastl::string8 encryptSHA1(uint64_t val)
            {
                return encryptSHA1(eastl::string8().append_sprintf("%I64u",val));
            }
            const eastl::string8 MD5Hash(uint64_t val)
            {
                return MD5Hash(eastl::string8().append_sprintf("%I64u",val));
            }

            const eastl::string8 printUint64AsHex(uint64_t u64)
            {
                eastl::string8 hexString;
                hexString.append_sprintf("%08X%08X", static_cast<uint32_t>((u64 >> 32) & 0xFFFFFFFF), static_cast<uint32_t>((u64)& 0xFFFFFFFF));
                return hexString;
            }

        }
    }

