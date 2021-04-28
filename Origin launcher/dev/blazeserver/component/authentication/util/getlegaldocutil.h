#ifndef BLAZE_AUTHENTICATION_GETLEGALDOCUTIL_H
#define BLAZE_AUTHENTICATION_GETLEGALDOCUTIL_H

#include "authentication/nucleushandler/nucleusgetlegaldoccontent.h"
#include "authentication/nucleushandler/nucleusgetlegaldocuri.h"

namespace Blaze
{
namespace Authentication
{

class AuthenticationSlaveImpl;

class GetLegalDocUtil
{
public:
    GetLegalDocUtil(AuthenticationSlaveImpl &componentImpl, const ClientType &clientType)
      : mComponent(componentImpl),
        mGetLegalDocContent(componentImpl.getConnectionManagerPtr(AuthenticationSlaveImpl::TOS), &(componentImpl.getNucleusBypassList()), componentImpl.isBypassNucleusEnabled(clientType)),
        mGetLegalDocUri(componentImpl.getConnectionManagerPtr(AuthenticationSlaveImpl::TOS), &(componentImpl.getNucleusBypassList()), componentImpl.isBypassNucleusEnabled(clientType))
    {
        mTermsOfServiceUri[0] = 0;
        mTermsOfServiceHost[0] = 0;
        mLegalDocUri[0] = 0;
        mLegalDocHost[0] = 0;
    }

    BlazeRpcError DEFINE_ASYNC_RET(getLegalDocContent(const char8_t *docType, const char8_t* lang, const char8_t* country, const ClientPlatformType clientPlatformType, const char8_t* contentType, const Command* callingCommand = nullptr));
    const char8_t *getTermsOfServiceUri() {return mTermsOfServiceUri;}
    const char8_t *getTermsOfServiceHost() {return mTermsOfServiceHost;}
    const char8_t *getLegalDocUri() {return mLegalDocUri;}
    const char8_t *getLegalDocHost() {return mLegalDocHost;}

    const char8_t *getLegalDocContentData() {return mLegalDocContent; }
    uint32_t getLegalDocContentDataSize() { return mLegalDocContentSize; }
    bool hasTosServerError() const { return mGetLegalDocContent.getResult()->hasTosServerError(); }

private:
    // memory owned by creator, don't free
    AuthenticationSlaveImpl &mComponent;

    // Owned memory
    char8_t mTermsOfServiceUri[256];
    char8_t mTermsOfServiceHost[256];
    char8_t mLegalDocUri[256];
    char8_t mLegalDocHost[256];

    const char8_t *mLegalDocContent;
    uint32_t mLegalDocContentSize;

    NucleusGetLegalDocContent mGetLegalDocContent;
    NucleusGetLegalDocUri mGetLegalDocUri;
};

}
}

#endif
