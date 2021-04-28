/*! ************************************************************************************************/
/*!
    \file censusinfo.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_CENSUS_INFO
#define BLAZE_GAMEMANAGER_CENSUS_INFO

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/component/gamemanager/tdf/gamemanager.h"
#else
#include "gamemanager/tdf/gamemanager.h"
#endif

namespace Blaze
{
namespace GameManager
{
    // loose member functions used by GameManager for packaging/unpacking census data into census data strings
    //   Note: code is shared between client & server

    /*! ************************************************************************************************/
    /*! \brief generate string type census data for game attribute which will be sent to client based on 
            attributeName, attributeValue, numOfGames with the specified attribute name/value and numOfPlayers
            in these games

        The combined string is in attributeName + "," + attributeValue format + "," + numOfGames + "," + numOfPlayers 
        The string will get decode on client side to four parts

        \param[in] gameAttributeCensusData - struct to holding info for game attribute census data
        \return GameAttributeCensusString combined from game attribute census data passed in
    ***************************************************************************************************/
    GameAttributeCensusString genDataItemForGameAttribute(const GameAttributeCensusData& gameAttributeCensusData);

    /*! ************************************************************************************************/
    /*! \brief decode GameAttributeCensusString sent from server to GameAttributeCensusData struct

        The combined string is in attributeName + "," + attributeValue format + "," + numOfGames + "," + numOfPlayers 

        \param[in] gameAttributeCensusString - string passed from server holding all game attribute census information
        \param[out] GameAttributeCensusData - with all detailed information parsed from string passed in
    ***************************************************************************************************/
    void genDataItemForGameAttribute(const GameAttributeCensusString& gameAttributeCensusString, 
                                     GameAttributeCensusData& gameAttributeCensusData);
    
    /*! ************************************************************************************************/
    /*! \brief combine attribute name and attributevalue 
            
        The combined string is in attributeName + "," + attributeValue format
        The function is used by server only

        \param[in] attributeName - game attribute name
        \param[in] attributeValue - game attribute value
        \return AttributeNameValue combined from attributeName and attributeValue passed in
    ***************************************************************************************************/
    Collections::AttributeNameValue genAttrubteNameValueCombination(const Collections::AttributeName& attributeName, 
                                                                    const Collections::AttributeValue& attributeValue);

    /*! ************************************************************************************************/
    /*! \brief whether the value string is CENSUS_ALL_VALUES 

        \param[in] value - the value string
        \return true if value is equal to CENSUS_ALL_VALUES, otherwise false.
    ***************************************************************************************************/
    bool isCensusAllValues(const char8_t* value);
}// namespace GameManager
}// namespace Blaze
#endif // BLAZE_GAMEMANAGER_CENSUS_INFO
