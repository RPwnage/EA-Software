/*************************************************************************************************/
/*!
    \file externaldatamanager.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ExternalDataManager

    Utility functions for retrieving external data for user's when matchmaking or joining a game.

    \notes

*/
/*************************************************************************************************/


#ifndef BLAZE_EXT_DATA_MANAGER
#define BLAZE_EXT_DATA_MANAGER

#include "EAJson/JsonReader.h"
#include "framework/util/shared/rawbufferistream.h"
#include "framework/protocol/restprotocolutil.h"
#include "framework/protocol/shared/restrequestbuilder.h"
#include "framework/oauth/accesstokenutil.h"
#include "EAJson/JsonReader.h"
#include "EAJson/JsonDomReader.h"
#include "gamemanager/tdf/scenarios.h"
#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/tdf/gamemanagerconfig_server.h"
#include "gamemanager/rpc/gamemanagerslave.h"
#include "framework/grpc/outboundgrpcjobhandlers.h"

namespace Blaze
{
namespace GameManager
{
    class GameManagerSlaveImpl;
    class IStreamToJsonReadStream : public EA::Json::IReadStream
    {
    public:
        IStreamToJsonReadStream(EA::IO::IStream& iStream)
            : mIStream(iStream)
        {
        }

        ~IStreamToJsonReadStream() override {}
        IStreamToJsonReadStream& operator=(const IStreamToJsonReadStream &tmp);

        size_t Read(void* pData, size_t nSize) override
        {
            return (size_t)mIStream.Read(pData, nSize);
        }

        EA::IO::IStream& mIStream;
    };


    // Special values which are fetched from the BlazeServer when making request to external source
    static const char8_t BLAZE_ID_KEY[] = "$BLAZEID";
    static const char8_t ACCOUNT_ID_KEY[] = "$ACCOUNTID";
    static const char8_t S2S_ACCESS_TOKEN_KEY[] = "$S2S_ACCESS_TOKEN";
    static const char8_t S2S_ACCESS_TOKEN_BEARER_PREFIX_KEY[] = "$S2S_ACCESS_TOKEN_BEARER_PREFIX";
    static const char8_t CLIENT_PLATFORM_KEY[] = "$CLIENT_PLATFORM";
    static const char8_t SOCIAL_GROUP_ID_ATTR_KEY[] = "$SOCIAL_GROUP_ID_ATTR";
    static const char8_t SOCIAL_GROUP_ATTRIBUTE_KEY[] = "SOCIAL_GROUP";

    // Used in mapping response from external data source to startMatchmakingScenarioRequest
    static const char8_t TEMPLATE_ATTRIBUTES_KEY[] = "TEMPLATE_ATTR.";
    static const char8_t UED_KEY[] = "UED.";

    // timeouts
    static const TimeValue PER_EXT_REQUEST_TIMEOUT = 2000000;
    static const TimeValue TOTAL_EXT_REQUEST_TIMEOUT = 10000000;

    class ExternalDataManager
    {
    public:
        ExternalDataManager(GameManagerSlaveImpl* componentImpl)
            : mComponent(componentImpl)
        {
        }

        ~ExternalDataManager() {}

        bool validateConfig(const GameManagerServerConfig& configTdf, ConfigureValidationErrors& validationErrors) const;
        void configure(const GameManagerServerConfig& configTdf);
        void reconfigure(const GameManagerServerConfig& configTdf);


        bool fetchAndPopulateExternalData(StartMatchmakingScenarioRequest& request);
        bool fetchAndPopulateExternalData(CreateGameTemplateRequest& request);
        bool fetchAndPopulateExternalData(ExternalDataSourceApiNameList& apiList, JoinGameRequest& joinGameRequest);
        bool fetchAndPopulateExternalData(GetGameListScenarioRequest& request);
        bool fetchAndPopulateExternalData(ExternalDataSourceApiNameList& apiList, TemplateAttributes& templateAttributes, PerPlayerJoinDataList& playerDataList, bool uedOnly = false);

    private:

        bool makeRpcCall(DataSourceName& externalApiName, RpcDataSourceDefinition& rpcCallData, TemplateAttributes& attributes, PerPlayerJoinDataPtr perPlayerJoinData, TimeValue& timeTaken);

