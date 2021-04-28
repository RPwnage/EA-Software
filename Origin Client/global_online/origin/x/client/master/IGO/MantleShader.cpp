///////////////////////////////////////////////////////////////////////////////
// MantleShader.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2013 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifdef ORIGIN_PC

#include "MantleShader.h"

namespace OriginIGO {


    GR_PIPELINE_SHADER gEmptyShader =
    {
        GR_NULL_HANDLE,
        {
            { 0, GR_NULL_HANDLE },    // to be filled at pipeline generation time
            { 0, GR_NULL_HANDLE }
        },
            0, //linkConstBufferCount
            NULL, //pLinkConstBufferInfo
            {
            GR_SLOT_UNUSED,     // GR_DESCRIPTOR_SET_SLOT_TYPE
            0 //GR_UINT shaderEntityIndex;
        } // dynamicBufferMapping
    };

    GR_GRAPHICS_PIPELINE_CREATE_INFO gPipelineInfo =
    {
        gEmptyShader, //vs
        gEmptyShader, //hs
        gEmptyShader, //ds
        gEmptyShader, //gs
        gEmptyShader, //ps
        { GR_TOPOLOGY_TRIANGLE_STRIP, GR_FALSE }, // iaState
        {}, //tessState;
        { GR_FALSE }, // rsState
        {
            GR_FALSE,
            GR_FALSE,
            GR_LOGIC_OP_COPY,
            {
                {
                    GR_TRUE,
                    { GR_CH_FMT_R8G8B8A8, GR_NUM_FMT_UNORM },
                    0xF
                }
            }
        },  // cbState
        {}, //dbState;
        0   //flags;
    };

    ILC_VS_INPUT_SLOT gVsInputSlots[2] =
    {
        {    // x,y,z,w
            0,                              // UINT               dxInputRegister;
            0,                              // UINT               vbSlot;
            0,                              // UINT               vertexOffset;
            ILC_FORMAT_R32G32B32A32_FLOAT,  // ILC_FORMAT         format;
            ILC_INPUT_PER_VERTEX_DATA,      // ILC_VS_INPUT_CLASS inputClass;
            0,                              // UINT               instanceDataStepRate;
        },
        {    // u,v
            1,                              // UINT               dxInputRegister;
            0,                              // UINT               vbSlot;
            16,                             // UINT               vertexOffset;
            ILC_FORMAT_R32G32_FLOAT,        // ILC_FORMAT         format;
            ILC_INPUT_PER_VERTEX_DATA,      // ILC_VS_INPUT_CLASS inputClass;
            0,                              // UINT               instanceDataStepRate;
        }
    };

    ILC_VS_INPUT_LAYOUT gVbLayout =
    {
        sizeof(gVsInputSlots) / sizeof(gVsInputSlots[0]), gVsInputSlots
    };



} // OriginIGO

#endif // PC
