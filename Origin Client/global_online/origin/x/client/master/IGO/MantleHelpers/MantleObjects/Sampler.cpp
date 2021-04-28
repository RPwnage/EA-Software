#include "Sampler.h"
#include "..\MantleAppWsi.h"

Sampler::~Sampler()
{
}

void Sampler::dealloc(GR_CMD_BUFFER cmd)
{
	if (GR_NULL_HANDLE != m_sampler) _grDestroyObjectInternal(m_sampler, cmd);
}

GR_RESULT Sampler::init(GR_UINT gpu, bool pointSampling)
{
    GR_RESULT result = GR_SUCCESS;

    GR_SAMPLER_CREATE_INFO samInfo = {};
	samInfo.filter = pointSampling ? GR_TEX_FILTER_MAG_POINT_MIN_POINT_MIP_POINT : GR_TEX_FILTER_MAG_LINEAR_MIN_LINEAR_MIP_LINEAR;
    samInfo.addressU      = GR_TEX_ADDRESS_CLAMP;
    samInfo.addressV      = GR_TEX_ADDRESS_CLAMP;
    samInfo.addressW      = GR_TEX_ADDRESS_CLAMP;
    samInfo.compareFunc   = GR_COMPARE_ALWAYS;
    samInfo.maxAnisotropy = 1;
    samInfo.borderColor   = GR_BORDER_COLOR_TYPE_BEGIN_RANGE;
    samInfo.minLod = 0.f;
    samInfo.maxLod = 16.f;

    result = _grCreateSampler(OriginIGO::getMantleDevice(gpu), &samInfo, &m_sampler);

    return result;
}

