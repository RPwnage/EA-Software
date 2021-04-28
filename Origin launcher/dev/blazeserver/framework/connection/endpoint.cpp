/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Endpoint

    Encapsulates all protocol, encoder and decoder objects for a single endpoint.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/endpoint.h"
#include "framework/connection/endpointhelper.h"
#include "framework/component/component.h"
#include "framework/util/shared/blazestring.h"
#include "framework/connection/inetaddress.h"
#include "framework/connection/serversocketchannel.h"
#include "framework/connection/serverchannelfactory.h"
#include "framework/connection/nameresolver.h"
#include "framework/connection/ratelimiter.h"
#include "framework/connection/sslcontext.h"
#include "framework/protocol/shared/encoderfactory.h"
#include "framework/protocol/shared/decoderfactory.h"
#include "framework/protocol/protocolfactory.h"
#include "framework/connection/socketutil.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/tdf/controllertypes_server.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


// Minimum value that can be set for the CFG_QUEUED_OUTPUT_DATA_MAX config setting
static const uint32_t QUEUED_OUTPUT_DATA_MAX_MINIMUM = 2048;

/*** Public Methods ******************************************************************************/
Endpoint::Endpoint(const char8_t* name, const char8_t* protocol, const char8_t* encoder, const char8_t* decoder, uint32_t maxFrameSize)
    : Endpoint(name)
{
    if (protocol != nullptr)
    {
        mConfig.setProtocol(protocol);
        mProtocolType = ProtocolFactory::getProtocolType(getProtocolName());
    }

    if (encoder != nullptr)
    {
        mConfig.setEncoder(encoder);
        mEncoderType = EncoderFactory::getEncoderType(getEncoderName());
    }

    if (decoder != nullptr)
    {
        mConfig.setDecoder(decoder);
        mDecoderType = DecoderFactory::getDecoderType(getDecoderName());
    }

    mConfig.setMaxFrameSize(maxFrameSize);
}

Endpoint::Endpoint(const char8_t* name)
    : mChannel(nullptr)
    , mProtocolType(Protocol::INVALID)
    , mEncoderType(Encoder::INVALID)
    , mDecoderType(Decoder::INVALID)
    , mServerAddress(nullptr)
    , mTrustedNetworks(nullptr)
    , mInternalNicAccessNetworks(nullptr)
    , mMetricsCollection(Metrics::gFrameworkCollection->getCollection(Metrics::Tag::endpoint, name))
    , mConnections(mMetricsCollection, "endpoint.connections")
    , mCountTotalRejectsMaxRateLimit(mMetricsCollection, "endpoint.rejectsMaxRateLimit")
    , mCountTotalRejects(mMetricsCollection, "endpoint.rejects")
    , mHighWatermarks(mMetricsCollection, "endpoint.currentHighWatermarks")
    , mTotalHighWatermarks(mMetricsCollection, "endpoint.highWatermarks")
    , mTotalExceededDataCloses(mMetricsCollection, "endpoint.exceededDataCloses")
    , mTotalResumedConnections(mMetricsCollection, "endpoint.resumedConnections")
    , mTotalResumedConnectionsFailed(mMetricsCollection, "endpoint.resumedConnectionsFailed")
    , mTotalExceededRpcFiberLimitDropped(mMetricsCollection, "endpoint.exceededRpcFiberLimitDropped")
    , mTotalExceededRpcFiberLimitPermitted(mMetricsCollection, "endpoint.exceededRpcFiberLimitPermitted")
    , mTotalQueuedOutputBytes(mMetricsCollection, "endpoint.queuedOutputBytes")
    , mTotalQueuedAsyncBytes(mMetricsCollection, "endpoint.queuedAsyncBytes")
    , mTotalRecvBytes(mMetricsCollection, "endpoint.recvBytes")
    , mTotalRecvCount(mMetricsCollection, "endpoint.recvCount")
    , mTotalSendBytes(mMetricsCollection, "endpoint.sendBytes")
    , mTotalSendCount(mMetricsCollection, "endpoint.sendCount")
    , mEndpointName(blaze_strdup(name))
    , mSquelchingEnabled(false)
    , mRpcAuthorizer(mMetricsCollection, "endpoint.rpcAuthorizationFailureCount")
{
}

Endpoint::~Endpoint()
{
    BLAZE_FREE(mEndpointName);
    delete mServerAddress;
    delete mChannel;
    delete mTrustedNetworks;
    delete mInternalNicAccessNetworks;
}

