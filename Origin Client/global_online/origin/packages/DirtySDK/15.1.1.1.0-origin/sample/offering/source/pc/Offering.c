/*H*************************************************************************************************/
/*!
\File Offering.c

\Description
A simple test application showing off the client credit card API.

\Copyright
Copyright (c) Tiburon Entertainment / Electronic Arts 2004.    ALL RIGHTS RESERVED.

\Version 03/11/2004 gschaefer
*/
/*************************************************************************************************H*/

/*** Include files *********************************************************************/

#pragma warning(push,0)
#include <windows.h>
#pragma warning(pop)

#include <commctrl.h>
#include <richedit.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "offering.h"

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/web/weboffer.h"
#include "DirtySDK/util/tagfield.h"
#include "DirtySDK/graph/dirtygif.h"
#include "DirtySDK/util/utf8.h"
#include "samplecore.h"

#include "DirtySDK/graph/dirtyjpg.h"
#include "DirtySDK/graph/dirtygraph.h"

#include "zmem.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/
const uint32_t COLOR_MANDATORY = RGB(255,0,0);
const uint32_t COLOR_NOTMANDATORY = RGB(0,0,0);
const uint32_t COLOR_DISABLED = RGB(236,236,233);
/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

//! filename of input file, if provided
//! leave blank if not used, and script will load from previous config
char g_strInputFile[32];

// UTF-8 to ASCII8 master translation table
static unsigned char _Utf8ToWindowsMain[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,     // 00-0f
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,     // 10-1f
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,     // 20-2f
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,     // 30-3f
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,     // 40-4f
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x05, 0x5e, 0x5f,     // 50-5f
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,     // 60-6f
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,     // 70-7f
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,     // 80-8f
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,     // 90-9f
        0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,     // a0-af
        0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,     // b0-bf
        0xcc, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,     // c0-cf
        0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,     // d0-df
        0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,     // e0-ef
        0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,     // f0-ff
};

static Utf8TransTblT _Utf8ToWindows[]=
{
    { 0x0000, 0x00ff, _Utf8ToWindowsMain },         // map as 1-to-1
    { 0x20ac, 0x20ac, _Utf8ToWindowsMain+0x80 },
    { 0x201a, 0x201a, _Utf8ToWindowsMain+0x82 },
    { 0x0192, 0x0192, _Utf8ToWindowsMain+0x83 },
    { 0x201e, 0x201e, _Utf8ToWindowsMain+0x84 },
    { 0x2026, 0x2026, _Utf8ToWindowsMain+0x85 },
    { 0x2020, 0x2021, _Utf8ToWindowsMain+0x86 },
    { 0x02c6, 0x02c6, _Utf8ToWindowsMain+0x88 },
    { 0x2030, 0x2030, _Utf8ToWindowsMain+0x89 },
    { 0x0160, 0x0160, _Utf8ToWindowsMain+0x8a },
    { 0x2039, 0x2039, _Utf8ToWindowsMain+0x8b },
    { 0x0152, 0x0152, _Utf8ToWindowsMain+0x8c },
    { 0x017d, 0x017d, _Utf8ToWindowsMain+0x8e },
    { 0x2018, 0x2019, _Utf8ToWindowsMain+0x91 },
    { 0x201c, 0x201d, _Utf8ToWindowsMain+0x93 },
    { 0x2022, 0x2022, _Utf8ToWindowsMain+0x95 },
    { 0x2013, 0x2014, _Utf8ToWindowsMain+0x96 },
    { 0x02dc, 0x02dc, _Utf8ToWindowsMain+0x98 },
    { 0x2122, 0x2122, _Utf8ToWindowsMain+0x99 },     // handle (tm)
    { 0x0161, 0x0161, _Utf8ToWindowsMain+0x9a },
    { 0x203a, 0x203a, _Utf8ToWindowsMain+0x9b },
    { 0x0153, 0x0153, _Utf8ToWindowsMain+0x9c },
    { 0x017e, 0x017e, _Utf8ToWindowsMain+0x9e },
    { 0x0178, 0x0178, _Utf8ToWindowsMain+0x9f },
    { 0x0000, 0x0000, NULL, },                      // end of list
};

/*** Functions *************************************************************************/

// alloc/free functions that must be supplied for the dirtysock library
void *DirtyMemAlloc(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    return(malloc(iSize));
}

void DirtyMemFree(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    free(pMem);
}

void SampleStartup(char *strCommandLine)
{
    NetConnStartup("-servicename=ticker");

    // set the filename if provided
    memset(g_strInputFile, 0, sizeof(g_strInputFile));
    ds_strnzcpy(g_strInputFile, strCommandLine, sizeof(g_strInputFile));
}

void SampleShutdown()
{
    NetConnShutdown(0);
}

// export a script to a text file
static int32_t _ExportScript(const char *pFilename, const char *pScript)
{
    FILE *fd;
    int32_t iSkip;
    char *pBogus;
    const unsigned char *pLineHead;
    const unsigned char *pLineTail;

    // prepare file for writing
    fd = fopen("script.txt", "wb");

    // write each line of the file
    for (pLineTail = (unsigned char *)pScript; *pLineTail != 0;)
    {
        // locate the line start
        for (pLineHead = pLineTail; (*pLineHead != 0) && (*pLineHead <= ' '); ++pLineHead)
            ;
        // locate the line end
        for (pLineTail = pLineHead; (*pLineTail != 0) && (*pLineTail != '\n'); ++pLineTail)
            ;

        // indent non-control lines
        if (*pLineHead != '%')
        {
            fwrite("  ", 2, 1, fd);
        }
        // store the line
        fwrite(pLineHead, pLineTail-pLineHead, 1, fd);
        // end of line
        fwrite("\r\n", 2, 1, fd);

        // add extra blank after %}
        if ((pLineHead[0] == '%') && (pLineHead[1] == '}'))
        {
            fwrite("\r\n", 2, 1, fd);
        }

        // handle binary data marker
        if ((pLineHead[0] == '%') && (pLineHead[1] == '$') && (pLineHead[10] == ':'))
        {
            // get binary data length and ensure its within buffer
            iSkip = strtol((char *)pLineHead+2, &pBogus, 16);
            iSkip += 12;
            pLineHead = pLineTail = pLineHead+iSkip;
            fprintf(fd, "  (%d resource data bytes)\n\n", iSkip);
        }
    }

    fclose(fd);
    return(0);
}

// map dialog button click to action identifier
static int32_t _MapIDCToAction(int32_t iButton)
{
    switch(iButton)
    {
    case IDC_BTN1:      iButton = WEBOFFER_ACTION_BTN1;         break;
    case IDC_BTN2:      iButton = WEBOFFER_ACTION_BTN2;         break;
    case IDC_BTN3:      iButton = WEBOFFER_ACTION_BTN3;         break;
    case IDC_BTN4:      iButton = WEBOFFER_ACTION_BTN4;         break;
    case IDC_BTN5:      iButton = WEBOFFER_ACTION_BTN5;         break;
    case IDC_BTN6:      iButton = WEBOFFER_ACTION_BTN6;         break;
    case IDC_BTN7:      iButton = WEBOFFER_ACTION_BTN7;         break;
    case IDC_BTN8:      iButton = WEBOFFER_ACTION_BTN8;         break;
    case IDC_ARTICLE1:  iButton = WEBOFFER_ACTION_ARTICLE1;     break;
    case IDC_ARTICLE2:  iButton = WEBOFFER_ACTION_ARTICLE2;     break;
    case IDC_ARTICLE3:  iButton = WEBOFFER_ACTION_ARTICLE3;     break;
    case IDC_ARTICLE4:  iButton = WEBOFFER_ACTION_ARTICLE4;     break;
    case IDC_ARTICLE5:  iButton = WEBOFFER_ACTION_ARTICLE5;     break;
    case IDC_ARTICLE6:  iButton = WEBOFFER_ACTION_ARTICLE6;     break;
    case IDC_ARTICLE7:  iButton = WEBOFFER_ACTION_ARTICLE7;     break;
    case IDC_ARTICLE8:  iButton = WEBOFFER_ACTION_ARTICLE8;     break;
    case IDC_ARTICLE9:  iButton = WEBOFFER_ACTION_ARTICLE9;     break;
    case IDC_ARTICLE10:  iButton = WEBOFFER_ACTION_ARTICLE10;   break;
    case IDC_ARTICLE11:  iButton = WEBOFFER_ACTION_ARTICLE11;   break;
    case IDC_ARTICLE12:  iButton = WEBOFFER_ACTION_ARTICLE12;   break;
    case IDC_ARTICLE13:  iButton = WEBOFFER_ACTION_ARTICLE13;   break;
    case IDC_ARTICLE14:  iButton = WEBOFFER_ACTION_ARTICLE14;   break;
    case IDC_ARTICLE15:  iButton = WEBOFFER_ACTION_ARTICLE15;   break;
    case IDC_ARTICLE16:  iButton = WEBOFFER_ACTION_ARTICLE16;   break;
   }
    return(iButton);
}

