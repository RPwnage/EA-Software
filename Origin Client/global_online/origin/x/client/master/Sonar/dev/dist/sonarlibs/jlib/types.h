#pragma once

namespace jlib
{

#ifdef _WIN32

typedef unsigned __int64 UINT64;
typedef unsigned __int32 UINT32;
typedef unsigned __int16 UINT16;
typedef unsigned __int8 UINT8;

typedef __int64 INT64;
typedef __int32 INT32;
typedef __int16 INT16;
typedef __int8 INT8;

#else

#include <sys/types.h>
typedef u_int64_t UINT64;
typedef u_int32_t UINT32;
typedef u_int16_t UINT16;
typedef u_int8_t UINT8;

typedef int64_t INT64;
typedef int32_t INT32;
typedef int16_t INT16;
typedef int8_t INT8;

#endif
}
