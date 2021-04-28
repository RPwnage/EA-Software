#pragma once
#include <mantle.h>

class Sampler
{
public:
    virtual ~Sampler();
	void dealloc(GR_CMD_BUFFER cmd);

	GR_RESULT init(GR_UINT gpu, bool pointSampling);
    const GR_SAMPLER&  sampler()
    {
        return m_sampler;
    }
private:
    GR_SAMPLER  m_sampler;
};