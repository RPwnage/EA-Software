/*************************************************************************************************/
/*!
    \file   nucleusgetlegaldocuri.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class NucleusGetLegalDocUri

    Gets Nkey

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "authentication/nucleushandler/nucleusgetlegaldocuri.h"

namespace Blaze
{
namespace Authentication
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

bool NucleusGetLegalDocUri::getLegalDocUri(const char8_t *docType, const char8_t* lang, const char8_t* country, const ClientPlatformType clientPlatformType, const char8_t* contentType, const char8_t* gameIdentifier, const Command* callingCommand)
{
    char8_t strURL[256];
    const char8_t* platform = AuthenticationSlaveImpl::getTOSPlatformStringFromClientPlatformType(clientPlatformType, contentType);
    if ((gameIdentifier == nullptr) || (strlen(gameIdentifier) == 0))
        blaze_snzprintf(strURL, sizeof(strURL), "/legalapp/%s/%s/%s/%s", docType, country, lang, platform);
    else
        blaze_snzprintf(strURL, sizeof(strURL), "/legalapp/%s/%s/%s/%s/%s", docType, country, lang, platform, gameIdentifier);

    TRACE_LOG("[GetLegalDocUri].Uri(" << strURL << ")");

    return sendHeadRequest(strURL, nullptr, 0, nullptr, 0, false, callingCommand);
}

/*************************************************************************************************/
/*! \brief Overridden from base to supply specific handling for this class.
*************************************************************************************************/

BlazeRpcError NucleusGetLegalDocUriResult::decodeResult(RawBuffer& responseBuf)
{
    // Process the payload.
    const char8_t* contentLoc = getHeader("Content-Location");
    if (contentLoc != nullptr)
    {
        setValue(nullptr, contentLoc, strlen(contentLoc));
        return ERR_OK;
    }
    else
    {
        return ERR_SYSTEM;
    }
}

} // Authentication
} // Blaze

