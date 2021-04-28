/*H*************************************************************************************/
/*!

    \File T2Host.c

    \Description
        Tester2 Host Application Framework

    \Copyright
        Copyright (c) Electronic Arts 2004.  ALL RIGHTS RESERVED.

    \Version    1.0 03/22/2005 (jfrank) First Version
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dirtysock.h"
#include "dirtyerr.h"
#include "netconn.h"
#include "lobbytagfield.h"
#include "zlib.h"
#include "zmemtrack.h"
#include "testerhostcore.h"
#include "testercomm.h"
#include "ZFile.h"

#include <cell/cell_fs.h>
#include <cell/gcm.h>
#include <cell/sysmodule.h>
#include <sysutil/sysutil_sysparam.h>
#include <sys/timer.h>

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

#define VOIP_IRX_PATH   (DIRTYSOCK_PATH"contrib/voip/library/iop")

#define HOST_SIZE (8*1024*1024)
#define COLOR_BUFFER_NUM 2

/*** Type Definitions ******************************************************************/

typedef struct T2HostAppT
{
    TesterHostCoreT *pCore;
} T2HostAppT;

typedef struct
{
    float x, y, z;
    uint32_t rgba;
} Vertex_t;

/*** Function Prototypes ***************************************************************/

extern uint32_t _binary_vpshader_vpo_start;
extern uint32_t _binary_fpshader_fpo_start;

/*** Variables *************************************************************************/

// tester host module
static TesterHostCoreT *g_pHostCore;

static uint8_t *host_addr = NULL;

static uint32_t local_mem_heap = 0;

static uint32_t display_width;
static uint32_t display_height;
static uint32_t color_pitch;
static uint32_t depth_pitch;
static uint32_t color_offset[4];
static uint32_t depth_offset;

static unsigned char *vertex_program_ptr = (unsigned char *)&_binary_vpshader_vpo_start;
static unsigned char *fragment_program_ptr = (unsigned char *)&_binary_fpshader_fpo_start;

static CGprogram vertex_program;
static CGprogram fragment_program;

static void *vertex_program_ucode;
static void *fragment_program_ucode;

static uint32_t frame_index = 0;

static volatile int iContinue=1;

static char strSnapshotName[32] = "";

/*** Private Functions *****************************************************************/


/*F********************************************************************************/
/*!
    \Function _T2HostSysUtilCb

    \Description
        Receives events from Sony's sysutil library.

    \Output void

    \Version 05/25/2006 (tdills)
*/
/********************************************************************************F*/
static void _T2HostSysUtilCb(uint64_t uStatus, uint64_t uParam, void *pUserData)
{
    if (uStatus == CELL_SYSUTIL_REQUEST_EXITGAME)
    {
        iContinue = 0;
    }
}

