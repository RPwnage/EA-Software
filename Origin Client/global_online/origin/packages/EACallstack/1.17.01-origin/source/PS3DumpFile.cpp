///////////////////////////////////////////////////////////////////////////////
// PS3DumpFile.cpp
//
// Copyright (c) 2009 Electronic Arts Inc.
// Created by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#include <EACallstack/PS3DumpFile.h>
#include <EACallstack/Context.h>
#include <EACallstack/EADasm.h>
#include <EAIO/PathString.h>
#include <EAIO/EAStreamAdapter.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/FnEncode.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EAString.h>
#include <EAStdC/Int128_t.h>
#include EA_ASSERT_HEADER


namespace EA
{
namespace Callstack
{
namespace Local
{

#ifndef EAArrayCount
    #define EAArrayCount(x) (sizeof(x) / sizeof(x[0]))
#endif

uint16_t SwizzleUint16(uint16_t x)
{
    return (uint16_t) ((x >> 8) | (x << 8));
}

uint32_t SwizzleUint32(uint32_t x)
{
    // An alternative to the mechanism of using shifts and ors below
    // is to use byte addressing.
    return (uint32_t)
        ((x >> 24)               |
        ((x << 24) & 0xff000000) |
        ((x <<  8) & 0x00ff0000) |
        ((x >>  8) & 0x0000ff00)); 
}

uint64_t SwizzleUint64(uint64_t x)
{
    return (uint64_t)
        ((x        & 0x000000ff) << 56) |
        ((x >> 56) & 0x000000ff)        |
        ((x        & 0x0000ff00) << 40) |
        ((x >> 40) & 0x0000ff00)        |
        ((x        & 0x00ff0000) << 24) |
        ((x >> 24) & 0x00ff0000)        |
        ((x        & 0xff000000) <<  8) |
        ((x >>  8) & 0xff000000);
}


// The input may refer to the same memory as the output.
void GetFileNameOnly(const wchar_t* pStrWIn, FixedStringW& strWOut, bool bMakeLower)
{
    strWOut = pStrWIn;

    wchar_t* pFileNameW = EA::IO::Path::GetFileName(strWOut.c_str());
    strWOut.erase(strWOut.begin(), pFileNameW);

    wchar_t* pFileExtensionW = EA::IO::Path::GetFileExtension(strWOut.c_str());
    strWOut.erase(pFileExtensionW, strWOut.end());

    if(bMakeLower)
        strWOut.make_lower();
}


void GetFileNameOnly(const char8_t* pStr8In, FixedStringW& strWOut, bool bMakeLower)
{
    strWOut.resize((eastl_size_t)EA::StdC::Strlen(pStr8In));
    for(eastl_size_t j = 0, jEnd = strWOut.size(); j < jEnd; j++)
        strWOut[j] = (uint8_t)pStr8In[j];

    wchar_t* pFileNameW = EA::IO::Path::GetFileName(strWOut.c_str());
    strWOut.erase(strWOut.begin(), pFileNameW);

    wchar_t* pFileExtensionW = EA::IO::Path::GetFileExtension(strWOut.c_str());
    strWOut.erase(pFileExtensionW, strWOut.end());

    if(bMakeLower)
        strWOut.make_lower();
}


#if (EASTDC_VERSION_N >= 10505)
static int StreamStringWriter(const char8_t* EA_RESTRICT pString, size_t nStringLength, void* EA_RESTRICT pContext8, EA::StdC::WriteFunctionState)
#else
static int StreamStringWriter(const char8_t* EA_RESTRICT pString, size_t nStringLength, void* EA_RESTRICT pContext8)
#endif
{
    EA::IO::IStream* pStream = static_cast<EA::IO::IStream*>(pContext8);

    pStream->Write(pString, (EA::IO::size_type)nStringLength);

    return (int)(unsigned)nStringLength;
}

bool WriteStringFormatted(EA::IO::IStream* pOS, const char8_t* pFormat, ...)
{
    va_list arguments;

    va_start(arguments, pFormat);
    int result = EA::StdC::Vcprintf(StreamStringWriter, pOS, pFormat, arguments);
    va_end(arguments);

    return (result >= 0); 
}

// This function supplements the EAIO Read functions.
bool ReadChar8(EA::IO::IStream* pIS, char8_t* value, EA::IO::size_type count)
{
    return pIS->Read(value, count * sizeof(*value)) == (count * sizeof(*value));
}

// This function supplements the EAIO Read functions.
bool ReadUint128(EA::IO::IStream* pIS, EA::StdC::uint128_t& value, EA::IO::Endian endianSource = EA::IO::kEndianBig)
{
    bool     result;
    uint64_t low = 0, high = 0;

    if(endianSource == EA::IO::kEndianBig)
        result = ReadUint64(pIS, high, endianSource) &&
                 ReadUint64(pIS, low,  endianSource);
    else
        result = ReadUint64(pIS, low,  endianSource) &&
                 ReadUint64(pIS, high, endianSource);

    value.SetPartUint64(0, low);
    value.SetPartUint64(1, high);

    return result;
}

// This function supplements the EAIO Read functions.
bool ReadVMXReg(EA::IO::IStream* pIS, VMXRegister& value, EA::IO::Endian endianSource = EA::IO::kEndianBig)
{
    if(endianSource == EA::IO::kEndianBig)
        return ReadUint64(pIS, value.mDword[1], endianSource) &&
               ReadUint64(pIS, value.mDword[0], endianSource);
    else
        return ReadUint64(pIS, value.mDword[0], endianSource) &&
               ReadUint64(pIS, value.mDword[1], endianSource);
}

// This function supplements the EAIO Read functions.
bool ReadVMXReg(EA::IO::IStream* pIS, VMXRegister* value, EA::IO::size_type count, EA::IO::Endian endianSource = EA::IO::kEndianBig)
{
    if(endianSource == EA::IO::kEndianLocal)
        return pIS->Read(value, count * sizeof(*value)) == (count * sizeof(*value));
    else
    {
        for(EA::IO::size_type i = 0; i < count; i++)
        {
            if(!ReadVMXReg(pIS, value[i], endianSource))
                return false;
        }
    }

    return true;
}

// This function supplements the EAIO Read functions.
bool ReadSPUReg(EA::IO::IStream* pIS, SPURegister& value, EA::IO::Endian endianSource = EA::IO::kEndianBig)
{
    if(endianSource == EA::IO::kEndianBig)
        return ReadUint64(pIS, value.mDword[1], endianSource) &&
               ReadUint64(pIS, value.mDword[0], endianSource);
    else
        return ReadUint64(pIS, value.mDword[0], endianSource) &&
               ReadUint64(pIS, value.mDword[1], endianSource);
}

// This function supplements the EAIO Read functions.
bool ReadSPUReg(EA::IO::IStream* pIS, SPURegister* value, EA::IO::size_type count, EA::IO::Endian endianSource = EA::IO::kEndianBig)
{
    if(endianSource == EA::IO::kEndianLocal)
        return pIS->Read(value, count * sizeof(*value)) == (count * sizeof(*value));
    else
    {
        for(EA::IO::size_type i = 0; i < count; i++)
        {
            if(!ReadSPUReg(pIS, value[i], endianSource))
                return false;
        }
    }

    return true;
}

// This function supplements the EAIO Read functions.
bool ReadBucket(EA::IO::IStream* pIS, EA::IO::size_type count)
{
    return pIS->SetPosition((EA::IO::off_type)(pIS->GetPosition() + count));
}


class StringStream : public EA::IO::IStream
{
public:
    typedef eastl::string String;
    String* mpString;

    StringStream(String* pString) { mpString = pString; }

    virtual int                 AddRef() { return 0; }
    virtual int                 Release() { return 0; }
    virtual uint32_t            GetType() const { return 0; }
    virtual int                 GetAccessFlags() const { return 0; }
    virtual int                 GetState() const { return 0; }
    virtual bool                Close() { return 0; }
    virtual EA::IO::size_type   GetSize() const { return 0; }
    virtual bool                SetSize(EA::IO::size_type) { return false; }
    virtual EA::IO::off_type    GetPosition(EA::IO::PositionType) const { return 0; }
    virtual bool                SetPosition(EA::IO::off_type, EA::IO::PositionType) { return 0; }
    virtual EA::IO::size_type   GetAvailable() const { return 0; }
    virtual EA::IO::size_type   Read(void*, EA::IO::size_type) { return 0; }
    virtual bool                Flush() { return false; }

    virtual bool Write(const void* pData, EA::IO::size_type nSize)
    {
        mpString->append(static_cast<const char8_t*>(pData), (eastl_size_t)nSize);
        return true;
    }
};


// The following is a copy from the PS3's gcm_enum.h

enum CellGcmEnum{
    //    Enable
    CELL_GCM_FALSE   = (0),
    CELL_GCM_TRUE    = (1),

    // Location
    CELL_GCM_LOCATION_LOCAL  = (0),
    CELL_GCM_LOCATION_MAIN   = (1),


    // SetSurface
    CELL_GCM_SURFACE_X1R5G5B5_Z1R5G5B5    = (1),
    CELL_GCM_SURFACE_X1R5G5B5_O1R5G5B5    = (2),
    CELL_GCM_SURFACE_R5G6B5               = (3),
    CELL_GCM_SURFACE_X8R8G8B8_Z8R8G8B8    = (4),
    CELL_GCM_SURFACE_X8R8G8B8_O8R8G8B8    = (5),
    CELL_GCM_SURFACE_A8R8G8B8             = (8),
    CELL_GCM_SURFACE_B8                   = (9),
    CELL_GCM_SURFACE_G8B8                 = (10),
    CELL_GCM_SURFACE_F_W16Z16Y16X16       = (11),
    CELL_GCM_SURFACE_F_W32Z32Y32X32       = (12),
    CELL_GCM_SURFACE_F_X32                = (13),
    CELL_GCM_SURFACE_X8B8G8R8_Z8B8G8R8    = (14),
    CELL_GCM_SURFACE_X8B8G8R8_O8B8G8R8    = (15),
    CELL_GCM_SURFACE_A8B8G8R8             = (16),

    CELL_GCM_SURFACE_Z16                  = (1),
    CELL_GCM_SURFACE_Z24S8                = (2),

    CELL_GCM_SURFACE_PITCH                = (1),
    CELL_GCM_SURFACE_SWIZZLE              = (2),

    CELL_GCM_SURFACE_CENTER_1             = (0),
    CELL_GCM_SURFACE_DIAGONAL_CENTERED_2  = (3),
    CELL_GCM_SURFACE_SQUARE_CENTERED_4    = (4),
    CELL_GCM_SURFACE_SQUARE_ROTATED_4     = (5),

    CELL_GCM_SURFACE_TARGET_NONE          = (0),
    CELL_GCM_SURFACE_TARGET_0             = (1),
    CELL_GCM_SURFACE_TARGET_1             = (2),
    CELL_GCM_SURFACE_TARGET_MRT1          = (0x13),
    CELL_GCM_SURFACE_TARGET_MRT2          = (0x17),
    CELL_GCM_SURFACE_TARGET_MRT3          = (0x1f),

    // SetSurfaceWindow
    CELL_GCM_WINDOW_ORIGIN_TOP            = (0),
    CELL_GCM_WINDOW_ORIGIN_BOTTOM         = (1),

    CELL_GCM_WINDOW_PIXEL_CENTER_HALF     = (0),
    CELL_GCM_WINDOW_PIXEL_CENTER_INTEGER  = (1),

    // SetClearSurface
    CELL_GCM_CLEAR_Z    = (1<<0),
    CELL_GCM_CLEAR_S    = (1<<1),
    CELL_GCM_CLEAR_R    = (1<<4),
    CELL_GCM_CLEAR_G    = (1<<5),
    CELL_GCM_CLEAR_B    = (1<<6),
    CELL_GCM_CLEAR_A    = (1<<7),
    CELL_GCM_CLEAR_M    = (0xf3),

    // SetVertexDataArray
    CELL_GCM_VERTEX_S1       = (1),
    CELL_GCM_VERTEX_F        = (2),
    CELL_GCM_VERTEX_SF       = (3),
    CELL_GCM_VERTEX_UB       = (4),
    CELL_GCM_VERTEX_S32K     = (5),
    CELL_GCM_VERTEX_CMP      = (6),
    CELL_GCM_VERTEX_UB256    = (7),

    CELL_GCM_VERTEX_S16_NR                   = (1),
    CELL_GCM_VERTEX_F32                      = (2),
    CELL_GCM_VERTEX_F16                      = (3),
    CELL_GCM_VERTEX_U8_NR                    = (4),
    CELL_GCM_VERTEX_S16_UN                   = (5),
    CELL_GCM_VERTEX_S11_11_10_NR             = (6),
    CELL_GCM_VERTEX_U8_UN                    = (7),

    // SetTexture
    CELL_GCM_TEXTURE_B8                      = (0x81),
    CELL_GCM_TEXTURE_A1R5G5B5                = (0x82),
    CELL_GCM_TEXTURE_A4R4G4B4                = (0x83),
    CELL_GCM_TEXTURE_R5G6B5                  = (0x84),
    CELL_GCM_TEXTURE_A8R8G8B8                = (0x85),
    CELL_GCM_TEXTURE_COMPRESSED_DXT1         = (0x86),
    CELL_GCM_TEXTURE_COMPRESSED_DXT23        = (0x87),
    CELL_GCM_TEXTURE_COMPRESSED_DXT45        = (0x88),
    CELL_GCM_TEXTURE_G8B8                    = (0x8B),
    CELL_GCM_TEXTURE_R6G5B5                  = (0x8F),
    CELL_GCM_TEXTURE_DEPTH24_D8              = (0x90),
    CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT        = (0x91),
    CELL_GCM_TEXTURE_DEPTH16                 = (0x92),
    CELL_GCM_TEXTURE_DEPTH16_FLOAT           = (0x93),
    CELL_GCM_TEXTURE_X16                     = (0x94),
    CELL_GCM_TEXTURE_Y16_X16                 = (0x95),
    CELL_GCM_TEXTURE_R5G5B5A1                = (0x97),
    CELL_GCM_TEXTURE_COMPRESSED_HILO8        = (0x98),
    CELL_GCM_TEXTURE_COMPRESSED_HILO_S8      = (0x99),
    CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT   = (0x9A),
    CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT   = (0x9B),
    CELL_GCM_TEXTURE_X32_FLOAT               = (0x9C),
    CELL_GCM_TEXTURE_D1R5G5B5                = (0x9D),
    CELL_GCM_TEXTURE_D8R8G8B8                = (0x9E),
    CELL_GCM_TEXTURE_Y16_X16_FLOAT           = (0x9F),
    CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8    = (0xAD),
    CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8    = (0xAE),

    CELL_GCM_TEXTURE_SZ    = (0x00),
    CELL_GCM_TEXTURE_LN    = (0x20),
    CELL_GCM_TEXTURE_NR    = (0x00),
    CELL_GCM_TEXTURE_UN    = (0x40),

    CELL_GCM_TEXTURE_DIMENSION_1        = (1),
    CELL_GCM_TEXTURE_DIMENSION_2        = (2),
    CELL_GCM_TEXTURE_DIMENSION_3        = (3),

    CELL_GCM_TEXTURE_REMAP_ORDER_XYXY    = (0),
    CELL_GCM_TEXTURE_REMAP_ORDER_XXXY    = (1),
    CELL_GCM_TEXTURE_REMAP_FROM_A        = (0),
    CELL_GCM_TEXTURE_REMAP_FROM_R        = (1),
    CELL_GCM_TEXTURE_REMAP_FROM_G        = (2),
    CELL_GCM_TEXTURE_REMAP_FROM_B        = (3),
    CELL_GCM_TEXTURE_REMAP_ZERO          = (0),
    CELL_GCM_TEXTURE_REMAP_ONE           = (1),
    CELL_GCM_TEXTURE_REMAP_REMAP         = (2),
    
    CELL_GCM_TEXTURE_BORDER_TEXTURE      = (0),
    CELL_GCM_TEXTURE_BORDER_COLOR        = (1),

    // SetTextureFilter
    CELL_GCM_TEXTURE_NEAREST                     = (1),
    CELL_GCM_TEXTURE_LINEAR                      = (2),
    CELL_GCM_TEXTURE_NEAREST_NEAREST             = (3),
    CELL_GCM_TEXTURE_LINEAR_NEAREST              = (4),
    CELL_GCM_TEXTURE_NEAREST_LINEAR              = (5),
    CELL_GCM_TEXTURE_LINEAR_LINEAR               = (6),
    CELL_GCM_TEXTURE_CONVOLUTION_MIN             = (7),
    CELL_GCM_TEXTURE_CONVOLUTION_MAG             = (4),
    CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX        = (1),
    CELL_GCM_TEXTURE_CONVOLUTION_GAUSSIAN        = (2),
    CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX_ALT    = (3),

    // SetTextureAddress
    CELL_GCM_TEXTURE_WRAP                         = (1),
    CELL_GCM_TEXTURE_MIRROR                       = (2),
    CELL_GCM_TEXTURE_CLAMP_TO_EDGE                = (3),
    CELL_GCM_TEXTURE_BORDER                       = (4),
    CELL_GCM_TEXTURE_CLAMP                        = (5),
    CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP_TO_EDGE    = (6),
    CELL_GCM_TEXTURE_MIRROR_ONCE_BORDER           = (7),
    CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP            = (8),

    CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL  = (0),
    CELL_GCM_TEXTURE_UNSIGNED_REMAP_BIASED  = (1),

    CELL_GCM_TEXTURE_SIGNED_REMAP_NORMAL    = (0x0),
    CELL_GCM_TEXTURE_SIGNED_REMAP_CLAMPED   = (0x3),

    CELL_GCM_TEXTURE_ZFUNC_NEVER     = (0),
    CELL_GCM_TEXTURE_ZFUNC_LESS      = (1),
    CELL_GCM_TEXTURE_ZFUNC_EQUAL     = (2),
    CELL_GCM_TEXTURE_ZFUNC_LEQUAL    = (3),
    CELL_GCM_TEXTURE_ZFUNC_GREATER   = (4),
    CELL_GCM_TEXTURE_ZFUNC_NOTEQUAL  = (5),
    CELL_GCM_TEXTURE_ZFUNC_GEQUAL    = (6),
    CELL_GCM_TEXTURE_ZFUNC_ALWAYS    = (7),

    CELL_GCM_TEXTURE_GAMMA_R    = (1<<0),
    CELL_GCM_TEXTURE_GAMMA_G    = (1<<1),
    CELL_GCM_TEXTURE_GAMMA_B    = (1<<2),
    CELL_GCM_TEXTURE_GAMMA_A    = (1<<3),

    // SetAnisoSpread
    CELL_GCM_TEXTURE_ANISO_SPREAD_0_50_TEXEL  = (0x0),
    CELL_GCM_TEXTURE_ANISO_SPREAD_1_00_TEXEL  = (0x1),
    CELL_GCM_TEXTURE_ANISO_SPREAD_1_125_TEXEL = (0x2),
    CELL_GCM_TEXTURE_ANISO_SPREAD_1_25_TEXEL  = (0x3),
    CELL_GCM_TEXTURE_ANISO_SPREAD_1_375_TEXEL = (0x4),
    CELL_GCM_TEXTURE_ANISO_SPREAD_1_50_TEXEL  = (0x5),
    CELL_GCM_TEXTURE_ANISO_SPREAD_1_75_TEXEL  = (0x6),
    CELL_GCM_TEXTURE_ANISO_SPREAD_2_00_TEXEL  = (0x7),

    // SetCylindricalWrap
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX0_U  = (1 << 0),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX0_V  = (1 << 1),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX0_P  = (1 << 2),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX0_Q  = (1 << 3),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX1_U  = (1 << 4),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX1_V  = (1 << 5),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX1_P  = (1 << 6),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX1_Q  = (1 << 7),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX2_U  = (1 << 8),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX2_V  = (1 << 9),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX2_P  = (1 << 10),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX2_Q  = (1 << 11),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX3_U  = (1 << 12),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX3_V  = (1 << 13),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX3_P  = (1 << 14),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX3_Q  = (1 << 15),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX4_U  = (1 << 16),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX4_V  = (1 << 17),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX4_P  = (1 << 18),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX4_Q  = (1 << 19),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX5_U  = (1 << 20),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX5_V  = (1 << 21),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX5_P  = (1 << 22),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX5_Q  = (1 << 23),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX6_U  = (1 << 24),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX6_V  = (1 << 25),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX6_P  = (1 << 26),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX6_Q  = (1 << 27),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX7_U  = (1 << 28),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX7_V  = (1 << 29),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX7_P  = (1 << 30),
    CELL_GCM_TEXTURE_CYLINDRICAL_WRAP_ENABLE_TEX7_Q  = (1 << 31),

    // SetTextureControl
    CELL_GCM_TEXTURE_MAX_ANISO_1    = (0),
    CELL_GCM_TEXTURE_MAX_ANISO_2    = (1),
    CELL_GCM_TEXTURE_MAX_ANISO_4    = (2),
    CELL_GCM_TEXTURE_MAX_ANISO_6    = (3),
    CELL_GCM_TEXTURE_MAX_ANISO_8    = (4),
    CELL_GCM_TEXTURE_MAX_ANISO_10    = (5),
    CELL_GCM_TEXTURE_MAX_ANISO_12    = (6),
    CELL_GCM_TEXTURE_MAX_ANISO_16    = (7),

    // SetDrawArrays, SetDrawIndexArray
    CELL_GCM_PRIMITIVE_POINTS          = (1),
    CELL_GCM_PRIMITIVE_LINES           = (2),
    CELL_GCM_PRIMITIVE_LINE_LOOP       = (3),
    CELL_GCM_PRIMITIVE_LINE_STRIP      = (4),
    CELL_GCM_PRIMITIVE_TRIANGLES       = (5),
    CELL_GCM_PRIMITIVE_TRIANGLE_STRIP  = (6),
    CELL_GCM_PRIMITIVE_TRIANGLE_FAN    = (7),
    CELL_GCM_PRIMITIVE_QUADS           = (8),
    CELL_GCM_PRIMITIVE_QUAD_STRIP      = (9),
    CELL_GCM_PRIMITIVE_POLYGON         = (10),

    // SetColorMask
    CELL_GCM_COLOR_MASK_B    = (1<<0),
    CELL_GCM_COLOR_MASK_G    = (1<<8),
    CELL_GCM_COLOR_MASK_R    = (1<<16),
    CELL_GCM_COLOR_MASK_A    = (1<<24),

    // SetColorMaskMrt
    CELL_GCM_COLOR_MASK_MRT1_A    = (1<<4),
    CELL_GCM_COLOR_MASK_MRT1_R    = (1<<5),
    CELL_GCM_COLOR_MASK_MRT1_G    = (1<<6),
    CELL_GCM_COLOR_MASK_MRT1_B    = (1<<7),
    CELL_GCM_COLOR_MASK_MRT2_A    = (1<<8),
    CELL_GCM_COLOR_MASK_MRT2_R    = (1<<9),
    CELL_GCM_COLOR_MASK_MRT2_G    = (1<<10),
    CELL_GCM_COLOR_MASK_MRT2_B    = (1<<11),
    CELL_GCM_COLOR_MASK_MRT3_A    = (1<<12),
    CELL_GCM_COLOR_MASK_MRT3_R    = (1<<13),
    CELL_GCM_COLOR_MASK_MRT3_G    = (1<<14),
    CELL_GCM_COLOR_MASK_MRT3_B    = (1<<15),

    // SetAlphaFunc, DepthFunc, StencilFunc
    CELL_GCM_NEVER      = (0x0200),
    CELL_GCM_LESS       = (0x0201),
    CELL_GCM_EQUAL      = (0x0202),
    CELL_GCM_LEQUAL     = (0x0203),
    CELL_GCM_GREATER    = (0x0204),
    CELL_GCM_NOTEQUAL   = (0x0205),
    CELL_GCM_GEQUAL     = (0x0206),
    CELL_GCM_ALWAYS     = (0x0207),

    // SetBlendFunc
    CELL_GCM_ZERO                        = (0),
    CELL_GCM_ONE                         = (1),
    CELL_GCM_SRC_COLOR                   = (0x0300),
    CELL_GCM_ONE_MINUS_SRC_COLOR         = (0x0301),
    CELL_GCM_SRC_ALPHA                   = (0x0302),
    CELL_GCM_ONE_MINUS_SRC_ALPHA         = (0x0303),
    CELL_GCM_DST_ALPHA                   = (0x0304),
    CELL_GCM_ONE_MINUS_DST_ALPHA         = (0x0305),
    CELL_GCM_DST_COLOR                   = (0x0306),
    CELL_GCM_ONE_MINUS_DST_COLOR         = (0x0307),
    CELL_GCM_SRC_ALPHA_SATURATE          = (0x0308),
    CELL_GCM_CONSTANT_COLOR              = (0x8001),
    CELL_GCM_ONE_MINUS_CONSTANT_COLOR    = (0x8002),
    CELL_GCM_CONSTANT_ALPHA              = (0x8003),
    CELL_GCM_ONE_MINUS_CONSTANT_ALPHA    = (0x8004),

    // SetBlendEquation
    CELL_GCM_FUNC_ADD                       = (0x8006),
    CELL_GCM_MIN                            = (0x8007),
    CELL_GCM_MAX                            = (0x8008),
    CELL_GCM_FUNC_SUBTRACT                  = (0x800A),
    CELL_GCM_FUNC_REVERSE_SUBTRACT          = (0x800B),
    CELL_GCM_FUNC_REVERSE_SUBTRACT_SIGNED   = (0x0000F005),
    CELL_GCM_FUNC_ADD_SIGNED                = (0x0000F006),
    CELL_GCM_FUNC_REVERSE_ADD_SIGNED        = (0x0000F007),

    // SetCullFace
    CELL_GCM_FRONT              = (0x0404),
    CELL_GCM_BACK               = (0x0405),
    CELL_GCM_FRONT_AND_BACK     = (0x0408),

    // SetShadeMode
    CELL_GCM_FLAT    = (0x1D00),
    CELL_GCM_SMOOTH    = (0x1D01),

    // SetFrontFace
    CELL_GCM_CW        = (0x0900),
    CELL_GCM_CCW    = (0x0901),

    // SetLogicOp
    CELL_GCM_CLEAR          = (0x1500),
    CELL_GCM_AND            = (0x1501),
    CELL_GCM_AND_REVERSE    = (0x1502),
    CELL_GCM_COPY           = (0x1503),
    CELL_GCM_AND_INVERTED   = (0x1504),
    CELL_GCM_NOOP           = (0x1505),
    CELL_GCM_XOR            = (0x1506),
    CELL_GCM_OR             = (0x1507),
    CELL_GCM_NOR            = (0x1508),
    CELL_GCM_EQUIV          = (0x1509),
    CELL_GCM_INVERT         = (0x150A),
    CELL_GCM_OR_REVERSE     = (0x150B),
    CELL_GCM_COPY_INVERTED  = (0x150C),
    CELL_GCM_OR_INVERTED    = (0x150D),
    CELL_GCM_NAND           = (0x150E),
    CELL_GCM_SET            = (0x150F),

    // SetStencilOp
    CELL_GCM_KEEP        = (0x1E00),
    CELL_GCM_REPLACE     = (0x1E01),
    CELL_GCM_INCR        = (0x1E02),
    CELL_GCM_DECR        = (0x1E03),
    CELL_GCM_INCR_WRAP   = (0x8507),
    CELL_GCM_DECR_WRAP   = (0x8508),

    // SetDrawIndexArray
    CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32    = (0),
    CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16    = (1),

    // SetTransfer
    CELL_GCM_TRANSFER_LOCAL_TO_LOCAL    = (0),
    CELL_GCM_TRANSFER_MAIN_TO_LOCAL     = (1),
    CELL_GCM_TRANSFER_LOCAL_TO_MAIN     = (2),
    CELL_GCM_TRANSFER_MAIN_TO_MAIN      = (3),

    // SetInvalidateTextureCache
    CELL_GCM_INVALIDATE_TEXTURE         = (1),
    CELL_GCM_INVALIDATE_VERTEX_TEXTURE   = (2),

    // SetFrequencyDividerOperation
    CELL_GCM_FREQUENCY_MODULO    = (1), 
    CELL_GCM_FREQUENCY_DIVIDE    = (0),

