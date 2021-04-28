//
//  Utilities.h
//  
//
//  Created by Kiss, Bela on 12-06-07.
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.
//

#ifndef _Utilities_h
#define _Utilities_h

#include "EASTL/string.h"
#include <cstdio>
#include <string>
#include "GenericTelemetryElements.h"
#ifdef ORIGIN_PC 
#include <Windows.h>
#include <sstream>
#endif


namespace NonQTOriginServices
{
    namespace Utilities
    {
        enum util_crypto {
            MD5
            ,SHA1
        };

        FILE* openFile(eastl::wstring const& fileName, eastl::wstring const& permissions);
        void getExecutablePath(char16_t& executablePath, size_t executablePathLenInChars);
        void getIOLocation(const eastl::wstring& sDataPath, eastl::wstring& sFilePath);
        void escape(std::string &str);

        void FourChars_2_UINT32(char8_t * dst, const char8_t *src, size_t len);
        uint64_t calculateFNV1A(const char8_t *str);
        uint64_t calculateFNV1A(const char8_t *str, size_t size);
        void CalculcateIncrementalChecksum(const void *data, uint64_t dataSize, TelemetryCheckSum &crc);
        const eastl::string BinaryToHexHashConverter(uint8_t *pBinary, int32_t uLength, const int hashLength);
        const eastl::string8 encryptSHA1(const eastl::string8& toHash);
        const eastl::string8 MD5Hash(eastl::string8 stringToHash);
        const eastl::string8 encryptSHA1(uint64_t);
        const eastl::string8 MD5Hash(uint64_t);
        /// Helper for writing out a uint64 as a hex number.
        const eastl::string8 printUint64AsHex(uint64_t u64);

    }
}

#endif
