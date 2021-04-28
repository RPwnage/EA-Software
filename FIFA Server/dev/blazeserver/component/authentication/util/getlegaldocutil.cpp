#include "framework/blaze.h"

#include "authentication/util/getlegaldocutil.h"

namespace Blaze
{
namespace Authentication
{

BlazeRpcError GetLegalDocUtil::getLegalDocContent(const char8_t *docType, const char8_t* lang, const char8_t* country, const ClientPlatformType clientPlatformType,
                                                  const char8_t* contentType, const Command* callingCommand)
{
    if(!mGetLegalDocContent.getLegalDocContent(docType, lang, country, clientPlatformType, contentType, mComponent.getLegalDocGameIdentifier(), callingCommand))
    {
        return AuthenticationSlaveImpl::getError(mGetLegalDocContent.getResult());
    }

    const char8_t * legalDocUri = mGetLegalDocContent.getResult()->getURI();
    blaze_strnzcpy(mLegalDocUri, legalDocUri, sizeof(mLegalDocUri));
    blaze_strnzcpy(mLegalDocHost, mComponent.getLegalDocServer(), sizeof(mLegalDocHost));

    blaze_strnzcpy(mTermsOfServiceUri, "/legalapp/", sizeof(mTermsOfServiceUri));
    blaze_strnzcat(mTermsOfServiceUri, mLegalDocUri, sizeof(mTermsOfServiceUri));
    blaze_strnzcpy(mTermsOfServiceHost, mLegalDocHost, sizeof(mTermsOfServiceHost));

    mLegalDocContent = mGetLegalDocContent.getResult()->getContent();
    mLegalDocContentSize = mGetLegalDocContent.getResult()->getContentSize();

    return Blaze::ERR_OK;
}

}
}
