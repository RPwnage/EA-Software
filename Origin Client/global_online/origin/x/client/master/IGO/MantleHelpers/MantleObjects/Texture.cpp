#include "Texture.h"
#include "..\MantleAppWsi.h"
#include "GpuMemMgr.h"
#include "CommandBuffer.h"

#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#define DDS_HEADER_FLAGS_TEXTURE        0x00001007  // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT 
#define DDS_HEADER_FLAGS_MIPMAP         0x00020000  // DDSD_MIPMAPCOUNT
#define DDS_HEADER_FLAGS_VOLUME         0x00800000  // DDSD_DEPTH
#define DDS_HEADER_FLAGS_PITCH          0x00000008  // DDSD_PITCH
#define DDS_HEADER_FLAGS_LINEARSIZE     0x00080000  // DDSD_LINEARSIZE

#define DDS_SURFACE_FLAGS_TEXTURE 0x00001000 // DDSCAPS_TEXTURE
#define DDS_SURFACE_FLAGS_MIPMAP  0x00400008 // DDSCAPS_COMPLEX | DDSCAPS_MIPMAP
#define DDS_SURFACE_FLAGS_CUBEMAP 0x00000008 // DDSCAPS_COMPLEX

#define DDS_CUBEMAP_POSITIVEX 0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX 0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY 0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY 0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ 0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ 0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP_ALLFACES ( DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |\
                               DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |\
                               DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ )

typedef enum D3D10_RESOURCE_DIMENSION {
  D3D10_RESOURCE_DIMENSION_UNKNOWN     = 0,
  D3D10_RESOURCE_DIMENSION_BUFFER      = 1,
  D3D10_RESOURCE_DIMENSION_TEXTURE1D   = 2,
  D3D10_RESOURCE_DIMENSION_TEXTURE2D   = 3,
  D3D10_RESOURCE_DIMENSION_TEXTURE3D   = 4 
} D3D10_RESOURCE_DIMENSION;

