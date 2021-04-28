///////////////////////////////////////////////////////////////////////////////
// IGOSurface.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGOSURFACE_H
#define IGOSURFACE_H

#include "EABase/eabase.h"
#include "EASTL/vector.h"
#include "IGO.h"
#include "IGOIPC/IGOIPC.h"

namespace OriginIGO {

    class IGOSurface
    {
    public:
	    IGOSurface() { }
	    virtual ~IGOSurface() { }
		virtual void dealloc(void *context) {};
		virtual void render(void *context) = 0;
		virtual void unrender(void *context) {};
        virtual void dumpInfo(char* buffer, size_t bufferSize);
		virtual void dumpBitmap(void *context) { };
#ifdef ORIGIN_MAC
        virtual bool isValid() const = 0;
#endif
		virtual void update(void *context, uint8_t alpha, int32_t width, int32_t height, const char *data, int size, uint32_t flags) = 0;
		virtual void updateRects(void *context, const eastl::vector<IGORECT> &rects, const char *data, int size) { };

    protected:
	    void copySurfaceData(const void *src, void *dst, uint32_t width, uint32_t height, uint32_t pitch);
    };
}
#endif 