static int32_t _MapArticleToIDC(int32_t iArticle)
{
    switch(iArticle)
    {
    case 1:     iArticle = IDC_ARTICLE1;     break;
    case 2:     iArticle = IDC_ARTICLE2;     break;
    case 3:     iArticle = IDC_ARTICLE3;     break;
    case 4:     iArticle = IDC_ARTICLE4;     break;
    case 5:     iArticle = IDC_ARTICLE5;     break;
    case 6:     iArticle = IDC_ARTICLE6;     break;
    case 7:     iArticle = IDC_ARTICLE7;     break;
    case 8:     iArticle = IDC_ARTICLE8;     break;
    case 9:     iArticle = IDC_ARTICLE9;     break;
    case 10:    iArticle = IDC_ARTICLE10;    break;
    case 11:    iArticle = IDC_ARTICLE11;    break;
    case 12:    iArticle = IDC_ARTICLE12;    break;
    case 13:    iArticle = IDC_ARTICLE13;    break;
    case 14:    iArticle = IDC_ARTICLE14;    break;
    case 15:    iArticle = IDC_ARTICLE15;    break;
    case 16:    iArticle = IDC_ARTICLE16;    break;
    }
    return(iArticle);
}

// map button number to IDC reference
static int32_t _MapButtonToIDC(int32_t iButton)
{
    switch(iButton)
    {
    case 1:     iButton = IDC_BTN1;     break;
    case 2:     iButton = IDC_BTN2;     break;
    case 3:     iButton = IDC_BTN3;     break;
    case 4:     iButton = IDC_BTN4;     break;
    case 5:     iButton = IDC_BTN5;     break;
    case 6:     iButton = IDC_BTN6;     break;
    case 7:     iButton = IDC_BTN7;     break;
    case 8:     iButton = IDC_BTN8;     break;
    }
    return(iButton);
}
// map Image number to IDC reference
static int32_t _MapArticleImageToIDC(int32_t iButton)
{
    switch(iButton)
    {
    case 1:     iButton = IDC_ARTICLE_IMG1 ;    break;
    case 2:     iButton = IDC_ARTICLE_IMG2;     break;
    case 3:     iButton = IDC_ARTICLE_IMG3;     break;
    case 4:     iButton = IDC_ARTICLE_IMG4;     break;
    case 5:     iButton = IDC_ARTICLE_IMG5;     break;
    case 6:     iButton = IDC_ARTICLE_IMG6;     break;
    case 7:     iButton = IDC_ARTICLE_IMG7;     break;
    case 8:     iButton = IDC_ARTICLE_IMG8;     break;
    case 9:     iButton = IDC_ARTICLE_IMG9;     break;
    case 10:    iButton = IDC_ARTICLE_IMG10;    break;
    case 11:    iButton = IDC_ARTICLE_IMG11;    break;
    case 12:    iButton = IDC_ARTICLE_IMG12;    break;
    case 13:    iButton = IDC_ARTICLE_IMG13;    break;
    case 14:    iButton = IDC_ARTICLE_IMG14;    break;
    case 15:    iButton = IDC_ARTICLE_IMG15;    break;
    case 16:    iButton = IDC_ARTICLE_IMG16;    break;
    }
    return(iButton);
}

// map type number to IDC reference
static int32_t _MapArticleTypeToIDC(int32_t iButton)
{
    switch(iButton)
    {
    case 1:     iButton = IDC_ARTICLE_TYPE1;     break;
    case 2:     iButton = IDC_ARTICLE_TYPE2;     break;
    case 3:     iButton = IDC_ARTICLE_TYPE3;     break;
    case 4:     iButton = IDC_ARTICLE_TYPE4;     break;
    case 5:     iButton = IDC_ARTICLE_TYPE5;     break;
    case 6:     iButton = IDC_ARTICLE_TYPE6;     break;
    case 7:     iButton = IDC_ARTICLE_TYPE7;     break;
    case 8:     iButton = IDC_ARTICLE_TYPE8;     break;
    case 9:     iButton = IDC_ARTICLE_TYPE9;     break;
    case 10:    iButton = IDC_ARTICLE_TYPE10;    break;
    case 11:    iButton = IDC_ARTICLE_TYPE11;    break;
    case 12:    iButton = IDC_ARTICLE_TYPE12;    break;
    case 13:    iButton = IDC_ARTICLE_TYPE13;    break;
    case 14:    iButton = IDC_ARTICLE_TYPE14;    break;
    case 15:    iButton = IDC_ARTICLE_TYPE15;    break;
    case 16:    iButton = IDC_ARTICLE_TYPE16;    break;
    }
    return(iButton);
}

// map Description number to IDC reference
static int32_t _MapArticleDescToIDC(int32_t iButton)
{
    switch(iButton)
    {
    case 1:     iButton = IDC_ARTICLE_DESC1;     break;
    case 2:     iButton = IDC_ARTICLE_DESC2;     break;
    case 3:     iButton = IDC_ARTICLE_DESC3;     break;
    case 4:     iButton = IDC_ARTICLE_DESC4;     break;
    case 5:     iButton = IDC_ARTICLE_DESC5;     break;
    case 6:     iButton = IDC_ARTICLE_DESC6;     break;
    case 7:     iButton = IDC_ARTICLE_DESC7;     break;
    case 8:     iButton = IDC_ARTICLE_DESC8;     break;
    case 9:     iButton = IDC_ARTICLE_DESC9;     break;
    case 10:    iButton = IDC_ARTICLE_DESC10;    break;
    case 11:    iButton = IDC_ARTICLE_DESC11;    break;
    case 12:    iButton = IDC_ARTICLE_DESC12;    break;
    case 13:    iButton = IDC_ARTICLE_DESC13;    break;
    case 14:    iButton = IDC_ARTICLE_DESC14;    break;
    case 15:    iButton = IDC_ARTICLE_DESC15;    break;
    case 16:    iButton = IDC_ARTICLE_DESC16;    break;
    }
    return(iButton);
}

// map Description number to IDC reference
static int32_t _MapComboToIDC(int32_t iCombo)
{
    switch(iCombo)
    {
    case 1:     iCombo = IDC_COMBO1;     break;
    case 2:     iCombo = IDC_COMBO2;     break;
    case 3:     iCombo = IDC_COMBO3;     break;
    case 4:     iCombo = IDC_COMBO4;     break;
    case 5:     iCombo = IDC_COMBO5;     break;
    case 6:     iCombo = IDC_COMBO6;     break;
    case 7:     iCombo = IDC_COMBO7;     break;
    case 8:     iCombo = IDC_COMBO8;     break;
    case 9:     iCombo = IDC_COMBO9;     break;
    case 10:    iCombo = IDC_COMBO10;    break;
    case 11:    iCombo = IDC_COMBO11;    break;
    case 12:    iCombo = IDC_COMBO12;    break;
    case 13:    iCombo = IDC_COMBO13;    break;
    case 14:    iCombo = IDC_COMBO14;    break;
    case 15:    iCombo = IDC_COMBO15;    break;
    case 16:    iCombo = IDC_COMBO16;    break;
    }
    return(iCombo);
}

// map Description number to IDC reference
static int32_t _MapEditToIDC(int32_t iEdit)
{
    switch(iEdit)
    {
    case 1:     iEdit = IDC_EDIT1;     break;
    case 2:     iEdit = IDC_EDIT2;     break;
    case 3:     iEdit = IDC_EDIT3;     break;
    case 4:     iEdit = IDC_EDIT4;     break;
    case 5:     iEdit = IDC_EDIT5;     break;
    case 6:     iEdit = IDC_EDIT6;     break;
    case 7:     iEdit = IDC_EDIT7;     break;
    case 8:     iEdit = IDC_EDIT8;     break;
    case 9:     iEdit = IDC_EDIT9;     break;
    case 10:    iEdit = IDC_EDIT10;    break;
    case 11:    iEdit = IDC_EDIT11;    break;
    case 12:    iEdit = IDC_EDIT12;    break;
    case 13:    iEdit = IDC_EDIT13;    break;
    case 14:    iEdit = IDC_EDIT14;    break;
    case 15:    iEdit = IDC_EDIT15;    break;
    case 16:    iEdit = IDC_EDIT16;    break;
    }
    return(iEdit);
}

// map Description number to IDC reference
static int32_t _MapNameToIDC(int32_t iName)
{
    switch(iName)
    {
    case 1:     iName = IDC_NAME1;     break;
    case 2:     iName = IDC_NAME2;     break;
    case 3:     iName = IDC_NAME3;     break;
    case 4:     iName = IDC_NAME4;     break;
    case 5:     iName = IDC_NAME5;     break;
    case 6:     iName = IDC_NAME6;     break;
    case 7:     iName = IDC_NAME7;     break;
    case 8:     iName = IDC_NAME8;     break;
    case 9:     iName = IDC_NAME9;     break;
    case 10:    iName = IDC_NAME10;    break;
    case 11:    iName = IDC_NAME11;    break;
    case 12:    iName = IDC_NAME12;    break;
    case 13:    iName = IDC_NAME13;    break;
    case 14:    iName = IDC_NAME14;    break;
    case 15:    iName = IDC_NAME15;    break;
    case 16:    iName = IDC_NAME16;    break;
    }
    return(iName);
}

