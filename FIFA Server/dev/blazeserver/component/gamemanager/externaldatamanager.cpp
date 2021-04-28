/*************************************************************************************************/
/*!
    \file externaldatamanager.cpp


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


#include "framework/blaze.h"
#include "gamemanager/externaldatamanager.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "framework/oauth/accesstokenutil.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/grpc/outboundgrpcmanager.h"
#include "framework/util/shared/blazestring.h"
#include "gamemanager/templateattributes.h"

#include <protogen/outbound/eadp/stats/stats_core.grpc.pb.h>

#include <google/protobuf/message.h>
#include <google/protobuf/reflection.h>
#include <google/protobuf/util/json_util.h>

#include <EATDF/tdfmap.h>
#include <math.h>

namespace Blaze
{
namespace GameManager
{

    //scalingFactor is guaranteed to be in range of -9 to 9.
    static const int64_t scale[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

    static int64_t getScaledInt64(EA::Json::JsonDomNode* jsonDomNode, const int64_t scalingFactor)
    {
        switch (jsonDomNode->GetNodeType())
        {
        case EA::Json::EventType::kETInteger:
        {
            if (scalingFactor >= 0)
                return (jsonDomNode->AsJsonDomInteger()->mValue * scale[scalingFactor]);
            else
                return (jsonDomNode->AsJsonDomInteger()->mValue / scale[-scalingFactor]);
        }
        break;
        case EA::Json::EventType::kETDouble:
        {
            if (scalingFactor >= 0)
                return (static_cast<int64_t>(jsonDomNode->AsJsonDomDouble()->mValue * scale[scalingFactor]));
            else
                return (static_cast<int64_t>(jsonDomNode->AsJsonDomDouble()->mValue / scale[-scalingFactor]));
        }
        break;
        default: 
        break;
        }

        return 0;
    }

    //returns true if conversion is successful
    static bool convertNumberStringToInt(int64_t& value, const char8_t* ptr, const int32_t scalingFactor = 0)
    {
        //try to parse string as integer first because int64_t has more precision than double
        int64_t intValue = 0;
        char8_t* text = blaze_str2int(ptr, &intValue);
        if (text == nullptr || text[0] == '\0')
        {
            if (scalingFactor >= 0)
                value = intValue * scale[scalingFactor];
            else
                value = intValue / scale[-scalingFactor];
            return true;
        }

        //try to parse string as double
        double doubleValue = 0.0;
        text = blaze_str2dbl(ptr, doubleValue);
        if (text == nullptr || text[0] == '\0')
        {
            if (scalingFactor >= 0)
                value = static_cast<int64_t>(doubleValue * scale[scalingFactor]);
            else
                value = static_cast<int64_t>(doubleValue / scale[-scalingFactor]);
            return true;
        }

        return false;
    }

    bool ExternalDataManager::validateConfig(const GameManagerServerConfig& configTdf, ConfigureValidationErrors& validationErrors) const
    {
        validateApiConfigMap(configTdf, validationErrors);
        validateGrpcConfigMap(configTdf, validationErrors);
        validateComponentConfigMap(configTdf, validationErrors);

        return validationErrors.getErrorMessages().empty();
    }

    void ExternalDataManager::validateApiConfigMap(const GameManagerServerConfig& configTdf, ConfigureValidationErrors& validationErrors)
    {
        StringBuilder strBuilder;
        for (auto& externalDataSourceApiDefinitionMapItr : configTdf.getExternalDataSourceConfig().getApiConfigMap())
        {
            ExternalApiName apiName = externalDataSourceApiDefinitionMapItr.first;
            ExternalDataSourceApiDefinitionPtr externalDataSourceApiDefinitionPtr = externalDataSourceApiDefinitionMapItr.second;

            if (gOutboundHttpService->getConnection(externalDataSourceApiDefinitionPtr->getExternalDataSource()) == nullptr)
            {
                strBuilder << "[ExternalDataManager].validateApiConfig: failed to configure api(" << apiName.c_str() << ") because an invalid http service(" << externalDataSourceApiDefinitionPtr->getExternalDataSource() << ") was specified.";
                validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
                continue;
            }

            if (externalDataSourceApiDefinitionPtr->getUrl()[0] == '\0')
            {
                strBuilder << "[ExternalDataManager].validateApiConfig: failed to configure api(" << apiName.c_str() << ") because url was not specified.";
                validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
                continue;
            }

            for (auto& urlPathParametersMapItr : externalDataSourceApiDefinitionPtr->getUrlPathParameters())
            {
                validateParamConfig(urlPathParametersMapItr.first, urlPathParametersMapItr.second, validationErrors);
            }

            for (auto& urlQueryParametersItr : externalDataSourceApiDefinitionPtr->getUrlQueryParameters())
            {
                validateParamConfig(urlQueryParametersItr.first, urlQueryParametersItr.second, validationErrors);
            }

            for (auto& headersMapItr : externalDataSourceApiDefinitionPtr->getHeaders())
            {
                validateParamConfig(headersMapItr.first, headersMapItr.second, validationErrors);
            }

            if (externalDataSourceApiDefinitionPtr->getOutput().empty())
            {
                strBuilder << "[ExternalDataManager].validateApiConfig: failed to configure configure api(" << apiName.c_str() << ") because output map for API is empty.";
                validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
                continue;
            }

            eastl::set<TemplateAttributeName> templateAttributeNameSet;
            for (auto& requestOverrideKeysMapItr : externalDataSourceApiDefinitionPtr->getOutput())
            {
                validateAttributeNames(configTdf, apiName, requestOverrideKeysMapItr.first, requestOverrideKeysMapItr.second->getName(), requestOverrideKeysMapItr.second->getDefaultValue(), templateAttributeNameSet, validationErrors);
                validateScalingFactor(requestOverrideKeysMapItr.first.c_str(), requestOverrideKeysMapItr.second->getScalingFactor(), validationErrors);
            }
        }
    }

    void ExternalDataManager::validateGrpcConfigMap(const GameManagerServerConfig& configTdf, ConfigureValidationErrors& validationErrors)
    {
        StringBuilder strBuilder;
        for (auto& itr : configTdf.getExternalDataSourceConfig().getGrpcConfigMap())
        {
            ExternalApiName apiName = itr.first;
            ExternalDataSourceGrpcDefinitionPtr dataSource = itr.second;
            if (!gController->getOutboundGrpcManager()->hasService(dataSource->getExternalDataSource()))
            {
                strBuilder << "[ExternalDataManager].validateGrpcConfigMap: failed to configure api(" << apiName.c_str() << ") because an invalid external data source(" << dataSource->getExternalDataSource() << ") was specified.";
                validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
            }

            Grpc::OutboundRpcBasePtr rpc = createOutboundRpc(*dataSource.get());
            if (rpc == nullptr)
            {
                strBuilder << "[ExternalDataManager].validateGrpcConfigMap: failed to configure api(" << apiName.c_str() << ") because a valid outbound gRPC object could not be created for service("
                    << dataSource->getService() << "), command(" << dataSource->getCommand() << "), commandType(" << ExternalDataSourceGrpcDefinition::GrpcCommandTypeToString(dataSource->getCommandType())
                    << ").";
                validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
            }

            if ((dataSource->getRequestType()[0] == '\0') || 
                (google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(dataSource->getRequestType()) == nullptr))
            {
                strBuilder << "[ExternalDataManager].validateGrpcConfigMap: failed to configure api(" << apiName.c_str() << ") because a valid outbound gRPC request type was not specified.";
                validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
            }

            // gRPC doesn't add Service/Method information to the DescriptorPool.
            // We will need to write our own custom proto file parser in order to get this information. 
            // Without this functionality it is possible to send the wrong data type to the function, without any warning.
            /*
            const auto* methodDesc = google::protobuf::DescriptorPool::generated_pool()->FindMethodByName(dataSource->getRequestName());
            if (methodDesc == nullptr || methodDesc->input_type()  == nullptr || methodDesc->input_type()->full_name() != dataSource->getRequestType())
            {
                strBuilder << "[ExternalDataManager].validateGrpcConfigMap: failed to configure api(" << apiName.c_str() << ") because the outbound gRPC request type ("<< dataSource->getRequestType() << ") did not match the method's request type.";
                validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
            }
            */

            for (auto& attr : dataSource->getRequestAttributes())
            {
                validateGrpcParamConfig(attr.first, attr.second, validationErrors);
            }

            if (dataSource->getOutput().empty())
            {
                strBuilder << "[ExternalDataManager].validateGrpcConfigMap: failed to configure configure api(" << apiName << ") because response map for RPC is empty.";
                validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
                continue;
            }

            eastl::set<TemplateAttributeName> templateAttributeNameSet;
            for (auto& attr : dataSource->getOutput())
            {
                validateGrpcResponseParamConfig(configTdf, apiName, attr.first, attr.second, templateAttributeNameSet, validationErrors);
            }
        }
    }

    void ExternalDataManager::validateComponentConfigMap(const GameManagerServerConfig& configTdf, ConfigureValidationErrors& validationErrors)
    {
        StringBuilder strBuilder;
        for (auto& rpcData : configTdf.getExternalDataSourceConfig().getComponentConfigMap())
        {
            DataSourceName dataSourceName = rpcData.first;
            RpcDataSourceDefinition& rpcCallData = *rpcData.second;
            const ComponentData* compData = ComponentData::getComponentDataByName(rpcCallData.getComponent());
            if (compData == nullptr)
            {
                strBuilder << "[ExternalDataManager].validateComponentConfigMap: Unable to find component (" << rpcCallData.getComponent() << ") for rpcCall " << dataSourceName << ".";
                validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
                continue;
            }

            const CommandInfo* commandInfo = compData->getCommandInfo(rpcCallData.getCommand());
            if (commandInfo == nullptr)
            {
                strBuilder << "[ExternalDataManager].validateComponentConfigMap: Unable to find command " << rpcCallData.getCommand() << " for component (" << rpcCallData.getComponent() << ") for rpcCall " << dataSourceName << ".";
                validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
                continue;
            }


            // Request Validation:
            {
                AttributeToTypeDescMap scenarioAttrToTypeDesc;
                EA::TDF::TdfPtr request = commandInfo->createRequest("EDM Rpc Request");
                EA::TDF::TdfGenericReference requestRef(request);
                StringBuilder errorPrefix;
                errorPrefix << "[ExternalDataManager].validateComponentConfigMap: Attribute for rpcCall " << dataSourceName << ": ";
                validateTemplateAttributeMapping(rpcCallData.getRequestAttributes(), scenarioAttrToTypeDesc, requestRef, validationErrors, errorPrefix.get());
            }

            // Response Validation:
            {
                EA::TDF::TdfPtr response = commandInfo->createResponse("EDM Rpc Response");
                EA::TDF::TdfGenericReference responseRef(response);

                // Not much to care about here.  Just make sure that the parsed 
                EA::TDF::TdfFactory& tdfFactory = EA::TDF::TdfFactory::get();

                // Verify all create game attributes:
                for (auto& responseTemplates : rpcCallData.getResponseAttributes().getTemplateAttributes())
                {
                    // Check that the value can be parsed by the Tdf system: 
                    const char* tdfMemberName = responseTemplates.second.c_str();
                    EA::TDF::TdfGenericReference criteriaAttr;
                    if (!tdfFactory.getTdfMemberReference(responseRef, tdfMemberName, criteriaAttr))
                    {
                        strBuilder << "[ExternalDataManager].validateComponentConfigMap: Attribute for rpcCall " << dataSourceName << ": " <<
                            "Unable to get TdfMemberReference for attribute. Member location: '" << tdfMemberName <<
                            "'.  Check if this value has changed its name or location. ";
                        validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
                        continue;
                    }
                }
            }
        }
    }

    void ExternalDataManager::validateParamConfig(const char8_t* paramName, ExternalDataSourceParameterPtr externalDataSourceParameterPtr, ConfigureValidationErrors& validationErrors)
    {
        StringBuilder strBuilder;
        if (externalDataSourceParameterPtr == nullptr)
        {
            strBuilder << "[ExternalDataManager].validateParamConfig: failed to configure parameter(" << paramName << ") because externalDataSourceParameterPtr was nullptr";
            validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
        }

        validateDefaultValue(paramName, externalDataSourceParameterPtr->getDefaultValue(), validationErrors);
        validateScalingFactor(paramName, externalDataSourceParameterPtr->getScalingFactor(), validationErrors);
    }

    void ExternalDataManager::validateGrpcParamConfig(const char8_t* paramName, ExternalDataSourceParameterPtr param, ConfigureValidationErrors& validationErrors)
    {
        StringBuilder strBuilder;
        if (param == nullptr)
        {
            strBuilder << "[ExternalDataManager].validateGrpcParamConfig: failed to configure parameter(" << paramName << ") because param was nullptr";
            validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
        }

        if (param->getName()[0] == '\0' && param->getDefaultValue()[0] == '\0')
        {
            strBuilder << "[ExternalDataManager].validateGrpcParamConfig: failed to configure parameter(" << paramName << ") because both the default value and name are empty.";
            validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
        }

        validateDefaultValue(paramName, param->getDefaultValue(), validationErrors);
        validateScalingFactor(paramName, param->getScalingFactor(), validationErrors);
    }

    void ExternalDataManager::validateGrpcResponseParamConfig(const GameManagerServerConfig& configTdf, const ExternalApiName& apiName, const ParamKey& key, ExternalDataSourceParameterPtr param, eastl::set<TemplateAttributeName>& templateAttributeNameSet, ConfigureValidationErrors& validationErrors)
    {
        StringBuilder strBuilder;
        if (param == nullptr)
        {
            strBuilder << "[ExternalDataManager].validateGrpcResponseParamConfig: failed to configure parameter(" << key << ") because param was nullptr";
            validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
        }

        if (param->getName()[0] == '\0' && param->getDefaultValue()[0] == '\0')
        {
            strBuilder << "[ExternalDataManager].validateGrpcResponseParamConfig: failed to configure parameter(" << key << ") because both the default value and name are empty.";
            validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
        }

        validateAttributeNames(configTdf, apiName, key, param->getName(), param->getDefaultValue(), templateAttributeNameSet, validationErrors);
        validateScalingFactor(param->getName(), param->getScalingFactor(), validationErrors);
    }

    void ExternalDataManager::validateDefaultValue(const char8_t* paramName, const char8_t* value, ConfigureValidationErrors& validationErrors)
    {
        if (value[0] != '\0')
        {
            // Check to see if the default values are 'key' values.
            if (value[0] == '$')
            {
                // syntax for 'key' values start with '$' - now check if they are valid key values.
                // ensure that the parameter types are correct.
                if (blaze_stricmp(value, BLAZE_ID_KEY) != 0
                    && blaze_stricmp(value, S2S_ACCESS_TOKEN_KEY) != 0
                    && blaze_stricmp(value, S2S_ACCESS_TOKEN_BEARER_PREFIX_KEY) != 0
                    && blaze_stricmp(value, ACCOUNT_ID_KEY) != 0
                    && blaze_stricmp(value, SOCIAL_GROUP_ID_ATTR_KEY) != 0)
                {
                    // invalid key value specified
                    StringBuilder strBuilder;
                    strBuilder << "[ExternalDataManager].validateDefaultValue: failed to configure parameter(" << paramName << ") because default value(" << value << ") is a non supported key value";
                    validationErrors.getErrorMessages().push_back(strBuilder.get());
                }
            }
        }
    }

    void ExternalDataManager::validateScalingFactor(const char8_t* paramName, const int32_t scalingFactor, ConfigureValidationErrors& validationErrors)
    {
        int32_t size = static_cast<int32_t>(sizeof(scale) / sizeof(scale[0]));
        if (scalingFactor >= size || scalingFactor <= -size)
        {
            StringBuilder strBuilder;
            strBuilder << "[ExternalDataManager].validateScalingFactor: failed to configure parameter(" << paramName << ") because scaling factor(" << scalingFactor << ") is out of valid range.";
            validationErrors.getErrorMessages().push_back(strBuilder.get());
        }
    }

    bool ExternalDataManager::overridesUED(const RequestOverrideKeyMap& output)
    {
        for (auto& itr : output)
        {
            ParamKey overrideKey = itr.first;
            if (blaze_strnistr(overrideKey, UED_KEY, overrideKey.length()) != nullptr)
                return true;
        }
        return false;
    }

    bool ExternalDataManager::overridesUED(const RpcResponseAttributes& attribs)
    {
        return !attribs.getUedAttributes().empty();
    }

    void ExternalDataManager::configure(const GameManagerServerConfig& configTdf)
    {
    }

    void ExternalDataManager::reconfigure(const GameManagerServerConfig& configTdf)
    {   
    }

    bool ExternalDataManager::fetchAndPopulateExternalData(StartMatchmakingScenarioRequest& request)
    {
        ScenarioMap::const_iterator sConfig = mComponent->getConfig().getScenariosConfig().getScenarios().find(request.getScenarioName());
        if (sConfig != mComponent->getConfig().getScenariosConfig().getScenarios().end())
        {
            return fetchAndPopulateExternalData(sConfig->second->getExternalDataSourceApiList(), request.getScenarioAttributes(), request.getPlayerJoinData().getPlayerDataList());
        }
        return true;
    }

    bool ExternalDataManager::fetchAndPopulateExternalData(CreateGameTemplateRequest& request)
    {
        CreateGameTemplateMap::const_iterator sConfig = mComponent->getConfig().getCreateGameTemplatesConfig().getCreateGameTemplates().find(request.getTemplateName());
        if (sConfig != mComponent->getConfig().getCreateGameTemplatesConfig().getCreateGameTemplates().end())
        {
            return fetchAndPopulateExternalData(sConfig->second->getExternalDataSourceApiList(), request.getTemplateAttributes(), request.getPlayerJoinData().getPlayerDataList());
        }
        return true;
    }
    
    bool ExternalDataManager::fetchAndPopulateExternalData(ExternalDataSourceApiNameList& apiList, JoinGameRequest& request)
    {
        TemplateAttributes templateAttributes;
        return fetchAndPopulateExternalData(apiList, templateAttributes, request.getPlayerJoinData().getPlayerDataList(), true /*uedOnly*/);
    }

    bool ExternalDataManager::fetchAndPopulateExternalData(GetGameListScenarioRequest& request)
    {        
        GameBrowserScenarioMap::const_iterator sConfig = mComponent->getConfig().getGameBrowserScenariosConfig().getScenarios().find(request.getGameBrowserScenarioName());
        if (sConfig != mComponent->getConfig().getGameBrowserScenariosConfig().getScenarios().end())
        {
            return fetchAndPopulateExternalData(sConfig->second->getExternalDataSourceApiList(), request.getGameBrowserScenarioAttributes(), request.getPlayerJoinData().getPlayerDataList());
        }
        return true;
    }

    bool ExternalDataManager::fetchAndPopulateExternalData(ExternalDataSourceApiNameList& apiList, TemplateAttributes& templateAttributes, PerPlayerJoinDataList& playerDataList, bool uedOnly /* = false */)
    {
        bool result = true;
        TimeValue timeTaken;
        for (auto& apiName : apiList)
        {
            ExternalDataSourceApiDefinitionMap::const_iterator apiConfigItr = mComponent->getConfig().getExternalDataSourceConfig().getApiConfigMap().find(apiName);
            if (apiConfigItr == mComponent->getConfig().getExternalDataSourceConfig().getApiConfigMap().end())
            {
                TRACE_LOG("[ExternalDataManager].fetchAndPopulateExternalData(): Unable to find api("<< apiName << ") in ExternalDataSourceConfig's Api Configuration Map.");
            }
            else if (!uedOnly || overridesUED(apiConfigItr->second->getOutput()))
            {
                for (auto& perPlayerJoinDataListItr : playerDataList)
                {
                    result &= sendExternalAPIRequest(apiName, *(apiConfigItr->second), templateAttributes, perPlayerJoinDataListItr, timeTaken);
                }
            }

            ExternalDataSourceGrpcDefinitionMap::const_iterator grpcConfigItr = mComponent->getConfig().getExternalDataSourceConfig().getGrpcConfigMap().find(apiName);
            if (grpcConfigItr == mComponent->getConfig().getExternalDataSourceConfig().getGrpcConfigMap().end())
            {
                TRACE_LOG("[ExternalDataManager].fetchAndPopulateExternalData(): Unable to find api(" << apiName << ") in ExternalDataSourceConfig's gRPC Configuration Map.");
            }
            else if (!uedOnly || overridesUED(grpcConfigItr->second->getOutput()))
            {
                for (auto& perPlayerJoinDataListItr : playerDataList)
                {
                    result &= sendExternalGrpcRequest(apiName, *(grpcConfigItr->second), templateAttributes, perPlayerJoinDataListItr);
                }
            }

            RpcDataSourceDefinitionMap::const_iterator internalConfigItr = mComponent->getConfig().getExternalDataSourceConfig().getComponentConfigMap().find(apiName);
            if (internalConfigItr == mComponent->getConfig().getExternalDataSourceConfig().getComponentConfigMap().end())
            {
                TRACE_LOG("[ExternalDataManager].fetchAndPopulateExternalData(): Unable to find api(" << apiName << ") in ExternalDataSourceConfigs Rpc Configuration Map.");
            }
            else if (!uedOnly || overridesUED(internalConfigItr->second->getResponseAttributes()))
            {
                for (auto& perPlayerJoinDataListItr : playerDataList)
                {
                    result &= makeRpcCall(apiName, *(internalConfigItr->second), templateAttributes, perPlayerJoinDataListItr, timeTaken);
                }

                templateAttributes.erase(BLAZE_ID_KEY);
                templateAttributes.erase(ACCOUNT_ID_KEY);
                templateAttributes.erase(S2S_ACCESS_TOKEN_KEY);
                templateAttributes.erase(S2S_ACCESS_TOKEN_BEARER_PREFIX_KEY);
                templateAttributes.erase(CLIENT_PLATFORM_KEY);
            }
        }
        return result;
    }

    bool ExternalDataManager::makeRpcCall(DataSourceName& dataSourceName, RpcDataSourceDefinition& rpcCallData, TemplateAttributes& attributes, PerPlayerJoinDataPtr perPlayerJoinData, TimeValue& timeTaken)
    {
        const ComponentData* compData = ComponentData::getComponentDataByName(rpcCallData.getComponent());
        if (compData == nullptr)
        {
            ERR_LOG("[ExternalDataManager].makeRpcCall(): Unable to find component (" << rpcCallData.getComponent() << ") for rpcCall " << dataSourceName << "." );
            return false;
        }
        
        const CommandInfo* commandInfo = compData->getCommandInfo(rpcCallData.getCommand());
        if (commandInfo == nullptr)
        {
            ERR_LOG("[ExternalDataManager].makeRpcCall(): Unable to find command "<< rpcCallData.getCommand() << " for component (" << rpcCallData.getComponent() << ") for rpcCall " << dataSourceName << ".");
            return false;
        }

        EA::TDF::TdfPtr request = commandInfo->createRequest("EDM Rpc Request");
        EA::TDF::TdfPtr response = commandInfo->createResponse("EDM Rpc Response");
        EA::TDF::TdfPtr errResponse = commandInfo->createErrorResponse("EDM Rpc Err Response");

        // Fill in the missing attributes:
        eastl::string accessToken;
        getNucleusAccessToken(accessToken, OAuth::TOKEN_TYPE_NONE);

        if (attributes.find(BLAZE_ID_KEY) == attributes.end())
            attributes[BLAZE_ID_KEY] = attributes.allocate_element();
        if (attributes.find(ACCOUNT_ID_KEY) == attributes.end())
            attributes[ACCOUNT_ID_KEY] = attributes.allocate_element();
        if (attributes.find(S2S_ACCESS_TOKEN_KEY) == attributes.end())
            attributes[S2S_ACCESS_TOKEN_KEY] = attributes.allocate_element();
        if (attributes.find(CLIENT_PLATFORM_KEY) == attributes.end())
            attributes[CLIENT_PLATFORM_KEY] = attributes.allocate_element();

        attributes[BLAZE_ID_KEY]->set(perPlayerJoinData->getUser().getBlazeId());
        attributes[ACCOUNT_ID_KEY]->set(perPlayerJoinData->getUser().getPlatformInfo().getEaIds().getNucleusAccountId());
        attributes[S2S_ACCESS_TOKEN_KEY]->set(accessToken.c_str());

        // we don't just use the default platform because there may be a HTTP/gRPC session matchmaking on behalf of a console user session
        // and we'll want to use the player's platform for recommendations.
        ClientPlatformType clientPlatform = gController->getDefaultClientPlatform();
        if (gCurrentLocalUserSession != nullptr)
        {
            ClientPlatformType playerPlatform = gCurrentLocalUserSession->getClientPlatform();
            if (playerPlatform != INVALID)
            {
                clientPlatform = playerPlatform;
            }
        }
        attributes[CLIENT_PLATFORM_KEY]->set(UserSessionManager::blazeClientPlatformTypeToPINClientPlatformString(clientPlatform));

        // Parse the template attributes and config data into the request - 
        if (request != nullptr)
        {
            const char8_t* failingAttribute = "";
            EA::TDF::TdfGenericReference requestRef(request);
            StringBuilder componentDescription;
            componentDescription << "In ExternalDataManager::makeRpcCall: in rpcCall " << dataSourceName << " ";
            // Dummy properties, because the real ones should already be applied
            PropertyNameMap properties;
            BlazeRpcError err = applyTemplateAttributes(requestRef, rpcCallData.getRequestAttributes(), attributes, properties, failingAttribute, componentDescription.get());
            if (err != ERR_OK)
            {
                ERR_LOG("[ExternalDataManager].makeRpcCall(): Unable to apply template attributes for attribute " << failingAttribute << " in rpcCall " << dataSourceName << ".");
                return false;
            }
        }

        Component* componentProxy = gController->getComponent(compData->id, false, false); 
        if (componentProxy == nullptr)
        {
            // Check if it is a proxy http component
            componentProxy = (Component*)Blaze::gOutboundHttpService->getService(compData->name);
            if (componentProxy == nullptr)
            {
                ERR_LOG("[ExternalDataManager].makeRpcCall(): Component (" << compData->name << " not exists.");
                return false;
            }
        }

        bool results = true;
        BlazeRpcError err = componentProxy->sendRequest(*commandInfo, request, response, errResponse, RpcCallOptions());
        if (err != ERR_OK)
        {
            ERR_LOG("[ExternalDataManager].makeRpcCall(): Failed to sendRequest (" << rpcCallData.getComponent() << " : " << rpcCallData.getCommand() << " in rpcCall " << dataSourceName << ".");
            results = false;
            // We don't return here, to allow the code to go and set defaults, even though the request failed. 
        }

        if (response != nullptr)
        {
            // Parse out the response into the data we need: 
            // This means that we need to:
            // 1) Iterate over the output lists
            // 2) Lookup/Set/Convert the data into the output values:
            EA::TDF::TdfGenericReference responseRef(response);


            // get all the user's user sessions
            UserSessionIdList userSessionIds;
            //bool isLeader = false;
            if (perPlayerJoinData != nullptr)
            {
                //isLeader = (gCurrentLocalUserSession->getBlazeId() == perPlayerJoinData->getUser().getBlazeId());
                if (perPlayerJoinData->getUser().getBlazeId() != INVALID_BLAZE_ID)
                {
                    gUserSessionManager->getUserSessionIdsByBlazeId(perPlayerJoinData->getUser().getBlazeId(), userSessionIds);
                }
                else if (perPlayerJoinData->getUser().getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID)
                {
                    gUserSessionManager->getUserSessionIdsByAccountId(perPlayerJoinData->getUser().getPlatformInfo().getEaIds().getNucleusAccountId(), userSessionIds); 
                }
                else
                {
                    gUserSessionManager->getUserSessionIdsByPlatformInfo(perPlayerJoinData->getUser().getPlatformInfo(), userSessionIds);
                }
            }

            // Template Mappings: 
            {
                EA::TDF::TdfFactory& tdfFactory = EA::TDF::TdfFactory::get();
                for (auto& attrMapping : rpcCallData.getResponseAttributes().getTemplateAttributes())
                {
                    // For each attribute, attempt to lookup the data.  If you can't find it, then it's not there I guess.
                    const EA::TDF::TypeDescriptionBitfieldMember* outBfMember = nullptr;
                    EA::TDF::TdfGenericReference responseAttributeData;
                    if (!tdfFactory.getTdfMemberReference(responseRef, attrMapping.second, responseAttributeData, &outBfMember))
                    {
                        // We should've already checked for this.
                        ERR_LOG("[ExternalDataManager].makeRpcCall(): Failed to unmap member " << attrMapping.first.c_str() << " in rpcCall " << dataSourceName << ".");
                        continue;
                    }

                    // Now we have the data from the response, so just add it to the template mapping: 
                    if (attributes.find(attrMapping.first) == attributes.end())
                        attributes[attrMapping.first] = attributes.allocate_element();

                    attributes[attrMapping.first]->set(responseAttributeData);
                }

                for (auto& sessionId : userSessionIds)
                {
                    // Skip invalid sessions:
                    if (sessionId != INVALID_USER_SESSION_ID)
                    {
                        // Do the same thing for the UED attributes- But attempt a int conversion: 
                        for (auto& uedMapping : rpcCallData.getResponseAttributes().getUedAttributes())
                        {
                            // For each attribute, attempt to lookup the data.  If you can't find it, then it's not there I guess.
                            const EA::TDF::TypeDescriptionBitfieldMember* outBfMember = nullptr;
                            EA::TDF::TdfGenericReference responseAttributeData;
                            if (!tdfFactory.getTdfMemberReference(responseRef, uedMapping.second, responseAttributeData, &outBfMember))
                            {
                                // We should've already checked for this.
                                ERR_LOG("[ExternalDataManager].makeRpcCall(): Failed to unmap member " << uedMapping.first.c_str() << " in rpcCall " << dataSourceName << ".");
                                continue;
                            }

                            Blaze::UserExtendedDataValue uedValue = 0;
                            EA::TDF::TdfGenericReference uedValueRef(uedValue);
                            bool convertSuccess = responseAttributeData.convertToResult(uedValueRef);

                            if (!convertSuccess)
                            {
                                ERR_LOG("[ExternalDataManager].makeRpcCall(): Failed to convert member " << uedMapping.first.c_str() << " to a UED value in rpcCall " << dataSourceName << ".");
                                continue;
                            }

                            // It would probably be better to do this automatically elsewhere, but w/e.
                            UserExtendedDataKey dataKey;
                            bool found = gUserSessionManager->getUserExtendedDataKey(uedMapping.first, dataKey);
                            if (!found)
                            {
                                WARN_LOG("[ExternalDataManager].makeRpcCall(): UED key not yet registered in system for UED(" << uedMapping.first << ") in rpcCall " << dataSourceName << ".");
                                continue;
                            }
                            BlazeRpcError error = gUserSessionManager->updateExtendedData(sessionId, COMPONENT_ID_FROM_UED_KEY(dataKey), DATA_ID_FROM_UED_KEY(dataKey), uedValue);
                            if (error != ERR_OK)
                            {
                                WARN_LOG("[ExternalDataManager].makeRpcCall(): updating UED value for keyid(" << uedMapping.first << ") for GameManagerSlave componenet failed due to err(" << ErrorHelp::getErrorName(error) << ")");
                            }
                        }
                    }
                }

            }
        }

        return results;
    }

    bool ExternalDataManager::sendExternalGrpcRequest(const ExternalApiName& externalApiName, ExternalDataSourceGrpcDefinition& externalDataSourceGrpcDefinition,
                                                      TemplateAttributes& attributes, PerPlayerJoinDataPtr perPlayerJoinData) const
    {
        Grpc::OutboundRpcBasePtr rpc = createOutboundRpc(externalDataSourceGrpcDefinition);
        if (rpc == nullptr)
        {
            ERR_LOG("[ExternalDataManager].sendExternalGrpcRequest: Failed to create outbound gRPC request!");
            
            //Set defaults:
            RawBuffer rawBuffer;
            decodeExternalData(externalDataSourceGrpcDefinition.getOutput(), rawBuffer, attributes, perPlayerJoinData);
            return false;
        }
        
        if (!filloutGrpcClientContext(externalDataSourceGrpcDefinition.getHeaders(), attributes, perPlayerJoinData, rpc->getClientContext()))
        {
            ERR_LOG("[ExternalDataManager].sendExternalGrpcRequest: Error setting the Grpc ClientContext!");

            //Set defaults:
            RawBuffer rawBuffer;
            decodeExternalData(externalDataSourceGrpcDefinition.getOutput(), rawBuffer, attributes, perPlayerJoinData);
            return false;
        }

        const google::protobuf::Descriptor* reqDesc = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(externalDataSourceGrpcDefinition.getRequestType());
        google::protobuf::Message* request = google::protobuf::MessageFactory::generated_factory()->GetPrototype(reqDesc)->New();
        for (auto& itr : externalDataSourceGrpcDefinition.getRequestAttributes())
        {
            ParamKey requestKey = itr.first;
            ExternalDataSourceParameterPtr param = itr.second;

            // Based on the request field type, attempt to fetch the attribute as that type
            TemplateAttributes::const_iterator scenarioAttrItr = attributes.find(param->getName());
            if (scenarioAttrItr != attributes.end())
            {
                EA::TDF::TdfGenericReferenceConst attr(scenarioAttrItr->second->get());
                bool ret = filloutGrpcRequest(reqDesc, attr, requestKey.c_str(), request);
                if (!ret)
                    continue;
            }
            else if (param->getDefaultValue()[0] != '\0')
            {
                StringBuilder sb;
                if ((blaze_stricmp(param->getDefaultValue(), BLAZE_ID_KEY)) == 0 && (perPlayerJoinData != nullptr) && (perPlayerJoinData->getUser().getBlazeId() != INVALID_BLAZE_ID))
                {
                    sb << perPlayerJoinData->getUser().getBlazeId();
                }
                else if ((blaze_stricmp(param->getDefaultValue(), ACCOUNT_ID_KEY) == 0) && (perPlayerJoinData != nullptr) && (perPlayerJoinData->getUser().getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID))
                {
                    sb << perPlayerJoinData->getUser().getPlatformInfo().getEaIds().getNucleusAccountId();
                }
                else if (blaze_stricmp(param->getDefaultValue(), SOCIAL_GROUP_ID_ATTR_KEY) == 0 && (perPlayerJoinData != nullptr))
                {
                    TemplateAttributes::iterator socialGroupAttrItr = attributes.find(SOCIAL_GROUP_ATTRIBUTE_KEY);
                    if (socialGroupAttrItr != attributes.end() && socialGroupAttrItr->second->get().isTypeString())
                    {
                        sb << socialGroupAttrItr->second->get().asString().c_str();
                    }
                    else
                    {
                        WARN_LOG("[ExternalDataManager].sendExternalGrpcRequest: failed to set request attribute(" << requestKey << "), because it did not have Social Group id sent via the client.");
                        continue;
                    }
                }
                else
                {
                    sb << param->getDefaultValue();
                }

                EA::TDF::TdfString str(sb.get());
                EA::TDF::TdfGenericReferenceConst attr(str);
                bool ret = filloutGrpcRequest(reqDesc, attr, requestKey.c_str(), request);
                if (!ret)
                    continue;
            }
            else
            {
                TRACE_LOG("[ExternalDataManager].sendExternalGrpcRequest: failed to set request attribute(" << requestKey << ") because it did not have a value sent via the client or a default value.");
            }
        }

        Blaze::Grpc::ResponsePtr response;
        BlazeRpcError rc = rpc->sendRequest(request, response);
        delete request;
        if (rc != ERR_OK)
        {
            ERR_LOG("[ExternalDataManager].sendExternalGrpcRequest: Failed to send outbound gRPC request with error " << ErrorHelp::getErrorName(rc));

            //Set defaults:
            RawBuffer rawBuffer;
            decodeExternalData(externalDataSourceGrpcDefinition.getOutput(), rawBuffer, attributes, perPlayerJoinData);
            return false;
        }

        const google::protobuf::Message& responseRaw = *response.get();
        std::string jsonString;
        google::protobuf::util::JsonPrintOptions jsonPrintOptions;
        jsonPrintOptions.always_print_primitive_fields = true;  //print primitive fields in json string even if value is equal to default 0/false
        jsonPrintOptions.always_print_enums_as_ints = true;
        jsonPrintOptions.preserve_proto_field_names = true;
        google::protobuf::util::MessageToJsonString(responseRaw, &jsonString, jsonPrintOptions);
        
        RawBuffer rawBuffer((uint8_t*)jsonString.c_str(), jsonString.size());
        rawBuffer.put(jsonString.size());
        decodeExternalData(externalDataSourceGrpcDefinition.getOutput(), rawBuffer, attributes, perPlayerJoinData);

        return true;
    }

    bool ExternalDataManager::sendExternalAPIRequest(ExternalApiName& externalApiName, ExternalDataSourceApiDefinition& externalDataSourceApiDefinition, TemplateAttributes& attributes, PerPlayerJoinDataPtr perPlayerJoinData, TimeValue& timeTaken)
    {
        bool result = true;
        const char8_t* externalHttpService = externalDataSourceApiDefinition.getExternalDataSource();

        HttpConnectionManagerPtr connectionManager = gOutboundHttpService->getConnection(externalHttpService);
        if (connectionManager == nullptr)
        {
            ERR_LOG("[ExternalDataManager].sendExternalAPIRequest: Unable to find external service("<< externalHttpService << ") registered in gOutboundHttpService for External Data API(" << externalApiName << ")");
            result = false;
            return result;
        }

        eastl::string url = externalDataSourceApiDefinition.getUrl();
        if (!prepareBaseURL(externalDataSourceApiDefinition.getUrlPathParameters(), attributes, url, perPlayerJoinData))
        {
            WARN_LOG("[ExternalDataManager].sendExternalAPIRequest: Failed to prepare base url for External Data API(" << externalApiName << ")");
            result = false;
        }

        RestRequestBuilder::HttpParamVector httpParams;
        if (!prepareUriQueryParameters(externalDataSourceApiDefinition.getUrlQueryParameters(), attributes, httpParams, perPlayerJoinData))
        {
            WARN_LOG("[ExternalDataManager].sendExternalAPIRequest: Failed to prepare uri query parameters for External Data API(" << externalApiName << ")");
            result = false;
        }

        RestRequestBuilder::HeaderVector headerVector;
        if (!prepareHttpHeaders(externalDataSourceApiDefinition.getHeaders(), attributes, headerVector, perPlayerJoinData))
        {
            WARN_LOG("[ExternalDataManager].sendExternalAPIRequest: Failed to prepare http headers for External Data API(" << externalApiName << ")");
            result = false;
        }

        const char8_t** httpHeaders = nullptr;
        httpHeaders = RestRequestBuilder::createHeaderCharArray(headerVector);

        OutboundHttpConnection::ContentPartList contentList;

        RawRestOutboundHttpResult httpResult;

        if (timeTaken < TOTAL_EXT_REQUEST_TIMEOUT)
        {
            RpcCallOptions rpcCallOptions;
            rpcCallOptions.timeoutOverride = ((TOTAL_EXT_REQUEST_TIMEOUT - timeTaken) > PER_EXT_REQUEST_TIMEOUT) ? PER_EXT_REQUEST_TIMEOUT : TOTAL_EXT_REQUEST_TIMEOUT - timeTaken;

            TimeValue preRequestTime = TimeValue::getTimeOfDay();
            connectionManager->sendRequest(HttpProtocolUtil::HttpMethod::HTTP_GET, url.c_str(), &httpParams[0], httpParams.size(), httpHeaders, 
                headerVector.size(), &httpResult, &contentList, HTTP_LOGGER_CATEGORY, "GameManager::startMatchmaking", nullptr, 0, RpcCallOptions());
            TimeValue requestTime = TimeValue::getTimeOfDay() - preRequestTime;
            timeTaken += requestTime;

            connectionManager->metricProxyRequest(externalHttpService, externalApiName.c_str(), requestTime, httpResult.getHttpStatusCode());
            decodeExternalData(externalDataSourceApiDefinition.getOutput(), httpResult.getBuffer(), attributes, perPlayerJoinData);
        }
        else
        {
            ERR_LOG("[ExternalDataManager].sendExternalAPIRequest: Failed to send http request to external data source(" << externalHttpService << ") for External Data API(" << externalApiName << "), because overall timeout of for external requests was reached.");
            result = false;

            // Decode values as defaults: 
            decodeExternalData(externalDataSourceApiDefinition.getOutput(), httpResult.getBuffer(), attributes, perPlayerJoinData);
        }

        // clean up locally allocated memory
        for (auto& httpParam : httpParams)
        {
            if (httpParam.name != nullptr)
            {
                BLAZE_FREE(httpParam.name);
            }
            if (httpParam.value != nullptr)
            {
                BLAZE_FREE(httpParam.value);
            }
        }
                
        if (httpHeaders != nullptr)
            delete[] httpHeaders;

        return result;
    }

    bool ExternalDataManager::prepareBaseURL(UrlPathParametersMap& urlPathParametersMap, TemplateAttributes& attributes, eastl::string& url, PerPlayerJoinDataPtr perPlayerJoinData) const
    {
        bool result = true;
        UrlPathParametersMap::iterator itr = urlPathParametersMap.begin();
        UrlPathParametersMap::iterator end = urlPathParametersMap.end();
        for (; itr != end; ++itr)
        {
            ParamKey paramName = itr->first;
            ExternalDataSourceParameterPtr uriParamPtr = itr->second;
            TemplateAttributes::iterator scenarioAttrItr = attributes.find(uriParamPtr->getName());
            size_t index = url.find(paramName);
            if (index == eastl::string::npos)
            {
                continue;
            }

            if (uriParamPtr->getName()[0] != '\0' && scenarioAttrItr != attributes.end())
            {
                // Check request to see if client sent up a value for this uri parameter via the request's attribute map
                EA::TDF::TdfGenericValue& attr = attributes[uriParamPtr->getName()]->get();
                switch (attr.getType())
                {
                case EA::TDF::TdfType::TDF_ACTUAL_TYPE_STRING:
                {
                    url.replace(index, strlen(paramName), attr.asString().c_str());
                }
                break;
                case EA::TDF::TdfType::TDF_ACTUAL_TYPE_LIST:
                {
                    EA::TDF::TdfVectorBase& tdfVector = attr.asList();
                    if (tdfVector.vectorSize() > 0)
                    {
                        // if attribute is not a string, we need to convert it to one. Create a temp generic of type string to store converted value in
                        EA::TDF::TdfString propertyString;
                        EA::TDF::TdfGenericReferenceConst itemValue;
                        EA::TDF::TdfGenericReference propertyGenRef(propertyString);
                        tdfVector.getValueByIndex(0, itemValue); // Just using the first value as once replaced there should not be more matches
                        if (itemValue.convertToString(propertyGenRef))
                        {
                            url.replace(index, strlen(paramName), propertyGenRef.asString().c_str());
                        }
                        else
                        {
                            WARN_LOG("[ExternalDataManager].prepareBaseURL: Failed to replace UrlPathParameter(" << paramName << ") for URL(" << url << "), because the value sent by client is a list that cannot be converted to a list of strings.");
                            result = false;
                        }
                    }
                    else
                    {
                        WARN_LOG("[ExternalDataManager].prepareBaseURL: Failed to replace UrlPathParameter(" << paramName << ") for URL(" << url << "), because the value sent by client is an empty list.");
                        result = false;
                    }
                }
                break;
                default:
                {
                    EA::TDF::TdfString keyString;
                    EA::TDF::TdfGenericReference criteriaAttr(keyString);
                    if (attr.convertToString(criteriaAttr))
                    {
                        url.replace(index, strlen(paramName), criteriaAttr.asString().c_str());
                    }
                    else
                    {
                        WARN_LOG("[ExternalDataManager].prepareBaseURL: Failed to replace UrlPathParameter(" << paramName << ") for URL(" << url << "), because the value sent by client is not a string and could not be converted to a string.");
                        result = false;
                    }
                }
                break;
                }                
            }
            else if (uriParamPtr->getDefaultValue()[0] != '\0')
            {
                StringBuilder str;
                if ((blaze_stricmp(uriParamPtr->getDefaultValue(), BLAZE_ID_KEY)) == 0 && (perPlayerJoinData != nullptr) && (perPlayerJoinData->getUser().getBlazeId() != INVALID_BLAZE_ID))
                {   
                    str << perPlayerJoinData->getUser().getBlazeId();
                }
                else if ((blaze_stricmp(uriParamPtr->getDefaultValue(), ACCOUNT_ID_KEY) == 0) && (perPlayerJoinData != nullptr) && (perPlayerJoinData->getUser().getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID))
                {   
                    str <<  perPlayerJoinData->getUser().getPlatformInfo().getEaIds().getNucleusAccountId();
                }
                else if (blaze_stricmp(uriParamPtr->getDefaultValue(), SOCIAL_GROUP_ID_ATTR_KEY) == 0 && (perPlayerJoinData != nullptr))
                {
                    TemplateAttributes::iterator socialGroupAttrItr = attributes.find(SOCIAL_GROUP_ATTRIBUTE_KEY);
                    if (socialGroupAttrItr != attributes.end() && socialGroupAttrItr->second->get().isTypeString())
                    {
                        str << socialGroupAttrItr->second->get().asString().c_str();
                    }
                    else
                    {
                        WARN_LOG("[ExternalDataManager].prepareBaseURL: Failed to replace UrlPathParameter(" << paramName << ") for URL(" << url << "), because it did not have Social Group id sent via the client.");
                        result = false;
                    }
                }
                else
                {
                    str <<  uriParamPtr->getDefaultValue();
                }
                url.replace(index, strlen(paramName), str.get());
            }
            else if (!uriParamPtr->getIsOptional())
            {
                ERR_LOG("[ExternalDataManager].prepareBaseURL: Failed to replace UrlPathParameter("<< paramName <<") for URL(" << url << "), because it did not have a value sent via the client or a default value.");
                result = false;
            }
        }

        return result;
    }

    bool ExternalDataManager::prepareUriQueryParameters(UrlQueryParametersMap& urlQueryParametersMap, TemplateAttributes& attributes, RestRequestBuilder::HttpParamVector& httpParams, PerPlayerJoinDataPtr perPlayerJoinData) const
    {
        bool result = true;
        UrlQueryParametersMap::iterator itr = urlQueryParametersMap.begin();
        UrlQueryParametersMap::iterator end = urlQueryParametersMap.end();
        for (; itr != end; ++itr)
        {
            ParamKey paramName = itr->first;
            ExternalDataSourceParameterPtr uriParamPtr = itr->second;
            TemplateAttributes::iterator scenarioAttrItr = attributes.find(uriParamPtr->getName());

            if (uriParamPtr->getName()[0] != '\0' && scenarioAttrItr != attributes.end())
            {
                // Check request to see if client sent up a value for this uri parameter via the request's attribute map
                RestRequestBuilder::HttpParamVector::reference param = httpParams.push_back();
                param.encodeValue = true;
                param.name = blaze_strdup(paramName);

                if (!attributes[uriParamPtr->getName()]->get().isTypeString())
                {
                    // if attribute is not a string, we need to convert it to one. Create a temp generic of type string to store converted value in.
                    EA::TDF::TdfString keyString;
                    EA::TDF::TdfGenericReference criteriaAttr(keyString);
                    attributes[uriParamPtr->getName()]->get().convertToString(criteriaAttr);
                    param.value = blaze_strdup(keyString.c_str());
                }
                else
                {
                    param.value = blaze_strdup(attributes[uriParamPtr->getName()]->get().asString().c_str());
                }
            }
            else if (uriParamPtr->getDefaultValue()[0] != '\0')
            {
                RestRequestBuilder::HttpParamVector::reference param = httpParams.push_back();
                param.name = blaze_strdup(paramName);
            
                if ((blaze_stricmp(uriParamPtr->getDefaultValue(), BLAZE_ID_KEY)) == 0 && (perPlayerJoinData != nullptr) && (perPlayerJoinData->getUser().getBlazeId() != INVALID_BLAZE_ID))
                {                   
                    StringBuilder str;
                    str << perPlayerJoinData->getUser().getBlazeId();
                    param.value = blaze_strdup(str.get());
                }
                else if ((blaze_stricmp(uriParamPtr->getDefaultValue(), ACCOUNT_ID_KEY) == 0) && (perPlayerJoinData != nullptr) && (perPlayerJoinData->getUser().getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID))
                {   
                    StringBuilder str;
                    str <<  perPlayerJoinData->getUser().getPlatformInfo().getEaIds().getNucleusAccountId();
                    param.value = blaze_strdup(str.get());
                }
                else if (blaze_stricmp(uriParamPtr->getDefaultValue(), S2S_ACCESS_TOKEN_KEY) == 0)
                {
                    eastl::string accessToken;
                    getNucleusAccessToken(accessToken, OAuth::TOKEN_TYPE_NONE);
                    param.value = blaze_strdup(accessToken.c_str());
                }
                else
                {
                    param.value = blaze_strdup(uriParamPtr->getDefaultValue());
                }
                param.encodeValue = true;
            }
            else if (!uriParamPtr->getIsOptional())
            {
                ERR_LOG("[ExternalDataManager].prepareUriQueryParameters: Failed to add uri query parameter(" << paramName << "), because it did not have a value sent via the client or a default value.");
                result = false;
            }
        }

        return result;
    }

    bool ExternalDataManager::prepareHttpHeaders(HeadersMap& headersMap, TemplateAttributes& attributes, RestRequestBuilder::HeaderVector& headerVector, PerPlayerJoinDataPtr perPlayerJoinData) const
    {
        bool result = true;
        HeadersMap::iterator itr = headersMap.begin();
        HeadersMap::iterator end = headersMap.end();
        for (; itr != end; ++itr)
        {
            ParamKey headerName = itr->first;
            ExternalDataSourceParameterPtr headerParamPtr = itr->second;
            StringBuilder str;
            str << headerName << ": ";

            if (headerParamPtr->getName()[0] != '\0' && attributes.find(headerParamPtr->getName()) != attributes.end())
            {
                // Check request to see if client sent up a value for this uri parameter via the request's attribute map
                if (!attributes[headerParamPtr->getName()]->get().isTypeString())
                {
                    // if attribute is not a string, we need to convert it to one. Create a temp generic of type string to store converted value in.
                    EA::TDF::TdfString keyString;
                    EA::TDF::TdfGenericReference criteriaAttr(keyString);
                    attributes[headerParamPtr->getName()]->get().convertToString(criteriaAttr);
                    str << keyString.c_str();
                }
                else
                {
                    str << attributes[headerParamPtr->getName()]->get().asString().c_str();
                }
                headerVector.push_back(str.get());
            }
            else if (headerParamPtr->getDefaultValue()[0] != '\0')
            {
                if ((blaze_stricmp(headerParamPtr->getDefaultValue(), BLAZE_ID_KEY)) == 0 && (perPlayerJoinData != nullptr) && (perPlayerJoinData->getUser().getBlazeId() != INVALID_BLAZE_ID))
                {   
                    str << perPlayerJoinData->getUser().getBlazeId();
                }
                else if ((blaze_stricmp(headerParamPtr->getDefaultValue(), ACCOUNT_ID_KEY) == 0) && (perPlayerJoinData != nullptr) && (perPlayerJoinData->getUser().getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID))
                {   
                    str <<  perPlayerJoinData->getUser().getPlatformInfo().getEaIds().getNucleusAccountId();
                }
                else if (blaze_stricmp(headerParamPtr->getDefaultValue(), S2S_ACCESS_TOKEN_KEY) == 0)
                {
                    eastl::string accessToken;
                    BlazeRpcError error = getNucleusAccessToken(accessToken, OAuth::TOKEN_TYPE_NEXUS_S2S);
                    if (error != ERR_OK)
                    {
                        WARN_LOG("[ExternalDataManager].prepareHttpHeaders: Failed to fetch access token for header(" <<  headerName << ") due to error(" << ErrorHelp::getErrorName(error) <<")");
                        result = false;
                    }
                    else
                    {
                        str << accessToken;
                    }
                }
                else if (blaze_stricmp(headerParamPtr->getDefaultValue(), S2S_ACCESS_TOKEN_BEARER_PREFIX_KEY) == 0)
                {
                    // Require access token with Bearer prefix, instead of NEXUS_S2S
                    eastl::string accessToken;
                    BlazeRpcError error = getNucleusAccessToken(accessToken, OAuth::TOKEN_TYPE_BEARER);
                    if (error != ERR_OK)
                    {
                        WARN_LOG("[ExternalDataManager].prepareHttpHeaders: Failed to fetch bearer access token for header(" << headerName << ") due to error(" << ErrorHelp::getErrorName(error) << ")");
                        result = false;
                    }
                    else
                    {
                        str << accessToken;
                    }
                }
                else
                {
                    str << headerParamPtr->getDefaultValue();
                }
                headerVector.push_back(str.get());
            }
            else if (!headerParamPtr->getIsOptional())
            {
                ERR_LOG("[ExternalDataManager].prepareHttpHeaders: Failed to prepare http header(" <<  headerName << ") because it did not have a value sent via the client or a default value.");
                result = false;
            }
        }

        return result;
    }

    void ExternalDataManager::decodeExternalData(const RequestOverrideKeyMap& requestOverrideKeyMap, RawBuffer& rawBuffer, TemplateAttributes& attributes, const PerPlayerJoinDataPtr perPlayerJoinData) const 
    {
        RawBufferIStream rawBufferISStream(rawBuffer);
        IStreamToJsonReadStream iStreamToJsonReadStream(rawBufferISStream);

        EA::Json::JsonDomReader domReader(Blaze::BlazeStlAllocator("DomReader", GameManagerSlave::COMPONENT_MEMORY_GROUP).get_allocator());
        EA::Json::JsonDomDocument domDocument(Blaze::BlazeStlAllocator("DomReader", GameManagerSlave::COMPONENT_MEMORY_GROUP).get_allocator());;

        domReader.SetStream(&iStreamToJsonReadStream);
        EA::Json::Result jsonResult = domReader.Build(domDocument);
        if (jsonResult != EA::Json::Result::kSuccess)
        {
            WARN_LOG("[ExternalDataManager].decodeExternalData: Failed to build JSON dom document; error(" << EA::Json::GetJsonReaderResultString(jsonResult) << ")");
            return;
        }

        bool isLeader = false;

        // get all the user's user sessions
        UserSessionIdList userSessionIds;

        UserSessionExtendedData userSessionExtendedData;
        if (perPlayerJoinData != nullptr)
        {
            isLeader = (gCurrentLocalUserSession->getBlazeId() == perPlayerJoinData->getUser().getBlazeId());
            if (perPlayerJoinData->getUser().getBlazeId() != INVALID_BLAZE_ID)
            {
                gUserSessionManager->getUserSessionIdsByBlazeId(perPlayerJoinData->getUser().getBlazeId(), userSessionIds);
            }
            else if (perPlayerJoinData->getUser().getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID)
            {
                gUserSessionManager->getUserSessionIdsByAccountId(perPlayerJoinData->getUser().getPlatformInfo().getEaIds().getNucleusAccountId(), userSessionIds);
            }
            else
            {
                gUserSessionManager->getUserSessionIdsByPlatformInfo(perPlayerJoinData->getUser().getPlatformInfo(), userSessionIds);
            }

            UserInfoData userInfo;
            userInfo.setId(perPlayerJoinData->getUser().getBlazeId());
            perPlayerJoinData->getUser().getPlatformInfo().copyInto(userInfo.getPlatformInfo());
            if (gUserSessionManager->requestUserExtendedData(userInfo, userSessionExtendedData) != ERR_OK)
            {
                TRACE_LOG("[ExternalDataManager].decodeExternalData unable to fetch extended data for user");
            }
        }

        for (auto& keyItr : requestOverrideKeyMap)
        {
            ParamKey requestOverrideKey = keyItr.first;
            ExternalDataSourceParameterPtr outputPtr = keyItr.second;

            char8_t* attr = nullptr;
            if ((attr = blaze_strnistr(requestOverrideKey, TEMPLATE_ATTRIBUTES_KEY, requestOverrideKey.length())) != nullptr)
            {
                if (!isLeader)
                {
                    TRACE_LOG("[ExternalDataManager].decodeExternalData only leader is allowed to set template attribute("
                            << requestOverrideKey << ").");
                    continue;
                }

                //Overriding Template Attributes
                attr += strlen(TEMPLATE_ATTRIBUTES_KEY);
                
                auto attrIter = attributes.find(attr);
                if (attrIter == attributes.end())
                {
                    attrIter = attributes.insert(eastl::make_pair(attr, attributes.allocate_element())).first;
                }

                TRACE_LOG("[ExternalDataManager].decodeExternalData"
                    << "\n  attr:\n    " << attr
                    << "\n  outputPtr:\n" << SbFormats::PrefixAppender("    ", (StringBuilder() << outputPtr).get()));

                // if we're looking for a list, we expect the format to be "<list_node_path>/*/<element_node_path>"
                // e.g. "/entries/*/personaId" where the resulting list node path would be "/entries/" and the resulting element node path would be "/personaId"
                const char8_t LIST_DELIMITER[] = "*/";
                const char8_t MAP_DELIMITER[] = "^/";

                eastl::string outputPath = outputPtr->getName();
                auto listStart = outputPath.find(LIST_DELIMITER);
                auto foundList = (listStart != eastl::string::npos);
                auto mapStart = outputPath.find(MAP_DELIMITER);
                auto foundMap = (mapStart != eastl::string::npos);
                if (foundMap)
                {
                    eastl::string mapNodePath = outputPath.substr(0, mapStart);
                    decodeExternalDataMap(attr, attributes, mapNodePath.c_str(), domDocument, outputPtr->getScalingFactor(), outputPtr->getMergeLists());
                }
                else if (foundList)
                {
                    // /{path1|path2}/*/value syntax merges lists /path1/*/value and /path2/*/value
                    const char8_t LIST_ALTPATH_BEGIN = '{';
                    const char8_t LIST_ALTPATH_SEPARATOR = '|';
                    const char8_t LIST_ALTPATH_END = '}';

                    eastl::string listNodePath = outputPath.substr(0, listStart);
                    eastl::string elementNodePath = outputPath.substr(listStart + 1); // skip the "*"

                    TRACE_LOG("[ExternalDataManager].decodeExternalData"
                        << "\n  list path: " << listNodePath.c_str()
                        << "\n  elem path: " << elementNodePath.c_str());

                    auto altpathBegin = listNodePath.find(LIST_ALTPATH_BEGIN);
                    bool hasAltPath = (altpathBegin != eastl::string::npos);
                    if (hasAltPath)
                    {
                        eastl::string listNodePathPrefix = listNodePath.substr(0, altpathBegin);
                        eastl::string listNodePathNoPrefix = listNodePath.substr(altpathBegin + 1); // skip the '{'

                        auto altpathEnd = listNodePathNoPrefix.find(LIST_ALTPATH_END);
                        if (altpathEnd == eastl::string::npos)
                        {
                            WARN_LOG("[ExternalDataManager].decodeExternalData Found unclosed '" << LIST_ALTPATH_BEGIN << "' in list path " << listNodePath.c_str());
                            hasAltPath = false;
                        }
                        else
                        {
                            eastl::string allPaths = listNodePathNoPrefix.substr(0, altpathEnd + 1); // include the '}'
                            eastl::string listNodePathSuffix = listNodePathNoPrefix.substr(altpathEnd + 1); // skip the '}'
                            size_t pathStart = 0;
                            size_t pathLen = 0;
                            bool mergeLists = outputPtr->getMergeLists();
                            for (auto& curChar : allPaths)
                            {
                                if (curChar == LIST_ALTPATH_SEPARATOR || curChar == LIST_ALTPATH_END)
                                {
                                    eastl::string altPath = listNodePathPrefix + allPaths.substr(pathStart, pathLen) + listNodePathSuffix;
                                    if (decodeExternalDataList(attr, attributes, requestOverrideKey, altPath.c_str(), elementNodePath.c_str(), domDocument, outputPtr->getScalingFactor(), mergeLists))
                                        mergeLists = true; // after successfully decoding the first list specified by the {path1|path2} syntax, merge all following lists
                                    pathStart += pathLen+1;
                                    pathLen = 0;
                                    continue;
                                }
                                ++pathLen;
                            }
                        }
                    }
                    if (!hasAltPath)
                    {
                        decodeExternalDataList(attr, attributes, requestOverrideKey, listNodePath.c_str(), elementNodePath.c_str(), domDocument, outputPtr->getScalingFactor(), outputPtr->getMergeLists());
                    }
                }
                else
                {
                    EA::Json::JsonDomNode* jsonDomNode = domDocument.GetNode(outputPtr->getName());
                    if (jsonDomNode != nullptr)
                    {
                        TRACE_LOG("[ExternalDataManager].decodeExternalData: Adding single element value to " << attr);
                        switch (jsonDomNode->GetNodeType())
                        {
                        case EA::Json::EventType::kETInteger:
                        case EA::Json::EventType::kETDouble:
                        {
                            attributes[attr]->set(getScaledInt64(jsonDomNode, outputPtr->getScalingFactor()));
                        }
                        break;
                        case EA::Json::EventType::kETBool:
                        {
                            attributes[attr]->set(jsonDomNode->AsJsonDomBool()->mValue);
                        }
                        break;
                        case EA::Json::EventType::kETString:
                        {
                            attributes[attr]->set(jsonDomNode->AsJsonDomString()->mValue.c_str());
                        }
                        break;
                        default:
                            TRACE_LOG("[ExternalDataManager].decodeExternalData unable to parse data from response for RequestOverrideKey(" << requestOverrideKey << ") because type("
                                << jsonDomNode->GetNodeType() << ") is not supported. Utilizing default value instead.");
                            attributes[attr]->set(outputPtr->getDefaultValue());
                            break;
                        }
                    }
                    else
                    {
                        attributes[attr]->set(outputPtr->getDefaultValue());
                    }
                }
            }
            else if ((attr = blaze_strnistr(requestOverrideKey, UED_KEY, requestOverrideKey.length())) != nullptr)
            {
                //Overriding User Extended Data
                Blaze::UserExtendedDataValue uedvalue;

                attr += strlen(UED_KEY);

                char8_t dataName[MAX_USEREXTENDEDDATANAME_LEN];
                blaze_snzprintf(dataName, MAX_USEREXTENDEDDATANAME_LEN, "%s_%s", mComponent->getComponentName(), attr);

                UserExtendedDataIdMap::const_iterator uedKeyByNameMapItr = mComponent->getGameManagerUserExtendedDataIdMap().find(dataName);
                if (uedKeyByNameMapItr == mComponent->getGameManagerUserExtendedDataIdMap().end())
                {
                    TRACE_LOG("[ExternalDataManager].decodeExternalData() could not find UEDKey in GameManagerUserExtendedDataIdMap for UEDKeyName("<< dataName <<").");
                    continue;
                }

                Blaze::UserExtendedDataKey uedKey = uedKeyByNameMapItr->second;

                EA::Json::JsonDomNode* jsonDomNode = domDocument.GetNode(outputPtr->getName());
                if (jsonDomNode != nullptr)
                {
                    switch (jsonDomNode->GetNodeType())
                    {
                    case EA::Json::EventType::kETInteger:
                    case EA::Json::EventType::kETDouble:
                        uedvalue = getScaledInt64(jsonDomNode, outputPtr->getScalingFactor());
                        break;
                    case EA::Json::EventType::kETBool:
                        {
                            uedvalue = (jsonDomNode->AsJsonDomBool()->mValue) ?  1 : 0;
                        }
                        break;
                    case EA::Json::EventType::kETString:
                        {
                            int64_t value = 0;
                            if (!convertNumberStringToInt(value, jsonDomNode->AsJsonDomString()->mValue.c_str(), outputPtr->getScalingFactor()))
                            {
                                // This means value partially has non-number values.
                                TRACE_LOG("[ExternalDataManager].decodeExternalData() error occured when attempting to convert json string(" << jsonDomNode->AsJsonDomString()->mValue.c_str() << ") to be used as a UED key ("
                                    << dataName << "). Default value will be used instead.");
                                convertNumberStringToInt(value, outputPtr->getDefaultValue(), outputPtr->getScalingFactor());
                            }
                            uedvalue = value;
                        }
                        break;
                    default:
                        {
                            TRACE_LOG("[ExternalDataManager].decodeExternalData() error occured unsupported type ("<< jsonDomNode->GetNodeType() <<") was used to set UED key (" << dataName << "). Default value will be used instead.");

                            // External data response does not have a valid - check to see if the user has an existing entry for the UED value. If they do, don't update it with default values
                            // If they do not, add an entry to their map with a default value
                            if (userSessionExtendedData.getDataMap().find(uedKey) == userSessionExtendedData.getDataMap().end())
                            {
                                int64_t defaultValue = 0;
                                convertNumberStringToInt(defaultValue, outputPtr->getDefaultValue(), outputPtr->getScalingFactor());
                                uedvalue = defaultValue;
                            }
                            else
                            {
                                continue;
                            }
                        }
                        break;
                    }
                }
                else
                {
                    // External data response does not have a valid - check to see if the user has an existing entry for the UED value. If they do, don't update it with default values
                    // If they do not, add an entry to their map with a default value

                    if (userSessionExtendedData.getDataMap().find(uedKey) == userSessionExtendedData.getDataMap().end())
                    {
                        int64_t defaultValue = 0;
                        convertNumberStringToInt(defaultValue, outputPtr->getDefaultValue(), outputPtr->getScalingFactor());
                        uedvalue = defaultValue;
                    }
                    else
                    {
                        continue;
                    }

                }

                for (auto& sessionId : userSessionIds)
                {
                    BlazeRpcError error = ERR_OK;
                    if (sessionId != INVALID_USER_SESSION_ID)
                    {
                        error = gUserSessionManager->updateExtendedData(sessionId, COMPONENT_ID_FROM_UED_KEY(uedKey), DATA_ID_FROM_UED_KEY(uedKey), uedvalue);
                    }
                    if (error != ERR_OK)
                    {
                        WARN_LOG("[ExternalDataManager].decodeExternalData updating UED value for keyid(" << uedKeyByNameMapItr->second << ") for GameManagerSlave componenet failed due to err(" << ErrorHelp::getErrorName(error)
                            << ") for usersession id " << sessionId);
                    }
                }
            }
            else
            {
                WARN_LOG("[ExternalDataManager].decodeExternalData unable to decode scenarioRequestOverrideKey(" << requestOverrideKey.c_str() << ") because type is invalid.");
            }
        }
    }

    bool ExternalDataManager::decodeExternalDataList(const char8_t* attr, TemplateAttributes& attributes, const char8_t* requestOverrideKey, const char8_t* listNodePath, const char8_t* elementNodePath, EA::Json::JsonDomDocument& domDocument, int64_t scalingFactor, bool mergeLists) const
    {
        TRACE_LOG("[ExternalDataManager].decodeExternalDataList"
            << "\n  list path: " << listNodePath
            << "\n  elem path: " << elementNodePath);

        auto attrIter = attributes.find(attr);
        if (attrIter == attributes.end())
        {
            attrIter = attributes.insert(eastl::make_pair(attr, attributes.allocate_element())).first;
        }

        // Note:  We don't actually care if the value uses the same data types in each key/value, as long as they're all primitives (int/double/bool/string):
        EA::Json::JsonDomArray* jsonDomArray = domDocument.GetArray(listNodePath);
        if (!jsonDomArray)
        {
            WARN_LOG("[ExternalDataManager].decodeExternalDataList: Unable to find JSON object entry for " << listNodePath << " mapped to attribute " << attr << ". Leave list as is.");
            return false;
        }

        // Check that the generic is set to a list:
        bool isAttributeValidList = (attrIter->second->isValid() && attrIter->second->get().getType() == EA::TDF::TDF_ACTUAL_TYPE_LIST);
        if (!isAttributeValidList)
        {
            if (mergeLists && attrIter->second->isValid())
            {
                WARN_LOG("[ExternalDataManager].decodeExternalDataList: Attempting to merge entry " << listNodePath << " on to attribute " << attr << " which is not a list.");
                return false;
            }
            else // Attempt to use convertToResults to add the values in:
            {
                // In this case, we either are not merging, or the data type was not set to a list yet.  (We set it to a string/string map here, since we don't know what type it really is.)
                EA::TDF::TdfPrimitiveVector<EA::TDF::TdfString> tdfList;
                attrIter->second->set(tdfList);
            }
        }


        for (auto& listNode : jsonDomArray->mJsonDomNodeArray)
        {
            if (!listNode)
                continue;

            EA::TDF::TdfGenericReference valueRef;

            // Insert the element:
            attrIter->second->get().asList().pullBackRef(valueRef);

            // Add the value:
            switch (listNode->GetNodeType())
            {
            case EA::Json::EventType::kETInteger:
            case EA::Json::EventType::kETDouble:
            {
                int64_t value = getScaledInt64(listNode, scalingFactor);

                EA::TDF::TdfGenericReference jsonValueRef(value);
                jsonValueRef.convertToResult(valueRef);
                TRACE_LOG("[ExternalDataManager].decodeExternalDataList: Added value " << value << " to list " << attr);
                break;
            }
            case EA::Json::EventType::kETBool:
            {
                bool value = listNode->AsJsonDomBool()->mValue;

                EA::TDF::TdfGenericReference jsonValueRef(value);
                jsonValueRef.convertToResult(valueRef);
                TRACE_LOG("[ExternalDataManager].decodeExternalDataList: Added value " << value << " to list " << attr);
                break;
            }
            case EA::Json::EventType::kETString:
            {
                EA::TDF::TdfString value = listNode->AsJsonDomString()->mValue.c_str();

                EA::TDF::TdfGenericReference jsonValueRef(value);
                jsonValueRef.convertToResult(valueRef);
                TRACE_LOG("[ExternalDataManager].decodeExternalDataList: Added value " << value << " to list " << attr);
                break;
            }
            default:
            {
                WARN_LOG("[ExternalDataManager].decodeExternalDataList unable to parse data from response for attribute (" << attr << ") because type("
                    << listNode->GetNodeType() << ") in list(" << listNodePath << ") is not supported");
                return false;
            }
            }
        
            // After inserting the node, we need to ensure that the value was not duplicated, when using the merge option:
            // Or maybe not?  We weren't doing that for strings previously, just the int/bool types, so it's unclear if that would be desired here.
        
            // IF THAT IS DESIRED FUNCTIONALITY, add eraseDuplicatesAsSet() as a virtual function on the TdfVectorBase (so it can call the TdfVectorPrimitives function), and call that.
        }

        return true;
    }

    // The expectation is that maps will be sent as a single object:
    //   { string : value, string : value }
    // Not in an object array:
    //   [ { string : value }, { string : value } ]
    bool ExternalDataManager::decodeExternalDataMap(const char8_t* attr, TemplateAttributes& attributes, const char8_t* mapNodePath, EA::Json::JsonDomDocument& domDocument, int64_t scalingFactor, bool mergeMaps) const
    {
        TRACE_LOG("[ExternalDataManager].decodeExternalDataMap"
            << "\n  map path: " << mapNodePath);

        auto attrIter = attributes.find(attr);
        if (attrIter == attributes.end())
        {
            attrIter = attributes.insert(eastl::make_pair(attr, attributes.allocate_element())).first;
        }
        
        // Note:  We don't actually care if the value uses the same data types in each key/value, as long as they're all primitives (int/double/bool/string):
        EA::Json::JsonDomObject* jsonDomObject = domDocument.GetObject(mapNodePath);
        if (!jsonDomObject)
        {
            WARN_LOG("[ExternalDataManager].decodeExternalDataMap: Unable to find JSON object entry for " << mapNodePath << " mapped to attribute "<< attr << ".");
            return false;
        }


        // Check that the generic is set to a map with key type string:  (we can support other key types later if needed)
        bool isAttributeValidStringMap = (attrIter->second->isValid() && attrIter->second->get().getType() == EA::TDF::TDF_ACTUAL_TYPE_MAP && attrIter->second->get().asMap().getKeyType() == EA::TDF::TDF_ACTUAL_TYPE_STRING);
        if (!isAttributeValidStringMap)
        {
            if (mergeMaps && attrIter->second->isValid())
            {
                WARN_LOG("[ExternalDataManager].decodeExternalDataMap: Attempting to merge entry " << mapNodePath << " on to attribute " << attr << " which is not a map with a string key (type = "<< attrIter->second->get().getFullName() <<").");
                return false;
            }
            else
            {
                // In this case, we either are not merging, or the data type was not set to a map yet.  (We set it to a string/string map here, since we don't know what type it really is.)
                EA::TDF::TdfPrimitiveMap<EA::TDF::TdfString, EA::TDF::TdfString> tdfMap;
                attrIter->second->set(tdfMap);
            }
        }
        // Attempt to use convertToResults to add the values in:

        for (auto& mapEntry : jsonDomObject->mJsonDomObjectValueArray)
        {
            if (!mapEntry.mpNode)
                continue;

            EA::TDF::TdfString key = mapEntry.mNodeName.c_str();
            EA::TDF::TdfGenericReferenceConst keyRef(key);
            EA::TDF::TdfGenericReference valueRef;

            // Insert the key:
            attrIter->second->get().asMap().insertKeyGetValue(keyRef, valueRef);

            // Add the value:
            switch (mapEntry.mpNode->GetNodeType())
            {
            case EA::Json::EventType::kETInteger:
            case EA::Json::EventType::kETDouble:
            {
                int64_t value = getScaledInt64(mapEntry.mpNode, scalingFactor);

                EA::TDF::TdfGenericReference jsonValueRef(value);
                jsonValueRef.convertToResult(valueRef);
                TRACE_LOG("[ExternalDataManager].decodeExternalDataMap: Added key " << key << " value " << value << " to list " << attr);
                break;
            }
            case EA::Json::EventType::kETBool:
            {
                bool value = mapEntry.mpNode->AsJsonDomBool()->mValue;

                EA::TDF::TdfGenericReference jsonValueRef(value);
                jsonValueRef.convertToResult(valueRef);
                TRACE_LOG("[ExternalDataManager].decodeExternalDataMap: Added key " << key << " value " << value << " to list " << attr);
                break;
            }
            case EA::Json::EventType::kETString:
            {
                EA::TDF::TdfString value = mapEntry.mpNode->AsJsonDomString()->mValue.c_str();

                EA::TDF::TdfGenericReference jsonValueRef(value);
                jsonValueRef.convertToResult(valueRef);
                TRACE_LOG("[ExternalDataManager].decodeExternalDataMap: Added key " << key << " value " << value << " to list " << attr);
                break;
            }
            default:
            {
                WARN_LOG("[ExternalDataManager].decodeExternalDataMap unable to parse data from response for attribute (" << attr << ") because type("
                          << mapEntry.mpNode->GetNodeType() << ") in map(" << mapNodePath << ") is not supported");
                return false;
            }
            }
        }
        return true;
    }

    void ExternalDataManager::validateAttributeNames(const GameManagerServerConfig& configTdf, const ExternalApiName& apiName, const ParamKey& key, const char8_t* paramName,
        const char8_t* paramDefaultValue, eastl::set<TemplateAttributeName>& templateAttributeNameSet, ConfigureValidationErrors& validationErrors)
    {
        StringBuilder strBuilder;
        if (paramName[0] == '\0')
        {
            strBuilder << "[ExternalDataManager].validateAttributeNames: failed to configure api(" << apiName << ")'s scenarioRequestOverrideKey (" << key << ") because no external data response mapping exists for equivalent matchmaking scenario request.";
            validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
        }

        char8_t* attr = nullptr;

        if ((attr = blaze_strnistr(key.c_str(), TEMPLATE_ATTRIBUTES_KEY, key.length())) != nullptr)
        {
            // ensure attribute exists
            attr += strlen(TEMPLATE_ATTRIBUTES_KEY);
            if (!isAttributeNameValid(attr, configTdf, templateAttributeNameSet))
            {
                strBuilder << "[ExternalDataManager].validateAttributeNames: failed to find attribute(" << attr << ") in list of global, scenario rules or game creation templates";
                validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
            }
        }
        else if (((attr = blaze_strnistr(key.c_str(), UED_KEY, key.length())) != nullptr))
        {
            // ensure that the type of attribute is a number, otherwise error out.
            int64_t defaultValue = 0;
            if (!convertNumberStringToInt(defaultValue, paramDefaultValue))
            {
                // This means value partially has non-number values.
                strBuilder << "[ExternalDataManager].validateAttributeNames: failed to configure requestOverrideKey(" << paramName << ") because default value(" << paramDefaultValue << ") isn't a number while it tried to store it in the UED.";
                validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
            }

            // ensure this key has been registered, if not error out.
            attr += strlen(UED_KEY);
            GameManagerUserExtendedDataByKeyIdMap::const_iterator uedKeyItr = configTdf.getGameManagerUserExtendedDataConfig().find(attr);
            if (uedKeyItr == configTdf.getGameManagerUserExtendedDataConfig().end())
            {
                strBuilder << "[ExternalDataManager].validateAttributeNames: failed to configure api(" << apiName << ")'s requestOverrideKey (" << paramName << ") to UED value (" << attr << ") because no UED value of this name has been registered under the game manager component.";
                validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
            }
        }
        else
        {
            strBuilder << "[ExternalDataManager].validateAttributeNames: failed to configure api(" << apiName << ")'s requestOverrideKey (" << paramName << ") to scenarioRequestOverrideKey (" << key << ") because an invalid type was specified.";
            validationErrors.getErrorMessages().push_back(strBuilder.get()); strBuilder.reset();
        }
    }

    bool ExternalDataManager::isAttributeNameValid(TemplateAttributeName templateAttribute, const GameManagerServerConfig& configTdf, eastl::set<TemplateAttributeName>& templateAttributeNameSet)
    {
        if (templateAttributeNameSet.empty())
        {
            //Scenario Attributes

            // Global Attributes
            for (auto& globalRuleMapItr : configTdf.getScenariosConfig().getGlobalRules())
            {
                for (auto& scenarioRuleAttributesItr : *globalRuleMapItr.second)
                {
                    addTemplateAttributesToTemplateAttributeSet(*(scenarioRuleAttributesItr.second), templateAttributeNameSet);
                }
            }

            // Non Global Attributes
            for (auto& scenarioRuleMapItr : configTdf.getScenariosConfig().getScenarioRuleMap())
            {
                for (auto& scenarioRuleAttributesItr : *scenarioRuleMapItr.second)
                {
                    addTemplateAttributesToTemplateAttributeSet(*(scenarioRuleAttributesItr.second), templateAttributeNameSet);
                }
            }

            // Create game attributes within Scenario Configuration
            for (auto& scenarioMapItr : configTdf.getScenariosConfig().getScenarios())
            {
                for (auto& subSessionConfigMapItr : scenarioMapItr.second->getSubSessions())
                {
                    if (subSessionConfigMapItr.second->getMatchmakingSettings().getSessionMode().getCreateGame())
                    {
                        addTemplateAttributesToTemplateAttributeSet(subSessionConfigMapItr.second->getCreateGameAttributes(), templateAttributeNameSet);
                    }
                }
            }

            // Create Game Template Attributes
            for (auto& createGameTemplateMapItr : configTdf.getCreateGameTemplatesConfig().getCreateGameTemplates())
            {
                addTemplateAttributesToTemplateAttributeSet(createGameTemplateMapItr.second->getAttributes(), templateAttributeNameSet);

                // Packer Quality Factor Attributes
                for (auto& qfIter : createGameTemplateMapItr.second->getPackerConfig().getQualityFactors())
                {
                    if (qfIter->getKeys().getAttrName()[0] != '\0')
                        templateAttributeNameSet.insert(qfIter->getKeys().getAttrName());
                    if (qfIter->getScoringMap().getAttrName()[0] != '\0')
                        templateAttributeNameSet.insert(qfIter->getScoringMap().getAttrName());

                    if (!qfIter->getScoreShaper().empty())
                        addTemplateAttributesToTemplateAttributeSet(*qfIter->getScoreShaper().begin()->second, templateAttributeNameSet);
                }
            }

            // Input Sanitizer Attributes?  (Currently prior to external data source lookup)

            // PropertyFilter Attributes
            for (auto& filterMapItr : configTdf.getFilters())
            {
                if (!filterMapItr.second->getRequirement().empty())
                    addTemplateAttributesToTemplateAttributeSet(*filterMapItr.second->getRequirement().begin()->second, templateAttributeNameSet);
            }

            // GameBrowser Scenario Attributes
            for (auto& gbScenarioMapItr : configTdf.getGameBrowserScenariosConfig().getScenarioRuleMap())
            {
                for (auto& gbScenarioRuleAttributesItr : *gbScenarioMapItr.second)
                {
                    addTemplateAttributesToTemplateAttributeSet(*(gbScenarioRuleAttributesItr.second), templateAttributeNameSet);
                }
            }
        }
   
        return (templateAttributeNameSet.find(templateAttribute) != templateAttributeNameSet.end());
    }

    void ExternalDataManager::addTemplateAttributesToTemplateAttributeSet(const TemplateAttributeMapping& templateAttributeMapping, eastl::set<TemplateAttributeName>& templateAttributeNameSet)
    {
        for (auto& templateAttributeMappingItr : templateAttributeMapping)
        {
            if (templateAttributeMappingItr.second->getAttrName()[0] != '\0')
            {
                templateAttributeNameSet.insert(templateAttributeMappingItr.second->getAttrName());
            }
        }
    }

    BlazeRpcError ExternalDataManager::getNucleusAccessToken(eastl::string& accessToken, OAuth::TokenType prefixType) 
    {
        UserSession::SuperUserPermissionAutoPtr ptr(true);
        OAuth::AccessTokenUtil tokenUtil;
        BlazeRpcError tokErr = tokenUtil.retrieveServerAccessToken(prefixType);
        if (tokErr == ERR_OK)
        {
            accessToken.sprintf("%s", tokenUtil.getAccessToken());
        }
        return tokErr;
    }

    bool ExternalDataManager::filloutGrpcRequest(const google::protobuf::Descriptor* d, const EA::TDF::TdfGenericReferenceConst& tdf, const char8_t* tdfMemberName, google::protobuf::Message* m,
        const google::protobuf::FieldDescriptor* fdOverride/* = nullptr*/)
    {
        bool ret = true;

        const char8_t* fieldName = (tdfMemberName ? tdfMemberName : "unknown");
        // For protobuf maps, we look up the key/value FieldDesriptors by index and pass them in
        const google::protobuf::FieldDescriptor* fd = fdOverride;
        if (fd == nullptr && tdfMemberName != nullptr)
        {
            fd = d->FindFieldByName(tdfMemberName);
        }

        if (fd == nullptr)
        {
            TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: No field descriptor found or provided for (" << fieldName << ")!");
            return false;
        }

        // Based on the request field type, attempt to fetch the attribute as that type
        switch (fd->cpp_type())
        {
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        {
            if (fd->is_repeated())
            {
                if (tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_LIST)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected list for (" << fieldName << "), but " << tdf.getFullName() << " was found at line " << __LINE__);
                    return false;
                }

                const EA::TDF::TdfVectorBase& ref = tdf.asList();
                if (ref.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_BOOL)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected value type bool for (" << fieldName << "), but " << ref.getValueTypeDesc().getFullName() << " was found.");
                    return false;
                }

                for (uint32_t i = 0; i < ref.vectorSize(); ++i)
                {
                    EA::TDF::TdfGenericReferenceConst val;
                    ref.getValueByIndex(i, val);
                    m->GetReflection()->AddBool(m, fd, val.asBool());
                }
            }
            else
            {
                if (tdf.isTypeString())
                {
                    bool val;
                    EA::TDF::TdfGenericReference ref(val);
                    if (!tdf.convertFromString(ref))
                    {
                        TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Failed to convert TDF from string to bool for (" << fieldName << ").");
                        return false;
                    }
                    m->GetReflection()->SetBool(m, fd, ref.asBool());
                }
                else if (tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_BOOL)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected type bool for (" << fieldName << "), but " << tdf.getFullName() << " was found.");
                    return false;
                }
                else
                    m->GetReflection()->SetBool(m, fd, tdf.asBool());
            }
            break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        {
            if (fd->is_repeated())
            {
                if (tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_LIST)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected list for (" << fieldName << "), but " << tdf.getFullName() << " was found at line " << __LINE__);
                    return false;
                }

                const EA::TDF::TdfVectorBase& ref = tdf.asList();
                if (ref.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_FLOAT)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected value type float for (" << fieldName << "), but " << ref.getValueTypeDesc().getFullName() << " was found.");
                    return false;
                }

                for (uint32_t i = 0; i < ref.vectorSize(); ++i)
                {
                    EA::TDF::TdfGenericReferenceConst val;
                    ref.getValueByIndex(i, val);
                    m->GetReflection()->AddDouble(m, fd, (double)val.asFloat());
                }
            }
            else
            {
                if (tdf.isTypeString())
                {
                    float val;
                    EA::TDF::TdfGenericReference ref(val);
                    if (!tdf.convertFromString(ref))
                    {
                        TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Failed to convert TDF from string to float for (" << fieldName << ").");
                        return false;
                    }
                    m->GetReflection()->SetDouble(m, fd, (double)ref.asFloat());
                }
                else if (tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_FLOAT)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected type float for (" << fieldName << "), but " << tdf.getFullName() << " was found.");
                    return false;
                }
                else
                    m->GetReflection()->SetDouble(m, fd, (double)tdf.asFloat());
            }
            break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        {
            if (fd->is_repeated())
            {
                if (tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_LIST)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected list for (" << fieldName << "), but " << tdf.getFullName() << " was found at line " << __LINE__);
                    return false;
                }

                const EA::TDF::TdfVectorBase& ref = tdf.asList();
                if (ref.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_FLOAT)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected value type float for (" << fieldName << "), but " << ref.getValueType() << " was found.");
                    return false;
                }

                for (uint32_t i = 0; i < ref.vectorSize(); ++i)
                {
                    EA::TDF::TdfGenericReferenceConst val;
                    ref.getValueByIndex(i, val);
                    m->GetReflection()->AddFloat(m, fd, val.asFloat());
                }
            }
            else
            {
                if (tdf.isTypeString())
                {
                    float val;
                    EA::TDF::TdfGenericReference ref(val);
                    if (!tdf.convertFromString(ref))
                    {
                        TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Failed to convert TDF from string to float for (" << fieldName << ").");
                        return false;
                    }
                    m->GetReflection()->SetFloat(m, fd, ref.asFloat());
                }
                else if (tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_FLOAT)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected type float for (" << fieldName << "), but " << tdf.getFullName() << " was found.");
                    return false;
                }
                else
                    m->GetReflection()->SetFloat(m, fd, tdf.asFloat());
            }
            break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        {
            const google::protobuf::EnumDescriptor* ed = fd->enum_type();
            if (fd->is_repeated())
            {
                if (tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_LIST)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected list for (" << fieldName << "), but " << tdf.getFullName() << " was found at line " << __LINE__);
                    return false;
                }

                const EA::TDF::TdfVectorBase& ref = tdf.asList();
                if (ref.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_ENUM)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected value type enum for (" << fieldName << "), but " << ref.getValueTypeDesc().getFullName() << " was found.");
                    return false;
                }

                for (uint32_t i = 0; i < ref.vectorSize(); ++i)
                {
                    EA::TDF::TdfGenericReferenceConst val;
                    ref.getValueByIndex(i, val);
                    const google::protobuf::EnumValueDescriptor* evd = ed->FindValueByNumber(val.asEnum());
                    if (evd == nullptr)
                        return false;
                    m->GetReflection()->AddEnum(m, fd, evd);
                }
            }
            else
            {
                if (tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_ENUM)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected type enum for (" << fieldName << "), but " << tdf.getFullName() << " was found.");
                    return false;
                }
                const google::protobuf::EnumValueDescriptor* evd = ed->FindValueByNumber(tdf.asEnum());
                if (evd == nullptr)
                    return false;
                m->GetReflection()->SetEnum(m, fd, evd);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        {
            if (fd->is_repeated())
            {
                if (tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_LIST)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected list for (" << fieldName << "), but " << tdf.getFullName() << " was found at line " << __LINE__);
                    return false;
                }

                const EA::TDF::TdfVectorBase& ref = tdf.asList();
                if (ref.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_INT32
                    && ref.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_INT16
                    && ref.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_INT8)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected value type int32_t for (" << fieldName << "), but " << ref.getValueTypeDesc().getFullName() << " was found.");
                    return false;
                }

                for (uint32_t i = 0; i < ref.vectorSize(); ++i)
                {
                    EA::TDF::TdfGenericReferenceConst val;
                    ref.getValueByIndex(i, val);
                    m->GetReflection()->AddInt32(m, fd, val.asInt32());
                }
            }
            else
            {
                if (tdf.isTypeString())
                {
                    int32_t val;
                    EA::TDF::TdfGenericReference ref(val);
                    if (!tdf.convertFromString(ref))
                    {
                        TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Failed to convert TDF from string to int32_t for (" << fieldName << ").");
                        return false;
                    }
                    m->GetReflection()->SetInt32(m, fd, ref.asInt32());
                }
                else if (tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_INT32
                    && tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_INT16
                    && tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_INT8)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected type int32_t for (" << fieldName << "), but " << tdf.getFullName() << " was found.");
                    return false;
                }
                else
                    m->GetReflection()->SetInt32(m, fd, tdf.asInt32());
            }
            break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        {
            if (fd->is_repeated())
            {
                if (tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_LIST)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected list for (" << fieldName << "), but " << tdf.getFullName() << " was found at line " << __LINE__);
                    return false;
                }

                const EA::TDF::TdfVectorBase& ref = tdf.asList();
                if (ref.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_INT64
                    && ref.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_INT32
                    && ref.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_INT16
                    && ref.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_INT8)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected value type int64_t for (" << fieldName << "), but " << ref.getValueTypeDesc().getFullName() << " was found.");
                    return false;
                }

                for (uint32_t i = 0; i < ref.vectorSize(); ++i)
                {
                    EA::TDF::TdfGenericReferenceConst val;
                    ref.getValueByIndex(i, val);
                    m->GetReflection()->AddInt64(m, fd, val.asInt64());
                }
            }
            else
            {
                if (tdf.isTypeString())
                {
                    int64_t val;
                    EA::TDF::TdfGenericReference ref(val);
                    if (!tdf.convertFromString(ref))
                    {
                        TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Failed to convert TDF from string to int64_t for (" << fieldName << ").");
                        return false;
                    }
                    m->GetReflection()->SetInt64(m, fd, ref.asInt64());
                }
                else if (tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_INT64
                    && tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_INT32
                    && tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_INT16
                    && tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_INT8)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected type int64_t for (" << fieldName << "), but " << tdf.getFullName() << " was found.");
                    return false;
                }
                else
                    m->GetReflection()->SetInt64(m, fd, tdf.asInt64());
            }
            break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
        {
            if (fd->is_map())
            {
                if (tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_MAP)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected map for (" << fieldName << "), but " << tdf.getFullName() << " was found at line " << __LINE__);
                    return false;
                }

                const EA::TDF::TdfMapBase& ref = tdf.asMap();
                google::protobuf::MutableRepeatedFieldRef<google::protobuf::Message> subm =
                    m->GetReflection()->GetMutableRepeatedFieldRef<google::protobuf::Message>(m, fd);
                for (uint32_t i = 0; i < ref.mapSize(); ++i)
                {
                    std::unique_ptr<google::protobuf::Message> entry(subm.NewMessage());
                    const google::protobuf::FieldDescriptor* keyd =
                        entry->GetDescriptor()->FindFieldByNumber(1); // Map key
                    const google::protobuf::FieldDescriptor* vald =
                        entry->GetDescriptor()->FindFieldByNumber(2); // Map value

                    EA::TDF::TdfGenericReferenceConst key;
                    EA::TDF::TdfGenericReferenceConst val;
                    ref.getValueByIndex(i, key, val);
                    // Keys can only be strings or integral types (except float/enum)
                    ret = filloutGrpcRequest(entry->GetDescriptor(), key, nullptr, entry.get(), keyd);
                    if (!ret)
                        return ret;
                    // Value can be anything except another map/list/oneof
                    ret = filloutGrpcRequest(entry->GetDescriptor(), val, nullptr, entry.get(), vald);
                    if (!ret)
                        return ret;

                    subm.Add(*entry);
                }
            }
            else
            {
                google::protobuf::Message* subm = m->GetReflection()->GetMessage(*m, fd).New();
                // Regular message, so process its members
                switch (tdf.getType())
                {
                case EA::TDF::TDF_ACTUAL_TYPE_TDF:
                {
                    const EA::TDF::Tdf& ref = tdf.asTdf();
                    EA::TDF::TdfMemberInfoIterator infoItr(ref);
                    while (infoItr.next())
                    {
                        EA::TDF::TdfGenericReferenceConst val;
                        ref.getValueByTag(infoItr.getTag(), val);
                        ret = filloutGrpcRequest(subm->GetDescriptor(), val, infoItr.getInfo()->getMemberName(), subm);
                        if (!ret)
                            return ret;
                    }
                    break;
                }
                case EA::TDF::TDF_ACTUAL_TYPE_VARIABLE:
                {
                    const EA::TDF::VariableTdfBase& ref = tdf.asVariable();
                    EA::TDF::TdfMemberInfoIterator infoItr(*ref.get());
                    while (infoItr.next())
                    {
                        EA::TDF::TdfGenericReferenceConst val;
                        ref.get()->getValueByTag(infoItr.getTag(), val);
                        ret = filloutGrpcRequest(subm->GetDescriptor(), val, infoItr.getInfo()->getMemberName(), subm);
                        if (!ret)
                            return ret;
                    }
                    break;
                }
                case EA::TDF::TDF_ACTUAL_TYPE_GENERIC_TYPE:
                {
                    ASSERT_LOG("Reference TDF is of type generic, which can not be decoded!");
                    return false;
                }
                default:
                {
                    // protobuf supports Well-Known Types, which act as a form of boxing.
                    // If we get a Message type and the corresponding TDF type isn't a TDF or VariableTDF
                    // Go one level deeper after getting the underlying protobuf type
                    ret = filloutGrpcRequest(subm->GetDescriptor(), tdf, tdfMemberName, subm);
                    if (!ret)
                        return ret;
                    break;
                }
                }
                m->GetReflection()->SetAllocatedMessage(m, subm, fd);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        {
            if (fd->is_repeated())
            {
                if (tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_LIST)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected list for (" << fieldName << "), but " << tdf.getFullName() << " was found at line " << __LINE__);
                    return false;
                }

                const EA::TDF::TdfVectorBase& ref = tdf.asList();
                if (ref.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_STRING)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected value type string for (" << fieldName << "), but " << ref.getValueTypeDesc().getFullName() << " was found.");
                    return false;
                }

                for (uint32_t i = 0; i < ref.vectorSize(); ++i)
                {
                    EA::TDF::TdfGenericReferenceConst val;
                    ref.getValueByIndex(i, val);
                    m->GetReflection()->AddString(m, fd, val.asString().c_str());
                }
            }
            else
            {
                if (tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_STRING)
                {
                    EA::TDF::TdfString val;
                    EA::TDF::TdfGenericReference ref(val);
                    if (!tdf.convertToString(ref))
                    {
                        TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected type string for (" << fieldName << "), but " << tdf.getFullName() << " was found and couldn't be converted to string.");
                        return false;
                    }
                    m->GetReflection()->SetString(m, fd, ref.asString().c_str());
                }
                m->GetReflection()->SetString(m, fd, tdf.asString().c_str());
            }
            break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        {
            if (fd->is_repeated())
            {
                if (tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_LIST)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected list for (" << fieldName << "), but " << tdf.getFullName() << " was found at line " << __LINE__);
                    return false;
                }

                const EA::TDF::TdfVectorBase& ref = tdf.asList();
                if (ref.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_UINT64
                    && ref.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_UINT32
                    && ref.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_UINT16
                    && ref.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_UINT8)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected value type uint32_t for (" << fieldName << "), but " << ref.getValueTypeDesc().getFullName() << " was found.");
                    return false;
                }

                for (uint32_t i = 0; i < ref.vectorSize(); ++i)
                {
                    EA::TDF::TdfGenericReferenceConst val;
                    ref.getValueByIndex(i, val);
                    m->GetReflection()->AddUInt32(m, fd, val.asUInt32());
                }
            }
            else
            {
                if (tdf.isTypeString())
                {
                    uint32_t val;
                    EA::TDF::TdfGenericReference ref(val);
                    if (!tdf.convertFromString(ref))
                    {
                        TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Failed to convert TDF from string to uint32_t for (" << fieldName << ").");
                        return false;
                    }
                    m->GetReflection()->SetUInt32(m, fd, ref.asUInt32());
                }
                else if (tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_UINT32
                    && tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_UINT16
                    && tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_UINT8)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected type uint32_t for (" << fieldName << "), but " << tdf.getFullName() << " was found.");
                    return false;
                }
                else
                    m->GetReflection()->SetUInt32(m, fd, tdf.asUInt32());
            }
            break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        {
            if (fd->is_repeated())
            {
                if (tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_LIST)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected list for (" << fieldName << "), but " << tdf.getFullName() << " was found at line " << __LINE__);
                    return false;
                }

                const EA::TDF::TdfVectorBase& ref = tdf.asList();
                if (ref.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_UINT64
                    && ref.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_UINT32
                    && ref.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_UINT16
                    && ref.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_UINT8)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected value type uint64_t for (" << fieldName << "), but " << ref.getValueTypeDesc().getFullName() << " was found.");
                    return false;
                }

                for (uint32_t i = 0; i < ref.vectorSize(); ++i)
                {
                    EA::TDF::TdfGenericReferenceConst val;
                    ref.getValueByIndex(i, val);
                    m->GetReflection()->AddUInt64(m, fd, val.asUInt64());
                }
            }
            else
            {
                if (tdf.isTypeString())
                {
                    uint64_t val;
                    EA::TDF::TdfGenericReference ref(val);
                    if (!tdf.convertFromString(ref))
                    {
                        TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Failed to convert TDF from string to uint64_t for (" << fieldName << ").");
                        return false;
                    }
                    m->GetReflection()->SetUInt64(m, fd, ref.asUInt64());
                }
                else if (tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_UINT64
                    && tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_UINT32
                    && tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_UINT16
                    && tdf.getType() != EA::TDF::TDF_ACTUAL_TYPE_UINT8)
                {
                    TRACE_LOG("[ExternalDataManager].filloutGrpcRequest: Request expected type uint64_t for (" << fieldName << "), but " << tdf.getFullName() << " was found.");
                    return false;
                }
                else
                    m->GetReflection()->SetUInt64(m, fd, tdf.asUInt64());
            }
            break;
        }
        }

        return ret;
    }

    bool ExternalDataManager::filloutGrpcClientContext(const HeadersMap& headersMap, const TemplateAttributes& attributes, PerPlayerJoinDataPtr perPlayerJoinData, grpc::ClientContext& context)
    {
        for (auto& itr : headersMap)
        {
            ParamKey headerName = itr.first;
            ExternalDataSourceParameterPtr headerParamPtr = itr.second;
            StringBuilder str;
            if (headerParamPtr->getName()[0] != '\0')
            {
                TemplateAttributes::const_iterator attrItr = attributes.find(headerParamPtr->getName());
                if (attrItr != attributes.end())
                {
                    auto& attr = attrItr->second->get();
                    // Check request to see if client sent up a value for this uri parameter via the request's attribute map
                    if (!attr.isTypeString())
                    {
                        // if attribute is not a string, we need to convert it to one. Create a temp generic of type string to store converted value in.
                        EA::TDF::TdfString val;
                        EA::TDF::TdfGenericReference criteriaAttr(val);
                        if (!attr.convertToString(criteriaAttr))
                        {
                            TRACE_LOG("[ExternalDataManager].filloutGrpcClientContext: Failed to convert header attribute of type (" << attr.getFullName() << ") to string!");
                            return false;
                        }

                        str << criteriaAttr.asString().c_str();
                    }
                    else
                    {
                        str << attr.asString().c_str();
                    }
                }
            }
            else if (headerParamPtr->getDefaultValue()[0] != '\0')
            {
                if ((blaze_stricmp(headerParamPtr->getDefaultValue(), BLAZE_ID_KEY)) == 0 && (perPlayerJoinData != nullptr) && (perPlayerJoinData->getUser().getBlazeId() != INVALID_BLAZE_ID))
                {
                    str << perPlayerJoinData->getUser().getBlazeId();
                }
                else if ((blaze_stricmp(headerParamPtr->getDefaultValue(), ACCOUNT_ID_KEY) == 0) && (perPlayerJoinData != nullptr) && (perPlayerJoinData->getUser().getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID))
                {
                    str << perPlayerJoinData->getUser().getPlatformInfo().getEaIds().getNucleusAccountId();
                }
                else if (blaze_stricmp(headerParamPtr->getDefaultValue(), S2S_ACCESS_TOKEN_KEY) == 0)
                {
                    eastl::string accessToken;
                    BlazeRpcError error = getNucleusAccessToken(accessToken, OAuth::TOKEN_TYPE_NEXUS_S2S);
                    if (error != ERR_OK)
                    {
                        ERR_LOG("[ExternalDataManager].filloutGrpcClientContext failed to fetch access token for header(" << headerName << ") due to error(" << ErrorHelp::getErrorName(error) << ")");
                        return false;
                    }
                    else
                    {
                        str << accessToken;
                    }
                }
                else if (blaze_stricmp(headerParamPtr->getDefaultValue(), S2S_ACCESS_TOKEN_BEARER_PREFIX_KEY) == 0)
                {
                    // Require access token with Bearer prefix, instead of NEXUS_S2S
                    eastl::string accessToken;
                    BlazeRpcError error = getNucleusAccessToken(accessToken, OAuth::TOKEN_TYPE_BEARER);
                    if (error != ERR_OK)
                    {
                        ERR_LOG("[ExternalDataManager].filloutGrpcClientContext failed to fetch bearer access token for header(" << headerName << ") due to error(" << ErrorHelp::getErrorName(error) << ")");
                        return false;
                    }
                    else
                    {
                        str << accessToken;
                    }
                }
                else
                {
                    str << headerParamPtr->getDefaultValue();
                }
            }

            context.AddMetadata(headerName.c_str(), str.get());
        }

        return true;
    }

    // Until we have code-gen support for instantiating outbound gRPC objects, this factory method will fill in
    // If configuring a new gRPC external data source, this method must be updated to create the correct outbound gRPC object
    Grpc::OutboundRpcBasePtr ExternalDataManager::createOutboundRpc(const ExternalDataSourceGrpcDefinition& externalDataSourceGrpcDefinition)
    {
        switch (externalDataSourceGrpcDefinition.getCommandType())
        {
        case ExternalDataSourceGrpcDefinition::UNARY:
        {
            if (blaze_stricmp(externalDataSourceGrpcDefinition.getService(), "eadp::stats::EntityStatistics") == 0)
            {
                if (blaze_stricmp(externalDataSourceGrpcDefinition.getCommand(), "AsyncGetStatsUnary") == 0)
                    return CREATE_GRPC_UNARY_RPC(externalDataSourceGrpcDefinition.getExternalDataSource(), eadp::stats::EntityStatistics, AsyncGetStatsUnary);
            }
            break;
        }
        case ExternalDataSourceGrpcDefinition::CLIENT:
            break;
        case ExternalDataSourceGrpcDefinition::SERVER:
            break;
        case ExternalDataSourceGrpcDefinition::BIDIRECTIONAL:
            break;
        default:
            ASSERT_LOG("Unknown gRPC command type specified. Was a new type added without including it here?");
            break;
        }

        return Grpc::OutboundRpcBasePtr();
    }

} // namespace GameManager
} // namespace Blaze