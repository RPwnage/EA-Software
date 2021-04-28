
#include "NisSmallUserStorage.h"


using namespace Blaze::Util;

namespace NonInteractiveSamples
{

NisSmallUserStorage::NisSmallUserStorage(NisBase &nisBase)
    : NisSample(nisBase)
{
}

NisSmallUserStorage::~NisSmallUserStorage()
{
}

void NisSmallUserStorage::onCreateApis()
{
}

void NisSmallUserStorage::onCleanup()
{
}

void NisSmallUserStorage::onRun()
{
    getUiDriver().addDiagnosticFunction();

    // Save a (key, data) pair into small user storage

    UserSettingsSaveRequest request;
    request.setKey("testKey");
    request.setData("testData");

    UtilComponent * component = getBlazeHub()->getComponentManager(0)->getUtilComponent();
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "userSettingsSave (%s, %s)", request.getKey(), request.getData());
    component->userSettingsSave(request, UtilComponent::UserSettingsSaveCb(this, &NisSmallUserStorage::UserSettingsSaveCb)); 
}

//------------------------------------------------------------------------------
// The result of userSettingsSave. If it succeeded, we'll read it back out via userSettingsLoad.
void NisSmallUserStorage::UserSettingsSaveCb(Blaze::BlazeError error, Blaze::JobId jobId)
{
    getUiDriver().addDiagnosticCallback();

    if (error == Blaze::ERR_OK)
    {
        // The save worked. Now read it back out.

        UserSettingsLoadRequest request;
        request.setKey("testKey");

        UtilComponent * component = getBlazeHub()->getComponentManager(0)->getUtilComponent();
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "userSettingsLoad (%s)", request.getKey());
        component->userSettingsLoad(request, UtilComponent::UserSettingsLoadCb(this, &NisSmallUserStorage::UserSettingsLoadCb));
    }
    else
    {
        reportBlazeError(error);
    }
}

//------------------------------------------------------------------------------
// The result of the userSettingsLoad call; if successful, the loaded data is provided in the response object.
void NisSmallUserStorage::UserSettingsLoadCb(const UserSettingsResponse * response, Blaze::BlazeError error, Blaze::JobId jobId)
{
    getUiDriver().addDiagnosticCallback();

    if (error == Blaze::ERR_OK)
    {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "Loaded (%s, %s)", response->getKey(), response->getData());
        done();
    }
    else
    {
        reportBlazeError(error);
    }
}

}

