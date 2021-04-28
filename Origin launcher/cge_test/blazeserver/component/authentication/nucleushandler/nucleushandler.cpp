/*************************************************************************************************/
/*!
    \file   nucleushandler.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class NucleusHandler

    Base class for all Nucleus Handlers.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/connection/socketutil.h"
#include "framework/tdf/userevents_server.h"

// authentication includes
#include "authentication/nucleushandler/nucleushandler.h"
#include "authentication/nucleusresult/nucleusresult.h"

namespace Blaze
{
namespace Authentication
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Protected Methods ***************************************************************************/
bool NucleusHandler::sendGetRequest(const char8_t* URI, const HttpParam params[], uint32_t paramsize, const char8_t* httpHeaders[], uint32_t headerCount, bool logOnError/* = false*/,
                                    const Command* callingCommand/* = nullptr*/, bool xmlEncodeRequest/* = false*/)
{
    getResultBase()->clearAll();

    // enable Nucleus bypass for configured dedicated servers
    if ( mNucleusBypassType != BYPASS_NONE)
    {
        TRACE_LOG("[NucleusHandler].sendGetRequest: Bypassing Nucleus and reading nucleus result from file - uri(" << URI << ")");
        if (readFromXmlFile(URI, params, paramsize))
        {
            return true;
        }
    }

    BlazeRpcError err = mConnectionManager->sendRequest(HttpProtocolUtil::HTTP_GET, URI, params, paramsize, httpHeaders, headerCount, getResultBase(),
        Blaze::COMPONENT_TYPE_INDEX_authentication, xmlEncodeRequest);

    if ((err != ERR_OK) &&
        (logOnError))
    {
        eastl::string strHttpRequestLog;
        HttpProtocolUtil::printHttpRequest(mConnectionManager->isSecure(), HttpProtocolUtil::HTTP_GET,
            mConnectionManager->getAddress().getHostname(), mConnectionManager->getAddress().getPort(InetAddress::HOST), URI, params, paramsize, httpHeaders, headerCount,
            strHttpRequestLog);
        BLAZE_WARN_LOG(Log::HTTP, "[NucleusHandler].sendRequest: failed with error[" << ErrorHelp::getErrorName(err) 
            << "], NucleusCode[" << getResultBase()->getCodeString() << "], NucleusField[" << getResultBase()->getFieldString()
            << "], NucleusCause[" << getResultBase()->getCauseString() << "\nrequest: " << strHttpRequestLog);
    }

    return (err == ERR_OK && !getResultBase()->hasError());
}

bool NucleusHandler::sendPostRequest(const char8_t* URI, const HttpParam params[], uint32_t paramsize, const char8_t* httpHeaders[], uint32_t headerCount, bool logOnError/* = false*/,
                                     const Command* callingCommand/* = nullptr*/, bool xmlEncodeRequest/* = false*/)
{
    getResultBase()->clearAll();
    // enable Nucleus bypass for configured dedicated servers
    if ( mNucleusBypassType != BYPASS_NONE )
    {
        TRACE_LOG("[NucleusHandler].sendPostRequest: Bypassing Nucleus and reading nucleus result from file - uri(" << URI << ")");
        if (readFromXmlFile(URI, params, paramsize))
        {
            return true;
        }
    }

    BlazeRpcError err = mConnectionManager->sendRequest(HttpProtocolUtil::HTTP_POST, URI, params, paramsize, httpHeaders, headerCount, getResultBase(),
        Blaze::COMPONENT_TYPE_INDEX_authentication, xmlEncodeRequest);

    if ((err != ERR_OK) &&
        (logOnError))
    {
        eastl::string strHttpRequestLog;
        HttpProtocolUtil::printHttpRequest(mConnectionManager->isSecure(), HttpProtocolUtil::HTTP_POST,
            mConnectionManager->getAddress().getHostname(), mConnectionManager->getAddress().getPort(InetAddress::HOST), URI, params, paramsize, httpHeaders, headerCount,
            strHttpRequestLog);
        BLAZE_WARN_LOG(Log::HTTP, "[NucleusHandler].sendRequest: failed with error[" << ErrorHelp::getErrorName(err) 
            << "], NucleusCode[" << getResultBase()->getCodeString() << "], NucleusField[" << getResultBase()->getFieldString()
            << "], NucleusCause[" << getResultBase()->getCauseString() << "\nrequest: " << strHttpRequestLog);
    }

    return (err == ERR_OK && !getResultBase()->hasError());
}