    // SetTile, SetZCull
    CELL_GCM_COMPMODE_DISABLED                  = (0),
    CELL_GCM_COMPMODE_C32_2X1                   = (7),
    CELL_GCM_COMPMODE_C32_2X2                   = (8),
    CELL_GCM_COMPMODE_Z32_SEPSTENCIL            = (9),
    CELL_GCM_COMPMODE_Z32_SEPSTENCIL_REG        = (10),
    CELL_GCM_COMPMODE_Z32_SEPSTENCIL_REGULAR    = (10),
    CELL_GCM_COMPMODE_Z32_SEPSTENCIL_DIAGONAL   = (11),
    CELL_GCM_COMPMODE_Z32_SEPSTENCIL_ROTATED    = (12),

    // SetZcull
    CELL_GCM_ZCULL_Z16        = (1),
    CELL_GCM_ZCULL_Z24S8      = (2),
    CELL_GCM_ZCULL_MSB        = (0),
    CELL_GCM_ZCULL_LONES      = (1),
    CELL_GCM_ZCULL_LESS       = (0),
    CELL_GCM_ZCULL_GREATER    = (1),

    CELL_GCM_SCULL_SFUNC_NEVER          = (0),
    CELL_GCM_SCULL_SFUNC_LESS           = (1),
    CELL_GCM_SCULL_SFUNC_EQUAL          = (2),
    CELL_GCM_SCULL_SFUNC_LEQUAL         = (3),
    CELL_GCM_SCULL_SFUNC_GREATER        = (4),
    CELL_GCM_SCULL_SFUNC_NOTEQUAL       = (5),
    CELL_GCM_SCULL_SFUNC_GEQUAL         = (6),
    CELL_GCM_SCULL_SFUNC_ALWAYS         = (7),

    // flip mode
    CELL_GCM_DISPLAY_HSYNC               = (1),
    CELL_GCM_DISPLAY_VSYNC               = (2),
    CELL_GCM_DISPLAY_HSYNC_WITH_NOISE    = (3),

    // vsync frequency
    CELL_GCM_DISPLAY_FREQUENCY_59_94HZ =(1),
    CELL_GCM_DISPLAY_FREQUENCY_SCANOUT =(2),
    CELL_GCM_DISPLAY_FREQUENCY_DISABLE =(3),

    CELL_GCM_TYPE_B      = (1),
    CELL_GCM_TYPE_C      = (2),
    CELL_GCM_TYPE_RSX    = (3),

    // MRT
    CELL_GCM_MRT_MAXCOUNT  = (4),

    // max display id
    CELL_GCM_DISPLAY_MAXID = (8),

    // Debug output level
    CELL_GCM_DEBUG_LEVEL0   = (0),
    CELL_GCM_DEBUG_LEVEL1   = (1),
    CELL_GCM_DEBUG_LEVEL2   = (2),

    // SetRenderEnable
    CELL_GCM_CONDITIONAL    = (2),

    // SetClearReport, SetReport, GetReport
    CELL_GCM_ZPASS_PIXEL_CNT = (1),
    CELL_GCM_ZCULL_STATS     = (2),
    CELL_GCM_ZCULL_STATS1    = (3),
    CELL_GCM_ZCULL_STATS2    = (4),
    CELL_GCM_ZCULL_STATS3    = (5),

    // SetPointSpriteControl
    CELL_GCM_POINT_SPRITE_RMODE_ZERO       = (0),
    CELL_GCM_POINT_SPRITE_RMODE_FROM_R     = (1),
    CELL_GCM_POINT_SPRITE_RMODE_FROM_S     = (2),

    CELL_GCM_POINT_SPRITE_TEX0             = (1<<8),
    CELL_GCM_POINT_SPRITE_TEX1             = (1<<9),
    CELL_GCM_POINT_SPRITE_TEX2             = (1<<10),
    CELL_GCM_POINT_SPRITE_TEX3             = (1<<11),
    CELL_GCM_POINT_SPRITE_TEX4             = (1<<12),
    CELL_GCM_POINT_SPRITE_TEX5             = (1<<13),
    CELL_GCM_POINT_SPRITE_TEX6             = (1<<14),
    CELL_GCM_POINT_SPRITE_TEX7             = (1<<15),
    CELL_GCM_POINT_SPRITE_TEX8             = (1<<16),
    CELL_GCM_POINT_SPRITE_TEX9             = (1<<17),

    // SetUserClipPlaneControl
    CELL_GCM_USER_CLIP_PLANE_DISABLE       = (0),
    CELL_GCM_USER_CLIP_PLANE_ENABLE_LT     = (1),
    CELL_GCM_USER_CLIP_PLANE_ENABLE_GE     = (2),

    // SetAttribOutputMask
    CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTDIFFUSE    = (1<< 0),
    CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTSPECULAR   = (1<< 1),
    CELL_GCM_ATTRIB_OUTPUT_MASK_BACKDIFFUSE     = (1<< 2),
    CELL_GCM_ATTRIB_OUTPUT_MASK_BACKSPECULAR    = (1<< 3),
    CELL_GCM_ATTRIB_OUTPUT_MASK_FOG             = (1<< 4),
    CELL_GCM_ATTRIB_OUTPUT_MASK_POINTSIZE       = (1<< 5),
    CELL_GCM_ATTRIB_OUTPUT_MASK_UC0             = (1<< 6),
    CELL_GCM_ATTRIB_OUTPUT_MASK_UC1             = (1<< 7),
    CELL_GCM_ATTRIB_OUTPUT_MASK_UC2             = (1<< 8),
    CELL_GCM_ATTRIB_OUTPUT_MASK_UC3             = (1<< 9),
    CELL_GCM_ATTRIB_OUTPUT_MASK_UC4             = (1<<10),
    CELL_GCM_ATTRIB_OUTPUT_MASK_UC5             = (1<<11),
    CELL_GCM_ATTRIB_OUTPUT_MASK_TEX8            = (1<<12),
    CELL_GCM_ATTRIB_OUTPUT_MASK_TEX9            = (1<<13),
    CELL_GCM_ATTRIB_OUTPUT_MASK_TEX0            = (1<<14),
    CELL_GCM_ATTRIB_OUTPUT_MASK_TEX1            = (1<<15),
    CELL_GCM_ATTRIB_OUTPUT_MASK_TEX2            = (1<<16),
    CELL_GCM_ATTRIB_OUTPUT_MASK_TEX3            = (1<<17),
    CELL_GCM_ATTRIB_OUTPUT_MASK_TEX4            = (1<<18),
    CELL_GCM_ATTRIB_OUTPUT_MASK_TEX5            = (1<<19),
    CELL_GCM_ATTRIB_OUTPUT_MASK_TEX6            = (1<<20),
    CELL_GCM_ATTRIB_OUTPUT_MASK_TEX7            = (1<<21),

    // SetFogMode
    CELL_GCM_FOG_MODE_LINEAR        = (0x2601),
    CELL_GCM_FOG_MODE_EXP           = (0x0800),
    CELL_GCM_FOG_MODE_EXP2          = (0x0801),
    CELL_GCM_FOG_MODE_EXP_ABS       = (0x0802),
    CELL_GCM_FOG_MODE_EXP2_ABS      = (0x0803),
    CELL_GCM_FOG_MODE_LINEAR_ABS    = (0x0804),

    // SetTextureOptimization
    CELL_GCM_TEXTURE_ISO_LOW    = (0),
    CELL_GCM_TEXTURE_ISO_HIGH   = (1),
    CELL_GCM_TEXTURE_ANISO_LOW  = (0),
    CELL_GCM_TEXTURE_ANISO_HIGH = (1),

    // SetDepthFormat
    CELL_GCM_DEPTH_FORMAT_FIXED  = (0),
    CELL_GCM_DEPTH_FORMAT_FLOAT  = (1),

    // SetFrontPolygonMode, SetBackPolygonMode
    CELL_GCM_POLYGON_MODE_POINT   = (0x1B00),
    CELL_GCM_POLYGON_MODE_LINE    = (0x1B01),
    CELL_GCM_POLYGON_MODE_FILL    = (0x1B02),

    // CellGcmTransferScale
    CELL_GCM_TRANSFER_SURFACE        = (0),
    CELL_GCM_TRANSFER_SWIZZLE        = (1),

    CELL_GCM_TRANSFER_CONVERSION_DITHER              = (0),
    CELL_GCM_TRANSFER_CONVERSION_TRUNCATE            = (1),
    CELL_GCM_TRANSFER_CONVERSION_SUBTRACT_TRUNCATE   = (2),

    CELL_GCM_TRANSFER_SCALE_FORMAT_A1R5G5B5          = (1),
    CELL_GCM_TRANSFER_SCALE_FORMAT_X1R5G5B5          = (2),
    CELL_GCM_TRANSFER_SCALE_FORMAT_A8R8G8B8          = (3),
    CELL_GCM_TRANSFER_SCALE_FORMAT_X8R8G8B8          = (4),
    CELL_GCM_TRANSFER_SCALE_FORMAT_CR8YB8CB8YA8      = (5),
    CELL_GCM_TRANSFER_SCALE_FORMAT_YB8CR8YA8CB8      = (6),
    CELL_GCM_TRANSFER_SCALE_FORMAT_R5G6B5            = (7),
    CELL_GCM_TRANSFER_SCALE_FORMAT_Y8                = (8),
    CELL_GCM_TRANSFER_SCALE_FORMAT_AY8               = (9),
    CELL_GCM_TRANSFER_SCALE_FORMAT_EYB8ECR8EYA8ECB8  = (0xA),
    CELL_GCM_TRANSFER_SCALE_FORMAT_ECR8EYB8ECB8EYA8  = (0xB),
    CELL_GCM_TRANSFER_SCALE_FORMAT_A8B8G8R8          = (0xC),
    CELL_GCM_TRANSFER_SCALE_FORMAT_X8B8G8R8          = (0xD),

    CELL_GCM_TRANSFER_OPERATION_SRCCOPY_AND      = (0),
    CELL_GCM_TRANSFER_OPERATION_ROP_AND          = (1),
    CELL_GCM_TRANSFER_OPERATION_BLEND_AND        = (2),
    CELL_GCM_TRANSFER_OPERATION_SRCCOPY          = (3),
    CELL_GCM_TRANSFER_OPERATION_SRCCOPY_PREMULT  = (4),
    CELL_GCM_TRANSFER_OPERATION_BLEND_PREMULT    = (5),

    CELL_GCM_TRANSFER_ORIGIN_CENTER    = (1),
    CELL_GCM_TRANSFER_ORIGIN_CORNER    = (2),

    CELL_GCM_TRANSFER_INTERPOLATOR_ZOH    = (0),
    CELL_GCM_TRANSFER_INTERPOLATOR_FOH    = (1),

    // CellGcmTransferSurface, CellGcmTransferSwizzle
    CELL_GCM_TRANSFER_SURFACE_FORMAT_R5G6B5     = (4),
    CELL_GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8   = (0xA),
    CELL_GCM_TRANSFER_SURFACE_FORMAT_Y32        = (0xB),

    // SetFragmentProgramLoad
    CELL_GCM_SHIFT_SET_SHADER_CONTROL_CONTROL_TXP  = (15),
    CELL_GCM_MASK_SET_SHADER_CONTROL_CONTROL_TXP   = (0x00008000),

    // MapEaIoAddressWithFlags
    CELL_GCM_IOMAP_FLAG_STRICT_ORDERING = (1<<1),

    // label
    CELL_GCM_INDEX_RANGE_LABEL_MIN      = 64,
    CELL_GCM_INDEX_RANGE_LABEL_MAX      = 255,
    CELL_GCM_INDEX_RANGE_LABEL_COUNT    = (256-64),

    // notify
    CELL_GCM_INDEX_RANGE_NOTIFY_MAIN_MIN   =    0,
    CELL_GCM_INDEX_RANGE_NOTIFY_MAIN_MAX   =  255,
    CELL_GCM_INDEX_RANGE_NOTIFY_MAIN_COUNT =  256,

    // report
    CELL_GCM_INDEX_RANGE_REPORT_MAIN_MIN    = 0,
    CELL_GCM_INDEX_RANGE_REPORT_MAIN_MAX    = (1024*1024-1),
    CELL_GCM_INDEX_RANGE_REPORT_MAIN_COUNT  = (1024*1024),
    CELL_GCM_INDEX_RANGE_REPORT_LOCAL_MIN   = 0,
    CELL_GCM_INDEX_RANGE_REPORT_LOCAL_MAX   = 2047,
    CELL_GCM_INDEX_RANGE_REPORT_LOCAL_COUNT = 2048,

    // tile
    CELL_GCM_INDEX_RANGE_TILE_MIN   = 0,
    CELL_GCM_INDEX_RANGE_TILE_MAX   = 14,
    CELL_GCM_INDEX_RANGE_TILE_COUNT = 15,

    // zcull
    CELL_GCM_INDEX_RANGE_ZCULL_MIN   = 0,
    CELL_GCM_INDEX_RANGE_ZCULL_MAX   = 7,
    CELL_GCM_INDEX_RANGE_ZCULL_COUNT = 8,

    // field
    CELL_GCM_DISPLAY_FIELD_TOP     = 1,
    CELL_GCM_DISPLAY_FIELD_BOTTOM  = 0,

    // flip status
    CELL_GCM_DISPLAY_FLIP_STATUS_DONE    = 0,
    CELL_GCM_DISPLAY_FLIP_STATUS_WAITING = 1
};

enum CellGcmDefaultFifoMode{
    CELL_GCM_DEFAULT_FIFO_MODE_TRADITIONAL,
    CELL_GCM_DEFAULT_FIFO_MODE_OPTIMIZE,
    CELL_GCM_DEFAULT_FIFO_MODE_CONDITIONAL,
};

enum CellGcmSystemMode{
    CELL_GCM_SYSTEM_MODE_IOMAP_512MB = (1<<0),
    CELL_GCM_SYSTEM_MODE_MASK        = (1)
};


} // namespace Local


///////////////////////////////////////////////////////////////////////////////
// PS3MemoryDump
///////////////////////////////////////////////////////////////////////////////

PS3MemoryDump::PS3MemoryDump()
  : mpStream(NULL),
    mProcessMemoryDataArray()
{
}


void PS3MemoryDump::Init(EA::IO::IStream* pStream)
{
    // Currently we don't AddRef mpStream.
    mpStream = pStream;
}


void PS3MemoryDump::AddProcessMemoryData(const PS3DumpELF::ProcessMemoryData& pmd)
{
    mProcessMemoryDataArray.push_back(pmd);
}


// Returns true if the memory could be entirely read.
//
// What we are trying to do here is the following:
//    memcpy(pDest, address, byteCount);
// But it is complicated by the fact that the source memory is not in RAM and is in segments on disk.
//
bool PS3MemoryDump::ReadMemory(void* pDest, uint64_t address, uint64_t byteCount) const
{
    // Question: Is is possible that the memory read request could straddle two segments?
    // We'll currently assume that it can do so.

    uint64_t copiedCount    = 0;
    uint64_t byteCountSaved = byteCount;

    #if (EA_PLATFORM_PTR_SIZE < 8)
        EA_ASSERT(byteCount <= UINT32_MAX);
    #endif
    memset(pDest, 0, (size_t)byteCount);

    // First we do some clipping to make sure that the user isn't requesting something outside a 64 bit address space.
    if((UINT64_MAX - byteCount) < address)  // If address + byteCount would overflow 64 bits (you need to do a subtraction like this to check for this) ...
        byteCount = (UINT64_MAX - address);

    for(eastl_size_t i = 0, iEnd = mProcessMemoryDataArray.size(); (i < iEnd) && (copiedCount < byteCount); ++i)
    {
        const PS3DumpELF::ProcessMemoryData& pmd = mProcessMemoryDataArray[i];

        // If there is an overlap with the input request and the current segment, copy it.
        if((address < (pmd.p_vaddr + pmd.p_filesz)) && 
          ((address + byteCount) >= pmd.p_vaddr)) // If there is any memory overlap...
        {
            // Clip the starting of the requested range to the current pmd range.
            int64_t sourceOffset = (int64_t)address - (int64_t)pmd.p_vaddr;
            int64_t copyCount    = (int64_t)byteCount;

            if(sourceOffset < 0)    // If requested address is a value less than the pmd address...
            {
                address   -= sourceOffset; 
                copyCount += sourceOffset;
                sourceOffset = 0;
            }

            // Clip the end of the requested range to the current pmd range.
            int64_t endOffset = (int64_t)((address + copyCount) - (pmd.p_vaddr + pmd.p_filesz));

            if(endOffset > 0)    // If the requested range goes beyond the pmd range...
            {
                EA_ASSERT(copyCount >= endOffset); // This should never fail, regardless of the user input. If it did then our math here would have to be wrong.
                copyCount -= endOffset;
            }

            EA_ASSERT(copyCount >= 0);
            EA_ASSERT(address >= pmd.p_vaddr);
            EA_ASSERT((address + copyCount) <= (pmd.p_vaddr + pmd.p_filesz));

            // EAIO can deal with 64 bit file IO on a 32 bit operating system if the file system supports 64 bit IO, as does Windows for example.
            mpStream->SetPosition((EA::IO::off_type)(pmd.p_offset + sourceOffset));
            EA::IO::size_type readCount = mpStream->Read(pDest, (EA::IO::size_type)copyCount);
            EA_ASSERT((int64_t)readCount == copyCount);

            if((int64_t)readCount != copyCount)
                break; // We have a major problem. Quit and return false.
            
            copiedCount += byteCount;
        }
    }

    return (copiedCount == byteCountSaved);
}

uint8_t PS3MemoryDump::ReadUint8(uint64_t address) const
{
    uint8_t result;
    ReadMemory(&result, address, sizeof(result));
    return result;
}

uint16_t PS3MemoryDump::ReadUint16(uint64_t address) const
{
    uint16_t result;
    ReadMemory(&result, address, sizeof(result));
    #if defined(EA_SYSTEM_LITTLE_ENDIAN)
        result = Local::SwizzleUint16(result);
    #endif
    return result;
}

uint32_t PS3MemoryDump::ReadUint32(uint64_t address) const
{
    uint32_t result;
    ReadMemory(&result, address, sizeof(result));
    #if defined(EA_SYSTEM_LITTLE_ENDIAN)
        result = Local::SwizzleUint32(result);
    #endif
    return result;
}

uint64_t PS3MemoryDump::ReadUint64(uint64_t address) const
{
    uint64_t result;
    ReadMemory(&result, address, sizeof(result));
    #if defined(EA_SYSTEM_LITTLE_ENDIAN)
        result = Local::SwizzleUint64(result);
    #endif
    return result;
}




///////////////////////////////////////////////////////////////////////////////
// PS3SPUMemoryDump
///////////////////////////////////////////////////////////////////////////////


PS3SPUMemoryDump::PS3SPUMemoryDump()
  : mpStream(NULL),
    mLocalStorageDataArray()
{
}


void PS3SPUMemoryDump::Init(EA::IO::IStream* pStream)
{
    // Currently we don't AddRef mpStream.
    mpStream = pStream;
}


void PS3SPUMemoryDump::AddLocalStorageData(const PS3DumpELF::SPULocalStorageData& lsd)
{
    mLocalStorageDataArray.push_back(lsd);
}


const PS3DumpELF::SPULocalStorageData* PS3SPUMemoryDump::GetLocalStorageData(uint32_t spuThreadID) const
{
    for(eastl_size_t i = 0, iEnd = mLocalStorageDataArray.size(); i != iEnd; ++i)
    {
        const PS3DumpELF::SPULocalStorageData& lsd = mLocalStorageDataArray[i];

        if(lsd.mSPUThreadID == spuThreadID)
            return &lsd;
    }

    return NULL;
}


bool PS3SPUMemoryDump::ReadMemory(uint32_t spuThreadID, void* pDest, uint32_t address, uint32_t byteCount) const
{
    memset(pDest, 0, byteCount);

    const PS3DumpELF::SPULocalStorageData* pLSD = GetLocalStorageData(spuThreadID);

    if(pLSD && (address < kMemorySize)) // Safety clipping.
    {
        if(byteCount > (kMemorySize - address))
           byteCount = (kMemorySize - address);

        memcpy(pDest, pLSD->mMemory + address, byteCount);
        return true;
    }

    return false;
}


// This function finds the beginning of the SPU process in SPU local storage (a.k.a. LS) by
// looking for a guid pattern which is present at the beginning of SPU processes. It's not quite 
// so simple as doing a memcmp() to test for the pattern, as the pattern has some bits that
// are variable. So we need to apply a mask to filter away the variable bits.
//
// According to Sony, this might not work with the EA job manager.
//
int32_t PS3SPUMemoryDump::GetELFImageOffset(uint32_t spuThreadID) const
{
    const PS3DumpELF::SPULocalStorageData* pLSD = GetLocalStorageData(spuThreadID);

    if(pLSD)
    {
        static const uint8_t pattern[16] =
        {
            0x42, 0x00, 0x00, 0x02,
            0x42, 0x00, 0x00, 0x82,
            0x42, 0x00, 0x01, 0x02,
            0x42, 0x00, 0x01, 0x82
        };
        
        static const uint8_t mask[4] = 
        {
            0xfe, 0x00, 0x01, 0xff
        };

        for(int32_t i = 0; i < (0x40000 - 0x100); ++i)
        {
            const uint8_t* ptr = pLSD->mMemory + i;

            if (((ptr[ 0] & mask[0]) == pattern[0]) &&
                ((ptr[ 1] & mask[1]) == pattern[1]) &&
                ((ptr[ 2] & mask[2]) == pattern[2]) &&
                ((ptr[ 3] & mask[3]) == pattern[3]) &&
                ((ptr[ 4] & mask[0]) == pattern[4]) &&
                ((ptr[ 5] & mask[1]) == pattern[5]) &&
                ((ptr[ 6] & mask[2]) == pattern[6]) &&
                ((ptr[ 7] & mask[3]) == pattern[7]) &&
                ((ptr[ 8] & mask[0]) == pattern[8]) &&
                ((ptr[ 9] & mask[1]) == pattern[9]) &&
                ((ptr[10] & mask[2]) == pattern[10]) &&
                ((ptr[11] & mask[3]) == pattern[11]) &&
                ((ptr[12] & mask[0]) == pattern[12]) &&
                ((ptr[13] & mask[1]) == pattern[13]) &&
                ((ptr[14] & mask[2]) == pattern[14]) &&
                ((ptr[15] & mask[3]) == pattern[15]))
            {
                //uint8_t guid[8];
                //
                //for (int i = 0; i < 16; i += 4)
                //{
                //    guid[i/2 ]  = ((ptr[i  ] & 0x01) << 7) | ((ptr[i+1] & 0xfe) >> 1);
                //    guid[i/2+1] = ((ptr[i+1] & 0x01) << 7) | ((ptr[i+2] & 0xfe) >> 1);
                //}

                return i;
            }
        }
    }

    return -1;
}



uint8_t PS3SPUMemoryDump::ReadUint8(uint32_t spuThreadID, uint32_t address) const
{
    uint8_t result;
    ReadMemory(spuThreadID, &result, address, sizeof(result));
    return result;
}


uint16_t PS3SPUMemoryDump::ReadUint16(uint32_t spuThreadID, uint32_t address) const
{
    uint16_t result;
    ReadMemory(spuThreadID, &result, address, sizeof(result));
    #if defined(EA_SYSTEM_LITTLE_ENDIAN)
        result = Local::SwizzleUint16(result);
    #endif
    return result;
}


uint32_t PS3SPUMemoryDump::ReadUint32(uint32_t spuThreadID, uint32_t address) const
{
    uint32_t result;
    ReadMemory(spuThreadID, &result, address, sizeof(result));
    #if defined(EA_SYSTEM_LITTLE_ENDIAN)
        result = Local::SwizzleUint32(result);
    #endif
    return result;
}


uint64_t PS3SPUMemoryDump::ReadUint64(uint32_t spuThreadID, uint32_t address) const
{
    uint64_t result;
    ReadMemory(spuThreadID, &result, address, sizeof(result));
    #if defined(EA_SYSTEM_LITTLE_ENDIAN)
        result = Local::SwizzleUint64(result);
    #endif
    return result;
}





///////////////////////////////////////////////////////////////////////////////
// PS3DumpFile
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// PS3DumpFile
//
PS3DumpFile::PS3DumpFile(EA::Allocator::ICoreAllocator* pCoreAllocator)
  : mpCoreAllocator(pCoreAllocator ? pCoreAllocator : EA::Callstack::GetAllocator()),
    msDumpFilePath(),
    mFileStream(),
    mpStream(NULL),
    mELFFile(),
    mbInitialized(false),
    mbLoaded(false),
    mAddressRepLookupSetPPU(),
    mAddressRepLookupSetSPU(),

    mSystemData(),
    mProcessData(),
    mPPUThreadRegisterDataArray(),
    mPPUThreadDataArray(),
    mSPUThreadRegisterDataArray(),
    mSPUThreadDataArray(),
    mSPUThreadGroupDataArray(),
    mSPUThreadGroupDataDefault(),

    mMutexDataArray(),
    mConditionVariableDataArray(),
    mRWLockDataArray(),
    mLWMutexDataArray(),
    mEventQueueDataArray(),
    mSemaphoreDataArray(),
    mLWConditionVariableDataArray(),

    mRSXDebugInfo1Data(),
    mMemoryStatisticsData(),
    mCoreDumpCauseUnion(),
    mPRXDataArray(),
    mMemoryPageAttributeDataArray(),
    mMemoryPageAttributeDataDefault(),
    mMemoryContainerDataArray(),
    mEventFlagDataArray(),