typedef enum DXGI_FORMAT {
  DXGI_FORMAT_UNKNOWN                      = 0,
  DXGI_FORMAT_R32G32B32A32_TYPELESS        = 1,
  DXGI_FORMAT_R32G32B32A32_FLOAT           = 2,
  DXGI_FORMAT_R32G32B32A32_UINT            = 3,
  DXGI_FORMAT_R32G32B32A32_SINT            = 4,
  DXGI_FORMAT_R32G32B32_TYPELESS           = 5,
  DXGI_FORMAT_R32G32B32_FLOAT              = 6,
  DXGI_FORMAT_R32G32B32_UINT               = 7,
  DXGI_FORMAT_R32G32B32_SINT               = 8,
  DXGI_FORMAT_R16G16B16A16_TYPELESS        = 9,
  DXGI_FORMAT_R16G16B16A16_FLOAT           = 10,
  DXGI_FORMAT_R16G16B16A16_UNORM           = 11,
  DXGI_FORMAT_R16G16B16A16_UINT            = 12,
  DXGI_FORMAT_R16G16B16A16_SNORM           = 13,
  DXGI_FORMAT_R16G16B16A16_SINT            = 14,
  DXGI_FORMAT_R32G32_TYPELESS              = 15,
  DXGI_FORMAT_R32G32_FLOAT                 = 16,
  DXGI_FORMAT_R32G32_UINT                  = 17,
  DXGI_FORMAT_R32G32_SINT                  = 18,
  DXGI_FORMAT_R32G8X24_TYPELESS            = 19,
  DXGI_FORMAT_D32_FLOAT_S8X24_UINT         = 20,
  DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS     = 21,
  DXGI_FORMAT_X32_TYPELESS_G8X24_UINT      = 22,
  DXGI_FORMAT_R10G10B10A2_TYPELESS         = 23,
  DXGI_FORMAT_R10G10B10A2_UNORM            = 24,
  DXGI_FORMAT_R10G10B10A2_UINT             = 25,
  DXGI_FORMAT_R11G11B10_FLOAT              = 26,
  DXGI_FORMAT_R8G8B8A8_TYPELESS            = 27,
  DXGI_FORMAT_R8G8B8A8_UNORM               = 28,
  DXGI_FORMAT_R8G8B8A8_UNORM_SRGB          = 29,
  DXGI_FORMAT_R8G8B8A8_UINT                = 30,
  DXGI_FORMAT_R8G8B8A8_SNORM               = 31,
  DXGI_FORMAT_R8G8B8A8_SINT                = 32,
  DXGI_FORMAT_R16G16_TYPELESS              = 33,
  DXGI_FORMAT_R16G16_FLOAT                 = 34,
  DXGI_FORMAT_R16G16_UNORM                 = 35,
  DXGI_FORMAT_R16G16_UINT                  = 36,
  DXGI_FORMAT_R16G16_SNORM                 = 37,
  DXGI_FORMAT_R16G16_SINT                  = 38,
  DXGI_FORMAT_R32_TYPELESS                 = 39,
  DXGI_FORMAT_D32_FLOAT                    = 40,
  DXGI_FORMAT_R32_FLOAT                    = 41,
  DXGI_FORMAT_R32_UINT                     = 42,
  DXGI_FORMAT_R32_SINT                     = 43,
  DXGI_FORMAT_R24G8_TYPELESS               = 44,
  DXGI_FORMAT_D24_UNORM_S8_UINT            = 45,
  DXGI_FORMAT_R24_UNORM_X8_TYPELESS        = 46,
  DXGI_FORMAT_X24_TYPELESS_G8_UINT         = 47,
  DXGI_FORMAT_R8G8_TYPELESS                = 48,
  DXGI_FORMAT_R8G8_UNORM                   = 49,
  DXGI_FORMAT_R8G8_UINT                    = 50,
  DXGI_FORMAT_R8G8_SNORM                   = 51,
  DXGI_FORMAT_R8G8_SINT                    = 52,
  DXGI_FORMAT_R16_TYPELESS                 = 53,
  DXGI_FORMAT_R16_FLOAT                    = 54,
  DXGI_FORMAT_D16_UNORM                    = 55,
  DXGI_FORMAT_R16_UNORM                    = 56,
  DXGI_FORMAT_R16_UINT                     = 57,
  DXGI_FORMAT_R16_SNORM                    = 58,
  DXGI_FORMAT_R16_SINT                     = 59,
  DXGI_FORMAT_R8_TYPELESS                  = 60,
  DXGI_FORMAT_R8_UNORM                     = 61,
  DXGI_FORMAT_R8_UINT                      = 62,
  DXGI_FORMAT_R8_SNORM                     = 63,
  DXGI_FORMAT_R8_SINT                      = 64,
  DXGI_FORMAT_A8_UNORM                     = 65,
  DXGI_FORMAT_R1_UNORM                     = 66,
  DXGI_FORMAT_R9G9B9E5_SHAREDEXP           = 67,
  DXGI_FORMAT_R8G8_B8G8_UNORM              = 68,
  DXGI_FORMAT_G8R8_G8B8_UNORM              = 69,
  DXGI_FORMAT_BC1_TYPELESS                 = 70,
  DXGI_FORMAT_BC1_UNORM                    = 71,
  DXGI_FORMAT_BC1_UNORM_SRGB               = 72,
  DXGI_FORMAT_BC2_TYPELESS                 = 73,
  DXGI_FORMAT_BC2_UNORM                    = 74,
  DXGI_FORMAT_BC2_UNORM_SRGB               = 75,
  DXGI_FORMAT_BC3_TYPELESS                 = 76,
  DXGI_FORMAT_BC3_UNORM                    = 77,
  DXGI_FORMAT_BC3_UNORM_SRGB               = 78,
  DXGI_FORMAT_BC4_TYPELESS                 = 79,
  DXGI_FORMAT_BC4_UNORM                    = 80,
  DXGI_FORMAT_BC4_SNORM                    = 81,
  DXGI_FORMAT_BC5_TYPELESS                 = 82,
  DXGI_FORMAT_BC5_UNORM                    = 83,
  DXGI_FORMAT_BC5_SNORM                    = 84,
  DXGI_FORMAT_B5G6R5_UNORM                 = 85,
  DXGI_FORMAT_B5G5R5A1_UNORM               = 86,
  DXGI_FORMAT_B8G8R8A8_UNORM               = 87,
  DXGI_FORMAT_B8G8R8X8_UNORM               = 88,
  DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM   = 89,
  DXGI_FORMAT_B8G8R8A8_TYPELESS            = 90,
  DXGI_FORMAT_B8G8R8A8_UNORM_SRGB          = 91,
  DXGI_FORMAT_B8G8R8X8_TYPELESS            = 92,
  DXGI_FORMAT_B8G8R8X8_UNORM_SRGB          = 93,
  DXGI_FORMAT_BC6H_TYPELESS                = 94,
  DXGI_FORMAT_BC6H_UF16                    = 95,
  DXGI_FORMAT_BC6H_SF16                    = 96,
  DXGI_FORMAT_BC7_TYPELESS                 = 97,
  DXGI_FORMAT_BC7_UNORM                    = 98,
  DXGI_FORMAT_BC7_UNORM_SRGB               = 99,
  DXGI_FORMAT_FORCE_UINT                   = 0xffffffffUL 
} DXGI_FORMAT, *LPDXGI_FORMAT;