bool Endpoint::initialize(const EndpointConfig::Endpoint& config, const RateConfig& rateConfig, const EndpointConfig::CommandListByNameMap& rpcControlLists, const EndpointConfig::PlatformSpecificControlListsMap& platformSpecificControlLists)
{
    config.copyInto(mConfig);

    mProtocolType = ProtocolFactory::getProtocolType(getProtocolName());

    mEncoderType = EncoderFactory::getEncoderType(getEncoderName());

    mDecoderType = DecoderFactory::getDecoderType(getDecoderName());


    mTrustedNetworks = BLAZE_NEW InetFilter();
    if (!mTrustedNetworks->initialize(true /*allowIfEmpty*/, mConfig.getTrust()))
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[Endpoint].initialize: invalid trust configuration for endpoint '" << getName() << "'");
        return false;
    }

    mInternalNicAccessNetworks = BLAZE_NEW InetFilter();
    if (!mInternalNicAccessNetworks->initialize(false /*allowIfEmpty*/, mConfig.getHasInternalNicAccess()))
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[Endpoint].initialize: invalid hasInternalNicAccess configuration for endpoint '" << getName() << "'");
        return false;
    }

    if (mConfig.getRateLimiter()[0] != '\0')
    {
        RateConfig::RateLimitersMap::const_iterator rateLimiterIt = rateConfig.getRateLimiters().find(mConfig.getRateLimiter());
        if (!mRateLimiter.initialize(rateLimiterIt->first.c_str(), *rateLimiterIt->second))
        {
            BLAZE_ERR_LOG(Log::CONNECTION, "[Endpoint].initialize: Failed to initialize RateLimiter '" << mConfig.getRateLimiter() << "' "
                "for endpoint '" << getName() << "'");
            return false;
        }
    }

    configureQueuedOutputData(mConfig.getQueuedOutputData());

    if (gSslContextManager->get(mConfig.getSslContext()) == nullptr)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[Endpoint].initialize: SSL context '" << mConfig.getSslContext() << "' not found for endpoint '" << getName() << "'");
            return false;
    }

    mRpcAuthorizer.initializeRpcAuthorizationList(config.getRpcWhiteList(), config.getRpcBlackList(), rpcControlLists, platformSpecificControlLists);

    return true;
}