    mPS3MemoryDump(),
    mPS3SPUMemoryDump()
{
    // AddRef these members so they are reference counted away.
    mFileStream.AddRef();
}


///////////////////////////////////////////////////////////////////////////////
// ~PS3DumpFile
//
PS3DumpFile::~PS3DumpFile()
{
    Shutdown();
}


///////////////////////////////////////////////////////////////////////////////
// SetAllocator
//
void PS3DumpFile::SetAllocator(EA::Allocator::ICoreAllocator* pCoreAllocator)
{
    mpCoreAllocator = pCoreAllocator;
    // If there are any members that need to see this, tell them here.
}


///////////////////////////////////////////////////////////////////////////////
// Init
//
bool PS3DumpFile::Init(const wchar_t* pDumpFilePath, bool bDelayLoad)
{
    if(!mbInitialized)
    {
        if(pDumpFilePath)
        {
            msDumpFilePath = pDumpFilePath;
            mFileStream.SetPath(pDumpFilePath);

            if(mFileStream.Open(EA::IO::kAccessFlagRead))
            {
                mpStream = &mFileStream;

                if(mELFFile.Init(mpStream))
                {
                    mbInitialized = true;

                    EA_ASSERT(mELFFile.mELFOffset == 0); // Assert that this is a .elf file and not an ELF file stored inside some other file such as a .self file).
                    mPS3MemoryDump.Init(mpStream);

                    if(!bDelayLoad)
                        Load();
                }
            }
        }
    }

    return mbInitialized;
}


///////////////////////////////////////////////////////////////////////////////
// Shutdown
//
bool PS3DumpFile::Shutdown()
{
    if(mbInitialized)
    {
        mbInitialized = false;

        // mpCoreAllocator      // Keep as-is.
        // msDumpFilePath       // Keep as-is.
        mFileStream.Close();
        // mpStream             // Keep as-is.
        // mELFFile
        mbLoaded = false;

        mSystemData  = PS3DumpELF::SystemData();
        mProcessData = PS3DumpELF::ProcessData();

        mPPUThreadRegisterDataArray.clear();
        mPPUThreadDataArray.clear();

        mSPUThreadRegisterDataArray.clear();
        mSPUThreadDataArray.clear();
        mSPUThreadGroupDataArray.clear();
        mSPUThreadGroupDataDefault = PS3DumpELF::SPUThreadGroupData();

        mMutexDataArray.clear();
        mConditionVariableDataArray.clear();
        mRWLockDataArray.clear();
        mLWMutexDataArray.clear();
        mEventQueueDataArray.clear();
        mSemaphoreDataArray.clear();
        mLWConditionVariableDataArray.clear();

        mRSXDebugInfo1Data = PS3DumpELF::RSXDebugInfo1Data();
        mMemoryStatisticsData = PS3DumpELF::MemoryStatisticsData();
        mCoreDumpCauseUnion = PS3DumpELF::CoreDumpCauseUnion();
        mPRXDataArray.clear();
        mMemoryPageAttributeDataArray.clear();
        mMemoryPageAttributeDataDefault = PS3DumpELF::MemoryPageAttributeData();
        mMemoryContainerDataArray.clear();
        mEventFlagDataArray.clear();

        mPS3MemoryDump = PS3MemoryDump();
        mPS3SPUMemoryDump = PS3SPUMemoryDump();
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// AddDatabaseFile
//
bool PS3DumpFile::AddDatabaseFile(const wchar_t* pDatabaseFilePath, uint64_t baseAddress, 
                                  bool bDelayLoad, EA::Allocator::ICoreAllocator* pCoreAllocator)
{
    using namespace PS3DumpELF;

    bool               bResult = false;
    TargetOS           targetOS;
    TargetProcessor    targetProcessor;
    IAddressRepLookup* pLookup = NULL;

    if(!pCoreAllocator)
        pCoreAllocator = mpCoreAllocator;

    SymbolInfoType sit = GetSymbolInfoTypeFromDatabase(pDatabaseFilePath, &targetOS, &targetProcessor);
    EA_ASSERT(sit != kSymbolInfoTypeNone); (void)sit;

    if(targetOS == kTargetOSSPU)
    {
        bResult = mAddressRepLookupSetSPU.AddDatabaseFile(pDatabaseFilePath, baseAddress, bDelayLoad, pCoreAllocator);
      //pLookup = mAddressRepLookupSetSPU.GetAddressRepLookup(pDatabaseFilePath); // No need for this because our use of pLookup is for PRXs below, and SPUs don't have PRXs.
    }
    else
    {
        bResult = mAddressRepLookupSetPPU.AddDatabaseFile(pDatabaseFilePath, baseAddress, bDelayLoad, pCoreAllocator);
        pLookup = mAddressRepLookupSetPPU.GetAddressRepLookup(pDatabaseFilePath);
    }

    if(pLookup) // This should always be true if bResult is true.
    {
        // If we've already been initialized, then we should be able to read mPRXDataArray and tell what
        // the base address of any PRX database file paths should be. 
        if(!mPRXDataArray.empty() && (baseAddress == 0))
        {
            // If there is any PRX in mPRXDataArray which matches the file name in pDatabaseFilePath, use its base address.
            FixedStringW dbFileNameW;
            Local::GetFileNameOnly(pDatabaseFilePath, dbFileNameW, true);

            for(eastl_size_t i = 0, iEnd = mPRXDataArray.size(); i < iEnd; i++)
            {
                const PRXData& prxData = mPRXDataArray[i];

                FixedStringW prxNameW;
                Local::GetFileNameOnly(prxData.mName, prxNameW, true);

                FixedStringW prxFileNameW;
                Local::GetFileNameOnly(prxData.mPath, prxFileNameW, true);

                if((dbFileNameW == prxNameW) || 
                   (dbFileNameW == prxFileNameW))
                {
                    if(prxData.mpTextSegment) // This should always be true in practice.
                        pLookup->SetBaseAddress(prxData.mpTextSegment->mBaseAddress);
                    break;
                }
            }
        }
    }

    return bResult;
}



///////////////////////////////////////////////////////////////////////////////
// AddSourceCodeDirectory
//
void PS3DumpFile::AddSourceCodeDirectory(const wchar_t* pSourceDirectory)
{
    mAddressRepLookupSetPPU.AddSourceCodeDirectory(pSourceDirectory);
    mAddressRepLookupSetSPU.AddSourceCodeDirectory(pSourceDirectory);
}


///////////////////////////////////////////////////////////////////////////////
// TraceInternal
//
void PS3DumpFile::TraceInternal(EA::IO::IStream* pOutput, int traceFilterFlags)
{
    using namespace PS3DumpELF;
    using namespace Local;

    char buffer[256];
    PS3DumpFile::CrashProcessor cp = GetCrashProcessor();
    PS3DumpELF::SyncInfo syncInfoDeadlockChain[6];

    // Summary Data / Core dump cause
    if((traceFilterFlags & kTFSummaryData) || (traceFilterFlags & kTFCrashCauseData))
    {
        WriteStringFormatted(pOutput, "Core dump cause\n");
        WriteStringFormatted(pOutput, "  Initiated by: %s\n", GetCoreDumpCauseString(mCoreDumpCauseUnion.mCoreDumpCauseData.mCauseType));
        WriteStringFormatted(pOutput, "  Order: %I32u\n", mCoreDumpCauseUnion.mCoreDumpCauseData.mOrder);

        switch (mCoreDumpCauseUnion.mCoreDumpCauseData.mCauseType)
        {
            case kCdctPPUException:
                // To do: Detect that PPU or SPU crash was due to running out of stack space.
                WriteStringFormatted(pOutput, "  Exception type: %s\n", GetPPUExceptionTypeString(mCoreDumpCauseUnion.mCoreDumpCausePPUException.mPPUExceptionType));
                WriteStringFormatted(pOutput, "  PPU thread ID: %I64u (0x%016I64x)\n", mCoreDumpCauseUnion.mCoreDumpCausePPUException.mPPUThreadID, mCoreDumpCauseUnion.mCoreDumpCausePPUException.mPPUThreadID);
                WriteStringFormatted(pOutput, "  PPU thread DAR: 0x%016I64x %s\n", mCoreDumpCauseUnion.mCoreDumpCausePPUException.mDAR, IsPPUExceptionDataAccess(mCoreDumpCauseUnion.mCoreDumpCausePPUException.mPPUExceptionType) ? "(exception is due to this value)" : "(meaningless for this type of exception)");
                WriteStringFormatted(pOutput, "  PPU thread IAR (PC): 0x%016I64x\n", mCoreDumpCauseUnion.mCoreDumpCausePPUException.mPC);
                break;

            case kCdctSPUException:
                GetSPUExceptionTypeString(mCoreDumpCauseUnion.mCoreDumpCauseSPUException.mSPUExceptionType, buffer);
                WriteStringFormatted(pOutput, "  Exception type: %s\n", buffer);
                WriteStringFormatted(pOutput, "  SPU thread group ID: %I32u (0x%08I32x)\n", mCoreDumpCauseUnion.mCoreDumpCauseSPUException.mSPUThreadGroupID, mCoreDumpCauseUnion.mCoreDumpCauseSPUException.mSPUThreadGroupID);
                WriteStringFormatted(pOutput, "  SPU thread ID: %I32u (0x%08I32x)\n", mCoreDumpCauseUnion.mCoreDumpCauseSPUException.mSPUThreadID, mCoreDumpCauseUnion.mCoreDumpCauseSPUException.mSPUThreadID);
                WriteStringFormatted(pOutput, "  SPU NPC: %I32u (0x%08I32x)\n", mCoreDumpCauseUnion.mCoreDumpCauseSPUException.mSPU_NPC, mCoreDumpCauseUnion.mCoreDumpCauseSPUException.mSPU_NPC);

                if(mCoreDumpCauseUnion.mCoreDumpCauseSPUException.mSPUExceptionType & (kSETMFCDataSegment | kSETMFCDataStorage | kSETMemoryAccessTrap))
                    WriteStringFormatted(pOutput, "  MFC DAR: 0x%016I64x\n", mCoreDumpCauseUnion.mCoreDumpCauseSPUException.mOption);
                else
                    WriteStringFormatted(pOutput, "  SPU Status: 0x%08I32x\n", (uint32_t)mCoreDumpCauseUnion.mCoreDumpCauseSPUException.mOption);

                break;

            case kCdctRSXException:
                WriteStringFormatted(pOutput, "  Channel ID: %I32u (0x%08I32x)\n", mCoreDumpCauseUnion.mCoreDumpCauseRSXException.mChid, mCoreDumpCauseUnion.mCoreDumpCauseRSXException.mChid);
                WriteStringFormatted(pOutput, "  Graphics error #: %I32u (0x%08I32x)\n", mCoreDumpCauseUnion.mCoreDumpCauseRSXException.mGraphicsErrorNo, mCoreDumpCauseUnion.mCoreDumpCauseRSXException.mGraphicsErrorNo);
                WriteStringFormatted(pOutput, "  Error param 0: %I32u (0x%08I32x)\n", mCoreDumpCauseUnion.mCoreDumpCauseRSXException.mErrorParam0, mCoreDumpCauseUnion.mCoreDumpCauseRSXException.mErrorParam0);
                WriteStringFormatted(pOutput, "  Error param 1: %I32u (0x%08I32x)\n", mCoreDumpCauseUnion.mCoreDumpCauseRSXException.mErrorParam1, mCoreDumpCauseUnion.mCoreDumpCauseRSXException.mErrorParam1);
                WriteStringFormatted(pOutput, "  Graphics error description: %s\n", GetRSXGraphicsErrorString(mCoreDumpCauseUnion.mCoreDumpCauseRSXException.mGraphicsErrorNo));
                // We display detailed RSX state data further below.
                break;

            case kCdctFootSwitch:
                // Nothing to do.
                break;

            case kCdctUserDefinedEvent:
                WriteStringFormatted(pOutput, "  User data:\n");
                for(int j = 0; j < 8; j++)
                    WriteStringFormatted(pOutput, "    %02x", mCoreDumpCauseUnion.mCoreDumpCauseUserDefinedEvent.mData1[j]);
                WriteStringFormatted(pOutput, "\n");

                WriteStringFormatted(pOutput, "  User data:\n");
                for(int j = 0; j < 8; j++)
                    WriteStringFormatted(pOutput, "    %02x", mCoreDumpCauseUnion.mCoreDumpCauseUserDefinedEvent.mData2[j]);
                WriteStringFormatted(pOutput, "\n");

                WriteStringFormatted(pOutput, "  User data:\n");
                for(int j = 0; j < 8; j++)
                    WriteStringFormatted(pOutput, "    %02x", mCoreDumpCauseUnion.mCoreDumpCauseUserDefinedEvent.mData3[j]);
                WriteStringFormatted(pOutput, "\n");
                break;

            case kCdctSystemEvent:
                // Nothing to do.
                break;
        }

        WriteStringFormatted(pOutput, "\n");
    }

    // Process data
    if(traceFilterFlags & kTFProcessData)
    {
        WriteStringFormatted(pOutput, "Process data\n");
        WriteStringFormatted(pOutput, "  Status: %s\n", (mProcessData.mStatus == 1) ? "created" : ((mProcessData.mStatus == 2) ? "started" : "ended"));
        WriteStringFormatted(pOutput, "  PPU thread count: %I32u\n", mProcessData.mPPUThreadCount);
        WriteStringFormatted(pOutput, "  SPU thread count: %I32u\n", mProcessData.mSPUThreadGroupCount);
        WriteStringFormatted(pOutput, "  Raw SPU count: %I32u\n", mProcessData.mRawSPUCount);
        WriteStringFormatted(pOutput, "  Max physical memory size: %I64u\n", mProcessData.mMaxPhysicalMemorySize);
        WriteStringFormatted(pOutput, "  Path: %s\n", mProcessData.mPath);
        WriteStringFormatted(pOutput, "\n");
    }

    // We display PPU thread data if the user specifically requested it or if the  
    // user requested just crash-related data and one of the PPU threads crashed.
    if((traceFilterFlags & kTFPPUData) || 
       ((cp == kCPPPU) && (traceFilterFlags & kTFCrashCauseData))) // If there was a crash then we might want to display the SPU data for the crashed thread.
    {
        EA_ASSERT(mPPUThreadDataArray.size() == mPPUThreadRegisterDataArray.size());

        const bool bDisplayOnlyCrashedThreadData = ((traceFilterFlags & kTFPPUData) == 0);

        for(eastl_size_t i = 0, iEnd = mPPUThreadDataArray.size(); i < iEnd; ++i) // For each PPU thread...
        {
            const PPUThreadData&         td  = mPPUThreadDataArray[i];
            const PPUThreadRegisterData& trd = mPPUThreadRegisterDataArray[i];

            EA_ASSERT(trd.mPPUThreadID == td.mPPUThreadID); // If this fails then the threads aren't in the same order and we need to find the right td in mPPUThreadDataArray.

            if((traceFilterFlags & kTFSkipCoreDumpThread) && ((EA::StdC::Strcmp(td.mName, "__COREDUMP_HANDLER__") == 0) || (EA::StdC::Strcmp(td.mName, "_DBG_PPU_EXCEPTION_HANDLER") == 0)))
                continue;

            bool bCrashedThread = (PS3DumpFile::GetCrashedPPUThreadID() == td.mPPUThreadID);

            if(bDisplayOnlyCrashedThreadData && !bCrashedThread)
                continue;

            uint64_t returnAddressArray[48];
            memset(returnAddressArray, 0, sizeof(returnAddressArray)); // Useful for debugging.
            size_t   returnAddressCount = GetPPUCallstack(returnAddressArray, EAArrayCount(returnAddressArray), td.mPPUThreadID);

            uint32_t      nDeadlockedCount = GetPPUThreadDeadlock(td.mPPUThreadID, syncInfoDeadlockChain, EAArrayCount(syncInfoDeadlockChain)); 
            SyncInfoArray lockArray;
            SyncInfoArray waitArray;

            WriteStringFormatted(pOutput, "PPU thread data\n");
            WriteStringFormatted(pOutput, "  ID: %I64u (0x%016I64x)\n", td.mPPUThreadID, td.mPPUThreadID);
            WriteStringFormatted(pOutput, "  Name: %s\n", td.mName);
            WriteStringFormatted(pOutput, "  State: %s%s%s\n", GetPPUThreadStateString(td.mState), bCrashedThread ? " (crashed)" : "", nDeadlockedCount ? " (deadlocked)" : "");
            WriteStringFormatted(pOutput, "  Priority: %I32d\n", td.mPriority);
            WriteStringFormatted(pOutput, "  Stack begin: 0x%016I64x\n", td.mStackAddress);
            WriteStringFormatted(pOutput, "  Stack end: 0x%016I64x\n", td.mStackAddress - td.mStackSize); // Stacks grow downward in memory.
            WriteStringFormatted(pOutput, "  Stack current: 0x%016I64x\n", trd.mGPR[1]);
            WriteStringFormatted(pOutput, "  IAR (PC): 0x%016I64x\n", trd.mPC);

            if(bCrashedThread)
            {
                WriteStringFormatted(pOutput, "  Exception type: %I32u (%s)\n", mCoreDumpCauseUnion.mCoreDumpCausePPUException.mCauseType, GetPPUExceptionTypeString(mCoreDumpCauseUnion.mCoreDumpCausePPUException.mPPUExceptionType));
                WriteStringFormatted(pOutput, "  PPU thread DAR: 0x%016I64x %s\n", mCoreDumpCauseUnion.mCoreDumpCausePPUException.mDAR, IsPPUExceptionDataAccess(mCoreDumpCauseUnion.mCoreDumpCausePPUException.mPPUExceptionType) ? "(exception is due to this value)" : "(meaningless for this type of exception)");
            }

            if(nDeadlockedCount) // It should usually be impossible that a thread is both crashed and deadlocked.
            {
                 // To do: We can greatly improve this report with more information.
                WriteStringFormatted(pOutput, "  Deadlock chain thread IDs:");

                for(uint32_t d = 0; d < nDeadlockedCount; ++d)
                    WriteStringFormatted(pOutput, " 0x%016I64x", syncInfoDeadlockChain[d].mOwnerThreadID);

                WriteStringFormatted(pOutput, "\n");
            }

            // Report callstack
            if(traceFilterFlags & kTFPPUCallstackData)
            {
                WriteStringFormatted(pOutput, "  Callstack: %s\n", mAddressRepLookupSetPPU.GetDatabaseFileCount() ? "" : "(no symbol .map, .elf, or .self files were supplied)");

                for(size_t i = 0; i < returnAddressCount; ++i)
                {
                    // Sometimes the dump file has a goofy impossible address. May have something to do with 
                    // the PS3 crash handler processing screwing with the processor register and memory state.
                    if(returnAddressArray[i] > 0xffffffff)
                        continue;

                    WriteStringFormatted(pOutput, "    0x%016I64x", returnAddressArray[i]);

                    if(traceFilterFlags & kTFPPUAddressRep)
                    {
                        if(mAddressRepLookupSetPPU.GetDatabaseFileCount())
                        {
                            FixedString pStrResults[kARTCount];
                            int         pIntResults[kARTCount];

                            // GetAddressRep currently uses a local concept of an address pointer, whereas it really should just use uint64_t. Until then, this code can fail for a 32 bit OS reading addresses of a 64 bit program.
                            int outputFlags = mAddressRepLookupSetPPU.GetAddressRep(kARTFlagFunctionOffset | kARTFlagFileLine, returnAddressArray[i], pStrResults, pIntResults);

                            if(outputFlags)
                            {
                                if(outputFlags & kARTFlagFunctionOffset)
                                    WriteStringFormatted(pOutput, " (%s + %d)", pStrResults[kARTFunctionOffset].c_str(), pIntResults[kARTFunctionOffset]);

                                if(outputFlags & kARTFlagFileLine)
                                    WriteStringFormatted(pOutput, " (%s: %d)", pStrResults[kARTFileLine].c_str(), pIntResults[kARTFileLine]);
                            }
                            else
                            {
                                // The address probably comes from a system PRX (i.e. DLL) or from a user DLL which doesn't have a .map/.prx/.sprx symbol file registered.
                                FixedString sUnknownAddressDescription;

                                GetUnknownAddressDescription(returnAddressArray[i], sUnknownAddressDescription);
                                WriteStringFormatted(pOutput, " %s", sUnknownAddressDescription.c_str());
                            }
                        }
                    }

                    WriteStringFormatted(pOutput, "\n");
                }

                WriteStringFormatted(pOutput, "\n");
            }

            if(traceFilterFlags & kTFPPURegisterData)
            {
                // Report registers
                WriteStringFormatted(pOutput, "  IAR (PC): 0x%016I64x\n", trd.mPC);
                WriteStringFormatted(pOutput, "  CR: 0x%016I32x\n", trd.mCR);
                WriteStringFormatted(pOutput, "  LR: 0x%016I64x\n", trd.mLR);
                WriteStringFormatted(pOutput, "  CTR: 0x%016I64x\n", trd.mCTR);
                WriteStringFormatted(pOutput, "  XER: 0x%016I64x\n", trd.mXER);
                WriteStringFormatted(pOutput, "  FPSCR: 0x%08I32x\n", trd.mFPSCR);
                trd.mVSCR.Int128ToStr(buffer, NULL, 16);    // EAStdC sprintf doesn't yet correctly support 128 bit types.
                WriteStringFormatted(pOutput, "  VSCR: %s\n", buffer);
                WriteStringFormatted(pOutput, "\n");

                for(int r = 0; r < 32; r += 2)
                    WriteStringFormatted(pOutput, "  GPR %2d: 0x%016I64x  GPR %2d: 0x%016I64x\n", r, trd.mGPR[r], r + 1, trd.mGPR[r + 1]);
                WriteStringFormatted(pOutput, "\n");

                for(int f = 0; f < 32; f += 2)
                {
                    WriteStringFormatted(pOutput, "  FPR %2d: 0x%016I64x  FPR %2d: 0x%016I64x\n", f, trd.mFPR[f].mUint64, f + 1, trd.mFPR[f + 1].mUint64);
                    WriteStringFormatted(pOutput, "          %18g          %18g\n",                  trd.mFPR[f].mDouble,        trd.mFPR[f + 1].mDouble);
                }
                WriteStringFormatted(pOutput, "\n");

                for(int v = 0; v < 32; v += 2)
                {
                    WriteStringFormatted(pOutput, "  VMX %2d: 0x%08I32x 0x%08I32x 0x%08I32x 0x%08I32x  VMX %2d: 0x%08I32x 0x%08I32x 0x%08I32x 0x%08I32x\n", 
                                                  v,     trd.mVMX[v  ].mWord[0], trd.mVMX[v  ].mWord[1], trd.mVMX[v  ].mWord[2], trd.mVMX[v  ].mWord[3], 
                                                  v + 1, trd.mVMX[v+1].mWord[0], trd.mVMX[v+1].mWord[1], trd.mVMX[v+1].mWord[2], trd.mVMX[v+1].mWord[3]);

                    WriteStringFormatted(pOutput, "          %10g %10g %10g %10g          %10g %10g %10g %10g\n", 
                                                  v,     trd.mVMX[v  ].mFloat[0], trd.mVMX[v  ].mFloat[1], trd.mVMX[v  ].mFloat[2], trd.mVMX[v  ].mFloat[3], 
                                                  v + 1, trd.mVMX[v+1].mFloat[0], trd.mVMX[v+1].mFloat[1], trd.mVMX[v+1].mFloat[2], trd.mVMX[v+1].mFloat[3]);
                }
                WriteStringFormatted(pOutput, "\n");
            }

            // Report synchronization primitives for this thread.
            if(traceFilterFlags & kTFPPUSyncData)
            {
                GetPPUThreadSyncInfo(td.mPPUThreadID, &lockArray, &waitArray);

                if(lockArray.empty())
                    WriteStringFormatted(pOutput, "  Locked sync primitives: none\n");
                else
                {
                    WriteStringFormatted(pOutput, "  Locked sync primitives:\n");

                    for(eastl_size_t i = 0, iEnd = lockArray.size(); i != iEnd; ++i)
                    {
                        const SyncInfo& si = lockArray[i];

                        WriteStringFormatted(pOutput, "    Type: %s, ID: %I32u, Name: %s\n", GetSyncTypeString(si.mSyncType), si.mID, si.mName);
                    }
                }

                if(waitArray.empty())
                    WriteStringFormatted(pOutput, "  Waiting sync primitives: none\n");
                else
                {
                    WriteStringFormatted(pOutput, "  Waiting sync primitives:\n");

                    for(eastl_size_t i = 0, iEnd = waitArray.size(); i != iEnd; ++i)
                    {
                        const SyncInfo& si = waitArray[i];

                        WriteStringFormatted(pOutput, "    Type: %s, ID: %I32u, Name: %s, Owner Thread ID: 0x%016I64x, Owner Thread Name: %s\n", 
                                             GetSyncTypeString(si.mSyncType), si.mID, si.mName, si.mOwnerThreadID, GetPPUThreadName(si.mOwnerThreadID));
                    }
                }

                WriteStringFormatted(pOutput, "\n");
            }

            if(bCrashedThread)
            {
                // Crash Source
                if(traceFilterFlags & kTFPPUCrashSource)
                {
                    WriteStringFormatted(pOutput, "  Callstack entries source code:\n");

                    if(mAddressRepLookupSetPPU.GetDatabaseFileCount())
                    {
                        for(size_t i = 0; i < returnAddressCount; ++i)
                        {
                            FixedString pStrResults[kARTCount];
                            int         pIntResults[kARTCount];

                            int outputFlags = mAddressRepLookupSetPPU.GetAddressRep(kARTFlagSource | kARTFlagFileLine, returnAddressArray[i], pStrResults, pIntResults);

                            if(outputFlags & kARTFlagSource) // If succeeded...
                            {
                                WriteStringFormatted(pOutput, "   %s: %d\n", pStrResults[kARTFileLine].c_str(), pIntResults[kARTFileLine]);

                                // Indent the text before printing it.
                                pStrResults[kARTSource].insert(0, "    ");
                                for(eastl_size_t i; (i = pStrResults[kARTSource].find("\n>")) != FixedString::npos; )
                                    pStrResults[kARTSource].replace(i, 2, "\n    >");

                                WriteStringFormatted(pOutput, "%s\n", pStrResults[kARTSource].c_str());
                            }
                            else
                            {
                                if(outputFlags & kARTFlagFileLine)
                                    WriteStringFormatted(pOutput, "No source code could be found for \"%s\"\n", pStrResults[kARTFileLine].c_str());
                                else
                                    WriteStringFormatted(pOutput, "No source code nor file/line could be found for address 0x%016I64x\n", returnAddressArray[i]);
                            }
                        }
                    }
                    else
                        WriteStringFormatted(pOutput, "    No source code directory was provided.\n");

                    WriteStringFormatted(pOutput, "\n");
                }

                // Crash Dasm
                if(traceFilterFlags & kTFPPUCrashDasm)
                {
                    WriteStringFormatted(pOutput, "  Disassembly around crash instruction at 0x%016I64x:\n", trd.mPC);

                    if(trd.mPC)
                    {
                        WriteStringFormatted(pOutput, "    Address     Instruction\n");
                        WriteStringFormatted(pOutput, "    ----------------------------------\n");

                        const size_t              kBufferSize  = 80;
                        uint64_t                  ps3StartAddress = (trd.mPC - 28) & ~31;
                        uint8_t                   instructionBuffer[kBufferSize];
                        const uint8_t*            pCurrent = instructionBuffer;
                        const uint8_t*            pEnd     = instructionBuffer + kBufferSize;
                        const uint8_t*            pPrev    = pCurrent;
                        Dasm::DisassemblerPowerPC dasm;
                        Dasm::DasmData            dd;

                        mPS3MemoryDump.ReadMemory(instructionBuffer, ps3StartAddress, kBufferSize);

                        while(pCurrent != pEnd)
                        {
                            pPrev    = pCurrent;
                            pCurrent = (const uint8_t*)dasm.Dasm(pCurrent, pEnd, dd, EA::Dasm::kOFHex | EA::Dasm::kOFMnemonics, (uintptr_t)pCurrent);

                            uint32_t ps3Address     = (uint32_t)ps3StartAddress + (uint32_t)(pPrev - instructionBuffer);
                            uint32_t ps3Instruction = 0;

                            memcpy(&ps3Instruction, pPrev, 4);
                            ps3Instruction = SwizzleUint32(ps3Instruction);

                            // To consider: Provide source code with this. Need to use AddressRepLookup for this.
                            if(ps3Address == trd.mPC)
                                WriteStringFormatted(pOutput, "    %08I32x => %08I32x %s %s\n", ps3Address, ps3Instruction, dd.mOperation, dd.mOperands);
                            else
                                WriteStringFormatted(pOutput, "    %08I32x    %08I32x %s %s\n", ps3Address, ps3Instruction, dd.mOperation, dd.mOperands);
                        }
                    }
                    else
                        WriteStringFormatted(pOutput, "  Disassembly is not available; program counter is 0.\n");

                    WriteStringFormatted(pOutput, "\n");
                }
            }

            if(nDeadlockedCount)
            {
                WriteStringFormatted(pOutput, "  Deadlock report:\n");

                if(nDeadlockedCount == 1)
                    WriteStringFormatted(pOutput, "    Thread is deadlocked against itself.\n");
                else
                    WriteStringFormatted(pOutput, "    This is a %d-way deadlock.\n", nDeadlockedCount);

                for(uint32_t d = 0; d < nDeadlockedCount; ++d)
                {
                    const SyncInfo& siD = syncInfoDeadlockChain[d];
                    const SyncInfo& siE = syncInfoDeadlockChain[(d + 1) % nDeadlockedCount]; // This is the next entry, wrapping around to first entry upon the last entry.

                    // Thread 0x1234 ("Main") has locked Mutex 201 ("AudioMutex"). It is waiting on Mutex 202 ("AIMutex").
                    WriteStringFormatted(pOutput, "    Thread 0x%016I64x (\"%s\") has locked %s %I32u (\"%s\"). It is waiting on %s %I32u (\"%s\").\n",
                                                    siD.mOwnerThreadID, GetPPUThreadName(siD.mOwnerThreadID), GetSyncTypeString(siD.mSyncType), siD.mID, siD.mName, 
                                                    GetSyncTypeString(siE.mSyncType), siE.mID, siE.mName);
                }
            }

            // Report possible implicit deadlocks, whereby this thread is waiting for a 
            // signal but owns a lock which some other thread is blocking on. This isn't 
            // the same as a true deadlock, but may possibly act as one in practice.

            if(!waitArray.empty() && !lockArray.empty())
            {
                for(eastl_size_t w = 0, wEnd = waitArray.size(); w != wEnd; ++w)
                {
                    const SyncInfo& siWait = waitArray[w];

                    // If this thread is waiting on an event type which is owner-less, report on any other threads that may be blocked due to this thread's wait.
                    if((siWait.mSyncType == kSTConditionVariable)   || 
                       (siWait.mSyncType == kSTEventQueue)          || 
                       (siWait.mSyncType == kSTSemaphore)           || 
                       (siWait.mSyncType == kSTLWConditionVariable) || 
                       (siWait.mSyncType == kSTEvent))
                    {
                        int nPossibleImplicitDeadlockCount = 0; // Count of found possible implicit deadlocks.
                        const char8_t* pSyncTypeWait = GetSyncTypeString(siWait.mSyncType);

                        for(eastl_size_t i = 0, iEnd = lockArray.size(); i != iEnd; ++i)
                        {
                            const SyncInfo& siLock = lockArray[i];
                            Uint64Array     threadArray;

                            if(GetWaitingThreads(siLock, threadArray)) // If somebody is indeed blocking on a mutex this thread owns...
                            {
                                if(nPossibleImplicitDeadlockCount++ == 0) // If this is the first one... write a header.
                                {
                                    WriteStringFormatted(pOutput, "  Possible implicit deadlock report:\n");

                                    // Thread 0x1234 ("Main") is waiting on Semaphore 201 ("AudioSemaphore").
                                    WriteStringFormatted(pOutput, "    Thread 0x%016I64x (\"%s\") is waiting on %s %I32u (\"%s\").\n",
                                                                    td.mPPUThreadID, td.mName, pSyncTypeWait, siWait.mID, siWait.mName);
                                }

                                for(eastl_size_t t = 0, tEnd = threadArray.size(); t != tEnd; t++)
                                {
                                    const uint64_t threadID = threadArray[t];

                                    // Thread 0x2345 ("AI") is waiting on Mutex 301 ("AI Mutex"), which thread 0x1234 ("Main") has locked.
                                    WriteStringFormatted(pOutput, "    Thread 0x%016I64x (\"%s\") is waiting on %s %I32u (\"%s\"), which thread 0x%016I64x (\"%s\") has locked.\n",
                                                                    threadID, GetPPUThreadName(threadID), GetSyncTypeString(siLock.mSyncType), siLock.mID, siLock.mName, td.mPPUThreadID, td.mName);
                                }
                            }
                        }
                    }
                }
            }

            WriteStringFormatted(pOutput, "\n");
        }
    }

    // We display SPU thread data if the user specifically requested it or if the  
    // user requested just crash-related data and one of the SPU threads crashed.
    if((traceFilterFlags & kTFSPUData) || 
       ((cp == kCPSPU) && (traceFilterFlags & kTFCrashCauseData))) // If there was a crash then we might want to dispaly the PPU data for the crashed thread.
    {
        EA_ASSERT(mSPUThreadDataArray.size() == mSPUThreadRegisterDataArray.size());

        const bool bDisplayOnlyCrashedThreadData = ((traceFilterFlags & kTFSPUData) == 0);

        for(eastl_size_t i = 0, iEnd = mSPUThreadDataArray.size(); i < iEnd; ++i)
        {
            const SPUThreadData&         td  = mSPUThreadDataArray[i];
            const SPUThreadRegisterData& trd = mSPUThreadRegisterDataArray[i];
            const SPUThreadGroupData&    tgd = GetSPUThreadGroupData(td.mSPUThreadGroupID);

            EA_ASSERT(trd.mSPUThreadID == td.mSPUThreadID); // If this fails then the threads aren't in the same order and we need to find the right td in mPPUThreadDataArray.

            bool bCrashedThread = (PS3DumpFile::GetCrashedSPUThreadID() == td.mSPUThreadID);

            if(bDisplayOnlyCrashedThreadData && !bCrashedThread)
                continue;

            uint32_t returnAddressArray[48];
            memset(returnAddressArray, 0, sizeof(returnAddressArray)); // Useful for debugging.
            size_t   returnAddressCount = GetSPUCallstack(returnAddressArray, EAArrayCount(returnAddressArray), td.mSPUThreadID);

            WriteStringFormatted(pOutput, "SPU thread data\n");
            WriteStringFormatted(pOutput, "  ID: %I32d (0x%08I32x)\n", td.mSPUThreadID, td.mSPUThreadID);
            WriteStringFormatted(pOutput, "  Name: %s\n", td.mName);
            WriteStringFormatted(pOutput, "  File name: %s\n", td.mFilename);
            WriteStringFormatted(pOutput, "  Group name: %s\n", tgd.mName);
            WriteStringFormatted(pOutput, "  ELF offset: %I32d\n", td.mELFOffset);
            WriteStringFormatted(pOutput, "  State: %s%s\n", GetSPUThreadStateString(tgd.mState), bCrashedThread ? " (crashed)" : "");
            WriteStringFormatted(pOutput, "  Priority: %I32d\n", tgd.mPriority);
            WriteStringFormatted(pOutput, "  Stack current: 0x%08I32x\n", trd.mGPR[1]);
            WriteStringFormatted(pOutput, "  NPC (PC): 0x%08I32x\n", trd.mNPC);

            if(bCrashedThread)
            {
                char buffer[256];
                GetSPUExceptionTypeString(mCoreDumpCauseUnion.mCoreDumpCauseSPUException.mSPUExceptionType, buffer);

                WriteStringFormatted(pOutput, "  Exception type: 0x%08I32x (%s)\n", mCoreDumpCauseUnion.mCoreDumpCauseSPUException.mCauseType, buffer);
                WriteStringFormatted(pOutput, "  SPU thread PC: 0x%08I32x\n", trd.mNPC);  // If this is a memory exception and DAR is 0, then (usually, always?) we have a NULL pointer exception.
                WriteStringFormatted(pOutput, "\n");
            }

            // Report callstack
            if(traceFilterFlags & kTFSPUCallstackData)
            {
                WriteStringFormatted(pOutput, "  Callstack: %s\n", mAddressRepLookupSetSPU.GetDatabaseFileCount() ? "" : "(no SPU symbol .map or .elf files were supplied)");

                for(size_t i = 0; i < returnAddressCount; ++i)
                {
                    WriteStringFormatted(pOutput, "    0x%08I32x (before relocation: 0x%08I32x) ", returnAddressArray[i], returnAddressArray[i] - td.mELFOffset);

                    if(traceFilterFlags & kTFSPUAddressRep)
                    {
                        IAddressRepLookup* pSPULookup = GetSPULookup(td.mFilename);

                        if(pSPULookup)
                        {
                            FixedString pStrResults[kARTCount];
                            int         pIntResults[kARTCount];

                            // For the lookups below we will use the original ELF location of the symbol, which was offset by tgd.mELFOffset bytes when loaded into SPU memory.
                            returnAddressArray[i] -= td.mELFOffset;

                            // GetAddressRep currently uses a local concept of an address pointer, whereas it really should just use uint64_t. Until then, this code can fail for a 32 bit OS reading addresses of a 64 bit program.
                            int outputFlags = pSPULookup->GetAddressRep(kARTFlagFunctionOffset | kARTFlagFileLine, returnAddressArray[i], pStrResults, pIntResults);
                          
                            if(outputFlags & kARTFlagFunctionOffset)
                                WriteStringFormatted(pOutput, " (%s: %d)", pStrResults[kARTFunctionOffset].c_str(), pIntResults[kARTFunctionOffset]);

                            if(outputFlags & kARTFlagFileLine)
                                WriteStringFormatted(pOutput, " (%s: %d)", pStrResults[kARTFileLine].c_str(), pIntResults[kARTFileLine]);
                        }
                    }

                    WriteStringFormatted(pOutput, "\n");
                }

                WriteStringFormatted(pOutput, "\n");
            }

            if(traceFilterFlags & kTFSPURegisterData)
            {
                WriteStringFormatted(pOutput, "  NPC (PC): 0x%08I32x\n", trd.mNPC);
                trd.mFPSCR.Int128ToStr(buffer, NULL, 16);    // EAStdC sprintf doesn't yet correctly support 128 bit types.
                WriteStringFormatted(pOutput, "  FPSCR: %s\n", buffer);
                WriteStringFormatted(pOutput, "  Decrementer: 0x%08I32x\n", trd.mDecrementer);
                WriteStringFormatted(pOutput, "  SRR0: 0x%08I32x\n", trd.mSRR0);
                WriteStringFormatted(pOutput, "  MB_STAT: 0x%08I32x\n", trd.mMB_STAT);
                WriteStringFormatted(pOutput, "  SPU_MB[0]: 0x%08I32x, SPU_MB[1]: 0x%08I32x, SPU_MB[2]: 0x%08I32x, SPU_MB[3]: 0x%08I32x\n", trd.mSPU_MB[0], trd.mSPU_MB[1], trd.mSPU_MB[2], trd.mSPU_MB[3]);
                WriteStringFormatted(pOutput, "  SPU_STATUS: 0x%08I32x\n", trd.mSPU_STATUS);
                WriteStringFormatted(pOutput, "  PPU_MB: 0x%08I32x\n", trd.mPPU_MB);
                WriteStringFormatted(pOutput, "  SPU_CFG: 0x%016I64x\n", trd.mSPU_CFG);
                WriteStringFormatted(pOutput, "\n");

                for(int v = 0; v < 128; v += 2)
                {
                    WriteStringFormatted(pOutput, "  GPR %2d: 0x%08I32x 0x%08I32x 0x%08I32x 0x%08I32x  GPR %2d: 0x%08I32x 0x%08I32x 0x%08I32x 0x%08I32x\n", 
                                                  v,     trd.mGPR[v  ].mWord[0], trd.mGPR[v  ].mWord[1], trd.mGPR[v  ].mWord[2], trd.mGPR[v  ].mWord[3], 
                                                  v + 1, trd.mGPR[v+1].mWord[0], trd.mGPR[v+1].mWord[1], trd.mGPR[v+1].mWord[2], trd.mGPR[v+1].mWord[3]);

                    WriteStringFormatted(pOutput, "          %10g %10g %10g %10g          %10g %10g %10g %10g\n", 
                                                  v,     trd.mGPR[v  ].mFloat[0], trd.mGPR[v  ].mFloat[1], trd.mGPR[v  ].mFloat[2], trd.mGPR[v  ].mFloat[3], 
                                                  v + 1, trd.mGPR[v+1].mFloat[0], trd.mGPR[v+1].mFloat[1], trd.mGPR[v+1].mFloat[2], trd.mGPR[v+1].mFloat[3]);
                }
                WriteStringFormatted(pOutput, "\n");

                for(int v = 0; v < 128; v += 2)
                    WriteStringFormatted(pOutput, "  MFC CQ SR %2d: 0x%016I64x MFC CQ SR %2d: 0x%016I64x\n", v, trd.mMFC_CQ_SR[v], v + 1, trd.mMFC_CQ_SR[v + 1]);

                WriteStringFormatted(pOutput, "\n");
            }

            if(bCrashedThread)
            {
                // Crash Source
                if(traceFilterFlags & kTFSPUCrashSource)
                {
                    // This code is essentially copied from the equivalent PPU code (kTFPPUCrashSource).

                    WriteStringFormatted(pOutput, "  Callstack entries source code:\n");

                    if(mAddressRepLookupSetSPU.GetDatabaseFileCount())
                    {
                        for(size_t i = 0; i < returnAddressCount; ++i)
                        {
                            FixedString pStrResults[kARTCount];
                            int         pIntResults[kARTCount];

                            int outputFlags = mAddressRepLookupSetSPU.GetAddressRep(kARTFlagSource | kARTFlagFileLine, returnAddressArray[i], pStrResults, pIntResults);

                            if(outputFlags & kARTFlagSource) // If succeeded...
                            {
                                WriteStringFormatted(pOutput, "   %s: %d\n", pStrResults[kARTFileLine].c_str(), pIntResults[kARTFileLine]);

                                // Indent the text before printing it.
                                pStrResults[kARTSource].insert(0, "    ");
                                for(eastl_size_t i; (i = pStrResults[kARTSource].find("\n>")) != FixedString::npos; )
                                    pStrResults[kARTSource].replace(i, 2, "\n    >");

                                WriteStringFormatted(pOutput, "%s\n", pStrResults[kARTSource].c_str());
                            }
                            else
                            {
                                if(outputFlags & kARTFlagFileLine)
                                    WriteStringFormatted(pOutput, "No source code could be found for \"%s\"\n", pStrResults[kARTFileLine].c_str());
                                else
                                    WriteStringFormatted(pOutput, "No source code nor file/line could be found for address 0x%08x\n", (uint32_t)returnAddressArray[i]);
                            }
                        }
                    }
                    else
                        WriteStringFormatted(pOutput, "    No source code directory was provided.\n");

                    WriteStringFormatted(pOutput, "\n");
                }
            }
        }
    }

    // We display RSX data if the user specifically requested it or if the  
    // user requested just crash-related data and there was an RSX crash.
    if((traceFilterFlags & kTFRSXData) || 
       ((cp == kCPRSX) && (traceFilterFlags & kTFCrashCauseData))) // If there was a crash then we might want to display the PPU data for the crashed thread.
    {
        PS3DumpELF::RSXDebugInfo1Data& mRSX = mRSXDebugInfo1Data; // Shortened alias.

        FixedString8 s8;
        int i;

        WriteStringFormatted(pOutput, "RSX data (please request upgrades to the reported data as needed)\n");

        GetInterruptErrorStatusString(mRSX.mInterruptErrorStatus, s8);
        WriteStringFormatted(pOutput, "  Interrupt error status: %s\n", s8.c_str());

        if(mRSX.mInterruptErrorStatus & kIESFIFOError)
        {
            GetInterruptFIFOErrorStatusString(mRSX.mInterruptFIFOErrorStatus, s8);
            WriteStringFormatted(pOutput, "  FIFO error: %s\n", s8.c_str());
        }

        if(mRSX.mInterruptErrorStatus & kIESIOIFError)
        {
            GetInterruptIOIFErrorStatusString(mRSX.mInterruptIOIFErrorStatus, s8);
            WriteStringFormatted(pOutput, "  IOIF error: %s\n", s8.c_str());
        }

        if(mRSX.mInterruptErrorStatus & kIESGraphicsError)
        {
            GetInterruptGraphicsErrorStatusString(mRSX.mInterruptGraphicsErrorStatus, s8);
            WriteStringFormatted(pOutput, "  Graphics error: %s\n", s8.c_str());
        }

        WriteStringFormatted(pOutput, "  Tile region unit data (CellDbgRsxTileRegion from <cell/dbgrsx.h>):\n");
        for(i = 0; i < (int)EAArrayCount(mRSX.mTileRegionUnitArray); i++)
        {
            // Info here comes from libdbgrsx-Reference_e.pdf
            const TileRegionUnit& tri = mRSX.mTileRegionUnitArray[i];

            WriteStringFormatted(pOutput, "    %2d: %7s, %s, start addr: 0x%08x, end addr: 0x%08x, pitch: %6u, compressed tag start: 0x%04x, compressed tag end: 0x%04x, Compression mode: %s\n", 
                                i, 
                                tri.mEnable ? (tri.mEnable-1 ? "bound main" : "bound local") : "unbound", 
                                tri.mBank ? "depth" : "color",
                                tri.mStartAddr,
                                tri.mLimitAddr,
                                tri.mPitchSize,
                                tri.mBaseTag,
                                tri.mLimitTag,
                                GetTileRegionUnitCompModeString(tri.mCompMode));
        }

        WriteStringFormatted(pOutput, "  Z cull region unit data (CellDbgRsxZcullRegion from <cell/dbgrsx.h>):\n");
        for(i = 0; i < (int)EAArrayCount(mRSX.mZcullRegionArray); i++)
        {
            // Info here comes from libdbgrsx-Reference_e.pdf
            const ZcullRegionUnit& zri = mRSX.mZcullRegionArray[i];

            WriteStringFormatted(pOutput, "    %2d: %7s, %s, %s, width: %5u, height: %5u, start addr: 0x%08x, addr offset: 0x%08x\n", 
                                i, 
                                zri.mEnable ? "bound" : "unbound", 
                                zri.mFormat == CELL_GCM_ZCULL_Z16 ? "CELL_GCM_ZCULL_Z16" : "CELL_GCM_ZCULL_Z24S8",
                                GetZCullRegionAntialiasString(zri.mAntialias),
                                zri.mWidth,
                                zri.mHeight,
                                zri.mStart,
                                zri.mOffset);
        }

        WriteStringFormatted(pOutput, "  Graphics error state data.\n");
        {
            const GraphicsErrorStateData& ges = mRSX.mGraphicsErrorStateData;

            // LAUNCH_CHECK
            WriteStringFormatted(pOutput, "    Launch check 1: 0x%08x\n", ges.mLCLaunchCheck1);
            WriteStringFormatted(pOutput, "    Launch check 2: 0x%08x\n", ges.mLCLaunchCheck2);
         
            // TRAPPED_ADDRESS_DATA
            WriteStringFormatted(pOutput, "    Trapped Address Dhv: 0x%02x\n", ges.mTADTrappedAddressDhv);
            WriteStringFormatted(pOutput, "    Trapped Address Subch & Mthd: 0x%04x\n", ges.mTADTrappedAddressSM);
            WriteStringFormatted(pOutput, "    Trapped Data Low: 0x%08x\n", ges.mTADTrappedDataLow);
            WriteStringFormatted(pOutput, "    Trapped Data High: 0x%08x\n", ges.mTADTrappedDataHigh);

            // COLOR_SURFACE_VIOLATION
            WriteStringFormatted(pOutput, "    Dstbpitch & Dstblimit & Dstovrflw & Dstmemsize & Dsttiled: 0x%02x\n", ges.mCSVDDDDD);
            WriteStringFormatted(pOutput, "    Srcbpitch & Srcblimit & Srcovrflw & Srcmemsize & Srctiled: 0x%02x\n", ges.mCSVSSSSS);
            WriteStringFormatted(pOutput, "    Swizzlex & swizzley: 0x%02x\n", ges.mCSVSS);
            WriteStringFormatted(pOutput, "    Ctilemode: 0x%02x\n", ges.mCSVCtilemode);
            WriteStringFormatted(pOutput, "    Ropmode: 0x%04x\n", ges.mCSVRopmode);
            WriteStringFormatted(pOutput, "    Address: 0x%08x\n", ges.mCSVAddress);

            // COLOR_SURFACE_LIMIT
            WriteStringFormatted(pOutput, "    Color Surface Limit 0: 0x%08x\n", ges.mCSLColorSurfaceLimit0);
            WriteStringFormatted(pOutput, "    Color Surface Limit 1: 0x%08x\n", ges.mCSLColorSurfaceLimit1);
            WriteStringFormatted(pOutput, "    Color Surface Limit 2: 0x%08x\n", ges.mCSLColorSurfaceLimit2);
            WriteStringFormatted(pOutput, "    Color Surface Limit 3: 0x%08x\n", ges.mCSLColorSurfaceLimit3);

            // DEPTH_SURFACE_VIOLATION
            WriteStringFormatted(pOutput, "    Dstbpitch & Dstblimit & Dstovrflw & Dstmemsize & Dsttiled: 0x%02x\n", ges.mDSVDDDDD);
            WriteStringFormatted(pOutput, "    Srcbpitch & Srcblimit & Srcovrflw & Srcmemsize & Srctiled: 0x%02x\n", ges.mDSVSSSSS);
            WriteStringFormatted(pOutput, "    Swizzlex & Swizzley : 0x%02x\n", ges.mDSVSS);
            WriteStringFormatted(pOutput, "    Ztilemode: 0x%02x\n", ges.mDSVZtilemode);
            WriteStringFormatted(pOutput, "    Ropmode: 0x%04x\n", ges.mDSVRopmode);
            WriteStringFormatted(pOutput, "    Address: 0x%08x\n", ges.mDSVAddress);

            // DEPTH_SURFACE_LIMIT
            WriteStringFormatted(pOutput, "    Depth Surface Limit : 0x%08x\n", ges.mDSLDepthSurfaceLimit);

            // TRANSFER_DATA_READ
            WriteStringFormatted(pOutput, "    Dst format: 0x%02x\n", ges.mTDRDstFormat);
            WriteStringFormatted(pOutput, "    Src format: 0x%02x\n", ges.mTDRSrcFormat);
            WriteStringFormatted(pOutput, "    Misc Count: 0x%04x\n", ges.mTDRMiscCount);
            WriteStringFormatted(pOutput, "    Read Address: 0x%08x\n", ges.mTDRReadAddress);

            // TRANSFER_DATA_WRITE
            WriteStringFormatted(pOutput, "    Dst format: 0x%02x\n", ges.mTDWDstFormat);
            WriteStringFormatted(pOutput, "    Src format: 0x%02x\n", ges.mTDWSrcFormat);
            WriteStringFormatted(pOutput, "    Misc Count: 0x%04x\n", ges.mTDWMiscCount);
            WriteStringFormatted(pOutput, "    Write Address: 0x%08x\n", ges.mTDWWriteAddress);

            // GRAPHICS_UNIT
            WriteStringFormatted(pOutput, "    Graphics Engine Status: 0x%02x\n", ges.mGUGraphicsEngineStatus);
            WriteStringFormatted(pOutput, "    3d Frontend Unit: 0x%02x\n", ges.mGU3dFrontendUnit);
            WriteStringFormatted(pOutput, "    Idx Unit: 0x%02x\n", ges.mGUIdxUnit);
            WriteStringFormatted(pOutput, "    Geometry Unit: 0x%02x\n", ges.mGUGeometryUnit);
            WriteStringFormatted(pOutput, "    Setup Unit: 0x%02x\n", ges.mGUSetupUnit);
            WriteStringFormatted(pOutput, "    Spill Unit: 0x%02x\n", ges.mGUSpillUnit);
            WriteStringFormatted(pOutput, "    Raster Unit: 0x%02x\n", ges.mGURasterUnit);
            WriteStringFormatted(pOutput, "    Zcull Update Unit: 0x%02x\n", ges.mGUZcullUpdateUnit);
            WriteStringFormatted(pOutput, "    Preshader Unit: 0x%02x\n", ges.mGUPreshaderUnit);
            WriteStringFormatted(pOutput, "    Shader Base Unit: 0x%02x\n", ges.mGUShaderBaseUnit);
            WriteStringFormatted(pOutput, "    Shader Pipe Unit: 0x%02x\n", ges.mGUShaderPipeUnit);
            WriteStringFormatted(pOutput, "    Texture Unit: 0x%02x\n", ges.mGUTextureUnit);
            WriteStringFormatted(pOutput, "    Rop Unit: 0x%02x\n", ges.mGURopUnit);
            WriteStringFormatted(pOutput, "    2d Front End: 0x%02x\n", ges.mGU2dFrontEnd);
            WriteStringFormatted(pOutput, "    2d Rasterizer: 0x%02x\n", ges.mGU2dRasterizer);

            // FRONTEND_INDEX_UNIT
            WriteStringFormatted(pOutput, "    FE Status: 0x%02x\n", ges.mFIUFEStatus);
            WriteStringFormatted(pOutput, "    IDX Status: 0x%02x\n", ges.mFIUIDXStatus);

            // GEOMETRY_UNIT
            WriteStringFormatted(pOutput, "    Geometry Vab Status: 0x%02x\n", ges.mGYUGeometryVabStatus);
            WriteStringFormatted(pOutput, "    Geometry Vpc Status: 0x%02x\n", ges.mGYUGeometryVpcStatus);
            WriteStringFormatted(pOutput, "    Geometry Vtf Status: 0x%02x\n", ges.mGYUGeometryVtfStatus);

            // SETUP_UNIT
            WriteStringFormatted(pOutput, "    Setup Status: 0x%02x\n", ges.mSUSetupStatus);

            // RASTER_UNIT
            WriteStringFormatted(pOutput, "    Coarse Raster: 0x%02x\n", ges.mRUCoarseRaster);
            WriteStringFormatted(pOutput, "    Fine Raster: 0x%02x\n", ges.mRUFineRaster);
            WriteStringFormatted(pOutput, "    Window ID: 0x%02x\n", ges.mRUWindowID);

            // ZCULL_UNIT
            WriteStringFormatted(pOutput, "    Zcull Proc: 0x%02x\n", ges.mZUZcullProc);
            WriteStringFormatted(pOutput, "    Update Units Status: 0x%02x\n", ges.mZUUpdateUnitsStatus);

            // PRE_SHADER_UNIT
            WriteStringFormatted(pOutput, "    Color Assembly: 0x%02x\n", ges.mPSUColorAssembly);
            WriteStringFormatted(pOutput, "    Shader Triangle Fifo: 0x%02x\n", ges.mPSUShaderTriangleFifo);
            WriteStringFormatted(pOutput, "    Shader Triangle Processor: 0x%02x\n", ges.mPSUShaderTriangleProcessor);

            // BASE_SHADER_UNIT
            WriteStringFormatted(pOutput, "    Shader Quad Distributor: 0x%02x\n", ges.mBSUShaderQuadDistributor);
            WriteStringFormatted(pOutput, "    Shader Quad Collector: 0x%02x\n", ges.mBSUShaderQuadCollector);

            // TEXTURE_CACHE_UNIT
            WriteStringFormatted(pOutput, "    Mcache: 0x%02x\n", ges.mTCUMcache);

            // ROP_UNIT
            WriteStringFormatted(pOutput, "    Rop Unit Status: 0x%02x\n", ges.mRPURopUnitStatus);

            // 2D_UNIT
            WriteStringFormatted(pOutput, "    2D Unit Status: 0x%02x\n", ges.mU2DUnitStatus);
            WriteStringFormatted(pOutput, "    Rasterizer 2D: 0x%02x\n", ges.mURasterizer2D);
        }

        WriteStringFormatted(pOutput, "  Not yet reported by this app: z cull status data.\n");
        WriteStringFormatted(pOutput, "  Not yet reported by this app: z cull math unit data.\n");
        WriteStringFormatted(pOutput, "  Not yet reported by this app: FIFO error state data.\n");
        WriteStringFormatted(pOutput, "  Not yet reported by this app: IOIF error state data.\n");
        WriteStringFormatted(pOutput, "  Not yet reported by this app: graphics FIFO unit data.\n");
        WriteStringFormatted(pOutput, "  Not yet reported by this app: FIFO cache unit data.\n");
        WriteStringFormatted(pOutput, "  Not yet reported by this app: EA to IO table data.\n");
        WriteStringFormatted(pOutput, "  Not yet reported by this app: IO to EA table data.\n");
        WriteStringFormatted(pOutput, "  Not yet reported by this app: bundle state data.\n");
        WriteStringFormatted(pOutput, "\n");
    }

    if(traceFilterFlags & kTFPPUSyncData)
    {
        eastl_size_t i, iEnd;

        WriteStringFormatted(pOutput, "Synchronization data\n");

        for(i = 0, iEnd = mMutexDataArray.size(); i != iEnd; ++i)
        {
            const MutexData& d = mMutexDataArray[i];

            // Mutex 301 ("AI Mutex"):
            //     Properties: Recursive, ...
            //     Owner: 0x1234 ("Main"), locked 2 time(s).
            //     Waiter(s): 0x1234 ("Main"), 0x2345 ("Network").

            WriteStringFormatted(pOutput, "  Mutex %I32u (\"%s\"):\n", d.mID, d.mName);
            WriteStringFormatted(pOutput, "    Properties: %s %s %s.\n", d.mRecursive ? "recursive" : "non-recursive", d.mShared ? "shared" : "non-shared", d.mAdaptive ? "adaptive" : "non-adaptive");
            WriteStringFormatted(pOutput, "    Owner: 0x%016I64x (\"%s\"), locked %I32u time(s).\n", d.mOwnerThreadID, GetPPUThreadName(d.mOwnerThreadID), d.mLockCounter);

            if(d.mWaitThreadIDArray.empty())
                WriteStringFormatted(pOutput, "    Waiters: none");
            else
                WriteStringFormatted(pOutput, "    Waiter(s):");

            for(eastl_size_t w = 0, wEnd = d.mWaitThreadIDArray.size(); w != wEnd; ++w)
                WriteStringFormatted(pOutput, " 0x%016I64x (\"%s\")", d.mWaitThreadIDArray[w], GetPPUThreadName(d.mWaitThreadIDArray[w]));

            WriteStringFormatted(pOutput, "\n");
        }

        for(i = 0, iEnd = mConditionVariableDataArray.size(); i != iEnd; ++i)
        {
            const ConditionVariableData& d = mConditionVariableDataArray[i];

            // Condition Variable 301 ("AI Cond"):
            //     Mutex: 0x1234 ("AI Mutex"), locked 2 time(s).
            //     Waiter(s): 0x1234 ("Main"), 0x2345 ("Network").

            WriteStringFormatted(pOutput, "  Condition Variable %I32u (\"%s\"):\n", d.mID, d.mName);
            WriteStringFormatted(pOutput, "    Mutex %I32u.\n", d.mMutexID);  // To do: More info here.

            if(d.mWaitThreadIDArray.empty())
                WriteStringFormatted(pOutput, "    Waiters: none");
            else
                WriteStringFormatted(pOutput, "    Waiter(s):");

            for(eastl_size_t w = 0, wEnd = d.mWaitThreadIDArray.size(); w != wEnd; ++w)
                WriteStringFormatted(pOutput, " 0x%016I64x (\"%s\")", d.mWaitThreadIDArray[w], GetPPUThreadName(d.mWaitThreadIDArray[w]));

            WriteStringFormatted(pOutput, "\n");
        }

        for(i = 0, iEnd = mRWLockDataArray.size(); i != iEnd; ++i)
        {
            const RWLockData& d = mRWLockDataArray[i];

            // RWLock 301 ("AI RWLock"):
            //     Owner: 0x1234 ("Main"). There may be more but the PS3 core dump fails to list multiple read owners.
            //     Read waiter(s): 0x1234 ("Main"), 0x2345 ("Network").
            //     Write waiter(s): 0x1234 ("Main"), 0x2345 ("Network").

            WriteStringFormatted(pOutput, "  RWLock %I32u (\"%s\"):\n", d.mID, d.mName);
            WriteStringFormatted(pOutput, "    Owner: 0x%016I64x (\"%s\"). There may be more but the PS3 core dump fails to list multiple read owners.\n", d.mOwnerThreadID, GetPPUThreadName(d.mOwnerThreadID));

            if(d.mReaderWaitThreadIDArray.empty())
                WriteStringFormatted(pOutput, "    Read waiters: none");
            else
                WriteStringFormatted(pOutput, "    Read waiter(s):");

            for(eastl_size_t w = 0, wEnd = d.mReaderWaitThreadIDArray.size(); w != wEnd; ++w)
                WriteStringFormatted(pOutput, " 0x%016I64x (\"%s\")", d.mReaderWaitThreadIDArray[w], GetPPUThreadName(d.mReaderWaitThreadIDArray[w]));

            WriteStringFormatted(pOutput, "\n");

            if(d.mWriterWaitThreadIDArray.empty())
                WriteStringFormatted(pOutput, "    Write waiters: none");
            else
                WriteStringFormatted(pOutput, "    Write waiter(s):");

            for(eastl_size_t w = 0, wEnd = d.mWriterWaitThreadIDArray.size(); w != wEnd; ++w)
                WriteStringFormatted(pOutput, " 0x%016I64x (\"%s\")", d.mWriterWaitThreadIDArray[w], GetPPUThreadName(d.mWriterWaitThreadIDArray[w]));

            WriteStringFormatted(pOutput, "\n");
        }

        for(i = 0, iEnd = mLWMutexDataArray.size(); i != iEnd; ++i)
        {
            const LWMutexData& d = mLWMutexDataArray[i];

            // LWMutex 301 ("AI Mutex"):
            //     Properties: Recursive, ...
            //     Owner: 0x1234 ("Main"), locked 2 time(s).
            //     Waiter(s): 0x1234 ("Main"), 0x2345 ("Network").

            WriteStringFormatted(pOutput, "  LWMutex %I32u (\"%s\"):\n", d.mID, d.mName);
            WriteStringFormatted(pOutput, "    Properties: %s.\n", d.mRecursive ? "recursive" : "non-recursive");
            WriteStringFormatted(pOutput, "    Owner: 0x%016I64x (\"%s\"), locked %I32u time(s).\n", d.mOwnerThreadID, GetPPUThreadName(d.mOwnerThreadID), d.mLockCounter);

            if(d.mWaitThreadIDArray.empty())
                WriteStringFormatted(pOutput, "    Waiters: none");
            else
                WriteStringFormatted(pOutput, "    Waiter(s):");

            for(eastl_size_t w = 0, wEnd = d.mWaitThreadIDArray.size(); w != wEnd; ++w)
                WriteStringFormatted(pOutput, " 0x%016I64x (\"%s\")", d.mWaitThreadIDArray[w], GetPPUThreadName(d.mWaitThreadIDArray[w]));

            WriteStringFormatted(pOutput, "\n");
        }

        for(i = 0, iEnd = mEventQueueDataArray.size(); i != iEnd; ++i)
        {
            const EventQueueData& d = mEventQueueDataArray[i];

            // EventQueue 301 ("AI Queue"):
            //     Queue entries: ("abc"), ("def").
            //     Waiter(s): 0x1234 ("Main"), 0x2345 ("Network").

            WriteStringFormatted(pOutput, "  Event Queue %I32u (\"%s\"):\n", d.mID, d.mName);

            // To do: Write d.mEntryArrayArray

            if(d.mWaitThreadIDArray.empty())
                WriteStringFormatted(pOutput, "    Waiters: none");
            else
                WriteStringFormatted(pOutput, "    Waiter(s):");

            for(eastl_size_t w = 0, wEnd = d.mWaitThreadIDArray.size(); w != wEnd; ++w)
                WriteStringFormatted(pOutput, " 0x%016I64x (\"%s\")", d.mWaitThreadIDArray[w], GetPPUThreadName(d.mWaitThreadIDArray[w]));

            WriteStringFormatted(pOutput, "\n");
        }

        for(i = 0, iEnd = mSemaphoreDataArray.size(); i != iEnd; ++i)
        {
            const SemaphoreData& d = mSemaphoreDataArray[i];

            // Semaphore 301 ("AI Sem"):
            //     Current value: 2, max value: 4.
            //     Waiter(s): 0x1234 ("Main"), 0x2345 ("Network").

            WriteStringFormatted(pOutput, "  Semaphore %I32u (\"%s\"):\n", d.mID, d.mName);
            WriteStringFormatted(pOutput, "    Current value %I32u, max value: %I32u\n", d.mCurrentValue, d.mMaxValue);

            if(d.mWaitThreadIDArray.empty())
                WriteStringFormatted(pOutput, "    Waiters: none");
            else
                WriteStringFormatted(pOutput, "    Waiter(s):");

            for(eastl_size_t w = 0, wEnd = d.mWaitThreadIDArray.size(); w != wEnd; ++w)
                WriteStringFormatted(pOutput, " 0x%016I64x (\"%s\")", d.mWaitThreadIDArray[w], GetPPUThreadName(d.mWaitThreadIDArray[w]));

            WriteStringFormatted(pOutput, "\n");
        }

        for(i = 0, iEnd = mLWConditionVariableDataArray.size(); i != iEnd; ++i)
        {
            const LWConditionVariableData& d = mLWConditionVariableDataArray[i];

            // LWCondition Variable 301 ("AI Cond"):
            //     LWMutex: 0x1234 ("AI Mutex"), locked 2 time(s).
            //     Waiter(s): 0x1234 ("Main"), 0x2345 ("Network").

            WriteStringFormatted(pOutput, "  LW Condition Variable %I32u (\"%s\"):\n", d.mID, d.mName);
            WriteStringFormatted(pOutput, "    LW Mutex %I32u.\n", d.mLWMutexID);  // To do: More info here.

            if(d.mWaitThreadIDArray.empty())
                WriteStringFormatted(pOutput, "    Waiters: none");
            else
                WriteStringFormatted(pOutput, "    Waiter(s):");

            for(eastl_size_t w = 0, wEnd = d.mWaitThreadIDArray.size(); w != wEnd; ++w)
                WriteStringFormatted(pOutput, " 0x%016I64x (\"%s\")", d.mWaitThreadIDArray[w], GetPPUThreadName(d.mWaitThreadIDArray[w]));

            WriteStringFormatted(pOutput, "\n");
        }

        WriteStringFormatted(pOutput, "\n");
    }

    if(traceFilterFlags & kTFPRXData)
    {
        EA_ASSERT(mPPUThreadDataArray.size() == mPPUThreadDataArray.size());

        WriteStringFormatted(pOutput, "PRX data\n");

        for(eastl_size_t i = 0, iEnd = mPRXDataArray.size(); i < iEnd; ++i)
        {
            const PRXData& d = mPRXDataArray[i];

            uint32_t textSegmentBaseAddress = 0;
            uint32_t textSegmentSize        = 0;

            if(d.mSegmentCount > 0) // This should always be so.
            {
                // It appears that the first segment is always the .text (code) segment.
                textSegmentBaseAddress = (uint32_t)d.mPRXSegmentInfoArray[0].mBaseAddress;
                textSegmentSize        = (uint32_t)d.mPRXSegmentInfoArray[0].mMemorySize;
            }

            WriteStringFormatted(pOutput, "  %-40s PRX ID: 0x%08x Code base address: 0x%08x Code size: 0x%08x Version: %4u Path: \"%s\"\n", d.mName, d.mPRX_ID, textSegmentBaseAddress, textSegmentSize, d.mVersion, d.mPath);
            // To consider: Print additional PRX segment info.
        }

        WriteStringFormatted(pOutput, "\n");
    }

    if(traceFilterFlags & kTFPPUMemoryData)
    {
        // Mapped memory segments.
        WriteStringFormatted(pOutput, "Memory segments\n");

        for(eastl_size_t i = 0, iEnd = mPS3MemoryDump.mProcessMemoryDataArray.size(); i < iEnd; ++i)
        {
            const ProcessMemoryData&       d = mPS3MemoryDump.mProcessMemoryDataArray[i];
            const MemoryPageAttributeData& a = GetMemoryPageAttributeData(d.p_vaddr);

            FixedString8 s8;
            GetMemoryPageAttributeString(a.mAttribute, a.mAccessRight, s8);

            WriteStringFormatted(pOutput, "  Start: 0x%08I32x, end: 0x%08I32x, size: %9I32u, %s\n", 
                                (uint32_t)d.p_vaddr, (uint32_t)(d.p_vaddr + d.p_memsz), (uint32_t)d.p_memsz, s8.c_str());

        }

        WriteStringFormatted(pOutput, "\n");
    }
}


///////////////////////////////////////////////////////////////////////////////
// Trace
//
void PS3DumpFile::Trace(EA::IO::IStream* pOutput, int traceStrategy, int traceFilterFlags)
{
    using namespace Local;

    if(mbLoaded)
    {
        PS3DumpFile::CrashProcessor cp = GetCrashProcessor();

        if(((traceStrategy == kTSAutoDetect) || (traceStrategy == kTSCrashAnalysis)) && (cp != kCPNone)) // If the user wants to auto-choose and this dump file represents a crash of some sort...
        {
            // First trace just the most pertinent data.
            traceFilterFlags = kTFSummaryData | kTFCrashCauseData | kTFProcessData | kTFSkipCoreDumpThread;

            switch(cp)
            {
                case kCPPPU:
                    traceFilterFlags |= kTFPPUCallstackData;
                    traceFilterFlags |= kTFPPUAddressRep;
                    traceFilterFlags |= kTFPPUCrashDasm;
                    traceFilterFlags |= kTFPPUCrashSource;
                    traceFilterFlags |= kTFPPURegisterData;
                    break;

                case kCPSPU:
                    traceFilterFlags |= kTFSPUCallstackData;
                    traceFilterFlags |= kTFSPUAddressRep;
                    traceFilterFlags |= kTFSPUCrashDasm;
                    traceFilterFlags |= kTFSPUCrashSource;
                    traceFilterFlags |= kTFSPURegisterData;
                    break;

                case kCPRSX:
                    traceFilterFlags |= kTFRSXData;
                    break;

                case kCPNone:
                    break;
            }

            TraceInternal(pOutput, traceFilterFlags);

            // Then trace general information.
            traceFilterFlags = kTFSkipCoreDumpThread;

            switch(cp)
            {
                case kCPPPU:
                    traceFilterFlags |= kTFPRXData;
                    traceFilterFlags |= kTFPPUData;
                    traceFilterFlags |= kTFPPUSyncData;
                    traceFilterFlags |= kTFPPUMemoryData;
                    traceFilterFlags |= kTFPPUCallstackData;
                    traceFilterFlags |= kTFPPUAddressRep;
                    traceFilterFlags |= kTFPPUCrashDasm;
                    traceFilterFlags |= kTFPPUCrashSource;
                    traceFilterFlags |= kTFPPURegisterData;
                    break;

                case kCPSPU:
                    traceFilterFlags |= kTFSPUData;
                    traceFilterFlags |= kTFSPUCallstackData;
                    traceFilterFlags |= kTFSPUAddressRep;
                    traceFilterFlags |= kTFSPUCrashDasm;
                    traceFilterFlags |= kTFSPUCrashSource;
                    traceFilterFlags |= kTFSPURegisterData;
                    break;

                case kCPRSX:
                    traceFilterFlags |= kTFRSXData;
                    break;

                case kCPNone:
                    break;
            }

            WriteStringFormatted(pOutput, "------------------------------------------------------------------------------\n");
            WriteStringFormatted(pOutput, "General core dump information (including some repetition of the above data)...\n\n");

            TraceInternal(pOutput, traceFilterFlags);
        }
        else
        {
            if((traceStrategy == kTSCrashAnalysis) && (cp == kCPNone)) // If the user wanted to do a crash analysis but there was no crash...
                WriteStringFormatted(pOutput, "A crash analysis was requested, but the core dump doesn't represent a crash. Proceeding with general core dump trace...\n");

            // Otherwise we just do a 
            TraceInternal(pOutput, kTFDefault);
        }
    }
    else
    {
        WriteStringFormatted(pOutput, "Unable to trace. No PS3 dump file has been loaded.\n");
    }
}


///////////////////////////////////////////////////////////////////////////////
// Trace
//
void PS3DumpFile::Trace(eastl::string& sOutput, int traceStrategy, int traceFilterFlags)
{
    Local::StringStream ss(&sOutput);
    Trace(&ss, traceStrategy, traceFilterFlags);
}


///////////////////////////////////////////////////////////////////////////////
// GetCrashProcessor
//
PS3DumpFile::CrashProcessor PS3DumpFile::GetCrashProcessor() const
{
    using namespace PS3DumpELF;

    if(mCoreDumpCauseUnion.mCoreDumpCauseData.mCauseType == kCdctPPUException)
        return kCPPPU;

    if(mCoreDumpCauseUnion.mCoreDumpCauseData.mCauseType == kCdctSPUException)
        return kCPSPU;

    if(mCoreDumpCauseUnion.mCoreDumpCauseData.mCauseType == kCdctRSXException)
        return kCPRSX;

    return kCPNone;
}


///////////////////////////////////////////////////////////////////////////////
// GetCrashedPPUThreadID
//
uint64_t PS3DumpFile::GetCrashedPPUThreadID() const
{
    using namespace PS3DumpELF;

    if(mCoreDumpCauseUnion.mCoreDumpCauseData.mCauseType == kCdctPPUException)
        return mCoreDumpCauseUnion.mCoreDumpCausePPUException.mPPUThreadID;
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GetCrashedSPUThreadID
//
uint32_t PS3DumpFile::GetCrashedSPUThreadID() const
{
    using namespace PS3DumpELF;

    if(mCoreDumpCauseUnion.mCoreDumpCauseData.mCauseType == kCdctSPUException)
        return mCoreDumpCauseUnion.mCoreDumpCauseSPUException.mSPUThreadID;

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GetSPUThreadData
//
PS3DumpELF::SPUThreadData* PS3DumpFile::GetSPUThreadData(uint32_t spuThreadID)
{
    using namespace PS3DumpELF;

    for(eastl_size_t i = 0, iEnd = mSPUThreadDataArray.size(); i < iEnd; ++i)
    {
        SPUThreadData& std = mSPUThreadDataArray[i];

        if(std.mSPUThreadID == spuThreadID)
            return &std;
    }

    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// GetPPUThreadName
//
const char8_t* PS3DumpFile::GetPPUThreadName(uint64_t ppuThreadID) const
{
    using namespace PS3DumpELF;

    if(ppuThreadID == 0)
        return "<non-thread>";

    for(eastl_size_t i = 0, iEnd = mPPUThreadDataArray.size(); i < iEnd; ++i)
    {
        const PPUThreadData& ptd = mPPUThreadDataArray[i];

        if(ptd.mPPUThreadID == ppuThreadID)
            return ptd.mName;
    }

    return "<unknown>";
}


///////////////////////////////////////////////////////////////////////////////
// GetSPUThreadName
//
const char8_t* PS3DumpFile::GetSPUThreadName(uint32_t spuThreadID) const
{
    using namespace PS3DumpELF;

    if(spuThreadID == 0)
        return "<non-thread>";

    for(eastl_size_t i = 0, iEnd = mSPUThreadDataArray.size(); i < iEnd; ++i)
    {
        const SPUThreadData& std = mSPUThreadDataArray[i];

        if(std.mSPUThreadID == spuThreadID)
            return std.mName;
    }

    return "<unknown>";
}


///////////////////////////////////////////////////////////////////////////////
// GetPPUCallstack
//
size_t PS3DumpFile::GetPPUCallstack(uint64_t pReturnAddressArray[], size_t nReturnAddressArrayCapacity, uint64_t ppuThreadID) const
{
    using namespace PS3DumpELF;

    size_t                       nEntryIndex = 0;
    const PPUThreadRegisterData* pPPUThreadRegisterData = NULL;

    for(eastl_size_t i = 0, iEnd = mPPUThreadRegisterDataArray.size(); (i < iEnd) && !pPPUThreadRegisterData; ++i)
    {
        if(mPPUThreadRegisterDataArray[i].mPPUThreadID == ppuThreadID)
            pPPUThreadRegisterData = &mPPUThreadRegisterDataArray[i];
    }

    if(pPPUThreadRegisterData)
    {
        // We follow the same approach as the EA::Callstack::GetCallstack function on PS3.
        // Unlike that code, we might not be executing on the same platform. So we use uint64_t
        // values like they are pointers.

        uint64_t pFrame       = pPPUThreadRegisterData->mGPR[1];    // This is the address of a frame.       
        uint64_t pInstruction = pPPUThreadRegisterData->mPC;        // This is the address of an instruction.

        while(pFrame && (nEntryIndex < (nReturnAddressArrayCapacity - 1)))
        {
            pInstruction = (nEntryIndex == 0) ? pInstruction : (pInstruction - 4);
            pReturnAddressArray[nEntryIndex++] = pInstruction;

            const uint64_t pPrevFrame = pFrame;
            pFrame = mPS3MemoryDump.ReadUint64(pFrame);

            if(!pFrame || (pFrame & 15) || (pFrame < pPrevFrame))
                break;

            pInstruction = mPS3MemoryDump.ReadUint64(pFrame + 16);
        }
    }

    EA_ASSERT(nReturnAddressArrayCapacity >= 1);
    pReturnAddressArray[nEntryIndex] = 0;

    return nEntryIndex;
}


///////////////////////////////////////////////////////////////////////////////
// GetSPUCallstack
//
// struct SPUStackFrame {
//     const SPUStackFrame* pNext;
//     uint32_t             availableStackSize;
//     uint32_t             unused1;
//     uint32_t             unused2;
//     const char*          pReturnAddress;
// };
// 
size_t PS3DumpFile::GetSPUCallstack(uint32_t pReturnAddressArray[], size_t nReturnAddressArrayCapacity, uint32_t spuThreadID) const
{
    using namespace PS3DumpELF;

    size_t                       nEntryIndex = 0;
    const SPUThreadRegisterData* pSPUThreadRegisterData = NULL;

    for(eastl_size_t i = 0, iEnd = mSPUThreadRegisterDataArray.size(); (i < iEnd) && !pSPUThreadRegisterData; ++i)
    {
        if(mSPUThreadRegisterDataArray[i].mSPUThreadID == spuThreadID)
            pSPUThreadRegisterData = &mSPUThreadRegisterDataArray[i];
    }

    if(pSPUThreadRegisterData)
    {
        // We follow the same approach as the EA::Callstack::GetCallstack function on PS3 SPU.
        // Unlike that code, we might not be executing on the same platform. So we use uint32_t
        // values like they are pointers.

        uint32_t pFrame             = pSPUThreadRegisterData->mGPR[1].mWord[0];   // This is the address of a frame.       
        uint32_t pInstruction       = pSPUThreadRegisterData->mNPC;               // This is the address of an instruction.
        uint32_t pReturnAddress     = pSPUThreadRegisterData->mGPR[0].mWord[0];   // This is the address of an instruction.
        uint32_t relocationDistance = 0;                                          // Currently the PS3 dump file doesn't currently provide relocation information. I submitted a support request regarding this, but in the meantime symbol lookups won't work for relocated SPU code.

        if(pInstruction < 0x00040000) // Sanity check. Sometimes it seems to be invalid, though maybe there's a bug in our code.
            pReturnAddressArray[nEntryIndex++] = pInstruction - relocationDistance;

        // The return address is suppsoed to be in GPR0.word0, but I'm sometimes seeing this value as zero (which it shouldn't be) whereas the current SPUStackFrame pReturnAddress value is sane.
        if(pReturnAddress == 0)
            pReturnAddress = mPS3SPUMemoryDump.ReadUint32(spuThreadID, pFrame + 16);

        for(pFrame = mPS3SPUMemoryDump.ReadUint32(spuThreadID, pFrame);                         // Same as: pFrame = pFrame->pNext
            pFrame && (pFrame <= 0x0003fff0) && (nEntryIndex < (nReturnAddressArrayCapacity - 2)); 
            pReturnAddress = mPS3SPUMemoryDump.ReadUint32(spuThreadID, pFrame + 16),            // Same as: pReturnAddress = pFrame->pReturnAddress
            pFrame = mPS3SPUMemoryDump.ReadUint32(spuThreadID, pFrame))                         // Same as: pFrame = pFrame->pNext
        {
            // The call address is often a single 32 bit instruction prior to the return address.
            // Sometimes it is more instructions prior; uncommonly it can be instructions later.
            if(pReturnAddress < 0x00040000) // Sanity check. Sometimes it seems to be invalid, though maybe there's a bug in our code.
                pReturnAddressArray[nEntryIndex++] = (pReturnAddress - 4) - relocationDistance;
        }
    }

    EA_ASSERT(nReturnAddressArrayCapacity >= 1);
    pReturnAddressArray[nEntryIndex] = 0;

    return nEntryIndex;
}


///////////////////////////////////////////////////////////////////////////////
// GetUnknownAddressDescription
//
// Returns true if the address was found in an unregistered PRX.
//
bool PS3DumpFile::GetUnknownAddressDescription(uint64_t address, FixedString& sUnknownAddressDescription)
{
    using namespace PS3DumpELF;

    for(eastl_size_t i = 0, iEnd = mPRXDataArray.size(); i < iEnd; i++)
    {
        const PRXData& prxData = mPRXDataArray[i];

        if(prxData.mpTextSegment) // This will nearly always be true.
        {
            if((address >=  prxData.mpTextSegment->mBaseAddress) && // If it's from this PRX...
               (address <  (prxData.mpTextSegment->mBaseAddress + prxData.mpTextSegment->mMemorySize)))
            {
                // System PRXs have paths like: "/dev_flash/sys/external/liblv2.sprx"
                const bool bSystemPRX = (EA::StdC::Strstr(prxData.mPath, "/dev_flash") == prxData.mPath); // Is there another way to tell a system PRX?

                sUnknownAddressDescription.sprintf("Unknown function from %s PRX %s (%s). ", bSystemPRX ? "system" : "user", prxData.mName, prxData.mPath);

                if(!bSystemPRX)
                    sUnknownAddressDescription.append("You can register .prx/.sprx/.map files with this core dump reader.");
                
                return true;
            }
        }
    }

    sUnknownAddressDescription.append("Unknown function");
    return false;
}


///////////////////////////////////////////////////////////////////////////////
// GetPPUThreadDeadlock
//
// This function searches for a deadlock that the given thread id is involved in.
//
// It works by seeing if there is circularity in the chain of threads waiting 
// for locks. For example:
//
//             |   Lock A      Lock B      Lock C
//    ---------|-----------------------------------
//    Owner    |  Thread 1    Thread 2    Thread 3
//    Waiter 1 |  Thread 3    Thread 1    Thread 2
//    Waiter 2 |  
//
// In this case we have a three-way deadlock (rare but possible). To auto-detect
// this deadlock. We start with Thread 1 which owns Lock A. Now look for any 
// lock which Thread 1 is waiting for. If there is one, see if the owning thread
// for that lock is a waiter for Lock A. If not, see if that owning thread is 
// waiting on any other locks. If so, see if the owning thread of that lock is 
// a waiter for Lock A. This is an algorithm which lends itself to a recursive 
// implementation, though deadlocks beyond two-way are rare and deadlocks beyond 
// three-way are virtually unheard-of. 
//
// FindDeadlockRecursive algorithm:
//    See if the owner of SyncInfoCurrent is a waiter in syncInfoEndpoint.
//    If so then we have a deadlock. Return true.
//    If not then see if the owner of SyncInfoCurrent is a waiter on any other lock.
//    If so, then call this function recursively with that other lock as syncInfoCurrent.
//    If not then return false.

uint32_t PS3DumpFile::GetPPUThreadDeadlock(uint64_t ppuThreadID, PS3DumpELF::SyncInfo* pSyncInfoChainResult, size_t syncInfoChainCapacity) const
{
    eastl::vector<PS3DumpELF::SyncInfo> lockArray; // This is a list of zero or more locks that ppuThreadID owns. We check each one for a deadlock loop.

    GetPPUThreadSyncInfo(ppuThreadID, &lockArray, NULL);

    for(eastl_size_t i = 0, iEnd = lockArray.size(); i != iEnd; ++i)
    {
        size_t syncInfoChainSize = 0;

        if(FindDeadlockRecursive(lockArray[i], lockArray[i], pSyncInfoChainResult, syncInfoChainCapacity, syncInfoChainSize))
            return (uint32_t)syncInfoChainSize;
    }

    return 0;
}

// This is a helper function for GetPPUThreadDeadlock.
bool PS3DumpFile::FindDeadlockRecursive(const PS3DumpELF::SyncInfo& syncInfoCurrent, const PS3DumpELF::SyncInfo& syncInfoEndpoint, 
                                        PS3DumpELF::SyncInfo* pSyncInfoChain, size_t syncInfoChainCapacity, size_t& syncInfoChainSize) const
{
    PS3DumpELF::SyncInfo syncInfo;

    pSyncInfoChain[syncInfoChainSize++] = syncInfoCurrent;

    if(ThreadIsWaitingOnSyncInfo(syncInfoCurrent.mOwnerThreadID, syncInfoEndpoint))
        return true;

    if(ThreadIsWaitingOnAnySyncInfo(syncInfoCurrent.mOwnerThreadID, syncInfo))
        return FindDeadlockRecursive(syncInfo, syncInfoEndpoint,  pSyncInfoChain, syncInfoChainCapacity, syncInfoChainSize);

    return false;
}

///////////////////////////////////////////////////////////////////////////////
// ThreadIsWaitingOnSyncInfo
//
// Returns true if the given thread is waiting on the given syncInfo.
//
bool PS3DumpFile::ThreadIsWaitingOnSyncInfo(uint64_t ppuThreadID, const PS3DumpELF::SyncInfo& syncInfo) const
{
    // This isn't the fastest way to solve this, but it's probably fast enough and re-uses existing functionality.
    PS3DumpELF::SyncInfoArray syncInfoArray;

    GetPPUThreadSyncInfo(ppuThreadID, NULL, &syncInfoArray); // Get a list of all synchronization primitives the thread is waiting on. This is almost always going to be just zero or one.

    if(!syncInfoArray.empty())
        return (syncInfo.mID == syncInfoArray[0].mID);

    return false;
}


///////////////////////////////////////////////////////////////////////////////
// ThreadIsWaitingOnSyncInfo
//
// Returns true if the given thread is waiting on any syncInfo. Fills in the 
// SyncInfo if so.
//
bool PS3DumpFile::ThreadIsWaitingOnAnySyncInfo(uint64_t ppuThreadID, PS3DumpELF::SyncInfo& syncInfo) const
{
    // This isn't the fastest way to solve this, but it's probably fast enough and re-uses existing functionality.
    PS3DumpELF::SyncInfoArray syncInfoArray;

    GetPPUThreadSyncInfo(ppuThreadID, NULL, &syncInfoArray); // Get a list of all synchronization primitives the thread is waiting on. This is almost always going to be just zero or one.

    if(!syncInfoArray.empty())
    {
        syncInfo = syncInfoArray[0];
        return true;
    }

    return false;
}


///////////////////////////////////////////////////////////////////////////////
// GetPPUThreadSyncInfo
//
void PS3DumpFile::GetPPUThreadSyncInfo(uint64_t ppuThreadID, PS3DumpELF::SyncInfoArray* pLockArray, PS3DumpELF::SyncInfoArray* pWaitArray) const
{
    using namespace PS3DumpELF;

    eastl_size_t i, iEnd;

    if(ppuThreadID == 0)
        return;

    // Mutex
    for(i = 0, iEnd = mMutexDataArray.size(); i != iEnd; ++i)
    {
        const MutexData& d = mMutexDataArray[i];

        if(pLockArray)
        {
            if(d.mOwnerThreadID == ppuThreadID)
            {
                pLockArray->push_back();
                CopySyncInfo(pLockArray->back(), d);
            }
        }

        if(pWaitArray)
        {
            Uint64Array::const_iterator it = eastl::find(d.mWaitThreadIDArray.begin(), d.mWaitThreadIDArray.end(), ppuThreadID);

            if(it != d.mWaitThreadIDArray.end())
            {
                pWaitArray->push_back();
                CopySyncInfo(pWaitArray->back(), d);
            }
        }
    }

    // LWMutex
    for(i = 0, iEnd = mLWMutexDataArray.size(); i != iEnd; ++i)
    {
        const LWMutexData& d = mLWMutexDataArray[i];

        if(pLockArray)
        {
            if(d.mOwnerThreadID == ppuThreadID)
            {
                pLockArray->push_back();
                CopySyncInfo(pLockArray->back(), d);
            }
        }

        if(pWaitArray)
        {
            Uint64Array::const_iterator it = eastl::find(d.mWaitThreadIDArray.begin(), d.mWaitThreadIDArray.end(), ppuThreadID);

            if(it != d.mWaitThreadIDArray.end())
            {
                pWaitArray->push_back();
                CopySyncInfo(pWaitArray->back(), d);
            }
        }
    }

    // Semaphore
    for(i = 0, iEnd = mSemaphoreDataArray.size(); i != iEnd; ++i)
    {
        const SemaphoreData& d = mSemaphoreDataArray[i];

        // Semaphores can only be waited on; they can't be owned.

        if(pWaitArray)
        {
            Uint64Array::const_iterator it = eastl::find(d.mWaitThreadIDArray.begin(), d.mWaitThreadIDArray.end(), ppuThreadID);

            if(it != d.mWaitThreadIDArray.end())
            {
                pWaitArray->push_back();
                CopySyncInfo(pWaitArray->back(), d);
            }
        }
    }

    // EventQueue
    for(i = 0, iEnd = mEventQueueDataArray.size(); i != iEnd; ++i)
    {
        const EventQueueData& d = mEventQueueDataArray[i];

        // Event queues can only be waited on; they can't be owned.

        if(pWaitArray)
        {
            Uint64Array::const_iterator it = eastl::find(d.mWaitThreadIDArray.begin(), d.mWaitThreadIDArray.end(), ppuThreadID);

            if(it != d.mWaitThreadIDArray.end())
            {
                pWaitArray->push_back();
                CopySyncInfo(pWaitArray->back(), d);
            }
        }
    }

    // ConditionVariable
    for(i = 0, iEnd = mConditionVariableDataArray.size(); i != iEnd; ++i)
    {
        const ConditionVariableData& d = mConditionVariableDataArray[i];

        // Condition variables can only be waited on; they can't be owned.

        if(pWaitArray)
        {
            Uint64Array::const_iterator it = eastl::find(d.mWaitThreadIDArray.begin(), d.mWaitThreadIDArray.end(), ppuThreadID);

            if(it != d.mWaitThreadIDArray.end())
            {
                pWaitArray->push_back();
                CopySyncInfo(pWaitArray->back(), d);
            }
        }
    }

    // LWConditionVariable
    for(i = 0, iEnd = mLWConditionVariableDataArray.size(); i != iEnd; ++i)
    {
        const LWConditionVariableData& d = mLWConditionVariableDataArray[i];

        // Condition variables can only be waited on; they can't be owned.

        if(pWaitArray)
        {
            Uint64Array::const_iterator it = eastl::find(d.mWaitThreadIDArray.begin(), d.mWaitThreadIDArray.end(), ppuThreadID);

            if(it != d.mWaitThreadIDArray.end())
            {
                pWaitArray->push_back();
                CopySyncInfo(pWaitArray->back(), d);
            }
        }
    }

    // RWLock
    for(i = 0, iEnd = mRWLockDataArray.size(); i != iEnd; ++i)
    {
        const RWLockData& d = mRWLockDataArray[i];

        if(pLockArray)
        {
            // Note by Paul Pedriana: As of this writing (SDK 280), the RWLockData core dump struct defined by 
            // Sony appears to be flawed, as it doesn't account for the fact that there may be multiple owners
            // of a read lock. It recognizes only a single owner thread for a RWLock, which is true only for 
            // the case of being a write lock.

            if(d.mOwnerThreadID == ppuThreadID)
            {
                pLockArray->push_back();
                CopySyncInfo(pLockArray->back(), d);
            }
        }

        if(pWaitArray)
        {
            Uint64Array::const_iterator it = eastl::find(d.mReaderWaitThreadIDArray.begin(), d.mReaderWaitThreadIDArray.end(), ppuThreadID);

            if(it != d.mReaderWaitThreadIDArray.end())
            {
                pWaitArray->push_back();
                CopySyncInfo(pWaitArray->back(), d);
            }

            it = eastl::find(d.mWriterWaitThreadIDArray.begin(), d.mWriterWaitThreadIDArray.end(), ppuThreadID);

            if(it != d.mWriterWaitThreadIDArray.end())
            {
                pWaitArray->push_back();
                CopySyncInfo(pWaitArray->back(), d);
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
// GetWaitingThreads
//
uint32_t PS3DumpFile::GetWaitingThreads(const PS3DumpELF::SyncInfo& syncInfo, Uint64Array& threadArray) const
{
    using namespace PS3DumpELF;

    switch (syncInfo.mSyncType)
    {
        case kSTMutex:
            threadArray = static_cast<const MutexData*>(syncInfo.mpCPSH)->mWaitThreadIDArray;
            break;

        case kSTConditionVariable:
            threadArray = static_cast<const ConditionVariableData*>(syncInfo.mpCPSH)->mWaitThreadIDArray;
            break;

        case kSTRWLock:
        {
            threadArray = static_cast<const RWLockData*>(syncInfo.mpCPSH)->mReaderWaitThreadIDArray;

            const Uint64Array& waitArray = static_cast<const RWLockData*>(syncInfo.mpCPSH)->mWriterWaitThreadIDArray;
            threadArray.insert(threadArray.end(), waitArray.begin(), waitArray.end());
            break;
        }

        case kSTLWMutex:
            threadArray = static_cast<const LWMutexData*>(syncInfo.mpCPSH)->mWaitThreadIDArray;
            break;

        case kSTEventQueue:
            threadArray = static_cast<const EventQueueData*>(syncInfo.mpCPSH)->mWaitThreadIDArray;
            break;

        case kSTSemaphore:
            threadArray = static_cast<const SemaphoreData*>(syncInfo.mpCPSH)->mWaitThreadIDArray;
            break;

        case kSTLWConditionVariable:
            threadArray = static_cast<const LWConditionVariableData*>(syncInfo.mpCPSH)->mWaitThreadIDArray;
            break;

        case kSTEvent:
            threadArray = static_cast<const EventQueueData*>(syncInfo.mpCPSH)->mWaitThreadIDArray;
            break;
    }

    return (uint32_t)threadArray.size();
}    


///////////////////////////////////////////////////////////////////////////////
// GetSPUThreadGroupData
//
const PS3DumpELF::SPUThreadGroupData& PS3DumpFile::GetSPUThreadGroupData(uint32_t spuThreadGroupID) const
{
    using namespace PS3DumpELF;

    for(eastl_size_t i = 0, iEnd = mSPUThreadGroupDataArray.size(); i != iEnd; ++i)
    {
        const SPUThreadGroupData& tgd = mSPUThreadGroupDataArray[i];

        if(spuThreadGroupID == tgd.mSPUThreadGroupID)
            return tgd;
    }

    return mSPUThreadGroupDataDefault;
}


///////////////////////////////////////////////////////////////////////////////
// GetMemoryPageAttributeData
//
const PS3DumpELF::MemoryPageAttributeData& PS3DumpFile::GetMemoryPageAttributeData(uint64_t startingAddress) const
{
    using namespace PS3DumpELF;

    for(eastl_size_t i = 0, iEnd = mMemoryPageAttributeDataArray.size(); i != iEnd; ++i)
    {
        const MemoryPageAttributeData& pad = mMemoryPageAttributeDataArray[i];

        if(startingAddress == pad.mMapAddress)
            return pad;
    }

    return mMemoryPageAttributeDataDefault;
}


///////////////////////////////////////////////////////////////////////////////
// GetSPULookup
//
IAddressRepLookup* PS3DumpFile::GetSPULookup(const char8_t* pELFFilePath8)
{
    size_t count = mAddressRepLookupSetSPU.GetDatabaseFileCount();

    for(size_t i = 0; i != count; ++i)
    {
        IAddressRepLookup* pLookup = mAddressRepLookupSetSPU.GetDatabaseFile(i);

        const wchar_t* pELFFilePathCurrentW = pLookup->GetDatabasePath();
        const wchar_t* pELFFileNameCurrentW = EA::IO::Path::GetFileName(pELFFilePathCurrentW);
        const char8_t* pELFFileName8        = EA::IO::Path::GetFileName(pELFFilePath8);

        // EAIO should have functionality like the following (copy 8 bit UTF8 to wchar_t string).
        EA::IO::Path::PathStringW sELFFileNameW((eastl_size_t)EA::StdC::Strlen(pELFFileName8), 0);
        size_t n = (size_t)EA::StdC::Strlcpy(&sELFFileNameW[0], pELFFileName8, sELFFileNameW.size());

        if(n > sELFFileNameW.size())
        {
            sELFFileNameW.resize((eastl_size_t)n);
            EA::StdC::Strlcpy(&sELFFileNameW[0], pELFFileName8, sELFFileNameW.size());
        }

        if(sELFFileNameW.comparei(pELFFileNameCurrentW) == 0)
            return pLookup;
    }

    //if(count) // We err conservatively by giving the user the first one found. Perhaps there was a misspelling. Perhaps we shouldn't do this.
    //    return mAddressRepLookupSetSPU.GetDatabaseFile(0);

    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// CopySyncInfo
//
void PS3DumpFile::CopySyncInfo(PS3DumpELF::SyncInfo& syncInfo, const PS3DumpELF::MutexData& d)
{
    using namespace PS3DumpELF;

    syncInfo.mpCPSH         = &d;
    syncInfo.mSyncType      = kSTMutex;
    syncInfo.mID            = d.mID;
    syncInfo.mOwnerThreadID = d.mOwnerThreadID;
    strcpy(syncInfo.mName, d.mName);
}


///////////////////////////////////////////////////////////////////////////////
// CopySyncInfo
//
void PS3DumpFile::CopySyncInfo(PS3DumpELF::SyncInfo& syncInfo, const PS3DumpELF::LWMutexData& d)
{
    using namespace PS3DumpELF;

    syncInfo.mpCPSH         = &d;
    syncInfo.mSyncType      = kSTLWMutex;
    syncInfo.mID            = d.mID;
    syncInfo.mOwnerThreadID = d.mOwnerThreadID;
    strcpy(syncInfo.mName, d.mName);
}


///////////////////////////////////////////////////////////////////////////////
// CopySyncInfo
//
void PS3DumpFile::CopySyncInfo(PS3DumpELF::SyncInfo& syncInfo, const PS3DumpELF::SemaphoreData& d)
{
    using namespace PS3DumpELF;

    syncInfo.mpCPSH         = &d;
    syncInfo.mSyncType      = kSTSemaphore;
    syncInfo.mID            = d.mID;
    syncInfo.mOwnerThreadID = 0;
    strcpy(syncInfo.mName, d.mName);
}


///////////////////////////////////////////////////////////////////////////////
// CopySyncInfo
//
void PS3DumpFile::CopySyncInfo(PS3DumpELF::SyncInfo& syncInfo, const PS3DumpELF::EventQueueData& d)
{
    using namespace PS3DumpELF;

    syncInfo.mpCPSH         = &d;
    syncInfo.mSyncType      = kSTEventQueue;
    syncInfo.mID            = d.mID;
    syncInfo.mOwnerThreadID = 0;
    strcpy(syncInfo.mName, d.mName);
}


///////////////////////////////////////////////////////////////////////////////
// CopySyncInfo
//
void PS3DumpFile::CopySyncInfo(PS3DumpELF::SyncInfo& syncInfo, const PS3DumpELF::ConditionVariableData& d)
{
    using namespace PS3DumpELF;

    syncInfo.mpCPSH         = &d;
    syncInfo.mSyncType      = kSTConditionVariable;
    syncInfo.mID            = d.mID;
    syncInfo.mOwnerThreadID = 0;
    strcpy(syncInfo.mName, d.mName);
}


///////////////////////////////////////////////////////////////////////////////
// CopySyncInfo
//
void PS3DumpFile::CopySyncInfo(PS3DumpELF::SyncInfo& syncInfo, const PS3DumpELF::LWConditionVariableData& d)
{
    using namespace PS3DumpELF;

    syncInfo.mpCPSH         = &d;
    syncInfo.mSyncType      = kSTConditionVariable;
    syncInfo.mID            = d.mID;
    syncInfo.mOwnerThreadID = 0;
    strcpy(syncInfo.mName, d.mName);
}


///////////////////////////////////////////////////////////////////////////////
// CopySyncInfo
//
void PS3DumpFile::CopySyncInfo(PS3DumpELF::SyncInfo& syncInfo, const PS3DumpELF::RWLockData& d)
{
    using namespace PS3DumpELF;

    syncInfo.mpCPSH         = &d;
    syncInfo.mSyncType      = kSTRWLock;
    syncInfo.mID            = d.mID;
    syncInfo.mOwnerThreadID = d.mOwnerThreadID;
    strcpy(syncInfo.mName, d.mName);
}


///////////////////////////////////////////////////////////////////////////////
// Load
//
void PS3DumpFile::Load()
{
    using namespace EA::IO;
    using namespace EA::Callstack::ELF;

    if(mbInitialized && !mbLoaded)
    {
        mbLoaded = true;

        // ELF64_Phdr (64 bit program segment header)
        // There is an array of (mELFFile.mHeader.e_phnum) ELF64_Phdr structs, each of 
        // size (mELFFile.mHeader.e_phentsize) at position (mELFFile.mHeader.e_phoff) within the file.
        // For each struct, its ELF64_Phdr.p_offset value identifies where the Program Segment is 
        // in the ELF file. The format of the program segment is defined by Sony and is specific to 
        // the PS3 dump file format. Sony defines a number of program segment types, though each 
        // of them shares the same first 48 bytes. See the Common Program Segment Headers section of
        // Sony's dump file document for documentation of this 48 bytes and all the segment types.

        eastl::vector<ELF::ELF64_Phdr> programHeaderArray;

        programHeaderArray.resize(mELFFile.mHeader.e_phnum);
        mpStream->SetPosition(64); // 64 is the offset of the first program segment header entry.

        for(uint32_t i = 0; i < mELFFile.mHeader.e_phnum; i++)
        {
            ELF::ELF64_Phdr& ph = programHeaderArray[i];

            ReadUint32(mpStream, ph.p_type,   kEndianBig);
            ReadUint32(mpStream, ph.p_flags,  kEndianBig);
            ReadUint64(mpStream, ph.p_offset, kEndianBig);
            ReadUint64(mpStream, ph.p_vaddr,  kEndianBig);
            ReadUint64(mpStream, ph.p_paddr,  kEndianBig);
            ReadUint64(mpStream, ph.p_filesz, kEndianBig);
            ReadUint64(mpStream, ph.p_memsz,  kEndianBig);
            ReadUint64(mpStream, ph.p_align,  kEndianBig);

            // Move to the next entry, which is right after the current one.
            // I don't think the following should be necessary. I think e_phentsize is supposed to 
            // always equal the size of the ELF64_Phdr members (usually sizeof(ELF64_Phdr)).  
            // mpStream->SetPosition(64 + (i * mELFFile.mHeader.e_phentsize));
        }

        // Now we have all the ELF64_Phdr entries in an array (programHeaderArray).
        for(eastl_size_t j = 0, jEnd = programHeaderArray.size(); j < jEnd; j++)
        {
            const ELF::ELF64_Phdr& ph = programHeaderArray[j];

            // We have either a PT_NOTE segment or a PT_LOAD segment. All segments except for memory dump segments
            // are of the PT_NOTE type. These PT_NOTE segments are all based on the CommonProgramSegmentHeader struct.
            // We need to determine here which of the segment types we are dealing with and load based on the type.

            if(ph.p_type == PT_NOTE)
                Load_PT_NOTE(ph);
            else if(ph.p_type == PT_LOAD)
                Load_PT_LOAD(ph);
            else
            {
                // Else the type is unexpected. Do we have a corrupt file or are we working with a new version of the PS3 dump file format?  
                EA_FAIL_FORMATTED(("PS3DumpFile::Load: Unknown type: %u\n", (unsigned)ph.p_type));
            }
        }
    }
}


bool PS3DumpFile::Load_PT_NOTE(const ELF::ELF64_Phdr& ph)
{
    using namespace EA::IO;
    using namespace EA::Callstack::PS3DumpELF;
    using namespace Local;

    // We create a temporary instance of this so we can load into it before loading the rest of the record.
    PS3DumpELF::CommonProgramSegmentHeader cpsh;

    if(mpStream->SetPosition((EA::IO::off_type)ph.p_offset))
    {
        // Since all the segment types below use the same header, we read the header here.
        ReadUint32(mpStream, cpsh.mNameSize,        kEndianBig);
        ReadUint32(mpStream, cpsh.mDescriptionSize, kEndianBig);
        ReadUint32(mpStream, cpsh.mSegmentType,     kEndianBig);
        ReadChar8 (mpStream, cpsh.mSegmentName,     sizeof(cpsh.mSegmentName));
        ReadUint32(mpStream, cpsh.mFixedSize,       kEndianBig);
        ReadUint32(mpStream, cpsh.mUnitCount,       kEndianBig);
        ReadBucket(mpStream, 12);

        switch(cpsh.mSegmentType)
        {
            case SYSTEM_INFO:
                LoadSystemData(cpsh);
                break;

            case PPU_EXCEP_INFO:
                // This format is deprecated and so we ignore it.
                break;

            case PROCESS_INFO:
                LoadProcessData(cpsh);
                break;

            case PPU_REG_INFO:
                LoadPPUThreadRegisterData(cpsh);
                break;

            case SPU_REG_INFO:
                LoadSPUThreadRegisterData(cpsh);
                break;

            case PPU_THR_INFO:
                LoadPPUThreadData(cpsh);
                break;

            case DUMP_CAUSE_INFO:
                LoadCoreDumpCause(cpsh);
                break;

            case SPU_LS_INFO:
                LoadSPULocalStorageData(cpsh);
                break;

            case SPU_THRGRP_INFO:
                LoadSPUThreadGroupData(cpsh);
                break;

            case SPU_THR_INFO:
                LoadSPUThreadData(cpsh);
                break;

            case PRX_INFO:
                LoadPRXData(cpsh);
                break;

            case PAGE_ATTR_INFO:
                LoadMemoryPageAttributeData(cpsh);
                break;

            case MUTEX_INFO:
                LoadMutexData(cpsh);
                break;

            case CONDVAR_INFO:
                LoadConditionVariableData(cpsh);
                break;

            case RWLOCK_INFO:
                LoadRWLockData(cpsh);
                break;

            case LWMUTEX_INFO:
                LoadLWMutexData(cpsh);
                break;

            case EVENTQUEUE_INFO:
                LoadEventQueueData(cpsh);
                break;

            case SEMAPHORE_INFO:
                LoadSemaphoreData(cpsh);
                break;

            case LWCONDVAR_INFO:
                LoadLWConditionVariableData(cpsh);
                break;

            case CONTAINER_INFO:
                LoadMemoryContainerData(cpsh);
                break;

            case EVENTFLAG_INFO:
                LoadEventFlagData(cpsh);
                break;

            case RSX_DEBUG_INFO:
                LoadRSXDebugData(cpsh);
                break;

            case MEM_STAT_INFO:
                LoadMemoryStatisticsData(cpsh);
                break;

            default:
                EA_FAIL_FORMATTED(("PS3DumpFile::Load_PT_NOTE: Unknown type: %u\n", (unsigned)cpsh.mSegmentType));
                break;
        }
    }

    return true;
}


bool PS3DumpFile::Load_PT_LOAD(const ELF::ELF64_Phdr& ph)
{
    // The PS3 Dump file has only one type of segment of type PT_LOAD: a memory dump segment.
    EA_ASSERT(ph.p_vaddr && ph.p_memsz);
    
    PS3DumpELF::ProcessMemoryData pmd;

    // There is no data for a memory dump segment, there is only the memory itself.
    memcpy(&pmd, &ph, sizeof ph);

    mPS3MemoryDump.AddProcessMemoryData(pmd);

    return true;
}


bool PS3DumpFile::LoadSystemData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;

    memcpy(&mSystemData, &cpsh, sizeof(cpsh));

    ReadUint8(mpStream, mSystemData.mPSID, sizeof(mSystemData.mPSID));
    ReadUint32(mpStream, mSystemData.mCorefileVersion, kEndianBig);
    EA_ASSERT(mSystemData.mCorefileVersion >= 3); // 3 means PS3 SDK 250 or later. Older versions have some core dump stream differences of significance.

    return true;
}


bool PS3DumpFile::LoadProcessData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace Local;

    memcpy(&mSystemData, &cpsh, sizeof(cpsh));

    ReadUint32(mpStream, mProcessData.mStatus,                kEndianBig);
    ReadUint32(mpStream, mProcessData.mPPUThreadCount,        kEndianBig);
    ReadUint32(mpStream, mProcessData.mSPUThreadGroupCount,   kEndianBig);
    ReadUint32(mpStream, mProcessData.mRawSPUCount,           kEndianBig);
    ReadUint32(mpStream, mProcessData.mParentProcessID,       kEndianBig);
    ReadUint64(mpStream, mProcessData.mMaxPhysicalMemorySize, kEndianBig);
    ReadChar8 (mpStream, mProcessData.mPath,                  sizeof(mProcessData.mPath));
    ReadUint8 (mpStream, mProcessData.mPPUGuidID,             sizeof(mProcessData.mPPUGuidID));
    ReadUint8 (mpStream, mProcessData.mPPUGuidRevision);
    ReadUint8 (mpStream, mProcessData.mPPUGuidReserved1,      sizeof(mProcessData.mPPUGuidReserved1));
    ReadUint8 (mpStream, mProcessData.mPPUGuidValue,          sizeof(mProcessData.mPPUGuidValue));
    ReadUint8 (mpStream, mProcessData.mPPUGuidReserved2,      sizeof(mProcessData.mPPUGuidReserved2));

    return true;
}


bool PS3DumpFile::LoadPPUThreadRegisterData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace Local;

    EA_ASSERT(cpsh.mUnitCount < 4096); // Do a sanity check.

    if(cpsh.mUnitCount < 4096)
    {
        mPPUThreadRegisterDataArray.resize(cpsh.mUnitCount);

        memcpy(&mPPUThreadRegisterDataArray[0], &cpsh, sizeof(cpsh));

        for(uint32_t i = 0; i < cpsh.mUnitCount; i++)
        {
            PS3DumpELF::PPUThreadRegisterData& trd = mPPUThreadRegisterDataArray[i];

            ReadUint64(mpStream, trd.mPPUThreadID,         kEndianBig);
            ReadUint64(mpStream, trd.mGPR,             32, kEndianBig);
            ReadDouble(mpStream, &trd.mFPR[0].mDouble, 32, kEndianBig);
            ReadUint64(mpStream, trd.mPC,                  kEndianBig);
            ReadUint32(mpStream, trd.mCR,                  kEndianBig);
            ReadUint64(mpStream, trd.mLR,                  kEndianBig);
            ReadUint64(mpStream, trd.mCTR,                 kEndianBig);
            ReadUint64(mpStream, trd.mXER,                 kEndianBig);
            ReadUint32(mpStream, trd.mFPSCR,               kEndianBig);
            ReadUint128(mpStream, trd.mVSCR,               kEndianBig);
            ReadVMXReg(mpStream, trd.mVMX,             32, kEndianBig);
            ReadBucket(mpStream, 16);
        }

        return true;
    }

    return false;
}


bool PS3DumpFile::LoadSPUThreadRegisterData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace Local;

    EA_ASSERT(cpsh.mUnitCount < 4096); // Do a sanity check.

    if(cpsh.mUnitCount < 4096)
    {
        mSPUThreadRegisterDataArray.resize(cpsh.mUnitCount);

        memcpy(&mSPUThreadRegisterDataArray[0], &cpsh, sizeof(cpsh));

        for(uint32_t i = 0; i < cpsh.mUnitCount; i++)
        {
            PS3DumpELF::SPUThreadRegisterData& srd = mSPUThreadRegisterDataArray[i];

            ReadUint32(mpStream, srd.mSPUThreadGroupID,      kEndianBig);
            ReadUint32(mpStream, srd.mSPUThreadID,           kEndianBig);
            ReadSPUReg(mpStream, srd.mGPR,              128, kEndianBig);
            ReadUint32(mpStream, srd.mNPC,                   kEndianBig);
            ReadUint128(mpStream, srd.mFPSCR,                kEndianBig);
            ReadUint32(mpStream, srd.mDecrementer,           kEndianBig);
            ReadUint32(mpStream, srd.mSRR0,                  kEndianBig);
            ReadUint32(mpStream, srd.mMB_STAT,               kEndianBig);
            ReadUint32(mpStream, srd.mSPU_MB,            4,  kEndianBig);
            ReadUint32(mpStream, srd.mPPU_MB,                kEndianBig);
            ReadUint32(mpStream, srd.mSPU_STATUS,            kEndianBig);
            ReadUint64(mpStream, srd.mSPU_CFG,               kEndianBig);
            ReadUint64(mpStream, srd.mMFC_CQ_SR,        96,  kEndianBig);
            ReadBucket(mpStream, 24);
        }

        return true;
    }

    return false;
}


bool PS3DumpFile::LoadPPUThreadData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace Local;

    EA_ASSERT(cpsh.mUnitCount < 4096); // Do a sanity check.

    if(cpsh.mUnitCount < 4096)
    {
        mPPUThreadDataArray.resize(cpsh.mUnitCount);

        memcpy(&mPPUThreadDataArray[0], &cpsh, sizeof(cpsh));

        for(uint32_t i = 0; i < cpsh.mUnitCount; i++)
        {
            PS3DumpELF::PPUThreadData& td = mPPUThreadDataArray[i];

            ReadUint64(mpStream, td.mPPUThreadID,     kEndianBig);
            ReadUint32(mpStream, td.mPriority,        kEndianBig);
            ReadUint32(mpStream, td.mState,           kEndianBig);
            ReadUint64(mpStream, td.mStackAddress,    kEndianBig);
            ReadUint64(mpStream, td.mStackSize,       kEndianBig);
            ReadChar8 (mpStream, td.mName,        128);
            ReadBucket(mpStream, 16);
        }

        return true;
    }

    return false;
}


bool PS3DumpFile::LoadCoreDumpCause(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace EA::Callstack::PS3DumpELF;
    using namespace Local;

    memcpy(&mCoreDumpCauseUnion.mCoreDumpCauseData, &cpsh, sizeof(cpsh));

    ReadUint32(mpStream, mCoreDumpCauseUnion.mCoreDumpCauseData.mCauseType, kEndianBig);
    ReadUint32(mpStream, mCoreDumpCauseUnion.mCoreDumpCauseData.mOrder,     kEndianBig);

    switch (mCoreDumpCauseUnion.mCoreDumpCauseData.mCauseType)
    {
        case kCdctPPUException:
            ReadUint64(mpStream, mCoreDumpCauseUnion.mCoreDumpCausePPUException.mPPUExceptionType, kEndianBig);
            ReadUint32(mpStream, mCoreDumpCauseUnion.mCoreDumpCausePPUException.mProcessID,        kEndianBig);
            ReadUint64(mpStream, mCoreDumpCauseUnion.mCoreDumpCausePPUException.mPPUThreadID,      kEndianBig);
            ReadUint64(mpStream, mCoreDumpCauseUnion.mCoreDumpCausePPUException.mDAR,              kEndianBig);
            ReadUint64(mpStream, mCoreDumpCauseUnion.mCoreDumpCausePPUException.mPC,               kEndianBig);
            // We can ignore the mReserved field, as we don't need to read anything after it.
            break;

        case kCdctSPUException:
            ReadUint32(mpStream, mCoreDumpCauseUnion.mCoreDumpCauseSPUException.mSPUExceptionType, kEndianBig);
            ReadUint32(mpStream, mCoreDumpCauseUnion.mCoreDumpCauseSPUException.mProcessID,        kEndianBig);
            ReadUint32(mpStream, mCoreDumpCauseUnion.mCoreDumpCauseSPUException.mSPUThreadGroupID, kEndianBig);
            ReadUint32(mpStream, mCoreDumpCauseUnion.mCoreDumpCauseSPUException.mSPUThreadID,      kEndianBig);
            ReadUint32(mpStream, mCoreDumpCauseUnion.mCoreDumpCauseSPUException.mSPU_NPC,          kEndianBig);
            ReadUint64(mpStream, mCoreDumpCauseUnion.mCoreDumpCauseSPUException.mOption,           kEndianBig);
            // We can ignore the mReserved field, as we don't need to read anything after it.
            break;

        case kCdctRSXException:
            ReadUint32(mpStream, mCoreDumpCauseUnion.mCoreDumpCauseRSXException.mChid,              kEndianBig);
            ReadUint32(mpStream, mCoreDumpCauseUnion.mCoreDumpCauseRSXException.mGraphicsErrorNo,   kEndianBig);
            ReadUint32(mpStream, mCoreDumpCauseUnion.mCoreDumpCauseRSXException.mErrorParam0,       kEndianBig);
            ReadUint32(mpStream, mCoreDumpCauseUnion.mCoreDumpCauseRSXException.mErrorParam1,       kEndianBig);
            // We can ignore the mReserved field, as we don't need to read anything after it.
            break;

        case kCdctFootSwitch:
            // There is no additional data beyond mCoreDumpCauseUnion.mCoreDumpCauseData.
            // This core dump was not triggered by a crash but triggered by the user manually.
            // We can ignore the mReserved field, as we don't need to read anything after it.
            break;

        case kCdctUserDefinedEvent:
            ReadUint8(mpStream, mCoreDumpCauseUnion.mCoreDumpCauseUserDefinedEvent.mData1, 8);
            ReadUint8(mpStream, mCoreDumpCauseUnion.mCoreDumpCauseUserDefinedEvent.mData2, 8);
            ReadUint8(mpStream, mCoreDumpCauseUnion.mCoreDumpCauseUserDefinedEvent.mData3, 8);
            // We can ignore the mReserved field, as we don't need to read anything after it.
            break;

        case kCdctSystemEvent:
            // There is no additional data beyond mCoreDumpCauseUnion.mCoreDumpCauseData.
            // This core dump was not triggered by a crash but triggered by the system for some reason (user-caused?).
            // We can ignore the mReserved field, as we don't need to read anything after it.
            break;
    }

    return true;
}


bool PS3DumpFile::LoadSPULocalStorageData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace Local;

    EA_ASSERT(cpsh.mUnitCount < 4096); // Do a sanity check.

    if(cpsh.mUnitCount < 4096)
    {
        PS3DumpELF::SPULocalStorageData lsd;

        memcpy(&lsd, &cpsh, sizeof(cpsh));

        for(uint32_t i = 0; i < cpsh.mUnitCount; i++)
        {
            ReadUint32(mpStream, lsd.mELFImageSize,     kEndianBig);
            ReadUint32(mpStream, lsd.mSPUThreadGroupID, kEndianBig);
            ReadUint32(mpStream, lsd.mSPUThreadID,      kEndianBig);
            ReadBucket(mpStream, 16);
            ReadBucket(mpStream, 84);   // For now we skip the mELFHeader and mPHeader.
            ReadUint8 (mpStream, lsd.mMemory, sizeof(lsd.mMemory));

            mPS3SPUMemoryDump.AddLocalStorageData(lsd);

            // If we have already loaded the SPUThreadData, then here we set the mELFOffset member
            // of the SPUThreadData. The PS3 core dump file format neglects to provide this information
            // so we currently have to manually calculate it upon reading the SPU local storage image
            // and then go back and write it into the SPUThreadData ourselves.
            PS3DumpELF::SPUThreadData* pSPUThreadData = GetSPUThreadData(lsd.mSPUThreadID);
            if(pSPUThreadData)
                pSPUThreadData->mELFOffset = mPS3SPUMemoryDump.GetELFImageOffset(lsd.mSPUThreadID);
        }

        return true;
    }

    return false;
}


bool PS3DumpFile::LoadSPUThreadGroupData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace Local;

    EA_ASSERT(cpsh.mUnitCount < 4096); // Do a sanity check.

    if(cpsh.mUnitCount < 4096)
    {
        mSPUThreadGroupDataArray.resize(cpsh.mUnitCount);

        memcpy(&mSPUThreadGroupDataArray[0], &cpsh, sizeof(cpsh));

        for(uint32_t i = 0; i < cpsh.mUnitCount; i++)
        {
            PS3DumpELF::SPUThreadGroupData& tgd = mSPUThreadGroupDataArray[i];

            ReadUint32(mpStream, tgd.mSPUThreadGroupID, kEndianBig);
            ReadUint32(mpStream, tgd.mState,            kEndianBig);
            ReadUint32(mpStream, tgd.mPriority,         kEndianBig);
            ReadChar8 (mpStream, tgd.mName,             128);
            ReadUint32(mpStream, tgd.mSPUThreadCount,   kEndianBig);
            ReadBucket(mpStream, 16);

            tgd.mSPUThreadIDArray.resize(tgd.mSPUThreadCount);
            ReadUint32(mpStream, &tgd.mSPUThreadIDArray[0], tgd.mSPUThreadCount, kEndianBig);
        }

        return true;
    }

    return false;
}


bool PS3DumpFile::LoadSPUThreadData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace Local;

    EA_ASSERT(cpsh.mUnitCount < 4096); // Do a sanity check.

    if(cpsh.mUnitCount < 4096)
    {
        mSPUThreadDataArray.resize(cpsh.mUnitCount);

        memcpy(&mSPUThreadDataArray[0], &cpsh, sizeof(cpsh));

        for(uint32_t i = 0; i < cpsh.mUnitCount; i++)
        {
            PS3DumpELF::SPUThreadData& std = mSPUThreadDataArray[i];

            ReadUint32(mpStream, std.mSPUThreadGroupID, kEndianBig);
            ReadUint32(mpStream, std.mSPUThreadID,      kEndianBig);
            ReadChar8 (mpStream, std.mName,             128);
            ReadChar8 (mpStream, std.mFilename,         512);
            ReadBucket(mpStream, 24);

            // The following is not part of the core dump record. We added this ourselves 
            // because this ought to be part of the core dump record but isn't. 
            // The following will set the offset to -1 if we haven't loaded the SPU storage
            // contents via the LoadSPULocalStorageData. That's OK, because we'll do it
            // when we get to LoadSPULocalStorageData if we can't do it here.
            std.mELFOffset = mPS3SPUMemoryDump.GetELFImageOffset(std.mSPUThreadID);
        }

        return true;
    }

    return false;
}


bool PS3DumpFile::LoadPRXData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace Local;

    EA_ASSERT(cpsh.mUnitCount < 4096); // Do a sanity check.

    if(cpsh.mUnitCount < 4096)
    {
        mPRXDataArray.resize(cpsh.mUnitCount);

        memcpy(&mPRXDataArray[0], &cpsh, sizeof(cpsh));

        for(uint32_t i = 0; i < cpsh.mUnitCount; i++)
        {
            PS3DumpELF::PRXData& pd = mPRXDataArray[i];

            ReadUint32(mpStream, pd.mPRX_ID,        kEndianBig);
            ReadUint32(mpStream, pd.mSize,          kEndianBig);
            ReadChar8 (mpStream, pd.mName,      30);
            ReadUint16(mpStream, pd.mVersion,       kEndianBig);
            ReadUint32(mpStream, pd.mAttribute,     kEndianBig);
            ReadUint32(mpStream, pd.mStartEntry,    kEndianBig);
            ReadUint32(mpStream, pd.mStopEntry,     kEndianBig);
            ReadChar8 (mpStream, pd.mPath,     512);
            ReadUint32(mpStream, pd.mSegmentCount,  kEndianBig);
            ReadUint8 (mpStream, pd.mID,         5);
            ReadUint8 (mpStream, pd.mRevision);
            ReadBucket(mpStream, 2);
            ReadUint8 (mpStream, pd.mPPUGuid,   20);
            ReadBucket(mpStream, 4);
            ReadBucket(mpStream, 256);

            EA_ASSERT(pd.mSegmentCount < 512); // Do a sanity check.
            pd.mPRXSegmentInfoArray.resize(pd.mSegmentCount);
            for(uint32_t j = 0; j < pd.mSegmentCount; j++)
            {
                PS3DumpELF::PRXSegmentInfo& psi = pd.mPRXSegmentInfoArray[j];

                ReadUint64(mpStream, psi.mBaseAddress, kEndianBig);
                ReadUint64(mpStream, psi.mFileSize,    kEndianBig);
                ReadUint64(mpStream, psi.mMemorySize,  kEndianBig);
                ReadUint64(mpStream, psi.mIndex,       kEndianBig);
                ReadUint64(mpStream, psi.mELFType,     kEndianBig);
                ReadBucket(mpStream, 24);

                // ** This doesn't work, at least up till SDK 280. **
                //
                // The dump file info doesn't provide a means to detect the .text segment, 
                // but we can tell what it is by looking at which segment mStartEntry comes from.
                //
                //if((pd.mStartEntry >=  psi.mBaseAddress) && 
                //   (pd.mStartEntry <  (psi.mBaseAddress + psi.mMemorySize)))
                //{
                //    pd.mpTextSegment = &psi;
                //}

                if(j == 0) // The first segment appears to always be the .text segment.
                    pd.mpTextSegment = &psi;
            }

            if(!pd.mpTextSegment && !pd.mPRXSegmentInfoArray.empty())
                pd.mpTextSegment = &pd.mPRXSegmentInfoArray[0];

            // We look to see if any map files or elf files have been registered for symbol lookups which 
            // correspond to the current PRX. If so then we can use the base address from the PRX (mpTextSegment->mBaseAddress).
            if(mAddressRepLookupSetPPU.GetDatabaseFileCount() && pd.mpTextSegment)
            {
                FixedStringW prxNameW;
                Local::GetFileNameOnly(pd.mName, prxNameW, true);

                FixedStringW prxFileNameW;
                Local::GetFileNameOnly(pd.mPath, prxFileNameW, true);

                for(eastl_size_t i = 0, iEnd = (eastl_size_t)mAddressRepLookupSetPPU.GetDatabaseFileCount(); i < iEnd; i++)
                {
                    IAddressRepLookup* pLookup = mAddressRepLookupSetPPU.GetDatabaseFile(i); // This will always succeed.

                    FixedStringW dbFileNameW;
                    Local::GetFileNameOnly(pLookup->GetDatabasePath(), dbFileNameW, true);

                    if((dbFileNameW == prxNameW) || 
                       (dbFileNameW == prxFileNameW))
                    {
                        pLookup->SetBaseAddress(pd.mpTextSegment->mBaseAddress);
                        break;
                    }
                }
            }
        }

        return true;
    }

    return false;
}


bool PS3DumpFile::LoadMemoryPageAttributeData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace Local;

    EA_ASSERT(cpsh.mUnitCount < 4096); // Do a sanity check.

    if(cpsh.mUnitCount < 4096)
    {
        mMemoryPageAttributeDataArray.resize(cpsh.mUnitCount);

        memcpy(&mMemoryPageAttributeDataArray[0], &cpsh, sizeof(cpsh));

        for(uint32_t i = 0; i < cpsh.mUnitCount; i++)
        {
            PS3DumpELF::MemoryPageAttributeData& mpa = mMemoryPageAttributeDataArray[i];

            ReadUint64(mpStream, mpa.mMapAddress,       kEndianBig);
            ReadUint64(mpStream, mpa.mMapSize,          kEndianBig);
            ReadUint64(mpStream, mpa.mAttribute,        kEndianBig);
            ReadUint64(mpStream, mpa.mAccessRight,      kEndianBig);
            ReadBucket(mpStream, 32);
        }

        return true;
    }

    return false;
}


bool PS3DumpFile::LoadMutexData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace Local;

