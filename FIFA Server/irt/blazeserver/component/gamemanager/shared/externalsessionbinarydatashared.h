/*! ************************************************************************************************/
/*!
    \file externalsessionbinarydatashared.h

    \brief This file contains defines and methods for dealing with the structured format of the
        external session's customizable binary data, used by DirtySDK and Blaze Server

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_EXTERNALSESSION_BINARYDATA_H
#define BLAZE_EXTERNALSESSION_BINARYDATA_H

#include "framework/util/shared/collections.h"

#if defined(BLAZE_CLIENT_SDK)
#include "DirtySDK/dirtysock/dirtysessionmanager.h"
#endif

namespace Blaze
{
namespace GameManager
{
    //!< data Blaze controls, within the first party session's binary blob
    struct ExternalSessionBinaryDataHead
    {
        // iLobbyId in DirtySessionManagerBinaryHeaderT
        int64_t iGameId;
        int64_t iGameType;
    };

    //!< data Blaze controls, within the first party session's changeable binary blob
    struct ExternalSessionChangeableBinaryDataHead
    {
        char8_t strGameMode[Blaze::Collections::MAX_ATTRIBUTEVALUE_LEN];
    };

#if defined(BLAZE_CLIENT_SDK) && defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN)
    // compile time check of the Blaze structure and its DirtySDK structure's equivalence
    static_assert(sizeof(ExternalSessionBinaryDataHead) == sizeof(DirtySessionManagerBinaryHeaderT), "Blaze ExternalSessionBinaryDataHead and DirtySDK DirtySessionManagerBinaryHeaderT must be equivalent, but they have different overall size.");
    static_assert(eastl::is_same<decltype(ExternalSessionBinaryDataHead().iGameId), decltype(DirtySessionManagerBinaryHeaderT().iLobbyId)>::value, "Blaze ExternalSessionBinaryDataHead and DirtySDK DirtySessionManagerBinaryHeaderT must be equivalent, but thier iLobbyId/gameId type is different.");
    static_assert(eastl::is_same<decltype(ExternalSessionBinaryDataHead().iGameType), decltype(DirtySessionManagerBinaryHeaderT().iGameType)>::value, "Blaze ExternalSessionBinaryDataHead and DirtySDK DirtySessionManagerBinaryHeaderT must be equivalent, but thier iGameType type is different.");

    static_assert(sizeof(ExternalSessionChangeableBinaryDataHead) == sizeof(DirtySessionManagerChangeableBinaryHeaderT), "Blaze ExternalSessionChangeableBinaryDataHead and DirtySDK DirtySessionManagerChangeableBinaryHeaderT must be equivalent, but they have different overall size.");
    static_assert(eastl::is_same<decltype(ExternalSessionChangeableBinaryDataHead().strGameMode), decltype(DirtySessionManagerChangeableBinaryHeaderT().strGameMode)>::value, "Blaze ExternalSessionChangeableBinaryDataHead and DirtySDK DirtySessionManagerChangeableBinaryHeaderT must be equivalent, but thier strGameMode type is different.");
#endif

    //!< converts first party session's raw binary data to Blaze/DirtySDK struct and title data
    template<class T> 
    inline bool externalSessionRawBinaryDataToBlazeData(const uint8_t* rawBinaryData, size_t rawBinaryDataLen,
        T& blazeStruct, EA::TDF::TdfBlob* blazeTitleData = nullptr)
    {
        if ((rawBinaryData == nullptr) || EA_UNLIKELY(rawBinaryDataLen < sizeof(blazeStruct)))
            return false;
        memset(&blazeStruct, 0, sizeof(blazeStruct));
        memcpy(&blazeStruct, rawBinaryData, sizeof(blazeStruct));

        // title data follows
        if (blazeTitleData != nullptr)
            blazeTitleData->setData(rawBinaryData + sizeof(blazeStruct), rawBinaryDataLen - sizeof(blazeStruct));
        return true;
    }

    //!< converts Blaze/DirtySDK struct and title data to first party session's raw binary data
    template<class T> 
    inline bool blazeDataToExternalSessionRawBinaryData(const T& blazeStruct, const EA::TDF::TdfBlob* blazeTitleData,
        uint8_t* rawBinaryData, size_t rawBinaryDataLen)
    {
        size_t totSize = (sizeof(blazeStruct) + ((blazeTitleData != nullptr)? blazeTitleData->getCount() : 0));
        if ((rawBinaryData == nullptr) || (rawBinaryDataLen < totSize))
            return false;
        memset(rawBinaryData, 0, rawBinaryDataLen);
        memcpy(rawBinaryData, &blazeStruct, sizeof(blazeStruct));
        if (blazeTitleData != nullptr)
            memcpy(rawBinaryData + sizeof(blazeStruct), blazeTitleData->getData(), blazeTitleData->getCount());
        return true;
    }
} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_EXTERNALSESSION_BINARYDATA_H