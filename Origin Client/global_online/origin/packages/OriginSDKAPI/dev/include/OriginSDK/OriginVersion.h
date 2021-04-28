/// \defgroup defs Version
/// \brief This module contains the Version information for the Origin SDK.

// This file is generated from the values in scripts/version.xml
// Generation happens in scripts/generate-code.xml
#ifndef __ORIGIN_VERSION_H__
#define __ORIGIN_VERSION_H__

/// \ingroup defs
/// \brief The version number of this SDK.
///
#define ORIGIN_SDK_VERSION              3

/// \ingroup defs
/// \brief The version string that the Origin Client will see in an connection attempt.
///
#define ORIGIN_SDK_VERSION_STR          "9.10.2.5"

/// \ingroup defs
/// \brief This converts the component parts of a version number into a single number.
///
#define ORIGIN_VERSION(a,b,c,d)         ((a)<<24) + ((b)<<20) + ((c)<<12) + (d)

/// \ingroup defs
/// \brief The minimal version of the Origin necessary to run this SDK.
///
/// \note This will change for each release. Please Update When Necessary.
#define ORIGIN_MIN_ORIGIN_VERSION       ORIGIN_VERSION(9,8,0,0)

/// \ingroup defs
/// \brief The is the numeric version of the Origin SDK for comparison
///
#define ORIGIN_SDK_VERSION_NO           ORIGIN_VERSION(9,10,2,5)
#endif
