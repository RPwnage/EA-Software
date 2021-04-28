///////////////////////////////////////////////////////////////////////////////
// IGOSurface.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGO.h"
#include "IGOSurface.h"

namespace OriginIGO {

    void IGOSurface::dumpInfo(char* buffer, size_t bufferSize)
    {
        if (buffer && bufferSize > 0)
            buffer[0] = '\0';
    }
    
    void IGOSurface::copySurfaceData(const void *src, void *dst, uint32_t width, uint32_t height, uint32_t pitch)
    {
        if (pitch == width*4)
        {
            CopyMemory(dst, src, 4*width*height);
        }
        else
        {                    
            uint32_t lineWidth = 4*width;
            char *dstPtr = (char *)dst;
            char *srcPtr = (char *)src;
            for (uint32_t i = 0; i < height; i++)
            {
                CopyMemory(dstPtr, srcPtr, lineWidth);
                dstPtr += pitch;
                srcPtr += lineWidth;
            }
        }
    }
}