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
#include "framework/connection/connectionmanager.h"
#include "framework/connection/nameresolver.h"
#include "framework/connection/socketutil.h"
#include "framework/petitionablecontent/petitionablecontent.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/controller/processcontroller.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/connection/outboundhttpconnectionmanager.h"

// contentreporter includes
#include "contentreporterslaveimpl.h"
#include "contentreporter/filereporthttpresponse.h"
#include "contentreporter/tdf/contentreporter_server.h"

namespace Blaze
{
namespace ContentReporter
{

// name of the http connection manager used by the contentreporter component, registered with gOutboundHttpService
const char8_t* BIFROST_HTTP_SERVICE_NAME = "BifrostConnMgr";

// static
ContentReporterSlave* ContentReporterSlave::createImpl()
{
    return BLAZE_NEW_NAMED("ContentReporterSlaveImpl") ContentReporterSlaveImpl();
}

/*** Public Methods ******************************************************************************/


bool ContentReporterSlaveImpl::onValidateConfig(ContentReporterConfig& config, const ContentReporterConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    //check the default product
    if (blaze_strcmp(config.getDefaultProduct(), "") == 0)
    {
        eastl::string msg;
        msg.sprintf("[ContentReporterSlaveImpl].onValidateConfig: missing configuration parameter: default product");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }

	//check for the MaxAllowedBodySize
	if (config.getMaxAllowedBodySize() == 0)
	{
		eastl::string msg;
		msg.sprintf("[ContentReporterSlaveImpl].onValidateConfig: missing configuration parameter: MaxAllowedBodySize");
		EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
		str.set(msg.c_str());
	}

	//check for the walProxyUrlBase
	if (blaze_strcmp(config.getWalProxyUrlBase(), "") == 0)
	{
		eastl::string msg;
		msg.sprintf("[ContentReporterSlaveImpl].onValidateConfig: missing configuration parameter: walProxyUrlBase");
		EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
		str.set(msg.c_str());
	}

	//check for the utasUrlBase
	if (blaze_strcmp(config.getUtasUrlBase(), "") == 0)
	{
		eastl::string msg;
		msg.sprintf("[ContentReporterSlaveImpl].onValidateConfig: missing configuration parameter: utasUrlBase");
		EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
		str.set(msg.c_str());
	}

    return validationErrors.getErrorMessages().empty();
}

bool ContentReporterSlaveImpl::onConfigure()
{
    TRACE_LOG("[ContentReporterSlaveImpl].onConfigure: reconfiguring component.");

    HttpConnectionManagerPtr bifrostConnMgr = gOutboundHttpService->getConnection(BIFROST_HTTP_SERVICE_NAME);
    if (bifrostConnMgr == nullptr)
    {
        FATAL_LOG("[ContentReporterSlaveImpl].onConfigure: ContentReporter component has dependency on http service '" << BIFROST_HTTP_SERVICE_NAME <<
            "'. Please make sure this service is defined in framework.cfg.");
        return false;
    }

    mRequestorId = gProcessController->getVersion().getVersion();

	const ContentReporterConfig &config = getConfig();
	mMaxAllowedBodySize = config.getMaxAllowedBodySize();
	mWalProxyUrlBase = config.getWalProxyUrlBase();
	mUtasUrlBase = config.getUtasUrlBase();
	mSsasUrlBase = config.getSsasUrlBase();
	mDefaultSSFTeamName = config.getDefaultSSFTeamName();
	mDefaultSSFAvatarName = config.getDefaultSSFAvatarName();
	mDefaultFUTGameName = config.getDefaultFUTGameName();
	mDefaultGameName = config.getDefaultGameName();

    return true;
}

bool ContentReporterSlaveImpl::onReconfigure()
{
    return onConfigure();
}

const char8_t* ContentReporterSlaveImpl::getPlatformStringFromClientPlatformType(const ClientPlatformType clientPlatformType) const
{
    switch (clientPlatformType)
    {
	    case ClientPlatformType::INVALID:
            return "invalid";
        case pc:
            return "pc";
        case android:
            return "android";
        case ios:
            return "ios";
        case common:
            return "common";        
        case legacyprofileid:
            return "legacyprofileid";
        case mobile:
        case verizon:
            return "mobile";
        case facebook:
        case facebook_eacom:
        case bebo:
        case friendster: 
        case twitter:
            return "web";
        case xone:
            return "xbox-one";
		case xbsx:
			return "xbsx";
        case ps4:
            return "ps4";
		case ps5:
			return "ps5";
        default:
            return ClientPlatformTypeToString(clientPlatformType);
    }
}

const char8_t* ContentReporterSlaveImpl::getContentTypeStringFromClientContentType(const ReportContentType contentType) const
{
	switch (contentType)
	{
	case CHAT:
		return "CHAT";
	case COMMENT:
		return "COMMENT";
	case POST:
		return "POST";
	case VIDEO:
		return "VIDEO";
	case IMAGE:
		return "IMAGE";
	case AVATAR:
		return "AVATAR";
	case NAME:
		return "NAME";
	case STATUSMESSAGE:
		return "STATUSMESSAGE";
	default:
		return "UNKNOWN";
	}
}

void ContentReporterSlaveImpl::AddItemToPayload(EA::Json::JsonWriter &jsonWriter, const char8_t* name, const char8_t* value) const
{
	jsonWriter.BeginObjectValue(name);
	jsonWriter.String(value);
}

void ContentReporterSlaveImpl::AddItemToPayload(EA::Json::JsonWriter &jsonWriter, const char8_t* name, const int64_t value) const
{
	jsonWriter.BeginObjectValue(name);
	jsonWriter.Integer(value);
}

BlazeRpcError ContentReporterSlaveImpl::fileReportHttpRequest(
	const FileReportRequest &req,
	FileReportResponse &resp) const
{
	AccountId nucleusId = gCurrentUserSession->getAccountId();

	// early return in case we don't have a valid user session
	if (nucleusId == INVALID_ACCOUNT_ID)
	{
		ERR_LOG("[ContentReporterSlaveImpl].fileOnlineReportHttpRequest: invalid nucleus id: " << nucleusId);
		return Blaze::ERR_SYSTEM;
	}

	EA::Json::StringWriteStream<eastl::string> jsonStream;
	EA::Json::JsonWriter jsonWriter;
	jsonWriter.SetFormatOption(EA::Json::JsonWriter::FormatOption::kFormatOptionLineEnd, 0);
	jsonWriter.SetStream(&jsonStream);

	jsonWriter.BeginObject();

	AddItemToPayload(jsonWriter, "petitionerUserId", nucleusId);
	AddItemToPayload(jsonWriter, "petitionerAccountType", "NUCLEUS");
	AddItemToPayload(jsonWriter, "petitionerPersonaName", gCurrentUserSession->getPersonaName());
	AddItemToPayload(jsonWriter, "petitionerPersonaId", gCurrentUserSession->getBlazeId());
	AddItemToPayload(jsonWriter, "petitionerEmail", "EA_unavailable");
	AddItemToPayload(jsonWriter, "product", getConfig().getDefaultProduct());

	const char8_t* currentUserServiceName = gCurrentUserSession != nullptr ? gCurrentUserSession->getServiceName() : gController->getDefaultServiceName();
	const ServiceNameInfo* serviceNameInfo = gController->getServiceNameInfo(currentUserServiceName);
	if (serviceNameInfo == nullptr)
	{
		ERR_LOG("[ContentReporterSlaveImpl].fileReportHttpRequest: service name info is NULL.");
		return Blaze::ERR_SYSTEM;
	}
	AddItemToPayload(jsonWriter, "platform", getPlatformStringFromClientPlatformType(serviceNameInfo->getPlatform()));
	AddItemToPayload(jsonWriter, "targetUserId", req.getTargetUser());
	AddItemToPayload(jsonWriter, "targetAccountType", "NUCLEUS");
	AddItemToPayload(jsonWriter, "category", "inappropriate-name");
	AddItemToPayload(jsonWriter, "contentType", getContentTypeStringFromClientContentType(req.getContentType()));
	AddItemToPayload(jsonWriter, "targetEmail", "EA_unavailable");
	AddItemToPayload(jsonWriter, "subject", req.getSubject());

	jsonWriter.EndObject();

	size_t datasize = jsonStream.mString.length();

	BlazeRpcError err = ERR_SYSTEM;

	if (datasize > 0 && datasize < mMaxAllowedBodySize)
	{
		err = fileHttpRequest(jsonStream.mString.c_str(), datasize, resp);
	}

	return err;
}

BlazeRpcError ContentReporterSlaveImpl::fileOnlineReportHttpRequest(
    const FileOnlineReportRequest &req, 
    FileReportResponse &resp) const
{
	AccountId nucleusId = gCurrentUserSession->getAccountId();

	// early return in case we don't have a valid user session
	if (nucleusId == INVALID_ACCOUNT_ID)
	{
		ERR_LOG("[ContentReporterSlaveImpl].fileOnlineReportHttpRequest: invalid nucleus id: " << nucleusId);
		return Blaze::ERR_SYSTEM;
	}

	EA::Json::StringWriteStream<eastl::string> jsonStream;
	EA::Json::JsonWriter jsonWriter;
	jsonWriter.SetFormatOption(EA::Json::JsonWriter::FormatOption::kFormatOptionLineEnd, 0);
	jsonWriter.SetStream(&jsonStream);

	jsonWriter.BeginObject();

	AddItemToPayload(jsonWriter, "petitionerUserId", nucleusId);
	AddItemToPayload(jsonWriter, "petitionerAccountType", "NUCLEUS");
	AddItemToPayload(jsonWriter, "petitionerPersonaName", gCurrentUserSession->getPersonaName());
	AddItemToPayload(jsonWriter, "petitionerPersonaId", gCurrentUserSession->getBlazeId());
	AddItemToPayload(jsonWriter, "petitionerEmail", "EA_unavailable");
	AddItemToPayload(jsonWriter, "product", getConfig().getDefaultProduct());

	const char8_t* currentUserServiceName = gCurrentUserSession != nullptr ? gCurrentUserSession->getServiceName() : gController->getDefaultServiceName();
	const ServiceNameInfo* serviceNameInfo = gController->getServiceNameInfo(currentUserServiceName);
	if (serviceNameInfo == nullptr)
	{
		ERR_LOG("[ContentReporterSlaveImpl].fileOnlineReportHttpRequest: service name info is NULL.");
		resp.setErrorMessage("Service name info is NULL");
		return Blaze::ERR_SYSTEM;
	}
	AddItemToPayload(jsonWriter, "platform", getPlatformStringFromClientPlatformType(serviceNameInfo->getPlatform()));
	AddItemToPayload(jsonWriter, "targetUserId", req.getCommonAbuseReport().getTargetNucleusId());
	AddItemToPayload(jsonWriter, "targetAccountType", "NUCLEUS");
	AddItemToPayload(jsonWriter, "targetEmail", "EA_unavailable");
	AddItemToPayload(jsonWriter, "category", "inappropriate-name");
	const char8_t* contentType = getContentTypeStringFromClientContentType(req.getCommonAbuseReport().getContentType());
	AddItemToPayload(jsonWriter, "contentType", contentType);
	int64_t targetBlazeId = req.getCommonAbuseReport().getTargetBlazeId();
	AddItemToPayload(jsonWriter, "targetPersonaId", targetBlazeId);
	AddItemToPayload(jsonWriter, "subject", req.getCommonAbuseReport().getSubject());
	AddItemToPayload(jsonWriter, "contentText", req.getContentText());

	OnlineReportType onlineContentType = req.getOnlineReportType();
	int64_t clubId = req.getClubId();
	eastl::string contentUrl;
	eastl::string hideUrl;

	bool validRequest = true;

	switch (onlineContentType)
	{
	case AVATAR_NAME:
		if (targetBlazeId != INVALID_BLAZE_ID)
		{
			contentUrl.sprintf("%s/%s/proclubscustom/getAvatarName/[sessionkey]?ganr=%" PRIu64, mWalProxyUrlBase.c_str(), currentUserServiceName, targetBlazeId);
			AddItemToPayload(jsonWriter, "contentUrl", contentUrl.c_str());

			hideUrl.sprintf("%s/%s/proclubscustom/updateProfaneAvatarName/[sessionkey]?upan=%" PRIu64, mWalProxyUrlBase.c_str(), currentUserServiceName, targetBlazeId);
			AddItemToPayload(jsonWriter, "hideUrl", hideUrl.c_str());
		}
		else
		{
			validRequest = false;
			resp.setErrorMessage("Invalid target blazeId");
		}
		break;
	case CLUB_NAME:
		if (clubId > 0)
		{
			contentUrl.sprintf("%s/%s/clubs/getClubName/[sessionkey]?gcid=%" PRId64, mWalProxyUrlBase.c_str(), currentUserServiceName, clubId);
			AddItemToPayload(jsonWriter, "contentUrl", contentUrl.c_str());

			hideUrl.sprintf("%s/%s/clubs/updateProfaneClubName/[sessionkey]?upid=%" PRId64, mWalProxyUrlBase.c_str(), currentUserServiceName, clubId);
			AddItemToPayload(jsonWriter, "hideUrl", hideUrl.c_str());
		}
		else
		{
			validRequest = false;
			resp.setErrorMessage("Invalid target clubId");
		}
		break;
	case AI_PLAYER_NAMES:
		if (clubId > 0)
		{
			contentUrl.sprintf("%s/%s/proclubscustom/getAiPlayerNames/[sessionkey]?gain=%" PRId64, mWalProxyUrlBase.c_str(), currentUserServiceName, clubId);
			AddItemToPayload(jsonWriter, "contentUrl", contentUrl.c_str());

			hideUrl.sprintf("%s/%s/proclubscustom/updateProfaneAiPlayerNames/[sessionkey]?upai=%" PRId64, mWalProxyUrlBase.c_str(), currentUserServiceName, clubId);
			AddItemToPayload(jsonWriter, "hideUrl", hideUrl.c_str());
		}
		else
		{
			validRequest = false;
			resp.setErrorMessage("Invalid target clubId");
		}
		break;
    case STADIUM_NAME:
        if (clubId > 0)
        {            
            contentUrl.sprintf("%s/%s/clubs/getStadiumName/[sessionkey]?gain=%" PRId64, mWalProxyUrlBase.c_str(), currentUserServiceName, clubId);
            AddItemToPayload(jsonWriter, "contentUrl", contentUrl.c_str());
            
            hideUrl.sprintf("%s/%s/clubs/updateProfaneStadiumName/[sessionkey]?upai=%" PRId64, mWalProxyUrlBase.c_str(), currentUserServiceName, clubId);
            AddItemToPayload(jsonWriter, "hideUrl", hideUrl.c_str());
        }
        else
        {
            validRequest = false;
            resp.setErrorMessage("Invalid target clubId");
        }
        break;

	case INVALID:
	default:
	{
		ERR_LOG("[ContentReporterSlaveImpl].fileOnlineReportHttpRequest: Invalid content type");
		validRequest = false;
		resp.setErrorMessage("Invalid content type");
	}
		break;
	}

	if (!validRequest)
	{
		// Early return in case we don't have a valid request
		return ERR_SYSTEM;
	}
	eastl::string location = mDefaultGameName + " ONLINE";
	AddItemToPayload(jsonWriter, "location", location.c_str());

	jsonWriter.EndObject();

	size_t datasize = jsonStream.mString.length();

	BlazeRpcError err = ERR_SYSTEM;

	if (datasize > 0 && datasize < mMaxAllowedBodySize)
	{
		err = fileHttpRequest(jsonStream.mString.c_str(), datasize, resp);
	}

	return err;
}

BlazeRpcError ContentReporterSlaveImpl::fileFUTReportHttpRequest(
	const FileFUTReportRequest &req,
	FileReportResponse &resp) const
{
	AccountId nucleusId = gCurrentUserSession->getAccountId();

	// early return in case we don't have a valid user session
	if (nucleusId == INVALID_ACCOUNT_ID)
	{
		ERR_LOG("[ContentReporterSlaveImpl].fileFUTReportHttpRequest: invalid nucleus id: " << nucleusId);
		return Blaze::ERR_SYSTEM;
	}

	EA::Json::StringWriteStream<eastl::string> jsonStream;
	EA::Json::JsonWriter jsonWriter;
	jsonWriter.SetFormatOption(EA::Json::JsonWriter::FormatOption::kFormatOptionLineEnd, 0);
	jsonWriter.SetStream(&jsonStream);

	jsonWriter.BeginObject();

	AddItemToPayload(jsonWriter, "petitionerUserId", nucleusId);
	AddItemToPayload(jsonWriter, "petitionerAccountType", "NUCLEUS");
	AddItemToPayload(jsonWriter, "petitionerPersonaName", gCurrentUserSession->getPersonaName());
	AddItemToPayload(jsonWriter, "petitionerPersonaId", gCurrentUserSession->getBlazeId());
	AddItemToPayload(jsonWriter, "petitionerEmail", "EA_unavailable");
	AddItemToPayload(jsonWriter, "product", getConfig().getDefaultProduct());

	const char8_t* currentUserServiceName = gCurrentUserSession != nullptr ? gCurrentUserSession->getServiceName() : gController->getDefaultServiceName();
	const ServiceNameInfo* serviceNameInfo = gController->getServiceNameInfo(currentUserServiceName);
	if (serviceNameInfo == nullptr)
	{
		ERR_LOG("[ContentReporterSlaveImpl].fileFUTReportHttpRequest: service name info is NULL.");
		return Blaze::ERR_SYSTEM;
	}
	AddItemToPayload(jsonWriter, "platform", getPlatformStringFromClientPlatformType(serviceNameInfo->getPlatform()));

	eastl::string targetNucleusId;
	targetNucleusId.sprintf("%" PRIu64, req.getCommonAbuseReport().getTargetNucleusId());
	AddItemToPayload(jsonWriter, "targetUserId", targetNucleusId.c_str());	
	AddItemToPayload(jsonWriter, "targetAccountType", "NUCLEUS");
	AddItemToPayload(jsonWriter, "targetEmail", "EA_unavailable");
	AddItemToPayload(jsonWriter, "category", "inappropriate-name");
	AddItemToPayload(jsonWriter, "contentType", getContentTypeStringFromClientContentType(req.getCommonAbuseReport().getContentType()));
	eastl::string targetBlazeId;
	targetBlazeId.sprintf("%" PRIu64, req.getCommonAbuseReport().getTargetBlazeId());
	AddItemToPayload(jsonWriter, "targetPersonaId", targetBlazeId.c_str());
	AddItemToPayload(jsonWriter, "subject", req.getCommonAbuseReport().getSubject());
	AddItemToPayload(jsonWriter, "contentText", req.getContentText());

	if (req.getSquadId() != -1)
	{
		eastl::string squadId;
		squadId.sprintf("%" PRId64, req.getSquadId());		

		eastl::string contentUrl;
		contentUrl.sprintf("%s/ut/admin/game/%s/sku/%s/nuc/%s/pers/%s/squadId/%s/squadName", mUtasUrlBase.c_str(), mDefaultFUTGameName.c_str(), req.getSku(), targetNucleusId.c_str(), targetBlazeId.c_str(), squadId.c_str());
		AddItemToPayload(jsonWriter, "contentUrl", contentUrl.c_str());

		eastl::string hideUrl;
		hideUrl.sprintf("%s/ut/admin/abuseReporting/sku/%s/nucUserId/%s/nucPersId/%s?squadId=%s", mUtasUrlBase.c_str(), req.getSku(), targetNucleusId.c_str(), targetBlazeId.c_str(), squadId.c_str());
		AddItemToPayload(jsonWriter, "hideUrl", hideUrl.c_str());
	}
	else
	{
		eastl::string contentUrl;
		contentUrl.sprintf("%s/ut/admin/game/%s/sku/%s/nuc/%s/pers/%s/clubName", mUtasUrlBase.c_str(), mDefaultFUTGameName.c_str(), req.getSku(), targetNucleusId.c_str(), targetBlazeId.c_str());
		AddItemToPayload(jsonWriter, "contentUrl", contentUrl.c_str());

		eastl::string hideUrl;
		hideUrl.sprintf("%s/ut/admin/abuseReporting/sku/%s/nucUserId/%s/nucPersId/%s", mUtasUrlBase.c_str(), req.getSku(), targetNucleusId.c_str(), targetBlazeId.c_str());
		AddItemToPayload(jsonWriter, "hideUrl", hideUrl.c_str());
	}

	eastl::string location = mDefaultGameName + " FUT";
	AddItemToPayload(jsonWriter, "location", location.c_str());

	jsonWriter.EndObject();

	size_t datasize = jsonStream.mString.length();

	BlazeRpcError err = ERR_SYSTEM;

	if (datasize > 0 && datasize < mMaxAllowedBodySize)
	{
		err = fileHttpRequest(jsonStream.mString.c_str(), datasize, resp);
	}

	return err;
}

BlazeRpcError ContentReporterSlaveImpl::fileSSFReportHttpRequest(
	const FileSSFReportRequest &req,
	FileReportResponse &resp) const
{
	AccountId nucleusId = gCurrentUserSession->getAccountId();

	// early return in case we don't have a valid user session
	if (nucleusId == INVALID_ACCOUNT_ID)
	{
		ERR_LOG("[ContentReporterSlaveImpl].fileSSFReportHttpRequest: invalid nucleus id: " << nucleusId);
		return Blaze::ERR_SYSTEM;
	}

	EA::Json::StringWriteStream<eastl::string> jsonStream;
	EA::Json::JsonWriter jsonWriter;
	jsonWriter.SetFormatOption(EA::Json::JsonWriter::FormatOption::kFormatOptionLineEnd, 0);
	jsonWriter.SetStream(&jsonStream);

	jsonWriter.BeginObject();

	AddItemToPayload(jsonWriter, "petitionerUserId", nucleusId);
	AddItemToPayload(jsonWriter, "petitionerAccountType", "NUCLEUS");
	AddItemToPayload(jsonWriter, "petitionerPersonaName", gCurrentUserSession->getPersonaName());
	AddItemToPayload(jsonWriter, "petitionerPersonaId", gCurrentUserSession->getBlazeId());
	AddItemToPayload(jsonWriter, "petitionerEmail", "EA_unavailable");
	AddItemToPayload(jsonWriter, "product", getConfig().getDefaultProduct());

	const char8_t* currentUserServiceName = gCurrentUserSession != nullptr ? gCurrentUserSession->getServiceName() : gController->getDefaultServiceName();
	const ServiceNameInfo* serviceNameInfo = gController->getServiceNameInfo(currentUserServiceName);
	if (serviceNameInfo == nullptr)
	{
		ERR_LOG("[ContentReporterSlaveImpl].fileSSFReportHttpRequest: service name info is NULL.");
		return Blaze::ERR_SYSTEM;
	}
	AddItemToPayload(jsonWriter, "platform", getPlatformStringFromClientPlatformType(serviceNameInfo->getPlatform()));

	const char8_t* contentType = getContentTypeStringFromClientContentType(req.getCommonAbuseReport().getContentType());

	eastl::string targetNucleusId;
	targetNucleusId.sprintf("%" PRIu64, req.getCommonAbuseReport().getTargetNucleusId());
	AddItemToPayload(jsonWriter, "targetUserId", targetNucleusId.c_str());
	AddItemToPayload(jsonWriter, "targetAccountType", "NUCLEUS");
	AddItemToPayload(jsonWriter, "targetEmail", "EA_unavailable");
	AddItemToPayload(jsonWriter, "category", "inappropriate-name");
	AddItemToPayload(jsonWriter, "contentType", contentType);

	eastl::string targetBlazeId;
	targetBlazeId.sprintf("%" PRIu64, req.getCommonAbuseReport().getTargetBlazeId());
	AddItemToPayload(jsonWriter, "targetPersonaId", targetBlazeId.c_str());
	AddItemToPayload(jsonWriter, "subject", req.getCommonAbuseReport().getSubject());
	AddItemToPayload(jsonWriter, "contentText", req.getContentText());
	
	if (EA::StdC::Strcmp(contentType, "NAME") == 0)
	{		
		eastl::string viewUrl;
		viewUrl.sprintf("%s/sf/admin/clubinfo/sku/%s/nucUserId/%s/nucPersId/%s/profanity/names?name=teamname", mSsasUrlBase.c_str(), req.getSku(), targetNucleusId.c_str(), targetBlazeId.c_str());
		AddItemToPayload(jsonWriter, "contentUrl", viewUrl.c_str());

		eastl::string hideUrl;
		hideUrl.sprintf("%s/sf/admin/clubinfo/nucUserId/%s/nucPersId/%s/sku/%s?teamName=%s", mSsasUrlBase.c_str(), targetNucleusId.c_str(), targetBlazeId.c_str(), req.getSku(), mDefaultSSFTeamName.c_str());
		AddItemToPayload(jsonWriter, "hideUrl", hideUrl.c_str());
	}
	else if (EA::StdC::Strcmp(contentType, "AVATAR") == 0)
	{
		eastl::string viewUrl;
		viewUrl.sprintf("%s/sf/admin/avatar/sku/%s/nucUserId/%s/nucPersId/%s/slotId/0/profanity/names?name=knownas", mSsasUrlBase.c_str(), req.getSku(), targetNucleusId.c_str(), targetBlazeId.c_str());
		AddItemToPayload(jsonWriter, "contentUrl", viewUrl.c_str());

		eastl::string hideUrl;
		hideUrl.sprintf("%s/sf/admin/avatar/sku/%s/nucUserId/%s/nucPersId/%s/slotId/0/attributes?knownas=%s", mSsasUrlBase.c_str(), req.getSku(), targetNucleusId.c_str(), targetBlazeId.c_str(), mDefaultSSFAvatarName.c_str());
		AddItemToPayload(jsonWriter, "hideUrl", hideUrl.c_str());
	}

	eastl::string location = mDefaultGameName + " SSF";
	AddItemToPayload(jsonWriter, "location", location.c_str());
			
	jsonWriter.EndObject();

	size_t datasize = jsonStream.mString.length();

	BlazeRpcError err = ERR_SYSTEM;

	if (datasize > 0 && datasize < mMaxAllowedBodySize)
	{
		err = fileHttpRequest(jsonStream.mString.c_str(), datasize, resp);
	}

	return err;
}

BlazeRpcError ContentReporterSlaveImpl::fileHttpRequest(const char8_t* bodyPayLoad, size_t datasize, FileReportResponse &resp) const
{
	OutboundHttpConnectionManagerPtr httpConnManager = gOutboundHttpService->getConnection(BIFROST_HTTP_SERVICE_NAME);
	if (httpConnManager == nullptr)
	{
		ERR_LOG("[ContentReporterSlaveImpl].fileHttpRequest: required http service '" << BIFROST_HTTP_SERVICE_NAME << "' is not registered.");
		return Blaze::ERR_SYSTEM;
	}

	const char8_t *httpHeaders[2];
	eastl::string strHttpHeader;
	strHttpHeader.sprintf("h-RequestorId: %s", mRequestorId.c_str());
	httpHeaders[0] = strHttpHeader.c_str();
	eastl::string strHttpHeader2;
	strHttpHeader2.sprintf("x-api-key: %s", mApiKey.c_str());
	httpHeaders[1] = strHttpHeader2.c_str();

	BlazeRpcError err = ERR_SYSTEM;

	TRACE_LOG("[ContentReporterSlaveImp].fileHttpRequest: bodyPayLoad=" << bodyPayLoad);

	if(bodyPayLoad)
	{
		// Send the HTTP Request.
		HttpResponse httpResponse;

		err = httpConnManager->sendRequest(
			HttpProtocolUtil::HTTP_POST,
			"/petition",
			httpHeaders,
			EAArrayCount(httpHeaders),
			&httpResponse,
			"Content-Type: application/json",
			bodyPayLoad,
			datasize
		);

		resp.setStatus(httpResponse.GetStatus());

		if (err == ERR_OK)
		{
			resp.setPetitionGuid(httpResponse.GetGUID());
		}
		else
		{
			if (httpResponse.HasErrorMessage())
			{
				WARN_LOG("[ContentReporterSlaveImpl].fileHttpRequest: File petition failed with error message: " << httpResponse.GetErrorMessage());

				resp.setErrorMessage(httpResponse.GetErrorMessage());
			}
		}
	}

	return err;
}

} // ContentReporter
} // Blaze
