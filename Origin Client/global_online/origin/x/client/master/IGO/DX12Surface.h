
///////////////////////////////////////////////////////////////////////////////
// DX12Surface.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef DX12SURFACE_H
#define DX12SURFACE_H

#include "IGOSurface.h"

#if defined(ORIGIN_PC)

namespace OriginIGO {

	class DX12SurfaceImpl;
	class IGOWindow;

	class DX12Surface : public IGOSurface
	{
	public:
		DX12Surface(void* context, IGOWindow* window);
		virtual ~DX12Surface();

		virtual void render(void* context);
		virtual void update(void* context, uint8_t alpha, int32_t width, int32_t height, const char* data, int size, uint32_t flags);

	private:
		DX12SurfaceImpl* mImpl;

	};
}
#endif

#endif 