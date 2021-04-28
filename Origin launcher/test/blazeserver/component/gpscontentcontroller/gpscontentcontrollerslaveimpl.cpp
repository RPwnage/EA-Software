/*************************************************************************************************/
/*!
    \file   gpscontentcontrollerslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GpsContentControllerSlaveImpl

    GpsContentController Slave implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"

// blaze includes
#include "framework/controller/controller.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"
#include "framework/protocol/shared/httpparam.h"
#include "framework/protocol/restprotocolutil.h"
#include "framework/util/xmlbuffer.h"
#include "framework/connection/connectionmanager.h"
#include "framework/connection/nameresolver.h"
#include "framework/connection/socketutil.h"
#include "framework/petitionablecontent/petitionablecontent.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/controller/processcontroller.h"
#include "framework/connection/outboundhttpservice.h"

// gpscontentcontroller includes
#include "gpscontentcontrollerslaveimpl.h"
#include "gpscontentcontroller/tdf/gpscontentcontrollertypes.h"
#include "gpscontentcontroller/tdf/gpscontentcontroller_server.h"

namespace Blaze
{
namespace GpsContentController
{

// name of the http connection manager used by the gpscontentcontroller component, registered with gOutboundHttpService
const char8_t* GPS_HTTP_SERVICE_NAME = "GpsConnMgr";

// static
GpsContentControllerSlave* GpsContentControllerSlave::createImpl()
{
    return BLAZE_NEW_NAMED("GpsContentControllerSlaveImpl") GpsContentControllerSlaveImpl();
}

/*** Public Methods ******************************************************************************/

