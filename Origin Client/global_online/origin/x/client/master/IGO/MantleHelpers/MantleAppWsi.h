#ifndef _MANTLEAPPWSI_
#define _MANTLEAPPWSI_

// declare global helpers...
#include <mantle.h>
#include <stdint.h>


#define MANTLE_FUNCTION_EXTERN_DECLARATION 1
#include "../../MantleFunctions.h"
#undef MANTLE_FUNCTION_EXTERN_DECLARATION

namespace OriginIGO{
	class RENDERTARGET;

	GR_DEVICE getMantleDevice(unsigned int gpu);
	bool getMantleDeviceFullscreen(unsigned int gpu /*not yet used*/);


}

#endif // _MANTLEAPPWSI_