

/*************************************************************************************************/
/*!
    \file   contentreporterslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CONTENTREPORTER_SLAVEIMPL_H
#define BLAZE_CONTENTREPORTER_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "contentreporter/rpc/contentreporterslave_stub.h"
#include "contentreporter/tdf/contentreportertypes.h"

#include "EAJson/JsonWriter.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
namespace ContentReporter
{

class ContentReporterSlaveImpl : public ContentReporterSlaveStub
{
public:
    ContentReporterSlaveImpl() {}
    ~ContentReporterSlaveImpl() override {}

	BlazeRpcError DEFINE_ASYNC_RET(fileReportHttpRequest(const FileReportRequest &req, FileReportResponse &resp) const);
	BlazeRpcError DEFINE_ASYNC_RET(fileOnlineReportHttpRequest(const FileOnlineReportRequest &req, FileReportResponse &resp) const);
	BlazeRpcError DEFINE_ASYNC_RET(fileFUTReportHttpRequest(const FileFUTReportRequest &req, FileReportResponse &resp) const);
	BlazeRpcError DEFINE_ASYNC_RET(fileSSFReportHttpRequest(const FileSSFReportRequest &req, FileReportResponse &resp) const);

private:
    // configuration parameters
    eastl::string mRequestorId;
	uint32_t mMaxAllowedBodySize;
	eastl::string mWalProxyUrlBase;
	eastl::string mUtasUrlBase;
	eastl::string mSsasUrlBase;
	eastl::string mDefaultSSFTeamName;
	eastl::string mDefaultSSFAvatarName;
	eastl::string mDefaultFUTGameName;
	eastl::string mDefaultGameName;

	const eastl::string mApiKey = "nJD9J5DynGaC2joMku06e6zTTvZwbMhr2dsbyUKp";

	bool onConfigure() override;
	bool onReconfigure() override;
	bool onValidateConfig(ContentReporterConfig& config, const ContentReporterConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;
	const char8_t* getPlatformStringFromClientPlatformType(const ClientPlatformType platformType) const;
	const char8_t* getContentTypeStringFromClientContentType(const ReportContentType contentType) const;

	void AddItemToPayload(EA::Json::JsonWriter &jsonWriter, const char8_t* name, const char8_t* value) const;
	void AddItemToPayload(EA::Json::JsonWriter &jsonWriter, const char8_t* name, const int64_t value) const;
	BlazeRpcError fileHttpRequest(const char8_t* bodyPayLoad, size_t datasize, FileReportResponse &resp) const;

	static const uint32_t NUM_REPORT_PARAMS = 27;
};

} // ContentReporter
} // Blaze

#endif // BLAZE_CONTENTREPORTER_SLAVEIMPL_H

