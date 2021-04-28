/*H********************************************************************************/
/*!
    \File dirtyjpg.c

    \Description
        Implements JFIF image decoding to an output 32bit buffer.  This decoder
        does not support progressive, lossless, or arithmetic options.

    \Notes
        References:
            [1] http://www.w3.org/Graphics/JPEG/jfif3.pdf
            [2] http://www.w3.org/Graphics/JPEG/itu-t81.pdf
            [3] http://class.ece.iastate.edu/ee528/Reading%20material/JPEG_File_Format.pdf
            [4] http://www.bsdg.org/SWAG/GRAPHICS/0143.PAS.html

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 02/23/2006 (jbrookes) First version, based on gshaefer code
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "dirtylib.h"
#include "dirtymem.h"

#include "dirtyjpg.h"

/*** Defines **********************************************************************/

#define DIRTYJPG_DEBUG         (TRUE)
#define DIRTYJPG_VERBOSE       (DIRTYJPG_DEBUG && FALSE)
#define DIRTYJPG_DEBUG_HUFF    (DIRTYJPG_DEBUG && FALSE)
#define DIRTYJPG_DEBUG_IDCT    (DIRTYJPG_DEBUG && FALSE)

#if !DIRTYJPG_DEBUG_IDCT
 #define _PrintMatrix16(_pMatrix, _pTitle) {;}
#endif

#define MCU_SIZE    (8*8*4)         // 8x8 32-bit
#define MCU_MAXSIZE (MCU_SIZE*4*4)  // max size is 4x4 blocks

// LLM constants
#define DCTSIZE 8
#define CONST_BITS 13
#define PASS1_BITS 2
#define FIX_0_298631336  ((int32_t)  2446)	/* FIX(0.298631336) */
#define FIX_0_390180644  ((int32_t)  3196)	/* FIX(0.390180644) */
#define FIX_0_541196100  ((int32_t)  4433)	/* FIX(0.541196100) */
#define FIX_0_765366865  ((int32_t)  6270)	/* FIX(0.765366865) */
#define FIX_0_899976223  ((int32_t)  7373)	/* FIX(0.899976223) */
#define FIX_1_175875602  ((int32_t)  9633)	/* FIX(1.175875602) */
#define FIX_1_501321110  ((int32_t)  12299)	/* FIX(1.501321110) */
#define FIX_1_847759065  ((int32_t)  15137)	/* FIX(1.847759065) */
#define FIX_1_961570560  ((int32_t)  16069)	/* FIX(1.961570560) */
#define FIX_2_053119869  ((int32_t)  16819)	/* FIX(2.053119869) */
#define FIX_2_562915447  ((int32_t)  20995)	/* FIX(2.562915447) */
#define FIX_3_072711026  ((int32_t)  25172)	/* FIX(3.072711026) */

/*** Type Definitions *************************************************************/

//! quantization/matrix
typedef uint16_t QuantTableT[64];

//! huffman decode table head
typedef struct HuffHeadT
{
    uint16_t uShift;        //!< number of shifts to normalize
    uint16_t uMinCode;      //!< min code in this table
    uint16_t uMaxCode;      //!< max code in this table
    uint16_t uOffset;       //!< offset to value/shift data
} HuffHeadT;

//! huffman decode table
typedef struct HuffTableT
{
    HuffHeadT Head[10];     //!< table code range
    uint8_t Code[512];      //!< code lookup
    uint8_t Step[512];      //!< code length lookup
} HuffTableT;

//! component table
typedef struct CompTableT
{
    uint16_t uMcuHorz;      //!< number of 8x8 blocks horizontally in an mcu
    uint16_t uMcuVert;      //!< number of 8x8 blocks vertically in an mcu
    uint16_t uCompHorz;     //!< horizontal sampling factor
    uint16_t uCompVert;     //!< vertical sampling factor
    uint16_t uExpHorz;      //!< horizontal expansion factor
    uint16_t uExpVert;      //!< vertical expansion factor

    uint32_t uStepHorz0;    //!< horizontal step
    uint32_t uStepVert0;    //!< vertical step
    uint32_t uStepHorz1;    //!< horizontal scaled step  
    uint32_t uStepVert1;    //!< vertical scaled stemp
    uint32_t uStepHorz8;    //!< horizontal step*8
    uint32_t uStepVert8;    //!< horizontal step*8
    uint32_t uStepHorzM;    //!< horizontal mcu step
    uint32_t uStepVertM;    //!< vertical mcu step

    uint16_t uQuantNum;     //!< quant table to use

    HuffTableT *pACHuff;    //!< pointer to current AC huffman decode table
    HuffTableT *pDCHuff;    //!< pointer to current DC huffman decode table

    void (*pExpand)(DirtyJpgStateT *pState, struct CompTableT *pCompTable, uint32_t uOutOffset);
    void (*pInvert)(DirtyJpgStateT *pState, struct CompTableT *pCompTable, uint32_t uOutOffset);

    QuantTableT Matrix;     //!< working matrix for decoding this component
    QuantTableT Quant;      //!< current quant table

} CompTableT;

//! module state
struct DirtyJpgStateT
{
    // module memory group
    int32_t iMemGroup;          //!< module mem group id
    void    *pMemGroupUserData; //!< user data associated with mem group
    
    int32_t iLastError;         //!< previous error
    uint32_t uLastOffset;       //!< offset where decoding previously stopped

    uint16_t uVersion;          //!< JFIF file version
    uint16_t uAspectRatio;      //!< image aspect ratio
    uint16_t uAspectHorz;       //!< horizontal aspect ratio
    uint16_t uAspectVert;       //!< vertical aspect ratio

    uint16_t uImageWidth;       //!< width of encoded image
    uint16_t uImageHeight;      //!< height of encoded image

    uint16_t uMcuHorz;          //!< maxumum horizontal MCU, out of all components
    uint16_t uMcuHorzM;         //!< number of MCUs to cover image horizontall
    uint16_t uMcuVert;          //!< maximum vertical MCU, out of all components
    uint16_t uMcuVertM;         //!< number of MCUs to cover image vertically

    uint32_t uBitfield;         //!< current bit buffer
    uint32_t uBitsAvail;        //!< number of bits in bit buffer
    uint32_t uBitsConsumed;     //!< number of bits consumed from bitstream
    uint32_t uBytesRead;        //!< number of bytes read from bitstream

    CompTableT  CompTable[4];   //!< component tables
    QuantTableT QuantTable[16]; //!< quantization tables
    HuffTableT  HuffTable[8];   //!< huffman decode tables
    uint8_t     aMCU[MCU_MAXSIZE];  //!< mcu decode buffer

    const uint8_t *pCompData;   //!< pointer to compressed component data
    
    uint8_t *pImageBuf;         //!< pointer to output image buffer (allocated by caller)
    uint32_t uBufWidth;         //!< width of buffer to decode into
    uint32_t uBufHeight;        //!< height of buffer to decode into

    uint8_t bRestart;           //!< if TRUE, a restart marker has been processed
    uint8_t _pad[3];
};

/*** Variables ********************************************************************/

//! helper table to negate a value based on its bitsize
static const uint16_t _ZagAdj_adjust[] =
{
    0x0000, 0x0001, 0x0003, 0x0007,
    0x000f, 0x001f, 0x003f, 0x007f,
    0x00ff, 0x01ff, 0x03ff, 0x07ff,
    0x0fff, 0x1fff, 0x3fff, 0x7fff
};

//! jpeg zig-zag sequence table
static const uint16_t _ZagAdj_zag[] =
{
      0,  1,  8, 16,  9,  2,  3, 10,
     17, 24, 32, 25, 18, 11,  4,  5,
     12, 19, 26, 33, 40, 48, 41, 34,
     27, 20, 13,  6,  7, 14, 21, 28,
     35, 42, 49, 56, 57, 50, 43, 36,
     29, 22, 15, 23, 30, 37, 44, 51,
     58, 59, 52, 45, 38, 31, 39, 46,
     53, 60, 61, 54, 47, 55, 62, 63
};