typedef struct {
  DXGI_FORMAT              dxgiFormat;
  D3D10_RESOURCE_DIMENSION resourceDimension;
  GR_UINT32                     miscFlag;
  GR_UINT32                     arraySize;
  GR_UINT32                     reserved;
} DDS_HEADER_DXT10;

Texture::Texture(GR_UINT gpu) :
    Image(gpu),
    DDS_FOURCC( 0x00000004),     // DDPF_FOURCC
    DDS_RGB( 0x00000040),        // DDPF_RGB
    DDS_RGBA( 0x00000041),       // DDPF_RGB | DDPF_ALPHAPIXELS
    DDS_LUMINANCE( 0x00020000),  // DDPF_LUMINANCE
    DDS_ALPHA( 0x00000002)       // DDPF_ALPHA
{
}

Texture::~Texture()
{   
}

GR_RESULT Texture::initFromFile( CommandBuffer* cmdBuffer, unsigned int heap, const WCHAR* pFilename )
{
    // get the last 4 char (assuming this is the file extension)
    GR_SIZE len = wcslen(pFilename);
    char ext[5] = {0};
    size_t numConverted = 0;
    wcstombs_s( &numConverted, ext, 5, &pFilename[len-4], 4 );
    for( int i = 0; i<4;++i)
    {
        ext[i] = static_cast<char>(tolower(ext[i]));
    }

    // check if the extension is known
    GR_UINT32 ext4CC = *reinterpret_cast<const GR_UINT32*>(ext);
    switch(ext4CC)
    {
    case 'sdd.':
        {
            GR_RESULT result = GR_ERROR_UNAVAILABLE;

            if( GetFileAttributes( pFilename ) != 0xFFFFFFFF )
            {
                HANDLE hFile = CreateFile( pFilename,             // file to open
                                           GENERIC_READ,          // open for reading
                                           FILE_SHARE_READ,       // share for reading
                                           NULL,                  // default security
                                           OPEN_EXISTING,         // existing file only
                                           FILE_ATTRIBUTE_NORMAL, // normal file
                                           NULL);                 // no attr. template
                if (hFile != INVALID_HANDLE_VALUE) 
                { 
                    LARGE_INTEGER fileSize;
                    GetFileSizeEx( hFile, &fileSize );
                    assert(0==fileSize.HighPart);
                    void* pData = new char[fileSize.LowPart];

                    DWORD dwBytesRead = 0;
                    if( FALSE != ReadFile(hFile, pData, fileSize.LowPart, &dwBytesRead, NULL) )
                    {
                        result = initFromDdsInMemory( cmdBuffer, heap, fileSize.LowPart, pData );
                    }

                    delete[] pData;
                    CloseHandle( hFile);
                }
            }
            if( result != GR_SUCCESS )
            {
                OutputDebugString( TEXT(__FUNCTION__) TEXT("failed.\n"));
            }

            return GR_ERROR_UNAVAILABLE;    
        }
    default:
        return GR_ERROR_INVALID_OBJECT_TYPE;
    }
}

