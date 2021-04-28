// (c) Electronic Arts. All Rights Reserved.

#ifndef ZLIB_VERSION_H
#define ZLIB_VERSION_H

#if defined _MSC_VER
#pragma once
#endif

// Define the major, minor and patch versions.
// This information is updated with each release.

//! This define indicates the major version number for the bigfile package.
//! \sa ZLIB_VERSION
#define ZLIB_VERSION_MAJOR   1
//! This define indicates the minor version number for the bigfile package.
//! \sa ZLIB_VERSION
#define ZLIB_VERSION_MINOR   2
//! This define indicates the patch version number for the bigfile package.
//! \sa ZLIB_VERSION
#define ZLIB_VERSION_PATCH   5

/*!
 * This is a utility macro that users may use to create a single version number
 * that can be compared against ZLIB_VERSION.
 *
 * For example:
 *
 * \code
 *
 * #if ZLIB_VERSION > ZLIB_CREATE_VERSION_NUMBER( 1, 1, 0 )
 * printf("Bigfile version is greater than 1.1.0.\n");
 * #endif
 *
 * \endcode
 */
#define ZLIB_CREATE_VERSION_NUMBER( major_ver, minor_ver, patch_ver ) \
    ((major_ver) * 1000000 + (minor_ver) * 1000 + (patch_ver))

/*!
 * This macro is an aggregate of the major, minor and patch version numbers.
 * \sa ZLIB_CREATE_VERSION_NUMBER
 */
#define ZLIB_VERSION \
    ZLIB_CREATE_VERSION_NUMBER( ZLIB_VERSION_MAJOR, ZLIB_VERSION_MINOR, ZLIB_VERSION_PATCH )

#endif // ZLIB_VERSION_H

