/*************************************************************************************************/
/*!
    \file   nucleusgetlegaldoccontent.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_AUTHENTICATION_NUCLEUSGETLEGALDOCCONTENT_H
#define BLAZE_AUTHENTICATION_NUCLEUSGETLEGALDOCCONTENT_H

/*** Include files *******************************************************************************/
#include "authentication/nucleushandler/nucleushandler.h"
#include "authentication/nucleusresult/nucleusintermediateresult.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Authentication
{

class NucleusGetLegalDocContentResult : public NucleusIntermediateResult
{
public:
    NucleusGetLegalDocContentResult()
      : mContent(nullptr),
        mContentSize(0),
        mHasTosServerError(false)
    {}

    ~NucleusGetLegalDocContentResult () override 
    {
        if (mContent != nullptr)
        {
            BLAZE_FREE(mContent);
            mContent = nullptr;
        }
    }

public:
    BlazeRpcError decodeResult(RawBuffer& buffer) override;

    const char8_t* getContent() const { return mContent; }
    uint32_t getContentSize() const { return mContentSize; }
    bool hasTosServerError() const { return mHasTosServerError; }

private:
    char8_t *mContent;
    uint32_t mContentSize;
    bool mHasTosServerError;
};

class NucleusGetLegalDocContent : public NucleusHandler
{
public:

    NucleusGetLegalDocContent(const HttpConnectionManagerPtr &connManager, const AuthenticationConfig::NucleusBypassURIList* bypassList = nullptr, NucleusBypassType bypassType = BYPASS_NONE) 
      : NucleusHandler(connManager, bypassList, bypassType) {}

    ~NucleusGetLegalDocContent () override {}

    bool getLegalDocContent(const char8_t *docType, const char8_t* lang, const char8_t* country, const ClientPlatformType clientPlatformType, const char8_t* contentType, const char8_t* gameIdentifier, const Command* callingCommand = nullptr);   

    const NucleusGetLegalDocContentResult *getResult() const { return &mResult; }

private:
    NucleusResult *getResultBase() override { return &mResult; }

    // Owned memory.
    NucleusGetLegalDocContentResult mResult;
};

} // Authentication
} // Blaze

#endif // BLAZE_AUTHENTICATION_NUCLEUSGETLEGALDOCCONTENT_H
