/*************************************************************************************************/
/*!
    \file   nucleusgetlegaldocuri.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_AUTHENTICATION_NUCLEUSGETLEGALDOCURI_H
#define BLAZE_AUTHENTICATION_NUCLEUSGETLEGALDOCURI_H

/*** Include files *******************************************************************************/
#include "authentication/nucleushandler/nucleushandler.h"
#include "authentication/nucleusresult/nucleusintermediateresult.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Authentication
{

class NucleusGetLegalDocUriResult : public NucleusIntermediateResult
{
public:
    BlazeRpcError decodeResult(RawBuffer& buffer) override;
};

class NucleusGetLegalDocUri : public NucleusHandler
{
public:

    NucleusGetLegalDocUri(const HttpConnectionManagerPtr &connManager, const AuthenticationConfig::NucleusBypassURIList* bypassList = nullptr, NucleusBypassType bypassType = BYPASS_NONE) 
      : NucleusHandler(connManager, bypassList, bypassType) {}

    ~NucleusGetLegalDocUri () override {}

    // Get account info.
    bool getLegalDocUri(const char8_t *docType, const char8_t* lang, const char8_t* country, const ClientPlatformType clientPlatformType, const char8_t* contentType, const char8_t* gameIdentifier, const Command* callingCommand = nullptr) WARN_UNUSED_RESULT;

    NucleusIntermediateResult *getResult() { return &mResult; }

private:
    NucleusResult *getResultBase() override { return &mResult; }

    // Owned memory.
    NucleusGetLegalDocUriResult mResult;
};

} // Authentication
} // Blaze

#endif // BLAZE_AUTHENTICATION_NUCLEUSGETLEGALDOCURI_H