bool NucleusHandler::sendPutRequest(const char8_t* URI, const HttpParam params[], uint32_t paramsize, const char8_t* httpHeaders[], uint32_t headerCount, bool logOnError/* = false*/,
                                    const Command* callingCommand/* = nullptr*/, bool xmlEncodeRequest/* = false*/)
{
    getResultBase()->clearAll();
    if (mNucleusBypassType == BYPASS_ALL)
    {
        TRACE_LOG("[NucleusHandler].sendPutRequest: Bypassing Nucleus - uri(" << URI << ")");
        return true;
    }

    // enable Nucleus bypass for configured dedicated servers and only skip PUT requests that match configuration
    if (mNucleusBypassType == BYPASS_DEDICATED_SERVER_CLIENT_TYPE)
    {
        TRACE_LOG("[NucleusHandler].sendPutRequest: Bypassing Nucleus and reading nucleus result from file - uri(" << URI << ")");
        if (readFromXmlFile(URI, params, paramsize))
        {
            return true;
        }
    }

    BlazeRpcError err = mConnectionManager->sendRequest(HttpProtocolUtil::HTTP_PUT, URI, params, paramsize, httpHeaders, headerCount, getResultBase(),
        Blaze::COMPONENT_TYPE_INDEX_authentication, xmlEncodeRequest);

    if ((err != ERR_OK) &&
        (logOnError))
    {
        eastl::string strHttpRequestLog;
        HttpProtocolUtil::printHttpRequest(mConnectionManager->isSecure(), HttpProtocolUtil::HTTP_PUT,
            mConnectionManager->getAddress().getHostname(), mConnectionManager->getAddress().getPort(InetAddress::HOST), URI, params, paramsize, httpHeaders, headerCount,
            strHttpRequestLog);
        BLAZE_WARN_LOG(Log::HTTP, "[NucleusHandler].sendRequest: failed with error[" << ErrorHelp::getErrorName(err) 
            << "], NucleusCode[" << getResultBase()->getCodeString() << "], NucleusField[" << getResultBase()->getFieldString()
            << "], NucleusCause[" << getResultBase()->getCauseString() << "\nrequest: " << strHttpRequestLog);
    }

    return (err == ERR_OK && !getResultBase()->hasError());
}

bool NucleusHandler::sendDeleteRequest(const char8_t* URI, const HttpParam params[], uint32_t paramsize, const char8_t* httpHeaders[], uint32_t headerCount, bool logOnError/* = false*/,
                                       const Command* callingCommand/* = nullptr*/, bool xmlEncodeRequest/* = false*/)
{
    getResultBase()->clearAll();

    BlazeRpcError err = mConnectionManager->sendRequest(HttpProtocolUtil::HTTP_DELETE, URI, params, paramsize, httpHeaders, headerCount, getResultBase(),
        Blaze::COMPONENT_TYPE_INDEX_authentication, xmlEncodeRequest);

    if ((err != ERR_OK) &&
        (logOnError))
    {
        eastl::string strHttpRequestLog;
        HttpProtocolUtil::printHttpRequest(mConnectionManager->isSecure(), HttpProtocolUtil::HTTP_DELETE,
            mConnectionManager->getAddress().getHostname(), mConnectionManager->getAddress().getPort(InetAddress::HOST), URI, params, paramsize, httpHeaders, headerCount,
            strHttpRequestLog);
        BLAZE_WARN_LOG(Log::HTTP, "[NucleusHandler].sendRequest: failed with error[" << ErrorHelp::getErrorName(err) 
            << "], NucleusCode[" << getResultBase()->getCodeString() << "], NucleusField[" << getResultBase()->getFieldString()
            << "], NucleusCause[" << getResultBase()->getCauseString() << "\nrequest: " << strHttpRequestLog);
    }

    return (err == ERR_OK && !getResultBase()->hasError());
}

