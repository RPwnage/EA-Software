// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <EABase/eabase.h>

 /*!
  * @def EADPREALTIMEMESSAGING_API
  * @brief This is used to label functions as DLL exports.
  */
#ifndef EADPREALTIMEMESSAGING_API
#if defined(EA_DLL) // If the build file hasn't already defined this to be dllexport...
#if defined(EA_PLATFORM_MICROSOFT)
#define EADPREALTIMEMESSAGING_API      __declspec(dllimport)
#else
  // rely on the dll building project to define dllexport
#define EADPREALTIMEMESSAGING_API
#endif
#else
#define EADPREALTIMEMESSAGING_API
#endif
#endif

namespace eadp
{
/*!
 *  @brief Root namespace for the RealTimeMessaging package.
 */
namespace realtimemessaging
{

/*!
 *  @brief Package name for the module
 */
EADPREALTIMEMESSAGING_API extern const char8_t* kPackageName;

}
}

namespace com
{
namespace ea
{
namespace eadp
{
namespace antelope
{

/*!
 * @brief Root namespace for protobuf messages for the old Social protocol
 *  Fully qualified namespace: com::ea::eadp::antelope::protocol
 */
namespace protocol
{
}

namespace rtm
{
/*!
 * @brief Root namespace for protobuf messages for the new RTM protocol
 *  Fully qualified namespace: com::ea::eadp::antelope::rtm::protocol
 */
namespace protocol
{
}
}

}
}
}
}