    EA_ASSERT(cpsh.mUnitCount < 4096); // Do a sanity check.

    if(cpsh.mUnitCount < 4096)
    {
        mMutexDataArray.resize(cpsh.mUnitCount);

        memcpy(&mMutexDataArray[0], &cpsh, sizeof(cpsh));

        for(uint32_t i = 0; i < cpsh.mUnitCount; i++)
        {
            PS3DumpELF::MutexData& md = mMutexDataArray[i];

            ReadUint32(mpStream, md.mID,                kEndianBig);
            ReadUint32(mpStream, md.mProtocol,          kEndianBig);
            ReadUint32(mpStream, md.mRecursive,         kEndianBig);
            ReadUint32(mpStream, md.mShared,            kEndianBig);
            ReadUint32(mpStream, md.mAdaptive,          kEndianBig);
            ReadUint64(mpStream, md.mKey,               kEndianBig);
            ReadUint32(mpStream, md.mFlags,             kEndianBig);
            ReadChar8 (mpStream, md.mName, 8);
            ReadUint64(mpStream, md.mOwnerThreadID,     kEndianBig);
            ReadUint32(mpStream, md.mLockCounter,       kEndianBig);
            ReadUint32(mpStream, md.mCondRefCounter,    kEndianBig);
            ReadUint32(mpStream, md.mCondvarID,         kEndianBig);
            ReadUint32(mpStream, md.mWaitThreadIDCount, kEndianBig);
            ReadBucket(mpStream, 16);

            EA_ASSERT(md.mWaitThreadIDCount < 4096); // Do a sanity check.
            md.mWaitThreadIDArray.resize(md.mWaitThreadIDCount);
            ReadUint64(mpStream, &md.mWaitThreadIDArray[0], md.mWaitThreadIDCount, kEndianBig);
        }

        return true;
    }