//! table for mult=-0.714140, scale=16, mult=1.402000, scale=1:
static const uint32_t _Mult07_14[] =
{
    0x05b6ff4d, 0x05abff4e, 0x059fff50, 0x0594ff51, 0x0588ff53, 0x057dff54, 0x0572ff55, 0x0566ff57,
    0x055bff58, 0x054fff5a, 0x0544ff5b, 0x0538ff5c, 0x052dff5e, 0x0522ff5f, 0x0516ff61, 0x050bff62,
    0x04ffff63, 0x04f4ff65, 0x04e8ff66, 0x04ddff68, 0x04d2ff69, 0x04c6ff6a, 0x04bbff6c, 0x04afff6d,
    0x04a4ff6f, 0x0498ff70, 0x048dff71, 0x0482ff73, 0x0476ff74, 0x046bff76, 0x045fff77, 0x0454ff79,
    0x0448ff7a, 0x043dff7b, 0x0432ff7d, 0x0426ff7e, 0x041bff80, 0x040fff81, 0x0404ff82, 0x03f8ff84,
    0x03edff85, 0x03e2ff87, 0x03d6ff88, 0x03cbff89, 0x03bfff8b, 0x03b4ff8c, 0x03a8ff8e, 0x039dff8f,
    0x0392ff90, 0x0386ff92, 0x037bff93, 0x036fff95, 0x0364ff96, 0x0358ff97, 0x034dff99, 0x0342ff9a,
    0x0336ff9c, 0x032bff9d, 0x031fff9e, 0x0314ffa0, 0x0308ffa1, 0x02fdffa3, 0x02f2ffa4, 0x02e6ffa5,
    0x02dbffa7, 0x02cfffa8, 0x02c4ffaa, 0x02b9ffab, 0x02adffac, 0x02a2ffae, 0x0296ffaf, 0x028bffb1,
    0x027fffb2, 0x0274ffb3, 0x0269ffb5, 0x025dffb6, 0x0252ffb8, 0x0246ffb9, 0x023bffba, 0x022fffbc,
    0x0224ffbd, 0x0219ffbf, 0x020dffc0, 0x0202ffc1, 0x01f6ffc3, 0x01ebffc4, 0x01dfffc6, 0x01d4ffc7,
    0x01c9ffc8, 0x01bdffca, 0x01b2ffcb, 0x01a6ffcd, 0x019bffce, 0x018fffcf, 0x0184ffd1, 0x0179ffd2,
    0x016dffd4, 0x0162ffd5, 0x0156ffd6, 0x014bffd8, 0x013fffd9, 0x0134ffdb, 0x0129ffdc, 0x011dffdd,
    0x0112ffdf, 0x0106ffe0, 0x00fbffe2, 0x00efffe3, 0x00e4ffe4, 0x00d9ffe6, 0x00cdffe7, 0x00c2ffe9,
    0x00b6ffea, 0x00abffeb, 0x009fffed, 0x0094ffee, 0x0089fff0, 0x007dfff1, 0x0072fff2, 0x0066fff4,
    0x005bfff5, 0x004ffff7, 0x0044fff8, 0x0039fff9, 0x002dfffb, 0x0022fffc, 0x0016fffe, 0x000bffff,
    0x00000000, 0xfff50001, 0xffea0002, 0xffde0004, 0xffd30005, 0xffc70007, 0xffbc0008, 0xffb10009,
    0xffa5000b, 0xff9a000c, 0xff8e000e, 0xff83000f, 0xff770010, 0xff6c0012, 0xff610013, 0xff550015,
    0xff4a0016, 0xff3e0017, 0xff330019, 0xff27001a, 0xff1c001c, 0xff11001d, 0xff05001e, 0xfefa0020,
    0xfeee0021, 0xfee30023, 0xfed70024, 0xfecc0025, 0xfec10027, 0xfeb50028, 0xfeaa002a, 0xfe9e002b,
    0xfe93002c, 0xfe87002e, 0xfe7c002f, 0xfe710031, 0xfe650032, 0xfe5a0033, 0xfe4e0035, 0xfe430036,
    0xfe370038, 0xfe2c0039, 0xfe21003a, 0xfe15003c, 0xfe0a003d, 0xfdfe003f, 0xfdf30040, 0xfde70041,
    0xfddc0043, 0xfdd10044, 0xfdc50046, 0xfdba0047, 0xfdae0048, 0xfda3004a, 0xfd97004b, 0xfd8c004d,
    0xfd81004e, 0xfd75004f, 0xfd6a0051, 0xfd5e0052, 0xfd530054, 0xfd470055, 0xfd3c0056, 0xfd310058,
    0xfd250059, 0xfd1a005b, 0xfd0e005c, 0xfd03005d, 0xfcf8005f, 0xfcec0060, 0xfce10062, 0xfcd50063,
    0xfcca0064, 0xfcbe0066, 0xfcb30067, 0xfca80069, 0xfc9c006a, 0xfc91006b, 0xfc85006d, 0xfc7a006e,
    0xfc6e0070, 0xfc630071, 0xfc580072, 0xfc4c0074, 0xfc410075, 0xfc350077, 0xfc2a0078, 0xfc1e0079,
    0xfc13007b, 0xfc08007c, 0xfbfc007e, 0xfbf1007f, 0xfbe50080, 0xfbda0082, 0xfbce0083, 0xfbc30085,
    0xfbb80086, 0xfbac0087, 0xfba10089, 0xfb95008a, 0xfb8a008c, 0xfb7e008d, 0xfb73008f, 0xfb680090,
    0xfb5c0091, 0xfb510093, 0xfb450094, 0xfb3a0096, 0xfb2e0097, 0xfb230098, 0xfb18009a, 0xfb0c009b,
    0xfb01009d, 0xfaf5009e, 0xfaea009f, 0xfade00a1, 0xfad300a2, 0xfac800a4, 0xfabc00a5, 0xfab100a6,
    0xfaa500a8, 0xfa9a00a9, 0xfa8e00ab, 0xfa8300ac, 0xfa7800ad, 0xfa6c00af, 0xfa6100b0, 0xfa5500b2
};

//! table for mult=1.772000, scale=1, mult=-0.344140, scale=16:
static const uint32_t _Mult17_03[] =
{
    0xff1e02c0, 0xff1f02bb, 0xff2102b5, 0xff2302b0, 0xff2502aa, 0xff2702a5, 0xff28029f, 0xff2a029a,
    0xff2c0294, 0xff2e028f, 0xff2f0289, 0xff310284, 0xff33027e, 0xff350279, 0xff360273, 0xff38026e,
    0xff3a0268, 0xff3c0263, 0xff3e025d, 0xff3f0258, 0xff410252, 0xff43024d, 0xff450247, 0xff460242,
    0xff48023c, 0xff4a0237, 0xff4c0231, 0xff4e022c, 0xff4f0226, 0xff510221, 0xff53021b, 0xff550216,
    0xff560210, 0xff58020b, 0xff5a0205, 0xff5c0200, 0xff5d01fa, 0xff5f01f5, 0xff6101ef, 0xff6301ea,
    0xff6501e4, 0xff6601df, 0xff6801d9, 0xff6a01d4, 0xff6c01ce, 0xff6d01c9, 0xff6f01c3, 0xff7101be,
    0xff7301b8, 0xff7501b2, 0xff7601ad, 0xff7801a7, 0xff7a01a2, 0xff7c019c, 0xff7d0197, 0xff7f0191,
    0xff81018c, 0xff830186, 0xff840181, 0xff86017b, 0xff880176, 0xff8a0170, 0xff8c016b, 0xff8d0165,
    0xff8f0160, 0xff91015a, 0xff930155, 0xff94014f, 0xff96014a, 0xff980144, 0xff9a013f, 0xff9b0139,
    0xff9d0134, 0xff9f012e, 0xffa10129, 0xffa30123, 0xffa4011e, 0xffa60118, 0xffa80113, 0xffaa010d,
    0xffab0108, 0xffad0102, 0xffaf00fd, 0xffb100f7, 0xffb300f2, 0xffb400ec, 0xffb600e7, 0xffb800e1,
    0xffba00dc, 0xffbb00d6, 0xffbd00d1, 0xffbf00cb, 0xffc100c6, 0xffc200c0, 0xffc400bb, 0xffc600b5,
    0xffc800b0, 0xffca00aa, 0xffcb00a5, 0xffcd009f, 0xffcf009a, 0xffd10094, 0xffd2008f, 0xffd40089,
    0xffd60084, 0xffd8007e, 0xffda0079, 0xffdb0073, 0xffdd006e, 0xffdf0068, 0xffe10063, 0xffe2005d,
    0xffe40058, 0xffe60052, 0xffe8004d, 0xffe90047, 0xffeb0042, 0xffed003c, 0xffef0037, 0xfff10031,
    0xfff2002c, 0xfff40026, 0xfff60021, 0xfff8001b, 0xfff90016, 0xfffb0010, 0xfffd000b, 0xffff0005,
    0x00000000, 0x0001fffb, 0x0003fff5, 0x0005fff0, 0x0007ffea, 0x0008ffe5, 0x000affdf, 0x000cffda,
    0x000effd4, 0x000fffcf, 0x0011ffc9, 0x0013ffc4, 0x0015ffbe, 0x0017ffb9, 0x0018ffb3, 0x001affae,
    0x001cffa8, 0x001effa3, 0x001fff9d, 0x0021ff98, 0x0023ff92, 0x0025ff8d, 0x0026ff87, 0x0028ff82,
    0x002aff7c, 0x002cff77, 0x002eff71, 0x002fff6c, 0x0031ff66, 0x0033ff61, 0x0035ff5b, 0x0036ff56,
    0x0038ff50, 0x003aff4b, 0x003cff45, 0x003eff40, 0x003fff3a, 0x0041ff35, 0x0043ff2f, 0x0045ff2a,
    0x0046ff24, 0x0048ff1f, 0x004aff19, 0x004cff14, 0x004dff0e, 0x004fff09, 0x0051ff03, 0x0053fefe,
    0x0055fef8, 0x0056fef3, 0x0058feed, 0x005afee8, 0x005cfee2, 0x005dfedd, 0x005ffed7, 0x0061fed2,
    0x0063fecc, 0x0065fec7, 0x0066fec1, 0x0068febc, 0x006afeb6, 0x006cfeb1, 0x006dfeab, 0x006ffea6,
    0x0071fea0, 0x0073fe9b, 0x0074fe95, 0x0076fe90, 0x0078fe8a, 0x007afe85, 0x007cfe7f, 0x007dfe7a,
    0x007ffe74, 0x0081fe6f, 0x0083fe69, 0x0084fe64, 0x0086fe5e, 0x0088fe59, 0x008afe53, 0x008bfe4e,
    0x008dfe48, 0x008ffe42, 0x0091fe3d, 0x0093fe37, 0x0094fe32, 0x0096fe2c, 0x0098fe27, 0x009afe21,
    0x009bfe1c, 0x009dfe16, 0x009ffe11, 0x00a1fe0b, 0x00a3fe06, 0x00a4fe00, 0x00a6fdfb, 0x00a8fdf5,
    0x00aafdf0, 0x00abfdea, 0x00adfde5, 0x00affddf, 0x00b1fdda, 0x00b2fdd4, 0x00b4fdcf, 0x00b6fdc9,
    0x00b8fdc4, 0x00bafdbe, 0x00bbfdb9, 0x00bdfdb3, 0x00bffdae, 0x00c1fda8, 0x00c2fda3, 0x00c4fd9d,
    0x00c6fd98, 0x00c8fd92, 0x00cafd8d, 0x00cbfd87, 0x00cdfd82, 0x00cffd7c, 0x00d1fd77, 0x00d2fd71,
    0x00d4fd6c, 0x00d6fd66, 0x00d8fd61, 0x00d9fd5b, 0x00dbfd56, 0x00ddfd50, 0x00dffd4b, 0x00e1fd45
};

