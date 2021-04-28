/*************************************************************************************************/
/*!
    \file   nucleusgetlegaldoccontent.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class NucleusGetLegalDocContent

    Gets Nkey

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "authentication/nucleushandler/nucleusgetlegaldoccontent.h"

namespace Blaze
{
namespace Authentication
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/
bool NucleusGetLegalDocContent::getLegalDocContent(const char8_t *docType, const char8_t* lang, const char8_t* country, const ClientPlatformType clientPlatformType, const char8_t* contentType, const char8_t* gameIdentifier, const Command* callingCommand)
{
    char8_t strURL[256];
    const char8_t* platform = AuthenticationSlaveImpl::getTOSPlatformStringFromClientPlatformType(clientPlatformType, contentType);
    if ((gameIdentifier == nullptr) || (strlen(gameIdentifier) == 0))
        blaze_snzprintf(strURL, sizeof(strURL), "/legalapp/%s/%s/%s/%s", docType, country, lang, platform);
    else
        blaze_snzprintf(strURL, sizeof(strURL), "/legalapp/%s/%s/%s/%s/%s", docType, country, lang, platform, gameIdentifier);

    TRACE_LOG("[GetLegalDocContent].Uri(" << strURL << ")");

    char8_t contentTypeHeader[256];
    blaze_snzprintf(contentTypeHeader, sizeof(contentTypeHeader), "Content-Type: text/%s", contentType);

    bool result = sendGetRequest(strURL, nullptr, 0, nullptr, 0, false, callingCommand);

    return result;
}

/*************************************************************************************************/
/*! \brief Overridden from base to supply specific handling for this class.
*************************************************************************************************/
BlazeRpcError NucleusGetLegalDocContentResult::decodeResult(RawBuffer& responseBuf)
{
    // Process the payload.
    mHasTosServerError = false;

    if ((getHttpStatusCode() >= HTTP_STATUS_SERVER_ERROR_START) &&
        (getHttpStatusCode() <= HTTP_STATUS_SERVER_ERROR_END))
    {
        // signal back to the caller that the TOS server is unavailable
        mHasTosServerError = true;
    }

    const char8_t* contentLoc = getHeader("Content-Location");
    if (contentLoc != nullptr)
    {
        setValue(nullptr, contentLoc, strlen(contentLoc));

        if (mContent != nullptr)
        {
            BLAZE_FREE(mContent);
            mContent = nullptr;
        }
        
        mContent = blaze_strdup(reinterpret_cast<const char8_t*>(responseBuf.data()));
        mContentSize = responseBuf.datasize();

        return ERR_OK;
    }
    else
    {
        return ERR_SYSTEM;
    }
}

} // Authentication
} // Blaze

