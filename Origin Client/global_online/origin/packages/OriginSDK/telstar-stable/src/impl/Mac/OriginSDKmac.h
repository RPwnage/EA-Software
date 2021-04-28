#ifndef __ORIGIN_SDK_MAC_H__
#define __ORIGIN_SDK_MAC_H__

/// \file This file contains MAC specific implementations of the SDK.

#ifdef ORIGIN_MAC

namespace Origin
{
    unsigned int FindProcessOrigin();
}

#define sprintf_s snprintf

#endif

#endif //__ORIGIN_SDK_MAC_H__
