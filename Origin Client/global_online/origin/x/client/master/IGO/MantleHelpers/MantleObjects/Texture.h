#pragma once
#include <Windows.h>

#include <mantle.h>
#include "Image.h"


#if !defined(MAKEFOURCC)
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
        (unsigned int(unsigned char(ch0)) | (unsigned int(unsigned char(ch1)) << 8) | \
        (unsigned int(unsigned char(ch2)) << 16) | (unsigned int(unsigned char(ch3)) << 24 ))
#endif


typedef enum _D3DFORMAT
{
    D3DFMT_UNKNOWN              =  0,

    D3DFMT_R8G8B8               = 20,
    D3DFMT_A8R8G8B8             = 21,
    D3DFMT_X8R8G8B8             = 22,
    D3DFMT_R5G6B5               = 23,
    D3DFMT_X1R5G5B5             = 24,
    D3DFMT_A1R5G5B5             = 25,
    D3DFMT_A4R4G4B4             = 26,
    D3DFMT_R3G3B2               = 27,
    D3DFMT_A8                   = 28,
    D3DFMT_A8R3G3B2             = 29,
    D3DFMT_X4R4G4B4             = 30,
    D3DFMT_A2B10G10R10          = 31,
    D3DFMT_A8B8G8R8             = 32,
    D3DFMT_X8B8G8R8             = 33,
    D3DFMT_G16R16               = 34,
    D3DFMT_A2R10G10B10          = 35,
    D3DFMT_A16B16G16R16         = 36,

    D3DFMT_A8P8                 = 40,
    D3DFMT_P8                   = 41,

    D3DFMT_L8                   = 50,
    D3DFMT_A8L8                 = 51,
    D3DFMT_A4L4                 = 52,

    D3DFMT_V8U8                 = 60,
    D3DFMT_L6V5U5               = 61,
    D3DFMT_X8L8V8U8             = 62,
    D3DFMT_Q8W8V8U8             = 63,
    D3DFMT_V16U16               = 64,
    D3DFMT_A2W10V10U10          = 67,

    D3DFMT_UYVY                 = MAKEFOURCC('U', 'Y', 'V', 'Y'),
    D3DFMT_R8G8_B8G8            = MAKEFOURCC('R', 'G', 'B', 'G'),
    D3DFMT_YUY2                 = MAKEFOURCC('Y', 'U', 'Y', '2'),
    D3DFMT_G8R8_G8B8            = MAKEFOURCC('G', 'R', 'G', 'B'),
    D3DFMT_DXT1                 = MAKEFOURCC('D', 'X', 'T', '1'),
    D3DFMT_DXT2                 = MAKEFOURCC('D', 'X', 'T', '2'),
    D3DFMT_DXT3                 = MAKEFOURCC('D', 'X', 'T', '3'),
    D3DFMT_DXT4                 = MAKEFOURCC('D', 'X', 'T', '4'),
    D3DFMT_DXT5                 = MAKEFOURCC('D', 'X', 'T', '5'),

    D3DFMT_D16_LOCKABLE         = 70,
    D3DFMT_D32                  = 71,
    D3DFMT_D15S1                = 73,
    D3DFMT_D24S8                = 75,
    D3DFMT_D24X8                = 77,
    D3DFMT_D24X4S4              = 79,
    D3DFMT_D16                  = 80,

    D3DFMT_D32F_LOCKABLE        = 82,
    D3DFMT_D24FS8               = 83,

/* D3D9Ex only -- */
#if !defined(D3D_DISABLE_9EX)

    /* Z-Stencil formats valid for CPU access */
    D3DFMT_D32_LOCKABLE         = 84,
    D3DFMT_S8_LOCKABLE          = 85,

#endif // !D3D_DISABLE_9EX
/* -- D3D9Ex only */


    D3DFMT_L16                  = 81,

    D3DFMT_VERTEXDATA           =100,
    D3DFMT_INDEX16              =101,
    D3DFMT_INDEX32              =102,

    D3DFMT_Q16W16V16U16         =110,

    D3DFMT_MULTI2_ARGB8         = MAKEFOURCC('M','E','T','1'),

    // Floating point surface formats

    // s10e5 formats (16-bits per channel)
    D3DFMT_R16F                 = 111,
    D3DFMT_G16R16F              = 112,
    D3DFMT_A16B16G16R16F        = 113,

    // IEEE s23e8 formats (32-bits per channel)
    D3DFMT_R32F                 = 114,
    D3DFMT_G32R32F              = 115,
    D3DFMT_A32B32G32R32F        = 116,

    D3DFMT_CxV8U8               = 117,

/* D3D9Ex only -- */
#if !defined(D3D_DISABLE_9EX)

    // Monochrome 1 bit per pixel format
    D3DFMT_A1                   = 118,

    // 2.8 biased fixed point
    D3DFMT_A2B10G10R10_XR_BIAS  = 119,


    // Binary format indicating that the data has no inherent type
    D3DFMT_BINARYBUFFER         = 199,
    
#endif // !D3D_DISABLE_9EX
/* -- D3D9Ex only */


    D3DFMT_FORCE_DWORD          =0x7fffffff
} D3DFORMAT;

