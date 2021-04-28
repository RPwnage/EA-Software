///////////////////////////////////////////////////////////////////////////////
// FnEncode.h
//
// Copyright (c) 2007, Electronic Arts Inc. All rights reserved.
// Created by Alex Liberman and Talin.
//
// Character transcoding functions for filenames
///////////////////////////////////////////////////////////////////////////////


#ifndef EAIO_FNENCODE_H
#define EAIO_FNENCODE_H


#include <EAIO/internal/Config.h>
#include <EAIO/EAStream.h>      // for kLengthNull
#include <EAIO/PathString.h>


namespace EA
{
    namespace IO
    {
        EAIO_API uint32_t ConvertPath(Path::PathString8&  dstPath8,  const char16_t* pSrcPath16);
        EAIO_API uint32_t ConvertPath(Path::PathString16& dstPath16, const char8_t*  pSrcPath8);

        EAIO_API uint32_t ConvertPath(Path::PathString8&  dstPath8,  const char32_t* pSrcPath32);
        EAIO_API uint32_t ConvertPath(Path::PathString32& dstPath32, const char8_t*  pSrcPath8);

        EAIO_API uint32_t ConvertPath(Path::PathString16& dstPath16, const char32_t* pSrcPath32);
        EAIO_API uint32_t ConvertPath(Path::PathString32& dstPath32, const char16_t* pSrcPath16);

        #if EA_WCHAR_UNIQUE
            EAIO_API uint32_t ConvertPath(Path::PathString8& dstPath16, const wchar_t* pSrcPath32);
            EAIO_API uint32_t ConvertPath(Path::PathString16& dstPath16, const wchar_t* pSrcPath32);
            EAIO_API uint32_t ConvertPath(Path::PathString32& dstPath16, const wchar_t* pSrcPath32);

            EAIO_API uint32_t ConvertPath(Path::PathStringW& dstPath32, const char8_t* pSrcPath16);
            EAIO_API uint32_t ConvertPath(Path::PathStringW& dstPath32, const char16_t* pSrcPath16);
            EAIO_API uint32_t ConvertPath(Path::PathStringW& dstPath32, const char32_t* pSrcPath16);
        #endif

        EAIO_API bool StrEq(const char16_t* str1, const char16_t* str2);
        EAIO_API bool StrEq(const char32_t* str1, const char32_t* str2);

        #if EA_WCHAR_UNIQUE
            EAIO_API bool StrEq(const wchar_t* str1, const wchar_t* str2);
        #endif




        ///////////////////////////////////////////////////////////////
        // Deprecated -- Use the above functions and EAStdC instead. //
        ///////////////////////////////////////////////////////////////

        EAIO_API size_t   StrlcpyUTF16ToUTF8(char8_t* pDest, size_t nDestLength, const char16_t* pSrc, size_t nSrcLength = kLengthNull);
        EAIO_API size_t   StrlcpyUTF8ToUTF16(char16_t* pDest, size_t nDestLength, const char8_t* pSrc, size_t nSrcLength = kLengthNull);
        EAIO_API uint32_t ConvertPathUTF8ToUTF16(Path::PathString16& dstPath16, const char8_t* pSrcPath8);
        EAIO_API uint32_t ConvertPathUTF16ToUTF8(Path::PathString8& dstPath8, const char16_t* pSrcPath16);
        EAIO_API bool     StrEq16(const char16_t* str1, const char16_t* str2);
        EAIO_API size_t   EAIOStrlen8(const char8_t* str);
        EAIO_API size_t   EAIOStrlen16(const char16_t* str);
        EAIO_API size_t   EAIOStrlcpy8(char8_t* pDestination, const char8_t* pSource, size_t nDestCapacity);
        EAIO_API size_t   EAIOStrlcpy16(char16_t* pDestination, const char16_t* pSource, size_t nDestCapacity);
        EAIO_API size_t   EAIOStrlcat16(char16_t* pDestination, const char16_t* pSource, size_t nDestCapacity);

    } // namespace IO

} // namespace EA

#endif // EAIO_FNENCODE_H