    return false;
}


bool PS3DumpFile::LoadConditionVariableData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace Local;

    EA_ASSERT(cpsh.mUnitCount < 4096); // Do a sanity check.

    if(cpsh.mUnitCount < 4096)
    {
        mConditionVariableDataArray.resize(cpsh.mUnitCount);

        memcpy(&mConditionVariableDataArray[0], &cpsh, sizeof(cpsh));

        for(uint32_t i = 0; i < cpsh.mUnitCount; i++)
        {
            PS3DumpELF::ConditionVariableData& cvd = mConditionVariableDataArray[i];

            ReadUint32(mpStream, cvd.mID,                kEndianBig);
            ReadUint32(mpStream, cvd.mShared,            kEndianBig);
            ReadUint64(mpStream, cvd.mKey,               kEndianBig);
            ReadUint32(mpStream, cvd.mFlags,             kEndianBig);
            ReadChar8 (mpStream, cvd.mName, 8);
            ReadUint32(mpStream, cvd.mMutexID,           kEndianBig);
            ReadUint32(mpStream, cvd.mWaitThreadIDCount, kEndianBig);
            ReadBucket(mpStream, 12);

            EA_ASSERT(cvd.mWaitThreadIDCount < 4096); // Do a sanity check.
            cvd.mWaitThreadIDArray.resize(cvd.mWaitThreadIDCount);
            ReadUint64(mpStream, &cvd.mWaitThreadIDArray[0], cvd.mWaitThreadIDCount, kEndianBig);
        }

        return true;
    }

