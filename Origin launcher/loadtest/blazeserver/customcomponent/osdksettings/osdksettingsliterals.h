/*************************************************************************************************/
/*!
    \file   osdksettingsliterals.h

    $Header: //gosdev/games/FIFA/2022/GenX/loadtest/blazeserver/customcomponent/osdksettings/osdksettingsliterals.h#1 $
    $Change: 1652815 $
    $DateTime: 2021/03/02 02:27:50 $

    \brief

    This contains any literals that the config parser will parse.

    \attention
        (c) Electronic Arts Inc. 2009
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
namespace OSDKSettings
{

enum OSDKSETTINGS_LITERAL_ENUM
{
    ENUM_EMAIL_ADDRESS = 0,

    NUM_LITERALS
};

const char8_t* OSDKSETTINGS_LITERALS[] =
{
    "___NUCLEUS_EMAIL"
};

} // OSDKSettings
} // Blaze



