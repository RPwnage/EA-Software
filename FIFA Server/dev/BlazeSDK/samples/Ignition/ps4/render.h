/*H********************************************************************************/
/*!
    \File render.h

    \Description
        Sample app rendering

    \Copyright
        Copyright (c) Electronic Arts 2004.  ALL RIGHTS RESERVED.

    \Version    1.0 06/04/2013 (cvienneau) First Version
*/
/********************************************************************************H*/
#ifndef _T2Render_h
#define _T2Render_h
#ifdef EA_PLATFORM_PS4

/*** Include files ****************************************************************/
#include "DirtySDK/dirtysock.h"
#include <kernel.h>

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/
typedef struct T2Render
{
    int32_t iVideoOut;
    int32_t ibufferIndex;
    SceKernelEqueue eqFlip;
    int64_t flipArg;
    int32_t loop;
} T2Render;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

int32_t T2RenderInit(T2Render *pApp);
int32_t T2RenderUpdate(T2Render *pApp);
int32_t T2RenderTerm(T2Render *pApp);

#ifdef __cplusplus
};
#endif

#endif // EA_PLATFORM_PS4
#endif //_T2Render_h
