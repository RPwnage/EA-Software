#pragma once

#include <gl/gl.h>
#include "OriginInGame.h"

#pragma pack(push, 1)   // pack structures

/// \ingroup types
/// \brief Detailed information about our OpenGL IGO rendering context.
struct OriginIGORenderingContext_OGL_T {

    OriginIGORenderingContextT base;

    HDC* deviceContext;
    HGLRC *renderDeviceContext;
    GLenum renderTarget;                ///< The surface that IGO will be rendered on to (example: Final game screen, typically GL_BACK).
    GLenum broadcastSurface;            ///< The surface that will be used as the broadcasting source (example: Final game screen without IGO, typically GL_BACK).
    bool saveAndRestoreGFXStates;       ///< IGO will save and restore GFX API states it changes.
                                        ///< additions to those structs have to follow down here, it is forbidden to remove elements, due to size check invalidations!
};

#pragma pack(pop)