GR_RESULT Texture::initFromDdsInMemory( CommandBuffer* cmdBuffer, unsigned int heap, GR_UINT size, GR_VOID* pData)
{
    unsigned char*    pByteData = reinterpret_cast<unsigned char*>(pData);
    GR_UINT32       dwMagic = *reinterpret_cast<GR_UINT32*>(pByteData);
    pByteData += 4;

    DDS_HEADER*        header = reinterpret_cast<DDS_HEADER*>(pByteData);
    pByteData += sizeof( DDS_HEADER );

    DDS_HEADER_DXT10* header10 = NULL;
    if( dwMagic == '01XD' ) // "DX10"
    {
        header10 = reinterpret_cast<DDS_HEADER_DXT10*>(&pByteData[4]);;
        pByteData += sizeof( DDS_HEADER_DXT10 );
    }
    else if( dwMagic != ' SDD' ) // "DDS "
    {
        return GR_ERROR_INVALID_FORMAT;
    }
    
    return CreateTextureFromDDS( cmdBuffer, heap, header, pByteData, size - (pByteData - reinterpret_cast<unsigned char*>(pData)) );
}

GR_RESULT Texture::initFromTex( CommandBuffer* cmdBuffer, unsigned int heap, const Texture* pTex )
{
    return GR_SUCCESS;
}

GR_RESULT Texture::initVirtual( CommandBuffer* cmdBuffer, unsigned int heap, const Texture* pTex )
{
    return GR_SUCCESS;
}

void Texture::update( const Texture* pTex )
{
}

//--------------------------------------------------------------------------------------
#define ISBITMASK( r,g,b,a ) ( ddpf.dwRBitMask == r && ddpf.dwGBitMask == g && ddpf.dwBBitMask == b && ddpf.dwABitMask == a )

