#include "RenderTarget.h"
#include "..\MantleAppWsi.h"
#include "GpuMemMgr.h"
#include "CommandBuffer.h"

RenderTarget::RenderTarget(const GR_IMAGE_STATE state, const GR_IMAGE_STATE stateAfterPresent, const GR_IMAGE img, const GR_GPU_MEMORY mem, const GR_COLOR_TARGET_VIEW targetView, GR_FORMAT fmt, GR_UINT gpu) :
    Image(gpu)
{
    m_image = img;
    m_targetView = targetView;
    m_memory = mem;
    m_format = fmt;

    m_imageInfo.view = NULL;
	m_imageInfo.state = state; // GR_IMAGE_STATE_UNINITIALIZED_TARGET;
	m_stateAfterPresent = stateAfterPresent;
}

void RenderTarget::update(const GR_IMAGE_STATE state, const GR_IMAGE_STATE stateAfterPresent, const GR_IMAGE img, const GR_GPU_MEMORY mem, const GR_COLOR_TARGET_VIEW targetView, GR_FORMAT fmt)
{
	m_image = img;
	m_targetView = targetView;
	m_memory = mem;
	m_format = fmt;

	m_imageInfo.view = NULL;
	m_imageInfo.state = state; // GR_IMAGE_STATE_UNINITIALIZED_TARGET;
	m_stateAfterPresent = stateAfterPresent;
}


RenderTarget::RenderTarget( GR_UINT32 w, GR_UINT32 h, GR_FORMAT fmt, GR_ENUM usage, GR_UINT gpu ) :
    Image(gpu)
{
    init( 0, w, h, fmt, usage, 1, false );
}

RenderTarget::~RenderTarget()
{
}
