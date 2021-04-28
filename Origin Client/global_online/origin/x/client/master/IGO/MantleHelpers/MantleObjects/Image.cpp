#include "Image.h"
#include "../MantleAppWsi.h"
#include "GpuMemMgr.h"
#include "CommandBuffer.h"
#include "../../HookAPI.h"
#include "../../IGOLogger.h"
#include "Defines.h"

namespace OriginIGO{
	EXTERN_NEXT_FUNC(GR_VOID, grCmdPrepareImagesHook, (GR_CMD_BUFFER cmdBuffer, GR_UINT transitionCount, const GR_IMAGE_STATE_TRANSITION* pStateTransitions));
	EXTERN_NEXT_FUNC(GR_RESULT, grCreateColorTargetViewHook, (GR_DEVICE device, const GR_COLOR_TARGET_VIEW_CREATE_INFO* pCreateInfo, GR_COLOR_TARGET_VIEW* pView));
	EXTERN_NEXT_FUNC(GR_RESULT, grCreateImageHook, (GR_DEVICE device, const GR_IMAGE_CREATE_INFO* pCreateInfo, GR_IMAGE* pImage));


};


Image::Image(GR_UINT gpu) :
    m_gpu(gpu),
    m_memory(GR_NULL_HANDLE), 
    m_image(GR_NULL_HANDLE),
    m_targetView(GR_NULL_HANDLE),
	m_freeMemory(false)
{
    m_imageInfo.view = GR_NULL_HANDLE;
    m_imageInfo.state = GR_IMAGE_STATE_UNINITIALIZED_TARGET;
}

GR_RESULT Image::init( GR_UINT32 heap, GR_UINT32 w, GR_UINT32 h, GR_FORMAT fmt, GR_ENUM usage, GR_UINT32 mipCount, bool cpuAccess )
{
    m_format = fmt;
    m_width = w;
    m_height = h;
    m_mipCount = mipCount;
	
	m_freeMemory = true;

    if( m_mipCount==0 )
    {
        while( (w>0) || (h>0) )
        {
            w>>=1; h>>=1; m_mipCount++;
        }
    }

    GR_RESULT result = GR_SUCCESS;
    
    GR_IMAGE_CREATE_INFO imageCreateInfo={};    
    imageCreateInfo.imageType = GR_IMAGE_2D;
    imageCreateInfo.format = fmt;
    imageCreateInfo.arraySize = 1;
    imageCreateInfo.extent.width = m_width;
    imageCreateInfo.extent.height = m_height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = m_mipCount;
    imageCreateInfo.samples = 1;
    imageCreateInfo.tiling = cpuAccess ? GR_LINEAR_TILING : GR_OPTIMAL_TILING;
    imageCreateInfo.usage = usage;
	result = OriginIGO::grCreateImageHookNext(OriginIGO::getMantleDevice(m_gpu), &imageCreateInfo, &m_image);

    if (result == GR_SUCCESS)
    {
        result = GpuMemMgr::Instance(m_gpu)->allocAndBindGpuMem( heap, m_image, &m_memory, &m_offset);
    }

    if (result == GR_SUCCESS && (usage&GR_IMAGE_USAGE_COLOR_TARGET))
    {
        GR_COLOR_TARGET_VIEW_CREATE_INFO viewInfo = {};
		viewInfo.format				  = fmt;
		viewInfo.image                = m_image;
        viewInfo.mipLevel             = 0;
        viewInfo.baseArraySlice       = 0;
        viewInfo.arraySize            = 1;

        result = OriginIGO::grCreateColorTargetViewHookNext(OriginIGO::getMantleDevice(m_gpu), &viewInfo, &m_targetView );
    }
    else
    {
        m_targetView = NULL;
    }
     
	if (result == GR_SUCCESS && ((usage & GR_IMAGE_USAGE_SHADER_ACCESS_WRITE) || usage & GR_IMAGE_USAGE_SHADER_ACCESS_READ))
    {
        GR_IMAGE_VIEW_CREATE_INFO viewInfo = {};
        viewInfo.image                = m_image;
        viewInfo.viewType             = GR_IMAGE_VIEW_2D;
        viewInfo.format               = fmt;
        viewInfo.channels.r           = GR_CHANNEL_SWIZZLE_R;
        viewInfo.channels.g           = GR_CHANNEL_SWIZZLE_G;
        viewInfo.channels.b           = GR_CHANNEL_SWIZZLE_B;
        viewInfo.channels.a           = GR_CHANNEL_SWIZZLE_A;
		viewInfo.minLod				  = 0;
		viewInfo.subresourceRange.baseArraySlice = 0;
		viewInfo.subresourceRange.arraySize = 1;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.mipLevels = 1;
		viewInfo.subresourceRange.aspect = GR_IMAGE_ASPECT_COLOR;

        result = _grCreateImageView(OriginIGO::getMantleDevice(m_gpu), &viewInfo, &m_imageInfo.view );
        m_imageInfo.state = GR_IMAGE_STATE_UNINITIALIZED_TARGET;
    }

    return result;
}

