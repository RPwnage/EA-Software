/*! *********************************************************************************************/
/*!
    \file   deprecatedconfig.h

    Fixes up any outstanding preprocessor options left over from the Blaze project.

    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef EA_TDF_INTERNAL_DEPRECATED_CONFIG_H
#define EA_TDF_INTERNAL_DEPRECATED_CONFIG_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#ifdef BLAZE_ENABLE_TDF_COPY_CONSTRUCTOR
#define EA_TDF_INCLUDE_COPY_CONSTRUCTOR 1
#endif

#ifdef BLAZE_ENABLE_CUSTOM_TDF_ATTRIBUTES 
#define EA_TDF_INCLUDE_CUSTOM_TDF_ATTRIBUTES 1
#endif


#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
#define EA_TDF_INCLUDE_CHANGE_TRACKING 1
#endif

#ifdef BLAZE_ENABLE_RECONFIGURE_TAG_INFO
#define EA_TDF_INCLUDE_RECONFIGURE_TAG_INFO 1
#endif

#ifdef BLAZE_ENABLE_EXTENDED_TDF_TAG_INFO
#define EA_TDF_INCLUDE_EXTENDED_TAG_INFO 1
#endif

#ifdef BLAZE_ENABLE_TDF_MEMBER_MERGE
#define EA_TDF_INCLUDE_MEMBER_MERGE 1
#endif

#endif // EA_TDF_INTERNAL_DEPRECATED_CONFIG_H