void Endpoint::validateConfig(const EndpointConfig& config, const EndpointConfig* oldConfig, const char8_t* endpointGroup, const RateConfig& rateConfig, ConfigureValidationErrors& validationErrors)
{
    if (oldConfig == nullptr)
    {
        validateEndpointConfig(config, endpointGroup, rateConfig, validationErrors);
    }
    else
    {
        bool validPlatformSpecificControlLists = RpcAuthorizer::validatePlatformSpecificRpcControlLists(config, validationErrors);
        RpcAuthorizer::CommandIdsByComponentIdByControlListMap controlListMap(BlazeStlAllocator("Endpoint::controlListMap"));
        RpcAuthorizer::RpcControlListComboMap controlListComboMap(BlazeStlAllocator("Endpoint::controlListComboMap"));

        // *RpcConnection endpoints
        {
            for (const auto& oldEndpoint : oldConfig->getEndpointTypes())
            {
                const auto& itNewEndpoint = config.getEndpointTypes().find(oldEndpoint.first);
                if (itNewEndpoint == config.getEndpointTypes().end())
                {
                    FixedString64 msg(FixedString64::CtorSprintf(), "[Endpoint].validateConfig() : Endpoint %s not found in new configuration", oldEndpoint.first.c_str());
                    validationErrors.getErrorMessages().push_back().set(msg.c_str());
                }
                // Channel type is not reconfigurable. We use the new endpoint's channel type when validating ssl context,
                // so make sure that the new channel type matches the old.
                else if (blaze_stricmp(itNewEndpoint->second->getChannel(), oldEndpoint.second->getChannel()) != 0)
                {
                    FixedString64 msg(FixedString64::CtorSprintf(), "[Endpoint].validateConfig() : Attempt to change channel type of endpoint %s (from \"%s\" to \"%s\")",
                        itNewEndpoint->first.c_str(), oldEndpoint.second->getChannel(), itNewEndpoint->second->getChannel());
                    validationErrors.getErrorMessages().push_back().set(msg.c_str());
                }
            }

            for (const auto& newEndpoint : config.getEndpointTypes())
            {
                validateSslContext(*(newEndpoint.second), newEndpoint.first.c_str(), validationErrors);
                if (validPlatformSpecificControlLists)
                    RpcAuthorizer::validateRpcControlLists(config, newEndpoint.first.c_str(), (*newEndpoint.second).getRpcWhiteList(), (*newEndpoint.second).getRpcBlackList(), controlListMap, controlListComboMap, validationErrors);
            }
        }

        // Grpc Endpoints (There is some code duplication here from above but keeping it separate as the two endpoint types are likely to evolve differently)
        {
            for (const auto& oldEndpoint : oldConfig->getGrpcEndpointTypes())
            {
                const auto& itNewEndpoint = config.getGrpcEndpointTypes().find(oldEndpoint.first);
                if (itNewEndpoint == config.getGrpcEndpointTypes().end())
                {
                    FixedString64 msg(FixedString64::CtorSprintf(), "[Endpoint].validateConfig() : GrpcEndpoint %s not found in new configuration", oldEndpoint.first.c_str());
                    validationErrors.getErrorMessages().push_back().set(msg.c_str());
                }
                // Channel type is not reconfigurable. We use the new endpoint's channel type when validating ssl context,
                // so make sure that the new channel type matches the old.
                else if (blaze_stricmp(itNewEndpoint->second->getChannel(), oldEndpoint.second->getChannel()) != 0)
                {
                    FixedString64 msg(FixedString64::CtorSprintf(), "[Endpoint].validateConfig() : Attempt to change channel type of endpoint %s (from \"%s\" to \"%s\")",
                        itNewEndpoint->first.c_str(), oldEndpoint.second->getChannel(), itNewEndpoint->second->getChannel());
                    validationErrors.getErrorMessages().push_back().set(msg.c_str());
                }
            }

            for (const auto& newEndpoint : config.getGrpcEndpointTypes())
            {
                validateSslContext(*(newEndpoint.second), newEndpoint.first.c_str(), validationErrors);
                if (validPlatformSpecificControlLists)
                    RpcAuthorizer::validateRpcControlLists(config, newEndpoint.first.c_str(), (*newEndpoint.second).getRpcWhiteList(), (*newEndpoint.second).getRpcBlackList(), controlListMap, controlListComboMap, validationErrors);
            }
        }
    }
}

