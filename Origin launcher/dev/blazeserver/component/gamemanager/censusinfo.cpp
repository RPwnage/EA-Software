/*! ************************************************************************************************/
/*!
    \file censusinfo.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/shared/blazestring.h"
#include "censusinfo.h"

namespace Blaze
{
namespace GameManager
{
    const char8_t* ATTRIBUTE_NAME_VALUE_SEPARATOR = ","; 
    const char8_t* GAMEMANAGER_CENSUS_ALL_VALUES = "CENSUS_ALL_VALUES";

    /*! ************************************************************************************************/
    /*! \brief generate string type census data for game attribute which will be sent to client based on 
        attributeName, attributeValue, numOfGames with the specified attribute name/value and numOfPlayers
        in these games

        The combined string is in attributeName + "," + attributeValue format + "," + numOfGames + "," + numOfPlayers 
        The string will get decode on client side to four parts

        \param[in] gameAttributeCensusData - struct to holding info for game attribute census data
        \return GameAttributeCensusString combined from game attribute census data passed in
    ***************************************************************************************************/
    GameAttributeCensusString genDataItemForGameAttribute(const GameAttributeCensusData& gameAttributeCensusData)
    {
        char8_t censusData[CENSUS_DATA_STRING_BUFFER] = "";

        blaze_strnzcat(censusData, gameAttributeCensusData.getAttributeName(), sizeof(censusData));
        blaze_strnzcat(censusData, ATTRIBUTE_NAME_VALUE_SEPARATOR, sizeof(censusData));
        
        blaze_strnzcat(censusData, gameAttributeCensusData.getAttributevalue(), sizeof(censusData));
        blaze_strnzcat(censusData, ATTRIBUTE_NAME_VALUE_SEPARATOR, sizeof(censusData));
        
        char8_t charGameNum[32] = "";
        blaze_snzprintf(charGameNum, sizeof(charGameNum), "%d", gameAttributeCensusData.getNumOfGames());
        blaze_strnzcat(censusData, charGameNum, sizeof(censusData));
        blaze_strnzcat(censusData, ATTRIBUTE_NAME_VALUE_SEPARATOR, sizeof(censusData));

        char8_t charPlayerNum[32] = "";
        blaze_snzprintf(charPlayerNum, sizeof(charPlayerNum), "%d", gameAttributeCensusData.getNumOfPlayers());
        blaze_strnzcat(censusData, charPlayerNum, sizeof(censusData));

        GameAttributeCensusString censusDataString;
        censusDataString.set(censusData);
        return censusDataString;
    }

    /*! ************************************************************************************************/
    /*! \brief decode GameAttributeCensusString sent from server to GameAttributeCensusData struct

        The combined string is in attributeName + "," + attributeValue format + "," + numOfGames + "," + numOfPlayers 

        \param[in]  gameAttributeCensusString - string passed from server holding all game attribute census information
        \param[out] GameAttributeCensusData - with all detailed information parsed from string passed in
    ***************************************************************************************************/
    void genDataItemForGameAttribute(const GameAttributeCensusString& gameAttributeCensusString, GameAttributeCensusData& gameAttributeCensusData)
    {
        char8_t* leftString = blaze_stristr(gameAttributeCensusString.c_str(), ATTRIBUTE_NAME_VALUE_SEPARATOR);
        size_t stringLength = strlen(gameAttributeCensusString.c_str());
        size_t unitLength = stringLength - strlen(leftString);

        // game attrib name
        char8_t attribName[Collections::MAX_ATTRIBUTENAME_LEN] = "";
        blaze_strsubzcat(attribName, sizeof(attribName), gameAttributeCensusString, unitLength);
        gameAttributeCensusData.setAttributeName(attribName);

        leftString++;

        // game attrib value
        char8_t* originString = leftString;
        leftString = blaze_stristr(leftString, ATTRIBUTE_NAME_VALUE_SEPARATOR);
        unitLength = strlen(originString) - strlen(leftString);
        char8_t attribValue[Collections::MAX_ATTRIBUTEVALUE_LEN] = "";
        blaze_strsubzcat(attribValue, sizeof(attribValue), originString, unitLength);
        gameAttributeCensusData.setAttributevalue(attribValue);

        leftString++;

        // number of games
        uint32_t censusNum = 0;
        originString = leftString;
        leftString = blaze_stristr(leftString, ATTRIBUTE_NAME_VALUE_SEPARATOR);
        unitLength = strlen(originString) - strlen(leftString);

        char8_t gameNum[32] = "";
        blaze_strsubzcat(gameNum, sizeof(gameNum), originString, unitLength);
        blaze_str2int(gameNum, &censusNum);
        gameAttributeCensusData.setNumOfGames(censusNum);

        leftString++;

        // number of players
        char8_t playerNum[32] = "";
        blaze_strsubzcat(playerNum, sizeof(playerNum), leftString, strlen(leftString));
        blaze_str2int(playerNum, &censusNum);
        gameAttributeCensusData.setNumOfPlayers(censusNum);
    }

    /*! ************************************************************************************************/
    /*! \brief combine attribute name and attributevalue 

        The combined string is in attributeName + "," + attributeValue format
        The function is used by server only

        \param[in] attributeName - game attribute name
        \param[in] attributeValue - game attribute value
        \return AttributeNameValue combined from attributeName and attributeValue passed in
    ***************************************************************************************************/
    Collections::AttributeNameValue genAttrubteNameValueCombination(const Collections::AttributeName& attributeName, 
                                                                    const Collections::AttributeValue& attributeValue)
    {
        char8_t attribCombine[Collections::MAX_ATTRIBUTENAMEVALUE_LEN] = "";
        blaze_strnzcat(attribCombine, attributeName, sizeof(attribCombine));
        blaze_strnzcat(attribCombine, ATTRIBUTE_NAME_VALUE_SEPARATOR, sizeof(attribCombine));
        blaze_strnzcat(attribCombine, attributeValue, sizeof(attribCombine));

        Collections::AttributeNameValue attribNameValue;
        attribNameValue.set(attribCombine);

        return attribNameValue;
    }

    /*! ************************************************************************************************/
    /*! \brief whether the value string is CENSUS_ALL_VALUES 

        \param[in] value - the value string
        \return true if value is equal to CENSUS_ALL_VALUES, otherwise false.
    ***************************************************************************************************/
    bool isCensusAllValues(const char8_t* value)
    {
        return (blaze_strcmp(GAMEMANAGER_CENSUS_ALL_VALUES, value) == 0);
    }

}// namespace GameManager
}// namespace Blaze

