/*! ************************************************************************************************/
/*!
    \file externalsessionjsondatashared.h

    \brief This file contains defines and methods for dealing with the structured format of the
        external session's customizable json based string data, used by BlazeSDK clients and Blaze Server

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_EXTERNALSESSION_JSONDATA_SHARED_H
#define BLAZE_EXTERNALSESSION_JSONDATA_SHARED_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#if !defined(BLAZE_CLIENT_SDK) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_PS4_CROSSGEN) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/component/framework/tdf/externalsessiontypes.h"
#else
#include "framework/tdf/externalsessiontypes.h"
#endif

#include "framework/util/shared/rawbuffer.h"

namespace Blaze
{
namespace GameManager
{
    /*! ************************************************************************************************/
    /*! \brief Extract Blaze data from the Blaze-specified section of the PSN PlayerSession's customdata.
        \param[in] psnCustomData PSN PlayerSession's customdata to extract and deserialize from
    ***************************************************************************************************/
    bool psnPlayerSessionCustomDataToBlazeData(ExternalSessionBlazeSpecifiedCustomDataPs5& blazeData, const eastl::string& psnCustomData);

    /*! ************************************************************************************************/
    /*! \brief Populate the Blaze-specified section of the PSN PlayerSession's customdata, with Blaze data.
        \param[in] blazeData Data to serialize and add to the PSN PlayerSession's customdata
    ***************************************************************************************************/
    bool blazeDataToPsnPlayerSessionCustomData(eastl::string& psnCustomData, const ExternalSessionBlazeSpecifiedCustomDataPs5& blazeData);

    bool toJson(RawBuffer& jsonBuf, const EA::TDF::Tdf& fromTdf);
    bool fromJson(EA::TDF::Tdf& toTdf, const eastl::string& jsonBuf);
    bool toBase64(eastl::string& base64Buf, const RawBuffer& fromData);
    bool fromBase64(eastl::string& toData, const eastl::string& base64buf);

} // namespace GameManager
} // namespace Blaze

#endif

#endif // BLAZE_EXTERNALSESSION_JSONDATA_SHARED_H