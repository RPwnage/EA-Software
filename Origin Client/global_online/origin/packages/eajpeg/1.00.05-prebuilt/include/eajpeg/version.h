// (c) Electronic Arts. All Rights Reserved.

#ifndef EAJPEG_VERSION_H
#define EAJPEG_VERSION_H

#if defined _MSC_VER
#pragma once
#endif

// Define the major, minor and patch versions.
// This information is updated with each release.

//! This define indicates the major version number for the EAJPEG package.
//! \sa EAJPEG
#define EAJPEG_VERSION_MAJOR   1
//! This define indicates the minor version number for the EAJPEG package.
//! \sa EAJPEG
#define EAJPEG_VERSION_MINOR   0
//! This define indicates the patch version number for the EAJPEG package.
//! \sa EAJPEG
#define EAJPEG_VERSION_PATCH   5

/*!
 * This is a utility macro that users may use to create a single version number
 * that can be compared against EAJPEG_VERSION.
 *
 * For example:
 *
 * \code
 *
 * #if EAJPEG_VERSION > EAJPEG_CREATE_VERSION_NUMBER( 1, 0, 0 )
 * printf("eajpeg version is greater than 1.0.0.\n");
 * #endif
 *
 * \endcode
 */
#define EAJPEG_CREATE_VERSION_NUMBER( major_ver, minor_ver, patch_ver ) \
    ((major_ver) * 1000000 + (minor_ver) * 1000 + (patch_ver))

/*!
 * This macro is an aggregate of the major, minor and patch version numbers.
 * \sa EAJPEG_CREATE_VERSION_NUMBER
 */
#define EAJPEG_VERSION \
    EAJPEG_CREATE_VERSION_NUMBER( EAJPEG_VERSION_MAJOR, EAJPEG_VERSION_MINOR, EAJPEG_VERSION_PATCH )

#endif // EAJPEG_VERSION_H