    return false;
}


bool PS3DumpFile::LoadRWLockData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace Local;

    EA_ASSERT(cpsh.mUnitCount < 4096); // Do a sanity check.

    if(cpsh.mUnitCount < 4096)
    {
        mRWLockDataArray.resize(cpsh.mUnitCount);

        memcpy(&mRWLockDataArray[0], &cpsh, sizeof(cpsh));

        for(uint32_t i = 0; i < cpsh.mUnitCount; i++)
        {
            PS3DumpELF::RWLockData& rwld = mRWLockDataArray[i];

            ReadUint32(mpStream, rwld.mID,                kEndianBig);
            ReadUint32(mpStream, rwld.mProtocol,                kEndianBig);
            ReadUint32(mpStream, rwld.mShared,                  kEndianBig);
            ReadUint64(mpStream, rwld.mKey,                     kEndianBig);
            ReadUint32(mpStream, rwld.mFlags,                   kEndianBig);
            ReadChar8 (mpStream, rwld.mName, 8);
            ReadUint64(mpStream, rwld.mOwnerThreadID,           kEndianBig);
            ReadUint32(mpStream, rwld.mReaderWaitThreadIDCount, kEndianBig);
            ReadUint32(mpStream, rwld.mWriterWaitThreadIDCount, kEndianBig);
            ReadBucket(mpStream, 16);

            EA_ASSERT(rwld.mReaderWaitThreadIDCount < 4096); // Do a sanity check.
            rwld.mReaderWaitThreadIDArray.resize(rwld.mReaderWaitThreadIDCount);
            ReadUint64(mpStream, &rwld.mReaderWaitThreadIDArray[0], rwld.mReaderWaitThreadIDCount, kEndianBig);

            EA_ASSERT(rwld.mReaderWaitThreadIDCount < 4096); // Do a sanity check.
            rwld.mWriterWaitThreadIDArray.resize(rwld.mReaderWaitThreadIDCount);
            ReadUint64(mpStream, &rwld.mWriterWaitThreadIDArray[0], rwld.mWriterWaitThreadIDCount, kEndianBig);
        }

        return true;
    }

    return false;
}


