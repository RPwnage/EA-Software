#include "../MantleAppWsi.h"
#include "RenderState.h"
#include "CommandBuffer.h"

RenderState::RenderState() :
    m_stateViewport( GR_NULL_HANDLE ),
    m_stateRaster( GR_NULL_HANDLE ),
    m_stateColorBlend( GR_NULL_HANDLE ),
    m_stateDepthStencil( GR_NULL_HANDLE ),
    m_stateMsaa( GR_NULL_HANDLE )
{
}

RenderState::~RenderState()
{
}

void RenderState::dealloc(GR_CMD_BUFFER cmd)
{
	if (GR_NULL_HANDLE != m_stateViewport) _grDestroyObjectInternal(m_stateViewport, cmd);
	if (GR_NULL_HANDLE != m_stateRaster) _grDestroyObjectInternal(m_stateRaster, cmd);
	if (GR_NULL_HANDLE != m_stateColorBlend) _grDestroyObjectInternal(m_stateColorBlend, cmd);
	if (GR_NULL_HANDLE != m_stateDepthStencil) _grDestroyObjectInternal(m_stateDepthStencil, cmd);
	if (GR_NULL_HANDLE != m_stateMsaa) _grDestroyObjectInternal(m_stateMsaa, cmd);
}

GR_RESULT RenderState::init(bool blendEnable, GR_FLOAT width, GR_FLOAT height, GR_UINT gpu)
{
    GR_RESULT result = GR_SUCCESS;
    
    if (result == GR_SUCCESS)
    {
        GR_RASTER_STATE_CREATE_INFO raster = {};
        raster.fillMode = GR_FILL_SOLID;
		raster.cullMode = GR_CULL_NONE;
        raster.frontFace = GR_FRONT_FACE_CCW;

        result = _grCreateRasterState(OriginIGO::getMantleDevice(gpu), &raster, &m_stateRaster);
    }

    if (result == GR_SUCCESS)
    {
        GR_VIEWPORT_STATE_CREATE_INFO viewport = {};
        viewport.viewportCount         = 1;
        viewport.scissorEnable         = GR_FALSE;
        viewport.viewports[0].originX  = 0;
        viewport.viewports[0].originY  = 0;
		viewport.viewports[0].width = width;
		viewport.viewports[0].height = height;
        viewport.viewports[0].minDepth = 0.f;
        viewport.viewports[0].maxDepth = 1.f;

        result = _grCreateViewportState(OriginIGO::getMantleDevice(gpu), &viewport, &m_stateViewport);
    }

    if (result == GR_SUCCESS)
    {
        GR_DEPTH_STENCIL_STATE_CREATE_INFO depthStencil = {};
        depthStencil.depthEnable      = GR_FALSE;
		depthStencil.depthWriteEnable = GR_FALSE;
        depthStencil.depthFunc = GR_COMPARE_LESS_EQUAL;
        depthStencil.depthBoundsEnable = GR_FALSE;
        depthStencil.minDepth = 0.f;
        depthStencil.maxDepth = 1.f;
        depthStencil.back.stencilDepthFailOp = GR_STENCIL_OP_KEEP;
        depthStencil.back.stencilFailOp = GR_STENCIL_OP_KEEP;
        depthStencil.back.stencilPassOp = GR_STENCIL_OP_KEEP;
        depthStencil.back.stencilRef = 0x00;
        depthStencil.back.stencilFunc = GR_COMPARE_ALWAYS;
        depthStencil.front = depthStencil.back;

        result = _grCreateDepthStencilState(OriginIGO::getMantleDevice(gpu), &depthStencil, &m_stateDepthStencil);
    }

    if (result == GR_SUCCESS)
    {
        GR_COLOR_BLEND_STATE_CREATE_INFO colorBlend = {};
        colorBlend.target[0].blendEnable = blendEnable;
		colorBlend.target[0].srcBlendColor = blendEnable ? GR_BLEND_SRC_ALPHA : GR_BLEND_ONE;
		colorBlend.target[0].destBlendColor = blendEnable ? GR_BLEND_ONE_MINUS_SRC_ALPHA : GR_BLEND_ZERO;
		colorBlend.target[0].blendFuncColor = GR_BLEND_FUNC_ADD;
		colorBlend.target[0].blendFuncAlpha = GR_BLEND_FUNC_ADD;
		colorBlend.target[0].srcBlendAlpha = blendEnable ? GR_BLEND_ONE_MINUS_DEST_ALPHA : GR_BLEND_ONE;
		colorBlend.target[0].destBlendAlpha = blendEnable ? GR_BLEND_ONE : GR_BLEND_ZERO;
        colorBlend.target[1] = colorBlend.target[0];
        colorBlend.target[2] = colorBlend.target[0];
        colorBlend.target[3] = colorBlend.target[0];
        colorBlend.target[4] = colorBlend.target[0];
        colorBlend.target[5] = colorBlend.target[0];
        colorBlend.target[6] = colorBlend.target[0];
        colorBlend.target[7] = colorBlend.target[0];

        result = _grCreateColorBlendState(OriginIGO::getMantleDevice(gpu), &colorBlend, &m_stateColorBlend);
    }

    if (result == GR_SUCCESS)
    {
        GR_MSAA_STATE_CREATE_INFO msaa = {};
        msaa.sampleMask = 1;
        msaa.samples = 1;

        result = _grCreateMsaaState(OriginIGO::getMantleDevice(gpu), &msaa, &m_stateMsaa);
    }

    return result;
}

void RenderState::bind( CommandBuffer* cmdBuffer )
{
        _grCmdBindStateObject(cmdBuffer->cmdBuffer(),
                             GR_STATE_BIND_COLOR_BLEND,
                             m_stateColorBlend);
        _grCmdBindStateObject(cmdBuffer->cmdBuffer(),
                             GR_STATE_BIND_RASTER,
                             m_stateRaster);
        _grCmdBindStateObject(cmdBuffer->cmdBuffer(),
                             GR_STATE_BIND_VIEWPORT,
                             m_stateViewport);
        _grCmdBindStateObject(cmdBuffer->cmdBuffer(),
                             GR_STATE_BIND_DEPTH_STENCIL,
                             m_stateDepthStencil);
        _grCmdBindStateObject(cmdBuffer->cmdBuffer(),
                             GR_STATE_BIND_MSAA,
                             m_stateMsaa);
}