static int32_t _ConvertToBMP(uint8_t *pImageData, int32_t iWidth, int32_t iHeight, uint8_t* pImageOut,uint32_t uFileSize, BITMAPINFOHEADER* pBitmapInfoHeader)
{
//    BITMAPFILEHEADER BitmapFileHeader;
    int32_t iW, iH;
    uint8_t *pPixelHi, *pPixelLo, uTmp;
    uint8_t* pImageTemp = pImageOut;
    // open the file for writing

    // vflip image and convert from ARGB to BGRA in one pass
    for (iH = 0; iH < (iHeight/2); iH++)
    {
        for (iW = 0; iW < iWidth; iW++)
        {
            pPixelHi = pImageData + (iW*4) + (iH*iWidth*4);
            pPixelLo = pImageData + (iW*4) + ((iHeight-iH-1)*iWidth*4);

            // swap a(hi) and b(lo)
            uTmp = pPixelHi[0];         // save a(hi)
            pPixelHi[0] = pPixelLo[3];  // b(lo)->a(hi)
            pPixelLo[3] = uTmp;         // a(hi)->b(lo)

            // swap b(hi) and a(lo)
            uTmp = pPixelHi[3];         // save b(hi)
            pPixelHi[3] = pPixelLo[0];  // a(lo)->b(hi)
            pPixelLo[0] = uTmp;         // b(hi)->a(lo)

            // swap r(hi) and g(lo)
            uTmp = pPixelHi[1];         // save r(hi)
            pPixelHi[1] = pPixelLo[2];  // g(lo)->r(hi)
            pPixelLo[2] = uTmp;         // r(hi)->g(lo)

            // swap g(hi) and r(lo)
            uTmp = pPixelHi[2];         // save g(hi)
            pPixelHi[2] = pPixelLo[1];  // r(lo)->g(hi)
            pPixelLo[1] = uTmp;         // g(hi)->r(lo)
        }
    }
/*
    // format bitmap header
    memset(&BitmapFileHeader, 0, sizeof(BitmapFileHeader));
    BitmapFileHeader.bfType = 'MB';
    BitmapFileHeader.bfSize = sizeof(BitmapFileHeader)+sizeof(BITMAPINFOHEADER)+(iWidth*iHeight*4);
    BitmapFileHeader.bfOffBits = sizeof(BitmapFileHeader)+sizeof(BITMAPINFOHEADER);

    memcpy(pImageTemp, &BitmapFileHeader, sizeof(BitmapFileHeader));
    pImageTemp += (sizeof(BitmapFileHeader));

    // format bitmapinfo header
    pBitmapInfoHeader->biSize = sizeof(BITMAPINFOHEADER);
    pBitmapInfoHeader->biWidth = iWidth;
    pBitmapInfoHeader->biHeight = iHeight;
    pBitmapInfoHeader->biPlanes = 1;
    pBitmapInfoHeader->biBitCount = 32;
    pBitmapInfoHeader->biCompression = BI_RGB;
    pBitmapInfoHeader->biSizeImage = iWidth*iHeight*4;
    pBitmapInfoHeader->biXPelsPerMeter = 0;
    pBitmapInfoHeader->biYPelsPerMeter = 0;
    pBitmapInfoHeader->biClrUsed = 0;
    pBitmapInfoHeader->biClrImportant = 0;
    memcpy(pImageTemp, pBitmapInfoHeader, sizeof(BITMAPINFOHEADER));
    pImageTemp += (sizeof(BITMAPINFOHEADER) );
*/

    memcpy(pImageTemp, pImageData, iWidth*iHeight*4 );

    // close the file
    return(1);

}

