
/*************************************************************************************************/
/*!
    \class LoginCommand

    Sends Nucleus an email and password for authentication.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "authentication/authenticationimpl.h"
#include "authentication/tdf/authentication.h"
#include "authentication/rpc/authenticationslave/getprivacypolicycontent_stub.h"
#include "authentication/util/getusercountryutil.h"
#include "authentication/util/authenticationutil.h"

#include "framework/controller/controller.h"

#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"
#include "framework/util/locales.h"

namespace Blaze
{
namespace Authentication
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

class GetPrivacyPolicyContentCommand : public GetPrivacyPolicyContentCommandStub
{
public:
    GetPrivacyPolicyContentCommand(Message* message, GetLegalDocContentRequest* request, AuthenticationSlaveImpl* componentImpl);

    ~GetPrivacyPolicyContentCommand() override {};

private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;

    // Owned memory
    GetUserCountryUtil mGetUserCountryUtil;

    // States
    GetPrivacyPolicyContentCommandStub::Errors execute() override;

};
DEFINE_GETPRIVACYPOLICYCONTENT_CREATE()

GetPrivacyPolicyContentCommand::GetPrivacyPolicyContentCommand(Message* message, GetLegalDocContentRequest* request, AuthenticationSlaveImpl* componentImpl)
    : GetPrivacyPolicyContentCommandStub(message, request),
    mComponent(componentImpl),
    mGetUserCountryUtil(*componentImpl, getPeerInfo())
{
}

/* Private methods *******************************************************************************/
GetPrivacyPolicyContentCommandStub::Errors GetPrivacyPolicyContentCommand::execute()
{    
    const char8_t *country = mRequest.getIsoCountryCode();
    const char8_t *language = mRequest.getIsoLanguage();
    ClientPlatformType clientPlatformType = mRequest.getPlatform();
    Blaze::Authentication::GetLegalDocContentRequest::ContentType contentType = mRequest.getContentType();

    char8_t langStr[MAX_LANG_ISO_CODE_LENGTH];
    if (strlen(language) == 0)
    {        
        LocaleTokenCreateLanguageString(langStr, getPeerInfo()->getLocale());
        language = langStr;
    }

    if (strlen(country) == 0)
    {
        country = mGetUserCountryUtil.getUserCountry(UserSession::getCurrentUserBlazeId());
        if (country == nullptr)
        {
            WARN_LOG("[GetPrivacyPolicyContentCommand].execute(): no country defined");
            return ERR_SYSTEM;
        }
    }

    if (clientPlatformType == INVALID)
    {
        clientPlatformType = mComponent->getPlatform(*this);
    }

    const LegalDocContentInfo *legalDocContentInfo = mComponent->getLastestLegalDocContentInfoByContentType(language, country, clientPlatformType, "webprivacy", contentType, this);


    if (legalDocContentInfo == nullptr || !(legalDocContentInfo->isValid()))
    {
        WARN_LOG("[GetPrivacyPolicyContentCommand].execute(): get latest Privacy Policy doc content info for language[" << language << "], country[" << country << "], platform[" << ClientPlatformTypeToString(clientPlatformType) 
            << "], contentType[" << contentType << "], failed with error[ERR_SYSTEM]");
        return ERR_SYSTEM;
    }

    if (mRequest.getFetchContent())
    {
        mResponse.setLegalDocContentLength(legalDocContentInfo->getLegalDocContentSize());
        mResponse.setLegalDocContent(legalDocContentInfo->getLegalDocContent().c_str());
    }
    mResponse.setLegalDocVersion(legalDocContentInfo->getLegalDocUri().c_str());

    return ERR_OK;
}

} // Authentication
} // Blaze
