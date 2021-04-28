
#include "NisQos.h"

namespace NonInteractiveSamples
{

NisQos::NisQos(NisBase &nisBase)
    : NisSample(nisBase)
{
}

NisQos::~NisQos()
{
}

void NisQos::onCreateApis()
{
    mConnectionManager = getBlazeHub()->getConnectionManager();
    getUiDriver().addDiagnosticCallFunc(mConnectionManager->addListener(this));
}

void NisQos::onCleanup()
{
    getUiDriver().addDiagnosticCallFunc(mConnectionManager->removeListener(this));
}

void NisQos::onRun()
{
}

//------------------------------------------------------------------------------
// QOS data is ready
void NisQos::onQosPingSiteLatencyRetrieved(bool success)
{
    getUiDriver().addDiagnosticFunction();

    LogQosData();
}

//------------------------------------------------------------------------------
void NisQos::LogQosData(void)
{
    getUiDriver().addDiagnosticFunction();

    const Blaze::Util::NetworkQosData * qosData = mConnectionManager->getNetworkQosData();
    if (qosData != nullptr)
    {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  NetworkQosData:");
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "    getNatType                 = %s", Blaze::Util::NatTypeToString(qosData->getNatType()));
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "    getUpstreamBitsPerSecond   = %d", qosData->getUpstreamBitsPerSecond());
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "    getDownstreamBitsPerSecond = %d", qosData->getDownstreamBitsPerSecond());
    }

    const Blaze::PingSiteLatencyByAliasMap * pingLatencyMap = mConnectionManager->getQosPingSitesLatency();
    if (pingLatencyMap != nullptr)
    {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  PingSiteLatencyByAliasMap:");

        Blaze::PingSiteLatencyByAliasMap::const_iterator iter = pingLatencyMap->begin();
        Blaze::PingSiteLatencyByAliasMap::const_iterator itend = pingLatencyMap->end();

        for (; iter != itend; ++iter)
        {
            const char * alias = iter->first.c_str();
            int32_t latency = iter->second;
            if (latency == (int32_t) Blaze::MAX_QOS_LATENCY)
            {
                getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "    alias: %s, latency: unknown", alias);
            }
            else
            {
                getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "    alias: %s, latency: %d", alias, latency);
            }
        }
    }

    done();
}

}