/*F********************************************************************************/
/*!
    \Function _CmdImgConvSaveRAW

    \Description
        Save input 32bit ARGB image as a 24bit RAW image.

    \Input *pImageData  - input image data
    \Input *pDst        - output image data
    \Input iWidth       - width of input image
    \Input iHeight      - height of input image

    \Output - None

\Version 02/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _Convert32BitImageTo24Bit(uint8_t *pImageData, int32_t iWidth, int32_t iHeight )
{
    uint8_t *pSrc;
    int32_t iW, iH;

    // convert 32bit ARGB image to 24bit RGB image (inline)
    for (pSrc=pImageData,  iH=0; iH < iHeight; iH++)
    {
        for (iW = 0; iW < iWidth; iW++)
        {
            int32_t iTemp[4];
            iTemp[0] = pSrc[0];
            iTemp[1] = pSrc[1];
            iTemp[2] = pSrc[2];
            iTemp[3] = pSrc[3];
            pSrc[1] = iTemp[3];
            pSrc[2] = iTemp[1];
            pSrc[3] = iTemp[0];
            pSrc[0] = iTemp[2];
            pSrc += 4;
        }
    }
}

static uint8_t * _ConvertImage(const uint8_t *pInputFile, uint32_t uInputSize, int32_t *pWidth, int32_t *pHeight)
{
    DirtyGraphRefT *pDirtyGraph;
    DirtyGraphInfoT ImageInfo;
    uint8_t *p32Bit;
    int32_t iError;

    // create module state
    pDirtyGraph = DirtyGraphCreate();

    // parse the jpeg header
    if ((iError = DirtyGraphDecodeHeader(pDirtyGraph, &ImageInfo, pInputFile, uInputSize)) < 0)
    {
        DirtyGraphDestroy(pDirtyGraph);
        return(NULL);
    }

    // allocate space for 32bit image
    if ((p32Bit = ZMemAlloc((ImageInfo.iWidth) * (ImageInfo.iHeight) * 4)) == NULL)
    {
        DirtyGraphDestroy(pDirtyGraph);
        return(NULL);
    }

    // decode the image
    if ((iError = DirtyGraphDecodeImage(pDirtyGraph, &ImageInfo, p32Bit, ImageInfo.iWidth, ImageInfo.iHeight)) < 0)
    {
        DirtyGraphDestroy(pDirtyGraph);
        ZMemFree(p32Bit);
        return(NULL);
    }

    // destroy state
    DirtyGraphDestroy(pDirtyGraph);

    // return 24bit image
    *pWidth = ImageInfo.iWidth;
    *pHeight = ImageInfo.iHeight;
    return(p32Bit);


}


static void _switchImageTopBottom(uint8_t* pImage, int32_t iWidth, int32_t iHeight)
{
    uint8_t* pLineTop = pImage;
    uint8_t* pLineDown = &pImage[(iHeight*iWidth*4) -  (iWidth*4)];
    while (pLineTop < pLineDown)
    {
        int32_t iIdx2;
        for (iIdx2 = 0 ;iIdx2 < iWidth*4; ++iIdx2)
        {
            int32_t iTemp = pLineTop[iIdx2];
            pLineTop[iIdx2] = pLineDown[iIdx2];
            pLineDown[iIdx2] = iTemp;
        }
        pLineTop += (iWidth*4);
        pLineDown -= (iWidth*4);
    }

}
// Take an image and display in a dialog.
// This function is application specific as web-offer does not impose any media type
// restrictions and simply delivers a binary resource the application must use as
// the image.
void _SetupImage(HWND pHandle, int32_t iItem, WebOfferT *pOffer, const char *pName)
{
    WebOfferResourceT Resource;
    int32_t iWidth, iHeight;

    // grab image if present
    if (WebOfferResource(pOffer, &Resource, pName) == 0)
    {
        return;
    }

    // debug code to store image to disk
    if (0)
    {
        FILE *fd = fopen("gifdata.gif", "wb");
        fwrite(Resource.pData, Resource.iSize, 1, fd);
        fclose(fd);
    }

    // handle gif8 via Dirtygif module
    if (Resource.iType == 'GIF8' || Resource.iType == 'JPEG')
    {
        uint8_t *p32BitImage;

        p32BitImage = _ConvertImage((uint8_t *)Resource.pData, Resource.iSize, &iWidth, &iHeight);
        if(p32BitImage)
        {
            _Convert32BitImageTo24Bit(p32BitImage, iWidth, iHeight);
            _switchImageTopBottom(p32BitImage, iWidth, iHeight);
            DialogImageSet2(pHandle, iItem, p32BitImage, iWidth, iHeight);
            ZMemFree(p32BitImage);
        }
        else
        {
            uint32_t uFlags;

            // need to change control from bitmap to static text
            uFlags = GetWindowLong(GetDlgItem(pHandle, iItem), GWL_STYLE);
            uFlags ^= SS_BITMAP;
            uFlags ^= (SS_CENTER|SS_CENTERIMAGE);
            SetWindowLong(GetDlgItem(pHandle, iItem), GWL_STYLE, uFlags);

            // show the resource info since we don't know how to display this type
            SetDlgItemText(pHandle, iItem, "NULL");
        }

    }
    // handle any other type by showing as text
    else
    {
        char strText[256];
        uint32_t uFlags;

        // need to change control from bitmap to static text
        uFlags = GetWindowLong(GetDlgItem(pHandle, iItem), GWL_STYLE);
        uFlags ^= SS_BITMAP;
        uFlags ^= (SS_CENTER|SS_CENTERIMAGE);
        SetWindowLong(GetDlgItem(pHandle, iItem), GWL_STYLE, uFlags);

        // show the resource info since we don't know how to display this type
        wsprintf(strText, "TYPE=%c%c%c%c\r\nSIZE=%d",
            (Resource.iType>>24), (Resource.iType>>16), (Resource.iType>>8), (Resource.iType>>0),
            Resource.iSize);
        SetDlgItemText(pHandle, iItem, strText);
    }
}

// get the default app with which to open this file extention
void _GetDefaultApp(char* strDefaultApp, char* strExtention)
{
    HKEY Key;
    DWORD dwSize=2*MAX_PATH;
    char strBrowser[2*MAX_PATH];
    memset(strDefaultApp, 0, sizeof(strDefaultApp));
    if(RegOpenKeyEx(HKEY_CLASSES_ROOT,strExtention,0,KEY_READ,&Key)!=ERROR_SUCCESS)
    {
        // open with the default browser
        RegOpenKeyEx(HKEY_CLASSES_ROOT,".htm",0,KEY_READ,&Key);
    }

    if(RegQueryValueEx(Key,"",NULL,NULL,(BYTE*)&strBrowser,&dwSize)==ERROR_SUCCESS)
    {
        RegCloseKey(Key);
        strcat(strBrowser, "\\shell\\open\\command");
        if(RegOpenKeyEx(HKEY_CLASSES_ROOT,strBrowser,0,KEY_READ,&Key)==ERROR_SUCCESS)
        {
            dwSize=2*MAX_PATH;
            if(RegQueryValueEx(Key,"",NULL,NULL,(BYTE*)&strBrowser,&dwSize)==ERROR_SUCCESS)
            {
                int iPos = -1;
                char* strNotNeeded = strstr(strBrowser,"\"%");
                if (strNotNeeded != NULL)
                {
                    iPos = (int)(strNotNeeded - strBrowser );
                }
                if (iPos > 0 )
                {
                    strBrowser[iPos-1] = 0;
                }
                strcpy(strDefaultApp, strBrowser);
            }
        }
    }
    RegCloseKey(Key);
}

// main sample harness -- written as linear sequence for readability
int32_t SampleCode(const char *pConfig)
{
    int32_t iError;
    int32_t iIndex;
    int32_t iButton;
    char *pConv;
    HWND pHandle;
    int32_t iSubmit;
    int32_t iIDCRef;
    int32_t BtnList[8];
    WebOfferT *pOffer;
    WebOfferSetupT Setup;
    WebOfferCommandT *pCommand;
    char strTemp[64];
    char strScript[512 * 1024];
    int32_t iScriptLen;
    char *pScriptBuf;

    /////// THIS SECTION IS SETUP ONLY -- REAL PRODUCTS GET THIS DATA ELSEWHERE ////////

    // collect user information which normally comes from application
    pHandle = DialogCreate("ADDRESS");

    // initialize default values
    GetPrivateProfileString("Main", "PERSONA", "", Setup.strPersona, sizeof(Setup.strPersona), pConfig);
    GetPrivateProfileString("Main", "FAVTEAM", "", Setup.strFavTeam, sizeof(Setup.strFavTeam), pConfig);
    GetPrivateProfileString("Main", "PRODUCT", "", Setup.strProduct, sizeof(Setup.strProduct), pConfig);
    GetPrivateProfileString("Main", "PLATFORM", "", Setup.strPlatform, sizeof(Setup.strPlatform), pConfig);
    GetPrivateProfileString("Main", "LANGUAGE", "", Setup.strLanguage, sizeof(Setup.strLanguage), pConfig);
    GetPrivateProfileString("Main", "SLUSCODE", "", Setup.strSlusCode, sizeof(Setup.strSlusCode), pConfig);
    GetPrivateProfileString("Main", "SKUCODE", "", Setup.strSkuCode, sizeof(Setup.strSkuCode), pConfig);
    GetPrivateProfileString("Main", "LKEY", "", Setup.strLKey, sizeof(Setup.strLKey), pConfig);
    GetPrivateProfileString("Main", "REGION", "", Setup.strGameRegion, sizeof(Setup.strGameRegion), pConfig);
    GetPrivateProfileString("Main", "FAVPLAYER", "", Setup.strFavPlyr, sizeof(Setup.strFavPlyr), pConfig);
    GetPrivateProfileString("Main", "PRIVILIGIES", "", Setup.strPrivileges, sizeof(Setup.strPrivileges), pConfig);
    Setup.uMemSize = WEBOFFER_SUGGESTED_MEM;
    GetPrivateProfileString("Main", "MEMORY", "", strTemp, sizeof(strTemp), pConfig);
    if(strlen(strTemp) > 0)
    {
        Setup.uMemSize = atoi(strTemp);
    }

    // clear the script to start with
    memset(strScript, 0, sizeof(strScript));
    // first try to use the input filename to load the script
    if(strlen(g_strInputFile) > 0)
    {
        // we have a filename - try to open the file and use as a script
        FILE *fp = fopen(g_strInputFile,"r");
        char *pScript = strScript;
        char cBuf;
        // if we successfully opened the file, read data and then close the file
        while(fp)
        {
            // walk through the entire file and process EOL.
            cBuf = fgetc(fp);
            // check for completion
            if(feof(fp))
            {
                fclose(fp);
                break;
            }
            // otherwise translate the character
            if(cBuf == '\n')
            {
                (*pScript++) = '\r';
                (*pScript++) = '\n';
            }
            else if((cBuf != '\r') && (cBuf != '\0'))
            {
                (*pScript++) = cBuf;
            }
            // check to make sure we haven't run out of room
            if(pScript - strScript >= sizeof(strScript))
            {
                // script too large to handle!
                fclose(fp);
                MessageBox(NULL, "Input Script Too Large!", "Unable to Read", MB_OK);
                return(-1);
            }
        }
    }

    // if we still don't have a script to run, try to get it from the config file
    if(strlen(strScript) == 0)
    {
        // load default script and do newline conversion
        GetPrivateProfileString("Main", "SCRIPT", "", strScript, sizeof(strScript), pConfig);
    }
    // convert special newlines if necessary
    for (pConv = strScript; *pConv != 0; ++pConv)
    {
        if ((pConv[0] == '%') && (pConv[1] == '\\'))
        {
            pConv[0] = 13;
            pConv[1] = 10;
        }
    }

    // show setup dialog
    SetDlgItemText(pHandle, IDC_PERSONA, Setup.strPersona);
    SetDlgItemText(pHandle, IDC_FAVTEAM, Setup.strFavTeam);
    SetDlgItemText(pHandle, IDC_PRODUCT, Setup.strProduct);
    SetDlgItemText(pHandle, IDC_PLATFORM, Setup.strPlatform);
    SetDlgItemText(pHandle, IDC_LANGUAGE, Setup.strLanguage);
    SetDlgItemText(pHandle, IDC_GAME_REGION, Setup.strGameRegion);
    SetDlgItemText(pHandle, IDC_SLUSCODE, Setup.strSlusCode);
    SetDlgItemText(pHandle, IDC_SKUCODE, Setup.strSkuCode);
    SetDlgItemText(pHandle, IDC_LKEY, Setup.strLKey);
    SetDlgItemText(pHandle, IDC_PRIVILIGIES, Setup.strPrivileges);
    SetDlgItemText(pHandle, IDC_FAVPLAYER, Setup.strFavPlyr);
    _itoa_s(Setup.uMemSize, strTemp, sizeof(strTemp),  10);
    SetDlgItemText(pHandle, IDC_MEMORY, strTemp);
    SetDlgItemText(pHandle, IDC_SCRIPT, strScript);

    // let user click buttons
    iButton = DialogWaitButton(pHandle);
    if (iButton == IDC_QUIT)
    {
        DialogQuit();
        return(0);
    }

    // get the user choices
    GetDlgItemText(pHandle, IDC_PERSONA, Setup.strPersona, sizeof(Setup.strPersona));
    GetDlgItemText(pHandle, IDC_FAVTEAM, Setup.strFavTeam, sizeof(Setup.strFavTeam));
    GetDlgItemText(pHandle, IDC_PRODUCT, Setup.strProduct, sizeof(Setup.strProduct));
    GetDlgItemText(pHandle, IDC_PLATFORM, Setup.strPlatform, sizeof(Setup.strPlatform));
    GetDlgItemText(pHandle, IDC_LANGUAGE, Setup.strLanguage, sizeof(Setup.strLanguage));
    GetDlgItemText(pHandle, IDC_GAME_REGION, Setup.strGameRegion, sizeof(Setup.strGameRegion));
    GetDlgItemText(pHandle, IDC_SLUSCODE, Setup.strSlusCode, sizeof(Setup.strSlusCode));
    GetDlgItemText(pHandle, IDC_SKUCODE, Setup.strSkuCode, sizeof(Setup.strSkuCode));
    GetDlgItemText(pHandle, IDC_LKEY, Setup.strLKey, sizeof(Setup.strLKey));
    GetDlgItemText(pHandle, IDC_PRIVILIGIES, Setup.strPrivileges, sizeof(Setup.strPrivileges));
    GetDlgItemText(pHandle, IDC_FAVPLAYER, Setup.strFavPlyr, sizeof(Setup.strFavPlyr));
    GetDlgItemText(pHandle, IDC_MEMORY, strTemp, sizeof(strTemp));
    Setup.uMemSize = atoi(strTemp);
    GetDlgItemText(pHandle, IDC_SCRIPT, strScript, sizeof(strScript));

    // save the default parms for next time
    WritePrivateProfileString("Main", "PERSONA", Setup.strPersona, pConfig);
    WritePrivateProfileString("Main", "FAVTEAM", Setup.strFavTeam, pConfig);
    WritePrivateProfileString("Main", "PRODUCT", Setup.strProduct, pConfig);
    WritePrivateProfileString("Main", "PLATFORM", Setup.strPlatform, pConfig);
    WritePrivateProfileString("Main", "LANGUAGE", Setup.strLanguage, pConfig);
    WritePrivateProfileString("Main", "REGION", Setup.strGameRegion, pConfig);
    WritePrivateProfileString("Main", "SLUSCODE", Setup.strSlusCode, pConfig);
    WritePrivateProfileString("Main", "SKUCODE", Setup.strSkuCode, pConfig);
    WritePrivateProfileString("Main", "LKEY", Setup.strLKey, pConfig);
    WritePrivateProfileString("Main", "PRIVILIGIES", Setup.strPrivileges, pConfig);
    WritePrivateProfileString("Main", "FAVPLAYER", Setup.strFavPlyr, pConfig);
    WritePrivateProfileString("Main", "MEMORY", strTemp, pConfig);

    // save script special by encoding newlines
    for (pConv = strScript; *pConv != 0; ++pConv)
    {
        if ((pConv[0] == 13) && (pConv[1] == 10))
        {
            pConv[0] = '%';
            pConv[1] = '\\';
        }
    }
    WritePrivateProfileString("Main", "SCRIPT", strScript, pConfig);

    // restore so we can use below
    for (pConv = strScript; *pConv != 0; ++pConv)
    {
        if ((pConv[0] == '%') && (pConv[1] == '\\'))
        {
            pConv[0] = 13;
            pConv[1] = 10;
        }
    }

    // close the window
    DialogDestroy(pHandle);

    // hack load a local file for debugging purposes
    if (strncmp(strScript, "file://", 7) == 0)
    {
        int32_t len;
        FILE *fd = fopen(strScript+7, "rb");
        if (fd == NULL)
        {
            MessageBox(NULL, strScript+7, "Unable to Read", MB_OK);
            DialogQuit();
            return(0);
        }
        len = fread(strScript, 1, sizeof(strScript)-1, fd);
        if (len < 0)
            len = 0;
        strScript[len] = 0;
        fclose(fd);
    }

    // handle direct web reference to make it fast for demo
    // (don't copy this code to consoles -- it bypasses minimum display times console must honor)
    if ((strncmp(strScript, "http://", 7) == 0) || (strncmp(strScript, "https://", 8) == 0))
    {
        strcpy(strScript+1024, strScript);
        sprintf(strScript, "%%{ CMD=http DISP=0 SUCCESS-GOTO=\"$home\" URL-GET=%s\n%%}", strScript+1024);
    }

    /////// THIS IS THE SAMPLE GUTS -- PRODUCT MUST IMPLEMENT THIS LOGIC ////////

    // do external buffer management since buffer is so large
    iScriptLen = Setup.uMemSize;
    if (iScriptLen < 8192)
        iScriptLen = 8192;
    pScriptBuf = malloc(iScriptLen);

    // startp the module
    pOffer = WebOfferCreate(pScriptBuf, iScriptLen);

    // provide user information
    WebOfferSetup(pOffer, &Setup);

    // execute the script
    WebOfferExecute(pOffer, strScript);

    // process all the commands
    while ((pCommand = WebOfferCommand(pOffer)) != NULL)
    {
        // diagnostic to store off current script for analysis
        if (1)
        {
            // don't try at home -- this is not valid for production code
            // tricky way to get at internal script buffer
            char *pScript = *((char **)pScriptBuf);
            _ExportScript("script.txt", pScript);
        }

        // do a web transfer
        if (pCommand->iCommand == WEBOFFER_CMD_HTTP)
        {
            WebOfferBusyT Busy;
            WebOfferGetBusy(pOffer, &Busy);

            // tell user to wait while we send the response
            pHandle = DialogCreate("BUSY");
            Utf8TranslateTo8Bit(Busy.strMessage, 9999, Busy.strMessage, '*', _Utf8ToWindows);
            SetDlgItemText(pHandle, IDC_WAITMSG, Busy.strMessage);
            if (Busy.MsgColor.uAlpha != 0)
            {
                DialogColor(pHandle, IDC_WAITMSG, RGB(Busy.MsgColor.uRed, Busy.MsgColor.uGreen, Busy.MsgColor.uBlue));
            }
            if (Busy.Button[0].strText[0] != 0)
            {
                Utf8TranslateTo8Bit(Busy.Button[0].strText, 9999, Busy.Button[0].strText, '*', _Utf8ToWindows);
                SetDlgItemText(pHandle, IDC_BTN1, Busy.Button[0].strText);
                ShowWindow(GetDlgItem(pHandle, IDC_BTN1), SW_SHOW);
            }
            if (Busy.Button[0].Color.uAlpha != 0)
            {
                DialogColor(pHandle, IDCANCEL, RGB(Busy.Button[0].Color.uRed,
                    Busy.Button[0].Color.uGreen, Busy.Button[0].Color.uBlue));
            }
            ShowWindow(pHandle, SW_SHOW);

            // send the data
            WebOfferHttp(pOffer);
            for (;;)
            {
                // check for completion/error
                iError = WebOfferHttpComplete(pOffer);
                // check for button click
                iButton = _MapIDCToAction(DialogCheckButton(pHandle));
                if ((iError != 0) || (iButton != 0))
                    break;
                // kill some cycles while we wait
                Sleep(50);
            }

            // if user clicked button, pass to action handler
            if (iButton != 0)
                WebOfferAction(pOffer, iButton);

            // remove the busy dialog
            DialogDestroy(pHandle);
        }

        // show an alert dialog
        if (pCommand->iCommand == WEBOFFER_CMD_ALERT)
        {
            WebOfferAlertT Alert;
            WebOfferGetAlert(pOffer, &Alert);

            // create the dialog
            pHandle = DialogCreate(Alert.strImage[0] ? "ALERT-IMAGE" : "ALERT");
            Utf8TranslateTo8Bit(Alert.strTitle, 9999, Alert.strTitle, '*', _Utf8ToWindows);
            SetWindowText(pHandle, Alert.strTitle);
            Utf8TranslateTo8Bit(Alert.strMessage, 9999, Alert.strMessage, '*', _Utf8ToWindows);
            SetDlgItemText(pHandle, IDC_ALERTTXT, Alert.strMessage);
            if (Alert.MsgColor.uAlpha != 0)
            {
                DialogColor(pHandle, IDC_ALERTTXT, RGB(Alert.MsgColor.uRed, Alert.MsgColor.uGreen, Alert.MsgColor.uBlue));
            }

            // setup image if present
            if (Alert.strImage[0] != 0)
            {
                _SetupImage(pHandle, IDC_ALERTIMG, pOffer, Alert.strImage);
            }

            // set & arrange buttons
            iButton = 0;
            for (iIndex = 0; iIndex < 4; ++iIndex)
            {
                // get the IDC to use - remember it takes in buttons 1-4, not 0-3
                iIDCRef = _MapButtonToIDC(iIndex+1);

                // check to see if the button exists
                if (Alert.Button[iIndex].strText[0] != 0)
                {
                    // this button exists
                    Utf8TranslateTo8Bit(Alert.Button[iIndex].strText, 9999, Alert.Button[iIndex].strText, '*', _Utf8ToWindows);
                    SetDlgItemText(pHandle, iIDCRef, Alert.Button[iIndex].strText);
                    ShowWindow(GetDlgItem(pHandle, iIDCRef), SW_SHOW);
                    BtnList[iButton++] = iIDCRef;

                    // assign a color to the dialog button if it has one
                    if (Alert.Button[iIndex].Color.uAlpha != 0)
                    {
                        DialogColor(pHandle, iIDCRef, RGB(Alert.Button[iIndex].Color.uRed,
                            Alert.Button[iIndex].Color.uGreen,
                            Alert.Button[iIndex].Color.uBlue));
                    }
                }
            }
            DialogButtonCenter(pHandle, BtnList, iButton);

            // get button input
            iButton = _MapIDCToAction(DialogWaitButton(pHandle));
            WebOfferAction(pOffer, iButton);

            // close the dialog
            DialogDestroy(pHandle);
        }

        // show the credit card screen
        if (pCommand->iCommand == WEBOFFER_CMD_CREDIT)
        {
            WebOfferCreditT Credit;
            WebOfferGetCredit(pOffer, &Credit);

            pHandle = DialogCreate("CREDITCARD");
            Utf8TranslateTo8Bit(Credit.strTitle, 9999, Credit.strTitle, '*', _Utf8ToWindows);
            SetWindowText(pHandle, Credit.strTitle);
            Utf8TranslateTo8Bit(Credit.strMessage, 9999, Credit.strMessage, '*', _Utf8ToWindows);
            SetDlgItemText(pHandle, IDC_OFFERTXT, Credit.strMessage);
            if (Credit.MsgColor.uAlpha != 0)
            {
                DialogColor(pHandle, IDC_OFFERTXT, RGB(Credit.MsgColor.uRed, Credit.MsgColor.uGreen, Credit.MsgColor.uBlue));
            }

            SetDlgItemText(pHandle, IDC_FIRSTNAME, Credit.strFirstName);
            SetDlgItemText(pHandle, IDC_LASTNAME, Credit.strLastName);
            SetDlgItemText(pHandle, IDC_ADDRESS1, Credit.strAddress[0]);
            SetDlgItemText(pHandle, IDC_ADDRESS2, Credit.strAddress[1]);
            SetDlgItemText(pHandle, IDC_CITY, Credit.strCity);
            SetDlgItemText(pHandle, IDC_STATE, Credit.strState);
            SetDlgItemText(pHandle, IDC_ZIPCODE, Credit.strPostCode);

            // populate country list
            DialogComboInit(pHandle, IDC_COUNTRY, "");
            for (iIndex = 0; WebOfferParamList(pOffer, strTemp, sizeof(strTemp), Credit.pCountryList, iIndex) >= 0; ++iIndex)
            {
                DialogComboAdd(pHandle, IDC_COUNTRY, strTemp);
            }
            DialogComboSelect(pHandle, IDC_COUNTRY, Credit.strCountry);

            // populate credit card list
            DialogComboInit(pHandle, IDC_CCTYPE, "");
            for (iIndex = 0; WebOfferParamList(pOffer, strTemp, sizeof(strTemp), Credit.pCardList, iIndex) >= 0; ++iIndex)
            {
                DialogComboAdd(pHandle, IDC_CCTYPE, strTemp);
            }
            DialogComboSelect(pHandle, IDC_CCTYPE, Credit.strCardType);

            // reset of credit card data
            SetDlgItemText(pHandle, IDC_CCNUMBER, Credit.strCardNumber);
            DialogComboInit(pHandle, IDC_CCMONTH, "01\0""02\0""03\0""04\0""05\0""06\0""07\0""08\0""09\0""10\0""11\0""12\0");
            if (Credit.iExpiryMonth > 0)
                SendDlgItemMessage(pHandle, IDC_CCMONTH, CB_SETCURSEL, (WPARAM)(Credit.iExpiryMonth-1), 0);

            // populate the years combo box
            DialogComboInit(pHandle, IDC_CCYEAR, "");
            for (iIndex = Credit.iYearEndExpiry - Credit.iYearStartExpiry ; iIndex >= 0; --iIndex)
            {
                sprintf(strTemp, "%d", Credit.iYearStartExpiry + iIndex);
                DialogComboAdd(pHandle, IDC_CCYEAR, strTemp);
            }
            DialogComboSelect(pHandle, IDC_CCYEAR, strTemp);

            SetDlgItemText(pHandle, IDC_EMAIL, Credit.strEmail);

            // set & arrange buttons
            iButton = 0;
            for (iIndex = 0; iIndex < 4; ++iIndex)
            {
                // get the IDC to use - remember it takes in buttons 1-4, not 0-3
                iIDCRef = _MapButtonToIDC(iIndex+1);

                if (Credit.Button[iIndex].strText[0] != 0)
                {
                    // this button exists
                    Utf8TranslateTo8Bit(Credit.Button[iIndex].strText, 9999, Credit.Button[iIndex].strText, '*', _Utf8ToWindows);
                    SetDlgItemText(pHandle, iIDCRef, Credit.Button[iIndex].strText);
                    ShowWindow(GetDlgItem(pHandle, iIDCRef), SW_SHOW);
                    BtnList[iButton++] = iIDCRef;

                    // assign a color to the dialog button if it has one
                    if (Credit.Button[iIndex].Color.uAlpha != 0)
                    {
                        DialogColor(pHandle, iIDCRef, RGB(  Credit.Button[iIndex].Color.uRed,
                            Credit.Button[iIndex].Color.uGreen,
                            Credit.Button[iIndex].Color.uBlue));
                    }
                }
            }
            DialogButtonCenter(pHandle, BtnList, iButton);

            // let user click buttons
            iButton = _MapIDCToAction(DialogWaitButton(pHandle));

            // determine if we should submit data based on button press
            iSubmit = 0;
            if(Credit.Button[iButton-1].strType[0] == WEBOFFER_TYPE_SUBMIT)
                iSubmit=1;

            // handle data submit
            if (iSubmit)
            {
                // store updated information in structure
                GetDlgItemText(pHandle, IDC_FIRSTNAME, Credit.strFirstName, sizeof(Credit.strFirstName));
                GetDlgItemText(pHandle, IDC_LASTNAME, Credit.strLastName, sizeof(Credit.strLastName));
                GetDlgItemText(pHandle, IDC_ADDRESS1, Credit.strAddress[0], sizeof(Credit.strAddress[0]));
                GetDlgItemText(pHandle, IDC_ADDRESS2, Credit.strAddress[1], sizeof(Credit.strAddress[1]));
                GetDlgItemText(pHandle, IDC_CITY, Credit.strCity, sizeof(Credit.strCity));
                GetDlgItemText(pHandle, IDC_STATE, Credit.strState, sizeof(Credit.strState));
                GetDlgItemText(pHandle, IDC_ZIPCODE, Credit.strPostCode, sizeof(Credit.strPostCode));
                GetDlgItemText(pHandle, IDC_COUNTRY, Credit.strCountry, sizeof(Credit.strCountry));
                GetDlgItemText(pHandle, IDC_CCTYPE, Credit.strCardType, sizeof(Credit.strCardType));
                GetDlgItemText(pHandle, IDC_CCNUMBER, Credit.strCardNumber, sizeof(Credit.strCardNumber));
                GetDlgItemText(pHandle, IDC_CCMONTH, strTemp, sizeof(strTemp));
                Credit.iExpiryMonth = atoi(strTemp);
                GetDlgItemText(pHandle, IDC_CCYEAR, strTemp, sizeof(strTemp));
                Credit.iExpiryYear = atoi(strTemp);
                GetDlgItemText(pHandle, IDC_EMAIL, Credit.strEmail, sizeof(Credit.strEmail));
                WebOfferSetCredit(pOffer, &Credit);
            }

            // pass on the action
            WebOfferAction(pOffer, iButton);
            DialogDestroy(pHandle);
        }

        // show a promo screen
        if (pCommand->iCommand == WEBOFFER_CMD_PROMO)
        {
            WebOfferPromoT Promo;
            WebOfferGetPromo(pOffer, &Promo);

            pHandle = DialogCreate("PROMO");
            Utf8TranslateTo8Bit(Promo.strTitle, 9999, Promo.strTitle, '*', _Utf8ToWindows);
            SetWindowText(pHandle, Promo.strTitle);
            Utf8TranslateTo8Bit(Promo.strMessage, 9999, Promo.strMessage, '*', _Utf8ToWindows);
            SetDlgItemText(pHandle, IDC_PROMOTXT, Promo.strMessage);
            if (Promo.MsgColor.uAlpha != 0)
            {
                DialogColor(pHandle, IDC_PROMOTXT, RGB(Promo.MsgColor.uRed, Promo.MsgColor.uGreen, Promo.MsgColor.uBlue));
            }
            SetDlgItemText(pHandle, IDC_PROMO, Promo.strPromo);

            // set & arrange buttons
            iButton = 0;
            for (iIndex = 0; iIndex < 4; ++iIndex)
            {
                // get the IDC to use - remember it takes in buttons 1-4, not 0-3
                iIDCRef = _MapButtonToIDC(iIndex+1);

                if (Promo.Button[iIndex].strText[0] != 0)
                {
                    // this button exists
                    SetDlgItemText(pHandle, iIDCRef, Promo.Button[iIndex].strText);
                    ShowWindow(GetDlgItem(pHandle, iIDCRef), SW_SHOW);
                    BtnList[iButton++] = iIDCRef;

                    // assign a color to the dialog button if it has one
                    if (Promo.Button[iIndex].Color.uAlpha != 0)
                    {
                        DialogColor(pHandle, iIDCRef, RGB(  Promo.Button[iIndex].Color.uRed,
                            Promo.Button[iIndex].Color.uGreen,
                            Promo.Button[iIndex].Color.uBlue));
                    }
                }

            }
            DialogButtonCenter(pHandle, BtnList, iButton);

            // let user click buttons
            iButton = _MapIDCToAction(DialogWaitButton(pHandle));

            // determine if we should submit data based on button press
            iSubmit = 0;
            if ((iButton > 0) && (Promo.Button[iButton-1].strType[0] == WEBOFFER_TYPE_SUBMIT))
                iSubmit = 1;

            // submit data back to script
            if (iSubmit)
            {
                // store updated information in structure
                GetDlgItemText(pHandle, IDC_PROMO, Promo.strPromo, sizeof(Promo.strPromo));
                WebOfferSetPromo(pOffer, &Promo);
            }

            // pass on the action
            WebOfferAction(pOffer, iButton);
            DialogDestroy(pHandle);
        }

        // show a dummy marketplace screen
        if (pCommand->iCommand == WEBOFFER_CMD_MARKET)
        {
            WebOfferMarketplaceT Market;
            WebOfferGetMarketplace(pOffer, &Market);

            pHandle = DialogCreate("MARKETPLACE");

            // let user click buttons
            iButton = _MapIDCToAction(DialogWaitButton(pHandle));

            // pass on the action
            WebOfferAction(pOffer, iButton);
            DialogDestroy(pHandle);
        }

        // show the news screen
        if (pCommand->iCommand == WEBOFFER_CMD_NEWS)
        {
            WebOfferNewsT News;
            char *pFormat, *pCopy;
            const char *pNews = WebOfferGetNews(pOffer, &News);

            // setup the main dialog
            pHandle = DialogCreate(News.strImage[0] ? "NEWS-IMAGE" : "NEWS");
            Utf8TranslateTo8Bit(News.strTitle, 9999, News.strTitle, '*', _Utf8ToWindows);
            SetWindowText(pHandle, News.strTitle);
            DialogColor(pHandle, IDC_NEWSTXT, RGB(News.NewsColor.uRed, News.NewsColor.uGreen,News.NewsColor.uBlue));

            // setup image if present
            if (News.strImage[0] != 0)
            {
                _SetupImage(pHandle, IDC_NEWSIMG, pOffer, News.strImage);
            }

            // build a cr/lf version of the message (needed by windows)
            iIndex = strlen(pNews)*2+1;
            pFormat = malloc(iIndex);
            for (pCopy = pFormat; *pNews != 0; ++pNews)
            {
                if (*pNews == '\n')
                    *pCopy++ = '\r';
                *pCopy++ = *pNews;
            }
            *pCopy = 0;
            Utf8TranslateTo8Bit(pFormat, iIndex, pFormat, '*', _Utf8ToWindows);
            SetDlgItemText(pHandle, IDC_NEWSTXT, pFormat);

            // set & arrange buttons
            iButton = 0;
            for (iIndex = 0; iIndex < 4; ++iIndex)
            {
                // get the IDC to use - remember it takes in buttons 1-4, not 0-3
                iIDCRef = _MapButtonToIDC(iIndex+1);

                if (News.Button[iIndex].strText[0] != 0)
                {
                    // this button exists
                    Utf8TranslateTo8Bit(News.Button[iIndex].strText, 9999, News.Button[iIndex].strText, '*', _Utf8ToWindows);
                    SetDlgItemText(pHandle, iIDCRef, News.Button[iIndex].strText);
                    ShowWindow(GetDlgItem(pHandle, iIDCRef), SW_SHOW);
                    BtnList[iButton++] = iIDCRef;

                    // assign a color to the dialog button if it has one
                    if (News.Button[iIndex].Color.uAlpha != 0)
                    {
                        DialogColor(pHandle, iIDCRef, RGB(  News.Button[iIndex].Color.uRed,
                            News.Button[iIndex].Color.uGreen,
                            News.Button[iIndex].Color.uBlue));
                    }
                }

            }
            DialogButtonCenter(pHandle, BtnList, iButton);

            // let user click buttons
            iButton = _MapIDCToAction(DialogWaitButton(pHandle));
            WebOfferAction(pOffer, iButton);
            DialogDestroy(pHandle);

        }
        // show the menu screen
        if (pCommand->iCommand == WEBOFFER_CMD_MENU)
        {
            WebOfferMenuT Menu;

            // get the menu
            WebOfferGetMenu(pOffer, &Menu);

            // setup the main dialog
            pHandle = DialogCreate("MENU");
            Utf8TranslateTo8Bit(Menu.strTitle, 9999, Menu.strTitle, '*', _Utf8ToWindows);
            SetWindowText(pHandle, Menu.strTitle);

            // set & arrange buttons
            for(iButton = 0; iButton < 8; iButton++)
            {
                // get the IDC to use - remember it takes in buttons 1-4, not 0-3
                iIDCRef = _MapButtonToIDC(iButton+1);

                if (Menu.Button[iButton].strText[0] != 0)
                {
                    // this button exists
                    Utf8TranslateTo8Bit(Menu.Button[iButton].strText, 9999, Menu.Button[iButton].strText, '*', _Utf8ToWindows);
                    SetDlgItemText(pHandle, iIDCRef, Menu.Button[iButton].strText);
                    ShowWindow(GetDlgItem(pHandle, iIDCRef), SW_SHOW);

                    // assign a color to the dialog button if it has one
                    if (Menu.Button[iButton].Color.uAlpha != 0)
                    {
                        DialogColor(pHandle, iIDCRef, RGB(  Menu.Button[iButton].Color.uRed,
                            Menu.Button[iButton].Color.uGreen,
                            Menu.Button[iButton].Color.uBlue));
                    }
                }

            }

            // let user click buttons
            iButton = _MapIDCToAction(DialogWaitButton(pHandle));
            WebOfferAction(pOffer, iButton);
            DialogDestroy(pHandle);
        }
        // show the articles screen
        if (pCommand->iCommand == WEBOFFER_CMD_ARTICLES)
        {
            WebOfferArticlesT articles;
            WebOfferGetArticles(pOffer, &articles);

            // setup the main dialog
            pHandle = DialogCreate("ARTICLES");
            Utf8TranslateTo8Bit(articles.strTitle, 9999, articles.strTitle, '*', _Utf8ToWindows);
            SetWindowText(pHandle, articles.strTitle);

            // set & arrange articles
            for(iButton = 0; iButton < 16; iButton++)
            {
                // get the IDC to use - remember it takes in buttons 1-4, not 0-3
                iIDCRef = _MapArticleToIDC(iButton+1);

                // set and arrange Articles
                if (articles.Article[iButton].strArticle[0] != 0)
                {
                    // this Articles exists
                    Utf8TranslateTo8Bit(articles.Article[iButton].strArticle, 9999, articles.Article[iButton].strArticle, '*', _Utf8ToWindows);

                    if (articles.Article[iButton].strImage[0] != 0)
                    {
                        // set up the image for the image next to the Articles
                        _SetupImage(pHandle, _MapArticleImageToIDC(iButton+1), pOffer, articles.Article[iButton].strImage);
                        ShowWindow(GetDlgItem(pHandle, _MapArticleImageToIDC(iButton+1)), SW_SHOW);
                    }
                    SetDlgItemText(pHandle, iIDCRef, articles.Article[iButton].strArticle);
                    SetDlgItemText(pHandle,_MapArticleTypeToIDC(iButton+1),articles.Article[iButton].strType);
                    SetDlgItemText(pHandle,_MapArticleDescToIDC(iButton+1),articles.Article[iButton].strDesc);
                    ShowWindow(GetDlgItem(pHandle, iIDCRef), SW_SHOW);
                    ShowWindow(GetDlgItem(pHandle, _MapArticleTypeToIDC(iButton+1)), SW_SHOW);
                    ShowWindow(GetDlgItem(pHandle, _MapArticleDescToIDC(iButton+1)), SW_SHOW);

                }

            }

            // set & arrange buttons
            for(iButton = 0; iButton < 4; iButton++)
            {
                // get the IDC to use - remember it takes in buttons 1-4, not 0-3
                iIDCRef = _MapButtonToIDC(iButton+1);
                ShowWindow(GetDlgItem(pHandle, iIDCRef), SW_HIDE);
                if (articles.Button[iButton].strText[0] != 0)
                {
                    Utf8TranslateTo8Bit(articles.Button[iButton].strText, 9999, articles.Button[iButton].strText, '*', _Utf8ToWindows);
                    SetDlgItemText(pHandle, iIDCRef, articles.Button[iButton].strText);
                    ShowWindow(GetDlgItem(pHandle, iIDCRef), SW_SHOW);

                    // assign a color to the dialog button if it has one
                    if (articles.Button[iButton].Color.uAlpha != 0)
                    {
                        DialogColor(pHandle, iIDCRef, RGB(  articles.Button[iButton].Color.uRed,
                            articles.Button[iButton].Color.uGreen,
                            articles.Button[iButton].Color.uBlue));
                    }
                }

            }


            iButton = _MapIDCToAction(DialogWaitButton(pHandle));
            WebOfferAction(pOffer, iButton);
            DialogDestroy(pHandle);
        }
        // show the story screen
        if (pCommand->iCommand == WEBOFFER_CMD_STORY)
        {
            WebOfferStoryT story;
            WebOfferGetStory(pOffer, &story);

            // setup the main dialog
            pHandle = DialogCreate("STORY");
            SetWindowText(pHandle, story.strTitle);
            // get the IDC to use - remember it takes in buttons 1-4, not 0-3
            iIDCRef = _MapButtonToIDC(1);

            if (story.pText != 0 && story.strTitle[0] != 0)
            {
                // this button exists
                SetDlgItemText(pHandle, 1185, story.pText);
                ShowWindow(GetDlgItem(pHandle, iIDCRef), SW_SHOW);

                // setup image if present
                if (story.strImage[0] != 0)
                {
                    _SetupImage(pHandle, IDC_STROYIMG, pOffer, story.strImage);
                }

                iButton = _MapIDCToAction(DialogWaitButton(pHandle));
                WebOfferAction(pOffer, iButton);
            }

        }
        // show the media screen
        if (pCommand->iCommand == WEBOFFER_CMD_MEDIA)
        {
            WebOfferMediaT media;
            WebOfferGetMedia(pOffer, &media);

            // setup the main dialog
            pHandle = DialogCreate("MENU");
            Utf8TranslateTo8Bit(media.strTitle, 9999, media.strTitle, '*', _Utf8ToWindows);
            SetWindowText(pHandle, media.strTitle);

            if(media.pMedia[0] != 0)
            {
                // find the extension of the file
                char strActualExtention[5];
                char strDefaultApp[256];
                STARTUPINFO si;
                PROCESS_INFORMATION pi;
                char strCmdLineBuffer[1024];
                char* strExtention = strrchr(media.pMedia , (int)'.');
                memset(strActualExtention, 0, sizeof(strActualExtention));
                ds_strnzcpy(strActualExtention, strExtention, 4);
                if(!IsCharAlphaNumeric(strActualExtention[4]))
                {
                    strActualExtention[4] = 0;
                }
                strExtention = strActualExtention;
                ZeroMemory( &si, sizeof(si) );
                si.cb = sizeof(si);
                ZeroMemory( &pi, sizeof(pi) );

                _GetDefaultApp(strDefaultApp, strExtention);

                sprintf(strCmdLineBuffer,"%s %s",strDefaultApp, media.pMedia);
                // Start the child process. which plays the media
                if( !CreateProcess( NULL,   // No module name (use command line).
                    strCmdLineBuffer, // Command line.
                    NULL,             // Process handle not inheritable.
                    NULL,             // Thread handle not inheritable.
                    FALSE,            // Set handle inheritance to FALSE.
                    0,                // No creation flags.
                    NULL,             // Use parent's environment block.
                    NULL,             // Use parent's starting directory.
                    &si,              // Pointer to STARTUPINFO structure.
                    &pi )             // Pointer to PROCESS_INFORMATION structure.
                    )
                {
                    char strLastError[500];
                    LPVOID lpMsgBuf;
                    DWORD dwErrorCode;
                    memset(strLastError, 0 , sizeof(strLastError));
                    dwErrorCode = GetLastError();
                    FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        dwErrorCode,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (LPTSTR) &lpMsgBuf,
                        0, NULL );
                    sprintf(strLastError, "calling %s failed with error code %d: %s", strDefaultApp, dwErrorCode, lpMsgBuf);
                    LocalFree(lpMsgBuf);
                    SetWindowText(pHandle, strLastError);
                }
                else
                {
                    // Wait until finishes playing the media.
                    WaitForSingleObject( pi.hProcess, INFINITE );

                    // Close process and thread handles.
                    CloseHandle( pi.hProcess );
                    CloseHandle( pi.hThread );
                }
            }
            WebOfferAction(pOffer, 1);
            DialogDestroy(pHandle);
        }
        if (pCommand->iCommand == WEBOFFER_CMD_FORM)
        {
            WebOfferFormT Form;
            BOOL bValid = FALSE;
            WebOfferGetForm(pOffer, &Form);
            pHandle = DialogCreate("FORM");
            Utf8TranslateTo8Bit(Form.strTitle, 9999, Form.strTitle, '*', _Utf8ToWindows);
            SetWindowText(pHandle, Form.strTitle);


            for (iIndex = 0; iIndex < Form.iNumFields; iIndex++)
            {
                ShowWindow(GetDlgItem(pHandle, _MapNameToIDC(iIndex+1)), SW_HIDE);
                ShowWindow(GetDlgItem(pHandle, _MapComboToIDC(iIndex+1)), SW_HIDE);
                ShowWindow(GetDlgItem(pHandle, _MapEditToIDC(iIndex+1)), SW_HIDE);
                if(Form.Fields[iIndex].strName[0] != 0)
                {
                    SetWindowText(GetDlgItem(pHandle, _MapNameToIDC(iIndex + 1)), Form.Fields[iIndex].strName);
                    ShowWindow(GetDlgItem(pHandle, _MapNameToIDC(iIndex + 1)), SW_SHOW);

                    // Color mandatory fields in red and not mandatory in blackB
                    if (Form.Fields[iIndex].bMandatory == 1)
                    {
                        DialogColor(pHandle, _MapNameToIDC(iIndex + 1),COLOR_MANDATORY);
                    }
                    else
                    {
                        DialogColor(pHandle, _MapNameToIDC(iIndex + 1),COLOR_NOTMANDATORY);
                    }

                    if (Form.Fields[iIndex].iStyle != WEB_OFFER_FORM_LIST)
                    {
                        ShowWindow(GetDlgItem(pHandle, _MapEditToIDC(iIndex + 1)), SW_SHOW);
                        SetWindowText(GetDlgItem(pHandle, _MapEditToIDC(iIndex + 1)),Form.Fields[iIndex].strValue);
                     }
                    else
                    {
                        int32_t iIndex2;

                        ShowWindow(GetDlgItem(pHandle, _MapComboToIDC(iIndex+1)), SW_SHOW);
                        // populate credit card list
                        DialogComboInit(pHandle, _MapComboToIDC(iIndex+1), "");
                        for (iIndex2 = 0; WebOfferParamList(pOffer, strTemp, sizeof(strTemp), Form.Fields[iIndex].pValues, iIndex2) >= 0; ++iIndex2)
                        {
                            DialogComboAdd(pHandle, _MapComboToIDC(iIndex+1), strTemp);
                        }
                        DialogComboSelect(pHandle, _MapComboToIDC(iIndex+1), Form.Fields[iIndex].strValue);
                    }
                }
            }



            // set & arrange buttons
            iButton = 0;
            for (iIndex = 0; iIndex < 4; ++iIndex)
            {
                // get the IDC to use - remember it takes in buttons 1-4, not 0-3
                iIDCRef = _MapButtonToIDC(iIndex+1);

                if (Form.Button[iIndex].strText[0] != 0)
                {
                    // this button exists
                    SetDlgItemText(pHandle, iIDCRef, Form.Button[iIndex].strText);
                    ShowWindow(GetDlgItem(pHandle, iIDCRef), SW_SHOW);
                    BtnList[iButton++] = iIDCRef;

                    // assign a color to the dialog button if it has one
                    if (Form.Button[iIndex].Color.uAlpha != 0)
                    {
                        DialogColor(pHandle, iIDCRef, RGB(  Form.Button[iIndex].Color.uRed,
                            Form.Button[iIndex].Color.uGreen,
                            Form.Button[iIndex].Color.uBlue));
                    }
                }
            }
            // check that all fields are filled correctly
            do
            {
                BOOL bMandatoryFailed = FALSE;
                BOOL bWrongValuesFailed = FALSE;
                char strErrorMsg[512];
                char strValue[64];
                strcpy(strErrorMsg, "The following Fields are mandatory:\n");
                iButton = _MapIDCToAction(DialogWaitButton(pHandle));
                if(Form.Button[iButton-1].strType[0] != WEBOFFER_TYPE_SUBMIT)
                {
                    break;
                }

                // set values back to the Form
                for (iIndex = 0; iIndex < Form.iNumFields; iIndex++)
                {
                    if(Form.Fields[iIndex].strName[0] != 0)
                    {
                        if (Form.Fields[iIndex].iStyle != WEB_OFFER_FORM_LIST)
                        {
                            GetDlgItemText(pHandle, _MapEditToIDC(iIndex + 1), strValue, sizeof(strValue));
                        }
                        else
                        {
                            GetDlgItemText(pHandle, _MapComboToIDC(iIndex + 1), strValue, sizeof(strValue));
                        }
                        strcpy(Form.Fields[iIndex].strValue, strValue);

                    }
                }

                for (iIndex = 0; iIndex < Form.iNumFields; iIndex++)
                {
                    if(Form.Fields[iIndex].strName[0] != 0)
                    {
                        if ((Form.Fields[iIndex].bMandatory == 1) && (Form.Fields[iIndex].strValue[0] == 0))
                        {
                            strcat(strErrorMsg, Form.Fields[iIndex].strName);
                            strcat(strErrorMsg, "\n");
                            bMandatoryFailed = TRUE;
                        }
                    }
                }
                if (bMandatoryFailed)
                {
                    MessageBox(pHandle, (LPCTSTR)strErrorMsg, "Mandatory Fields not filled", MB_OK);
                    continue;
                }

                for (iIndex = 0; iIndex < Form.iNumFields; iIndex++)
                {
                    if(Form.Fields[iIndex].strName[0] != 0)
                    {
                        GetDlgItemText(pHandle, _MapEditToIDC(iIndex + 1), strValue, sizeof(strValue));
                        if(Form.Fields[iIndex].iStyle == WEB_OFFER_FORM_STRING &&
                           Form.Fields[iIndex].iMaxWidth < (int32_t)strlen(strValue))
                        {
                            strcat(strErrorMsg, Form.Fields[iIndex].strName);
                            strcat(strErrorMsg, " too long\n");
                            bWrongValuesFailed = TRUE;

                        }
                        if (Form.Fields[iIndex].iStyle == WEB_OFFER_FORM_NUMERIC ||
                            Form.Fields[iIndex].iStyle == WEB_OFFER_FORM_FLOAT)
                        {
                            int32_t iLength = strlen(strValue);
                            if (iLength > 10)
                            {
                                strcat(strErrorMsg, Form.Fields[iIndex].strName);
                                strcat(strErrorMsg, " too big a number\n");
                                bWrongValuesFailed = TRUE;
                            }
                            else
                            {
                                int iIdx;
                                for (iIdx = 0; iIdx < iLength ; ++iIdx)
                                {
                                    char ch = Form.Fields[iIndex].strValue[iIdx];
                                    if(ch < '0' || ch > '9')
                                    {
                                        if (Form.Fields[iIndex].iStyle == WEB_OFFER_FORM_NUMERIC)
                                        {
                                            strcat(strErrorMsg, Form.Fields[iIndex].strName);
                                            strcat(strErrorMsg, " not a number\n");
                                            bWrongValuesFailed = TRUE;
                                            break;
                                        }
                                        if(ch != '.')
                                        {
                                            strcat(strErrorMsg, Form.Fields[iIndex].strName);
                                            strcat(strErrorMsg, " not a number\n");
                                            bWrongValuesFailed = TRUE;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                if(bWrongValuesFailed)
                {

                    MessageBox(pHandle, (LPCTSTR)strErrorMsg, "Wrong values", MB_OK);
                    continue;
                }
                bValid = TRUE;

            } while(!bValid);

            WebOfferSetForm(pOffer, &Form);
            WebOfferAction(pOffer, iButton);

        }
    }

    // get result code (optionally set by script)
    WebOfferResultData(pOffer, strTemp, sizeof(strTemp));
    if (strTemp[0] != 0)
    {
        MessageBox(NULL, strTemp, "Exit Value", MB_OK);
    }

    // shut down offer system
    WebOfferDestroy(pOffer);
    // done with script buffer
    free(pScriptBuf);
    // tell caller to restart process
    return(1);
}

// run the sample multiple times until user quits from setup dialog
int32_t SampleThread(const char *pConfig)
{
    // run sample in loop
    while (SampleCode(pConfig) > 0)
        ;
    // done with state
    DialogQuit();
    return(0);
}