/*F********************************************************************************/
/*!
    \Function _T2HostLoadSysUtils

    \Description
        Load required Sony sysutils

    \Version 11/29/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _T2HostLoadSysUtils(void)
{
    int32_t iResult;
    // load up required system prx modules
    if ((iResult = cellSysmoduleLoadModule(CELL_SYSMODULE_IO)) < 0)
    {
        NetPrintf(("t2host: cellSysmoduleLoadModule(CELL_SYSMODULE_IO) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    if ((iResult = cellSysmoduleLoadModule(CELL_SYSMODULE_GCM_SYS)) < 0)
    {
        NetPrintf(("t2host: cellSysmoduleLoadModule(CELL_SYSMODULE_GCM_SYS) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    if ((iResult = cellSysmoduleLoadModule(CELL_SYSMODULE_FS)) < 0)
    {
        NetPrintf(("t2host: cellSysmoduleLoadModule(CELL_SYSMODULE_FS) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    if ((iResult = cellSysmoduleLoadModule(CELL_SYSMODULE_NET)) < 0)
    {
        NetPrintf(("t2host: cellSysmoduleLoadModule(CELL_SYSMODULE_NET) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    if ((iResult = cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_NP)) < 0)
    {
        NetPrintf(("t2host: cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_NP) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    if ((iResult = cellSysmoduleLoadModule(CELL_SYSMODULE_RTC)) < 0)
    {
        NetPrintf(("t2host: cellSysmoduleLoadModule(CELL_SYSMODULE_RTC) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    if ((iResult = cellSysmoduleLoadModule(CELL_SYSMODULE_AUDIO)) < 0)
    {
        NetPrintf(("t2host: cellSysmoduleLoadModule(CELL_SYSMODULE_AUDIO) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    if ((iResult = cellSysmoduleLoadModule(CELL_SYSMODULE_MIC)) < 0)
    {
        NetPrintf(("t2host: cellSysmoduleLoadModule(CELL_SYSMODULE_MIC) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    if ((iResult = cellSysmoduleLoadModule(CELL_SYSMODULE_CELPENC)) < 0)
    {
        NetPrintf(("t2host: cellSysmoduleLoadModuleEx(CELL_SYSMODULE_CELPENC) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    if ((iResult = cellSysmoduleLoadModule(CELL_SYSMODULE_ADEC_CELP)) < 0)
    {
        NetPrintf(("t2host: cellSysmoduleLoadModuleEx(CELL_SYSMODULE_ADEC_CELP) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    if ((iResult = cellSysmoduleLoadModule(CELL_SYSMODULE_CELP8ENC)) < 0)
    {
        NetPrintf(("t2host: cellSysmoduleLoadModuleEx(CELL_SYSMODULE_CELP8ENC) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    if ((iResult = cellSysmoduleLoadModule(CELL_SYSMODULE_ADEC_CELP8)) < 0)
    {
        NetPrintf(("t2host: cellSysmoduleLoadModuleEx(CELL_SYSMODULE_ADEC_CELP8) failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    // register the Sony sysutil callback
    if (cellSysutilRegisterCallback( 3, _T2HostSysUtilCb, NULL ) != 0)
    {
        ZPrintf("cellSysutilRegisterCallback failed\n");
    }
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
        strnzcpy(strSnapshotName, argv[1], sizeof(strSnapshotName));
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

/*F********************************************************************************/
/*!
    \Function _T2HostLocalMemAlloc

    \Description
        Allocates unaligned memory in 1024 byte chunks from the memory pointed to
        by local_mem_heap.

    \Input size - the number of bytes actually required.

    \Output void* - a pointer to the allocated memory

    \Version 05/25/2006 (tdills)
*/
/********************************************************************************F*/
static void *_T2HostLocalMemAlloc(const uint32_t size)
{
    uint32_t allocated_size = (size + 1023) & (~1023);
    uint32_t base = local_mem_heap;
    local_mem_heap += allocated_size;
    return((void*)base);
}

/*F********************************************************************************/
/*!
    \Function _T2HostLocalMemAlign

    \Description
        Allocated aligned memory from the memory pointed to by local_mem_heap.

    \Input alignment - the number of bytes on which to align the memory
    \Input size      - the size of the memory to allocate from the heap

    \Output void*    - a pointer to the allocated memory

    \Version 05/25/2006 (tdills)
*/
/********************************************************************************F*/
static void *_T2HostLocalMemAlign(const uint32_t alignment,
        const uint32_t size)
{
    local_mem_heap = (local_mem_heap + alignment-1) & (~(alignment-1));
    return((void*)_T2HostLocalMemAlloc(size));
}