void Endpoint::validateEndpointConfig(const EndpointConfig& config, const char8_t* endpointGroup, const RateConfig& rateConfig, ConfigureValidationErrors& validationErrors)
{
    EndpointConfig::EndpointGroupsMap::const_iterator group = config.getEndpointGroups().find(endpointGroup);
    if (group == config.getEndpointGroups().end())
    {
        FixedString64 msg(FixedString64::CtorSprintf(),"[Endpoint].validateEndpointConfig() : Undefined endpoint group %s specified for instance.", endpointGroup);
        validationErrors.getErrorMessages().push_back().set(msg.c_str());
    }
    else
    {
        bool validPlatformSpecificControlLists = RpcAuthorizer::validatePlatformSpecificRpcControlLists(config, validationErrors);
        RpcAuthorizer::CommandIdsByComponentIdByControlListMap controlListMap(BlazeStlAllocator("Endpoint::controlListMap"));
        RpcAuthorizer::RpcControlListComboMap controlListComboMap(BlazeStlAllocator("Endpoint::controlListComboMap"));

        const EndpointConfig::EndpointGroup::StringEndpointsList& names = group->second->getEndpoints();
        if (names.empty())
        {
            FixedString64 msg(FixedString64::CtorSprintf(),"[Endpoint].validateEndpointConfig() : Empty endpoint group %s specified for instance.", endpointGroup);
            validationErrors.getErrorMessages().push_back().set(msg.c_str());
        }
        else
        {
            for (EndpointConfig::EndpointGroup::StringEndpointsList::const_iterator name = names.begin(), ename = names.end(); name != ename; ++name)
            {
                EndpointConfig::EndpointTypesMap::const_iterator itr = config.getEndpointTypes().find(*name);
                
                if (itr == config.getEndpointTypes().end())
                {
                    BLAZE_WARN_LOG(Log::CONNECTION, "[Endpoint].validateEndpointConfig() : Undefined endpoint " << name->c_str() 
                        << " specified in endpoint group " << endpointGroup << ", skipping.");
                }
                else
                {
                    if (ProtocolFactory::getProtocolType(itr->second->getProtocol()) == Protocol::INVALID)
                    {
                        FixedString64 msg(FixedString64::CtorSprintf(), "[Endpoint].validateEndpointConfig() : Invalid protocol name '%s' for endpoint '%s'", itr->second->getProtocol(), itr->first.c_str());
                        validationErrors.getErrorMessages().push_back().set(msg.c_str());
                    }

                    if (EncoderFactory::getEncoderType(itr->second->getEncoder()) == Encoder::INVALID)
                    {
                        FixedString64 msg(FixedString64::CtorSprintf(), "[Endpoint].validateEndpointConfig() : Invalid encoder name '%s' for endpoint '%s'", itr->second->getEncoder(), itr->first.c_str());
                        validationErrors.getErrorMessages().push_back().set(msg.c_str());
                    }

                    if (DecoderFactory::getDecoderType(itr->second->getDecoder()) == Decoder::INVALID)
                    {
                        FixedString64 msg(FixedString64::CtorSprintf(), "[Endpoint].validateEndpointConfig() : Invalid decoder name '%s' for endpoint '%s'", itr->second->getDecoder(), itr->first.c_str());
                        validationErrors.getErrorMessages().push_back().set(msg.c_str());
                    }

                    if (itr->second->getRateLimiter()[0] != '\0')
                    {
                        RateConfig::RateLimitersMap::const_iterator rateLimiterIt = rateConfig.getRateLimiters().find(itr->second->getRateLimiter());
                        if (rateLimiterIt == rateConfig.getRateLimiters().end())
                        {
                            FixedString64 msg(FixedString64::CtorSprintf(), "[Endpoint].validateEndpointConfig() : Undefined RateLimiter %s for endpoint %s.", itr->second->getRateLimiter(), itr->first.c_str());
                            validationErrors.getErrorMessages().push_back().set(msg.c_str());
                        }
                    }

                    if (itr->second->getQueuedOutputData().getMax() < QUEUED_OUTPUT_DATA_MAX_MINIMUM)
                    {
                        BLAZE_WARN_LOG(Log::CONNECTION, "[Endpoint].validateEndpointConfig() : QueuedOutputData: max queued data "
                                "configured to be less than minimum (" << QUEUED_OUTPUT_DATA_MAX_MINIMUM << ").");
                    }

                    if (itr->second->getQueuedOutputData().getHighWatermark() > itr->second->getQueuedOutputData().getMax())
                    {
                        BLAZE_WARN_LOG(Log::CONNECTION, "[Endpoint].validateEndpointConfig() : QueuedOutputData: High watermark "
                                "larger than max data size");
                    }

                    if (itr->second->getQueuedOutputData().getLowWatermark() > itr->second->getQueuedOutputData().getHighWatermark())
                    {
                        BLAZE_WARN_LOG(Log::CONNECTION, "[Endpoint].validateEndpointConfig() : QueuedOutputData: Low watermark "
                                "larger than high watermark.");
                    }

                    if ((itr->second->getInactivityTimeout().getMillis() != 0) &&
                        (itr->second->getInactivityTimeout().getMillis() < Blaze::NETWORK_CONNECTIONS_RECOMMENDED_MINIMUM_TIMEOUT_MS))
                    {
                        BLAZE_WARN_LOG(Log::CONNECTION, "[Endpoint].validateEndpointConfig() : InactivityTimeout: configured as value '" << itr->second->getInactivityTimeout().getMillis() <<
                            "' is less than the recommended minimum of " << Blaze::NETWORK_CONNECTIONS_RECOMMENDED_MINIMUM_TIMEOUT_MS << " ms.");
                    }

                    validateSslContext(*(itr->second), itr->first.c_str(), validationErrors);
                    if (validPlatformSpecificControlLists)
                        RpcAuthorizer::validateRpcControlLists(config, itr->first.c_str(), (*itr->second).getRpcWhiteList(), (*itr->second).getRpcBlackList(), controlListMap, controlListComboMap, validationErrors);
                }
            }
        }

        const auto& grpcEndpointNames = group->second->getGrpcEndpoints();
        // We allow for no gRPC endpoints so just traverse the list and validate each entry if available.
        for (const auto& name : grpcEndpointNames)
        {
            const auto itr = config.getGrpcEndpointTypes().find(name);
            if (itr == config.getGrpcEndpointTypes().end())
            {
                BLAZE_WARN_LOG(Log::CONNECTION, "[Endpoint].validateEndpointConfig() : Undefined endpoint " << name.c_str()
                    << " specified in endpoint group " << endpointGroup << ", skipping.");
            }
            else
            {
                if (itr->second->getRateLimiter()[0] != '\0')
                {
                    RateConfig::RateLimitersMap::const_iterator rateLimiterIt = rateConfig.getRateLimiters().find(itr->second->getRateLimiter());
                    if (rateLimiterIt == rateConfig.getRateLimiters().end())
                    {
                        FixedString64 msg(FixedString64::CtorSprintf(), "[Endpoint].validateEndpointConfig() : Undefined RateLimiter %s for endpoint %s.", itr->second->getRateLimiter(), itr->first.c_str());
                        validationErrors.getErrorMessages().push_back().set(msg.c_str());
                    }
                }

                validateSslContext(*(itr->second), itr->first.c_str(), validationErrors);
                if (validPlatformSpecificControlLists)
                    RpcAuthorizer::validateRpcControlLists(config, itr->first.c_str(), (*itr->second).getRpcWhiteList(), (*itr->second).getRpcBlackList(), controlListMap, controlListComboMap, validationErrors);
            }
        }
    }
}

