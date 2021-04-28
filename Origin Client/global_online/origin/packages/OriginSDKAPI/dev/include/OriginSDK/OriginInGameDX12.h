#pragma once

#include <d3d12.h>
#include "OriginInGame.h"

#pragma pack(push, 1)   // pack structures

// TODO: add multi-GPU support

/// \ingroup types
/// \brief Detailed information about our DirectX 12 IGO rendering context.
struct OriginIGORenderingContext_DX12_T
{
    OriginIGORenderingContextT base;

    ID3D12Device* renderDevice;
    ID3D12Resource* renderTarget;   ///< The surface that IGO will be rendered on to (example: Final game screen).
    ID3D12Resource* broadcastSurface;    ///< The surface that will be used as the broadcasting source (example: Final game screen without IGO).
    ID3D12CommandQueue *queue;          ///< GPU queue where IGO will be executed
    
    ///< additions to those structs have to follow down here, it is forbidden to remove elements, due to size check invalidations!
};
 
#pragma pack(pop)