bool PS3DumpFile::LoadLWMutexData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace Local;

    EA_ASSERT(cpsh.mUnitCount < 4096); // Do a sanity check.

    if(cpsh.mUnitCount < 4096)
    {
        mLWMutexDataArray.resize(cpsh.mUnitCount);

        memcpy(&mLWMutexDataArray[0], &cpsh, sizeof(cpsh));

        for(uint32_t i = 0; i < cpsh.mUnitCount; i++)
        {
            PS3DumpELF::LWMutexData& md = mLWMutexDataArray[i];

            ReadUint32(mpStream, md.mID,         kEndianBig);
            ReadUint32(mpStream, md.mProtocol,          kEndianBig);
            ReadUint32(mpStream, md.mRecursive,         kEndianBig);
            ReadChar8 (mpStream, md.mName, 8);
            ReadUint64(mpStream, md.mOwnerThreadID,     kEndianBig);
            ReadUint32(mpStream, md.mLockCounter,       kEndianBig);
            ReadUint32(mpStream, md.mWaitThreadIDCount, kEndianBig);
            ReadBucket(mpStream, 12);

            EA_ASSERT(md.mWaitThreadIDCount < 4096); // Do a sanity check.
            md.mWaitThreadIDArray.resize(md.mWaitThreadIDCount);
            ReadUint64(mpStream, &md.mWaitThreadIDArray[0], md.mWaitThreadIDCount, kEndianBig);
        }

        return true;
    }

    return false;
}


bool PS3DumpFile::LoadEventQueueData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace Local;

    EA_ASSERT(cpsh.mUnitCount < 4096); // Do a sanity check.

    if(cpsh.mUnitCount < 4096)
    {
        mEventQueueDataArray.resize(cpsh.mUnitCount);

        memcpy(&mEventQueueDataArray[0], &cpsh, sizeof(cpsh));

        for(uint32_t i = 0; i < cpsh.mUnitCount; i++)
        {
            PS3DumpELF::EventQueueData& eqd = mEventQueueDataArray[i];

            ReadUint32(mpStream, eqd.mID,            kEndianBig);
            ReadUint32(mpStream, eqd.mProtocol,                kEndianBig);
            ReadUint32(mpStream, eqd.mType,                    kEndianBig);
            ReadChar8 (mpStream, eqd.mName, 8);
            ReadUint64(mpStream, eqd.mQueueKey,                kEndianBig);
            ReadUint32(mpStream, eqd.mQueueDepth,              kEndianBig);
            ReadUint32(mpStream, eqd.mWaitThreadIDCount,       kEndianBig);
            ReadUint32(mpStream, eqd.mReadableQueueEntryCount, kEndianBig);
            ReadBucket(mpStream, 8);

            EA_ASSERT(eqd.mWaitThreadIDCount < 4096); // Do a sanity check.
            eqd.mWaitThreadIDArray.resize(eqd.mWaitThreadIDCount);
            ReadUint64(mpStream, &eqd.mWaitThreadIDArray[0], eqd.mWaitThreadIDCount, kEndianBig);

            EA_ASSERT(eqd.mReadableQueueEntryCount < 4096); // Do a sanity check.
            eqd.mEntryArrayArray.resize(eqd.mReadableQueueEntryCount);
            ReadUint8(mpStream, (uint8_t*)&eqd.mEntryArrayArray[0], eqd.mReadableQueueEntryCount * sizeof(PS3DumpELF::ReadableQueueEntry));
        }

        return true;
    }

    return false;
}


bool PS3DumpFile::LoadSemaphoreData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace Local;

    EA_ASSERT(cpsh.mUnitCount < 4096); // Do a sanity check.

    if(cpsh.mUnitCount < 4096)
    {
        mSemaphoreDataArray.resize(cpsh.mUnitCount);

        memcpy(&mSemaphoreDataArray[0], &cpsh, sizeof(cpsh));

        for(uint32_t i = 0; i < cpsh.mUnitCount; i++)
        {
            PS3DumpELF::SemaphoreData& sd = mSemaphoreDataArray[i];

            ReadUint32(mpStream, sd.mID,                kEndianBig);
            ReadUint32(mpStream, sd.mProtocol,          kEndianBig);
            ReadUint32(mpStream, sd.mShared,            kEndianBig);
            ReadUint64(mpStream, sd.mKey,               kEndianBig);
            ReadUint32(mpStream, sd.mFlags,             kEndianBig);
            ReadChar8 (mpStream, sd.mName, 8);
            ReadUint32(mpStream, sd.mMaxValue,          kEndianBig);
            ReadUint32(mpStream, sd.mCurrentValue,      kEndianBig);
            ReadUint32(mpStream, sd.mWaitThreadIDCount, kEndianBig);
            ReadBucket(mpStream, 20);

            EA_ASSERT(sd.mWaitThreadIDCount < 4096); // Do a sanity check.
            sd.mWaitThreadIDArray.resize(sd.mWaitThreadIDCount);
            ReadUint64(mpStream, &sd.mWaitThreadIDArray[0], sd.mWaitThreadIDCount, kEndianBig);
        }

        return true;
    }

    return false;
}


bool PS3DumpFile::LoadLWConditionVariableData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace Local;

    EA_ASSERT(cpsh.mUnitCount < 4096); // Do a sanity check.

    if(cpsh.mUnitCount < 4096)
    {
        mLWConditionVariableDataArray.resize(cpsh.mUnitCount);

        memcpy(&mLWConditionVariableDataArray[0], &cpsh, sizeof(cpsh));

        for(uint32_t i = 0; i < cpsh.mUnitCount; i++)
        {
            PS3DumpELF::LWConditionVariableData& cd = mLWConditionVariableDataArray[i];

            ReadUint32(mpStream, cd.mID,                 kEndianBig);
            ReadChar8 (mpStream, cd.mName, 8);
            ReadUint32(mpStream, cd.mLWMutexID,          kEndianBig);
            ReadUint32(mpStream, cd.mWaitThreadIDCount,  kEndianBig);
            ReadBucket(mpStream, 12);

            EA_ASSERT(cd.mWaitThreadIDCount < 4096); // Do a sanity check.
            cd.mWaitThreadIDArray.resize(cd.mWaitThreadIDCount);
            ReadUint64(mpStream, &cd.mWaitThreadIDArray[0], cd.mWaitThreadIDCount, kEndianBig);
        }

        return true;
    }

    return false;
}


bool PS3DumpFile::LoadMemoryContainerData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace Local;

    EA_ASSERT(cpsh.mUnitCount < 4096); // Do a sanity check.

    if(cpsh.mUnitCount < 4096)
    {
        mMemoryContainerDataArray.resize(cpsh.mUnitCount);

        memcpy(&mMemoryContainerDataArray[0], &cpsh, sizeof(cpsh));

        for(uint32_t i = 0; i < cpsh.mUnitCount; i++)
        {
            PS3DumpELF::MemoryContainerData& cd = mMemoryContainerDataArray[i];

            ReadUint32(mpStream, cd.mID,                kEndianBig);
            ReadUint32(mpStream, cd.mTotalSize,         kEndianBig);
            ReadUint32(mpStream, cd.mAvailableSize,     kEndianBig);
            ReadUint32(mpStream, cd.mParentContainerID, kEndianBig);
            ReadBucket(mpStream, 32);
        }

        return true;
    }

    return false;
}


bool PS3DumpFile::LoadEventFlagData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace Local;

    EA_ASSERT(cpsh.mUnitCount < 4096); // Do a sanity check.

    if(cpsh.mUnitCount < 4096)
    {
        mEventFlagDataArray.resize(cpsh.mUnitCount);

        memcpy(&mEventFlagDataArray[0], &cpsh, sizeof(cpsh));

        for(uint32_t i = 0; i < cpsh.mUnitCount; i++)
        {
            PS3DumpELF::EventFlagData& fd = mEventFlagDataArray[i];

            ReadUint32(mpStream, fd.mID,                kEndianBig);
            ReadUint32(mpStream, fd.mProtocol,          kEndianBig);
            ReadUint32(mpStream, fd.mShared,            kEndianBig);
            ReadUint64(mpStream, fd.mKey,               kEndianBig);
            ReadUint32(mpStream, fd.mFlags,             kEndianBig);
            ReadUint32(mpStream, fd.mType,              kEndianBig);
            ReadChar8 (mpStream, fd.mName, 8);
            ReadUint64(mpStream, fd.mCurrentBitPattern, kEndianBig);
            ReadUint32(mpStream, fd.mWaitThreadCount  , kEndianBig);
            ReadBucket(mpStream, 16);

            EA_ASSERT(fd.mWaitThreadCount < 4096); // Do a sanity check.
            fd.mWTIArray.resize(fd.mWaitThreadCount);
            for(uint32_t j = 0; j < fd.mWaitThreadCount; j++)
            {
                ReadUint64(mpStream, fd.mWTIArray[j].mThreadID,   kEndianBig);
                ReadUint64(mpStream, fd.mWTIArray[j].mBitPattern, kEndianBig);
                ReadUint32(mpStream, fd.mWTIArray[j].mMode,       kEndianBig);
            }
        }

        return true;
    }

    return false;
}


