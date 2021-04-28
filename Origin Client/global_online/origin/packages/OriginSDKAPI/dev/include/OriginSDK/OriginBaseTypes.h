#ifndef __ORIGIN_BASE_TYPES_H__
#define __ORIGIN_BASE_TYPES_H__

/// \defgroup types Types
/// \brief This module contains the structs, defines, and typedefs used by a variety of Origin functions.

// Opaque Parameters

/// \ingroup types
/// \brief A unique ID identifying the user.
///
typedef uint64_t	OriginUserT;

/// \ingroup types
/// \brief A unique ID identifying the persona.
///
typedef uint64_t	OriginPersonaT;

/// \ingroup types
/// \brief An Ebisi error code.
///
typedef int32_t		OriginErrorT;

/// \ingroup types
/// \brief A general handle used to point to a resource.
///
typedef intptr_t	OriginHandleT;

/// \ingroup types
/// \brief A handle to an SDK instance.
///
typedef intptr_t	OriginInstanceT;

/// \ingroup types
/// \brief The character type.
///
typedef char		OriginCharT;

/// \ingroup types
/// \brief The size parameter. 
///
typedef size_t		OriginSizeT;

/// \ingroup types
/// \brief The index parameter. 
///
typedef size_t		OriginIndexT;

#endif //__ORIGIN_BASE_TYPES_H__