//--------------------------------------------------------------------------------------
D3DFORMAT Texture::GetD3D9Format( const Texture::DDS_PIXELFORMAT& ddpf )
{
    if( ddpf.dwFlags & Texture::DDS_RGB )
    {
        switch (ddpf.dwRGBBitCount)
        {
        case 32:
            if( ISBITMASK(0x00ff0000,0x0000ff00,0x000000ff,0xff000000) )
                return D3DFMT_A8R8G8B8;
            if( ISBITMASK(0x00ff0000,0x0000ff00,0x000000ff,0x00000000) )
                return D3DFMT_X8R8G8B8;
            if( ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0xff000000) )
                return D3DFMT_A8B8G8R8;
            if( ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0x00000000) )
                return D3DFMT_X8B8G8R8;

            // Note that many common DDS reader/writers swap the
            // the RED/BLUE masks for 10:10:10:2 formats. We assumme
            // below that the 'correct' header mask is being used
            if( ISBITMASK(0x3ff00000,0x000ffc00,0x000003ff,0xc0000000) )
                return D3DFMT_A2R10G10B10;
            if( ISBITMASK(0x000003ff,0x000ffc00,0x3ff00000,0xc0000000) )
                return D3DFMT_A2B10G10R10;

            if( ISBITMASK(0x0000ffff,0xffff0000,0x00000000,0x00000000) )
                return D3DFMT_G16R16;
            if( ISBITMASK(0xffffffff,0x00000000,0x00000000,0x00000000) )
                return D3DFMT_R32F; // D3DX writes this out as a FourCC of 114
            break;

        case 24:
            if( ISBITMASK(0x00ff0000,0x0000ff00,0x000000ff,0x00000000) )
                return D3DFMT_R8G8B8;
            break;

        case 16:
            if( ISBITMASK(0x0000f800,0x000007e0,0x0000001f,0x00000000) )
                return D3DFMT_R5G6B5;
            if( ISBITMASK(0x00007c00,0x000003e0,0x0000001f,0x00008000) )
                return D3DFMT_A1R5G5B5;
            if( ISBITMASK(0x00007c00,0x000003e0,0x0000001f,0x00000000) )
                return D3DFMT_X1R5G5B5;
            if( ISBITMASK(0x00000f00,0x000000f0,0x0000000f,0x0000f000) )
                return D3DFMT_A4R4G4B4;
            if( ISBITMASK(0x00000f00,0x000000f0,0x0000000f,0x00000000) )
                return D3DFMT_X4R4G4B4;
            if( ISBITMASK(0x000000e0,0x0000001c,0x00000003,0x0000ff00) )
                return D3DFMT_A8R3G3B2;
            break;
        }
    }
    else if( ddpf.dwFlags & DDS_LUMINANCE )
    {
        if( 8 == ddpf.dwRGBBitCount )
        {
            if( ISBITMASK(0x0000000f,0x00000000,0x00000000,0x000000f0) )
                return D3DFMT_A4L4;
            if( ISBITMASK(0x000000ff,0x00000000,0x00000000,0x00000000) )
                return D3DFMT_L8;
        }

        if( 16 == ddpf.dwRGBBitCount )
        {
            if( ISBITMASK(0x0000ffff,0x00000000,0x00000000,0x00000000) )
                return D3DFMT_L16;
            if( ISBITMASK(0x000000ff,0x00000000,0x00000000,0x0000ff00) )
                return D3DFMT_A8L8;
        }
    }
    else if( ddpf.dwFlags & DDS_ALPHA )
    {
        if( 8 == ddpf.dwRGBBitCount )
        {
            return D3DFMT_A8;
        }
    }
    else if( ddpf.dwFlags & DDS_FOURCC )
    {
        if( MAKEFOURCC( 'D', 'X', 'T', '1' ) == ddpf.dwFourCC )
            return D3DFMT_DXT1;
        if( MAKEFOURCC( 'D', 'X', 'T', '2' ) == ddpf.dwFourCC )
            return D3DFMT_DXT2;
        if( MAKEFOURCC( 'D', 'X', 'T', '3' ) == ddpf.dwFourCC )
            return D3DFMT_DXT3;
        if( MAKEFOURCC( 'D', 'X', 'T', '4' ) == ddpf.dwFourCC )
            return D3DFMT_DXT4;
        if( MAKEFOURCC( 'D', 'X', 'T', '5' ) == ddpf.dwFourCC )
            return D3DFMT_DXT5;

        if( MAKEFOURCC( 'R', 'G', 'B', 'G' ) == ddpf.dwFourCC )
            return D3DFMT_R8G8_B8G8;
        if( MAKEFOURCC( 'G', 'R', 'G', 'B' ) == ddpf.dwFourCC )
            return D3DFMT_G8R8_G8B8;

        if( MAKEFOURCC( 'U', 'Y', 'V', 'Y' ) == ddpf.dwFourCC )
            return D3DFMT_UYVY;
        if( MAKEFOURCC( 'Y', 'U', 'Y', '2' ) == ddpf.dwFourCC )
            return D3DFMT_YUY2;

        // Check for D3DFORMAT enums being set here
        switch( ddpf.dwFourCC )
        {
        case D3DFMT_A16B16G16R16:
        case D3DFMT_Q16W16V16U16:
        case D3DFMT_R16F:
        case D3DFMT_G16R16F:
        case D3DFMT_A16B16G16R16F:
        case D3DFMT_R32F:
        case D3DFMT_G32R32F:
        case D3DFMT_A32B32G32R32F:
        case D3DFMT_CxV8U8:
            return (D3DFORMAT)ddpf.dwFourCC;
        }
    }

    return D3DFMT_UNKNOWN;
}