/*F********************************************************************************/
/*!
    \Function _T2HostNetStart

    \Description
        Connect network using "net" commands, wait for network to initialize.

    \Input *pCore   - TesterHostCoreT ref

    \Output
        None.

    \Version 08/11/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _T2HostNetConnect(TesterHostCoreT *pCore)
{
    int32_t iStatus, iTimeout;

    // connect the network using "net" module
    TesterHostCoreDispatch(pCore, "net connect");

    // wait for network interface activation
    for (iTimeout = NetTick()+15*1000 ; ; )
    {
        // update network
        NetConnIdle();

        // get current status
        iStatus = NetConnStatus('conn', 0, NULL, 0);
        if ((iStatus == '+onl') || ((iStatus >> 24) == '-'))
        {
            break;
        }

        // check for timeout
        if (iTimeout < (signed)NetTick())
        {
            NetPrintf(("t2host: timeout waiting for interface activation\n"));
            break;
        }

        // give time to other threads
        NetConnSleep(500);
    }

    // check result code
    if ((iStatus = NetConnStatus('conn', 0, NULL, 0)) == '+onl')
    {
        NetPrintf(("t2host: interface active\n"));
    }
    else if ((iStatus >> 14) == '-')
    {
        NetPrintf(("t2host: error bringing up interface\n"));
    }
}

/*F********************************************************************************/
/*!
    \Function _T2HostInitDisplay

    \Description
        Initializes the display.

    \Output void

    \Version 05/25/2006 (tdills)
*/
/********************************************************************************F*/
static int32_t _T2HostInitDisplay(void)
{
    uint32_t color_depth=4; // ARGB8
    uint32_t z_depth=4;     // COMPONENT24
    void *color_base_addr;
    void *depth_base_addr;
    void *color_addr[4];
    void *depth_addr;
    int32_t ret;
    CellVideoOutResolution resolution;

    // read the current video status
    // INITIAL DISPLAY MODE HAS TO BE SET BY RUNNING SETMONITOR.SELF
    CellVideoOutState videoState;
    cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &videoState);
    cellVideoOutGetResolution(videoState.displayMode.resolutionId, &resolution);

    display_width = resolution.width;
    display_height = resolution.height;
    color_pitch = display_width*color_depth;
    depth_pitch = display_width*z_depth;

    uint32_t color_size =   color_pitch*display_height;
    uint32_t depth_size =  depth_pitch*display_height;

    CellVideoOutConfiguration videocfg;
    memset(&videocfg, 0, sizeof(CellVideoOutConfiguration));
    videocfg.resolutionId = videoState.displayMode.resolutionId;
    videocfg.format = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;
    videocfg.pitch = color_pitch;

    ret = cellVideoOutConfigure(CELL_VIDEO_OUT_PRIMARY, &videocfg, NULL, 0);
    if (ret != CELL_OK){
        printf("cellVideoOutConfigure() failed. (0x%x)\n", ret);
        return(-1);
    }

    cellGcmSetFlipMode(CELL_GCM_DISPLAY_VSYNC);

    // get config
    CellGcmConfig config;
    cellGcmGetConfiguration(&config);
    // buffer memory allocation
    local_mem_heap = (uint32_t)config.localAddress;

    color_base_addr = _T2HostLocalMemAlign(16, COLOR_BUFFER_NUM*color_size);

    for (int i = 0; i < COLOR_BUFFER_NUM; i++) {
        color_addr[i]
            = (void *)((uint32_t)color_base_addr+ (i*color_size));
        ret = cellGcmAddressToOffset(color_addr[i], &color_offset[i]);
        if(ret != CELL_OK)
            return(-1);
    }

    // regist surface
    for (int i = 0; i < COLOR_BUFFER_NUM; i++) {
        ret = cellGcmSetDisplayBuffer(i, color_offset[i], color_pitch, display_width, display_height);
        if(ret != CELL_OK)
            return(-1);
    }

    depth_base_addr = _T2HostLocalMemAlign(16, depth_size);
    depth_addr = depth_base_addr;
    ret = cellGcmAddressToOffset(depth_addr, &depth_offset);
    if(ret != CELL_OK)
        return(-1);

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _T2HostInitShader

    \Description
        Initializes the vertex and fragment objects which point at binary code that
        was compiled from .cg files.

    \Output void

    \Version 05/25/2006 (tdills)
*/
/********************************************************************************F*/
static void _T2HostInitShader(void)
{
    vertex_program   = (CGprogram)vertex_program_ptr;
    fragment_program = (CGprogram)fragment_program_ptr;

    // init
    cellGcmCgInitProgram(vertex_program);
    cellGcmCgInitProgram(fragment_program);

    uint32_t ucode_size;
    void *ucode;
    cellGcmCgGetUCode(fragment_program, &ucode, &ucode_size);
    // 64B alignment required
    void *ret = _T2HostLocalMemAlign(64, ucode_size);
    fragment_program_ucode = ret;
    memcpy(fragment_program_ucode, ucode, ucode_size);

    cellGcmCgGetUCode(vertex_program, &ucode, &ucode_size);
    vertex_program_ucode = ucode;
}

