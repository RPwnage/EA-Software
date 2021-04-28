/*H*************************************************************************************/
/*!

    \File T2Host.c

    \Description
        Tester2 Host Application Framework

    \Copyright
        Copyright (c) Electronic Arts 2004.  ALL RIGHTS RESERVED.

    \Version    1.0 10/15/2012 (akirchner) First Version
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gxm.h>
#include <display.h>
#include <libdbgfont.h>
#include <libsysmodule.h>
#include <common_dialog.h>

// dirtysock includes
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/misc/weblog.h"
#include "DirtySDK/proto/protohttp.h"
#include "DirtySDK/util/tagfield.h"

#include "zlib.h"
#include "zmemtrack.h"
#include "testerhostcore.h"
#include "testercomm.h"
#include "ZFile.h"

/*** Defines ***************************************************************************/

/*
Define the maximum number of queued swaps that the display queue will allow.
This limits the number of frames that the CPU can get ahead of the GPU,
and is independent of the actual number of back buffers.  The display
queue will block during sceGxmDisplayQueueAddEntry if this number of swaps
have already been queued.
*/
#define DISPLAY_MAX_PENDING_SWAPS   (2)

//Define the debug font pixel color format to render to.
#define DBGFONT_PIXEL_FORMAT        (SCE_DBGFONT_PIXELFORMAT_A8B8G8R8)

// Define the width and height to render at the native resolution
#define DISPLAY_WIDTH               (960)
#define DISPLAY_HEIGHT              (544)
#define DISPLAY_STRIDE_IN_PIXELS    (1024)

/*
Define the libgxm color format to render to.
This should be kept in sync with the display format to use with the SceDisplay library.
*/
#define DISPLAY_COLOR_FORMAT        (SCE_GXM_COLOR_FORMAT_A8B8G8R8)
#define DISPLAY_PIXEL_FORMAT        (SCE_DISPLAY_PIXELFORMAT_A8B8G8R8)

/*
Define the number of back buffers to use with this sample. Most applications
should use a value of 2 (double buffering) or 3 (triple buffering).
*/
#define DISPLAY_BUFFER_COUNT        (3)

#define XSTART                      (1)
#define YSTART                      (1)
#define XCHAR_INTERVAL              (16)
#define YCHAR_INTERVAL              (20)
#define DEFAULT_XCHAR_INTERVAL      (XCHAR_INTERVAL)
#define DEFAULT_YCHAR_INTERVAL      (YCHAR_INTERVAL)

#define LINE_BOTTOM_Y               (DISPLAY_HEIGHT - YCHAR_INTERVAL)

#define COLOR_WHITE                 (0x00ffffff)
#define COLOR_GRAY                  (0x00696969)
#define COLOR_YELLOW                (0x0000ffff)
#define COLOR_GREEN                 (0x0000ff00)
#define COLOR_RED                   (0x000000ff)
#define COLOR_LIGHT_BLUE            (0x00ffff00)

/*** Macros ****************************************************************************/

#define CENTERING_X_POS(str)        (XSTART + (DISPLAY_WIDTH - strlen(str) * XCHAR_INTERVAL) / 2)
#define DIALOG_CENTERING_X_POS(str) (DIALOG_WINDOW_POS1_X + (DIALOG_WINDOW_WIDTH - strlen(str) * XCHAR_INTERVAL) / 2)
#define ALIGN_RIGHT_X_POS(str)      (XSTART + (DISPLAY_WIDTH - (strlen(str) + 1) * XCHAR_INTERVAL))

/* Helper macro to align a value */
#define ALIGN(x, a)  (((x) + ((a) - 1)) & ~((a) - 1))

/*** Type Definitions ******************************************************************/

typedef struct T2HostAppT
{
    TesterHostCoreT *pCore;
} T2HostAppT;

/*
Data structure to pass through the display queue.  This structure is
serialized during sceGxmDisplayQueueAddEntry, and is used to pass
arbitrary data to the display callback function, called from an internal
thread once the back buffer is ready to be displayed.
In this example, we only need to pass the base address of the buffer.
*/
typedef struct DisplayData
{
    void *address;
} DisplayData;

// Data structure for clear geometry
typedef struct ClearVertex
{
    float x;
    float y;
} ClearVertex;

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

/*
The build process for the sample embeds the shader programs directly into the
executable using the symbols below.  This is purely for convenience, it is
equivalent to simply load the binary file into memory and cast the contents
to type SceGxmProgram.
*/
extern const SceGxmProgram binaryClearVGxpStart;
extern const SceGxmProgram binaryClearFGxpStart;

static SceGxmContextParams          g_ContextParams;            /* libgxm context parameter */
static SceGxmRenderTargetParams     g_RenderTargetParams;       /* libgxm render target parameter */
static SceGxmContext                *g_pContext         = NULL; /* libgxm context */
static SceGxmRenderTarget           *g_pRenderTarget    = NULL; /* libgxm render target */
static SceGxmShaderPatcher          *g_pShaderPatcher   = NULL; /* libgxm shader patcher */

/* display data */
static void                         *g_pDisplayBufferData[DISPLAY_BUFFER_COUNT];
static SceGxmSyncObject             *g_pDisplayBufferSync[DISPLAY_BUFFER_COUNT];
static void                         *g_pDepthBufferData;
static int32_t                      g_displayBufferUId[DISPLAY_BUFFER_COUNT];
static SceGxmColorSurface           g_displaySurface[DISPLAY_BUFFER_COUNT];
static uint32_t                     g_displayFrontBufferIndex = 0;
static uint32_t                     g_displayBackBufferIndex = 0;
static SceGxmDepthStencilSurface    g_depthSurface;

/* shader data */
static int32_t                      g_clearVerticesUId;
static int32_t                      g_clearIndicesUId;
static SceGxmShaderPatcherId        g_clearVertexProgramId;
static SceGxmShaderPatcherId        g_clearFragmentProgramId;
static SceUID                       g_pPatcherFragmentUsseUId;
static SceUID                       g_pPatcherVertexUsseUId;
static SceUID                       g_pPatcherBufferUId;
static SceUID                       g_depthBufferUId;
static SceUID                       g_vdmRingBufferUId;
static SceUID                       g_vertexRingBufferUId;
static SceUID                       g_fragmentRingBufferUId;
static SceUID                       g_fragmentUsseRingBufferUId;
static ClearVertex                  *g_pClearVertices = NULL;
static uint16_t                     *g_pClearIndices = NULL;
static SceGxmVertexProgram          *g_pClearVertexProgram = NULL;
static SceGxmFragmentProgram        *g_pClearFragmentProgram = NULL;