//--------------------------------------------------------------------------------------
GR_RESULT Texture::CreateTextureFromDDS( CommandBuffer* cmdBuffer, unsigned int heap, DDS_HEADER* header, const unsigned char* data, GR_UINT64 size)
{
    GR_RESULT result = GR_SUCCESS;

    m_width = header->dwWidth;
    m_height = header->dwHeight;

    UINT iWidth = header->dwWidth;
    UINT iHeight = header->dwHeight;
    UINT iMipCount = header->dwMipMapCount;
    if( 0 == iMipCount )
        iMipCount = 1;

    if (header->dwCubemapFlags != 0
        || (header->dwHeaderFlags & DDS_HEADER_FLAGS_VOLUME) )
    {
        // For now only support 2D textures, not cubemaps or volumes
        return GR_UNSUPPORTED;
    }

    D3DFORMAT fmt = GetD3D9Format( header->ddspf );
    if( fmt != D3DFMT_A8R8G8B8 )
    {
        return GR_UNSUPPORTED;
    }

    GR_FORMAT format = {};
    if( fmt == D3DFMT_A8R8G8B8 )
    {
        format.channelFormat = GR_CH_FMT_R8G8B8A8;
    }
    else if(fmt == D3DFMT_DXT5)
    {
        format.channelFormat = GR_CH_FMT_BC3;
    }
    else
        return GR_UNSUPPORTED;
    format.numericFormat = GR_NUM_FMT_UNORM;
    m_format = format;

    result = init( heap, iWidth, iHeight, format, GR_IMAGE_USAGE_SHADER_ACCESS_READ, iMipCount, true );

    if (result == GR_SUCCESS)
    {
        // Lock, fill, unlock
        UINT RowBytes, NumRows;
         const BYTE* pSrcBits = data;

        for( UINT i = 0; i < iMipCount; i++ )
        {
            GetSurfaceInfo( iWidth, iHeight, D3DFMT_A8R8G8B8, NULL, &RowBytes, &NumRows );

            GR_SUBRESOURCE_LAYOUT srLayout={};
            BYTE*    pData = reinterpret_cast<BYTE*>(Map(i, 0, &srLayout));

            if (pData != NULL)
            {
                // Copy stride line by line
                for( UINT h = 0; h < NumRows; h++ )
                {
                    memcpy( pData, pSrcBits, RowBytes );
                    pData += srLayout.rowPitch;
                    pSrcBits += RowBytes;
                }
                _grUnmapMemory(m_memory);
            }
            
            iWidth = iWidth >> 1;
            iHeight = iHeight >> 1;
            if( iWidth == 0 )
                iWidth = 1;
            if( iHeight == 0 )
                iHeight = 1;
        }
    }

    // prepare texture for shader read
    prepare( cmdBuffer, GR_IMAGE_STATE_GRAPHICS_SHADER_READ_ONLY);

    return result;
}

GR_VOID Texture::Unmap()
{
	_grUnmapMemory(m_memory);
}

GR_VOID* Texture::Map( GR_UINT      mipLevel,
                       GR_FLAGS     mapFlags,
                       GR_SUBRESOURCE_LAYOUT* psrInfo
                     ) const
{
    GR_VOID* pData = GR_NULL_HANDLE;
	const GR_IMAGE_SUBRESOURCE subresource = { GR_IMAGE_ASPECT_COLOR, mipLevel, 0 };
   
    GR_SIZE infoSize = sizeof(GR_SUBRESOURCE_LAYOUT);
	GR_RESULT result = _grGetImageSubresourceInfo(m_image, &subresource,
                                        GR_INFO_TYPE_SUBRESOURCE_LAYOUT,
                                        &infoSize, psrInfo);
	
    if ((result == GR_SUCCESS) &&
        (_grMapMemory(m_memory, mapFlags, &pData) == GR_SUCCESS))
    {
        pData = (static_cast<GR_BYTE*>(pData) + m_offset + psrInfo->offset);
    }

    return pData;
}