/*F********************************************************************************/
/*!
    \Function _T2HostInitVertex

    \Description
        Initializes a vertex

    \Input *vertex_buffer - the vertex to be initialized

    \Output void

    \Version 05/25/2006 (tdills)
*/
/********************************************************************************F*/
static void _T2HostInitVertex(Vertex_t* vertex_buffer)
{
    vertex_buffer[0].x = -1.0f;
    vertex_buffer[0].y = -1.0f;
    vertex_buffer[0].z = -1.0f;
    vertex_buffer[0].rgba=0x00ff0000;

    vertex_buffer[1].x =  1.0f;
    vertex_buffer[1].y = -1.0f;
    vertex_buffer[1].z = -1.0f;
    vertex_buffer[1].rgba=0x0000ff00;

    vertex_buffer[2].x = -1.0f;
    vertex_buffer[2].y =  1.0f;
    vertex_buffer[2].z = -1.0f;
    vertex_buffer[2].rgba=0xff000000;
}

/*F********************************************************************************/
/*!
    \Function _T2HostBuildProjection

    \Description
        I'm not sure what this does

    \Input *M   - the resulting matrix
    \Input top    -
    \Input bottom -
    \Input left   -
    \Input right  -
    \Input near   -
    \Input far    -

    \Output void

    \Version 05/25/2006 (tdills)
*/
/********************************************************************************F*/
static void _T2HostBuildProjection(float *M,
        const float top,
        const float bottom,
        const float left,
        const float right,
        const float near,
        const float far)
{
    memset(M, 0, 16*sizeof(float));

    M[0*4+0] = (2.0f*near) / (right - left);
    M[1*4+1] = (2.0f*near) / (bottom - top);

    float A = (right + left) / (right - left);
    float B = (top + bottom) / (top - bottom);
    float C = -(far + near) / (far - near);
    float D = -(2.0f*far*near) / (far - near);

    M[0*4 + 2] = A;
    M[1*4 + 2] = B;
    M[2*4 + 2] = C;
    M[3*4 + 2] = -1.0f;
    M[2*4 + 3] = D;
}

/*F********************************************************************************/
/*!
    \Function _T2HostMatrixTranslate

    \Description
        Constructs a translation matrix with the passed-in coordinates in the 4th
        column and 1s in the 1st column.

    \Input *M   - the resulting matrix
    \Input x    - the x-component of the translation
    \Input y    - the y-component of the translation
    \Input z    - the z-component of the translation

    \Output void

    \Version 05/25/2006 (tdills)
*/
/********************************************************************************F*/
static void _T2HostMatrixTranslate(float *M,
        const float x,
        const float y,
        const float z)
{
    memset(M, 0, sizeof(float)*16);
    M[0*4+3] = x;
    M[1*4+3] = y;
    M[2*4+3] = z;

    M[0*4+0] = 1.0f;
    M[1*4+1] = 1.0f;
    M[2*4+2] = 1.0f;
    M[3*4+3] = 1.0f;
}

