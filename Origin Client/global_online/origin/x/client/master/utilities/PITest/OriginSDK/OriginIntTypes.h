#ifndef __ORIGIN_INT_TYPES__
#define __ORIGIN_INT_TYPES__

/// \defgroup inttypes Integer Types
/// \brief This module contains the common type definitions provided for pre-Visual Studio 2010 releases.

/// \ingroup inttypes
/// \brief A 64 bit signed integer.
typedef __int64				int64_t;

#if !defined( __int8_t_defined )

/// \ingroup inttypes
/// \brief A 32 bit signed integer.
typedef __int32				int32_t;

/// \ingroup inttypes
/// \brief A 16 bit signed integer.
typedef __int16				int16_t;

/// \ingroup inttypes
/// \brief An 8 bit signed integer.
typedef __int8				int8_t;

#define __int8_t_defined
#endif

/// \ingroup inttypes
/// \brief An 8 bit unsigned integer.
typedef unsigned __int8		uint8_t;

/// \ingroup inttypes
/// \brief A 64 bit unsigned integer.
typedef unsigned __int64	uint64_t;

#if !defined( __uint32_t_defined )

/// \ingroup inttypes
/// \brief A 32 bit unsigned integer.
typedef unsigned __int32	uint32_t;

#define __uint32    _t_defined
#endif

/// \ingroup inttypes
/// \brief A 16 bit unsigned integer.
typedef unsigned __int16	uint16_t;


#ifdef _WIN64

typedef __int64 intptr_t;

#else

typedef __int32 intptr_t;

#endif


#endif //__ORIGIN_INT_TYPES__