void Image::dealloc(GR_CMD_BUFFER cmdBuffer)
{
	if (m_freeMemory)
	{
		SAFE_FREE(m_memory, cmdBuffer);
		SAFE_DESTROY(m_image, cmdBuffer);
        SAFE_DESTROY(m_targetView, cmdBuffer);
        SAFE_DESTROY(m_imageInfo.view, cmdBuffer);
	}
}

Image::~Image()
{
	m_memory = GR_NULL_HANDLE;
	m_image = GR_NULL_HANDLE;
	m_targetView = GR_NULL_HANDLE;
	m_imageInfo.view = GR_NULL_HANDLE;
	m_imageInfo.state = GR_IMAGE_STATE_UNINITIALIZED_TARGET;
}

void Image::clear( CommandBuffer* cmdBuffer, float clearColor[4] )
{
    GR_IMAGE_STATE oldState = (GR_IMAGE_STATE)m_imageInfo.state;

    prepare(cmdBuffer, GR_IMAGE_STATE_CLEAR);

	const GR_IMAGE_SUBRESOURCE_RANGE r = { GR_IMAGE_ASPECT_COLOR, 0, 1, 0, 1 };
	
    _grCmdClearColorImage(cmdBuffer->cmdBuffer(), m_image, clearColor, 1, &r);

    prepare(cmdBuffer, oldState);
}


void Image::syncState(GR_IMAGE_STATE state)
{
	OriginIGO::IGOLogWarn("Image::syncState rendertarget im %p os %x ns %x", m_image, m_imageInfo.state, state);
	m_imageInfo.state = state;
}

void Image::prepare( CommandBuffer* cmdBuffer, GR_IMAGE_STATE state )
{
	if (m_imageInfo.state == state)
		return;

    GR_IMAGE_STATE_TRANSITION transition = 
    {
        m_image,
        m_imageInfo.state,
        state,
		GR_IMAGE_ASPECT_COLOR,
		0,
		1,
		0,
		1
    };

	OriginIGO::IGOLogWarn("Image::prepare rendertarget im %p os %x ns %x", m_image, m_imageInfo.state, state);


	IGO_ASSERT(OriginIGO::grCmdPrepareImagesHookNext != NULL);

	/*_grCmdPrepareImages*/OriginIGO::grCmdPrepareImagesHookNext(cmdBuffer->cmdBuffer(), 1, &transition);
    m_imageInfo.state = state;

    GR_MEMORY_REF memRef;
    memRef.mem = m_memory;
    memRef.flags = 0;

    OriginIGO::IGOLogDebug("Image::prepare %p", memRef.mem);

    cmdBuffer->addMemReferences(1, &memRef);
}

void Image::prepareAfterPresent(CommandBuffer* cmdBuffer)
{
	prepare(cmdBuffer, m_stateAfterPresent);
}