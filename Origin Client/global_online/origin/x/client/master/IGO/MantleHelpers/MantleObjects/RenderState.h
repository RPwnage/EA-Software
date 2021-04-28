#include <mantle.h>

class CommandBuffer;

class RenderState
{
public:
    RenderState();
    virtual ~RenderState();
	
	void dealloc(GR_CMD_BUFFER cmd);

	GR_RESULT init(bool blendEnable, GR_FLOAT width, GR_FLOAT height, GR_UINT gpu);
    void bind( CommandBuffer* cmdBuffer );

private:
    GR_VIEWPORT_STATE_OBJECT      m_stateViewport;
    GR_RASTER_STATE_OBJECT        m_stateRaster;
    GR_DEPTH_STENCIL_STATE_OBJECT m_stateDepthStencil;
    GR_COLOR_BLEND_STATE_OBJECT   m_stateColorBlend;
    GR_MSAA_STATE_OBJECT          m_stateMsaa;
};