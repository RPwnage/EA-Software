/*************************************************************************************************/
/*!
    \file   osdksettingsliterals.h

    $Header: //gosdev/games/FIFA/2022/GenX/cge_test/blazeserver/customcomponent/osdksettings/osdksettingsliterals.h#1 $
    $Change: 1653004 $
    $DateTime: 2021/03/02 12:48:45 $

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