/* T2Host data */
static volatile int                 iContinue = 1;
static TesterHostCoreT              *g_pHostCore;
static char                         strSnapshotName[32] = "";

/*** Private Functions ************************************************************/
size_t sceLibcHeapSize = SCE_LIBC_HEAP_SIZE_EXTENDED_ALLOC_NO_LIMIT;
unsigned int sceLibcHeapExtendedAlloc = 1;

/*F********************************************************************************/
/*!
    \Function _LoadPRXs

    \Description
        Load PRX required by the sample

    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _LoadPRXx()
{
    int32_t iResult;

    if ((iResult = sceSysmoduleLoadModule(SCE_SYSMODULE_NET)) < 0)
    {
        NetPrintf(("t2host: sceSysmoduleLoadModule(SCE_SYSMODULE_NET) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    if ((iResult = sceSysmoduleLoadModule(SCE_SYSMODULE_NP)) < 0)
    {
        NetPrintf(("t2host: sceSysmoduleLoadModule(SCE_SYSMODULE_NP) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    if ((iResult = sceSysmoduleLoadModule(SCE_SYSMODULE_NP_UTILITY)) < 0)
    {
        NetPrintf(("t2host: sceSysmoduleLoadModule(SCE_SYSMODULE_NP_UTILITY) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    if ((iResult = sceSysmoduleLoadModule(SCE_SYSMODULE_NP_BASIC)) < 0)
    {
        NetPrintf(("t2host: sceSysmoduleLoadModule(SCE_SYSMODULE_NP_BASIC) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    if ((iResult = sceSysmoduleLoadModule(SCE_SYSMODULE_HTTPS)) < 0)
    {
        NetPrintf(("t2host: sceSysmoduleLoadModule(SCE_SYSMODULE_NP_HTTPS) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    NetPrintf(("t2host: system PRXs loaded\n"));
}

/*F********************************************************************************/
/*!
    \Function _UnloadPRXs

    \Description
        Unload PRX required by the sample

    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _UnloadPRXx()
{
    int32_t iResult;

    if ((iResult = sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTPS)) < 0)
    {
        NetPrintf(("t2host: sceSysmoduleUnloadModule(SCE_SYSMODULE_NP_HTTPS) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    if ((iResult = sceSysmoduleUnloadModule(SCE_SYSMODULE_NP_BASIC)) < 0)
    {
        NetPrintf(("t2host: sceSysmoduleUnloadModule(SCE_SYSMODULE_NP_BASIC) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    if ((iResult = sceSysmoduleUnloadModule(SCE_SYSMODULE_NP_UTILITY)) < 0)
    {
        NetPrintf(("t2host: sceSysmoduleUnloadModule(SCE_SYSMODULE_NP_UTILITY) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    if ((iResult = sceSysmoduleUnloadModule(SCE_SYSMODULE_NP)) < 0)
    {
        NetPrintf(("t2host: sceSysmoduleUnloadModule(SCE_SYSMODULE_NP) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    if ((iResult = sceSysmoduleUnloadModule(SCE_SYSMODULE_NET)) < 0)
    {
        NetPrintf(("t2host: sceSysmoduleUnloadModule(SCE_SYSMODULE_NET) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    NetPrintf(("t2host: system PRXs unloaded\n"));
}

/*F********************************************************************************/
/*!
    \Function _PatcherHostAlloc

    \Description
        Host alloc

    \Input *pUserData   - pointer to user dat
    \Input uSize        - size

    \Output
        void *          - mem pointer

    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
static void *_PatcherHostAlloc(void *pUserData, uint32_t uSize)
{
    (void)(pUserData);

    return malloc(uSize);
}

/*F********************************************************************************/
/*!
    \Function _PatcherHostFree

    \Description
        Host free

    \Input *pUserData   - pointer to user dat
    \Input *pMem        - mem pointer

    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _PatcherHostFree( void *pUserData, void *pMem)
{
    (void)(pUserData);

    free(pMem);
}

/*F********************************************************************************/
/*!
    \Function _GetRenderData

    \Description
        Find render data buffers

    \Input **ppColor - color buffer
    \Input **ppDepth - depth buffer
    \Input **ppSync  - sync object

    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
void _GetRenderData(void **ppColor, void **ppDepth, void **ppSync)
{
    *ppColor = g_pDisplayBufferData[g_displayBackBufferIndex];
    *ppDepth = g_pDepthBufferData;
    *ppSync  = g_pDisplayBufferSync[g_displayBackBufferIndex];
}

/*F********************************************************************************/
/*!
    \Function _UpdateCheckDialog

    \Description
        Call sceCommonDialogUpdate()

    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
void _UpdateCheckDialog(void)
{
    int32_t iResult = 0;
    SceCommonDialogUpdateParam updateParam;
    void *pColor, *pDepth, *pSync;

    _GetRenderData(&pColor, &pDepth, &pSync);

    memset(&updateParam, 0, sizeof(updateParam));
    updateParam.renderTarget.colorFormat    = DISPLAY_COLOR_FORMAT;
    updateParam.renderTarget.surfaceType    = SCE_GXM_COLOR_SURFACE_LINEAR;
    updateParam.renderTarget.width          = DISPLAY_WIDTH;
    updateParam.renderTarget.height         = DISPLAY_HEIGHT;
    updateParam.renderTarget.strideInPixels = DISPLAY_STRIDE_IN_PIXELS;

    updateParam.renderTarget.colorSurfaceData = pColor;
    updateParam.renderTarget.depthSurfaceData = pDepth;
    updateParam.displaySyncObject = pSync;

    if ((iResult = sceCommonDialogUpdate(&updateParam)) < 0)
    {
        NetPrintf(("t2host: sceCommonDialogUpdate() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
}

/*F********************************************************************************/
/*!
    \Function _RenderDebugText

    \Description
        Render text

    \Output
        int32_t     - 0 for success; negative for failure

    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
void _RenderDebugText(const DisplayData* displayData)
{
    char buf[256];
    int32_t iResult;

    // Display a title name at the top of the screen
    snprintf(buf, sizeof(buf), "********* DirtySock Ticker Sample *********");
    sceDbgFontPrint(CENTERING_X_POS(buf), YSTART, COLOR_WHITE, buf);

    // set framebuffer info
    SceDbgFontFrameBufInfo info;
    memset(&info, 0, sizeof(SceDbgFontFrameBufInfo));
    info.frameBufAddr = (SceUChar8 *)displayData->address;
    info.frameBufPitch = DISPLAY_STRIDE_IN_PIXELS;
    info.frameBufWidth = DISPLAY_WIDTH;
    info.frameBufHeight = DISPLAY_HEIGHT;
    info.frameBufPixelformat = DBGFONT_PIXEL_FORMAT;

    // flush font buffer
    iResult = sceDbgFontFlush(&info);
}

/*F********************************************************************************/
/*!
    \Function _CycleDisplayBuffers

    \Description
        Cycle Display Buffers


    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
void _CycleDisplayBuffers( void )
{
    int32_t iResult;

    /* -----------------------------------------------------------------
    9-a. Flip operation

    Now we have finished submitting rendering work for this frame it
    is time to submit a flip operation.  As part of specifying this
    flip operation we must provide the sync objects for both the old
    buffer and the new buffer.  This is to allow synchronization both
    ways: to not flip until rendering is complete, but also to ensure
    that future rendering to these buffers does not start until the
    flip operation is complete.

    Once we have queued our flip, we manually cycle through our back
    buffers before starting the next frame.
    ----------------------------------------------------------------- */

    // PA heartbeat to notify end of frame
    iResult = sceGxmPadHeartbeat(&g_displaySurface[g_displayBackBufferIndex], g_pDisplayBufferSync[g_displayBackBufferIndex]);
    if (iResult < 0)
    {
        NetPrintf(("t2host: sceGxmDisplayQueueAddEntry() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    // queue the display swap for this frame
    DisplayData displayData;
    displayData.address = g_pDisplayBufferData[g_displayBackBufferIndex];

    // front buffer is OLD buffer, back buffer is NEW buffer
    iResult = sceGxmDisplayQueueAddEntry(g_pDisplayBufferSync[g_displayFrontBufferIndex], g_pDisplayBufferSync[g_displayBackBufferIndex], &displayData);
    if (iResult < 0)
    {
        NetPrintf(("t2host: sceGxmDisplayQueueAddEntry() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    // update buffer indices
    g_displayFrontBufferIndex = g_displayBackBufferIndex;
    g_displayBackBufferIndex = (g_displayBackBufferIndex + 1) % DISPLAY_BUFFER_COUNT;
}

/*F********************************************************************************/
/*!
    \Function _InitLibDbgFont

    \Description
        Initialize libdbgfont

    \Output
        int32_t     - 0 for success; negative for failure

    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t _InitLibDbgFont(void)
{
    int32_t iResult;

    /* ---------------------------------------------------------------------
    1. Initialize libdbgfont

    First we must initialize the libdbgfont library by calling sceDbgFontInit
    with SceDbgFontConfig structure, with set debug font size mode.
    --------------------------------------------------------------------- */

    SceDbgFontConfig config;
    memset(&config, 0, sizeof(SceDbgFontConfig));
    config.fontSize = SCE_DBGFONT_FONTSIZE_LARGE;

    if ((iResult = sceDbgFontInit(&config)) < 0)
    {
        NetPrintf(("t2host: sceDbgFontInit() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ShutdownLibDbgFont

    \Description
        Shutdown libdbgfont

    \Version 23/05/2012 (mclouatre)
*/
/********************************************************************************F*/
void _ShutdownLibDbgFont(void)
{
    int32_t iResult;

    if ((iResult = sceDbgFontExit()) != SCE_OK)
    {
        NetPrintf(("t2host: sceDbgFontExit() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    return;
}

/*F********************************************************************************/
/*!
    \Function _GraphicsAlloc

    \Description
        Alloc used by libgxm

    \Input type         - type
    \Input size         - size
    \Input alignment    - alignment
    \Input attribs      - attributes
    \Input uid          - id

    \Output
        void *          - mem pointer

    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
static void *_GraphicsAlloc(SceKernelMemBlockType type, uint32_t size, uint32_t alignment, uint32_t attribs, SceUID *uid)
{
    void *mem = NULL;
    int32_t iResult;

    /*
    Since we are using sceKernelAllocMemBlock directly, we cannot directly
    use the alignment parameter.  Instead, we must allocate the size to the
    minimum for this memblock type, and just assert that this will cover
    our desired alignment.

    Developers using their own heaps should be able to use the alignment
    parameter directly for more minimal padding.
    */

    if( type == SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RWDATA )
    {
        // CDRAM memblocks must be 256KiB aligned
        size = ALIGN( size, 256*1024 );
    }
    else
    {
        // LPDDR memblocks must be 4KiB aligned
        size = ALIGN( size, 4*1024 );
    }

    // allocate some memory
    *uid = sceKernelAllocMemBlock("t2host", type, size, NULL);

    // grab the base address
    if ((iResult = sceKernelGetMemBlockBase(*uid, &mem)) < 0)
    {
        NetPrintf(("t2host: sceKernelGetMemBlockBase() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    // map for the GPU
    if ((iResult = sceGxmMapMemory(mem, size, attribs)) < 0)
    {
        NetPrintf(("t2host: sceGxmMapMemory() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    return mem;
}

/*F********************************************************************************/
/*!
    \Function _GraphicsFree

    \Description
        Free used by libgxm

    \Input uid  - id

    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _GraphicsFree( SceUID uid )
{
    void *mem = NULL;
    int32_t iResult;

    /* grab the base address */
    if ((iResult = sceKernelGetMemBlockBase(uid, &mem)) < 0)
    {
        NetPrintf(("t2host: sceKernelGetMemBlockBase() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    // unmap memory
    if ((iResult =  sceGxmUnmapMemory(mem)) < 0)
    {
        NetPrintf(("t2host: sceGxmUnmapMemory() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    // free the memory block
    if ((iResult = sceKernelFreeMemBlock(uid)) < 0)
    {
        NetPrintf(("t2host: sceKernelFreeMemBlock() failed in _GraphicsFree() (err=%s)\n", DirtyErrGetName(iResult)));
    }
}


/*F********************************************************************************/
/*!
    \Function _DisplayCallback

    \Description
        Display Callback

    \Input *pUserData   - callback user data

    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
void _DisplayCallback( const void *pUserData)
{
    /* -----------------------------------------------------------------
    10-b. Flip operation

    The callback function will be called from an internal thread once
    queued GPU operations involving the sync objects is complete.
    Assuming we have not reached our maximum number of queued frames,
    this function returns immediately.
    ----------------------------------------------------------------- */

    SceDisplayFrameBuf framebuf;

    // cast the parameters back
    const DisplayData *displayData = (const DisplayData *)pUserData;

    // render debug font
    _RenderDebugText(displayData);

    // swap to the new buffer on the next VSYNC
    memset(&framebuf, 0x00, sizeof(SceDisplayFrameBuf));
    framebuf.size        = sizeof(SceDisplayFrameBuf);
    framebuf.base        = displayData->address;
    framebuf.pitch       = DISPLAY_STRIDE_IN_PIXELS;
    framebuf.pixelformat = DISPLAY_PIXEL_FORMAT;
    framebuf.width       = DISPLAY_WIDTH;
    framebuf.height      = DISPLAY_HEIGHT;

    sceDisplaySetFrameBuf(&framebuf, SCE_DISPLAY_UPDATETIMING_NEXTVSYNC);

    /* block this callback until the swap has occurred and the old buffer is no longer displayed */
    sceDisplayWaitVblankStart();
}

/*F********************************************************************************/
/*!
    \Function _FragmentUsseAlloc

    \Description
        Fragment alloc used by libgxm

    \Input uSize         - size
    \Input uid          - id
    \Input *pUsseOffset - offset pointer

    \Output
        void *          - mem pointer

    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
static void *_FragmentUsseAlloc(uint32_t uSize, SceUID *uid, uint32_t *pUsseOffset)
{
    int32_t iResult;
    void *mem = NULL;

    // align to memblock alignment for LPDDR
    uSize = ALIGN(uSize, 4096);

    // allocate some memory
    *uid = sceKernelAllocMemBlock("t2host", SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, uSize, NULL);

    // grab the base address
    if ((iResult = sceKernelGetMemBlockBase(*uid, &mem)) < 0)
    {
        NetPrintf(("t2host: sceKernelGetMemBlockBase() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    // map as fragment USSE code for the GPU
    if ((iResult = sceGxmMapFragmentUsseMemory(mem, uSize, pUsseOffset)) < 0)
    {
        NetPrintf(("t2host: sceGxmMapFragmentUsseMemory() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    return mem;
}

/*F********************************************************************************/
/*!
    \Function _FragmentUsseFree

    \Description
        Fragment free used by libgxm

    \Input uid          - id

    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _FragmentUsseFree( SceUID uid )
{
    int32_t iResult;
    void *mem = NULL;

    // grab the base address
    if ((iResult = sceKernelGetMemBlockBase(uid, &mem)) < 0)
    {
        NetPrintf(("t2host: sceKernelGetMemBlockBase() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    // unmap memory
    if ((iResult = sceGxmUnmapFragmentUsseMemory(mem)) < 0)
    {
        NetPrintf(("t2host: sceGxmUnmapFragmentUsseMemory() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    // free the memory block
    if ((iResult = sceKernelFreeMemBlock(uid)) < 0)
    {
        NetPrintf(("t2host: sceKernelFreeMemBlock() failed in _FragmentUsseFree() (err=%s)\n", DirtyErrGetName(iResult)));
    }
}

/*F********************************************************************************/
/*!
    \Function _VertexUsseAlloc

    \Description
        Vertex alloc used by libgxm

    \Input uSize         - size
    \Input uid          - id
    \Input *pUsseOffset - offset pointer

    \Output
        void *          - mem pointer

    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
static void *_VertexUsseAlloc(uint32_t uSize, SceUID *uid, uint32_t *pUsseOffset)
{
    void *mem = NULL;
    int32_t iResult;

    // align to memblock alignment for LPDDR
    uSize = ALIGN(uSize, 4096);

    // allocate some memory
    *uid = sceKernelAllocMemBlock("t2host", SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, uSize, NULL);

    // grab the base address
    if ((iResult = sceKernelGetMemBlockBase( *uid, &mem )) < 0)
    {
        NetPrintf(("t2host: sceKernelGetMemBlockBase() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    // map as vertex USSE code for the GPU
    if ((iResult = sceGxmMapVertexUsseMemory(mem, uSize, pUsseOffset)) < 0)
    {
        NetPrintf(("t2host: sceGxmMapVertexUsseMemory() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    return mem;
}

/*F********************************************************************************/
/*!
    \Function _VertexUsseFree

    \Description
        Vertex free used by libgxm

    \Input uid          - id

    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _VertexUsseFree(SceUID uid)
{
    void *mem = NULL;
    int32_t iResult;

    // grab the base address
    if ((iResult = sceKernelGetMemBlockBase(uid, &mem)) < 0)
    {
        NetPrintf(("t2host: sceKernelGetMemBlockBase() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    // unmap memory
    if ((iResult = sceGxmUnmapVertexUsseMemory(mem)) < 0)
    {
        NetPrintf(("t2host: sceGxmUnmapVertexUsseMemory() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    // free the memory block
    if ((iResult = sceKernelFreeMemBlock(uid)) < 0)
    {
        NetPrintf(("t2host: sceKernelFreeMemBlock(uid) failed in _VertexUsseFree() (err=%s)\n", DirtyErrGetName(iResult)));
    }
}


/*F********************************************************************************/
/*!
    \Function _InitLibGxm

    \Description
        Initialize libgxm

    \Output
        int32_t     - 0 for success; negative for failure

    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t _InitLibGxm(void)
{
    int32_t iResult = SCE_OK;

    /* ---------------------------------------------------------------------
    2. Initialize libgxm

    First we must initialize the libgxm library by calling sceGxmInitialize.
    The single argument to this function is the size of the parameter buffer to
    allocate for the GPU.  We will use the default 16MiB here.

    Once initialized, we need to create a rendering context to allow to us
    to render scenes on the GPU.  We use the default initialization
    parameters here to set the sizes of the various context ring buffers.

    Finally we create a render target to describe the geometry of the back
    buffers we will render to.  This object is used purely to schedule
    rendering jobs for the given dimensions, the color surface and
    depth/stencil surface must be allocated separately.
    --------------------------------------------------------------------- */

    // set up parameters
    SceGxmInitializeParams initializeParams;
    memset(&initializeParams, 0, sizeof(SceGxmInitializeParams));
    initializeParams.flags = 0;
    initializeParams.displayQueueMaxPendingCount = DISPLAY_MAX_PENDING_SWAPS;
    initializeParams.displayQueueCallback = _DisplayCallback;
    initializeParams.displayQueueCallbackDataSize = sizeof(DisplayData);
    initializeParams.parameterBufferSize = SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE;

    // start libgxm
    if ((iResult = sceGxmInitialize(&initializeParams)) < 0)
    {
        NetPrintf(("t2host: sceGxmInitialize() failed (err=%s)\n", DirtyErrGetName(iResult)));
        return(iResult);
    }

    // allocate ring buffer memory using default sizes
    void *vdmRingBuffer = _GraphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE, 4, SCE_GXM_MEMORY_ATTRIB_READ, &g_vdmRingBufferUId);

    void *vertexRingBuffer = _GraphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE, 4, SCE_GXM_MEMORY_ATTRIB_READ, &g_vertexRingBufferUId);

    void *fragmentRingBuffer = _GraphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE, 4, SCE_GXM_MEMORY_ATTRIB_READ, &g_fragmentRingBufferUId);

    uint32_t fragmentUsseRingBufferOffset;
    void *fragmentUsseRingBuffer = _FragmentUsseAlloc(SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE, &g_fragmentUsseRingBufferUId, &fragmentUsseRingBufferOffset);

    // create a rendering context
    memset(&g_ContextParams, 0, sizeof(SceGxmContextParams));
    g_ContextParams.hostMem = malloc( SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE );
    g_ContextParams.hostMemSize = SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE;
    g_ContextParams.vdmRingBufferMem = vdmRingBuffer;
    g_ContextParams.vdmRingBufferMemSize = SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE;
    g_ContextParams.vertexRingBufferMem = vertexRingBuffer;
    g_ContextParams.vertexRingBufferMemSize = SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE;
    g_ContextParams.fragmentRingBufferMem = fragmentRingBuffer;
    g_ContextParams.fragmentRingBufferMemSize = SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE;
    g_ContextParams.fragmentUsseRingBufferMem = fragmentUsseRingBuffer;
    g_ContextParams.fragmentUsseRingBufferMemSize = SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE;
    g_ContextParams.fragmentUsseRingBufferOffset = fragmentUsseRingBufferOffset;
    if ((iResult = sceGxmCreateContext(&g_ContextParams, &g_pContext)) < 0)
    {
        NetPrintf(("t2host: sceGxmInitialize() failed (err=%s)\n", DirtyErrGetName(iResult)));
        return(iResult);
    }

    // set buffer sizes for this sample
    const uint32_t patcherBufferSize = 64*1024;
    const uint32_t patcherVertexUsseSize = 64*1024;
    const uint32_t patcherFragmentUsseSize = 64*1024;

    // allocate memory for buffers and USSE code
    void *patcherBuffer = _GraphicsAlloc( SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, patcherBufferSize, 4, SCE_GXM_MEMORY_ATTRIB_WRITE|SCE_GXM_MEMORY_ATTRIB_WRITE, &g_pPatcherBufferUId );

    uint32_t patcherVertexUsseOffset;
    void *patcherVertexUsse = _VertexUsseAlloc( patcherVertexUsseSize, &g_pPatcherVertexUsseUId, &patcherVertexUsseOffset );

    uint32_t patcherFragmentUsseOffset;
    void *patcherFragmentUsse = _FragmentUsseAlloc(patcherFragmentUsseSize, &g_pPatcherFragmentUsseUId, &patcherFragmentUsseOffset);

    // create a shader patcher
    SceGxmShaderPatcherParams patcherParams;
    memset( &patcherParams, 0, sizeof(SceGxmShaderPatcherParams) );
    patcherParams.userData = NULL;
    patcherParams.hostAllocCallback = &_PatcherHostAlloc;
    patcherParams.hostFreeCallback = &_PatcherHostFree;
    patcherParams.bufferAllocCallback = NULL;
    patcherParams.bufferFreeCallback = NULL;
    patcherParams.bufferMem = patcherBuffer;
    patcherParams.bufferMemSize = patcherBufferSize;
    patcherParams.vertexUsseAllocCallback = NULL;
    patcherParams.vertexUsseFreeCallback = NULL;
    patcherParams.vertexUsseMem = patcherVertexUsse;
    patcherParams.vertexUsseMemSize = patcherVertexUsseSize;
    patcherParams.vertexUsseOffset = patcherVertexUsseOffset;
    patcherParams.fragmentUsseAllocCallback = NULL;
    patcherParams.fragmentUsseFreeCallback = NULL;
    patcherParams.fragmentUsseMem = patcherFragmentUsse;
    patcherParams.fragmentUsseMemSize = patcherFragmentUsseSize;
    patcherParams.fragmentUsseOffset = patcherFragmentUsseOffset;
    if ((iResult = sceGxmShaderPatcherCreate(&patcherParams, &g_pShaderPatcher)) < 0)
    {
        NetPrintf(("t2host: sceGxmShaderPatcherCreate() failed (err=%s)\n", DirtyErrGetName(iResult)));
        return(iResult);
    }

    // create a render target
    memset( &g_RenderTargetParams, 0, sizeof(SceGxmRenderTargetParams) );
    g_RenderTargetParams.flags = 0;
    g_RenderTargetParams.width = DISPLAY_WIDTH;
    g_RenderTargetParams.height = DISPLAY_HEIGHT;
    g_RenderTargetParams.scenesPerFrame = 1;
    g_RenderTargetParams.multisampleMode = SCE_GXM_MULTISAMPLE_NONE;
    g_RenderTargetParams.multisampleLocations = 0;
    g_RenderTargetParams.driverMemBlock = -1;
#if SCE_PSP2_SDK_VERSION < 0x01000071
    g_RenderTargetParams.hostMem = NULL;
    g_RenderTargetParams.hostMemSize = 0;

    // compute sizes
    uint32_t hostMemSize, driverMemSize;
    sceGxmGetRenderTargetMemSizes(&g_RenderTargetParams, &hostMemSize, &driverMemSize);

    // allocate host memory
    g_RenderTargetParams.hostMem = malloc(hostMemSize);
    g_RenderTargetParams.hostMemSize = hostMemSize;
#else
    // compute sizes
    uint32_t driverMemSize;
    sceGxmGetRenderTargetMemSize(&g_RenderTargetParams, &driverMemSize);
#endif
    // allocate driver memory
    g_RenderTargetParams.driverMemBlock = sceKernelAllocMemBlock("t2host", SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, driverMemSize, NULL);

    if ((iResult = sceGxmCreateRenderTarget(&g_RenderTargetParams, &g_pRenderTarget)) < 0)
    {
        NetPrintf(("t2host: sceGxmCreateRenderTarget() failed (err=%s)\n", DirtyErrGetName(iResult)));
        return(iResult);
    }

    /* ---------------------------------------------------------------------
    3. Allocate display buffers, set up the display queue

    We will allocate our back buffers in CDRAM, and create a color
    surface for each of them.

    To allow display operations done by the CPU to be synchronized with
    rendering done by the GPU, we also create a SceGxmSyncObject for each
    display buffer.  This sync object will be used with each scene that
    renders to that buffer and when queueing display flips that involve
    that buffer (either flipping from or to).

    Finally we create a display queue object that points to our callback
    function.
    --------------------------------------------------------------------- */

    // allocate memory and sync objects for display buffers
    for (uint32_t i= 0 ; i < DISPLAY_BUFFER_COUNT ; ++i)
    {
        // allocate memory with large size to ensure physical contiguity
        g_pDisplayBufferData[i] = _GraphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RWDATA, ALIGN(4*DISPLAY_STRIDE_IN_PIXELS*DISPLAY_HEIGHT, 1*1024*1024), SCE_GXM_COLOR_SURFACE_ALIGNMENT, SCE_GXM_MEMORY_ATTRIB_READ|SCE_GXM_MEMORY_ATTRIB_WRITE, &g_displayBufferUId[i]);

        // memset the buffer to debug color
        for (uint32_t y = 0 ; y < DISPLAY_HEIGHT ; ++y)
        {
            uint32_t *row = (uint32_t *)g_pDisplayBufferData[i] + y*DISPLAY_STRIDE_IN_PIXELS;

            for (uint32_t x = 0 ; x < DISPLAY_WIDTH ; ++x)
            {
                row[x] = 0x0;
            }
        }

        // initialize a color surface for this display buffer
        iResult = sceGxmColorSurfaceInit(&g_displaySurface[i], DISPLAY_COLOR_FORMAT, SCE_GXM_COLOR_SURFACE_LINEAR, SCE_GXM_COLOR_SURFACE_SCALE_NONE,
                                         SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_STRIDE_IN_PIXELS, g_pDisplayBufferData[i]);
        if (iResult < 0)
        {
            NetPrintf(("t2host: sceGxmColorSurfaceInit() failed (err=%s)\n", DirtyErrGetName(iResult)));
            return(iResult);
        }

        // create a sync object that we will associate with this buffer
        if ((iResult = sceGxmSyncObjectCreate(&g_pDisplayBufferSync[i])) < 0)
        {
            NetPrintf(("t2host: sceGxmSyncObjectCreate() failed (err=%s)\n", DirtyErrGetName(iResult)));
            return(iResult);
        }
    }

    // compute the memory footprint of the depth buffer
    const uint32_t alignedWidth = ALIGN(DISPLAY_WIDTH, SCE_GXM_TILE_SIZEX);
    const uint32_t alignedHeight = ALIGN(DISPLAY_HEIGHT, SCE_GXM_TILE_SIZEY);
    uint32_t sampleCount = alignedWidth*alignedHeight;
    uint32_t depthStrideInSamples = alignedWidth;

    // allocate it
    g_pDepthBufferData = _GraphicsAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, 4*sampleCount, SCE_GXM_DEPTHSTENCIL_SURFACE_ALIGNMENT, SCE_GXM_MEMORY_ATTRIB_READ|SCE_GXM_MEMORY_ATTRIB_WRITE, &g_depthBufferUId);

    // create the SceGxmDepthStencilSurface structure
    iResult = sceGxmDepthStencilSurfaceInit( &g_depthSurface, SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24, SCE_GXM_DEPTH_STENCIL_SURFACE_TILED, depthStrideInSamples, g_pDepthBufferData, NULL );
    if (iResult < 0)
    {
        NetPrintf(("t2host: sceGxmDepthStencilSurfaceInit() failed (err=%s)\n", DirtyErrGetName(iResult)));
        return(iResult);
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ShutdownLibGxm

    \Description
        Shutdow libgxm

    \Version 23/05/2011 (mclouatre)
*/
/********************************************************************************F*/
void _ShutdownLibGxm(void)
{
    int32_t iResult;
    uint32_t uIndex;

    // Once the GPU is finished, we deallocate all our memory,
    // destroy all object and finally terminate libgxm.

    _GraphicsFree(g_depthBufferUId);

    for (uIndex = 0 ; uIndex < DISPLAY_BUFFER_COUNT; ++uIndex)
    {
        memset(g_pDisplayBufferData[uIndex], 0, DISPLAY_HEIGHT*DISPLAY_STRIDE_IN_PIXELS*4);
        _GraphicsFree(g_displayBufferUId[uIndex]);
        if ((iResult = sceGxmSyncObjectDestroy(g_pDisplayBufferSync[uIndex])) != SCE_OK)
        {
            NetPrintf(("t2host: sceGxmSyncObjectDestroy() failed (err=%s)\n", DirtyErrGetName(iResult)));
        }
    }

    if ((iResult = sceGxmDestroyRenderTarget(g_pRenderTarget)) != SCE_OK)
    {
        NetPrintf(("t2host: sceGxmDestroyRenderTarget() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    if ((iResult = sceKernelFreeMemBlock(g_RenderTargetParams.driverMemBlock)) != SCE_OK)
    {
        NetPrintf(("t2host: sceKernelFreeMemBlock(g_RenderTargetParams.driverMemBlock) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    // destroy the context
    if ((iResult = sceGxmDestroyContext(g_pContext)) != SCE_OK)
    {
        NetPrintf(("t2host: sceGxmDestroyContext() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    _FragmentUsseFree(g_fragmentUsseRingBufferUId);
    _GraphicsFree(g_fragmentRingBufferUId);
    _GraphicsFree(g_vertexRingBufferUId);
    _GraphicsFree(g_vdmRingBufferUId);
    free(g_ContextParams.hostMem);

    // terminate libgxm/
    if ((iResult = sceGxmTerminate()) != SCE_OK)
    {
        NetPrintf(("t2host: sceGxmTerminate() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    return;
}


/*F********************************************************************************/
/*!
    \Function _SystemInit

    \Description
        Initialise system - memory, graphics, etc.

    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
void _SystemInit(void)
{
    int32_t iResult;

    // initialize libdbgfont
    if ((iResult = _InitLibDbgFont()) < 0)
    {
        NetPrintf(("t2host: failed to initialize libdgfont\n"));
    }

    // initialize libgxm
    if ((iResult = _InitLibGxm()) < 0)
    {
        NetPrintf(("t2host: failed to initialize libgxm\n"));
    }
}

/*F********************************************************************************/
/*!
    \Function _SystemShutdown

    \Description
        Shutdown system - memory, graphics, etc.

    \Version 05/23/2012 (mclouatre)
*/
/********************************************************************************F*/
void _SystemShutdown(void)
{
    _ShutdownLibGxm();
    _ShutdownLibDbgFont();

    NetPrintf(("t2host: system resources shutdown complete\n"));
}

/*F********************************************************************************/
/*!
    \Function _CreateGxmData

    \Description
        Create libgxm scenes

    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
void _CreateGxmData( void )
{
    int32_t iResult;
    const SceGxmProgram *clearProgram;
    const SceGxmProgramParameter *paramClearPositionAttribute;

    /* ---------------------------------------------------------------------
    4. Create a shader patcher and register programs

    A shader patcher object is required to produce vertex and fragment
    programs from the shader compiler output.  First we create a shader
    patcher instance, using callback functions to allow it to allocate
    and free host memory for internal state.

    In order to create vertex and fragment programs for a particular
    shader, the compiler output must first be registered to obtain an ID
    for that shader.  Within a single ID, vertex and fragment programs
    are reference counted and could be shared if created with identical
    parameters.  To maximise this sharing, programs should only be
    registered with the shader patcher once if possible, so we will do
    this now.
    --------------------------------------------------------------------- */

    // register programs with the patcher
    iResult = sceGxmShaderPatcherRegisterProgram(g_pShaderPatcher, &binaryClearVGxpStart, &g_clearVertexProgramId);
    if (iResult < 0)
    {
        NetPrintf(("t2host: sceGxmShaderPatcherRegisterProgram() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    iResult = sceGxmShaderPatcherRegisterProgram(g_pShaderPatcher, &binaryClearFGxpStart, &g_clearFragmentProgramId);
    if (iResult < 0)
    {
        NetPrintf(("t2host: sceGxmShaderPatcherRegisterProgram() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    /* ---------------------------------------------------------------------
    5. Create the programs and data for the clear

    On SGX hardware, vertex programs must perform the unpack operations
    on vertex data, so we must define our vertex formats in order to
    create the vertex program.  Similarly, fragment programs must be
    specialized based on how they output their pixels and MSAA mode
    (and texture format on ES1).

    We define the clear geometry vertex format here and create the vertex
    and fragment program.

    The clear vertex and index data is static, we allocate and write the
    data here.
    --------------------------------------------------------------------- */

    // get attributes by name to create vertex format bindings
    clearProgram = sceGxmShaderPatcherGetProgramFromId( g_clearVertexProgramId );
    paramClearPositionAttribute = sceGxmProgramFindParameterByName( clearProgram, "aPosition" );

    // create clear vertex format
    SceGxmVertexAttribute clearVertexAttributes[1];
    SceGxmVertexStream clearVertexStreams[1];
    clearVertexAttributes[0].streamIndex = 0;
    clearVertexAttributes[0].offset = 0;
    clearVertexAttributes[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
    clearVertexAttributes[0].componentCount = 2;
    clearVertexAttributes[0].regIndex = sceGxmProgramParameterGetResourceIndex( paramClearPositionAttribute );
    clearVertexStreams[0].stride = sizeof(ClearVertex);
    clearVertexStreams[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

    // create clear programs
    iResult = sceGxmShaderPatcherCreateVertexProgram(g_pShaderPatcher, g_clearVertexProgramId, clearVertexAttributes, 1, clearVertexStreams, 1, &g_pClearVertexProgram);
    if (iResult < 0)
    {
        NetPrintf(("t2host: sceGxmShaderPatcherCreateVertexProgram() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    iResult = sceGxmShaderPatcherCreateFragmentProgram(g_pShaderPatcher, g_clearFragmentProgramId,
                                                           SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4, SCE_GXM_MULTISAMPLE_NONE, NULL,
                                                           sceGxmShaderPatcherGetProgramFromId(g_clearVertexProgramId), &g_pClearFragmentProgram);
    if (iResult < 0)
    {
        NetPrintf(("t2host: sceGxmShaderPatcherCreateFragmentProgram() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    // create the clear triangle vertex/index data
    g_pClearVertices = (ClearVertex *)_GraphicsAlloc( SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, 3*sizeof(ClearVertex), 4, SCE_GXM_MEMORY_ATTRIB_READ, &g_clearVerticesUId );
    g_pClearIndices = (uint16_t *)_GraphicsAlloc( SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE, 3*sizeof(uint16_t), 2, SCE_GXM_MEMORY_ATTRIB_READ, &g_clearIndicesUId );

    g_pClearVertices[0].x = -1.0f;
    g_pClearVertices[0].y = -1.0f;
    g_pClearVertices[1].x =  3.0f;
    g_pClearVertices[1].y = -1.0f;
    g_pClearVertices[2].x = -1.0f;
    g_pClearVertices[2].y =  3.0f;

    g_pClearIndices[0] = 0;
    g_pClearIndices[1] = 1;
    g_pClearIndices[2] = 2;
}

/*F********************************************************************************/
/*!
    \Function _DestroyGxmData

    \Description
        Destroy Gxm Data

    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
void _DestroyGxmData( void )
{
    /* ---------------------------------------------------------------------
    11. Destroy the programs and data for the clear and spinning triangle

    Once the GPU is finished, we release all our programs.
    --------------------------------------------------------------------- */

    // clean up allocations
    sceGxmShaderPatcherReleaseFragmentProgram(g_pShaderPatcher, g_pClearFragmentProgram);
    sceGxmShaderPatcherReleaseVertexProgram(g_pShaderPatcher, g_pClearVertexProgram);
    _GraphicsFree(g_clearIndicesUId);
    _GraphicsFree(g_clearVerticesUId);

    // wait until display queue is finished before deallocating display buffers
    sceGxmDisplayQueueFinish();

    // unregister programs and destroy shader patcher
    sceGxmShaderPatcherUnregisterProgram(g_pShaderPatcher, g_clearFragmentProgramId);
    sceGxmShaderPatcherUnregisterProgram(g_pShaderPatcher, g_clearVertexProgramId);
    sceGxmShaderPatcherDestroy(g_pShaderPatcher);
    _FragmentUsseFree(g_pPatcherFragmentUsseUId);
    _VertexUsseFree(g_pPatcherVertexUsseUId);
    _GraphicsFree(g_pPatcherBufferUId);
}


/*F********************************************************************************/
/*!
    \Function _RenderGxm

    \Description
        Render GXM scene

    \Version 04/27/2011 (mclouatre)
*/
/********************************************************************************F*/
void _RenderGxm(void)
{
    /* -----------------------------------------------------------------
    8. Rendering step

    This sample renders a single scene containing the clear triangle.
    Before any drawing can take place, a scene must be started.
    We render to the back buffer, so it is also important to use a
    sync object to ensure that these rendering operations are
    synchronized with display operations.

    The clear triangle shaders do not declare any uniform variables,
    so this may be rendered immediately after setting the vertex and
    fragment program.

    Once clear triangle have been drawn the scene can be ended, which
    submits it for rendering on the GPU.
    ----------------------------------------------------------------- */

    // start rendering to the render target
    sceGxmBeginScene(g_pContext, 0, g_pRenderTarget, NULL, NULL, g_pDisplayBufferSync[g_displayBackBufferIndex], &g_displaySurface[g_displayBackBufferIndex], &g_depthSurface);

    // set clear shaders
    sceGxmSetVertexProgram(g_pContext, g_pClearVertexProgram);
    sceGxmSetFragmentProgram(g_pContext, g_pClearFragmentProgram);

    // draw ther clear triangle
    sceGxmSetVertexStream(g_pContext, 0, g_pClearVertices);
    sceGxmDraw(g_pContext, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, g_pClearIndices, 3);

    // stop rendering to the render target
    sceGxmEndScene(g_pContext, NULL, NULL);
}


/*F********************************************************************************/
/*!
    \Function _SetupCommonDialog

    \Description
        Startup the Common Dialog library, which provides various high-level functions
        Each function has the system to dialog with the end users and operates
        independently.

    \Version 05/16/2011 (mclouatre)
*/
/********************************************************************************F*/
void _SetupCommonDialog(void)
{
    int32_t iResult = 0;
    SceCommonDialogConfigParam config;
    sceCommonDialogConfigParamInit(&config);

    config.language = SCE_SYSTEM_PARAM_LANG_ENGLISH_US;
    config.enterButtonAssign = SCE_SYSTEM_PARAM_ENTER_BUTTON_ASSIGN_CROSS;

    iResult = sceCommonDialogSetConfigParam(&config);
    if (iResult < 0)
    {
        NetPrintf(("t2host: sceCommonDialogSetConfigParam() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    return;
}

/*F********************************************************************************/
/*!
    \Function _T2HostCmdExit

    \Description
        Quit

    \Input  *argz   - environment
    \Input   argc   - num args
    \Input **argv   - arg list

    \Output  int32_t    - standard return code

    \Version 05/25/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _T2HostCmdExit(ZContext *argz, int32_t argc, char **argv)
{
    if (argc >= 1)
    {
        iContinue = 0;
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _T2HostCmdSnapshot

    \Description
        Set up to take a snapshot.

    \Input  *argz   - environment
    \Input   argc   - num args
    \Input **argv   - arg list

    \Output  int32_t    - standard return code

    \Version 05/25/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _T2HostCmdSnapshot(ZContext *argz, int32_t argc, char **argv)
{
    if (argc == 2)
    {
        ds_strnzcpy(strSnapshotName, argv[1], sizeof(strSnapshotName));
    }
    else
    {
        ZPrintf("usage: snap <snapshotname>\n");
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _T2HostUpdate

    \Description
        Update tester2 processing

    \Input  *argz   - environment
    \Input   argc   - num args
    \Input **argv   - arg list

    \Output  int32_t    - standard return code

    \Version 05/25/2006 (tdills)
*/
/********************************************************************************F*/
static void _T2HostUpdate(T2HostAppT *pApp)
{
    // pump the hostcore module
    TesterHostCoreUpdate(pApp->pCore, 1);

    // give time to zlib
    ZTask();
    ZCleanup();

    // give time to network
    NetConnIdle();
}

/*** Public Functions ******************************************************************/

/*F********************************************************************************/
/*!
    \Function main

    \Description
        Main routine for PS3 T2Host application.

    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Output int32_t - zero

    \Version 10/15/2012 (akirchner)
*/
/********************************************************************************F*/
int main(int32_t argc, char *argv[])
{
    T2HostAppT T2App;
    char strBase    [128] = "";
    char strHostName[128] = "";
    char strParams  [512];
    int32_t iArg;

    // load up required system prx modules
    _LoadPRXx();

     NetPrintf(("t2host: PRXs loading completed\n"));

    // init system resources (memory, graphics, etc.)
    _SystemInit();

     NetPrintf(("t2host: system-level initialization completed\n"));

    // create gxm graphics data
    _CreateGxmData();

     NetPrintf(("t2host: GXM data creation completed\n"));

    // startup the common dialog library
    _SetupCommonDialog();

     NetPrintf(("t2host: Common Dialog setup completed\n"));

    // get arguments
    for (iArg = 0; iArg < argc; iArg++)
    {
        if (!strncmp(argv[iArg], "-path=", 6))
        {
            ds_strnzcpy(strBase, &argv[iArg][6], sizeof(strBase));
            ZPrintf("t2host: base path=%s\n", strBase);
        }
        if (!strncmp(argv[iArg], "-connect=", 9))
        {
            ds_strnzcpy(strHostName, &argv[iArg][9], sizeof(strHostName));
            ZPrintf("t2host: connect=%s\n", strHostName);
        }
    }

    // start the memtracker before we do anything else
    ZMemtrackStartup();

    // create the module
    TagFieldClear(strParams, sizeof(strParams));
    TagFieldSetNumber(strParams, sizeof(strParams), "PLATFORM", TESTER_PLATFORM_NORMAL);
    TagFieldSetString(strParams, sizeof(strParams), "INPUTFILE", TESTERCOMM_HOSTINPUTFILE);
    TagFieldSetString(strParams, sizeof(strParams), "OUTPUTFILE", TESTERCOMM_HOSTOUTPUTFILE);
    TagFieldSetString(strParams, sizeof(strParams), "CONTROLDIR", strBase);
    TagFieldSetString(strParams, sizeof(strParams), "HOSTNAME", strHostName);

    // start network
    NetConnStartup("-servicename=tester2");

    NetPrintf(("t2host: NetConnStartup() completed\n"));

    // create tester2 host core
    g_pHostCore = TesterHostCoreCreate(strParams);

    // bring up the interface
    NetConnConnect(NULL, "silent=false", 0);

    NetPrintf(("t2host: NetConnConnect() completed\n"));

    // register some basic commands
    TesterHostCoreRegister(g_pHostCore, "exit", _T2HostCmdExit);
    TesterHostCoreRegister(g_pHostCore, "snap", _T2HostCmdSnapshot);

    // init app structure
    T2App.pCore = g_pHostCore;

    ZPrintf("entering polling loop\n");

    // wait for network interface activation (requires user to sign-in)
    while(iContinue)
    {
        // update status
        _T2HostUpdate(&T2App);

        // render (required for Net Check Dialog window to pop up)
        _RenderGxm();
        _UpdateCheckDialog();
        _CycleDisplayBuffers();
    }

    NetPrintf(("t2host: done\n"));

    // clean up and exit
    TesterHostCoreDisconnect(g_pHostCore);
    TesterHostCoreDestroy(g_pHostCore);

    // shutdown the network connections && destroy the dirtysock code
    NetConnShutdown(FALSE);

    // destroy gxm graphics data
    _DestroyGxmData();

    // destroy other system resources
    _SystemShutdown();

    // unload system prx modules
    _UnloadPRXx();

    return(0);
}
