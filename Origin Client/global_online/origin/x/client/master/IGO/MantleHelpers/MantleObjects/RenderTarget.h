#pragma once
#include <mantle.h>
#include "Image.h"

class CommandBuffer;

class RenderTarget : public Image
{
public:
	RenderTarget(const GR_IMAGE_STATE, const GR_IMAGE_STATE, const GR_IMAGE, const GR_GPU_MEMORY, const GR_COLOR_TARGET_VIEW, GR_FORMAT fmt, GR_UINT gpu);
	void update(const GR_IMAGE_STATE, const GR_IMAGE_STATE, const GR_IMAGE img, const GR_GPU_MEMORY mem, const GR_COLOR_TARGET_VIEW targetView, GR_FORMAT fmt);

	RenderTarget( GR_UINT32 w, GR_UINT32 h, GR_FORMAT fmt, GR_ENUM usage, GR_UINT gpu );

    virtual ~RenderTarget();
private:
};