/*************************************************************************************************/
/*!
    \file   nucleushandler.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_AUTHENTICATION_NUCLEUSHANDLER_H
#define BLAZE_AUTHENTICATION_NUCLEUSHANDLER_H

/*** Include files *******************************************************************************/
#include "authentication/authenticationimpl.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
// Forward declarations
class OutboundHttpConnectionManager;

namespace Authentication
{

class NucleusResult;

class NucleusHandler
{
public:
    NucleusHandler(const HttpConnectionManagerPtr &connManager, const AuthenticationConfig::NucleusBypassURIList* bypassList = nullptr, NucleusBypassType bypassType = BYPASS_NONE)
      : mConnectionManager(connManager),
        mNucleusBypassType(bypassType),
        mBypassList(bypassList)
    {}

    virtual ~NucleusHandler() {}

    NucleusHandler& operator=(const NucleusHandler&);

    static char8_t *padExternalRef16(const char8_t *extRef, char8_t *buf, size_t bufSize)
    {
        const char* zeros="0000000000000000";
        blaze_snzprintf(buf, bufSize, "%.*s%s",static_cast<int>(16-strlen(extRef)),zeros, extRef);
        return buf;
    }

protected:
    virtual NucleusResult *getResultBase() =0;

    // Client functions.
    bool sendGetRequest(const char8_t* URI, const HttpParam params[] = nullptr, uint32_t paramsize = 0, const char8_t* httpHeaders[] = nullptr, uint32_t headerCount = 0, bool logOnError = false,
        const Command* callingCommand = nullptr, bool xmlEncodeRequest = false) WARN_UNUSED_RESULT;
    bool sendPostRequest(const char8_t* URI, const HttpParam params[] = nullptr, uint32_t paramsize = 0, const char8_t* httpHeaders[] = nullptr, uint32_t headerCount = 0, bool logOnError = false,
        const Command* callingCommand = nullptr, bool xmlEncodeRequest = false) WARN_UNUSED_RESULT;
    bool sendPutRequest(const char8_t* URI, const HttpParam params[] = nullptr, uint32_t paramsize = 0, const char8_t* httpHeaders[] = nullptr, uint32_t headerCount = 0, bool logOnError = false,
        const Command* callingCommand = nullptr, bool xmlEncodeRequest = false) WARN_UNUSED_RESULT;
    bool sendDeleteRequest(const char8_t* URI, const HttpParam params[] = nullptr, uint32_t paramsize = 0, const char8_t* httpHeaders[] = nullptr, uint32_t headerCount = 0, bool logOnError = false,
        const Command* callingCommand = nullptr, bool xmlEncodeRequest = false) WARN_UNUSED_RESULT;
    bool sendHeadRequest(const char8_t* URI, const HttpParam params[], uint32_t paramsize, const char8_t *httpHeaders[] = nullptr, uint32_t headerCount = 0, bool logOnError = false,
        const Command* callingCommand = nullptr, bool xmlEncodeRequest = false) WARN_UNUSED_RESULT;

    // Owned memory.
    HttpConnectionManagerPtr mConnectionManager;
    NucleusBypassType mNucleusBypassType;
    const AuthenticationConfig::NucleusBypassURIList* mBypassList;

private:
    bool readFromXmlFile(const char8_t* URI, const HttpParam params[] = nullptr, uint32_t paramsize = 0) WARN_UNUSED_RESULT;
};

} // Authentication
} // Blaze

#endif // BLAZE_AUTHENTICATION_NUCLEUSHANDLER_H
