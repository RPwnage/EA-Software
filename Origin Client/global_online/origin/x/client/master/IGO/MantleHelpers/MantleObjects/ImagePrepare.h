#pragma once

#include <mantle.h>
#include "DrawCommand.h"

class Image;

class ImagePrepare : public DrawCommand
{
public:
    ImagePrepare(GR_IMAGE_STATE state, Image* pImage, int priority = 0);

    GR_RESULT           execute( CommandBuffer*, RenderPass* );
    unsigned int        countCommandsRec(DRAW_CALL_TYPE);

    GR_RESULT           updateImage( Image* pImage );

private:
    Image*          m_image;
    GR_IMAGE_STATE  m_state;
};