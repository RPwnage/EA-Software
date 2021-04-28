#include "framework/blaze.h"
#include "framework/util/locales.h"

#include "authentication/gettermsofservicecontentbase.h"
#include "authentication/util/authenticationutil.h"

namespace Blaze
{
namespace Authentication
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/
GetTermsOfServiceContentBase::GetTermsOfServiceContentBase(AuthenticationSlaveImpl* componentImpl, const PeerInfo* peerInfo)
    : mComponent(componentImpl),
      mPeerInfo(peerInfo),
      mGetUserCountryUtil(*componentImpl, peerInfo)
{
}

GetTermsOfServiceContentCommandStub::Errors GetTermsOfServiceContentBase::execute(
    const Command* caller, 
    const GetLegalDocContentRequest& request, 
    GetLegalDocContentResponse& response)
{
    const char8_t *country = request.getIsoCountryCode();
    const char8_t *language = request.getIsoLanguage();
    ClientPlatformType clientPlatformType = request.getPlatform();
    Blaze::Authentication::GetLegalDocContentRequest::ContentType contentType = request.getContentType();

    char8_t langStr[MAX_LANG_ISO_CODE_LENGTH];
    if (strlen(language) == 0)
    {
        LocaleTokenCreateLanguageString(langStr, mPeerInfo->getLocale());
        language = langStr;
    }

    if (strlen(country) == 0)
    {
        country = mGetUserCountryUtil.getUserCountry(UserSession::getCurrentUserBlazeId());
        if (country == nullptr)
        {
            WARN_LOG("[GetTermsOfServiceContentBase].execute(): no country defined");
            return GetTermsOfServiceContentCommandStub::ERR_SYSTEM;
        }
    }

    if (clientPlatformType == INVALID)
    {
        clientPlatformType = mComponent->getPlatform(*caller);
    }

    const LegalDocContentInfo *legalDocContentInfo = mComponent->getLastestLegalDocContentInfoByContentType(language, country, clientPlatformType, "webterms", contentType, caller);

    if (legalDocContentInfo == nullptr || !(legalDocContentInfo->isValid()))
    {
        WARN_LOG("[GetTermsOfServiceContentBase].execute(): get latest Terms of Service doc content info for language[" << language << "], country[" << country << "], platform[" << ClientPlatformTypeToString(clientPlatformType) 
           << "], contentType[" << contentType << "], failed with error[ERR_SYSTEM]");
        return GetTermsOfServiceContentCommandStub::ERR_SYSTEM;
    }

    if (request.getFetchContent())
    {
        response.setLegalDocContentLength(legalDocContentInfo->getLegalDocContentSize());
        response.setLegalDocContent(legalDocContentInfo->getLegalDocContent().c_str());
    }
    response.setLegalDocVersion(legalDocContentInfo->getLegalDocUri().c_str());

    return GetTermsOfServiceContentCommandStub::ERR_OK;
}

} // Authentication
} // Blaze