void Endpoint::reconfigure(const EndpointConfig::Endpoint& newConfig, const RateConfig& newRateConfig, const EndpointConfig::CommandListByNameMap& rpcControlLists, const EndpointConfig::PlatformSpecificControlListsMap& platformSpecificControlLists)
{
    mConfig.setSerializeRequests(newConfig.getSerializeRequests());
    mConfig.setInactivityTimeout(newConfig.getInactivityTimeout());
    mConfig.setMaxFrameSize(newConfig.getMaxFrameSize());
    mConfig.setCommandTimeout(newConfig.getCommandTimeout());
    mConfig.setMaxConnections(newConfig.getMaxConnections());
    mConfig.setAllowResumeConnection(newConfig.getAllowResumeConnection());
    mConfig.setSslContext(newConfig.getSslContext());

    InetFilter* newFilter = BLAZE_NEW InetFilter();
    if (!newFilter->initialize(true /*allowIfEmpty*/, newConfig.getTrust()))
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[Endpoint].reconfigure: invalid trust configuration for endpoint '" << getName() << "'");

        //Kill the new one
        delete newFilter;
    }
    else
    {
        //config worked, use the new one
        delete mTrustedNetworks;
        mTrustedNetworks = newFilter;
    }

    newFilter = BLAZE_NEW InetFilter();
    if (!newFilter->initialize(false /*allowIfEmpty*/, newConfig.getHasInternalNicAccess()))
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[Endpoint].reconfigure: invalid hasInternalNicAccess configuration for endpoint '" << getName() << "'");

        //Kill the new one
        delete newFilter;
    }
    else
    {
        //config worked, use the new one
        delete mInternalNicAccessNetworks;
        mInternalNicAccessNetworks = newFilter;
    }

    if (newConfig.getRateLimiter()[0] != '\0')
    {
        // RateLimiter configuration
        RateLimiter newRateLimiter;

        RateConfig::RateLimitersMap::const_iterator rateLimiterIt = newRateConfig.getRateLimiters().find(newConfig.getRateLimiter());
        if (newRateLimiter.initialize(rateLimiterIt->first.c_str(), *rateLimiterIt->second))
        {
            mRateLimiter.initialize(rateLimiterIt->first.c_str(), *rateLimiterIt->second);
        }
        else
        {
            BLAZE_ERR_LOG(Log::CONNECTION, "[Endpoint].initialize: Failed to initialize RateLimiter '" << newConfig.getRateLimiter() << "' "
                "for endpoint '" << getName() << "'");
        }
    }
    else
    {
        mRateLimiter.finalize();
    }

    newConfig.getQueuedOutputData().copyInto(mConfig.getQueuedOutputData());
    configureQueuedOutputData(mConfig.getQueuedOutputData());
    mRpcAuthorizer.initializeRpcAuthorizationList(newConfig.getRpcWhiteList(), newConfig.getRpcBlackList(), rpcControlLists, platformSpecificControlLists);
}

