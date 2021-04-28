#pragma once
#include <mantle.h>

class CommandBuffer;

class Image
{
public:
    Image( GR_UINT gpu );

    virtual ~Image();

	void						dealloc(GR_CMD_BUFFER cmdBuffer);

    GR_RESULT                   init( GR_UINT32 heap, GR_UINT32 w, GR_UINT32 h, GR_FORMAT fmt, GR_ENUM usage, GR_UINT32 mipCount, bool cpuAccess = false);

    void                        clear( CommandBuffer*, float[4] );
	void						syncState(GR_IMAGE_STATE state);

	void                        prepare( CommandBuffer*, GR_IMAGE_STATE );
	void						prepareAfterPresent(CommandBuffer* cmdBuffer);

    GR_IMAGE                    image() const       {return m_image;}
    GR_COLOR_TARGET_VIEW        targetView()const   {return m_targetView;}
    GR_IMAGE_VIEW_ATTACH_INFO   imageView()const    {return m_imageInfo;}
    const GR_GPU_MEMORY&        memory() const      {return m_memory;}
    const GR_GPU_SIZE&          offset() const      {return m_offset;}

    GR_IMAGE_STATE              state() const       {return (GR_IMAGE_STATE)m_imageInfo.state;}
    GR_FORMAT                   format() const      {return m_format;}
    GR_UINT32                   width() const       {return m_width;}
    GR_UINT32                   height() const      {return m_height;}
    GR_UINT32                   mipCount() const    {return m_mipCount;}

protected:
    GR_UINT                     m_gpu;

    GR_IMAGE                    m_image;
    GR_COLOR_TARGET_VIEW        m_targetView;
    GR_IMAGE_VIEW_ATTACH_INFO   m_imageInfo;
	GR_IMAGE_STATE				m_stateAfterPresent;

    GR_GPU_MEMORY               m_memory;
    GR_GPU_SIZE                 m_offset;
    GR_FORMAT                   m_format;

    GR_UINT32                   m_width;
    GR_UINT32                   m_height;
    GR_UINT32                   m_mipCount;

	bool						m_freeMemory;
};