//--------------------------------------------------------------------------------------
// Return the BPP for a particular format
//--------------------------------------------------------------------------------------
GR_UINT32 Texture::BitsPerPixel( D3DFORMAT fmt )
{
    UINT fmtU = ( UINT )fmt;
    switch( fmtU )
    {
        case D3DFMT_A32B32G32R32F:
            return 128;

        case D3DFMT_A16B16G16R16:
        case D3DFMT_Q16W16V16U16:
        case D3DFMT_A16B16G16R16F:
        case D3DFMT_G32R32F:
            return 64;

        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
        case D3DFMT_A2B10G10R10:
        case D3DFMT_A8B8G8R8:
        case D3DFMT_X8B8G8R8:
        case D3DFMT_G16R16:
        case D3DFMT_A2R10G10B10:
        case D3DFMT_Q8W8V8U8:
        case D3DFMT_V16U16:
        case D3DFMT_X8L8V8U8:
        case D3DFMT_A2W10V10U10:
        case D3DFMT_D32:
        case D3DFMT_D24S8:
        case D3DFMT_D24X8:
        case D3DFMT_D24X4S4:
        case D3DFMT_D32F_LOCKABLE:
        case D3DFMT_D24FS8:
        case D3DFMT_INDEX32:
        case D3DFMT_G16R16F:
        case D3DFMT_R32F:
            return 32;

        case D3DFMT_R8G8B8:
            return 24;

        case D3DFMT_A4R4G4B4:
        case D3DFMT_X4R4G4B4:
        case D3DFMT_R5G6B5:
        case D3DFMT_L16:
        case D3DFMT_A8L8:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A8R3G3B2:
        case D3DFMT_V8U8:
        case D3DFMT_CxV8U8:
        case D3DFMT_L6V5U5:
        case D3DFMT_G8R8_G8B8:
        case D3DFMT_R8G8_B8G8:
        case D3DFMT_D16_LOCKABLE:
        case D3DFMT_D15S1:
        case D3DFMT_D16:
        case D3DFMT_INDEX16:
        case D3DFMT_R16F:
        case D3DFMT_YUY2:
            return 16;

        case D3DFMT_R3G3B2:
        case D3DFMT_A8:
        case D3DFMT_A8P8:
        case D3DFMT_P8:
        case D3DFMT_L8:
        case D3DFMT_A4L4:
            return 8;

        case D3DFMT_DXT1:
            return 4;

        case D3DFMT_DXT2:
        case D3DFMT_DXT3:
        case D3DFMT_DXT4:
        case D3DFMT_DXT5:
            return  8;

            // From DX docs, reference/d3d/enums/d3dformat.asp
            // (note how it says that D3DFMT_R8G8_B8G8 is "A 16-bit packed RGB format analogous to UYVY (U0Y0, V0Y1, U2Y2, and so on)")
        case D3DFMT_UYVY:
            return 16;

            // http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directshow/htm/directxvideoaccelerationdxvavideosubtypes.asp
        case MAKEFOURCC( 'A', 'I', '4', '4' ):
        case MAKEFOURCC( 'I', 'A', '4', '4' ):
            return 8;

        case MAKEFOURCC( 'Y', 'V', '1', '2' ):
            return 12;

#if !defined(D3D_DISABLE_9EX)
        case D3DFMT_D32_LOCKABLE:
            return 32;

        case D3DFMT_S8_LOCKABLE:
            return 8;

        case D3DFMT_A1:
            return 1;
#endif // !D3D_DISABLE_9EX

        default:
            return 0;
    }
}

