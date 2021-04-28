/*! ************************************************************************************************/
/*!
    \file censusinfo.h

    Contains internal blaze helpers for encoding/decoding GameManager census data keys.


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#ifndef BLAZE_GAMEMANAGER_CENSUS_INFO_H
#define BLAZE_GAMEMANAGER_CENSUS_INFO_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/component/gamemanager/tdf/gamemanager.h"
#else
#include "framework/tdf/attributes.h"
#include "framework/tdf/censusdatatype.h"
#include "gamemanager/tdf/gamemanager.h"
#endif

namespace Blaze
{
namespace GameManager
{
    /*! ************************************************************************************************/
    /*! \brief generate string type census data for game attribute which will be sent to client based on 
            attributeName, attributeValue, numOfGames with the specified attribute name/value and numOfPlayers
            in these games

        The combined string is in attributeName + "," + attributeValue format + "," + numOfGames + "," + numOfPlayers 
        The string will get decode on client side to four parts

        \param[in] gameAttributeCensusData - struct to holding info for game attribute census data
        \return GameAttributeCensusString combined from game attribute census data passed in
    ***************************************************************************************************/
    BLAZESDK_API GameAttributeCensusString genDataItemForGameAttribute(const GameAttributeCensusData& gameAttributeCensusData);

    /*! ************************************************************************************************/
    /*! \brief decode GameAttributeCensusString sent from server to GameAttributeCensusData struct

        The combined string is in attributeName + "," + attributeValue format + "," + numOfGames + "," + numOfPlayers 

        \param[in] gameAttributeCensusString - string passed from server holding all game attribute census information
        \param[out] gameAttributeCensusData - with all detailed information parsed from string passed in
    ***************************************************************************************************/
    BLAZESDK_API void genDataItemForGameAttribute(const GameAttributeCensusString& gameAttributeCensusString, GameAttributeCensusData& gameAttributeCensusData);


    /*! ************************************************************************************************/
    /*! \brief combine attribute name and attributevalue 
            
        The combined string is in attributeName + "," + attributeValue format
        The function is used by server only

        \param[in] attributeName - game attribute name
        \param[in] attributeValue - game attribute value
        \return AttributeNameValue combined from attributeName and attributeValue passed in
    ***************************************************************************************************/
    BLAZESDK_API Collections::AttributeNameValue genAttrubteNameValueCombination(const Collections::AttributeName& attributeName, 
                                                                    const Collections::AttributeValue& attributeValue);
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_CENSUS_INFO_H
