#ifndef BLAZE_VERSION_H
#define BLAZE_VERSION_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "EABase/eabase.h"

#include "blazesdk.h" // for BLAZESDK_API

/********************************************/
/*
   BLAZE_SDK_VERSION
   e.g. 1234567890
   two decimal digits for year: 12
   two decimal digits for season: 34
   two decimal digits for major release: 56
   two decimal digits for minor release: 78
   two decimal digits for patch: 90
   
   BLAZE_SDK_BRANCH
   Used to indicate a specific branch.  This is added onto the string returned
   by getBlazeSdkVersionString().  This may be set to an empty string.

*********************************************/

// NOTE: The BLAZE_SDK_VERSION #define line of code must NOT be followed by any comment

// 15.1.1.10.0
#define BLAZE_SDK_VERSION_YEAR (15)
#define BLAZE_SDK_VERSION_SEASON (1)
#define BLAZE_SDK_VERSION_MAJOR (1)
#define BLAZE_SDK_VERSION_MINOR (10)
#define BLAZE_SDK_VERSION_PATCH (1)
#define BLAZE_SDK_VERSION_MODIFIERS ""

#define BLAZE_SDK_MAKE_VERSION(year,season,major,minor,patch) ((year*100000000) + (season*1000000) + (major*10000) + (minor*100) + (patch))

#define BLAZE_SDK_VERSION   BLAZE_SDK_MAKE_VERSION(BLAZE_SDK_VERSION_YEAR, BLAZE_SDK_VERSION_SEASON, BLAZE_SDK_VERSION_MAJOR, BLAZE_SDK_VERSION_MINOR, BLAZE_SDK_VERSION_PATCH)
#define BLAZE_SDK_BRANCH ""

namespace Blaze
{
    /*! *******************************************************************************************/
    /*! \brief Returns the entire BlazeSDK version as a uint64_t value
    ***********************************************************************************************/
    BLAZESDK_API uint64_t getBlazeSdkVersion();

    /*! *******************************************************************************************/
    /*! \brief Returns the BlazeSDK two digit year version number as a uint8_t value
    ***********************************************************************************************/
    BLAZESDK_API uint8_t getBlazeSdkVersionYear();

    /*! *******************************************************************************************/
    /*! \brief Returns the BlazeSDK two digit season version number as a uint8_t value
    ***********************************************************************************************/
    BLAZESDK_API uint8_t getBlazeSdkVersionSeason();

    /*! *******************************************************************************************/
    /*! \brief Returns the BlazeSDK two digit major version number as a uint8_t value
    ***********************************************************************************************/
    BLAZESDK_API uint8_t getBlazeSdkVersionMajor();

    /*! *******************************************************************************************/
    /*! \brief Returns the BlazeSDK two digit minor version number as a uint8_t value
    ***********************************************************************************************/
    BLAZESDK_API uint8_t getBlazeSdkVersionMinor();

    /*! *******************************************************************************************/
    /*! \brief Returns the BlazeSDK two digit patch release number as a uint8_t value
    ***********************************************************************************************/
    BLAZESDK_API uint8_t getBlazeSdkVersionPatchRel();

    /*! *******************************************************************************************/
    /*! \brief Returns the BlazeSDK modifiers as a char8_t* value
    ***********************************************************************************************/
    BLAZESDK_API const char8_t* getBlazeSdkVersionModifiers();

    /*! *******************************************************************************************/
    /*! \brief Returns the BlazeSDK version as a string, and appends the BLAZE_SDK_BRANCH value if it is not empty
    ***********************************************************************************************/
    BLAZESDK_API const char8_t *getBlazeSdkVersionString();
}

#endif
