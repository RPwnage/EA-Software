
#include "NisServerConfig.h"

using namespace Blaze::Util;


namespace NonInteractiveSamples
{

NisServerConfig::NisServerConfig(NisBase &nisBase)
    : NisSample(nisBase)
{
}

NisServerConfig::~NisServerConfig()
{
}

void NisServerConfig::onCreateApis()
{
}

void NisServerConfig::onCleanup()
{
}

void NisServerConfig::onRun()
{
    getUiDriver().addDiagnosticFunction();

    // Fetch a value from util.cfg on the Blaze Server; we'll fetch qosServicePort from the BlazeSDK section
    //
    // You could fetch your own data from util.cfg, with a section like this:
    //  GameSettings = {
    //      Monster1Health = 500
    //      Monster2Health = 300
    //  }
    //
    // You could also put your settings into a file at \etc, like this:
    //  GameSettings = {
    //      #include "game_settings.cfg"
    //  }
    //

    const char8_t * CONFIG_SECTION = "BlazeSDK";

    FetchClientConfigRequest request;
    request.setConfigSection(CONFIG_SECTION);

    UtilComponent * component = getBlazeHub()->getComponentManager(0)->getUtilComponent();
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "fetchConfig for section \"%s\"", CONFIG_SECTION);
    component->fetchClientConfig(request, UtilComponent::FetchClientConfigCb(this, &NisServerConfig::FetchClientConfigCb));
}

//------------------------------------------------------------------------------
// The result of the fetchConfig call; this includes the keys for the requested section; we'll dig out the item that interest us.
void NisServerConfig::FetchClientConfigCb(const FetchConfigResponse * response, Blaze::BlazeError error, Blaze::JobId jobId)
{
    getUiDriver().addDiagnosticCallback();

    if (error == Blaze::ERR_OK)
    {
        const char8_t * KEY = "pingPeriod";//SAMPLES_TODO - use own NIS custom key

        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "find for key \"%s\"", KEY);
        FetchConfigResponse::ConfigMap::const_iterator find = response->getConfig().find(KEY);

        if (find == response->getConfig().end())
        {
            getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "not found");
        }
        else
        {
            getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "found: value = %s", find->second.c_str());
        }

        done();
    }
    else
    {
        reportBlazeError(error);
    }
}

}