//! 8-bit table for clamping range [-256...511]->[0...255]
static const uint8_t _Ranger8[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,

    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

//! offset clamp table to zero
static const uint8_t *_pRanger8 = _Ranger8+256;


/*** Private Functions ************************************************************/

#if DIRTYJPG_DEBUG_IDCT
/*F********************************************************************************/
/*!
    \Function _PrintMatrix16

    \Description
        Print 16bit 8x8 matrix to debug output.

    \Input *pMatrix - pointer to matrix to print
    \Input *pTitle  - output title

    \Version 03/09/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _PrintMatrix16(int16_t *pMatrix, const char *pTitle)
{
    int32_t ctr;

    NetPrintf(("%s\n", pTitle));
    for (ctr = 0; ctr < 64; ctr++)
    {
        char strBuffer[32];
        sprintf(strBuffer, "%+10d ", pMatrix[ctr]);
        if ((ctr !=0 ) && (((ctr+1) % 8) == 0))
        {
            strcat(strBuffer, "\n");
        }
        NetPrintf(("%s", strBuffer));
    }
}
#endif

/*

    Huffman decoding routines

*/

/*F********************************************************************************/
/*!
    \Function _ReadByte
    
    \Description
        Read a byte into the bitbuffer, handling any dle bytes.

    \Input *pState  - state ref
    \Input *pData   - compressed data base pointer
    \Input *pValue  - byte read 

    \Output
        uint32_t    - number of bytes consumed

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _ReadByte(DirtyJpgStateT *pState, const uint8_t *pData, uint8_t *pValue)
{
    uint32_t uValue, uOffset=0;

    *pValue = uValue = pData[uOffset++];
    while (uValue == 0xff)
    {
        // eat marker data
        uValue = pData[uOffset++];
        pState->uBitsConsumed += 8;

        // restart marker?
        if ((uValue >= 0xd0) && (uValue <= 0xd7))
        {
            #if DIRTYJPG_DEBUG_HUFF
            NetPrintf(("dirtyjpg: restart marker %d\n", uValue&0xf));
            #endif
            // remember that there was a restart marker
            pState->bRestart = TRUE;
        }
    }
    
    return(uOffset);
}

/*F********************************************************************************/
/*!
    \Function _ExtractBits
    
    \Description
        Read bits from bitstream.

    \Input *pState  - state ref
    \Input *pData   - compressed data base pointer
    \Input uBitSize - number of bits to read
    \Input *pSign   - positive if leading bit of extracted data is not set, else negative
    \Input bAdvance - if TRUE, advance the bit offset

    \Output
        uint32_t    - extracted value

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _ExtractBits(DirtyJpgStateT *pState, const uint8_t *pData, uint32_t uBitSize, int32_t *pSign, uint8_t bAdvance)
{
    uint32_t uValue, uBytesRead;
    uint8_t uByte;

    // load data into bit buffer
    while ((pState->uBitsAvail < 16) && (pState->bRestart != TRUE))
    {
        uBytesRead = _ReadByte(pState, pData+pState->uBytesRead, &uByte);
        pState->uBytesRead += uBytesRead;
        pState->uBitfield |= uByte << (24-pState->uBitsAvail);
        pState->uBitsAvail += 8;
    }

    // determine sign of extracted value
    if (pSign)
    {
        *pSign = (pState->uBitfield & 0x80000000) ? -1 : 1;
    }
    
    // take top bits
    uValue = pState->uBitfield >> (32-uBitSize);
    if (bAdvance)
    {
        pState->uBitfield <<= uBitSize;
        pState->uBitsAvail -= uBitSize;
        pState->uBitsConsumed += uBitSize;
    }

    // return extracted value to caller
    return(uValue);    
}

/*F********************************************************************************/
/*!
    \Function _HuffDecode
    
    \Description
        Decode a huffman-encoded value

    \Input *pState      - state ref
    \Input *pHuffTable  - pointer to huffman table to use for decode
    \Input *pData       - compressed data base pointer

    \Output
        uint32_t        - decoded value

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _HuffDecode(DirtyJpgStateT *pState, HuffTableT *pHuffTable, const uint8_t *pData)
{
    HuffHeadT *pHuffHead;
    uint32_t uValue;
    
    // extract 16 bits, but don't advance bit offset
    uValue = _ExtractBits(pState, pData, 16, NULL, FALSE);

    // find table range for this value
    for (pHuffHead = pHuffTable->Head; uValue >= pHuffHead->uMaxCode; pHuffHead += 1)
    {
        if (pHuffHead->uShift == 0)
        {
             NetPrintf(("dirtyjpg: decode error\n"));
            return(0xffffffff);
        }
    }
    #if DIRTYJPG_DECODE_HUFF
    NetPrintf(("dirtyjpg: using range %d\n", pHuffHead-pHuffTable->Head));
    #endif
    
    // subtract mincode to get offset into table
    uValue -= pHuffHead->uMinCode;
    // shift down to discard extra bits
    uValue >>= pHuffHead->uShift;
    // offset by the start of this table into code/step buffer
    uValue += pHuffHead->uOffset;

    // lookup skip value and consume bits
    _ExtractBits(pState, pData, pHuffTable->Step[uValue], NULL, TRUE);

    #if DIRTYJPG_DEBUG_HUFF
    NetPrintf(("_decode: 0x%02x->s%d,", uValue, pHuffTable->Step[uValue]));
    #endif

    // lookup code value
    uValue = pHuffTable->Code[uValue];

    #if DIRTYJPG_DEBUG_HUFF
    NetPrintf(("c%02x\n", uValue));
    #endif

    // return decoded value to caller
    return(uValue);
}

/*F********************************************************************************/
/*!
    \Function _UnpackComp
    
    \Description
        Unnpack the next sample into a component record's matrix

    \Input *pState      - state ref
    \Input *pCompTable  - pointer to component table
    \Input *pData       - compressed data base pointer

    \Output
        int32_t         - negative=error, else success

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _UnpackComp(DirtyJpgStateT *pState, CompTableT *pCompTable, const uint8_t *pData)
{
    uint16_t *pMatrix = (uint16_t *)&pCompTable->Matrix;
    uint16_t *pQuant = (uint16_t *)&pCompTable->Quant;
    uint32_t uCodeLen, uElemIdx, uValue=0;
    int32_t iSign = 0;

    // decode a token (get code length)
    if ((uCodeLen = _HuffDecode(pState, pCompTable->pDCHuff, pData)) == 0xffffffff)
    {
        return(uCodeLen);
    }
    else if (uCodeLen != 0)
    {
        // get the raw value
        uValue = _ExtractBits(pState, pData, uCodeLen, &iSign, TRUE);
        // if non-negative, adjust to negative value
        if (iSign > 0)
        {
            uValue -= _ZagAdj_adjust[uCodeLen];
        }
        #if DIRTYJPG_DEBUG_HUFF
        NetPrintf(("dc=0x%02x\n", uValue));
        #endif
        // quantify the difference
        uValue *= pQuant[0];
    }

    // accumulate and save the dc coefficient
    uValue += pMatrix[0];
    pMatrix[0] = uValue;

    // clear the rest of the matrix
    memset(&pMatrix[1], 0, sizeof(pCompTable->Matrix)-sizeof(pMatrix[0]));

    // decode ac coefficients
    for (uElemIdx = 1; uElemIdx < 64; )
    {
        // decode ac code
        if ((uValue = _HuffDecode(pState, pCompTable->pACHuff, pData)) == 0xffffffff)
        {
            return(uValue);
        }
    
        // get vli length
        if ((uCodeLen = uValue & 0xf) == 0)
        {
            // end of block?
            if (uValue != 0xf0)
            {
                return(0);
            }
            // skip 16 (zero skip)
            uElemIdx += 16;
        }
        else
        {
            // skip count
            uValue >>= 4;
            // advance matrix offset (zero skip)
            uElemIdx = (uElemIdx+uValue)&63;
            // get the raw value and advance the bit offset
            uValue = _ExtractBits(pState, pData, uCodeLen, &iSign, TRUE);
            // if non-negative, adjust to negative value
            if (iSign > 0)
            {
                uValue -= _ZagAdj_adjust[uCodeLen];
            }
            #if DIRTYJPG_DEBUG_HUFF
            NetPrintf(("ac=0x%02x\n", uValue));
            #endif
            // quantify the value
            uValue *= pQuant[uElemIdx];
            // store the quantized value, with zag
            pMatrix[_ZagAdj_zag[uElemIdx]] = uValue;
            // increment index
            uElemIdx += 1;
        }
    }

    // return success
    return(0);
}


/*

    Image coefficient expansion routines

*/

/*F********************************************************************************/
/*!
    \Function _Expand0

    \Description
        Expand buffered component scan line data (no expansion).

    \Input *pState      - state ref
    \Input *pCompTable  - pointer to component table
    \Input uOutOffset   - offset in mcu buffer

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _Expand0(DirtyJpgStateT *pState, CompTableT *pCompTable, uint32_t uOutOffset)
{
}

/*F********************************************************************************/
/*!
    \Function _Expand3hv

    \Description
        Expand buffered component scan line data (2x2 special case).

    \Input *pState      - state ref
    \Input *pCompTable  - pointer to component table
    \Input uOutOffset   - offset in mcu buffer

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _Expand3hv(DirtyJpgStateT *pState, CompTableT *pCompTable, uint32_t uOutOffset)
{
    uint32_t uStepV0 = pCompTable->uStepVert0;
    uint32_t uStepVM = pCompTable->uStepVertM;
    uint32_t uStepH0 = pCompTable->uStepHorz0;
    uint8_t *pData = pState->aMCU + uOutOffset;
    uint8_t *pEndRow, *pEndCol;
    uint32_t uData;

    for (pEndRow = pData + uStepVM; pData != pEndRow;)
    {    
        for (pEndCol = pData + uStepV0; pData != pEndCol;)
        {
            uData = *pData;             // get upper left
            pData[uStepV0] = uData;     // save lower-left
            pData += uStepH0;
            pData[0] = uData;           // save upper-right
            pData[uStepV0] = uData;     // save lower-right
            pData += uStepH0;           
        }
        
        // skip extra col
        pData += uStepV0;
    }
}

/*F********************************************************************************/
/*!
    \Function _ExpandAny

    \Description
        Expand buffered component scan line data (general case).

    \Input *pState      - state ref
    \Input *pCompTable  - pointer to component table
    \Input uOutOffset   - offset in mcu buffer

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _ExpandAny(DirtyJpgStateT *pState, CompTableT *pCompTable, uint32_t uOutOffset)
{
    uint32_t uData, uExpHorz, uExpVert, uRptCnt;
    uint32_t uStepV0 = pCompTable->uStepVert0;
    uint32_t uStepVM = pCompTable->uStepVertM;
    uint32_t uStepH0 = pCompTable->uStepHorz0;
    uint8_t *pData, *pEndRow, *pEndCol;

    // get zero-relative vertical expansion factor    
    if ((uExpVert = pCompTable->uExpVert - 1) != 0)
    {
        pData = pState->aMCU + uOutOffset;
        for (pEndCol = pData + uStepV0; pData < pEndCol; pData += pCompTable->uStepHorz1)
        {    
            uint8_t *pTemp = pData;
            for (pEndRow = pData + uStepVM; pData < pEndRow; )
            {
                // get source data and index past it
                uData = *pData;
                pData += uStepV0;
                
                // expand the data
                for (uRptCnt = 0; uRptCnt < uExpVert; uRptCnt += 1, pData += uStepV0)
                {
                    *pData = uData;
                }
            }
            pData = pTemp;
        }
    }
    
    // get zero-relative horizontal expansion factor    
    if ((uExpHorz = pCompTable->uExpHorz - 1) != 0)
    {
        pData = pState->aMCU + uOutOffset;
        for (pEndRow = pData + uStepVM; pData < pEndRow; )
        {
            // get source data and index past it
            uData = *pData;
            pData += uStepH0;
        
            // expand the data
            for (uRptCnt = 0; uRptCnt < uExpHorz; uRptCnt += 1, pData += uStepH0)
            {
                *pData = uData;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _TransLLM

    \Description
        Perform iDCT and quantify (component record matrix -> mcu buffer).

    \Input *pState      - state ref
    \Input *pCompTable  - pointer to component table
    \Input uOutOffset   - offset in mcu buffer

    \Version 03/03/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _TransLLM(DirtyJpgStateT *pState, CompTableT *pCompTable, uint32_t uOutOffset)
{
    uint32_t uStepH1, uStepV1, uRow;
    uint8_t *pDataDest;
    QuantTableT WorkingMatrix;
    #if DIRTYJPG_DEBUG_IDCT
    QuantTable2 WorkingMatrix2;
    #endif
    int16_t *pWorking, *pMatrix;
    const uint8_t *pRanger8 = _pRanger8 + 128;
    int32_t tmp0, tmp1, tmp2, tmp3;
    int32_t tmp10, tmp11, tmp12, tmp13;
    int32_t z1, z2, z3, z4, z5;

    // point to dst data
    if ((pDataDest = pState->aMCU) == NULL)
    {
        return;
    }
    
    // increment to offset
    pDataDest += uOutOffset;
    
    // ref step values
    uStepH1 = pCompTable->uStepHorz1;
    uStepV1 = pCompTable->uStepVert1;

    // debug input
    _PrintMatrix16((int16_t *)&pCompTable->Matrix, "iDCT input matrix");

    // do eight columns
    for (uRow = 0, pWorking=(int16_t*)&WorkingMatrix, pMatrix=(int16_t*)&pCompTable->Matrix; uRow < 8; uRow++)
    {
        /* Due to quantization, we will usually find that many of the input
           coefficients are zero, especially the AC terms.  We can exploit this
           by short-circuiting the IDCT calculation for any column in which all
           the AC terms are zero.  In that case each output is equal to the
           DC coefficient (with scale factor as needed).
           With typical images and quantization tables, half or more of the
           column DCT calculations can be simplified this way. */
        if ((pMatrix[DCTSIZE*1] | pMatrix[DCTSIZE*2] | pMatrix[DCTSIZE*3] |
            pMatrix[DCTSIZE*4] | pMatrix[DCTSIZE*5] | pMatrix[DCTSIZE*6] |
            pMatrix[DCTSIZE*7]) == 0)
        {
            /* AC terms all zero */
            int16_t dcval = (int16_t) (pMatrix[DCTSIZE*0] << PASS1_BITS);
    
            pWorking[DCTSIZE*0] = dcval;
            pWorking[DCTSIZE*1] = dcval;
            pWorking[DCTSIZE*2] = dcval;
            pWorking[DCTSIZE*3] = dcval;
            pWorking[DCTSIZE*4] = dcval;
            pWorking[DCTSIZE*5] = dcval;
            pWorking[DCTSIZE*6] = dcval;
            pWorking[DCTSIZE*7] = dcval;
    
            pMatrix++;			// advance pointers to next column
            pWorking++;
            continue;
        }

        // Even part: reverse the even part of the forward DCT. The rotator is sqrt(2)*c(-6).
        z2 = pMatrix[DCTSIZE*2];
        z3 = pMatrix[DCTSIZE*6];

        z1 = (z2 + z3) * FIX_0_541196100;
        tmp2 = z1 + (z3 * -FIX_1_847759065);
        tmp3 = z1 + (z2 * FIX_0_765366865);

        z2 = pMatrix[DCTSIZE*0];
        z3 = pMatrix[DCTSIZE*4];

        tmp0 = (z2 + z3) << CONST_BITS;
        tmp1 = (z2 - z3) << CONST_BITS;

        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        /* Odd part per figure 8; the matrix is unitary and hence its
           transpose is its inverse.  i0..i3 are y7,y5,y3,y1 respectively. */
        tmp0 = pMatrix[DCTSIZE*7];
        tmp1 = pMatrix[DCTSIZE*5];
        tmp2 = pMatrix[DCTSIZE*3];
        tmp3 = pMatrix[DCTSIZE*1];

        z1 = tmp0 + tmp3;
        z2 = tmp1 + tmp2;
        z3 = tmp0 + tmp2;
        z4 = tmp1 + tmp3;
        z5 = (z3 + z4) * FIX_1_175875602; // sqrt(2) * c3

        tmp0 = tmp0 * FIX_0_298631336; // sqrt(2) * (-c1+c3+c5-c7)
        tmp1 = tmp1 * FIX_2_053119869; // sqrt(2) * ( c1+c3-c5+c7)
        tmp2 = tmp2 * FIX_3_072711026; // sqrt(2) * ( c1+c3+c5-c7)
        tmp3 = tmp3 * FIX_1_501321110; // sqrt(2) * ( c1+c3-c5-c7)
        z1 = z1 * -FIX_0_899976223; // sqrt(2) * (c7-c3) 
        z2 = z2 * -FIX_2_562915447; // sqrt(2) * (-c1-c3)
        z3 = z3 * -FIX_1_961570560; // sqrt(2) * (-c3-c5)
        z4 = z4 * -FIX_0_390180644; // sqrt(2) * (c5-c3) 

        z3 += z5;
        z4 += z5;

        tmp0 += z1 + z3;
        tmp1 += z2 + z4;
        tmp2 += z2 + z3;
        tmp3 += z1 + z4;

        // Final output stage: inputs are tmp10..tmp13, tmp0..tmp3
        pWorking[DCTSIZE*0] = (int16_t) ((tmp10 + tmp3) >> (CONST_BITS-PASS1_BITS));
        pWorking[DCTSIZE*7] = (int16_t) ((tmp10 - tmp3) >> (CONST_BITS-PASS1_BITS));
        pWorking[DCTSIZE*1] = (int16_t) ((tmp11 + tmp2) >> (CONST_BITS-PASS1_BITS));
        pWorking[DCTSIZE*6] = (int16_t) ((tmp11 - tmp2) >> (CONST_BITS-PASS1_BITS));
        pWorking[DCTSIZE*2] = (int16_t) ((tmp12 + tmp1) >> (CONST_BITS-PASS1_BITS));
        pWorking[DCTSIZE*5] = (int16_t) ((tmp12 - tmp1) >> (CONST_BITS-PASS1_BITS));
        pWorking[DCTSIZE*3] = (int16_t) ((tmp13 + tmp0) >> (CONST_BITS-PASS1_BITS));
        pWorking[DCTSIZE*4] = (int16_t) ((tmp13 - tmp0) >> (CONST_BITS-PASS1_BITS));

        pMatrix++;      // advance pointers to next column
        pWorking++;
    }

    // debug first pass
    _PrintMatrix16((int16_t *)&WorkingMatrix, "iDCT pass one");

    /* Pass 2: process rows from work array, store into output array.
       Note that we must descale the results by a factor of 8 == 2**3,
       and also undo the PASS1_BITS scaling. */
    for (uRow = 0, pWorking=(int16_t*)&WorkingMatrix; uRow < DCTSIZE; uRow++, pDataDest += uStepV1)
    {
        // Even part: reverse the even part of the forward DCT. The rotator is sqrt(2)*c(-6).
        z2 = (int32_t) pWorking[2];
        z3 = (int32_t) pWorking[6];

        z1 = (z2 + z3) * FIX_0_541196100;
        tmp2 = z1 + (z3 * -FIX_1_847759065);
        tmp3 = z1 + (z2 * FIX_0_765366865);

        tmp0 = ((int32_t) pWorking[0] + (int32_t) pWorking[4]) << CONST_BITS;
        tmp1 = ((int32_t) pWorking[0] - (int32_t) pWorking[4]) << CONST_BITS;

        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        /* Odd part per figure 8; the matrix is unitary and hence its
           transpose is its inverse.  i0..i3 are y7,y5,y3,y1 respectively. */
        tmp0 = (int32_t) pWorking[7];
        tmp1 = (int32_t) pWorking[5];
        tmp2 = (int32_t) pWorking[3];
        tmp3 = (int32_t) pWorking[1];

        z1 = tmp0 + tmp3;
        z2 = tmp1 + tmp2;
        z3 = tmp0 + tmp2;
        z4 = tmp1 + tmp3;
        z5 = (z3 + z4) * FIX_1_175875602; // sqrt(2) * c3

        tmp0 = tmp0 * FIX_0_298631336; // sqrt(2) * (-c1+c3+c5-c7)
        tmp1 = tmp1 * FIX_2_053119869; // sqrt(2) * ( c1+c3-c5+c7)
        tmp2 = tmp2 * FIX_3_072711026; // sqrt(2) * ( c1+c3+c5-c7)
        tmp3 = tmp3 * FIX_1_501321110; // sqrt(2) * ( c1+c3-c5-c7)
        z1 = z1 * -FIX_0_899976223; // sqrt(2) * (c7-c3)
        z2 = z2 * -FIX_2_562915447; // sqrt(2) * (-c1-c3)
        z3 = z3 * -FIX_1_961570560; // sqrt(2) * (-c3-c5)
        z4 = z4 * -FIX_0_390180644; // sqrt(2) * (c5-c3)

        z3 += z5;
        z4 += z5;

        tmp0 += z1 + z3;
        tmp1 += z2 + z4;
        tmp2 += z2 + z3;
        tmp3 += z1 + z4;

        #if DIRTYJPG_DEBUG_IDCT
        WorkingMatrix2[0+(uRow*8)] = (((tmp10 + tmp3) >> (CONST_BITS+PASS1_BITS+3)));
        WorkingMatrix2[7+(uRow*8)] = (((tmp10 - tmp3) >> (CONST_BITS+PASS1_BITS+3)));
        WorkingMatrix2[1+(uRow*8)] = (((tmp11 + tmp2) >> (CONST_BITS+PASS1_BITS+3)));
        WorkingMatrix2[6+(uRow*8)] = (((tmp11 - tmp2) >> (CONST_BITS+PASS1_BITS+3)));
        WorkingMatrix2[2+(uRow*8)] = (((tmp12 + tmp1) >> (CONST_BITS+PASS1_BITS+3)));
        WorkingMatrix2[5+(uRow*8)] = (((tmp12 - tmp1) >> (CONST_BITS+PASS1_BITS+3)));
        WorkingMatrix2[3+(uRow*8)] = (((tmp13 + tmp0) >> (CONST_BITS+PASS1_BITS+3)));
        WorkingMatrix2[4+(uRow*8)] = (((tmp13 - tmp0) >> (CONST_BITS+PASS1_BITS+3)));
        #endif

        pDataDest[0*uStepH1] = pRanger8[(((tmp10 + tmp3) >> (CONST_BITS+PASS1_BITS+3)))];
        pDataDest[7*uStepH1] = pRanger8[(((tmp10 - tmp3) >> (CONST_BITS+PASS1_BITS+3)))];
        pDataDest[1*uStepH1] = pRanger8[(((tmp11 + tmp2) >> (CONST_BITS+PASS1_BITS+3)))];
        pDataDest[6*uStepH1] = pRanger8[(((tmp11 - tmp2) >> (CONST_BITS+PASS1_BITS+3)))];
        pDataDest[2*uStepH1] = pRanger8[(((tmp12 + tmp1) >> (CONST_BITS+PASS1_BITS+3)))];
        pDataDest[5*uStepH1] = pRanger8[(((tmp12 - tmp1) >> (CONST_BITS+PASS1_BITS+3)))];
        pDataDest[3*uStepH1] = pRanger8[(((tmp13 + tmp0) >> (CONST_BITS+PASS1_BITS+3)))];
        pDataDest[4*uStepH1] = pRanger8[(((tmp13 - tmp0) >> (CONST_BITS+PASS1_BITS+3)))];

        pWorking += DCTSIZE;		// advance pointer to next row
    }

    // debug second pass
    _PrintMatrix16((int16_t *)&WorkingMatrix2, "iDCT pass two");
}