class CommandBuffer;

class Texture : public Image
{
public:
    Texture(GR_UINT gpu);
    virtual ~Texture(); 
    
	GR_RESULT updateTextureFromMemARGB8(const unsigned char* data);
	GR_RESULT createTextureFromMemARGB8(GR_UINT32 heap, CommandBuffer* cmdBuffer, const unsigned char* data, GR_UINT32 iWidth, GR_UINT32 iHeight);

	GR_RESULT   initFromFile( CommandBuffer* cmdBuffer, unsigned int heap, const WCHAR* szFilename);
    GR_RESULT   initFromDdsInMemory( CommandBuffer* cmdBuffer, unsigned int heap, GR_UINT size, GR_VOID* pData);
    GR_RESULT   initFromTex( CommandBuffer* cmdBuffer, unsigned int heap, const Texture* pTex );

    GR_RESULT   initVirtual( CommandBuffer* cmdBuffer, unsigned int heap, const Texture* pTex );
    void        update( const Texture* pTex );

	GR_VOID*    Map(GR_UINT mipLevel, GR_FLAGS mapFlags, GR_SUBRESOURCE_LAYOUT* psrInfo) const;
	GR_VOID     Unmap();

private:
    typedef struct {
        GR_UINT32 dwSize;
        GR_UINT32 dwFlags;
        GR_UINT32 dwFourCC;
        GR_UINT32 dwRGBBitCount;
        GR_UINT32 dwRBitMask;
        GR_UINT32 dwGBitMask;
        GR_UINT32 dwBBitMask;
        GR_UINT32 dwABitMask;
    } DDS_PIXELFORMAT;
    const int DDS_FOURCC;
    const int DDS_RGB;
    const int DDS_RGBA;
    const int DDS_LUMINANCE;
    const int DDS_ALPHA;

    typedef struct {
        GR_UINT32           dwSize;
        GR_UINT32           dwHeaderFlags;
        GR_UINT32           dwHeight;
        GR_UINT32           dwWidth;
        GR_UINT32           dwPitchOrLinearSize;
        GR_UINT32           dwDepth;
        GR_UINT32           dwMipMapCount;
        GR_UINT32           dwReserved1[11];
        DDS_PIXELFORMAT     ddspf;
        GR_UINT32           dwSurfaceFlags;
        GR_UINT32           dwCubemapFlags;
        GR_UINT32           dwCaps3;
        GR_UINT32           dwCaps4;
        GR_UINT32           dwReserved2;
    } DDS_HEADER;

    // actual function to create texture from DDS
    GR_RESULT    CreateTextureFromDDS( CommandBuffer* cmdBuffer, unsigned int heap, DDS_HEADER* header, const unsigned char* data, GR_UINT64 size );

    //helper functions
    void        GetSurfaceInfo( GR_UINT32 width, GR_UINT32 height, D3DFORMAT fmt, GR_UINT32* pNumBytes, GR_UINT32* pRowBytes, GR_UINT32* pNumRows );
    GR_UINT32   BitsPerPixel( D3DFORMAT fmt );
    D3DFORMAT   GetD3D9Format( const Texture::DDS_PIXELFORMAT& ddpf );
private:
};