bool NucleusHandler::sendHeadRequest(const char8_t* URI, const HttpParam params[], uint32_t paramsize, const char8_t *httpHeaders[], uint32_t headerCount, bool logOnError/* = false*/,
                                     const Command* callingCommand/* = nullptr*/, bool xmlEncodeRequest/* = false*/)
{
    getResultBase()->clearAll();
    // enable Nucleus bypass for configured dedicated servers
    if (  mNucleusBypassType != BYPASS_NONE )
    {
        TRACE_LOG("[NucleusHandler].sendHeadRequest: Bypassing Nucleus and reading nucleus result from file - uri(" << URI << ")");
        if (readFromXmlFile(URI, params, paramsize))
        {
            return true;
        }
    }

    BlazeRpcError err = mConnectionManager->sendRequest(HttpProtocolUtil::HTTP_HEAD, URI, params, paramsize, httpHeaders, headerCount, getResultBase(),
        Blaze::COMPONENT_TYPE_INDEX_authentication, xmlEncodeRequest);

    if ((err != ERR_OK) &&
        (logOnError))
    {
        eastl::string strHttpRequestLog;
        HttpProtocolUtil::printHttpRequest(mConnectionManager->isSecure(), HttpProtocolUtil::HTTP_HEAD,
            mConnectionManager->getAddress().getHostname(), mConnectionManager->getAddress().getPort(InetAddress::HOST), URI, params, paramsize, 
            httpHeaders, headerCount,
            strHttpRequestLog);
        BLAZE_WARN_LOG(Log::HTTP, "[NucleusHandler].sendRequest: failed with error[" << ErrorHelp::getErrorName(err) 
            << "], NucleusCode[" << getResultBase()->getCodeString() << "], NucleusField[" << getResultBase()->getFieldString()
            << "], NucleusCause[" << getResultBase()->getCauseString() << "\nrequest: " << strHttpRequestLog);
    }

    return (err == ERR_OK && !getResultBase()->hasError());
}

bool NucleusHandler::readFromXmlFile(const char8_t* URI, const HttpParam params[], uint32_t paramsize)
{
    if (mBypassList == nullptr)
    {
        return false;
    }

    for (AuthenticationConfig::NucleusBypassURIList::const_iterator bitr = mBypassList->begin(), bend = mBypassList->end(); bitr != bend; ++bitr)
    {
        const NucleusBypassURI* currentURI = *bitr;
        if (blaze_strcmp(currentURI->getURI(), URI) != 0)
            continue;

        for (NucleusBypassURI::ParamMapList::const_iterator pitr = currentURI->getXML().begin(), pend = currentURI->getXML().end(); pitr != pend; ++pitr)
        {
            const NucleusBypassURI::ParamMap* pmap = *pitr;
            bool paramsMatched = true;
            NucleusBypassURI::ParamMap::const_iterator itr = pmap->begin();
            NucleusBypassURI::ParamMap::const_iterator end = pmap->end();

            for(;itr != end; itr++)
            {
                // the params specified in config file must match in value with the HttpParams
                // the params not specified in config file can be wildcard value
                const char8_t* paramKey = itr->first;
                const char8_t* paramValue = itr->second;
                if (blaze_strcmp(paramKey, "XML_OUTPUT") == 0)
                    continue;
                bool paramFound = false;
                for (uint32_t i = 0; i < paramsize; i++)
                {
                    if (blaze_strcmp(paramKey, params[i].name) == 0)
                    {
                        paramFound = true;
                        if (blaze_strcmp(paramValue, params[i].value) != 0)
                        {
                            SPAM_LOG("[NucleusHandler].readFromXmlFile: Failed to find parameter '" << params[i].name << "' in config. Expected: '" 
                                << params[i].value << "', but got '" << paramValue << "'");
                            paramsMatched = false;
                        }
                        break;
                    }
                }
                if (!paramFound)
                    paramsMatched = false;
            }

            if (paramsMatched)
            {
                NucleusBypassURI::ParamMap::const_iterator xmlItr = pmap->find("XML_OUTPUT");
                if (xmlItr != pmap->end())
                {
                    const char8_t* xmlOutput = xmlItr->second.c_str();
                    TRACE_LOG("[NucleusHandler].readFromXmlFile: Read nucleus result from XML file: \n " << xmlOutput);

                    getResultBase()->setHttpStatusCode(HTTP_STATUS_OK);

                    RawBuffer buffer((uint8_t*)xmlOutput, sizeof(xmlOutput));
                    BlazeRpcError result = getResultBase()->decodeResult(buffer);
                    if (result != ERR_OK)
                    {
                        getResultBase()->setHttpError(result);
                    }
                    return true;
                }
            }
        }
    }

    // Don't print the warning if the current user is already logged in but is trying to make a Nucleus call
    // to retrieve info for another Nucleus user.  i.e. Dedicated server trying to fetch entitlements for one of the other players
    AccountId userToLookup = INVALID_ACCOUNT_ID;
    const char8_t USERID_PREFIX[] = "/users/";
    const char8_t* pUserId = blaze_strstr(URI, USERID_PREFIX);
    if (pUserId != nullptr)
    {
        pUserId += sizeof(USERID_PREFIX) - 1;
        blaze_str2int(pUserId, &userToLookup);
    }
    if (gCurrentUserSession == nullptr ||
        gCurrentUserSession->getBlazeId() == userToLookup)
    {
        WARN_LOG("[NucleusHandler].readFromXmlFile: No nucleus XML file configured for uri=" << URI);
    }

    return false;
}

/*** Private Methods *****************************************************************************/

} // Authentication
} // Blaze