/*F********************************************************************************/
/*!
    \Function _InitComp

    \Description
        Init component table.

    \Input *pState      - state ref
    \Input *pFrame      - pointer to SOS (start of scan) frame

    \Version 03/01/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _InitComp(DirtyJpgStateT *pState, const uint8_t *pFrame)
{
    CompTableT *pCompTable;
    QuantTableT *pQuantTable;
    uint32_t uExpFunc;
    
    // ref comp table
    pCompTable = &pState->CompTable[pFrame[0]];
    
    // ref huffman tables
    pCompTable->pDCHuff = &pState->HuffTable[0+(pFrame[1]>>4)];
    pCompTable->pACHuff = &pState->HuffTable[4+(pFrame[1]&3)];
    #if DIRTYJPG_DEBUG_HUFF
    NetPrintf(("dirtyjpg: using dc=%d ac=%d\n", pFrame[1]>>4, pFrame[1]&3));
    #endif
    
    // reset differential encoding
    pCompTable->Matrix[0] = 0;
    
    // determine expansion function
    uExpFunc = ((pCompTable->uExpVert - 1) << 2) + (pCompTable->uExpHorz - 1);

    // ref expansion table
    if (uExpFunc == 0)
    {
        pCompTable->pExpand = _Expand0;
    }
    else if (uExpFunc == 0x0101)
    {
        pCompTable->pExpand = _Expand3hv;
    }
    else
    {
        pCompTable->pExpand = _ExpandAny;
    }
    
    // get quantify table pointer
    pQuantTable = &pState->QuantTable[pCompTable->uQuantNum];
    
    // copy the quantify table
    memcpy(pCompTable->Quant, pQuantTable, sizeof(pCompTable->Quant));

    // ref inverse function    
    pCompTable->pInvert = _TransLLM;
}

/*F********************************************************************************/
/*!
    \Function _PutColor

    \Description
        Convert an MCU from YCbCr to ARGB ($$tmp assume color)

    \Input *pState      - state ref
    \Input *pCompTable  - pointer to component table
    \Input uDstHOff     - horizontal mcu offset
    \Input uDstVOff     - vertical mcu offset

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _PutColor(DirtyJpgStateT *pState, CompTableT *pCompTable, uint32_t uDstHOff, uint32_t uDstVOff)
{
    uint32_t uSrcH, uSrcV, uStepSrcH, uStepSrcV, uMaxSrcH, uMaxSrcV;
    uint32_t uDstH, uDstV, uStepDstH, uStepDstV, uMaxDstH, uMaxDstV;
    const uint8_t *pDataSrc;
    uint8_t *pDataDst;
    int16_t Y, Cb_hi, Cb_lo, Cr_hi, Cr_lo;
    uint32_t R, G, B;
    
    // ref source step values
    uStepSrcH = pCompTable->uStepHorz0;
    uMaxSrcH = pCompTable->uStepHorzM;
    uStepSrcV = pCompTable->uStepVert0;
    uMaxSrcV = pCompTable->uStepVertM;

    // ref dest step values
    uStepDstH = 4; // ARGB
    uMaxDstH = uStepDstH * pState->uBufWidth;
    uStepDstV = uStepDstH * pState->uBufWidth;
    uMaxDstV = uStepDstV * pState->uBufHeight;

    // scale mcu step values to get pixel offsets
    uDstHOff *= 8*4*pState->uMcuHorz;
    uDstVOff *= uMaxDstH * 8 * pState->uMcuVert;

    // transform the data
    for (uSrcV = 0, uDstV = uDstVOff; (uSrcV < uMaxSrcV) && (uDstV < uMaxDstV); uSrcV += uStepSrcV, uDstV += uStepDstV)
    {
        for (uSrcH = 0, uDstH = uDstHOff; (uSrcH < uMaxSrcH) && (uDstH < uMaxDstH); uSrcH += uStepSrcH, uDstH += uStepDstH)
        {
            pDataSrc = pState->aMCU + uSrcH + uSrcV;
            pDataDst = pState->pImageBuf + uDstH + uDstV;
            
            // get YCbCr components
            Y = pDataSrc[0];
            Cb_hi = _Mult17_03[pDataSrc[1]]>>16;    // get 1.772(Cb-128)
            Cb_lo = _Mult17_03[pDataSrc[1]]&0xffff; // get -0.34414(Cb-128) 
            Cr_hi = _Mult07_14[pDataSrc[2]]>>16;    // get -0.71414(Cr-128)
            Cr_lo = _Mult07_14[pDataSrc[2]]&0xffff; // get 1.402(Cr-128)

            // convert to RGB    
#ifdef __SNC__
            R = *((uint8_t *)(_pRanger8 + Y + Cr_lo));                  // Y + 1.402(Cr-128)
            G = *((uint8_t *)(_pRanger8 + Y + ((Cr_hi + Cb_lo) >> 4))); // Y - 0.71414(Cr-128) - 0.34414(Cb-128)
            B = *((uint8_t *)(_pRanger8 + Y + Cb_hi));                  // Y + 1.772(Cb-128)
#else
            R = _pRanger8[Y + Cr_lo];                  // Y + 1.402(Cr-128)
            G = _pRanger8[Y + ((Cr_hi + Cb_lo) >> 4)]; // Y - 0.71414(Cr-128) - 0.34414(Cb-128)
            B = _pRanger8[Y + Cb_hi];                  // Y + 1.772(Cb-128)
#endif
            // store in output buffer
            pDataDst[0] = 0xff;
            pDataDst[1] = R;
            pDataDst[2] = G;
            pDataDst[3] = B;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _DoRST

    \Description
        Restart decoder after an RST marker was encountered.

    \Input *pState      - state ref

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _DoRST(DirtyJpgStateT *pState)
{
    uint32_t uComp;
    
    // reset bitbuffer
    pState->uBitsConsumed += pState->uBitsAvail;
    pState->uBitsAvail = 0;
    pState->uBitfield = 0;
    
    // reset dc for all components
    for (uComp = 0; uComp < sizeof(pState->CompTable)/sizeof(pState->CompTable[0]); uComp++)
    {
        pState->CompTable[uComp].Matrix[0] = 0;
    }
    
    // done the restart, clear the flag
    pState->bRestart = FALSE;
}

/*F********************************************************************************/
/*!
    \Function _GetMCU

    \Description
        Decode an MCU into MCU buffer.

    \Input *pState      - state ref
    \Input *pFrame      - pointer to SOS (start of scan) component frame data
    \Input uCompCount   - number of components (must be 1 or 3)

    \Output
        uint32_t        - number of bits consumed from the bitstream, or -1

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _GetMCU(DirtyJpgStateT *pState, const uint8_t *pFrame, uint32_t uCompCount)
{
    uint32_t uCompIdx, uCompVert, uCompHorz, uOutOffset;
    CompTableT *pCompTable;

    // unpack and translate all components
    for (uCompIdx = 0; uCompIdx < uCompCount; uCompIdx += 1)
    {
        // ref comp table
        pCompTable = &pState->CompTable[pFrame[uCompIdx*2]];

        // unpack and translate component
        for (uOutOffset = uCompIdx, uCompVert = 0; uCompVert < pCompTable->uCompVert; uCompVert += 1)
        {
            // save start of row
            uint32_t uTmpOffset = uOutOffset;

            // unpack and translate row            
            for (uCompHorz = 0; uCompHorz < pCompTable->uCompHorz; uCompHorz += 1)
            {
                // unpack a component into record matrix
                if (_UnpackComp(pState, pCompTable, pState->pCompData) == 0xffffffff)
                {
                    return(0xffffffff);
                }
    
                // translate the component into mcu buffer
                pCompTable->pInvert(pState, pCompTable, uOutOffset);
            
                // increment to next component
                uOutOffset += pCompTable->uStepHorz8;
            }
        
            // increment to next output row
            uOutOffset = uTmpOffset + pCompTable->uStepVert8;
        }
        
        // expand component
        pCompTable->pExpand(pState, pCompTable, uCompIdx);
    }
    
    // if there was a restart, do it now
    if (pState->bRestart)
    {
        _DoRST(pState);
    }

    // return updated bit offset
    return(pState->uBitsConsumed);
}

/*F********************************************************************************/
/*!
    \Function _ParseDQT

    \Description
        Parse a DQT (quantization) table.

    \Input *pState  - state ref
    \Input *pFrame  - pointer to DQT frame data
    \Input *pFinal  - end of DQT frame

    \Output
        int32_t     - DIRTYJPG_ERR_*

    \Version 02/23/2006 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _ParseDQT(DirtyJpgStateT *pState, const uint8_t *pFrame, const uint8_t *pFinal)
{
    uint32_t uCount;
    uint32_t uIndex;
    uint32_t uMask = 0;
    uint32_t uShift = 0;
    uint32_t uSkip = 1;

    // process tables
    while (pFrame < pFinal)
    {
        // decode index
        uIndex = pFrame[0] & 15;

        // if 16-bit encoded, setup to shift first byte and unmask second
        if (pFrame[0] >= 16)
        {
            uMask = 255;
            uShift = 8;
            uSkip = 2;
        }

        // skip the header
        pFrame += 1;

        // decode and copy the table
        for (uCount = 0; uCount < 64; ++uCount)
        {
            pState->QuantTable[uIndex][uCount] = (pFrame[0]<<uShift)|(pFrame[1]&uMask);
            pFrame += uSkip;
        }
    }

    // return error if buffer size is wrong
    return(pFrame == pFinal ? 0 : DIRTYJPG_ERR_BADDQT);
}

/*F********************************************************************************/
/*!
    \Function _ParseDHT

    \Description
        Parse a DHT (huffman) table.

    \Input *pState  - state ref
    \Input *pFrame  - pointer to DHT frame data
    \Input *pFinal  - end of DHT frame

    \Output
        int32_t     - DIRTYJPG_ERR_*

    \Version 02/23/2006 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _ParseDHT(DirtyJpgStateT *pState, const uint8_t *pFrame, const uint8_t *pFinal)
{
    uint32_t uCount, uIndex, uOffset, uSize;
    uint16_t uCode, uShift;
    const uint8_t *pData;
    HuffTableT *pTable;
    HuffHeadT *pHead;

    while (pFrame < pFinal)
    {
        // ensure all "should be zero bits" are really zero
        if ((pFrame[0] & 0xec) != 0)
        {
            return(DIRTYJPG_ERR_BADDHT);
        }

        // point to the correct table
        pTable = &pState->HuffTable[(pFrame[0]>>2) | (pFrame[0]&3)];

        // offset to value/shift lookup
        uOffset = 0;

        // figure out max code based on first 8 huffman tables (1-8 bits consolidated)
        for (uSize=1, uCode=0; uSize < 8; uSize++)
        {
            uCode += pFrame[uSize];
            uCode += uCode;
        }
        uCode += pFrame[uSize];

        // setup first table entry
        pHead = &pTable->Head[0];
        pHead->uOffset = uOffset;
        // initial table is larger than the rest (based on code size)
        uOffset += uCode;
        pHead->uMaxCode = uCode;
        pHead->uShift = uSize;
        pHead += 1;

        // do remaining tables
        while (++uSize < 17)
        {
            uCode += uCode;
            uCode += pFrame[uSize];
            
            // only add header if it contains data
            if (pFrame[uSize] > 0)
            {
                pHead->uOffset = uOffset;
                // other tables are sized based on exact code counts
                uOffset += pFrame[uSize];
                pHead->uMaxCode = uCode;
                pHead->uShift = uSize;
                pHead += 1;
            }
        }

        // terminate header list
        pHead->uShift = 0;
        pHead->uMaxCode = uCode*2;
        pHead->uOffset = uOffset;       // so we can calc length of final table

        // previous max code
        uCode = 0;
        // start with one-bit codes
        uSize = 1;
        // copy value/shift data to all the tables
        pData = pFrame+17;
        for (pHead = &pTable->Head[0]; pHead->uShift != 0; ++pHead)
        {
            // change from size to shift count
            uShift = pHead->uShift;
            pHead->uShift = 16-uShift;

            // save the previous max code as our min code
            pHead->uMinCode = uCode;
            // get current max code
            uCode = pHead->uMaxCode;
            // right align the bits
            uCode <<= (16-uShift);
            // save the new right-aligned max code
            pHead->uMaxCode = uCode;

            // fill out the data portion (walk current data offset to next data offset)
            for (uCount = pHead[0].uOffset; uCount < pHead[1].uOffset;)
            {
                // copy over the huffman values
                for (uIndex = 0; uIndex < pFrame[uSize]; ++uIndex)
                {
                    // repeat count is variable in table 0 but always 1 for tables 1+
                    uint32_t uRepeat = (1 << uShift) >> uSize;
                    // copy the huffman value into the appropriate slots
                    memset(pTable->Code+uCount, *pData, uRepeat);
                    // copy the corresponding shift counts into the matching slots
                    memset(pTable->Step+uCount, uSize, uRepeat);
                    uCount += uRepeat;
                    pData += 1;
                }
                uSize += 1;
            }
        }

        // move to end of data
        pFrame = pData;
    }

    // make sure all data was processed
    return(pFrame == pFinal ? 0 : DIRTYJPG_ERR_BADDHT);
}

/*F********************************************************************************/
/*!
    \Function _ParseSOF0

    \Description
        Parse a SOF0 (start of frame 0) frame.

    \Input *pState  - state ref
    \Input *pFrame  - pointer to sof0 frame data
    \Input *pFinal  - end of sof0 frame

    \Output
        int32_t     - DIRTYJPG_ERR_*

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ParseSOF0(DirtyJpgStateT *pState, const uint8_t *pFrame, const uint8_t *pFinal)
{
    uint32_t uComp, uCompCount;
    uint32_t uHorzSampFactor, uVertSampFactor;
    CompTableT *pCompTable;
    const uint8_t *pComp;

    // validate 8bits per component
    if (pFrame[0] != 8)
    {
        return(DIRTYJPG_ERR_BITDEPTH);
    }

    pState->uImageHeight = (pFrame[1]<<8)|(pFrame[2]<<0);
    pState->uImageWidth = (pFrame[3]<<8)|(pFrame[4]<<0);
    uCompCount = pFrame[5];

    // parse components to find image mcu
    for (pComp = pFrame+6, uComp = 0; uComp < uCompCount; uComp++, pComp += 3)
    {
        uVertSampFactor = pComp[1] & 0xf;
        if (pState->uMcuVert < uVertSampFactor)
        {
            pState->uMcuVert = uVertSampFactor;
        }
        uHorzSampFactor = pComp[1] >> 4;
        if (pState->uMcuHorz < uHorzSampFactor)
        {
            pState->uMcuHorz = uHorzSampFactor;
        }
    }

    // calculate image dimensions in terms of number of mcu blocks
    pState->uMcuHorzM = (pState->uImageWidth + (8*pState->uMcuHorz)-1) / (pState->uMcuHorz*8);
    pState->uMcuVertM = (pState->uImageHeight + (8*pState->uMcuVert)-1) / (pState->uMcuVert*8);

    // parse components to fill in comp table
    for (pComp = pFrame+6, uComp = 0; uComp < uCompCount; uComp++, pComp += 3)
    {
        // skip invalid tabls
        if (pComp[0] > 3)
        {
            NetPrintf(("dirtyjpg: skipping invalid comp table index\n"));
            continue;
        }

        // ref comp table
        pCompTable = &pState->CompTable[pComp[0]];
        
        // init mcu
        pCompTable->uMcuHorz = pState->uMcuHorz;
        pCompTable->uMcuVert = pState->uMcuVert;

        // get sampling factors        
        pCompTable->uCompVert = pComp[1] & 0xf;
        pCompTable->uCompHorz = pComp[1] >> 4;
        
        // compute expansion factors
        pCompTable->uExpHorz = pCompTable->uMcuHorz / pCompTable->uCompHorz;
        pCompTable->uExpVert = pCompTable->uMcuVert / pCompTable->uCompVert;

        // calculate step based on aligned size
        pCompTable->uStepHorz0 = 4;
        pCompTable->uStepVert0 = pCompTable->uMcuHorz * 8 * 4;

        // compute scaled sample step sizes
        pCompTable->uStepHorz1 = pCompTable->uStepHorz0 * pCompTable->uExpHorz;
        pCompTable->uStepVert1 = pCompTable->uStepVert0 * pCompTable->uExpVert;
        
        // compute step * 8
        pCompTable->uStepHorz8 = pCompTable->uStepHorz1 * 8;
        pCompTable->uStepVert8 = pCompTable->uStepVert1 * 8;

        // compute mcu step sizes
        pCompTable->uStepHorzM = (pCompTable->uCompHorz * 8) * pCompTable->uStepHorz1;        
        pCompTable->uStepVertM = (pCompTable->uCompVert * 8) * pCompTable->uStepVert1;        

        // reference quantization table        
        pCompTable->uQuantNum = pComp[2]&0x3;
    }

    // index past comp table
    pFrame = pComp;

    return(pFrame == pFinal ? 0 : DIRTYJPG_ERR_BADSOF0);
}

/*F********************************************************************************/
/*!
    \Function _ParseSOS

    \Description
        Parse the SOS (Start of Scan) frame

    \Input *pState      - state ref
    \Input *pFrame      - pointer to SOS (start of scan) frame data
    \Input *pFinal      - end of frame data

    \Output
        uint8_t *       - pointer past end of frame, or NULL if there was an error

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static const uint8_t *_ParseSOS(DirtyJpgStateT *pState, const uint8_t *pFrame, const uint8_t *pFinal)
{
    uint32_t uCompIdx, uCompCount, uMcuH, uMcuV;
    const uint8_t *pCompData;

    // point to compressed data
    pCompData = pFrame - 2;
    pCompData = pCompData + ((pCompData[0] << 8) | pCompData[1]);

    // save compressed data
    pState->pCompData = pCompData;

    // get component count and skip to next byte
    uCompCount = *pFrame++;

    // init component tables
    for (uCompIdx = 0; uCompIdx < uCompCount; uCompIdx++)
    {
        _InitComp(pState, &pFrame[uCompIdx*2]);
    }

    // decode mcus and blit them to the output buffer
    for (uMcuV = 0; uMcuV < pState->uMcuVertM; uMcuV++)
    {
        for (uMcuH = 0; uMcuH < pState->uMcuHorzM; uMcuH++)
        {
            // process an MCU
            if (_GetMCU(pState, pFrame, uCompCount) == 0xffffffff)
            {
                return(NULL);
            }

            // put the colors into buffer        
            _PutColor(pState, &pState->CompTable[1], uMcuH, uMcuV);
        }
    }
    
    // return number of bytes consumed
    return(pState->pCompData + (pState->uBitsConsumed+7)/8);
}

/*F********************************************************************************/
/*!
    \Function _DirtyJpgDecodeParse

    \Description
        Parse the JFIF image data.

    \Input *pState      - state ref
    \Input *pImage      - pointer to image data
    \Input uLength      - size of image data
    \Input bIdentify    - identify if this is a JFIF image or not

    \Output
        int32_t         - DIRTYJPG_ERR_*

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _DirtyJpgDecodeParse(DirtyJpgStateT *pState, const uint8_t *pImage, uint32_t uLength, uint8_t bIdentify)
{
    int32_t iError  = 0;
    const uint8_t *pStart=pImage, *pFinal = pImage+uLength;
    uint8_t uType;
    uint32_t uSize;

    // decode all the frames
    while (pImage < pFinal)
    {
        // get the frame info
        uType = *pImage++;

        // gobble alignment byte
        if (uType == 0xff)
        {
            continue;
        }

        // get size (no size for eoi)
        uSize = (uType != 0xd9) ? (pImage[0]<<8)|(pImage[1]<<0) : 0;

        // validate the frame size
        if ((pImage+uSize) > pFinal)
        {
            iError = DIRTYJPG_ERR_ENDOFDATA;
            break;
        }

        #if DIRTYJPG_VERBOSE
        NetPrintf(("dirtyjpg: frame=0x%02x size=%d\n", uType, uSize));
        #endif

        // parse the frame
        switch (uType)
        {
            // app0 frame
            case 0xe0:
            {
                // verify its got JFIF header
                if ((pImage[2] == 'J') && (pImage[3] == 'F') && (pImage[4] == 'I') && (pImage[5] == 'F'))
                {
                    // extract the version and make sure we support it
                    pState->uVersion = (pImage[7]<<8)|(pImage[8]>>0);
                    if ((pState->uVersion < 0x0100) || (pState->uVersion > 0x0102))
                    {
                        iError = DIRTYJPG_ERR_BADVERS;
                        break;
                    }

                    // extract & save aspect info
                    pState->uAspectRatio = pImage[9];
                    pState->uAspectHorz = (pImage[10]<<8)|(pImage[11]<<0);
                    pState->uAspectVert = (pImage[12]<<8)|(pImage[13]<<0);
                    
                    // if just doing identification, bail
                    if (bIdentify)
                    {
                        return(DIRTYJPG_ERR_NONE);
                    }
                    break;
                }
            }
            // dqt frame
            case 0xdb:
            {
                iError = _ParseDQT(pState, pImage+2, pImage+uSize);
                break;
            }
            // dht frame
            case 0xc4:
            {
                iError = _ParseDHT(pState, pImage+2, pImage+uSize);
                break;
            }
            // sof0 frame (baseline jpeg)
            case 0xc0:
            {
                iError = _ParseSOF0(pState, pImage+2, pImage+uSize);
                break;
            }
            // sof2 frame (progressive jpeg)
            case 0xc2:
                NetPrintf(("dirtyjpg: warning; SOF2 frame indicates a progressively encoded image, which is not supported\n"));
                iError = DIRTYJPG_ERR_NOSUPPORT;
                break;

            // sos (start of scan) frame
            case 0xda:
            {
                if (pState->pImageBuf != NULL)
                {
                    pImage = _ParseSOS(pState, pImage+2, pImage+uSize);
                    if (pImage == NULL)
                    {
                        iError = DIRTYJPG_ERR_DECODER;
                    }
                }
                else
                {
                    iError = DIRTYJPG_ERR_NOBUFFER;
                    pState->uLastOffset = pImage-pStart;
                }
                break;
            }
            // skip "no action" frames
            case 0xd9:      // eoi frame
            case 0xfe:      // com frame
            {
                break;
            }

            default:
                if ((uType & 0xf0) != 0xe0)
                {
                    NetPrintf(("dirtyjpg: ignoring unrecognized frame type 0x%02x\n", uType));
                }
                #if DIRTYJPG_VERBOSE
                else
                {
                    NetPrintf(("dirtyjpg: ignoring application-specific frame type 0x%02x\n", uType));
                }
                #endif
                break;
        }

        // bail if we got an error
        if (iError != 0)
        {
            break;
        }

        // move to next record
        pImage += uSize;
    }

    // save last error
    pState->iLastError = iError;
    return(iError);
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function DirtyJpgCreate

    \Description
        Create the DirtyJpg module state

    \Output
        DirtyJpgStateT *   - pointer to new ref, or NULL if error

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
DirtyJpgStateT *DirtyJpgCreate(void)
{
    DirtyJpgStateT *pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pState = DirtyMemAlloc(sizeof(*pState), DIRTYJPG_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtyjpg: error allocating module state\n"));
        return(NULL);
    }
    memset(pState, 0, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;

    // return the state pointer
    return(pState);
}

/*F********************************************************************************/
/*!
    \Function DirtyJpgReset

    \Description
        Reset the DirtyJpg module state

    \Input *pState  - pointer to module state

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
void DirtyJpgReset(DirtyJpgStateT *pState)
{
    pState->uLastOffset = 0;

    pState->uBitfield = 0;
    pState->uBitsAvail = 0;
    pState->uBitsConsumed = 0;
    pState->uBytesRead = 0;

    // clear all comp tables
    memset(pState->CompTable, 0, sizeof(pState->CompTable));

    // clear all quant tables
    memset(pState->QuantTable, 0, sizeof(pState->QuantTable));

    // clear all huffman tables
    memset(pState->HuffTable, 0, sizeof(pState->HuffTable));

    // set the image buffer to NULL
    pState->pImageBuf = NULL;

    pState->bRestart = FALSE;
}

/*F********************************************************************************/
/*!
    \Function DirtyJpgDestroy

    \Description
        Destroy the DirtyJpg module

    \Input *pState  - pointer to module state

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
void DirtyJpgDestroy(DirtyJpgStateT *pState)
{
    DirtyMemFree(pState, DIRTYJPG_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function DirtyJpgIdentify

    \Description
        Identify if input image is a JPG (JFIF) image.

    \Input *pState      - jpg module state
    \Input *pImageData  - pointer to image data
    \Input uImageLen    - size of image data

    \Output
        int32_t         - TRUE if a JPG, else FALSE

    \Version 03/09/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyJpgIdentify(DirtyJpgStateT *pState, const uint8_t *pImageData, uint32_t uImageLen)
{
    int32_t iResult;
    // basic data validation
    if (uImageLen < 16)
    {
        return(0);
    }
    // validate the header
    if ((pImageData[0] != 0xff) || (pImageData[1] != 0xd8))
    {
        return(0);
    }
    // run the parser
    iResult = _DirtyJpgDecodeParse(pState, pImageData+2, uImageLen-2, TRUE);
    return((iResult == DIRTYJPG_ERR_NOBUFFER) || (iResult == DIRTYJPG_ERR_NONE));
}

/*F********************************************************************************/
/*!
    \Function DirtyJpgDecodeHeader

    \Description
        Parse JPG header.

    \Input *pState  - pointer to module state
    \Input *pJpgHdr - pointer to jpg header
    \Input *pImage  - pointer to image buf
    \Input uLength  - size of input image

    \Output
        int32_t     - DIRTYJPG_ERR_*

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyJpgDecodeHeader(DirtyJpgStateT *pState, DirtyJpgHdrT *pJpgHdr, const uint8_t *pImage, uint32_t uLength)
{
    int32_t iError;

    // reset internal state
    DirtyJpgReset(pState);

    // basic data validation
    if (uLength < 16)
    {
        return(DIRTYJPG_ERR_TOOSHORT);
    }
    // validate the header
    if ((pImage[0] != 0xff) || (pImage[1] != 0xd8))
    {
        return(DIRTYJPG_ERR_NOMAGIC);
    }

    // run the parser -- we should get a no buffer error
    if ((iError = _DirtyJpgDecodeParse(pState, pImage+2, uLength-2, FALSE)) == DIRTYJPG_ERR_NOBUFFER)
    {
        // this is expected during header parsing -- save info and return no error
        pJpgHdr->pImageData = pImage;
        pJpgHdr->uLength = uLength;
        pJpgHdr->uWidth = pState->uImageWidth;
        pJpgHdr->uHeight = pState->uImageHeight;
        iError = 0;
    }
    return(iError);
}

/*F********************************************************************************/
/*!
    \Function DirtyJpgDecodeImage

    \Description
        Parse JPG image.

    \Input *pState      - pointer to module state
    \Input *pJpgHdr     - pointer to jpg header
    \Input *pImageBuf   - pointer to image buf
    \Input iBufWidth    - image buf width
    \Input iBufHeight   - image buf height

    \Output
        int32_t     - DIRTYJPG_ERR_*

    \Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyJpgDecodeImage(DirtyJpgStateT *pState, DirtyJpgHdrT *pJpgHdr, uint8_t *pImageBuf, int32_t iBufWidth, int32_t iBufHeight)
{
    int32_t iError;

    // make sure we are in proper state
    if (pState->iLastError != DIRTYJPG_ERR_NOBUFFER)
    {
        return(DIRTYJPG_ERR_BADSTATE);
    }

    // setup the output buffer
    pState->pImageBuf = pImageBuf;
    pState->uBufWidth = (unsigned)iBufWidth;
    pState->uBufHeight = (unsigned)iBufHeight;

    // resume parsing at last spot
    iError = _DirtyJpgDecodeParse(pState, pJpgHdr->pImageData + pState->uLastOffset, pJpgHdr->uLength-pState->uLastOffset, FALSE);
    return(iError);
}
