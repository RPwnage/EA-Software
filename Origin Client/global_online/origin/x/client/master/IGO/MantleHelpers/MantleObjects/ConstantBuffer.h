#pragma once
#include <mantle.h>

class CommandBuffer;

class ConstantBuffer
{
public:
    ConstantBuffer();
    virtual ~ConstantBuffer();
	void	dealloc(GR_CMD_BUFFER cmd);
    void    init( CommandBuffer* cmdBuffer, const GR_GPU_SIZE stride, const GR_GPU_SIZE byteSize, const void* data, GR_UINT gpu );
    void    Prepare( CommandBuffer* cmdBuffer, GR_MEMORY_STATE state );

    GR_MEMORY_VIEW_ATTACH_INFO view() const
    {
        return m_bufferView;
    }
    GR_GPU_MEMORY memory() const
    {
        return m_bufferMem;
    }
    GR_GPU_SIZE offset() const
    {
        return m_bufferOffset;
    }
    GR_GPU_SIZE   size() const
    {
        return m_bufferSize;
    }
private:
    GR_UINT                         m_gpu;
    GR_MEMORY_VIEW_ATTACH_INFO      m_bufferView;
    GR_GPU_MEMORY                   m_bufferMem;
    GR_GPU_SIZE                     m_bufferSize;
    GR_GPU_SIZE                     m_bufferOffset;
};