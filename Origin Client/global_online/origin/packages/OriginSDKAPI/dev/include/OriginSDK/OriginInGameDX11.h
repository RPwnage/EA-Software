#pragma once

#include <d3d11.h>
#include "OriginInGame.h"

#pragma pack(push, 1)   // pack structures

/// \ingroup types
/// \brief Detailed information about our DirectX 11 IGO rendering context.
struct OriginIGORenderingContext_DX11_T
{
    OriginIGORenderingContextT base;

    ID3D11Device* renderDevice;
    ID3D11Texture2D* renderTarget;   ///< The surface that IGO will be rendered on to (example: Final game screen).
    ID3D11Texture2D* broadcastSurface;    ///< The surface that will be used as the broadcasting source (example: Final game screen without IGO).
    ID3D11RenderTargetView* renderTargetView;   ///< The view that IGO will be rendered on to (example: Final game screen).
    ID3D11RenderTargetView* broadcastSurfaceView;    ///< The view that will be used as the broadcasting source (example: Final game screen without IGO).
    bool saveAndRestoreGFXStates;   ///< IGO will save and restore GFX API states it changes.

    ///< additions to those structs have to follow down here, it is forbidden to remove elements, due to size check invalidations!
};
 
#pragma pack(pop)