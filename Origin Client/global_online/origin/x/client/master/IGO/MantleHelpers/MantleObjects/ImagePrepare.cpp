#include "ImagePrepare.h"
#include "RenderTarget.h"

ImagePrepare::ImagePrepare(GR_IMAGE_STATE newState, Image* pImage, int priority) :
    DrawCommand(priority),
    m_state(newState),
    m_image(pImage)
{
}

GR_RESULT ImagePrepare::execute( CommandBuffer* cmdBuffer, RenderPass*)
{
    m_image->prepare( cmdBuffer, m_state);
    return GR_SUCCESS;
}

unsigned int ImagePrepare::countCommandsRec(DRAW_CALL_TYPE dct)
{
    return dct == DCT_PREPARE ? 1 : 0;
}

GR_RESULT ImagePrepare::updateImage( Image* pImage )
{
    m_image = pImage;
    return GR_SUCCESS;
}