/*F********************************************************************************/
/*!
    \Function _T2HostMatrixMul

    \Description
        Cross-multiply 2 4x4 matrices and place the results in the diagol of *Dest

    \Input *Dest- the result
    \Input *A   - the left-hand 4x4 matrix
    \Input *B   - the right-hand 4x4 matrix

    \Output void

    \Version 05/25/2006 (tdills)
*/
/********************************************************************************F*/
static void _T2HostMatrixMul(float *Dest, float *A, float *B)
{
    for (int i=0; i < 4; i++) {
        for (int j=0; j < 4; j++) {
            Dest[i*4+j]
                = A[i*4+0]*B[0*4+j]
                + A[i*4+1]*B[1*4+j]
                + A[i*4+2]*B[2*4+j]
                + A[i*4+3]*B[3*4+j];
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _T2HostUnitMatrix

    \Description
        Set the input 4x4 matrix to a unit matrix (with 1s along the diagonal)

    \Input *M   - the 4x4 matrix to initialize.

    \Output void

    \Version 05/25/2006 (tdills)
*/
/********************************************************************************F*/
static void _T2HostUnitMatrix(float *M)
{
    M[0*4+0] = 1.0f;
    M[0*4+1] = 0.0f;
    M[0*4+2] = 0.0f;
    M[0*4+3] = 0.0f;

    M[1*4+0] = 0.0f;
    M[1*4+1] = 1.0f;
    M[1*4+2] = 0.0f;
    M[1*4+3] = 0.0f;

    M[2*4+0] = 0.0f;
    M[2*4+1] = 0.0f;
    M[2*4+2] = 1.0f;
    M[2*4+3] = 0.0f;

    M[3*4+0] = 0.0f;
    M[3*4+1] = 0.0f;
    M[3*4+2] = 0.0f;
    M[3*4+3] = 1.0f;
}

/*F********************************************************************************/
/*!
    \Function _T2HostSetRenderObject

    \Description
        Create the object to render

    \Output void

    \Version 05/25/2006 (tdills)
*/
/********************************************************************************F*/
static int32_t _T2HostSetRenderObject(void)
{
    static Vertex_t *vertex_buffer;
    void *ret = _T2HostLocalMemAlign(128*1024, 1024);
    vertex_buffer = (Vertex_t*)ret;

    _T2HostInitVertex(vertex_buffer);

    // transform
    float M[16];
    float P[16];
    float V[16];
    float VP[16];
    float MVP[16];

    // projection
    _T2HostBuildProjection(P, -1.0f, 1.0f, -1.0f, 1.0f, 1.0, 10000.0f);

    // 16:9 scale
    _T2HostMatrixTranslate(V, 0.0f, 0.0f, -4.0);
    V[0*4 + 0] = 9.0f / 16.0f;
    V[1*4 + 1] = 1.0f;

    // model view
    _T2HostMatrixMul(VP, P, V);

    _T2HostUnitMatrix(M);
    _T2HostMatrixMul(MVP, VP, M);

    CGparameter model_view_projection
        = cellGcmCgGetNamedParameter(vertex_program, "modelViewProj");
    CGparameter position
        = cellGcmCgGetNamedParameter(vertex_program, "position");
    CGparameter color
        = cellGcmCgGetNamedParameter(vertex_program, "color");
    // get Vertex Attribute index
    uint32_t position_index
        = cellGcmCgGetParameterResource(vertex_program, position) - CG_ATTR0;
    uint32_t color_index
        = cellGcmCgGetParameterResource(vertex_program, color) - CG_ATTR0;

    cellGcmSetVertexProgram(gCellGcmCurrentContext, vertex_program, vertex_program_ucode);

    cellGcmSetVertexProgramParameter(gCellGcmCurrentContext, model_view_projection, MVP);

    // fragment program offset
    uint32_t fragment_offset;
    if(cellGcmAddressToOffset(fragment_program_ucode, &fragment_offset) != CELL_OK)
        return(-1);

    cellGcmSetFragmentProgram(gCellGcmCurrentContext, fragment_program, fragment_offset);

    uint32_t vertex_offset[2];
    if (cellGcmAddressToOffset(&vertex_buffer->x, &vertex_offset[0]) != CELL_OK)
        return(-1);
    if (cellGcmAddressToOffset(&vertex_buffer->rgba, &vertex_offset[1]) != CELL_OK)
        return(-1);

    cellGcmSetVertexDataArray(gCellGcmCurrentContext,
            position_index,
            0,
            sizeof(Vertex_t),
            3,
            CELL_GCM_VERTEX_F,
            CELL_GCM_LOCATION_LOCAL,
            (uint32_t)vertex_offset[0]);
    cellGcmSetVertexDataArray(
            gCellGcmCurrentContext,
            color_index,
            0,
            sizeof(Vertex_t),
            4,
            CELL_GCM_VERTEX_UB,
            CELL_GCM_LOCATION_LOCAL,
            (uint32_t)vertex_offset[1]);

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _T2HostSetRenderTarget

    \Description
        Specify the surface on which to render

    \Output void

    \Version 05/25/2006 (tdills)
*/
/********************************************************************************F*/
static void _T2HostSetRenderTarget(const uint32_t Index)
{
    CellGcmSurface sf;
    sf.colorFormat  = CELL_GCM_SURFACE_A8R8G8B8;
    sf.colorTarget  = CELL_GCM_SURFACE_TARGET_0;
    sf.colorLocation[0] = CELL_GCM_LOCATION_LOCAL;
    sf.colorOffset[0]   = color_offset[Index];
    sf.colorPitch[0]    = color_pitch;

    sf.colorLocation[1] = CELL_GCM_LOCATION_LOCAL;
    sf.colorLocation[2] = CELL_GCM_LOCATION_LOCAL;
    sf.colorLocation[3] = CELL_GCM_LOCATION_LOCAL;
    sf.colorOffset[1]   = 0;
    sf.colorOffset[2]   = 0;
    sf.colorOffset[3]   = 0;
    sf.colorPitch[1]    = 64;
    sf.colorPitch[2]    = 64;
    sf.colorPitch[3]    = 64;

    sf.depthFormat  = CELL_GCM_SURFACE_Z24S8;
    sf.depthLocation    = CELL_GCM_LOCATION_LOCAL;
    sf.depthOffset  = depth_offset;
    sf.depthPitch   = depth_pitch;

    sf.type     = CELL_GCM_SURFACE_PITCH;
    sf.antialias    = CELL_GCM_SURFACE_CENTER_1;

    sf.width        = display_width;
    sf.height       = display_height;
    sf.x        = 0;
    sf.y        = 0;
    cellGcmSetSurface(gCellGcmCurrentContext, &sf);
}

/*F********************************************************************************/
/*!
    \Function _T2HostSnapshotSave

    \Description
        Save a screenshot in RAW format (RGB interleaved)

    \Output void

    \Version 05/25/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _T2HostSnapshotSave(const char *pFileName, uint8_t *pImageData, int32_t iWidth, int32_t iHeight)
{
    uint8_t *pSrc, *pDst;
    int32_t iW, iH;

    ZPrintf("t2host: saving %dx%d screenshot in raw format\n", iWidth, iHeight);

    // convert 32bit ARGB image to 24bit RGB image (inline)
    for (pSrc=pImageData, pDst=pImageData, iH=0; iH < iHeight; iH++)
    {
        for (iW = 0; iW < iWidth; iW++)
        {
            pDst[0] = pSrc[1];
            pDst[1] = pSrc[2];
            pDst[2] = pSrc[3];
            pSrc += 4;
            pDst += 3;
        }
    }

    // write the file
    return(ZFileSave(pFileName, pImageData, iWidth*iHeight*3, ZFILE_OPENFLAG_WRONLY|ZFILE_OPENFLAG_CREATE|ZFILE_OPENFLAG_BINARY));
}

/*F********************************************************************************/
/*!
    \Function _T2HostSnapshotProcess

    \Description
        Save a screenshot in RAW format (RGB interleaved)

    \Output void

    \Version 05/25/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _T2HostSnapshotProcess(int32_t iFrameIndex)
{
    if (strSnapshotName[0] != '\0')
    {
        int32_t iResult;
        uint32_t uMainOffset;
        if ((iResult = cellGcmAddressToOffset(host_addr+(1024*1024), &uMainOffset)) == CELL_OK)
        {
            cellGcmFinish(gCellGcmCurrentContext, 0);
            cellGcmSetTransferData(
                gCellGcmCurrentContext,
                CELL_GCM_TRANSFER_LOCAL_TO_MAIN,    // mode (LOCAL_TO_LOCAL, MAIN_TO_LOCAL, LOCAL_TO_MAIN)
                uMainOffset,                        // main memory buffer to store image to
                display_width*4,                    // dst pitch
                color_offset[iFrameIndex],          // local memory buffer offset to get image from
                color_pitch,                        // src pitch
                display_width*4,                    // bytes per row
                display_height);                    // number of rows
            cellGcmFinish(gCellGcmCurrentContext, 0);
            if ((iResult = _T2HostSnapshotSave(strSnapshotName, host_addr+(1024*1024), display_width, display_height)) < 0)
            {
                ZPrintf("t2host: error %d saving screenshot\n", iResult);
            }
        }

        // clear screenshot name
        strSnapshotName[0] = '\0';
    }
}

/*F********************************************************************************/
/*!
    \Function _T2HostWaitFlip

    \Description
        Waits for cellGcmSetFlip to complete

    \Output void

    \Version 05/25/2006 (tdills)
*/
/********************************************************************************F*/
static void _T2HostWaitFlip(void)
{
    while (cellGcmGetFlipStatus()!=0){
        sys_timer_usleep(300ULL);
    }
    cellGcmResetFlipStatus();
}


/*F********************************************************************************/
/*!
    \Function _T2HostFlip

    \Description
        Flips the video frames

    \Output void

    \Version 05/25/2006 (tdills)
*/
/********************************************************************************F*/
static void _T2HostFlip(void)
{
    static int first=1;

    // wait until the previous flip executed
    if (first!=1) _T2HostWaitFlip();

    if(cellGcmSetFlip(gCellGcmCurrentContext, frame_index) != CELL_OK) return;
    cellGcmFlush(gCellGcmCurrentContext);

    cellGcmSetWaitFlip(gCellGcmCurrentContext);

    // take a screenshot if requested
    _T2HostSnapshotProcess(frame_index);

    // New render target
    frame_index = (frame_index+1)%COLOR_BUFFER_NUM;
    _T2HostSetRenderTarget(frame_index);

    first=0;
}


/*** Public Functions ******************************************************************/


/*F********************************************************************************/
/*!
    \Function main

    \Description
        Main routine for PS2 T2Host application.

    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Output int32_t - zero

    \Version 03/22/2006 (tdills)
*/
/********************************************************************************F*/
int main(int32_t argc, char *argv[])
{
    char strBase[128] = "", strHostName[128] = "", strParams[512];
    T2HostAppT T2App;
    int32_t iArg;

    // load required system utilities
    _T2HostLoadSysUtils();

    // get arguments
    for (iArg = 0; iArg < argc; iArg++)
    {
        if (!strncmp(argv[iArg], "-path=", 6))
        {
            strnzcpy(strBase, &argv[iArg][6], sizeof(strBase));
            ZPrintf("t2host: base path=%s\n", strBase);
        }
        if (!strncmp(argv[iArg], "-connect=", 9))
        {
            strnzcpy(strHostName, &argv[iArg][9], sizeof(strHostName));
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

    // startup dirtysdk (must come before TesterHostCoreCreate() if we are using socket comm)
    NetConnStartup("");

    // create tester2 host core
    g_pHostCore = TesterHostCoreCreate(strParams);

    // connect to the network (blocking; waits for connection success)
    _T2HostNetConnect(g_pHostCore);

    // register some basic commands
    TesterHostCoreRegister(g_pHostCore, "exit", _T2HostCmdExit);
    TesterHostCoreRegister(g_pHostCore, "snap", _T2HostCmdSnapshot);

    host_addr = memalign(1024*1024, HOST_SIZE);
    ZPrintf("host_addr allocated at 0x%08x\n", host_addr);

    ZPrintf("initializing cellGcm\n");

    // initialize PS3 Graphics module
    if (cellGcmInit(0x10000, HOST_SIZE, host_addr) != CELL_OK)
    {
        ZPrintf("t2host: could not initialize PS3 libgcm\n");
    }

    ZPrintf("initializing display\n");
    // initialize the PS3 display
    if (_T2HostInitDisplay()!=0)
    {
        ZPrintf("t2host: could not initialize PS3 display\n");
    }

    ZPrintf("initializing shader\n");

    _T2HostInitShader();

    ZPrintf("setting up render object\n");

    if (_T2HostSetRenderObject())
    {
        ZPrintf("t2host: _T2HostSetRenderObject failed\n");
    }

    ZPrintf("setting render target\n");
    _T2HostSetRenderTarget(frame_index);

    // set clear color
    cellGcmSetClearColor(gCellGcmCurrentContext, 0xff00ffff);

    // init app structure
    T2App.pCore = g_pHostCore;

    ZPrintf("entering polling loop\n");
    // burn some time
    while(iContinue)
    {
        // clear frame buffer
        cellGcmSetClearSurface(gCellGcmCurrentContext,
                CELL_GCM_CLEAR_Z|
                CELL_GCM_CLEAR_R|
                CELL_GCM_CLEAR_G|
                CELL_GCM_CLEAR_B|
                CELL_GCM_CLEAR_A);
        // set draw command
        cellGcmSetDrawArrays(gCellGcmCurrentContext, CELL_GCM_PRIMITIVE_TRIANGLES, 0, 3);

        // start reading the command buffer
        _T2HostFlip();

        _T2HostUpdate(&T2App);
    }

    // clean up and exit
    TesterHostCoreDisconnect(g_pHostCore);
    TesterHostCoreDestroy(g_pHostCore);

    // shut down the network
    NetConnShutdown(0);

    ZMemtrackShutdown();

    cellSysutilUnregisterCallback(3);

    ZPrintf("\nQuitting T2Host.\n\n");
    return(0);
}