void Texture::GetSurfaceInfo( UINT width, UINT height, D3DFORMAT fmt, UINT* pNumBytes, UINT* pRowBytes, UINT* pNumRows )
{
    UINT numBytes = 0;
    UINT rowBytes = 0;
    UINT numRows = 0;

    // From the DXSDK docs:
    //
    //     When computing DXTn compressed sizes for non-square textures, the 
    //     following formula should be used at each mipmap level:
    //
    //         max(1, width ÷ 4) x max(1, height ÷ 4) x 8(DXT1) or 16(DXT2-5)
    //
    //     The pitch for DXTn formats is different from what was returned in 
    //     Microsoft DirectX 7.0. It now refers the pitch of a row of blocks. 
    //     For example, if you have a width of 16, then you will have a pitch 
    //     of four blocks (4*8 for DXT1, 4*16 for DXT2-5.)"

    if( fmt == D3DFMT_DXT1 || fmt == D3DFMT_DXT2 || fmt == D3DFMT_DXT3 || fmt == D3DFMT_DXT4 || fmt == D3DFMT_DXT5 )
    {
        int numBlocksWide = 0;
		int one = 1;
		int w4 = width / 4;
		int h4 = height / 4;

        if( width > 0 )
            numBlocksWide = eastl::max( one, w4 );
        int numBlocksHigh = 0;
        if( height > 0 )
            numBlocksHigh = eastl::max( one, h4 );
        int numBytesPerBlock = ( fmt == D3DFMT_DXT1 ? 8 : 16 );
        rowBytes = numBlocksWide * numBytesPerBlock;
        numRows = numBlocksHigh;
    }
    else
    {
        UINT bpp = BitsPerPixel( fmt );
        rowBytes = ( width * bpp + 7 ) / 8; // round up to nearest byte
        numRows = height;
    }
    numBytes = rowBytes * numRows;
    if( pNumBytes != NULL )
        *pNumBytes = numBytes;
    if( pRowBytes != NULL )
        *pRowBytes = rowBytes;
    if( pNumRows != NULL )
        *pNumRows = numRows;
}


GR_RESULT Texture::createTextureFromMemARGB8(GR_UINT32 heap, CommandBuffer* cmdBuffer, const unsigned char* data, GR_UINT32 iWidth, GR_UINT32 iHeight)
{
	GR_FORMAT format = { GR_CH_FMT_R8G8B8A8, GR_NUM_FMT_UNORM };

	GR_RESULT result = init(heap, iWidth, iHeight, format, GR_IMAGE_USAGE_SHADER_ACCESS_READ, 1, true);

	if (result == GR_SUCCESS)
	{
		// Lock, fill, unlock
		UINT RowBytes = width() * 4 /*ARGB*/;
		UINT NumRows = height();
		const BYTE* pSrcBits = data;

		GR_SUBRESOURCE_LAYOUT srLayout = {};
		BYTE*    pData = reinterpret_cast<BYTE*>(Map(0, 0, &srLayout));
		// find the right subresource
		// map memory
		if (pData != NULL)
		{
			for (UINT h = 0; h < NumRows; h++)
			{
				memcpy(pData, pSrcBits, RowBytes);
				pData += srLayout.rowPitch;
				pSrcBits += RowBytes;
			}
			_grUnmapMemory(m_memory);
		}
	}

	// prepare texture for shader read
	prepare(cmdBuffer, GR_IMAGE_STATE_GRAPHICS_SHADER_READ_ONLY);

	return result;
}


GR_RESULT Texture::updateTextureFromMemARGB8(const unsigned char* data)
{
	// Lock, fill, unlock
	UINT RowBytes = width() * 4 /*ARGB*/;
	UINT NumRows = height();
	const BYTE* pSrcBits = data;

	GR_RESULT result = GR_SUCCESS;

	GR_SUBRESOURCE_LAYOUT srLayout = {};
	BYTE*    pData = reinterpret_cast<BYTE*>(Map(0, 0, &srLayout));
	// find the right subresource
	// map memory
	if (pData != NULL)
	{
		for (UINT h = 0; h < NumRows; h++)
		{
			memcpy(pData, pSrcBits, RowBytes);
			pData += srLayout.rowPitch;
			pSrcBits += RowBytes;
		}
		_grUnmapMemory(m_memory);
	}

	return result;
}