bool GpsContentControllerSlaveImpl::onValidateConfig(GpsContentControllerConfig& config, const GpsContentControllerConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    //check the product by service name
    for(GpsParametersByServiceName::const_iterator iter=getConfig().getGpsParameters().begin();
        iter!=getConfig().getGpsParameters().end();++iter)
    {
        if (blaze_strcmp(iter->second->getGpsProduct(), "") == 0)
        {
            eastl::string msg;
            msg.sprintf("[GpsContentControllerSlaveImpl].onValidateConfig: missing configuration parameter: product in BlazeServiceName:%s", (iter->first).c_str());
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
    }

    //check the default product
    if (blaze_strcmp(config.getDefaultProduct(), "") == 0)
    {
        eastl::string msg;
        msg.sprintf("[GpsContentControllerSlaveImpl].onValidateConfig: missing configuration parameter: default product");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }

    return validationErrors.getErrorMessages().empty();
}

bool GpsContentControllerSlaveImpl::onConfigure()
{
    TRACE_LOG("[GpsContentControllerSlaveImpl].onConfigure: reconfiguring component.");

    HttpConnectionManagerPtr gpsConnMgr = gOutboundHttpService->getConnection(GPS_HTTP_SERVICE_NAME);
    if (gpsConnMgr == nullptr)
    {
        FATAL_LOG("[GpsContentControllerSlaveImpl].onConfigure: GpsContentController component has dependency on http service '" << GPS_HTTP_SERVICE_NAME <<
            "'. Please make sure this service is defined in framework.cfg.");
        return false;
    }

    mRequestorId = gProcessController->getVersion().getVersion();

    return true;
}

bool GpsContentControllerSlaveImpl::onReconfigure()
{
    return onConfigure();
}

const char8_t* GpsContentControllerSlaveImpl::getGPSPlatformStringFromClientPlatformType(const ClientPlatformType clientPlatformType) const
{
    switch (clientPlatformType)
    {
        case INVALID:
            return "Invalid";
        case pc:
            return "PC";
        case android:
            return "Android";
        case ios:
            return "iOS";
        case common:
            return "Common";        
        case legacyprofileid:
            return "Legacyprofileid";
        case mobile:
        case verizon:
            return "Mobile";
        case facebook:
        case facebook_eacom:
        case bebo:
        case friendster: 
        case twitter:
            return "Web";
        case xone:
            return "XBOXONE";
        case ps4:
            return "PS4";
        case steam:
            return "STEAM";
        case stadia:
            return "STADIA"; 
        default:
            // There are no new platform names needed to map here for Gen5, as WWCE has deprecated/decommissioned GPS in favor of their newer Terms of Service Petitioning Tool, for all future titles to use instead.
            return ClientPlatformTypeToString(clientPlatformType);
    }
} 

BlazeRpcError GpsContentControllerSlaveImpl::filePetitionHttpRequest(
    const FilePetitionRequest &req, 
    FilePetitionResponse &resp) const
{
    const char8_t* accountIdType = "NUCLEUS";

    OutboundHttpConnectionManagerPtr httpConnManager = gOutboundHttpService->getConnection(GPS_HTTP_SERVICE_NAME);
    if (httpConnManager == nullptr)
    {
        ERR_LOG("[GpsContentControllerSlaveImpl].filePetitionHttpRequest: required http service '" << GPS_HTTP_SERVICE_NAME << "' is not registered.");
        return Blaze::ERR_SYSTEM;
    }

    eastl::string strHttpHeader;
    strHttpHeader.sprintf("GPS-RequestorId: %s", mRequestorId.c_str());
    const char8_t *httpHeaders[1];
    httpHeaders[0] = strHttpHeader.c_str();
    
    // create a list of HttpParam
    eastl::vector<HttpParam> params;

    // reserve 
    // 1) known fixed params plus 
    // + assumed specified 3 params per target
    // + customer specified name/value pair of petition
    // + assumed content attribute name/value pair (hard code as 10)
    params.reserve(11 + req.getTargetUsers().size() * 3  + req.getAttributeMap().size()+ 10);

    HttpParam param;

    param.name = "accountId";
    eastl::string accountId;
    accountId.sprintf("%" PRId64, gCurrentUserSession->getAccountId());
    param.value = accountId.c_str();
    param.encodeValue = true;
    params.push_back(param);

    param.name = "accountType";
    param.value = accountIdType;
    param.encodeValue = true;
    params.push_back(param);

    param.name = "persona";
    eastl::string personaId;
    personaId.sprintf("%" PRId64, gCurrentUserSession->getBlazeId());
    param.value = personaId.c_str();
    param.encodeValue = true;
    params.push_back(param);

    param.name = "platform";
    const char8_t* currentUserServiceName = gCurrentUserSession->getServiceName();
    const ServiceNameInfo* serviceNameInfo = gController->getServiceNameInfo(currentUserServiceName);
    if(serviceNameInfo == nullptr)
    {
        ERR_LOG("[GpsContentControllerSlaveImpl].filePetitionHttpRequest: service name info is nullptr.");
        return Blaze::ERR_SYSTEM;
    }    
    param.value = getGPSPlatformStringFromClientPlatformType(serviceNameInfo->getPlatform());
    param.encodeValue = true;
    params.push_back(param);

    param.name = "product";
    //get the product by service name
    bool isGetProductName = false;
    for(GpsParametersByServiceName::const_iterator gpsiter = getConfig().getGpsParameters().begin();
        gpsiter != getConfig().getGpsParameters().end(); ++gpsiter)
    {
        const char8_t* serviceName = gpsiter->first;
        if (blaze_stricmp(serviceName, currentUserServiceName) == 0)
        {
            param.value = gpsiter->second->getGpsProduct();
            isGetProductName = true;
            break;
        }
    }

    if (!isGetProductName)
    {
        param.value = getConfig().getDefaultProduct();
    }

    param.encodeValue = true;
    params.push_back(param);

    param.name = "complaintType";
    param.value = req.getComplaintType();
    param.encodeValue = true;
    params.push_back(param);

    param.name = "subject";
    param.value = req.getSubject();
    param.encodeValue = true;
    params.push_back(param);

    param.name = "timeZone";
    param.value = req.getTimeZone();
    param.encodeValue = true;
    params.push_back(param);

    param.name = "petitionDetail";
    param.value = req.getPetitionDetail();
    param.encodeValue = true;
    params.push_back(param);

    param.name = "contentType";
    param.value = PetitionContentTypeToString(req.getContentType());
    param.encodeValue = true;
    params.push_back(param);

    param.name = "location";
    eastl::string location;
    // FIFA SPECIFIC CODE START
    location.sprintf("FIFA22KC");
    // FIFA SPECIFIC CODE END
    param.value = location.c_str();
    param.encodeValue = true;
    params.push_back(param);

    // customer specified name/value pairs for the petition
    Collections::AttributeMap::const_iterator itA = req.getAttributeMap().begin();
    Collections::AttributeMap::const_iterator itAend = req.getAttributeMap().end();
    for (; itA != itAend; ++itA)
    {
        param.name = itA->first.c_str();
        param.value = itA->second.c_str();
        params.push_back(param);
    }

    // add content for each component to params
    Collections::AttributeMap attribMap;
    eastl::string url;

    if (req.getContentId() != EA::TDF::OBJECT_ID_INVALID)
    {
        if (gPetitionableContentManager->fetchContent(req.getContentId(), attribMap, url) == Blaze::ERR_OK)
        {
            // attribute name/value pair for the content
            Collections::AttributeMap::const_iterator it = attribMap.begin();
            Collections::AttributeMap::const_iterator itend = attribMap.end();
            for (; it != itend; ++it)
            {
                param.name = it->first.c_str();
                param.value = it->second.c_str();
                params.push_back(param);
            }
        }
    }

    eastl::string urlView;
    eastl::string urlShow;
    eastl::string urlHide;
    
    if (req.getContentId() != EA::TDF::OBJECT_ID_INVALID)
    {
        // setup show/hide/view URLS
        eastl::string baseUrl;

        if ( *(getConfig().getWalProxyUrlBase()) != '\0')
        {
            const char8_t* hostName = nullptr;
            bool secure = false;
            HttpProtocolUtil::getHostnameFromConfig(getConfig().getWalProxyUrlBase(), hostName, secure);
            baseUrl.sprintf("%s%s/%s", (secure ? "https://" : "http://"), hostName, gCurrentUserSession->getServiceName());
        }
        else
        {
            const Blaze::ConnectionManager::EndpointList &endPoints 
                = gController->getConnectionManager().getEndpoints();

            Blaze::ConnectionManager::EndpointList::const_iterator it = endPoints.begin();
            for (;it != endPoints.end(); it ++)
            {
                const Endpoint *endPoint = *it;
                if (endPoint->getDecoderType() == Decoder::HTTP)
                {
                    uint16_t port = endPoint->getPort(InetAddress::HOST);
                    uint32_t addr = NameResolver::getInternalAddress().getIp();
                    baseUrl.sprintf("http://%d.%d.%d.%d:%u", NIPQUAD(addr), port);
                    break;
                }
            }
        }

        if (baseUrl.length() == 0)
        {
            ERR_LOG("[GpsContentControllerSlaveImpl].filePetitionHttpRequest: Unable to resolve base URL for this blaze server");
            return Blaze::ERR_SYSTEM;
        }

        param.name = "viewURL";
        if (url.length() == 0)
            urlView.sprintf("%s/gpscontentcontroller/fetchContent/[sessionkey]?coid=%s", baseUrl.c_str(), req.getContentId().toString().c_str());
        else
            urlView = url;
        param.value = urlView.c_str();
        param.encodeValue = true;
        params.push_back(param);

        param.name = "hideURL";
        urlHide.sprintf("%s/gpscontentcontroller/showContent/[sessionkey]?coid=%s&show=0", baseUrl.c_str(), req.getContentId().toString().c_str());
        param.value = urlHide.c_str();
        param.encodeValue = true;
        params.push_back(param);
        
        param.name = "showURL";
        urlShow.sprintf("%s/gpscontentcontroller/showContent/[sessionkey]?coid=%s&show=1", baseUrl.c_str(), req.getContentId().toString().c_str());
        param.value = urlShow.c_str();
        param.encodeValue = true;
        params.push_back(param);
        
        TRACE_LOG("[GpsContentControllerSlaveImpl].filePetitionHttpRequest: viewURL: " << urlView.c_str());
        TRACE_LOG("[GpsContentControllerSlaveImpl].filePetitionHttpRequest: showURL: " << urlShow.c_str());
        TRACE_LOG("[GpsContentControllerSlaveImpl].filePetitionHttpRequest: hideURL: " << urlHide.c_str());
    }
    
    // check for targets
    BlazeIdList::const_iterator it = req.getTargetUsers().begin();
    BlazeIdList::const_iterator end = req.getTargetUsers().end();

    eastl::vector<eastl::string> paramNames;
    eastl::vector<eastl::string> personas;
    eastl::vector<eastl::string> accountIds;

    paramNames.insert(0, req.getTargetUsers().size() * 3, eastl::string());
    personas.insert(0, req.getTargetUsers().size(), eastl::string());
    accountIds.insert(0, req.getTargetUsers().size(), eastl::string());
    
    int count = 0;
    for (;it != end; it++, ++count)
    {
        BlazeId blazeId = *it;
        UserInfoPtr info;
        BlazeRpcError lookupError = gUserSessionManager->lookupUserInfoByBlazeId(blazeId, info);
        
        if (lookupError != Blaze::ERR_OK)
        {
            WARN_LOG("[filePetitionHttpRequest] Could not resolve BlazeId: " << blazeId);
            return Blaze::ERR_SYSTEM;
        }

        const uint32_t index = count * 2;
        const uint32_t userIndex = count + 1;

        paramNames[index].sprintf("target%daccountId", userIndex);
        param.name = paramNames[index].c_str();
        accountIds[count].sprintf("%" PRId64, info->getPlatformInfo().getEaIds().getNucleusAccountId());
        param.value = accountIds[count].c_str();
        param.encodeValue = true;
        params.push_back(param);

        paramNames[index + 1].sprintf("target%daccountType", userIndex);
        param.name = paramNames[index + 1].c_str();
        param.value = accountIdType;
        param.encodeValue = true;
        params.push_back(param);

        paramNames[index + 2].sprintf("target%dpersona", userIndex);
        param.name = paramNames[index + 2].c_str();
        personas[count].sprintf("%" PRId64, info->getId());
        param.value = personas[count].c_str();
        param.encodeValue = true;
        params.push_back(param);
    }

    const HttpParam* httpParams = &params[0];

    Blaze::GpsContentController::ErrorResponse errorResponse;
    RestOutboundHttpResult restResult(Blaze::GpsContentController::GpsContentControllerSlave::CMD_INFO_FILEPETITION.restResourceInfo, &resp, &errorResponse, nullptr, nullptr);

    BlazeRpcError err = httpConnManager->sendRequest(
        HttpProtocolUtil::HTTP_POST,
        "/event/petition",
        httpParams,
        static_cast<uint32_t>(params.size()),
        httpHeaders,
        EAArrayCount(httpHeaders),
        &restResult);

    if (err != ERR_OK || restResult.getBlazeErrorCode() != ERR_OK)
    {
        if (restResult.getBlazeErrorCode() != ERR_OK)
            err = restResult.getBlazeErrorCode();

        if (errorResponse.getErrors().empty())
        {
            WARN_LOG("[GpsContentControllerSlaveImpl].filePetitionHttpRequest: File petition failed with error: " << ErrorHelp::getErrorName(err));
        }
        else
        {
            WARN_LOG("[GpsContentControllerSlaveImpl].filePetitionHttpRequest: File petition failed with error: " << ErrorHelp::getErrorName(err) << ", Code: "
                << errorResponse.getErrors()[0]->getCode() << ", Field: " << errorResponse.getErrors()[0]->getField() << ", Details: " << errorResponse.getErrors()[0]->getMessage());
        }
    }

   return err;
}

} // GpsContentController
} // Blaze