void Endpoint::configureQueuedOutputData(const EndpointConfig::OutputQueueConfig& config)
{
    mSquelchingEnabled = true;
    if (mConfig.getQueuedOutputData().getMax() < QUEUED_OUTPUT_DATA_MAX_MINIMUM)
    {
        BLAZE_WARN_LOG(Log::CONNECTION, "[Endpoint].configureQueuedOutputData: max queued data "
                "configured to be less than minimum (" << QUEUED_OUTPUT_DATA_MAX_MINIMUM << "); setting to minimum instead.");
        mConfig.getQueuedOutputData().setMax(2048);
    }

    if (mConfig.getQueuedOutputData().getHighWatermark() > mConfig.getQueuedOutputData().getMax())
    {
        BLAZE_WARN_LOG(Log::CONNECTION, "[Endpoint].configureQueuedOutputData: High watermark larger than max data size; setting high watermark to max data size");
        mConfig.getQueuedOutputData().setHighWatermark(mConfig.getQueuedOutputData().getMax());
    }

    if (mConfig.getQueuedOutputData().getLowWatermark() > mConfig.getQueuedOutputData().getHighWatermark())
    {
        BLAZE_WARN_LOG(Log::CONNECTION, "[Endpoint].configureQueuedOutputData: Low watermark larger than high watermark; setting low watermark to half of high watermark");
        mConfig.getQueuedOutputData().setLowWatermark(mConfig.getQueuedOutputData().getHighWatermark() / 2);
    }
}

uint16_t Endpoint::getPort(InetAddress::Order order) const
{
    if (mServerAddress == nullptr)
        return 0;
    return mServerAddress->getPort(order);
}

Endpoint* Endpoint::createFromConfig(const char8_t* name, const EndpointConfig::Endpoint& config, const RateConfig& rateConfig, const EndpointConfig::CommandListByNameMap& rpcControlLists, const EndpointConfig::PlatformSpecificControlListsMap& platformSpecificControlLists)
{
    Endpoint* endpoint = BLAZE_NEW Endpoint(name);
    if (!endpoint->initialize(config, rateConfig, rpcControlLists, platformSpecificControlLists))
    {
        delete endpoint;
        return nullptr;
    }

    return endpoint;
}

bool Endpoint::assignPort(uint16_t port)
{
    mServerAddress = BLAZE_NEW InetAddress(Blaze::getInetAddress(getBindType(), port));
    
    switch (getBindType())
    {
    case Blaze::BIND_ALL:
        BLAZE_INFO_LOG(Log::CONNECTION, "[Endpoint].initialize: Opening port (" << port << ") with bind type (" << BindTypeToString(getBindType()) << ") for endpoint (" << getName() << "). Interface(INADDR_ANY).");
        break;

    case Blaze::BIND_INTERNAL:
        BLAZE_INFO_LOG(Log::CONNECTION, "[Endpoint].initialize: Opening port (" << port << ") with bind type (" << BindTypeToString(getBindType()) << ") for endpoint (" << getName() << "). Interface(" << Blaze::getInetAddress(BIND_INTERNAL, port).getIpAsString() <<").");
        break;
    
    case Blaze::BIND_EXTERNAL:
        BLAZE_INFO_LOG(Log::CONNECTION, "[Endpoint].initialize: Opening port (" << port << ") with bind type (" << BindTypeToString(getBindType()) << ") for endpoint (" << getName() << "). Interface(" << Blaze::getInetAddress(BIND_EXTERNAL, port).getIpAsString() << ").");
    
    default:
        return false;
    }

    // ChannelHandler will be set later by the connection manager
    mChannel = ServerChannelFactory::create(*this, *mServerAddress);
    return (mChannel != nullptr);
}

const char8_t* Endpoint::getCommonName() const
{
    return mChannel ? mChannel->getCommonName() : nullptr;
}

SslContextPtr Endpoint::getSslContext() const
{
    return gSslContextManager->get(mConfig.getSslContext());
}

template<typename EndpointConfigT>
void Endpoint::validateSslContext(const EndpointConfigT& config, const char8_t* endpointType, ConfigureValidationErrors& validationErrors)
{
    if (blaze_stricmp(config.getChannel(), "ssl") == 0)
    {
        // The ssl context assigned to this endpoint must be in the map of ssl contexts,
        // or the map of pending ssl contexts if we've just initialized new ssl contexts.
        // (The pending map will replace the current map if configuration is successful.)
        if ( !gSslContextManager->verifyContextExists(config.getSslContext()) )
        {
            FixedString64 msg(FixedString64::CtorSprintf(), "[Endpoint].validateEndpointConfig() : Invalid ssl context '%s' for endpoint '%s'", config.getSslContext(), endpointType);
            validationErrors.getErrorMessages().push_back().set(msg.c_str());
        }
    }
}

bool Endpoint::getInternalCommandsAllowed() const
{
    return (getBindType() == BIND_INTERNAL);
}

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

} // namespace Blaze