        static void validateApiConfigMap(const GameManagerServerConfig& configTdf, ConfigureValidationErrors& validationErrors);
        static void validateGrpcConfigMap(const GameManagerServerConfig& configTdf, ConfigureValidationErrors& validationErrors);
        static void validateComponentConfigMap(const GameManagerServerConfig& configTdf, ConfigureValidationErrors& validationErrors);
        static void validateParamConfig(const char8_t* paramName, ExternalDataSourceParameterPtr externalDataSourceParameterPtr, ConfigureValidationErrors& validationErrors);
        static void validateGrpcParamConfig(const char8_t* paramName, ExternalDataSourceParameterPtr param, ConfigureValidationErrors& validationErrors);
        static void validateGrpcResponseParamConfig(const GameManagerServerConfig& configTdf, const ExternalApiName& apiName, const ParamKey& key, ExternalDataSourceParameterPtr param, eastl::set<TemplateAttributeName>& templateAttributeNameSet, ConfigureValidationErrors& validationErrors);
        static void validateDefaultValue(const char8_t* paramName, const char8_t* value, ConfigureValidationErrors& validationErrors);
        static void validateScalingFactor(const char8_t* paramName, const int32_t scalingFactor, ConfigureValidationErrors& validationErrors);

        static bool overridesUED(const RequestOverrideKeyMap& output);
        static bool overridesUED(const RpcResponseAttributes& attribs);

        bool sendExternalAPIRequest(ExternalApiName& externalApiName, ExternalDataSourceApiDefinition& externalDataSourceAPIDefinition, TemplateAttributes& attributes, PerPlayerJoinDataPtr perPlayerJoinData, TimeValue& timeTaken);
        bool sendExternalGrpcRequest(const ExternalApiName& externalApiName, ExternalDataSourceGrpcDefinition& externalDataSourceGrpcDefinition, TemplateAttributes& attributes, PerPlayerJoinDataPtr perPlayerJoinData) const;
        bool prepareBaseURL(UrlPathParametersMap& urlPathParametersMap, TemplateAttributes& attributes, eastl::string& url, PerPlayerJoinDataPtr perPlayerJoinData) const;
        bool prepareUriQueryParameters(UrlQueryParametersMap& urlQueryParametersMap, TemplateAttributes& attributes, RestRequestBuilder::HttpParamVector& httpParams, PerPlayerJoinDataPtr perPlayerJoinData) const;
        bool prepareHttpHeaders(HeadersMap& headersMap, TemplateAttributes& attributes, RestRequestBuilder::HeaderVector& headerVector, PerPlayerJoinDataPtr perPlayerJoinData) const;
        void decodeExternalData(const RequestOverrideKeyMap& requestOverrideKeyMap, RawBuffer& rawBuffer, TemplateAttributes& attributes, const PerPlayerJoinDataPtr perPlayerJoinData) const;
        bool decodeExternalDataList(const char8_t* attr, TemplateAttributes& attributes, const char8_t* requestOverrideKey, const char8_t* listNodePath, const char8_t* elementNodePath, EA::Json::JsonDomDocument& domDocument, int64_t scalingFactor, bool mergeLists) const;
        bool decodeExternalDataMap(const char8_t* attr, TemplateAttributes& attributes, const char8_t* mapNodePath, EA::Json::JsonDomDocument& domDocument, int64_t scalingFactor, bool mergeMaps) const;

        static void validateAttributeNames(const GameManagerServerConfig& configTdf, const ExternalApiName& apiName, const ParamKey& key, const char8_t* paramName, const char8_t* paramDefaultValue,
            eastl::set<TemplateAttributeName>& templateAttributeNameSet, ConfigureValidationErrors& validationErrors);
        static bool isAttributeNameValid(TemplateAttributeName templateAttribute, const GameManagerServerConfig& configTdf, eastl::set<TemplateAttributeName>& templateAttributeNameSet);
        static void addTemplateAttributesToTemplateAttributeSet(const TemplateAttributeMapping& templateAttributeMapping, eastl::set<TemplateAttributeName>& templateAttributeNameSet);

        static bool filloutGrpcRequest(const google::protobuf::Descriptor* d, const EA::TDF::TdfGenericReferenceConst& tdf, const char8_t* tdfMemberName, google::protobuf::Message* m,
            const google::protobuf::FieldDescriptor* fdOverride = nullptr);
        static bool filloutGrpcClientContext(const HeadersMap& headersMap, const TemplateAttributes& attributes, PerPlayerJoinDataPtr perPlayerJoinData, grpc::ClientContext& context);
        static Grpc::OutboundRpcBasePtr createOutboundRpc(const ExternalDataSourceGrpcDefinition& externalDataSourceGrpcDefinition);

        static BlazeRpcError getNucleusAccessToken(eastl::string& accessToken, OAuth::TokenType prefixType);
        GameManagerSlaveImpl* mComponent;
    };

} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_EXT_DATA_MANAGER