///////////////////////////////////////////////////////////////////////////////
// IGO.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include "IGO.h"

namespace OriginIGO {

    // shared across IGO & across platforms & across renderers
    int volatile gCalledFromInsideIGO = 0;
	bool gWindowedMode = false;
#if defined(ORIGIN_PC)
	HWND gRenderingWindow = NULL;
#endif
}