bool PS3DumpFile::LoadRSXDebugData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace Local;
    using namespace PS3DumpELF;

    PS3DumpELF::RSXDebugInfo1Data& mRSX = mRSXDebugInfo1Data; // Shortened alias.
    size_t i;

    // Need to verify the mRSXDebugInfoVersion value.
    memcpy(&mRSX, &cpsh, sizeof(cpsh));

    ReadUint32(mpStream, mRSX.mRSXDebugInfoVersion, kEndianBig);
    ReadUint32(mpStream, mRSX.mSize,                kEndianBig);

    ReadUint32(mpStream, mRSX.mInterruptErrorStatus,       kEndianBig);
    ReadUint32(mpStream, mRSX.mInterruptFIFOErrorStatus,   kEndianBig);
    ReadUint32(mpStream, mRSX.mInterruptIOIFErrorStatus,   kEndianBig);
    ReadBucket(mpStream, 4);
    ReadUint64(mpStream, mRSX.mInterruptGraphicsErrorStatus,   kEndianBig);

    for(i = 0; i < EAArrayCount(mRSX.mTileRegionUnitArray); ++i)
    {
        ReadUint32(mpStream, mRSX.mTileRegionUnitArray[i].mEnable,    kEndianBig);
        ReadUint32(mpStream, mRSX.mTileRegionUnitArray[i].mBank,      kEndianBig);
        ReadUint32(mpStream, mRSX.mTileRegionUnitArray[i].mStartAddr, kEndianBig);
        ReadUint32(mpStream, mRSX.mTileRegionUnitArray[i].mLimitAddr, kEndianBig);
        ReadUint32(mpStream, mRSX.mTileRegionUnitArray[i].mPitchSize, kEndianBig);
        ReadUint32(mpStream, mRSX.mTileRegionUnitArray[i].mBaseTag,   kEndianBig);
        ReadUint32(mpStream, mRSX.mTileRegionUnitArray[i].mLimitTag,  kEndianBig);
        ReadUint32(mpStream, mRSX.mTileRegionUnitArray[i].mCompMode,  kEndianBig);
    };

    for(i = 0; i < EAArrayCount(mRSX.mZcullRegionArray); ++i)
    {
        ReadUint32(mpStream, mRSX.mZcullRegionArray[i].mEnable,    kEndianBig);
        ReadUint32(mpStream, mRSX.mZcullRegionArray[i].mFormat,    kEndianBig);
        ReadUint32(mpStream, mRSX.mZcullRegionArray[i].mAntialias, kEndianBig);
        ReadUint32(mpStream, mRSX.mZcullRegionArray[i].mWidth,     kEndianBig);
        ReadUint32(mpStream, mRSX.mZcullRegionArray[i].mHeight,    kEndianBig);
        ReadUint32(mpStream, mRSX.mZcullRegionArray[i].mStart,     kEndianBig);
        ReadUint32(mpStream, mRSX.mZcullRegionArray[i].mOffset,    kEndianBig);
    }

    for(i = 0; i < EAArrayCount(mRSX.mZcullStatusArray); ++i)
    {
        ReadUint32(mpStream, mRSX.mZcullStatusArray[i].mValid,            kEndianBig);
        ReadUint32(mpStream, mRSX.mZcullStatusArray[i].mZdir,             kEndianBig);
        ReadUint32(mpStream, mRSX.mZcullStatusArray[i].mZformat,          kEndianBig);
        ReadUint32(mpStream, mRSX.mZcullStatusArray[i].mSfunc,            kEndianBig);
        ReadUint32(mpStream, mRSX.mZcullStatusArray[i].mSref,             kEndianBig);
        ReadUint32(mpStream, mRSX.mZcullStatusArray[i].mSmask,            kEndianBig);
        ReadUint16(mpStream, mRSX.mZcullStatusArray[i].mPushbacklimit,    kEndianBig);
        ReadUint16(mpStream, mRSX.mZcullStatusArray[i].mMoveforwardlimit, kEndianBig);
    }

    // mZcullMatchUnit
    {
        ReadUint32(mpStream, mRSX.mZcullMatchUnit.mValid,  kEndianBig);
        ReadUint32(mpStream, mRSX.mZcullMatchUnit.mRegion, kEndianBig);
    }

    // mFIFOErrorStateDataArray
    {
        ReadUint8 (mpStream, mRSX.mFIFOErrorStateData.mDMAMethodType);
        ReadBucket(mpStream, 1);
        ReadUint16(mpStream, mRSX.mFIFOErrorStateData.mDMAMethodCount,  kEndianBig);
        ReadUint16(mpStream, mRSX.mFIFOErrorStateData.mDMAMethodOffset, kEndianBig);
        ReadBucket(mpStream, 10);

        ReadUint8 (mpStream, mRSX.mFIFOErrorStateData.mDMASubInSubroutine);
        ReadBucket(mpStream, 3);
        ReadUint32(mpStream, mRSX.mFIFOErrorStateData.mDMASubJumpCallOffset, kEndianBig);
        ReadUint32(mpStream, mRSX.mFIFOErrorStateData.mDMASubReturnOffset,   kEndianBig);
        ReadBucket(mpStream, 4);

        ReadUint8 (mpStream, mRSX.mFIFOErrorStateData.mDMAControlChannelID);
        ReadBucket(mpStream, 3);
        ReadUint32(mpStream, mRSX.mFIFOErrorStateData.mDMAControlPutRegister, kEndianBig);
        ReadUint32(mpStream, mRSX.mFIFOErrorStateData.mDMAControlGetRegister, kEndianBig);
        ReadUint32(mpStream, mRSX.mFIFOErrorStateData.mDMAControlRefRegister, kEndianBig);

        ReadUint32(mpStream, mRSX.mFIFOErrorStateData.mDMACmdMethodHeader,    kEndianBig);
        ReadUint32(mpStream, mRSX.mFIFOErrorStateData.mDMACmdMethodData,      kEndianBig);
        ReadBucket(mpStream, 8);

        ReadUint8 (mpStream, mRSX.mFIFOErrorStateData.mLabelContextDmaInvalidFlag);
        ReadUint8 (mpStream, mRSX.mFIFOErrorStateData.mLabelSemaphoreLocation);
        ReadBucket(mpStream, 2);
        ReadUint32(mpStream, mRSX.mFIFOErrorStateData.mLabelSemaphoreOffset,   kEndianBig);
        ReadUint32(mpStream, mRSX.mFIFOErrorStateData.mLabelAcquireLabelValue, kEndianBig);
        ReadBucket(mpStream, 4);

        ReadUint32(mpStream, mRSX.mFIFOErrorStateData.mCacheControlCache1Put, kEndianBig);
        ReadUint32(mpStream, mRSX.mFIFOErrorStateData.mCacheControlCache1Get, kEndianBig);
        ReadBucket(mpStream, 8);
    }

    // mIOIFErrorStateData
    {
        ReadUint8 (mpStream, mRSX.mIOIFErrorStateData.mIOIFCommand);
        ReadUint8 (mpStream, mRSX.mIOIFErrorStateData.mFBRequestor);
        ReadBucket(mpStream, 2);
        ReadUint32(mpStream, mRSX.mIOIFErrorStateData.mIOIFAddress, kEndianBig);
        ReadBucket(mpStream, 8);
    }

    // mGraphicsErrorStateData
    {
        // LAUNCH_CHECK
        ReadUint32(mpStream, mRSX.mGraphicsErrorStateData.mLCLaunchCheck1,   kEndianBig);
        ReadUint32(mpStream, mRSX.mGraphicsErrorStateData.mLCLaunchCheck2,   kEndianBig);
        ReadBucket(mpStream, 8);

        // TRAPPED_ADDRESS_DATA
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mTADTrappedAddressDhv);
        ReadBucket(mpStream, 1);
        ReadUint16(mpStream, mRSX.mGraphicsErrorStateData.mTADTrappedAddressSM,   kEndianBig);
        ReadUint32(mpStream, mRSX.mGraphicsErrorStateData.mTADTrappedDataLow,   kEndianBig);
        ReadUint32(mpStream, mRSX.mGraphicsErrorStateData.mTADTrappedDataHigh,   kEndianBig);
        ReadBucket(mpStream, 4);

        // COLOR_SURFACE_VIOLATION
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mCSVDDDDD);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mCSVSSSSS);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mCSVSS);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mCSVCtilemode);
        ReadUint16(mpStream, mRSX.mGraphicsErrorStateData.mCSVRopmode,   kEndianBig);
        ReadBucket(mpStream, 2);
        ReadUint32(mpStream, mRSX.mGraphicsErrorStateData.mCSVAddress,   kEndianBig);
        ReadBucket(mpStream, 4);
         
        // COLOR_SURFACE_LIMIT
        ReadUint32(mpStream, mRSX.mGraphicsErrorStateData.mCSLColorSurfaceLimit0,   kEndianBig);
        ReadUint32(mpStream, mRSX.mGraphicsErrorStateData.mCSLColorSurfaceLimit1,   kEndianBig);
        ReadUint32(mpStream, mRSX.mGraphicsErrorStateData.mCSLColorSurfaceLimit2,   kEndianBig);
        ReadUint32(mpStream, mRSX.mGraphicsErrorStateData.mCSLColorSurfaceLimit3,   kEndianBig);

        // DEPTH_SURFACE_VIOLATION
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mDSVDDDDD);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mDSVSSSSS);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mDSVSS);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mDSVZtilemode);
        ReadUint16(mpStream, mRSX.mGraphicsErrorStateData.mDSVRopmode,   kEndianBig);
        ReadBucket(mpStream, 2);
        ReadUint32(mpStream, mRSX.mGraphicsErrorStateData.mDSVAddress,   kEndianBig);
        ReadBucket(mpStream, 4);

        // DEPTH_SURFACE_LIMIT
        ReadUint32(mpStream, mRSX.mGraphicsErrorStateData.mDSLDepthSurfaceLimit,   kEndianBig);
        ReadBucket(mpStream, 12);
         
        // TRANSFER_DATA_READ
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mTDRDstFormat);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mTDRSrcFormat);
        ReadUint16(mpStream, mRSX.mGraphicsErrorStateData.mTDRMiscCount,   kEndianBig);
        ReadUint32(mpStream, mRSX.mGraphicsErrorStateData.mTDRReadAddress,   kEndianBig);
        ReadBucket(mpStream, 8);
         
        // TRANSFER_DATA_WRITE
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mTDWDstFormat);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mTDWSrcFormat);
        ReadUint16(mpStream, mRSX.mGraphicsErrorStateData.mTDWMiscCount,   kEndianBig);
        ReadUint32(mpStream, mRSX.mGraphicsErrorStateData.mTDWWriteAddress,   kEndianBig);
        ReadBucket(mpStream, 8);

        // GRAPHICS_UNIT
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mGUGraphicsEngineStatus);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mGU3dFrontendUnit);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mGUIdxUnit);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mGUGeometryUnit);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mGUSetupUnit);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mGUSpillUnit);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mGURasterUnit);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mGUZcullUpdateUnit);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mGUPreshaderUnit);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mGUShaderBaseUnit);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mGUShaderPipeUnit);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mGUTextureUnit);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mGURopUnit);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mGU2dFrontEnd);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mGU2dRasterizer);
        ReadBucket(mpStream, 1);

        // FRONTEND_INDEX_UNIT
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mFIUFEStatus);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mFIUIDXStatus);
        ReadBucket(mpStream, 14);

        // GEOMETRY_UNIT
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mGYUGeometryVabStatus);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mGYUGeometryVpcStatus);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mGYUGeometryVtfStatus);
        ReadBucket(mpStream, 13);

        // SETUP_UNIT
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mSUSetupStatus);
        ReadBucket(mpStream, 15);

        // RASTER_UNIT
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mRUCoarseRaster);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mRUFineRaster);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mRUWindowID);
        ReadBucket(mpStream, 13);

        // ZCULL_UNIT
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mZUZcullProc);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mZUUpdateUnitsStatus);
        ReadBucket(mpStream, 14);

        // PRE_SHADER_UNIT
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mPSUColorAssembly);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mPSUShaderTriangleFifo);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mPSUShaderTriangleProcessor);
        ReadBucket(mpStream, 13);

        // BASE_SHADER_UNIT
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mBSUShaderQuadDistributor);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mBSUShaderQuadCollector);
        ReadBucket(mpStream, 14);

        // RESERVED_0
        ReadBucket(mpStream, 16);

        // RESERVED_1
        ReadBucket(mpStream, 16);

        // RESERVED_2
        ReadBucket(mpStream, 16);

        // RESERVED_3
        ReadBucket(mpStream, 16);

        // TEXTURE_CACHE_UNIT
        ReadBucket(mpStream, 8);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mTCUMcache);
        ReadBucket(mpStream, 7);

        // ROP_UNIT
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mRPURopUnitStatus);
        ReadBucket(mpStream, 15);

        // 2D_UNIT
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mU2DUnitStatus);
        ReadUint8 (mpStream, mRSX.mGraphicsErrorStateData.mURasterizer2D);
        ReadBucket(mpStream, 14);
    }

    for(i = 0; i < EAArrayCount(mRSX.mGraphicsFIFOUnitArray); ++i)
    {
        ReadUint16(mpStream, mRSX.mGraphicsFIFOUnitArray[i].mType,   kEndianBig);
        ReadUint16(mpStream, mRSX.mGraphicsFIFOUnitArray[i].mAddr,   kEndianBig);
        ReadUint32(mpStream, mRSX.mGraphicsFIFOUnitArray[i].mData1,  kEndianBig);
        ReadUint32(mpStream, mRSX.mGraphicsFIFOUnitArray[i].mData2,  kEndianBig);
    }

    for(i = 0; i < EAArrayCount(mRSX.mFIFOCacheUnitArray); ++i)
    {
        ReadUint16(mpStream, mRSX.mFIFOCacheUnitArray[i].mType,   kEndianBig);
        ReadUint16(mpStream, mRSX.mFIFOCacheUnitArray[i].mAddr,   kEndianBig);
        ReadUint32(mpStream, mRSX.mFIFOCacheUnitArray[i].mData,   kEndianBig);
    }

    // mEAtoIOTableArray / mIOtoEATableArray
    {
        ReadUint16(mpStream, mRSX.mEAtoIOTableArray, EAArrayCount(mRSX.mEAtoIOTableArray), kEndianBig);
        ReadUint16(mpStream, mRSX.mIOtoEATableArray, EAArrayCount(mRSX.mIOtoEATableArray), kEndianBig);
    }

    for(i = 0; i < 110; ++i)
    {
        ReadUint16(mpStream, mRSX.mBundleState[i].mID,   kEndianBig);
        ReadUint16(mpStream, mRSX.mBundleState[i].mNum,  kEndianBig);

        EA_ASSERT(mRSX.mBundleState[i].mID <= kBundleTypeCount);
        EA_ASSERT(mRSX.mBundleState[i].mNum <= EAArrayCount(mRSX.mBundleState[i].mData));

        ReadUint32(mpStream, mRSX.mBundleState[i].mData, mRSX.mBundleState[i].mNum, kEndianBig);
    }

    return true;
}


bool PS3DumpFile::LoadMemoryStatisticsData(PS3DumpELF::CommonProgramSegmentHeader& cpsh)
{
    using namespace EA::IO;
    using namespace Local;

    // Need to verify the mRSXDebugInfoVersion value.
    memcpy(&mMemoryStatisticsData, &cpsh, sizeof(cpsh));

    ReadUint32(mpStream, mMemoryStatisticsData.mCreatedSharedMemorySize,    kEndianBig);
    ReadUint32(mpStream, mMemoryStatisticsData.mAttachedSharedMemorySize,   kEndianBig);
    ReadUint32(mpStream, mMemoryStatisticsData.mProcessLocalMemorySize,     kEndianBig);
    ReadUint32(mpStream, mMemoryStatisticsData.mProcessLocalTextSize,       kEndianBig);
    ReadUint32(mpStream, mMemoryStatisticsData.mPRXTextSize,                kEndianBig);
    ReadUint32(mpStream, mMemoryStatisticsData.mPRXDataSize,                kEndianBig);
    ReadUint32(mpStream, mMemoryStatisticsData.mRemainMemorySize,           kEndianBig);
    ReadBucket(mpStream, 36);

    return true;
}


const char* PS3DumpFile::GetCoreDumpCauseString(uint32_t type)
{
    using namespace PS3DumpELF;

    switch ((CoreDumpCauseType)type)
    {
        case kCdctPPUException:
            return "PPU Exception";
        case kCdctSPUException:
            return "SPU Exception";
        case kCdctRSXException:
            return "RSX Exception";
        case kCdctFootSwitch:
            return "Foot Switch"; // User-triggered
        case kCdctUserDefinedEvent:
            return "User Defined Event";
        case kCdctSystemEvent:
            break;
    }

    return "System Event";
}


const char* PS3DumpFile::GetPPUExceptionTypeString(uint64_t type)
{
    using namespace PS3DumpELF;

    switch (type)
    {
        case kETTrap:
            return "Trap";
        case kETPrivilegedInstruction:
            return "Privileged Instruction";
        case kETIllegalInstruction:
            return "Illegal Instruction";
        case kETInstructionStorage:
            return "Instruction Storage";
        case kETInstructionSegment:
            return "Instruction Segment";
        case kETDataStorage:
            return "Data Storage";
        case kETDataSegment:
            return "Data Segment";
        case kETFloatingPoint:
            return "Floating Point";
        case kETDABRMatch:
            return "DABR Match";
        case kETAlignment:
            return "Alignment";
        case kETMemoryAccessTrap:
            return "Memory Access Trap";
    }

    return "Unknown";
}


bool PS3DumpFile::IsPPUExceptionDataAccess(uint64_t type)
{
    using namespace PS3DumpELF;

    return ((type == kETDataStorage) || 
            (type == kETDataSegment) || 
            (type == kETAlignment));
}


uint32_t PS3DumpFile::GetSPUExceptionTypeString(uint32_t typeFlags, char* pString)
{
    using namespace PS3DumpELF;
    using namespace EA::StdC;

    uint32_t n = 0;

    // The SPU can have multiple exception types at the same time.
    const uint32_t valueArray[] = 
    {
        kSETMFC_DMAAlignment,
        kSETInvalidDMACommand,
        kSETSPUError,
        kSETMFC_FIR,
        kSETMFCDataSegment,
        kSETMFCDataStorage,
        kSETStop,
        kSETHalt,
        kSETStopUnknown,
        kSETMemoryAccessTrap
    };

    const char* stringArray[] = 
    {
        "MFC DMA Alignment",
        "Invalid DMA Command",
        "SPU Error",
        "MFC FIR",
        "MFC Data Segment",
        "MFC Data Storage",
        "Stop",
        "Halt",
        "Stop Unknown",
        "Memory Access Trap"
    };

    pString[0] = 0;

    for(size_t i = 0; i < EAArrayCount(valueArray); i++)
    {
        if(typeFlags & valueArray[i])
        {
            if(n++)
                Strcat(pString, ", ");
            Strcat(pString, stringArray[i]);
        }
    }

    return n;
}


const char* PS3DumpFile::GetPPUThreadStateString(uint32_t state)
{
    using namespace PS3DumpELF;

    switch (state)
    {
        case kTSIdle:
            return "Idle";
        case kTSRunnable:
            return "Runnable";
        case kTSOnProc:
            return "OnProc";
        case kTSSleep:
            return "Sleeping";
        case kTSStop:
            return "Stopped";
        case kTSZombie:
            return "Zombie";
        case kTSDead:
            return "Dead";
    }

    return "Invalid";
}


const char* PS3DumpFile::GetSPUThreadStateString(uint32_t state)
{
    using namespace PS3DumpELF;

    switch (state)
    {
        case kSTSNotInitialized:
            return "Not Initialized";
        case kSTSInitialized:
            return "Initialized";
        case kSTSReady:
            return "Ready";
        case kSTSWaiting:
            return "Waiting";
        case kSTSSuspended:
            return "Suspended";
        case kSTSWaitingAndSuspended:
            return "Waiting and Suspended";
        case kSTSRunning:
            return "Running";
        case kSTSStopped:
            return "Stopped";
    }

    return "Invalid";
}


const char* PS3DumpFile::GetSyncTypeString(PS3DumpELF::SyncType type)
{
    using namespace PS3DumpELF;

    switch (type)
    {
        case kSTMutex:
            return "Mutex";
        case kSTConditionVariable:
            return "Condition Variable";
        case kSTRWLock:
            return "RW Lock";
        case kSTLWMutex:
            return "LW Mutex";
        case kSTEventQueue:
            return "Event Queue";
        case kSTSemaphore:
            return "Semaphore";
        case kSTLWConditionVariable:
            return "LW Condition Variable";
        case kSTEvent:
            return "Event";
    }

    return "Unknown";
}


void PS3DumpFile::GetMemoryPageAttributeString(uint64_t attribute, uint64_t /*accessRight*/, FixedString8& s8)
{
    using namespace PS3DumpELF;

    const uint32_t valueArray[] = 
    {
        kARAccessiblePPU,
        kARAccessibleHandler,
        kARAccessibleSPUThread,
        kARAccessibleRawSPU
    };

    const char* stringArray[] = 
    {
        "PPU",
        "Handler",
        "SPU Thread",
        "Raw SPU"
    };

    if(attribute & kMPAWriteEnabled)
        s8 = "writable";
    else
        s8 = "read-only";

    s8 += ", accessible from ";

    for(size_t n = 0, i = 0; i < EAArrayCount(valueArray); i++)
    {
        if(attribute & valueArray[i])
        {
            if(n++)
                s8 += ", ";
            s8 += stringArray[i];
        }
    }
}


void PS3DumpFile::GetInterruptErrorStatusString(uint32_t status, FixedString8& s8)
{
    using namespace PS3DumpELF;

    const uint32_t valueArray[] = 
    {
        kIESFIFOError,
        kIESIOIFError,
        kIESGraphicsError
    };

    const char* stringArray[] = 
    {
        "FIFO error",
        "IOIF error",
        "Graphics error",
    };

    for(size_t n = 0, i = 0; i < EAArrayCount(valueArray); i++)
    {
        if(status & valueArray[i])
        {
            if(n++)
                s8 += ", ";
            s8 += stringArray[i];
        }
    }
}


void PS3DumpFile::GetInterruptFIFOErrorStatusString(uint32_t status, FixedString8& s8)
{
    using namespace PS3DumpELF;

    const uint32_t valueArray[] = 
    {
        kIESFMethod,
        kIESFCall,
        kIESFReturn,
        kIESFCommand,
        kIESFProtection,
        kIESFLabelBusy,
        kIESFLabelError,
        kIESFLabelTimeout
    };

    const char* stringArray[] = 
    {
        "method",
        "call",
        "return",
        "command",
        "protection",
        "label busy",
        "label error",
        "label timeout"
    };

    for(size_t n = 0, i = 0; i < EAArrayCount(valueArray); i++)
    {
        if(status & valueArray[i])
        {
            if(n++)
                s8 += ", ";
            s8 += stringArray[i];
        }
    }
}


void PS3DumpFile::GetInterruptIOIFErrorStatusString(uint32_t status, FixedString8& s8)
{
    using namespace PS3DumpELF;

    uint32_t i = 0;

    if(status & kIESIRSXMaster)
    {
        if(i++)
            s8 += ", ";
        s8 += "RSX master";
    }
}


void PS3DumpFile::GetInterruptGraphicsErrorStatusString(uint64_t status, FixedString8& s8)
{
    using namespace PS3DumpELF;

    const uint32_t valueArray[] = 
    {
        kIGENotify,
        kIGEMethodData,
        kIGEMethodAddr,
        kIGERange2D,
        kIGEColorSurface,
        kIGEDepthSurface,
        kIGETransferDataRead,
        kIGETransferDataWrite,
        kIGESurface2D,
        kIGEState,
        kIGEMethod2D,
        kIGEVertexAccess,
        kIGEIDX,
        kIGEBeginEnd,
        kIGETextureLocalMemory,
        kIGETextureMainMemory,
        kIGEZcull,
        kIGESIP
    };

    const char* stringArray[] = 
    {
        "notify",
        "method data",
        "method addr",
        "range 2D",
        "color surface",
        "depth surface",
        "transfer data read",
        "transfer data write",
        "surface 2D",
        "state",
        "method 2D",
        "vertex access",
        "idx",
        "begin end",
        "texture local memory",
        "texture main memory",
        "z cull",
        "SIP"
    };

    for(size_t n = 0, i = 0; i < EAArrayCount(valueArray); i++)
    {
        if(status & valueArray[i])
        {
            if(n++)
                s8 += ", ";
            s8 += stringArray[i];
        }
    }
}


// See CoreDumpCauseRSXException.mGraphicsErrorNo
const char* PS3DumpFile::GetRSXGraphicsErrorString(uint32_t error)
{
    switch (error)
    {
        // The text here is just a copy of the libgcm overview text.
        case 1:
            return "A command fetched to the FIFO could not be interpreted. It is generated when an undefined command "
                   "in the command buffer was fetched to the RSX and an attempt was made to interpret it. In many cases, "
                   "this is because the command buffer has been corrupted or that it has been overwritten by non-command "
                   "data. Other possible causes include executing a jump/call command without a proper command at the "
                   "jump destination, or that the PUT pointer jumped to an unexpected location that has not been mapped "
                   "to the memory space accessible by RSX.";

        case 3:
            return "Overwrite rendering by the system software failed. This often occurs when an invalid buffer ID is "
                   "passed for execution to a flip command or a flip preprocessing command.";

        case 256:
            return "A tiled area was accessed as a color buffer. Specifically, it means that a Tiled area and linear area were "
                   "accessed during drawing or, in other words, the color buffer area that was specified in cellGcmSetSurface() "
                   "may not have been set properly for the Tiled area. Other cases for which this error can occur are when "
                   "the pitch sizes of both area are not set properly or if the pitch size specified in cellGcmSetSurface() "
                   "is larger than the pitch size specified by cellGcmSetTileInfo().";

        case 257:
            return "Among the errors that can occur when accessing a Tiled area, this error is generated when the area is "
                   "accessed as a depth buffer. The depth buffer area may have straddled a Tiled area and linear area or, "
                   "in other words, the depth buffer area specified in cellGcmSetSurface() may not have been set properly "
                   "for the Tiled area. Also, the pitch sizes of both area may not have been set properly or the pitch size "
                   "specified in cellGcmSetSurface() may be larger than the pitch size specified by "
                   "cellGcmSetTileInfo().";

        case 258:
            return "This error is generated when the RSX performs a vertex or index fetch from an invalid offset. "
                   "This often occurs when the value of the offset argument of cellGcmSetVertexDataArray() is invalid or  "
                   "when the index that was being used by the previous vertex shader is not currently being used, but it has "
                   "not been invalidated and a vertex fetch occurs unexpectedly. The error also occurs when the value of the "
                   "indices argument for specifying the offset to the index for cellGcmSetDrawIndexArray() is invalid.";

        case 259:
            return "The RSX performed a texture fetch from an invalid offset in local memory. Specifically, this occurs "
                   "when the value of the offset member of the CellGcmTexture structure is invalid. Be sure to confirm "
                   "that a correct offset is passed to the structure that is assigned in cellGcmSetTexture() and that "
                   "this value is the result of converting to an offset using cellGcmAddressToOffset().";

        case 260:
            return "The RSX performed a texture fetch from an invalid offset in main memory. Specifically, this occurs "
                   "when the value of the offset member of the CellGcmTexture structure is invalid. Be sure to confirm "
                   "that a correct offset is passed to the structure that is assigned in cellGcmSetTexture() and that "
                   "this value is the result of converting to an offset using cellGcmAddressToOffset(). ";

        case 261:
            return "An attempt was made to use a function that is not supported by the hardware. The RSX has various "
                   "restrictions that appear in the documentation, and this error corresponds to those restrictions.";

        case 262:
            return "This error is generated when invalid data is specified for a command. Basically, the RSX interprets "
                   "a command and data together as a set, and this error indicates that the command is supported but the "
                   "data for that command is a value outside of the allowable range. Be sure to verify that the argument "
                   "values that are set for each command generation function fall within the ranges that are specified in "
                   "the libgcm Reference.";

        case 265:
            return "An unsupported command was executed. Like graphics error 1, this error is generated when an unsupported "
                   "command is passed to the RSX. While graphics error 1 is generated when an Invalid command is encountered "
                   "in the command fetch engine, this error occurs when an Invalid command is encountered in the rendering "
                   "pipeline. Since an undefined command is fetched and only the unit in which the error occurs differs, "
                   "the command buffer may also have been corrupted or overwritten in this case.";

        case 512:
            return "An IOIF error occurred. This error occurs when the RSX attempts to access main memory, but the access "
                   "cannot be performed normally since the address (more precisely, the offset) has not been mapped. Be sure "
                   "to confirm that a main memory area was specified in cellGcmInit(), that the area that was allocated by "
                   "cellGcmMapMainMemory() has been passed to libgcm, and that the argument is an offset value that was "
                   "converted from an address by cellGcmAddressToOffset().";

        default:
            return "An unknown error type occurred.";
    }
}


const char* PS3DumpFile::GetTileRegionUnitCompModeString(uint32_t nCompressionMode)
{
    using namespace Local;

    switch(nCompressionMode)
    {
        // We have to use literal numbers here because we are building on a PC and don't have a header for them.
        case CELL_GCM_COMPMODE_DISABLED:
            return "CELL_GCM_COMPMODE_DISABLED";
        case CELL_GCM_COMPMODE_C32_2X1:
            return "CELL_GCM_COMPMODE_C32_2X1";
        case CELL_GCM_COMPMODE_C32_2X2:
            return "CELL_GCM_COMPMODE_C32_2X2";
        case CELL_GCM_COMPMODE_Z32_SEPSTENCIL:
            return "CELL_GCM_COMPMODE_Z32_SEPSTENCIL";
        case CELL_GCM_COMPMODE_Z32_SEPSTENCIL_REGULAR:
            return "CELL_GCM_COMPMODE_Z32_SEPSTENCIL_REGULAR";
        case CELL_GCM_COMPMODE_Z32_SEPSTENCIL_DIAGONAL:
            return "CELL_GCM_COMPMODE_Z32_SEPSTENCIL_DIAGONAL";
        case CELL_GCM_COMPMODE_Z32_SEPSTENCIL_ROTATED:
            return "CELL_GCM_COMPMODE_Z32_SEPSTENCIL_ROTATED";
        default:
            return "<unknown>";
    }
}


const char* PS3DumpFile::GetZCullRegionAntialiasString(uint32_t nAntialias)
{
    using namespace Local;

    switch (nAntialias)
    {
        case (CELL_GCM_SURFACE_CENTER_1):
            return "CELL_GCM_SURFACE_CENTER_1";
        case (CELL_GCM_SURFACE_DIAGONAL_CENTERED_2):
            return "CELL_GCM_SURFACE_DIAGONAL_CENTERED_2";
        case (CELL_GCM_SURFACE_SQUARE_CENTERED_4):
            return "CELL_GCM_SURFACE_SQUARE_CENTERED_4";
        case (CELL_GCM_SURFACE_SQUARE_ROTATED_4):
            return "CELL_GCM_SURFACE_SQUARE_ROTATED_4";
        default:
            return "<unknown>";
    }
}



} // namespace Callstack
} // namespace EA

