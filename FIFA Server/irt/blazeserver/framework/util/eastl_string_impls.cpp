/*************************************************************************************************/
/*!
    \file eastl_string_impls.cpp

    The EASTL string class requires the user to implement/wrap a few snprintf functions 
      (which the eastl defines as extern).  We want to do this for blaze server components, but
      we don't want to blow away a game team's impl on the client.


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"

int Vsnprintf8 (char8_t*  pDestination, size_t n, const char8_t*  pFormat, va_list arguments)
{
    // NOTE: we cannot use blaze_vsnzprintf here, because it has a non-standard return code
#ifdef EA_PLATFORM_LINUX
    return vsnprintf(pDestination, n, pFormat, arguments);
#else
    return vsnprintf(pDestination, n, pFormat, arguments);
#endif
}
